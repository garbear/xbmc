/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "CryptoTypes.h"

namespace KODI
{
namespace CRYPTO
{
/*!
 * \brief Cryptographic key
 */
struct Key
{
  // Supported types of keys
  enum class Type
  {
    Unspecified,
    // ECDSA,
    Ed25519,
    // Secp256k1,
    // RSA,
  };

  // Key type
  Key::Type type = Key::Type::Unspecified;

  // Key content
  ByteArray data{};
};

//! Allow for stronger typing of public keys
struct PublicKey : public Key
{
};

//! Allow for stronger typing of private keys
struct PrivateKey : public Key
{
};

struct KeyPair
{
  PublicKey publicKey{};
  PrivateKey privateKey{};
};
} // namespace CRYPTO
} // namespace KODI
