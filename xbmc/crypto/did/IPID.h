/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "crypto/Key.h"
#include "utils/Variant.h"

#include <string>
#include <vector> //! @todo Move CIPFS

//! @todo
#define DEFAULT_LIFETIME_SECS (60 * 60 * 24) // 1 day
#define DEFAULT_LIFETIME_HOURS 87600

namespace KODI
{
namespace CRYPTO
{

//! @todo
class CCID
{
public:
  std::string ToBaseEncodedString() const { return ""; }
};

class CIPFS
{
public:
  std::string ResolveName(const std::string& identifier) { return ""; }
  void PublishName(const std::string& ipfsPath,
                   unsigned int lifetimeSecs,
                   unsigned int ttlSecs,
                   const std::string& keyName)
  {
  }
  CVariant GetDAG(const std::string& cid) { return CVariant{}; }
  CCID PutDAG(const CVariant& content) { return CCID(); }
  std::vector<std::string> ListKeys() { return std::vector<std::string>{}; }
  void RemoveKey(const std::string& keyName) {}
  void ImportKey(const std::string& keyName,
                 const PrivateKey& privateKey,
                 const std::string& password)
  {
  }
};

/*!
 * \brief IPID DID Method
 *
 * The Interplanetary Identifiers DID method (did:ipid:) supports DIDs on the
 * public and private Interplanetary File System (IPFS) networks.
 */
class CIPID
{
public:
  CIPID(CIPFS* ipfs, unsigned int lifetimeSecs);

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
  CIPFS* const m_ipfs;
  const unsigned int m_lifetimeSecs;
};
} // namespace CRYPTO
} // namespace KODI
