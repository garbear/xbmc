/*
 *  Copyright (C) 2020-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CryptoProvider.h"

#include "crypto/ed25519/IEd25519Provider.h"
#include "crypto/random/IRandomGenerator.h"
#include "utils/log.h"

#include <openssl/rand.h>

using namespace KODI;
using namespace CRYPTO;

namespace
{
// Number of random bytes mixed into OpenSSL's PRNG during provider
// initialization. Value borrowed from Ripple.
constexpr size_t SEED_BYTES_COUNT = 128 * 4;
} // namespace

CCryptoProvider::CCryptoProvider(std::unique_ptr<IEd25519Provider> ed25519Provider,
                                 std::unique_ptr<IRandomGenerator> randomProvider)
  : m_ed25519Provider(std::move(ed25519Provider)),
    m_randomProvider(std::move(randomProvider))
{
  Initialize();
}

void CCryptoProvider::Initialize()
{
  if (!m_randomProvider)
  {
    CLog::Log(LOGWARNING, "CCryptoProvider::Initialize - random provider is unavailable");
    return;
  }

  // Generate the random seed
  auto bytes = m_randomProvider->RandomBytes(SEED_BYTES_COUNT);
  if (bytes.empty())
  {
    CLog::Log(LOGWARNING, "CCryptoProvider::Initialize - random provider returned an empty seed");
    return;
  }

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
  if (!m_ed25519Provider)
  {
    CLog::Log(LOGWARNING, "CCryptoProvider::GenerateEd25519 - Ed25519 provider is unavailable");
    return {};
  }

  return m_ed25519Provider->Generate();
}
