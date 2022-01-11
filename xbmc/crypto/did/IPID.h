/*
 *  Copyright (C) 2020-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  This file was derived from the js-did-ipid project under the
 *  MIT License - https://github.com/ipfs-shipyard/js-did-ipid
 *  Copyright (C) 2019 Protocol Labs Inc.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later AND MIT
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/Key.h"
#include "utils/Variant.h"

#include <memory>
#include <string>
#include <vector> //! @todo Move CIPFS

//! @todo
#define DEFAULT_LIFETIME_SECS (60 * 60 * 24) // 1 day
#define DEFAULT_LIFETIME_HOURS 87600

namespace XFILE
{
class IIPFS;
}

namespace KODI
{
namespace CRYPTO
{

/*!
 * \brief IPID DID Method
 *
 * The InterPlanetary Identifiers DID method (did:ipid:) supports DIDs on the
 * public and private InterPlanetary File System (IPFS) networks.
 */
class CIPID
{
public:
  CIPID(std::shared_ptr<XFILE::IIPFS> ipfs,
        unsigned int lifetimeSecs = 87600 * 60 * 60); // 10 years

  /*!
   * \brief Resolves a DID and provides the respective DID Document
   *
   * \param did The DID to resolve
   *
   * \return The DID Document
   */
  CVariant Resolve(const std::string& did);

  /*!
   * \brief Creates a new DID and respective DID Document by applying all the
   * specified operations
   *
   * \param privateKeyPem A private key in PEM format
   *
   * \return The DID Document
   */
  CVariant Create(const PrivateKey& privateKey);

  /*!
   * \brief Updates an existing DID Document by applying all the specified
   * operations
   *
   * \param privateKeyPem A private key in PEM format
   *
   * \return The updated DID Document
   */
  CVariant Update(const PrivateKey& privateKey);

private:
  CVariant Publish(const PrivateKey& privateKey, const CVariant& content);

  void RemoveKey(const std::string& keyName);

  void ImportKey(const std::string& keyName,
                 const PrivateKey& privateKey,
                 const std::string& password);

  // Construction parameters
  const std::shared_ptr<XFILE::IIPFS> m_ipfs;
  const unsigned int m_lifetimeSecs;
};
} // namespace CRYPTO
} // namespace KODI
