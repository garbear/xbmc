/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFS.h"

#include "ServiceBroker.h"
#include "UnixFS.h"
#include "datastore/Block.h"
#include "datastore/BlockStore.h"
#include "datastore/CID.h"
#include "datastore/DataStore.h"
#include "datastore/IDataStore.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Digest.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <utility>
#include <vector>

using namespace KODI;
using namespace XFILE;

namespace
{
// Name of the data store
constexpr const char* DATA_STORE_NAME = "ipfs";

// Hash at the ingestion boundary and carry the resulting CID with the block.
// Reads address blocks by CID and do not rehash unless explicit validation is
// requested.
bool MakeAddressedBlock(DATASTORE::CIDCodec codec,
                        const uint8_t* data,
                        size_t size,
                        DATASTORE::CCID& cid,
                        DATASTORE::CBlock& block)
{
  if (data == nullptr && size != 0)
    return false;

  UTILITY::CDigest digest{UTILITY::CDigest::Type::SHA256};
  if (data != nullptr && size != 0)
    digest.Update(data, size);

  const std::string rawDigest = digest.FinalizeRaw();
  std::vector<uint8_t> multihash;
  multihash.reserve(2 + rawDigest.size());
  multihash.push_back(0x12);
  multihash.push_back(0x20);
  multihash.insert(multihash.end(), rawDigest.begin(), rawDigest.end());

  DATASTORE::CCID newCid{codec, std::move(multihash)};
  std::vector<uint8_t> blockData;
  if (data != nullptr && size != 0)
    blockData.assign(data, data + size);

  DATASTORE::CBlock newBlock{newCid, std::move(blockData)};

  cid = std::move(newCid);
  block = std::move(newBlock);
  return true;
}
} // namespace

CIPFS::CIPFS() = default;

CIPFS::~CIPFS()
{
  Deinitialize();
}

bool CIPFS::Initialize(const std::string& dataStoreRoot)
{
  // Validate parameters
  if (dataStoreRoot.empty())
    return false;

  const std::string dataStorePath = URIUtils::AddFileToFolder(dataStoreRoot, DATA_STORE_NAME);

  m_dataStore = std::make_unique<DATASTORE::CDataStore>();
  if (m_dataStore && m_dataStore->Open(dataStorePath))
  {
    m_blockStore = std::make_unique<DATASTORE::CBlockStore>(*m_dataStore);
    return true;
  }

  return false;
}

void CIPFS::Deinitialize()
{
  m_blockStore.reset();
  m_dataStore.reset();
}

bool CIPFS::IsOnline()
{
  //! @todo
  return false;
}

std::string CIPFS::ResolveName(const std::string& identifier)
{
  //! @todo
  return "";
}

void CIPFS::PublishName(const std::string& ipfsPath,
                        unsigned int lifetimeSecs,
                        unsigned int ttlSecs,
                        const std::string& keyName)
{
  //! @todo
}

CVariant CIPFS::GetDAG(const std::string& cid)
{
  if (!m_blockStore)
    return CVariant{};

  DATASTORE::CCID ccid;
  if (!DATASTORE::CCID::FromString(cid, ccid))
    return CVariant{};

  DATASTORE::CBlock block;
  if (!m_blockStore->Get(ccid, block))
    return CVariant{};

  //! @todo zstd-decompress block
  std::string json;
  if (!block.Empty())
    json.assign(reinterpret_cast<const char*>(block.Data()), block.Size());

  CVariant data;
  if (!CJSONVariantParser::Parse(json, data))
    return CVariant{};

  return data;
}

std::string CIPFS::PutDAG(const CVariant& content)
{
  if (!m_blockStore)
    return "";

  std::string json;
  if (!CJSONVariantWriter::Write(content, json, true))
    return "";

  //! @todo zstd-compress data
  std::vector<uint8_t> data{json.begin(), json.end()};
  if (data.empty())
    return "";

  DATASTORE::CCID ccid;
  DATASTORE::CBlock block;
  if (!MakeAddressedBlock(DATASTORE::CIDCodec::DAG_JSON, data.data(), data.size(), ccid, block))
    return "";

  if (!m_blockStore->Put(block))
    return "";

  const std::string cid = ccid.ToString();
  if (cid.empty())
    return "";

  return cid;
}

bool CIPFS::AddFile(const uint8_t* data, size_t size, std::string& cidString)
{
  if (!m_blockStore)
    return false;

  DATASTORE::CCID cid;
  DATASTORE::CBlock block;
  if (!MakeAddressedBlock(DATASTORE::CIDCodec::RAW, data, size, cid, block))
    return false;

  if (!m_blockStore->Put(block))
    return false;

  std::string newCidString = cid.ToString();
  if (newCidString.empty())
    return false;

  cidString = std::move(newCidString);
  return true;
}

bool CIPFS::GetFile(const std::string& cid, std::vector<uint8_t>& data)
{
  if (!m_blockStore)
    return false;

  DATASTORE::CCID ccid;
  if (!DATASTORE::CCID::FromString(cid, ccid))
    return false;

  DATASTORE::CBlock block;
  if (!m_blockStore->Get(ccid, block))
    return false;

  std::vector<uint8_t> output;
  if (block.Data() != nullptr && block.Size() != 0)
    output.assign(block.Data(), block.Data() + block.Size());

  if (ccid.Codec() == DATASTORE::CIDCodec::RAW)
  {
    data = std::move(output);
    return true;
  }

  if (ccid.Codec() == DATASTORE::CIDCodec::DAG_FLATBUFFER)
  {
    XFILE::IPFS::UnixFSNode node;
    if (!XFILE::IPFS::CUnixFS::DecodeNode(output.data(), output.size(), node))
      return false;

    data = std::move(node.data);
    return true;
  }

  return false;
}

std::vector<std::string> CIPFS::ListKeys()
{
  //! @todo
  return {};
}

void CIPFS::RemoveKey(const std::string& keyName)
{
  //! @todo
}

void CIPFS::ImportKey(const std::string& keyName,
                      const KODI::CRYPTO::PrivateKey& privateKey,
                      const std::string& password)
{
  //! @todo
}
