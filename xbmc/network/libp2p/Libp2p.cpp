/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Libp2p.h"

#include "crypto/Key.h" //! @todo Remove me
#include "crypto/did/IPID.h" //! @todo Remove me
#include "datastore/BlockStore.h" //! @todo Remove me
#include "datastore/DataStore.h" //! @todo Remove me
#include "datastore/IDataStore.h" //! @todo Remove me
#include "filesystem/File.h" //! @todo Remove me
#include "filesystem/IPFS.h" //! @todo Remove me
#include "filesystem/SpecialProtocol.h" //! @todo Remove me
#include "utils/URIUtils.h" //! @todo Remove me
#include "utils/log.h"

#include <string>

#include <boost/asio/io_context.hpp>
#include <gsl/multi_span>
#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/protocol/echo.hpp>
#include <libp2p/security/noise.hpp>
#include <libp2p/security/plaintext.hpp>
#include <soralog/configurator.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logging_system.hpp>

using namespace KODI;
using namespace NETWORK;

namespace
{
// Logging parameters
const std::string loggerConfig(R"(
# ----------------
sinks:
  - name: console
    type: console
    color: false
groups:
  - name: main
    sink: console
    level: info
    children:
      - name: libp2p
# ----------------
)");

struct ServerContext
{
  std::shared_ptr<libp2p::Host> host;
  std::shared_ptr<boost::asio::io_context> ioContext;
};

ServerContext initSecureServer(const libp2p::crypto::KeyPair& keypair)
{
  auto injector = libp2p::injector::makeHostInjector(
      libp2p::injector::useKeyPair(keypair),
      libp2p::injector::useSecurityAdaptors<libp2p::security::Noise>());

  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  auto ioContext = injector.create<std::shared_ptr<boost::asio::io_context>>();

  return {std::move(host), std::move(ioContext)};
}

ServerContext initInsecureServer(const libp2p::crypto::KeyPair& keypair)
{
  auto injector = libp2p::injector::makeHostInjector(
      libp2p::injector::useKeyPair(keypair),
      libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());

  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  auto ioContext = injector.create<std::shared_ptr<boost::asio::io_context>>();

  return {std::move(host), std::move(ioContext)};
}
} // namespace

std::shared_ptr<soralog::LoggingSystem> CLibp2p::m_loggingSystem;

CLibp2p::CLibp2p() : CThread("libp2p")
{
  InitializeLogging();
}

CLibp2p::~CLibp2p()
{
  // Stop libp2p
  Stop(true);

  // Reset state
  m_ipfs.reset();
  m_ioContext.reset();
}

void CLibp2p::InitializeLogging()
{
  if (m_loggingSystem)
    return;

  // Prepare log system
  auto configurator =
      std::make_shared<libp2p::log::Configurator>(); // Original LibP2P logging config
  auto configuratorFromYaml =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::move(configurator), loggerConfig);
  m_loggingSystem = std::make_shared<soralog::LoggingSystem>(std::move(configuratorFromYaml));

  soralog::Configurator::Result result = m_loggingSystem->configure();

  if (not result.message.empty())
    CLog::Log(result.has_error ? LOGERROR : LOGINFO, "{}", result.message);

  if (!result.has_error)
  {
    libp2p::log::setLoggingSystem(m_loggingSystem);
    //! @todo
    //libp2p::log::setLevelOfGroup("main", soralog::Level::ERROR);
    libp2p::log::setLevelOfGroup("main", soralog::Level::TRACE);
  }
}

void CLibp2p::Start()
{
  //! @todo
  //std::string dataStoreRoot =
  //    CSpecialProtocol::TranslatePath(m_profileManager.GetDataStoreFolder());
  const std::string dataStoreRoot = URIUtils::AddFileToFolder(
      CSpecialProtocol::TranslatePath("special://masterprofile"), "Datastore");

  // Clear previous lock file if it exists
  const std::string ipfsStorePath = URIUtils::AddFileToFolder(dataStoreRoot, "ipfs");
  const std::string ipfsLockFile = URIUtils::AddFileToFolder(ipfsStorePath, "lock.mdb");
  if (XFILE::CFile::Exists(ipfsLockFile))
    XFILE::CFile::Delete(ipfsLockFile);

  m_ipfs = std::make_shared<XFILE::CIPFS>();
  if (m_ipfs->Initialize(dataStoreRoot))
  {
    CRYPTO::CIPID ipid(m_ipfs);

    ipid.Create(CRYPTO::PrivateKey{});
  }

  if (!IsRunning())
    Create(false);
}

bool CLibp2p::IsOnline() const
{
  //! @todo
  return IsRunning();
}

void CLibp2p::Stop(bool bWait)
{
  StopThread(false);

  if (m_ioContext)
    m_ioContext->stop();

  if (bWait)
  {
    // Wait for thread to end
    StopThread(true);

    // Reset IO context
    m_ioContext.reset();
  }
}

void CLibp2p::Process()
{
  // Resulting PeerId should be
  // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
  using libp2p::common::operator""_unhex;
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;
  const KeyPair keypair{PublicKey{{Key::Type::Ed25519, "48453469c62f4885373099421a7365520b5ffb"
                                                       "0d93726c124166be4b81d852e6"_unhex}},
                        PrivateKey{{Key::Type::Ed25519, "4a9361c525840f7086b893d584ebbe475b4ec"
                                                        "7069951d2e897e8bceb0a3f35ce"_unhex}}};

  const bool insecureMode = false;
  CLog::Log(LOGINFO, "Starting libp2p in {} mode", insecureMode ? "insecure" : "secure");

  // Create a default Host via an injector, overriding a random-generated
  // keypair with ours
  ServerContext server = insecureMode ? initInsecureServer(keypair) : initSecureServer(keypair);
  std::shared_ptr<libp2p::Host> host = std::move(server.host);
  m_ioContext = std::move(server.ioContext);

  // Set a handler for Echo protocol
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{
      .max_server_repeats = libp2p::protocol::EchoConfig::kInfiniteNumberOfRepeats,
      .max_recv_size = libp2p::muxer::MuxedConnectionConfig::kDefaultMaxWindowSize}};
  host->setProtocolHandler(echo.getProtocolId(),
                           [&echo](std::shared_ptr<libp2p::connection::Stream> received_stream) {
                             echo.handle(std::move(received_stream));
                           });

  // Launch a Listener part of the Host
  m_ioContext->post([host{std::move(host)}] {
    auto ma = libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40010").value();

    libp2p::outcome::result<void> listenResult = host->listen(ma);
    if (!listenResult)
    {
      CLog::Log(LOGERROR, "Host cannot listen the given multiaddress: {}",
                listenResult.error().message());
    }
    else
    {
      host->start();

      CLog::Log(LOGINFO, "Started libp2p server");
      CLog::Log(LOGINFO, "Listening on: {}", ma.getStringAddress());
      CLog::Log(LOGINFO, "Peer id: {}", host->getPeerInfo().id.toBase58());
      CLog::Log(LOGINFO, "Connection string: {}/ipfs/{}", ma.getStringAddress(),
                host->getPeerInfo().id.toBase58());
    }
  });

  // Run the IO context
  try
  {
    m_ioContext->run();
  }
  catch (const boost::system::error_code& ec)
  {
    CLog::Log(LOGERROR, "Server cannot run: {}", ec.message());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Unknown error happened");
  }
}
