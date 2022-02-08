/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OpenSSLEd25519Provider.h"

#include <memory>

#include <openssl/evp.h>

using namespace KODI;
using namespace CRYPTO;

constexpr size_t ED25519_PUBLIC_KEY_SIZE = 32u;
constexpr size_t ED25519_PRIVATE_KEY_SIZE = 32u;

namespace
{
/*!
 * \brief Deleter for OpenSSL public key algorithm context
 */
struct FreePublicKeyContext
{
  void operator()(EVP_PKEY_CTX* pctx) { EVP_PKEY_CTX_free(pctx); }
};

/*!
 * \brief Deleter for structure used by OpenSSL to store public and private
 *        keys
 */
struct FreeKeyStruct
{
  void operator()(EVP_PKEY* pkey) { EVP_PKEY_free(pkey); }
};
} // namespace

COpenSSLEd25519Provider::~COpenSSLEd25519Provider() = default;

KeyPair COpenSSLEd25519Provider::Generate()
{
  // Allocate a public key algorithm context for Edwards curve
  std::unique_ptr<EVP_PKEY_CTX, FreePublicKeyContext> context;
  context.reset(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr));

  if (!context)
    return {};

  // Initialize a public key algorithm context
  if (1 != EVP_PKEY_keygen_init(context.get()))
    return {};

  // It is mandatory to nullify the pointer before performing key generation
  EVP_PKEY* keyPtr = nullptr;
  if (1 != EVP_PKEY_keygen(context.get(), &keyPtr))
    return {};

  std::unique_ptr<EVP_PKEY, FreeKeyStruct> keyStruct(keyPtr);

  // Allocate a key pair
  KeyPair keyPair{
      {
          Key::Type::Ed25519,
          ByteArray(ED25519_PUBLIC_KEY_SIZE),
      },
      {
          Key::Type::Ed25519,
          ByteArray(ED25519_PRIVATE_KEY_SIZE),
      },
  };

  // Get buffer lengths
  size_t pubLen = keyPair.publicKey.data.size();
  size_t privLen = keyPair.privateKey.data.size();

  // Fill buffer with raw public key data
  if (1 != EVP_PKEY_get_raw_public_key(keyStruct.get(), keyPair.publicKey.data.data(), &pubLen))
    return {};

  // Fill buffer with raw private key data
  if (1 != EVP_PKEY_get_raw_private_key(keyStruct.get(), keyPair.privateKey.data.data(), &privLen))
    return {};

  return keyPair;
}
