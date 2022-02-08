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

#include "filesystem/ipfs/ipid/IPID.h"

#include "crypto/did/DIDDocument.h"
#include "crypto/did/DIDUtils.h"
#include "filesystem/ipfs/IIPFS.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cstring>

using namespace KODI;
using namespace CRYPTO;
using namespace XFILE::IPFS;

namespace
{
constexpr const char* IPFS_PATH_PREFIX = "/ipfs/";

bool IsReady(const std::shared_ptr<XFILE::IIPFS>& ipfs)
{
  return ipfs && ipfs->IsOnline();
}
} // namespace

CIPID::CIPID(std::shared_ptr<XFILE::IIPFS> ipfs, unsigned int lifetimeSecs)
  : m_ipfs(std::move(ipfs)),
    m_lifetimeSecs(lifetimeSecs)
{
}

CVariant CIPID::Resolve(const std::string& did)
{
  if (!IsReady(m_ipfs))
    return {};

  std::string method;
  std::string identifier;
  if (!CDIDUtils::ParseDID(did, method, identifier) || method != "ipid" || identifier.empty())
    return {};

  const std::string path = m_ipfs->ResolveName(identifier);
  if (!StringUtils::StartsWith(path, IPFS_PATH_PREFIX))
    return {};

  const std::string cidStr = path.substr(std::strlen(IPFS_PATH_PREFIX));
  if (cidStr.empty())
    return {};

  CVariant document = m_ipfs->GetDAG(cidStr);
  if (!document.isObject())
    return {};

  if (document["id"].asString() != did)
    return {};

  return document;
}

CVariant CIPID::Create(const PrivateKey& privateKey)
{
  if (!IsReady(m_ipfs))
    return {};

  const std::string did = CDIDUtils::GetDID(privateKey);
  if (did.empty())
    return {};

  if (Resolve(did).empty())
  {
    CVariant content(CVariant::VariantTypeObject);

    std::shared_ptr<CDIDDocument> didDocument = CDIDDocument::CreateDocument(did, content);

    return Publish(privateKey, didDocument->GetContent());
  }

  return CVariant{};
}

CVariant CIPID::Update(const PrivateKey& privateKey)
{
  if (!IsReady(m_ipfs))
    return {};

  const std::string did = CDIDUtils::GetDID(privateKey);
  if (did.empty())
    return {};

  CVariant content = Resolve(did);
  if (content.empty())
    return {};

  std::shared_ptr<CDIDDocument> didDocument = CDIDDocument::CreateDocument(did, std::move(content));

  return Publish(privateKey, didDocument->GetContent());
}

CVariant CIPID::Publish(const PrivateKey& privateKey, const CVariant& content)
{
  if (!IsReady(m_ipfs) || privateKey.data.empty() || content.empty())
    return {};

  const std::string random = CDIDUtils::GenerateRandomString();
  if (random.empty())
    return {};

  const std::string keyName = "kodi-ipid-" + random;

  ImportKey(keyName, privateKey, "");
  const std::vector<std::string> keyList = m_ipfs->ListKeys();
  const bool imported = std::find(keyList.begin(), keyList.end(), keyName) != keyList.end();
  if (!imported)
  {
    RemoveKey(keyName);
    return {};
  }

  const std::string cid = m_ipfs->PutDAG(content);
  if (cid.empty())
  {
    RemoveKey(keyName);
    return {};
  }

  const std::string path = std::string(IPFS_PATH_PREFIX) + cid;

  const bool published = m_ipfs->PublishName(path, m_lifetimeSecs, m_lifetimeSecs, keyName);
  RemoveKey(keyName);

  if (!published)
    return {};

  return content;
}

bool CIPID::RemoveKey(const std::string& keyName)
{
  if (!m_ipfs || keyName.empty())
    return false;

  m_ipfs->RemoveKey(keyName);

  return true;
}

void CIPID::ImportKey(const std::string& keyName,
                      const PrivateKey& privateKey,
                      const std::string& password)
{
  if (!m_ipfs || keyName.empty() || privateKey.data.empty())
    return;

  RemoveKey(keyName);

  m_ipfs->ImportKey(keyName, privateKey, password);
}
