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

#include "utils/Variant.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace CRYPTO
{
class CDIDDocument
{
public:
  // Utility function
  static std::shared_ptr<CDIDDocument> CreateDocument(const std::string& did, CVariant content);

  /*!
   * \brief Create a new DID document
   */
  CDIDDocument(CVariant content);

  /*!
   * \brief Returns the current state of the document's content
   */
  const CVariant& GetContent() const { return m_content; }

  /*!
   * \brief Adds a new Public Key to the document
   *
   * \param publicKey An object with all the Public Key required as defined in
   * the DID Public Keys spec.
   */
  void AddPublicKey(CVariant& properties, const std::string& idPrefix);

  /*!
   * \brief Revokes a Public Key from the document
   *
   * \param publicKeyId The ID of the public key
   */
  void RevokePublicKey(const std::string& publicKeyId);

  /*!
   * \brief Adds a new Authentication to the document
   *
   * \param authentication The ID of the public key that is being referenced
   */
  void AddAuthentication(const std::string& authentication);

  /*!
   * \brief Removes an Authentication from the document
   *
   * \param authenticationId The authentication ID
   */
  void RemoveAuthentication(const std::string& authenticationId);

  /*!
   * \brief Adds a new Service Endpoint to the document
   *
   * \param service An object with all the Service Endpoint required properties
   * as defined in the DID Service Endpoints spec.
   */
  void AddService(CVariant& properties, const std::string& idPrefix);

  /*!
   * \brief Revokes a Service Endpoint from the document
   *
   * \param serviceId The ID of the service endpoint
   */
  void RemoveService(const std::string& serviceId);

private:
  void RefreshUpdated();

  // Utility functions
  static CVariant GenerateDocument(const std::string& did);

  CVariant m_content;
  std::vector<std::string> m_publicKeys;
  std::vector<std::string> m_authentication;
  std::vector<std::string> m_services;
};
} // namespace CRYPTO
} // namespace KODI
