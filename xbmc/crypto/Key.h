/*
 *  Copyright (C) 2020 Team Kodi
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

//! @todo C++17 will allow the following for stronger typing:
//!
//! struct PublicKey : public Key { };
//! struct PrivateKey : public Key { };
using PublicKey = Key;
using PrivateKey = Key;

struct KeyPair
{
  PublicKey publicKey{};
  PrivateKey privateKey{};
};
} // namespace CRYPTO
} // namespace KODI
