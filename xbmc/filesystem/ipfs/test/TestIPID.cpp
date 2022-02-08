/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/Key.h"
#include "crypto/did/DIDUtils.h"
#include "filesystem/ipfs/IIPFS.h"
#include "filesystem/ipfs/ipid/IPID.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;
using namespace XFILE::IPFS;

namespace XFILE::IPFS
{
class CIPIDTestAccess
{
public:
  static CVariant Publish(CIPID& ipid,
                          const KODI::CRYPTO::PrivateKey& privateKey,
                          const CVariant& content)
  {
    return ipid.Publish(privateKey, content);
  }
};
} // namespace XFILE::IPFS

namespace
{
const std::string mockHash = "zdpuApA2CCoPHQEoP4nResbK2dq2zawFX3verNkMFmNbpDnXZ";
const std::string mockPath = "/ipfs/" + mockHash;
const std::string mockIpnsHash = "QmUTE4cxTxihntPEFqTprgbqyyS9YRaRcC8FXp6PACEjFG";
const std::string mockDid = "did:ipid:" + mockIpnsHash;

CVariant GetMockDocument()
{
  CVariant mockDocument{CVariant::VariantTypeObject};

  mockDocument["@context"] = "https://w3id.org/did/v1";
  mockDocument["id"] = mockDid;
  mockDocument["created"] = "2019-03-19T16:52:44.948Z";
  mockDocument["updated"] = "2019-03-19T16:53:56.463Z";

  CVariant publicKey{CVariant::VariantTypeObject};

  publicKey["id"] = mockDid + "#bqvnazrkarh";
  publicKey["type"] = "myType";
  publicKey["controller"] = "myController";
  publicKey["publicKeyHex"] = "1A2B3C";

  mockDocument["publicKey"].push_back(std::move(publicKey));
  mockDocument["authentication"].push_back(mockDid + "#bqvnazrkarh");

  CVariant service{CVariant::VariantTypeObject};
  service["id"] = mockDid + ";myServiceId";
  service["type"] = "serviceType";
  service["serviceEndpoint"] = "http://myserviceendpoint.com";

  mockDocument["service"].push_back(std::move(service));

  return mockDocument;
}

class CMockIPFS : public XFILE::IIPFS
{
public:
  bool Initialize(const std::string& dataStoreRoot) override { return !dataStoreRoot.empty(); }
  void Deinitialize() override {}
  bool IsOnline() override { return online; }

  std::string ResolveName(const std::string& identifier) override
  {
    resolvedIdentifiers.emplace_back(identifier);
    return resolveNameResult;
  }

  bool PublishName(const std::string& ipfsPath,
                   unsigned int lifetimeSecs,
                   unsigned int ttlSecs,
                   const std::string& keyName) override
  {
    publishNameCalls++;
    lastPublishedPath = ipfsPath;
    lastPublishedLifetimeSecs = lifetimeSecs;
    lastPublishedTtlSecs = ttlSecs;
    lastPublishedKeyName = keyName;
    return publishNameResult;
  }

  CVariant GetDAG(const std::string& cid) override
  {
    requestedCids.emplace_back(cid);
    return dagResult;
  }

  std::string PutDAG(const CVariant& content, KODI::DATASTORE::CIDCodec codec) override
  {
    putDagCalls++;
    lastPutDagContent = content;
    lastPutDagCodec = codec;
    return putDagResult;
  }

  bool AddFile(const uint8_t* data, size_t size, std::string& cid) override
  {
    if (!data || size == 0)
      return false;

    cid = mockHash;
    return true;
  }

  bool GetFile(const std::string& cid, std::vector<uint8_t>& data) override
  {
    if (cid.empty())
      return false;

    data = {1, 2, 3};
    return true;
  }

  bool IsDirectory(const std::string& cid) override { return !cid.empty(); }

  bool ListDirectory(const std::string& cid, std::vector<XFILE::CIPFSEntry>& entries) override
  {
    if (cid.empty())
      return false;

    entries.clear();
    return true;
  }

  std::vector<std::string> ListKeys() override { return keychainKeys; }

  void RemoveKey(const std::string& keyName) override
  {
    removedKeys.emplace_back(keyName);
    keychainKeys.erase(std::remove(keychainKeys.begin(), keychainKeys.end(), keyName),
                       keychainKeys.end());
  }

  void ImportKey(const std::string& keyName,
                 const PrivateKey& privateKey,
                 const std::string& password) override
  {
    importedKeys.emplace_back(keyName);
    lastImportedKey = privateKey;
    lastImportPassword = password;
    if (importAddsKeychainKey)
      keychainKeys.emplace_back(keyName);
  }

  bool online{true};
  bool importAddsKeychainKey{true};
  bool publishNameResult{true};
  std::string resolveNameResult{mockPath};
  CVariant dagResult{GetMockDocument()};
  std::string putDagResult{mockHash};
  std::vector<std::string> keychainKeys{"key1", "key2"};
  std::vector<std::string> resolvedIdentifiers;
  std::vector<std::string> requestedCids;
  std::vector<std::string> importedKeys;
  std::vector<std::string> removedKeys;
  CVariant lastPutDagContent;
  KODI::DATASTORE::CIDCodec lastPutDagCodec{KODI::DATASTORE::CIDCodec::DAG_JSON};
  PrivateKey lastImportedKey;
  std::string lastImportPassword;
  std::string lastPublishedPath;
  std::string lastPublishedKeyName;
  unsigned int lastPublishedLifetimeSecs{0};
  unsigned int lastPublishedTtlSecs{0};
  unsigned int publishNameCalls{0};
  unsigned int putDagCalls{0};
};

std::string ToJson(const CVariant& value)
{
  std::string json;
  EXPECT_TRUE(CJSONVariantWriter::Write(value, json, false));
  return json;
}
} // namespace

TEST(TestIPID, ParseDID)
{
  std::string method;
  std::string identifier;
  EXPECT_TRUE(CDIDUtils::ParseDID(mockDid, method, identifier));

  EXPECT_EQ(method, "ipid");
  EXPECT_EQ(identifier, mockIpnsHash);

  EXPECT_FALSE(CDIDUtils::ParseDID("did:ipid:#", method, identifier));
  EXPECT_TRUE(method.empty());
  EXPECT_TRUE(identifier.empty());

  EXPECT_FALSE(CDIDUtils::ParseDID("did::abc", method, identifier));
  EXPECT_FALSE(CDIDUtils::ParseDID("did:ipid:", method, identifier));
  EXPECT_TRUE(CDIDUtils::ParseDID("did:key:z6MkjchhfUsD6S8D9aYxke1xp3", method, identifier));
  EXPECT_EQ(method, "key");
  EXPECT_EQ(identifier, "z6MkjchhfUsD6S8D9aYxke1xp3");
}

TEST(TestIPID, FactoryAcceptsIPFS)
{
  const std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs);
}

TEST(TestIPID, ResolveRejectsInvalidDIDs)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs);

  EXPECT_TRUE(ipid.Resolve("not-a-did").empty());
  EXPECT_TRUE(ipid.Resolve("did:key:z6MkjchhfUsD6S8D9aYxke1xp3").empty());
  EXPECT_TRUE(ipid.Resolve("did:ipid:").empty());
  EXPECT_TRUE(ipid.Resolve("did:ipid:#").empty());
  EXPECT_TRUE(mockIpfs->resolvedIdentifiers.empty());
}

TEST(TestIPID, ResolveReturnsEmptyWhenOfflineOrNull)
{
  {
    CIPID ipid(nullptr);
    EXPECT_TRUE(ipid.Resolve(mockDid).empty());
  }

  {
    const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
    mockIpfs->online = false;
    CIPID ipid(mockIpfs);

    EXPECT_TRUE(ipid.Resolve(mockDid).empty());
    EXPECT_TRUE(mockIpfs->resolvedIdentifiers.empty());
  }
}

TEST(TestIPID, ResolveUsesIpnsNameAndFetchesDag)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs);

  const CVariant document = ipid.Resolve(mockDid);

  EXPECT_EQ(mockIpfs->resolvedIdentifiers, std::vector<std::string>{mockIpnsHash});
  EXPECT_EQ(mockIpfs->requestedCids, std::vector<std::string>{mockHash});
  EXPECT_EQ(ToJson(document), ToJson(GetMockDocument()));
}

TEST(TestIPID, ResolveRejectsInvalidDagResults)
{
  {
    const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
    mockIpfs->dagResult = CVariant{};
    CIPID ipid(mockIpfs);

    EXPECT_TRUE(ipid.Resolve(mockDid).empty());
    EXPECT_EQ(mockIpfs->requestedCids, std::vector<std::string>{mockHash});
  }

  {
    const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
    mockIpfs->dagResult = CVariant{"not an object"};
    CIPID ipid(mockIpfs);

    EXPECT_TRUE(ipid.Resolve(mockDid).empty());
    EXPECT_EQ(mockIpfs->requestedCids, std::vector<std::string>{mockHash});
  }

  {
    const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
    CVariant wrongDocument = GetMockDocument();
    wrongDocument["id"] = "did:ipid:other";
    mockIpfs->dagResult = wrongDocument;
    CIPID ipid(mockIpfs);

    EXPECT_TRUE(ipid.Resolve(mockDid).empty());
    EXPECT_EQ(mockIpfs->requestedCids, std::vector<std::string>{mockHash});
  }
}

TEST(TestIPID, ResolveRejectsMissingOrNonIpfsNameResolution)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs);

  mockIpfs->resolveNameResult.clear();
  EXPECT_TRUE(ipid.Resolve(mockDid).empty());
  EXPECT_TRUE(mockIpfs->requestedCids.empty());

  mockIpfs->resolveNameResult = "/ipns/" + mockHash;
  EXPECT_TRUE(ipid.Resolve(mockDid).empty());
  EXPECT_TRUE(mockIpfs->requestedCids.empty());

  mockIpfs->resolveNameResult = "/ipfs/";
  EXPECT_TRUE(ipid.Resolve(mockDid).empty());
  EXPECT_TRUE(mockIpfs->requestedCids.empty());
}

TEST(TestIPID, CreateFailsSafelyWhenDIDCannotBeGenerated)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs);

  PrivateKey privateKey;
  privateKey.data = {1, 2, 3};

  EXPECT_TRUE(ipid.Create(privateKey).empty());
  EXPECT_TRUE(mockIpfs->resolvedIdentifiers.empty());
  EXPECT_EQ(mockIpfs->putDagCalls, 0U);
  EXPECT_EQ(mockIpfs->publishNameCalls, 0U);
}

TEST(TestIPID, UpdateFailsSafelyWhenDIDCannotBeGenerated)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs);

  PrivateKey privateKey;
  privateKey.data = {1, 2, 3};

  EXPECT_TRUE(ipid.Update(privateKey).empty());
  EXPECT_TRUE(mockIpfs->resolvedIdentifiers.empty());
  EXPECT_EQ(mockIpfs->putDagCalls, 0U);
  EXPECT_EQ(mockIpfs->publishNameCalls, 0U);
}

TEST(TestIPID, PublishRemovesTemporaryKeyWhenImportVerificationFails)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  mockIpfs->importAddsKeychainKey = false;
  CIPID ipid(mockIpfs);

  PrivateKey privateKey;
  privateKey.data = {1, 2, 3};

  EXPECT_TRUE(CIPIDTestAccess::Publish(ipid, privateKey, GetMockDocument()).empty());
  ASSERT_EQ(mockIpfs->importedKeys.size(), 1U);
  ASSERT_GE(mockIpfs->removedKeys.size(), 1U);
  EXPECT_EQ(mockIpfs->removedKeys.back(), mockIpfs->importedKeys.front());
  EXPECT_EQ(mockIpfs->putDagCalls, 0U);
  EXPECT_EQ(mockIpfs->publishNameCalls, 0U);
}

TEST(TestIPID, PublishReturnsEmptyAndRemovesTemporaryKeyWhenPublishNameFails)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  mockIpfs->publishNameResult = false;
  CIPID ipid(mockIpfs);

  PrivateKey privateKey;
  privateKey.data = {1, 2, 3};
  const CVariant content = GetMockDocument();

  EXPECT_TRUE(CIPIDTestAccess::Publish(ipid, privateKey, content).empty());
  ASSERT_EQ(mockIpfs->importedKeys.size(), 1U);
  ASSERT_GE(mockIpfs->removedKeys.size(), 1U);
  EXPECT_EQ(mockIpfs->removedKeys.back(), mockIpfs->importedKeys.front());
  EXPECT_EQ(mockIpfs->putDagCalls, 1U);
  EXPECT_EQ(mockIpfs->publishNameCalls, 1U);
  EXPECT_EQ(mockIpfs->lastPublishedPath, mockPath);
  EXPECT_EQ(mockIpfs->lastPublishedKeyName, mockIpfs->importedKeys.front());
}

TEST(TestIPID, PublishReturnsContentWhenPublishNameSucceeds)
{
  const std::shared_ptr<CMockIPFS> mockIpfs = std::make_shared<CMockIPFS>();
  CIPID ipid(mockIpfs, 123);

  PrivateKey privateKey;
  privateKey.data = {1, 2, 3};
  const CVariant content = GetMockDocument();
  const CVariant published = CIPIDTestAccess::Publish(ipid, privateKey, content);

  EXPECT_EQ(ToJson(published), ToJson(content));
  ASSERT_EQ(mockIpfs->importedKeys.size(), 1U);
  ASSERT_GE(mockIpfs->removedKeys.size(), 1U);
  EXPECT_EQ(mockIpfs->removedKeys.back(), mockIpfs->importedKeys.front());
  EXPECT_EQ(mockIpfs->putDagCalls, 1U);
  EXPECT_EQ(mockIpfs->publishNameCalls, 1U);
  EXPECT_EQ(mockIpfs->lastPublishedPath, mockPath);
  EXPECT_EQ(mockIpfs->lastPublishedLifetimeSecs, 123U);
  EXPECT_EQ(mockIpfs->lastPublishedTtlSecs, 123U);
  EXPECT_EQ(mockIpfs->lastPublishedKeyName, mockIpfs->importedKeys.front());
}
