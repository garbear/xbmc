/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/CryptoProvider.h"
#include "crypto/ed25519/IEd25519Provider.h"
#include "crypto/random/IRandomGenerator.h"

#include <cstddef>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

namespace
{
class CEmptyRandomGenerator : public IRandomGenerator
{
public:
  std::vector<uint8_t> RandomBytes(size_t length) override
  {
    (void)length;
    return {};
  }
};

class CTestEd25519Provider : public IEd25519Provider
{
public:
  KeyPair Generate() override
  {
    KeyPair keyPair;
    keyPair.publicKey.type = Key::Type::Ed25519;
    keyPair.publicKey.data = {0x01, 0x02, 0x03};
    keyPair.privateKey.type = Key::Type::Ed25519;
    keyPair.privateKey.data = {0x04, 0x05, 0x06};
    return keyPair;
  }
};
} // namespace

TEST(TestCryptoProvider, MissingRandomProviderDoesNotCrash)
{
  CCryptoProvider cryptoProvider(std::unique_ptr<IEd25519Provider>(new CTestEd25519Provider),
                                 nullptr);

  KeyPair keyPair = cryptoProvider.GenerateKeys(Key::Type::Ed25519);

  EXPECT_EQ(Key::Type::Ed25519, keyPair.publicKey.type);
  EXPECT_EQ(std::vector<uint8_t>({0x01, 0x02, 0x03}), keyPair.publicKey.data);
}

TEST(TestCryptoProvider, EmptyRandomSeedDoesNotCrash)
{
  CCryptoProvider cryptoProvider(std::unique_ptr<IEd25519Provider>(new CTestEd25519Provider),
                                 std::unique_ptr<IRandomGenerator>(new CEmptyRandomGenerator));

  KeyPair keyPair = cryptoProvider.GenerateKeys(Key::Type::Ed25519);

  EXPECT_EQ(Key::Type::Ed25519, keyPair.publicKey.type);
  EXPECT_EQ(std::vector<uint8_t>({0x01, 0x02, 0x03}), keyPair.publicKey.data);
}

TEST(TestCryptoProvider, MissingEd25519ProviderReturnsEmptyKeys)
{
  CCryptoProvider cryptoProvider(nullptr, nullptr);

  KeyPair keyPair = cryptoProvider.GenerateKeys(Key::Type::Ed25519);

  EXPECT_EQ(Key::Type::Unspecified, keyPair.publicKey.type);
  EXPECT_TRUE(keyPair.publicKey.data.empty());
  EXPECT_EQ(Key::Type::Unspecified, keyPair.privateKey.type);
  EXPECT_TRUE(keyPair.privateKey.data.empty());
}
