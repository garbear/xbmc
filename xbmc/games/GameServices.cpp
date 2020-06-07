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

// Crypto testing
#include "crypto/CryptoProvider.h"
#include "crypto/CryptoTypes.h"
#include "crypto/Key.h"
#include "crypto/codecs/Base58.h"
#include "crypto/ed25519/OpenSSLEd25519Provider.h"
#include "crypto/random/OpenSSLRandomGenerator.h"

using namespace KODI;
using namespace GAME;

CGameServices::CGameServices(CControllerManager& controllerManager,
                             RETRO::CGUIGameRenderManager& renderManager,
                             PERIPHERALS::CPeripherals& peripheralManager,
                             const CProfileManager& profileManager,
                             ADDON::CAddonMgr& addons)
  : m_controllerManager(controllerManager),
    m_gameRenderManager(renderManager),
    m_profileManager(profileManager),
    m_gameSettings(new CGameSettings()),
    m_videoShaders(new SHADER::CShaderPresetFactory(addons))
{
  // Create CSPRNG
  std::unique_ptr<CRYPTO::IRandomGenerator> csprng(new CRYPTO::COpenSSLRandomGenerator);
  CRYPTO::ByteArray buffer = csprng->RandomBytes(32);

  // Create Ed25519 signature scheme
  std::unique_ptr<CRYPTO::IEd25519Provider> ed25519Provider(new CRYPTO::COpenSSLEd25519Provider);
  std::unique_ptr<CRYPTO::IRandomGenerator> csrng(new CRYPTO::COpenSSLRandomGenerator);

  // Create crypto provider
  CRYPTO::CCryptoProvider cryptoProvider(std::move(ed25519Provider), std::move(csrng));

  // Generate public/private key pair
  auto keyPair = cryptoProvider.GenerateKeys(CRYPTO::Key::Type::Ed25519);

  /*!
   * DID format:
   *
   *   {
   *     "@context": "https://www.w3.org/ns/did/v1",
   *     "id": "did:example:123456789abcdefghi",
   *     "authentication": [
   *       {
   *         "id": "did:example:123456789abcdefghi#keys-1",
   *         "type": "Ed25519VerificationKey2018",
   *         "controller": "did:example:123",
   *         "publicKeyBase58": "H3C2AVvLMv6gmMNam3uVAjZpfkcJCwDwnZn6z3wXmqPV"
   *       }
   *     ],
   *     "service": [{
   *       "id":"did:example:123456789abcdefghi#vcs",
   *       "type": "VerifiableCredentialService",
   *       "serviceEndpoint": "https://example.com/vc/"
   *     }]
   *   }
   */
  const std::string id = "did:example:123456789abcdefghi"; //! @todo
  const std::string type = "Ed25519VerificationKey2018"; //! @todo
  const std::string controller = "did:example:123"; //! @todo
  auto publicKeyBase58 = CRYPTO::CBase58::EncodeBase58(keyPair.publicKey.data);

  // Inter-Planetary Naming System (IPNS)
  //
  // Create some content to publish
  //
  // DID syntax
  //
  //   Scheme: did
  //   Method: ipid
  //   IPFS ID: QmeJGfbW6bhapSfyjV5kDq5wt3h2g46Pwj15pJBVvy7jM3
  //
  std::string content = "did:ipid:QmeJGfbW6bhapSfyjV5kDq5wt3h2g46Pwj15pJBVvy7jM3";

  // Sample DDO stored using did method spec stored on IPFS
  //
  //   {
  //     "@context": "/ipfs/QmfS56jDfrXNaS6Xcsp3RJiXd2wyY7smeEAwyTAnL1RhEG",
  //     "id": "did:ipid:<IPFS ID>",
  //     "owner": [{
  //       "id": "did:ipid:<IPFS ID>",
  //       "type": ["CryptographicKey", "EdDsaPublicKey"],
  //       "curve": "ed25519",
  //       "expires": "2100-01-01T00:00:00Z",
  //       "publicKeyBase64": "lji9qTtkCydxtez/bt1zdLxVMMbz4SzWvlqgOBmURoM="
  //     }, {
  //       "id": "did:ipid:<IPFS ID>",
  //       "type": ["CryptographicKey", "Ed25519VerificationKey2018"],
  //       "expires": "2100-01-01T00:00:00Z",
  //       "publicKeyBase58": "H3C2AVvLMv6gmMNam3uVAjZpfkcJCwDwnZn6z3wXmqPV"
  //     }],
  //     "created": "2017-09-24T17:00:00Z",
  //     "updated": "2018-09-24T02:41:00Z",
  //     "signature": {} //! @todo
  //   }
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
