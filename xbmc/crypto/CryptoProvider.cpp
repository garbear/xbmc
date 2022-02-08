/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CryptoProvider.h"

#include "crypto/ed25519/IEd25519Provider.h"
#include "crypto/random/IRandomGenerator.h"

#include <openssl/rand.h>

using namespace KODI;
using namespace CRYPTO;

CCryptoProvider::CCryptoProvider(std::unique_ptr<IEd25519Provider> ed25519Provider,
                                 std::unique_ptr<IRandomGenerator> randomProvider)
  : m_ed25519Provider(std::move(ed25519Provider)), m_randomProvider(std::move(randomProvider))
{
  Initialize();
}

void CCryptoProvider::Initialize()
{
  // Constant borrowed from Ripple
  constexpr size_t SEED_BYTES_COUNT = 128 * 4;

  // Generate the random seed
  auto bytes = m_randomProvider->RandomBytes(SEED_BYTES_COUNT);

  // Seed OpenSSL random provider
  RAND_seed(static_cast<const void*>(bytes.data()), bytes.size());
}

KeyPair CCryptoProvider::GenerateKeys(Key::Type keyType)
{
  switch (keyType)
  {
    case Key::Type::Ed25519:
      return GenerateEd25519();
    default:
      break;
  }

  return {};
}

KeyPair CCryptoProvider::GenerateEd25519()
{
  return m_ed25519Provider->Generate();
}
