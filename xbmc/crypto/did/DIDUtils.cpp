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

#include "DIDUtils.h"

#include "crypto/codecs/Base58BTC.h"
#include "utils/Variant.h"

#include <cstring>
#include <regex>
#include <sstream>

using namespace KODI;
using namespace CRYPTO;

const std::string CDIDUtils::DEFAULT_CONTEXT = "https://w3id.org/did/v1";
const std::string CDIDUtils::SEPARATOR_PUBLIC_KEY = "#";
const std::string CDIDUtils::SEPARATOR_SERVICE = ";";

std::string CDIDUtils::GetDID(const PrivateKey& privateKey)
{
  return GenerateDID(privateKey);
}

bool CDIDUtils::ParseDID(const std::string& did, std::string& method, std::string& identifier)
{
  static const std::regex regex("^did:(\\w+):(\\w+).*");

  std::smatch matches;
  if (std::regex_search(did, matches, regex))
  {
    method = matches[1];
    identifier = matches[2];
    return true;
  }

  return false;
}

std::string CDIDUtils::CreatePublicKeyID(const std::string& did,
                                         std::string fragment,
                                         const std::string& prefix)
{
  return CreateID(did, fragment, SEPARATOR_PUBLIC_KEY, prefix);
}

std::string CDIDUtils::CreateServiceID(const std::string& did,
                                       std::string fragment,
                                       const std::string& prefix)
{
  return CreateID(did, fragment, SEPARATOR_SERVICE, prefix);
}

bool CDIDUtils::IsEquivalentID(std::string id1, std::string id2, const std::string& separator)
{
  const size_t sepPos1 = id1.find(separator);
  const size_t sepPos2 = id2.find(separator);

  if (sepPos1 != std::string::npos)
    id1 = id1.substr(sepPos1 + 1);

  if (sepPos2 != std::string::npos)
    id2 = id2.substr(sepPos2 + 1);

  return id1 == id2;
}

std::string CDIDUtils::ParseAuthenticationID(const CVariant& authentication)
{
  if (authentication.isObject())
    return authentication["id"].asString();

  return authentication.asString();
}

std::string CDIDUtils::GenerateRandomString()
{
  //! @todo
  return "";
}

std::string CDIDUtils::GenerateIpnsName(const PrivateKey& key)
{
  const std::string peerIdBase58;

  /*! @todo
  const CPeerID peerId; //! @todo CreateFromPrivateKey(key)

  const std::string peerIdBase58 = CBase58BTC::EncodeBase58(peerId.GetData());
  */

  return peerIdBase58;
}

std::string CDIDUtils::GenerateDID(const PrivateKey& key)
{
  const std::string identifier = GenerateIpnsName(key);

  return "did:ipid:" + identifier;
}

std::string CDIDUtils::CreateID(const std::string& did,
                                std::string fragment,
                                const std::string& separator,
                                const std::string& prefix)
{
  if (fragment.empty())
    fragment = GenerateRandomString();

  std::ostringstream identifier;

  identifier << did;
  identifier << separator;
  identifier << prefix;
  identifier << fragment;

  return identifier.str();
}
