/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameServices.h"
#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "games/GameSettings.h"
#include "profiles/ProfileManager.h"

// Echo client
#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/echo.hpp>

// Echo server
//#include <gsl/multi_span>
#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/echo.hpp>
#include <libp2p/security/plaintext.hpp>
#include <libp2p/security/secio.hpp>

// Kademlia factory
#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/kademlia/impl/routing_table_impl.hpp>

// Kademlia example
#include <libp2p/common/literals.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/kademlia/impl/kad_impl.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

// Async reader
#include <boost/asio.hpp>

// Gossip chat example
#include <boost/program_options.hpp>
#include <libp2p/injector/gossip_injector.hpp>
#include <spdlog/fmt/fmt.h>

// Gossip utility
#include <boost/asio/io_context.hpp>
#include <libp2p/peer/peer_info.hpp>

// Filecoin
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfileManager.h"
#include "utils/log.h"

//#include <storage/ipfs/datastore.hpp>
#include <filecoin/common/outcome.hpp>
#include <leveldb/options.h>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/hash_type.hpp>
#include <libp2p/multi/multicodec_type.hpp>
#include <libp2p/multi/multihash.hpp>
#include <filecoin/storage/repository/impl/filesystem_repository.hpp>
#include <filecoin/crypto/blake2/blake2b160.hpp>

#include <memory>

using namespace KODI;
using namespace GAME;

#define MULTIBASE_IDENTITY  "identity"  // Value 0x00

/*!
 * \brief Convert a hex string to a byte array
 */
inline std::vector<uint8_t> operator""_unhex(const char *c, size_t s)
{
  return fc::common::unhex(std::string_view(c, s)).value();
}

/*!
 * \brief Get the CID of a raw chunk of data
 */
fc::outcome::result<fc::CID> getCidOf(gsl::span<const uint8_t> bytes)
{
  using fc::CID;
  using libp2p::common::Hash256;
  using libp2p::multi::HashType;
  using libp2p::multi::MulticodecType;
  using libp2p::multi::Multihash;

  Hash256 hash_raw = libp2p::crypto::sha256(bytes);
  fc::outcome::result<Multihash> hashResult = Multihash::create(HashType::sha256, hash_raw);

  if (hashResult)
    return CID(CID::Version::V1, MulticodecType::RAW, hashResult.value());
  else
    return hashResult.error();
}

void RunFilecoinTest(const std::string& dataStorePath)
{
  using fc::CID;
  using fc::common::Buffer;
  using fc::outcome::result;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::repository::FileSystemRepository;
  using fc::storage::repository::Repository;
  using libp2p::multi::HashType;
  using libp2p::multi::MulticodecType;
  using libp2p::multi::Multihash;

  // Ensure data store path exists exists
  if (!XFILE::CDirectory::Exists(dataStorePath))
  {
    CLog::Log(LOGDEBUG, "Creating data store path: {}", dataStorePath);
    XFILE::CDirectory::Create(dataStorePath);
  }

  std::string apiAddress; // TODO
  leveldb::Options leveldbOptions{};
  leveldbOptions.create_if_missing = true;

  result<std::shared_ptr<Repository>> repoResult = FileSystemRepository::create(dataStorePath, apiAddress, leveldbOptions);
  if (!repoResult)
  {
    CLog::Log(LOGERROR, "Failed to open data store: {}", repoResult.error().message());
    return;
  }

  std::shared_ptr<Repository> repo = repoResult.value();
  result<unsigned int> versionResult = repo->getVersion();
  if (!versionResult)
  {
    CLog::Log(LOGERROR, "Can't get data store version");
    return;
  }

  CLog::Log(LOGDEBUG, "Opened data store at version: {}", versionResult.value());

  std::shared_ptr<IpfsDatastore> datastore = repo->getIpldStore();

  CID cid1{
      CID::Version::V1,
      MulticodecType::SHA2_256,
      Multihash::create(HashType::sha256,
                        "0123456789ABCDEF0123456789ABCDEF"_unhex).value()
  };
  CID cid2{
      CID::Version::V1,
      MulticodecType::SHA2_256,
      Multihash::create(HashType::sha256,
                        "FEDCBA9876543210FEDCBA9876543210"_unhex).value()
  };

  Buffer value{"0123456789ABCDEF0123456789ABCDEF"_unhex};

  if (!datastore->set(cid1, value))
  {
    CLog::Log(LOGERROR, "Failed to set value in data store");
    return;
  }
  CLog::Log(LOGDEBUG, "Successfully set value in data store");

  result<Buffer> valueResult = datastore->get(cid1);
  if (!valueResult)
  {
    CLog::Log(LOGERROR, "Failed to get value from data store");
    return;
  }
  CLog::Log(LOGDEBUG, "Successfully retrieved value from data store");

  const Buffer& buffer = valueResult.value();

  if (buffer == value)
    CLog::Log(LOGDEBUG, "Values equal! Value: {}, Size: {}", buffer.toHex(), buffer.toVector().size());
  else
    CLog::Log(LOGERROR, "Values diverge!");

  std::vector<uint8_t> data{2, 3};

  result<CID> cidResult = getCidOf(data);
  if (!cidResult)
  {
    CLog::Log(LOGERROR, "Error encoding CID: {}", cidResult.error().message());
    return;
  }

  const CID& key = cidResult.value();

  CLog::Log(LOGERROR, "Encoded CID: {}", key.toPrettyString(MULTIBASE_IDENTITY));

  auto setResult = datastore->set(key, Buffer(std::move(data)));
  if (!setResult)
  {
    CLog::Log(LOGERROR, "Failed to store data: {}", setResult.error().message());
    return;
  }

  result<Buffer> getResult = datastore->get(key);
  if (!getResult)
  {
    CLog::Log(LOGERROR, "Failed to retrieve data: {}", getResult.error().message());
    return;
  }

  const auto& dataResult = getResult.value();

  CLog::Log(LOGDEBUG, "Data size: {}", dataResult.size());
  CLog::Log(LOGDEBUG, "Data: {}", dataResult[0]);
}

CGameServices::CGameServices(CControllerManager& controllerManager,
                             RETRO::CGUIGameRenderManager& renderManager,
                             PERIPHERALS::CPeripherals& peripheralManager,
                             const CProfileManager& profileManager,
                             ADDON::CAddonMgr& addons,
                             ADDON::CBinaryAddonManager& binaryAddons)
    : m_controllerManager(controllerManager)
    , m_gameRenderManager(renderManager)
    , m_profileManager(profileManager)
    , m_gameSettings(new CGameSettings())
    , m_videoShaders(new SHADER::CShaderPresetFactory(addons, binaryAddons))
{
  // Filecoin test
  const std::string dataStorePath = CSpecialProtocol::TranslatePath(profileManager.GetDataStoreFolder());
  RunFilecoinTest(dataStorePath);

  CLog::Log(LOGDEBUG, "Here1");
}

CGameServices::~CGameServices() = default;

ControllerPtr CGameServices::GetController(const std::string& controllerId)
{
  return m_controllerManager.GetController(controllerId);
}

ControllerPtr CGameServices::GetDefaultController()
{
  return m_controllerManager.GetDefaultController();
}

ControllerPtr CGameServices::GetDefaultKeyboard()
{
  return m_controllerManager.GetDefaultKeyboard();
}

ControllerPtr CGameServices::GetDefaultMouse()
{
  return m_controllerManager.GetDefaultMouse();
}

ControllerVector CGameServices::GetControllers()
{
  return m_controllerManager.GetControllers();
}

std::string CGameServices::GetSavestatesFolder() const
{
  return m_profileManager.GetSavestatesFolder();
}
