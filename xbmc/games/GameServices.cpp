/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameServices.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "games/GameSettings.h"
#include "profiles/ProfileManager.h"

// Crypto testing
#include "crypto/CryptoProvider.h"
#include "crypto/CryptoTypes.h"
#include "crypto/Key.h"
#include "crypto/codecs/Base58.h"
#include "crypto/ed25519/OpenSSLEd25519Provider.h"
#include "crypto/random/BoostRandomGenerator.h"

using namespace KODI;
using namespace GAME;

CGameServices::CGameServices(CControllerManager &controllerManager,
                             RETRO:: CGUIGameRenderManager &renderManager,
                             PERIPHERALS::CPeripherals &peripheralManager,
                             const CProfileManager &profileManager,
                             ADDON::CAddonMgr &addons,
                             ADDON::CBinaryAddonManager &binaryAddons) :
  m_controllerManager(controllerManager),
  m_gameRenderManager(renderManager),
  m_peripheralManager(peripheralManager),
  m_profileManager(profileManager),
  m_gameSettings(new CGameSettings()),
  m_videoShaders(new SHADER::CShaderPresetFactory(addons, binaryAddons))
{
  // Create CSPRNG
  std::unique_ptr<CRYPTO::IRandomGenerator> csprng(new CRYPTO::CBoostRandomGenerator);
  CRYPTO::ByteArray buffer = csprng->RandomBytes(32);

  // Create Ed25519 signature scheme
  std::unique_ptr<CRYPTO::IEd25519Provider> ed25519Provider(new CRYPTO::COpenSSLEd25519Provider);
  std::unique_ptr<CRYPTO::IRandomGenerator> csrng(new CRYPTO::CBoostRandomGenerator);

  // Create crypto provider
  CRYPTO::CCryptoProvider cryptoProvider(std::move(ed25519Provider), std::move(csrng));

  // Generate public/private key pair
  auto keyPair = cryptoProvider.GenerateKeys(CRYPTO::Key::Type::Ed25519);

  /*!
   * DID format:
   *
   *   {
   *     "id": "did:example:123#ZC2jXTO6t4R501bfCXv3RxarZyUbdP2w_psLwMuY6ec",
   *     "type": "Ed25519VerificationKey2018",
   *     "controller": "did:example:123",
   *     "publicKeyBase58": "H3C2AVvLMv6gmMNam3uVAjZpfkcJCwDwnZn6z3wXmqPV"
   *   }
   */
  const std::string id = "did:example:123#ZC2jXTO6t4R501bfCXv3RxarZyUbdP2w_psLwMuY6ec"; //! @todo
  const std::string type = "Ed25519VerificationKey2018"; //! @todo
  const std::string controller = "did:example:123"; //! @todo
  auto publicKeyBase58 = CRYPTO::CBase58::EncodeBase58(keyPair.publicKey.data);
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
