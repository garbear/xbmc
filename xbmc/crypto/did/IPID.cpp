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

#include "IPID.h"

#include "DIDDocument.h"
#include "DIDUtils.h"
#include "filesystem/IIPFS.h"

#include <algorithm>
#include <cstring>

using namespace KODI;
using namespace CRYPTO;

CIPID::CIPID(std::shared_ptr<XFILE::IIPFS> ipfs, unsigned int lifetimeSecs)
  : m_ipfs(std::move(ipfs)), m_lifetimeSecs(lifetimeSecs)
{
}

CVariant CIPID::Resolve(const std::string& did)
{
  std::string method;
  std::string identifier;
  CDIDUtils::ParseDID(did, method, identifier);

  //! @todo
  //const std::string path = m_ipfs->ResolveName(identifier);
  const std::string path = "/ipfs/" + identifier;

  std::string cidStr = path.substr(std::strlen("/ipfs/"));

  CVariant document = m_ipfs->GetDAG(cidStr);

  //! @todo
  // Assert document is an object
  // Assert document contains valid "@context" property (string or ordered set)
  // If context is an array, assert first context is CDIDUtils::DEFAULT_CONTEXT
  // If context is a string, assert equality with CDIDUtils::DEFAULT_CONTEXT
  // Assert document contains "id" property
  // Assert "id" property is a valid DID

  return document;
}

CVariant CIPID::Create(const PrivateKey& privateKey)
{
  const std::string did = CDIDUtils::GetDID(privateKey);

  if (Resolve(did).empty())
  {
    CVariant content(CVariant::VariantTypeObject);

    std::shared_ptr<CDIDDocument> didDocument = CDIDDocument::CreateDocument(did, content);

    //operations(didDocument)

    return Publish(privateKey, didDocument->GetContent());
  }

  //! @todo Handle creation error
  return CVariant{};
}

CVariant CIPID::Update(const PrivateKey& privateKey)
{
  CVariant document;

  const std::string did = CDIDUtils::GetDID(privateKey);

  CVariant content = Resolve(did);
  std::shared_ptr<CDIDDocument> didDocument = CDIDDocument::CreateDocument(did, std::move(content));

  //! @todo
  //operations(didDocument)

  return Publish(privateKey, didDocument->GetContent());
}

CVariant CIPID::Publish(const PrivateKey& privateKey, const CVariant& content)
{
  const std::string keyName = "kodi-ipid-" + CDIDUtils::GenerateRandomString();

  ImportKey(keyName, privateKey, "");

  const std::string cid = m_ipfs->PutDAG(content);
  const std::string path = "/ipfs/" + cid; //cid.ToBaseEncodedString();

  m_ipfs->PublishName(path, m_lifetimeSecs, m_lifetimeSecs, keyName);

  //! @todo Only remove key on error?
  RemoveKey(keyName);

  return content;
}

void CIPID::RemoveKey(const std::string& keyName)
{
  std::vector<std::string> keyList = m_ipfs->ListKeys();

  const bool bHasKey =
      std::find_if(keyList.begin(), keyList.end(),
                   [&keyName](const std::string& key) { return key == keyName; }) != keyList.end();

  if (!bHasKey)
    return;

  m_ipfs->RemoveKey(keyName);
}

void CIPID::ImportKey(const std::string& keyName,
                      const PrivateKey& privateKey,
                      const std::string& password)
{
  RemoveKey(keyName);

  m_ipfs->ImportKey(keyName, privateKey, password);
}
