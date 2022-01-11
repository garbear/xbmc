/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/CryptoProvider.h"
#include "crypto/Key.h"
#include "crypto/codecs/Base58BTC.h"
#include "crypto/ed25519/OpenSSLEd25519Provider.h"
#include "crypto/random/OpenSSLRandomGenerator.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

TEST(TestDID, TestDID)
{
  // Create Ed25519 signature scheme
  std::unique_ptr<IEd25519Provider> ed25519Provider(new COpenSSLEd25519Provider);
  std::unique_ptr<IRandomGenerator> csrng(new COpenSSLRandomGenerator);

  // Create crypto provider
  CCryptoProvider cryptoProvider(std::move(ed25519Provider), std::move(csrng));

  // Generate public/private key pair
  KeyPair keyPair = cryptoProvider.GenerateKeys(CRYPTO::Key::Type::Ed25519);

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

  const std::string publicKeyBase58 = CRYPTO::CBase58BTC::EncodeBase58(keyPair.publicKey.data);

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
  const std::string content = "did:ipid:QmeJGfbW6bhapSfyjV5kDq5wt3h2g46Pwj15pJBVvy7jM3";

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
