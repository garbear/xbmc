/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "crypto/Key.h"
#include "crypto/did/DIDUtils.h"
#include "crypto/did/IPID.h"
#include "datastore/BlockStore.h"
#include "datastore/DataStore.h"
#include "datastore/IDataStore.h"
#include "filesystem/IPFS.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI;
using namespace CRYPTO;

namespace
{
const std::string mockHash = "zdpuApA2CCoPHQEoP4nResbK2dq2zawFX3verNkMFmNbpDnXZ";

const std::string mockPath = "/ipfs/" + mockHash;

const std::string mockIpnsHash = "QmUTE4cxTxihntPEFqTprgbqyyS9YRaRcC8FXp6PACEjFG";

const std::string mockDid = "did:ipid:" + mockIpnsHash;

const std::string mockPem = "-----BEGIN RSA PRIVATE KEY-----\n"
                            "MIICXQIBAAKBgQDCQZyRCPMcBPL2J2SuI2TduR7sy28wmcRzfj8fXQTbj1zJURku\n"
                            "y93CIt4M6caXL29sgN2iAArLr73r2ezs+VGiCoAtIudl6qMwUG2O0hjdyiHDtCcj\n"
                            "w6Ic6LVCWr6HcyShTmvRGNC6ZdONgjOHVubzoev1lqxIEVMXzCMm7RkQOwIDAQAB\n"
                            "AoGBAKMi8teiingXd+tdPeI4ezbxhpUaa7CHEkJj3aL7PV8eULAI2XtBXmTxX0W8\n"
                            "9jh1b7/RoU+xdV+FoZv2klCZOQHCavqryGV7ffZlETtdxz2vmBHEh04j3eBcWCod\n"
                            "ppFhx3jx2EhYwIh1klHj1Ybl/r3MCR6aRhER5zCMCC1XSgVxAkEA9F60bp6imTSb\n"
                            "+C4CagcMiD36e+6K1CZ2refJ4T3dj88hqxjK9SKlji0aYqIK1sMNcEoeNjz6bn/u\n"
                            "1TyeCteWpwJBAMuAWCQwuA/4wKFB3OERB3gsBi+9yjJqZE9b648I+uTdbP1EHGVV\n"
                            "iHSVHxBQjOJ/vG48GrsWDBlSKsz6txCRQE0CQQC536NMlNtGv053er+ZWF0+8C2r\n"
                            "wKjWb59L7gePjRgO/9UzKDuQM9dLiqEMLwchjeGV7LqINN+j1ymaBm6L/qn3AkAI\n"
                            "9h/riBGy8ltZPpNBfgR8MEQdehgbXEAKlpuq8tRJm86e4I73j2qw55g0mbd6ifF8\n"
                            "UT1EG9ZwjwO/fxLssdjJAkBFTNbIqFnSkaVXIi51oXwqYl1/1h/MqoHoWdY0ZVCc\n"
                            "ttrI1rZSmCBbKkicdvBsJo2c916giPwGpcGIzlrt83sW\n"
                            "-----END RSA PRIVATE KEY-----";

CVariant GetMockDocument()
{
  CVariant mockDocument{CVariant::VariantTypeObject};

  mockDocument["@context"] = "https://w3id.org/did/v1";
  mockDocument["id"] = mockDid;
  mockDocument["created"] = "2019-03-19T16:52:44.948Z";
  mockDocument["created"] = "2019-03-19T16:53:56.463Z";

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

/*
class CMockIPFS : public XFILE::IIPFS
{
public:
  CMockIPFS() : m_keychainKeys{"key1", "key2"} {}
  ~CMockIPFS() override = default;

  // Implementation of IIPFS
  bool Initialize() override { return true; }
  void Deinitialize() override {}
  bool IsOnline() override { return true; }
  std::string ResolveName(const std::string& identifier) override { return mockPath; }
  void PublishName(const std::string& ipfsPath,
                   unsigned int lifetimeSecs,
                   unsigned int ttlSecs,
                   const std::string& keyName) override
  {
  }
  CVariant GetDAG(const std::string& cid) override { return GetMockDocument(); }
  std::string PutDAG(const CVariant& content) override { return mockHash; }
  std::vector<std::string> ListKeys() override { return m_keychainKeys; }
  void RemoveKey(const std::string& keyName) override
  {
    m_keychainKeys.erase(std::remove(m_keychainKeys.begin(), m_keychainKeys.end(), keyName),
                         m_keychainKeys.end());
  }
  void ImportKey(const std::string& keyName,
                 const PrivateKey& privateKey,
                 const std::string& password) override
  {
    RemoveKey(keyName);
    m_keychainKeys.emplace_back(keyName);
  }

private:
  std::vector<std::string> m_keychainKeys;
};
*/
} // namespace

TEST(TestIPID, ParseDID)
{
  //
  // Spec: Should parse DID into method and identifier
  //

  {
    std::string method;
    std::string identifier;
    EXPECT_TRUE(CDIDUtils::ParseDID(mockDid, method, identifier));

    EXPECT_EQ(method, "ipid");
    EXPECT_EQ(identifier, mockIpnsHash);
  }

  //
  // Spec: Should fail to parse invalid IPID DID
  //

  {
    std::string method;
    std::string identifier;

    EXPECT_FALSE(CDIDUtils::ParseDID("did:ipid:#", method, identifier));

    EXPECT_TRUE(method.empty());
    EXPECT_TRUE(identifier.empty());
  }
}

TEST(TestIPID, Factory)
{
  //
  // Spec: Should create IPID with all specification methods
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));
  }
}

TEST(TestIPID, Resolve)
{
  //
  // Spec: Should resolve successfully
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));

    const CVariant document = ipid.Resolve(mockDid);
    const CVariant mockDocument = GetMockDocument();

    std::string documentJson;
    std::string mockJson;

    EXPECT_TRUE(CJSONVariantWriter::Write(document, documentJson, false));
    EXPECT_TRUE(CJSONVariantWriter::Write(mockDocument, mockJson, false));

    //EXPECT_EQ(documentJson, mockJson);
  }

  //
  // Spec: Should fail if no IPNS record found
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));

    //! @todo
  }

  //
  // Spec: Should fail if can't get file
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));

    //! @todo
  }

  //
  // Spec: Should fail if document content is invalid
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));

    //! @todo
  }
}

TEST(TestIPID, Create)
{
  //
  // Spec: Should create successfully
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));

    //ipid.Create(mockPem, nullptr);
  }

  //
  // Spec: Should fail if document already exists
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));
  }

  //
  // Spec: Should fail if a document operation fails
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));
  }
}

TEST(TestIPID, Update)
{
  //
  // Spec: Should update successfully
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));
  }

  //
  // Spec: Should fail if no document available
  //

  {
    std::shared_ptr<XFILE::IIPFS> mockIpfs = std::make_shared<XFILE::CIPFS>();
    CIPID ipid(std::move(mockIpfs));
  }
}
