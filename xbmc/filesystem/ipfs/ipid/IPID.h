/*
 *  Copyright (C) 2020-2026 Team Kodi
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

namespace XFILE
{
class IIPFS;

namespace IPFS
{
constexpr unsigned int DEFAULT_IPID_LIFETIME_SECS = 87600 * 60 * 60; // 10 years

/*!
 * \brief IPID DID Method
 *
 * The InterPlanetary Identifiers DID method (did:ipid:) supports DIDs on the
 * public and private InterPlanetary File System (IPFS) networks.
 */
class CIPID
{
public:
  CIPID(std::shared_ptr<XFILE::IIPFS> ipfs, unsigned int lifetimeSecs = DEFAULT_IPID_LIFETIME_SECS);

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
  CVariant Create(const KODI::CRYPTO::PrivateKey& privateKey);

  /*!
   * \brief Updates an existing DID Document by applying all the specified
   * operations
   *
   * \param privateKeyPem A private key in PEM format
   *
   * \return The updated DID Document
   */
  CVariant Update(const KODI::CRYPTO::PrivateKey& privateKey);

private:
  // Test access for safe-failure publish paths while IPNS private-key derivation
  // is not implemented and Create()/Update() cannot reach Publish().
  friend class CIPIDTestAccess;

  /*!
   * \brief Publishes the given content to IPFS and returns the resulting CID
   * as the new version of the DID Document
   *
   * \param privateKeyPem A private key in PEM format
   * \param content The content to be published to IPFS
   *
   * \return The new version of the DID Document
   */
  CVariant Publish(const KODI::CRYPTO::PrivateKey& privateKey, const CVariant& content);

  /*!
   * \brief Removes a key from the DID Document
   *
   * \param keyName The name of the key to remove
   *
   * \return True if the key was listed and removed, false otherwise
   */
  bool RemoveKey(const std::string& keyName);

  /*!
   * \brief Imports a key into the DID Document
   *
   * \param keyName The name of the key to import
   * \param privateKey The private key to import
   * \param password The password for the private key
   */
  void ImportKey(const std::string& keyName,
                 const KODI::CRYPTO::PrivateKey& privateKey,
                 const std::string& password);

  // Construction parameters
  const std::shared_ptr<XFILE::IIPFS> m_ipfs;
  const unsigned int m_lifetimeSecs;
};
} // namespace IPFS
} // namespace XFILE
