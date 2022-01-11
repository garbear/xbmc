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

#include "DIDDocument.h"

#include "DIDUtils.h"
#include "XBDateTime.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace CRYPTO;

namespace
{
//! @todo Move variant field names to constants
} // namespace

std::shared_ptr<CDIDDocument> CDIDDocument::CreateDocument(const std::string& did, CVariant content)
{
  if (content.empty())
    content = GenerateDocument(did);

  return std::make_shared<CDIDDocument>(std::move(content));
}

CDIDDocument::CDIDDocument(CVariant content) : m_content(std::move(content))
{
}

void CDIDDocument::AddPublicKey(CVariant& properties, const std::string& idPrefix)
{
  properties["id"] = CDIDUtils::CreatePublicKeyID(m_content["id"].asString(),
                                                  properties["id"].asString(), idPrefix);
  if (properties["controller"].empty())
    properties["controller"] = m_content["id"];

  //! @todo
  // Assert public key is not a duplicate, causing a collision
  // Assert that public key contains the fields: "id", "type", "controller"
  // Assert that encoding is one of "publicKeyPem", "publicKeyJwk", "publicKeyHex", "publicKeyBase64", "publicKeyBase58", "publicKeyMultibase"

  m_content["publicKey"].push_back(properties);

  RefreshUpdated();

  //return properties;
}

void CDIDDocument::RevokePublicKey(const std::string& publicKeyId)
{
  CVariant filtered(CVariant::VariantTypeArray);

  //! @todo Filter in-place
  const CVariant& publicKeys = m_content["publicKey"];
  for (auto it = publicKeys.begin_array(); it != publicKeys.end_array(); ++it)
  {
    const CVariant& publicKey = *it;

    if (!CDIDUtils::IsEquivalentID(publicKey["id"].asString(), publicKeyId,
                                   CDIDUtils::SEPARATOR_PUBLIC_KEY))
      filtered.push_back(publicKey);
  }

  if (m_content["publicKey"].size() == filtered.size())
    return;

  RemoveAuthentication(publicKeyId);

  m_content["publicKey"] = filtered;

  RefreshUpdated();
}

void CDIDDocument::AddAuthentication(const std::string& authentication)
{
  CVariant key(CVariant::VariantTypeObject);

  const CVariant& publicKeys = m_content["publicKey"];
  for (auto it = publicKeys.begin_array(); it != publicKeys.end_array(); ++it)
  {
    const CVariant& publicKey = *it;

    if (CDIDUtils::IsEquivalentID(publicKey["id"].asString(), authentication,
                                  CDIDUtils::SEPARATOR_PUBLIC_KEY))
    {
      key = publicKey;
      break;
    }
  }

  //! @todo
  // Assert key["id"] does not collide with authentications in m_content["authentication"]

  m_content["authentication"].push_back(key["id"]);

  RefreshUpdated();

  //return key["id"].asString();
}

void CDIDDocument::RemoveAuthentication(const std::string& authenticationId)
{
  CVariant filtered(CVariant::VariantTypeArray);

  //! @todo Filter in-place
  const CVariant& authentications = m_content["authentication"];
  for (auto it = authentications.begin_array(); it != authentications.end_array(); ++it)
  {
    const CVariant& authentication = *it;

    if (!CDIDUtils::IsEquivalentID(CDIDUtils::ParseAuthenticationID(authentication),
                                   authenticationId, CDIDUtils::SEPARATOR_PUBLIC_KEY))
      filtered.push_back(authentication);
  }

  m_content["authentication"] = filtered;

  RefreshUpdated();
}

void CDIDDocument::AddService(CVariant& properties, const std::string& idPrefix)
{
  properties["id"] =
      CDIDUtils::CreateServiceID(m_content["id"].asString(), properties["id"].asString(), idPrefix);

  //! @todo
  // Assert properties["id"] does not collide with services in m_content["service"]
  // Assert that properties contains the fields: "type", "serviceEndpoint"

  m_content["service"].push_back(properties);

  RefreshUpdated();

  //return properties;
}

void CDIDDocument::RemoveService(const std::string& serviceId)
{
  CVariant filtered(CVariant::VariantTypeArray);

  //! @todo Filter in-place
  const CVariant& services = m_content["service"];
  for (auto it = services.begin_array(); it != services.end_array(); ++it)
  {
    const CVariant& service = *it;

    if (!CDIDUtils::IsEquivalentID(service["id"].asString(), serviceId,
                                   CDIDUtils::SEPARATOR_SERVICE))
      filtered.push_back(service);
  }

  if (m_content["service"].size() == filtered.size())
    return;

  m_content["service"] = filtered;

  RefreshUpdated();
}

void CDIDDocument::RefreshUpdated()
{
  m_content["updated"] = CDateTime::GetUTCDateTime().GetAsW3CDateTime(true);
}

CVariant CDIDDocument::GenerateDocument(const std::string& did)
{
  CVariant content(CVariant::VariantTypeObject);

  content["@context"] = CDIDUtils::DEFAULT_CONTEXT;
  content["id"] = did;
  content["created"] = CDateTime::GetUTCDateTime().GetAsW3CDateTime(true);

  return content;
}
