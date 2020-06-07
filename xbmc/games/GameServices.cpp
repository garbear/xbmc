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
#include "games/GameSettings.h"
#include "profiles/ProfileManager.h"

#ifdef HAS_LIBP2P
// Echo keys
#include <libp2p/crypto/key.hpp>

// Echo client
#include "utils/log.h"
//#include <boost/di.hpp>
#include <boost/asio/io_context.hpp>
#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/protocol/echo.hpp>

// Echo server
#include "threads/IRunnable.h"
#include "threads/Thread.h"
//#include <gsl/multi_span>
#include <string>

#include <libp2p/common/literals.hpp>
#include <libp2p/host/basic_host.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/echo.hpp>
#include <libp2p/security/plaintext.hpp>
#include <libp2p/security/secio.hpp>

const std::string LOCALHOST_MULTIADDRESS = "/ip4/127.0.0.1/tcp/40010/ipfs";

/*
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
*/

//#include <memory>
#endif // HAS_LIBP2P

using namespace KODI;
using namespace GAME;

#ifdef HAS_LIBP2P
#define MULTIBASE_IDENTITY "identity" // Value 0x00

/*!
 * \brief Convert a hex string to a byte array
 */
using libp2p::common::operator""_unhex;

/*!
 * \brief Echo server example
 */
class Libp2pServer : public IRunnable
{
public:
  ~Libp2pServer() override = default;

  void Run() override
  {
    using libp2p::crypto::Key;
    using libp2p::crypto::KeyPair;
    using libp2p::crypto::PrivateKey;
    using libp2p::crypto::PublicKey;

    // Resulting Peer ID should be
    // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
    KeyPair keypair{PublicKey{{Key::Type::Ed25519, "48453469c62f4885373099421a7365520b5ffb"
                                                   "0d93726c124166be4b81d852e6"_unhex}},
                    PrivateKey{{Key::Type::Ed25519, "4a9361c525840f7086b893d584ebbe475b4ec"
                                                    "7069951d2e897e8bceb0a3f35ce"_unhex}}};

    const bool insecure_mode = false; //! @todo
    if (insecure_mode)
      CLog::Log(LOGDEBUG, "Starting in insecure mode");
    else
      CLog::Log(LOGDEBUG, "Starting in secure mode");

    struct ServerContext
    {
      std::shared_ptr<libp2p::Host> host;
      std::shared_ptr<boost::asio::io_context> io_context;
    };

    auto initSecureServer = [](const libp2p::crypto::KeyPair& keypair) -> ServerContext {
      auto injector = libp2p::injector::makeHostInjector(
          libp2p::injector::useKeyPair(keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Secio>());
      auto host = injector.create<std::shared_ptr<libp2p::Host>>();
      auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
      return {host, context};
    };

    auto initInsecureServer = [](const libp2p::crypto::KeyPair& keypair) -> ServerContext {
      auto injector = libp2p::injector::makeHostInjector(
          libp2p::injector::useKeyPair(keypair),
          libp2p::injector::useSecurityAdaptors<libp2p::security::Plaintext>());
      auto host = injector.create<std::shared_ptr<libp2p::Host>>();
      auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
      return {host, context};
    };

    // Create a default Host via an injector, overriding a random-generated
    // keypair with ours
    ServerContext server = insecure_mode ? initInsecureServer(keypair) : initSecureServer(keypair);

    // Set a handler for Echo protocol
    libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{1}};
    server.host->setProtocolHandler(
        echo.getProtocolId(), [&echo](std::shared_ptr<libp2p::connection::Stream> received_stream) {
          echo.handle(std::move(received_stream));
        });

    // Launch a Listener part of the Host
    server.io_context->post([host{std::move(server.host)}] {
      // Create the multiaddress
      auto ma = libp2p::multi::Multiaddress::create(LOCALHOST_MULTIADDRESS).value();

      auto listen_res = host->listen(ma);
      if (!listen_res)
      {
        CLog::Log(LOGERROR, "Host cannot listen the given multiaddress: {}",
                  listen_res.error().message());
        return;
      }

      host->start();

      CLog::Log(LOGDEBUG, "Server started. Listening on: {}, Peer ID: {}", ma.getStringAddress(),
                host->getPeerInfo().id.toBase58());
      CLog::Log(LOGDEBUG, "Connection string: {}/ipfs/{}", ma.getStringAddress(),
                host->getPeerInfo().id.toBase58());
    });

    // Run the IO context
    try
    {
      server.io_context->run();
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
};

/*!
 * \brief Echo client example
 */
void RunClient()
{
  using libp2p::crypto::Key;
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;

  // create Echo protocol object - it implement the logic of both server and
  // client, but in this example it's used as a client-only
  libp2p::protocol::Echo echo{libp2p::protocol::EchoConfig{1}};

  // Create a default Host via an injector
  auto injector = libp2p::injector::makeHostInjector();
  std::shared_ptr<libp2p::Host> host = injector.create<std::shared_ptr<libp2p::Host>>();

  //! @todo
  std::string multiaddr =
      LOCALHOST_MULTIADDRESS + "/12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV";

  // Create IO context, which allows us to execute async operations
  std::shared_ptr<boost::asio::io_context> context =
      injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, &echo, &multiaddr] {
    auto server_ma_res = libp2p::multi::Multiaddress::create(multiaddr);
    if (!server_ma_res)
    {
      CLog::Log(LOGERROR, "Unable to create server multiaddress: {}",
                server_ma_res.error().message());
      return;
    }

    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str)
    {
      CLog::Log(LOGERROR, "Unable to get peer id");
      return;
    }

    auto server_peer_id_res = libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res)
    {
      CLog::Log(LOGERROR, "Unable to decode peer id from base 58: {}",
                server_peer_id_res.error().message());
      return;
    }

    auto server_peer_id = std::move(server_peer_id_res.value());

    auto peer_info = libp2p::peer::PeerInfo{server_peer_id, {server_ma}};

    // Create Host object and open a stream through it
    host->newStream(peer_info, echo.getProtocolId(), [&echo](auto&& stream_res) {
      if (!stream_res)
      {
        CLog::Log(LOGERROR, "Cannot connect to server: {}", stream_res.error().message());
        return;
      }

      auto stream_p = std::move(stream_res.value());

      auto echo_client = echo.createClient(stream_p);

      CLog::Log(LOGDEBUG, "SENDING: Hello from C++!");

      echo_client->sendAnd("Hello from C++!\n",
                           [stream = std::move(stream_p)](auto&& response_result) {
                             CLog::Log(LOGDEBUG, "RESPONSE: {}", response_result.value());
                             stream->close([](auto&&) { CLog::Log(LOGDEBUG, "Stream closed"); });
                           });
    });
  });

  // Run the IO context
  try
  {
    context->run_for(std::chrono::seconds(5));
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

/*!
 * \brief Kademlia example
 */
/*
static const uint16_t BASE_PORT_NUMBER = 40000;
static const size_t HOST_COUNT = 6;

struct PerHostObjects
{
  std::shared_ptr<libp2p::Host> host;
  std::shared_ptr<libp2p::protocol::kademlia::RoutingTable> routing_table;
  std::shared_ptr<libp2p::crypto::CryptoProvider> key_gen;
  std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller;
};

boost::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string& str)
{
  auto server_ma_res = libp2p::multi::Multiaddress::create(str);
  if (!server_ma_res)
  {
    CLog::Log(LOGERROR, "Unable to create server multiaddress: {}",
               server_ma_res.error().message());
    return boost::none;
  }

  auto server_ma = std::move(server_ma_res.value());

  auto server_peer_id_str = server_ma.getPeerId();
  if (!server_peer_id_str)
  {
    CLog::Log(LOGERROR, "Unable to extract peer id from multiaddress");
    return boost::none;
  }

  auto server_peer_id_res = libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
  if (!server_peer_id_res)
  {
    CLog::Log(LOGERROR, "Unable to decode peer id from base 58: {}",
               server_peer_id_res.error().message());
    return boost::none;
  }

  return libp2p::peer::PeerInfo{server_peer_id_res.value(), {server_ma}};
}

template <typename... Ts>
auto makeInjector(Ts&& ... args)
{
  auto csprng = std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();
  auto ed25519_provider =
      std::make_shared<libp2p::crypto::ed25519::Ed25519ProviderImpl>();
  auto rsa_provider = std::make_shared<libp2p::crypto::rsa::RsaProviderImpl>();
  auto ecdsa_provider =
      std::make_shared<libp2p::crypto::ecdsa::EcdsaProviderImpl>();
  auto secp256k1_provider =
      std::make_shared<libp2p::crypto::secp256k1::Secp256k1ProviderImpl>();
  auto hmac_provider = std::make_shared<libp2p::crypto::hmac::HmacProviderImpl>();

  std::shared_ptr<libp2p::crypto::CryptoProvider> crypto_provider =
      std::make_shared<libp2p::crypto::CryptoProviderImpl>(
          csprng, ed25519_provider, rsa_provider, ecdsa_provider,
          secp256k1_provider, hmac_provider);
  auto validator = std::make_shared<libp2p::crypto::validator::KeyValidatorImpl>(
      crypto_provider);

  // Assume no error here. otherwise... just blow up executable
  auto keypair =
      crypto_provider->generateKeys(libp2p::crypto::Key::Type::Ed25519).value();

  return libp2p::injector::makeHostInjector<boost::di::extension::shared_config>(
    boost::di::bind<libp2p::crypto::CryptoProvider>().to(
        crypto_provider)[boost::di::override],
    boost::di::bind<libp2p::crypto::KeyPair>().template to(
        std::move(keypair))[boost::di::override],
    boost::di::bind<libp2p::crypto::random::CSPRNG>().template to(
        std::move(csprng))[boost::di::override],
    boost::di::bind<libp2p::crypto::marshaller::KeyMarshaller>().template to<
        libp2p::crypto::marshaller::KeyMarshallerImpl>()[boost::di::override],
    boost::di::bind<libp2p::crypto::validator::KeyValidator>().template to(
        std::move(validator))[boost::di::override],
    // Overrides
    std::forward<decltype(args)>(args)...
  );
}

void createPerHostObjects(PerHostObjects& objects, const libp2p::protocol::kademlia::KademliaConfig& conf, std::shared_ptr<boost::asio::io_context>& io)
{
  auto injector = makeInjector(
    boost::di::bind<boost::asio::io_context>.to(io)[boost::di::override]
  );

  objects.host = injector.create<std::shared_ptr<libp2p::Host>>();

  objects.key_gen =
      injector.create<std::shared_ptr<libp2p::crypto::CryptoProvider>>();
  objects.key_marshaller = injector.create<
      std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller>>();
  objects.routing_table = std::make_shared<libp2p::protocol::kademlia::RoutingTableImpl>(
      injector.create<std::shared_ptr<libp2p::peer::IdentityManager>>(),
      injector.create<std::shared_ptr<libp2p::event::Bus>>(), conf);
}

libp2p::peer::PeerId genRandomPeerId(libp2p::crypto::CryptoProvider& gen,
    libp2p::crypto::marshaller::KeyMarshaller& marshaller)
{
  auto keypair = gen.generateKeys(libp2p::crypto::Key::Type::Ed25519).value();

  return libp2p::peer::PeerId::fromPublicKey(
    marshaller.marshal(keypair.publicKey).value()
  ).value();
}

const libp2p::protocol::kademlia::KademliaConfig& getConfig()
{
  static libp2p::protocol::kademlia::KademliaConfig config = (
    []
    {
      libp2p::protocol::kademlia::KademliaConfig c;
      c.randomWalk.delay = std::chrono::seconds(5);
      return c;
    }
  )();

  return config;
}

class Hosts
{
public:
  struct Host
  {
    size_t index = 0;
    PerHostObjects m_obj;
    std::shared_ptr<libp2p::protocol::kademlia::KadImpl> kad;
    std::string listen_to;
    std::string connect_to;
    boost::optional<libp2p::peer::PeerId> find_id;
    libp2p::protocol::Scheduler::Handle htimer;
    libp2p::protocol::Scheduler::Handle hbootstrap;
    bool found = false;
    bool verbose = true;
    bool request_sent = false;

    Host(size_t i, const std::shared_ptr<libp2p::protocol::Scheduler>& scheduler, PerHostObjects obj) :
      index(i),
      m_obj(std::move(obj))
    {
      kad = std::make_shared<libp2p::protocol::kademlia::KadImpl>(
        m_obj.host,
        scheduler,
        m_obj.routing_table,
        libp2p::protocol::kademlia::createDefaultValueStoreBackend(),
        getConfig()
      );
    }

    void checkPeers()
    {
      auto peers = m_obj.host->getPeerRepository().getPeers();

      CLog::Log(LOGINFO, "Host {}: peers in repo: {}, found: {}", index,
                peers.size(), found);
    }

    void listen(std::shared_ptr<boost::asio::io_context> &io)
    {
      if (listen_to.empty())
        return;

      io->post([this] { onListen(); });
    }

    void onListen()
    {
      auto multiaddress = libp2p::multi::Multiaddress::create(listen_to).value();
      auto listen_res = m_obj.host->listen(multiaddress);
      if (!listen_res)
      {
        CLog::Log(LOGERROR, "Server {} cannot listen the given multiaddress: {}",
                  index, listen_res.error().message());
      }

      m_obj.host->start();
      CLog::Log(LOGINFO, "Server {} listening to: {} peerId={}", index,
                   multiaddress.getStringAddress(),
                   m_obj.host->getPeerInfo().id.toBase58());

      kad->start(true);
    }

    void connect()
    {
      if (connect_to.empty())
        return;

      auto peerInfo = str2peerInfo(connect_to);
      kad->addPeer(std::move(peerInfo.value()), true);
    }

    void findPeer(const libp2p::peer::PeerId& id)
    {
      find_id = id;
      htimer = kad->scheduler().schedule(20000, [this] { onFindPeerTimer(); });
      hbootstrap = kad->scheduler().schedule(100, [this] { onBootstrapTimer(); });
    }

    void onBootstrapTimer()
    {
      hbootstrap.reschedule(2000);

      if (!request_sent)
      {
        request_sent = kad->findPeer(
          genRandomPeerId(*m_obj.key_gen, *m_obj.key_marshaller),
          [this](const libp2p::peer::PeerId &peer, libp2p::protocol::kademlia::Kad::FindPeerQueryResult res)
          {
            CLog::Log(LOGINFO, "Bootstrap return from findPeer, i={}, peer={} peers={} ({})",
                index, peer.toBase58(), res.closer_peers.size(), res.success);
            request_sent = false;
          }
        );

        CLog::Log(LOGINFO, "Bootstrap sent request, i={}, request_sent={}",
            index, request_sent);
      }
      else
      {
        CLog::Log(LOGINFO, "Bootstrap waiting for result, i={}", index);
      }

      m_obj.host->getNetwork().getConnectionManager().collectGarbage();
    }

    void onFindPeerTimer()
    {
      CLog::Log(LOGINFO, "Find peer timer callback, i={}", index);

      if (!found)
      {
        kad->findPeer(
          find_id.value(),
          [this](const libp2p::peer::PeerId& peer, libp2p::protocol::kademlia::Kad::FindPeerQueryResult res)
          {
            onFindPeer(peer, std::move(res));
          }
        );
      }
    }

    void onFindPeer(const libp2p::peer::PeerId& peer, libp2p::protocol::kademlia::Kad::FindPeerQueryResult res)
    {
      if (found)
        return;

      CLog::Log(LOGINFO, "onFindPeer: i={}, res: success={}, peers={}", index,
          res.success, res.closer_peers.size());

      if (res.success)
      {
        found = true;
        return;
      }

      htimer = kad->scheduler().schedule(
        1000,
        [this, peers = std::move(res.closer_peers)]
        {
          kad->findPeer(
            find_id.value(),
            [this](const libp2p::peer::PeerId& peer, libp2p::protocol::kademlia::Kad::FindPeerQueryResult res)
            {
              onFindPeer(peer, std::move(res));
            }
          );

          if (!found && !peers.empty())
          {
            kad->findPeer(
              find_id.value(),
              peers,
              [this](const libp2p::peer::PeerId& peer, libp2p::protocol::kademlia::Kad::FindPeerQueryResult res)
              {
                onFindPeer(peer, std::move(res));
              }
            );
          }
        }
      );
    }
  };

  Hosts(size_t count, const std::shared_ptr<libp2p::protocol::Scheduler>& sch, std::shared_ptr<boost::asio::io_context>& io)
  {
    m_hosts.reserve(count);

    for (size_t i = 0; i < count; ++i)
      newHost(sch, io);

    makeConnectTopologyCircle();
  }

  void newHost(const std::shared_ptr<libp2p::protocol::Scheduler>& sch, std::shared_ptr<boost::asio::io_context>& io)
  {
    size_t index = m_hosts.size();

    PerHostObjects obj;
    createPerHostObjects(obj, getConfig(), io);
    assert(obj.host && obj.routing_table);
    m_hosts.emplace_back(index, sch, std::move(obj));
  }

  void makeConnectTopologyCircle()
  {
    for (auto& host : m_hosts)
      host.listen_to = std::string("/ip4/127.0.0.1/tcp/") + std::to_string(BASE_PORT_NUMBER + host.index);

    for (auto& host : m_hosts)
    {
      Host& server = m_hosts[(host.index > 0) ? host.index - 1 : m_hosts.size() - 1];

      if (!server.listen_to.empty())
        host.connect_to = server.listen_to + "/ipfs/" + server.m_obj.host->getId().toBase58();
    }
  }

  void listen(std::shared_ptr<boost::asio::io_context>& io)
  {
    for (auto& host : m_hosts)
      host.listen(io);
  }

  void connect()
  {
    for (auto& host : m_hosts)
      host.connect();
  }

  void findPeers()
  {
    for (auto& host : m_hosts)
    {
      auto half = m_hosts.size() / 2;

      auto index = host.index;
      if (index > half)
        index -= half;
      else
        index += (half - 1);

      Host &server = m_hosts[index];
      host.findPeer(server.m_obj.host->getId());
    }
  }

  void checkPeers()
  {
    for (auto& host : m_hosts)
      host.checkPeers();

    m_hosts.clear();
  }

private:
  std::vector<Host> m_hosts;
};

void RunKademlia()
{
  try
  {
    std::shared_ptr<boost::asio::io_context> io = std::make_shared<boost::asio::io_context>();

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler = std::make_shared<libp2p::protocol::AsioScheduler>(
      *io,
      libp2p::protocol::SchedulerConfig{}
    );

    Hosts hosts(HOST_COUNT, scheduler, io);

    hosts.listen(io);
    hosts.connect();
    hosts.findPeers();

    boost::asio::signal_set signals(*io, SIGINT, SIGTERM);

    signals.async_wait(
      [&io](const boost::system::error_code&, int)
      {
        io->stop();
      }
    );

    io->run_for(std::chrono::seconds(5));

    hosts.checkPeers();
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "Exception: {}", e.what());
  }
}
*/

/*!
 * \brief Gossip example
 */
/*
const uint16_t PORT = 39999; //! @todo
const std::string TOPIC = "test"; //! @todo

std::string bufferToString(const std::vector<uint8_t>& buf)
{
  return std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
}

std::vector<uint8_t> stringToBuffer(const std::string& str)
{
  std::vector<uint8_t> ret;

  if (!str.empty())
  {
    ret.reserve(str.size());
    ret.assign(str.begin(), str.end());
  }

  return ret;
}

std::string formatPeerId(const std::vector<uint8_t>& bytes)
{
  auto res = libp2p::peer::PeerId::fromBytes(bytes);

  if (res)
  {
    libp2p::peer::PeerId peerId = std::move(res.value());
    return peerId.toBase58();
  }

  return "???";
}

std::string getLocalIP(boost::asio::io_context& io)
{
  std::string address("127.0.0.1");

  boost::asio::ip::tcp::resolver resolver(io);
  std::string hostname = boost::asio::ip::host_name();
  boost::asio::ip::tcp::resolver::query query(hostname, "");

  try
  {
    boost::asio::ip::tcp::resolver::iterator it;
    boost::asio::ip::tcp::resolver::iterator end;
    for (it = resolver.resolve(query); it != end; ++it)
    {
      auto endpoint = it->endpoint();
      if (endpoint.address().is_v4())
      {
        address = endpoint.address().to_string();
        break;
      }
    }
  }
  catch (const boost::system::system_error& error)
  {
    CLog::Log(LOGERROR, "Failed to resolve hostname: {}", error.what());
  }

  return address;
}

void RunGossip()
{
  using libp2p::protocol::gossip::Gossip;

  // Overriding default config to see local messages as well (echo mode)
  libp2p::protocol::gossip::Config config;
  config.echo_forward_mode = true;

  // Injector creates and ties the dependent objects
  auto injector = libp2p::injector::makeGossipInjector(
    libp2p::injector::useGossipConfig(config)
  );

  // Create asio context
  std::shared_ptr<boost::asio::io_context> io = std::make_shared<boost::asio::io_context>();

  // Host is our local libp2p node
  std::shared_ptr<libp2p::Host> host = injector.create<std::shared_ptr<libp2p::Host>>();

  // Make peer URI of local node
  std::string localAddress = "/ip4/" + getLocalIP(*io) + "/tcp/" + std::to_string(PORT) + "/p2p/" + host->getId().toBase58();

  // Local address -> peer info
  auto peerInfo = str2peerInfo(localAddress);
  if (!peerInfo)
  {
    CLog::Log(LOGERROR, "Cannot resolve local peer from {}", localAddress);
    return;
  }

  CLog::Log(LOGINFO, "Local address: {}", localAddress);

  // Create gossip node
  std::shared_ptr<Gossip> gossip = injector.create<std::shared_ptr<Gossip>>();

  // Subscribe to chat topic, print messages to the log
  auto subscription = gossip->subscribe(
    { TOPIC },
    [](boost::optional<const Gossip::Message&> message)
    {
      if (!message)
      {
        // Message with no value means EOS, this occurs when the node has
        // stopped
        return;
      }

      CLog::Log(LOGDEBUG, "{}: {}", formatPeerId(message->from),
          bufferToString(message->data));
    }
  );

  // Tell gossip to connect to remote peer, if specified
  //gossip->addBootstrapPeer(REMOTE_PEER_ID, REMOTE_PEER_ADDRESSES[0]);

  // Start the node as soon as async engine starts
  io->post(
    [&]
    {
      auto listenResult = host->listen(peerInfo->addresses[0]);
      if (!listenResult)
      {
        CLog::Log(LOGERROR, "Cannot listen to multiaddress {}: {}",
            peerInfo->addresses[0].getStringAddress(),
            listenResult.error().message());

        io->stop();
        return;
      }

      CLog::Log(LOGDEBUG, "Listening to multiaddress {}",
          peerInfo->addresses[0].getStringAddress());

      host->start();
      gossip->start();

      CLog::Log(LOGDEBUG, "Node started");
    }
  );

  // Gracefully shutdown on signal
  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
    [&io](const boost::system::error_code&, int)
    {
      io->stop();
    }
  );

  // Run event loop
  io->run_for(std::chrono::seconds(1));

  // Read lines from stdin in async manner and publish them into the chat
  gossip->publish({ TOPIC }, stringToBuffer("Hello world via gossip!"));

  // Run event loop
  io->run_for(std::chrono::seconds(1));

  CLog::Log(LOGDEBUG, "Node stopped");
}
*/
#endif // HAS_LIBP2P

CGameServices::CGameServices(CControllerManager& controllerManager,
                             RETRO::CGUIGameRenderManager& renderManager,
                             PERIPHERALS::CPeripherals& peripheralManager,
                             const CProfileManager& profileManager)
  : m_controllerManager(controllerManager),
    m_gameRenderManager(renderManager),
    m_profileManager(profileManager),
    m_gameSettings(new CGameSettings())
{
#ifdef HAS_LIBP2P
  // libp2p echo server
  Libp2pServer libp2pThread;
  CThread m_libp2pServer(&libp2pThread, "Libp2p Server");
  //m_libp2pServer.Create();

  // libp2p echo client
  //RunClient();

  // Kademlia example
  //RunKademlia();

  // Gossip example
  //RunGossip();

  CLog::Log(LOGDEBUG, "Finished");
#endif // HAS_LIBP2P
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
