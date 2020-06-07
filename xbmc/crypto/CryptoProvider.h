/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/CryptoTypes.h"
#include "crypto/Key.h"

#include <memory>

namespace KODI
{
namespace CRYPTO
{
class IEd25519Provider;
class IRandomGenerator;

class CCryptoProvider
{
public:
  explicit CCryptoProvider(std::unique_ptr<IEd25519Provider> ed25519Provider,
                           std::unique_ptr<IRandomGenerator> randomProvider);

  KeyPair GenerateKeys(Key::Type keyType);

private:
  void Initialize();

  KeyPair GenerateEd25519();

  // Construction parameters
  const std::unique_ptr<IEd25519Provider> m_ed25519Provider;
  const std::unique_ptr<IRandomGenerator> m_randomProvider;
};
} // namespace CRYPTO
} // namespace KODI
