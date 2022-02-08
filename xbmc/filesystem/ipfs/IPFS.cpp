/*
 *  Copyright (C) 2022-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IPFS.h"

#include "ServiceBroker.h"
#include "datastore/Block.h"
#include "datastore/BlockStore.h"
#include "datastore/CID.h"
#include "datastore/DataStore.h"
#include "datastore/IDataStore.h"
#include "filesystem/ipfs/ZstdCompression.h"
#include "filesystem/ipfs/block/IPFSBlockUtils.h"
#include "filesystem/ipfs/unixfs/UnixFSDirectory.h"
#include "filesystem/ipfs/unixfs/UnixFSImporter.h"
#include "filesystem/ipfs/unixfs/UnixFSResolver.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
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

bool CIPFS::PublishName(const std::string& ipfsPath,
                        unsigned int lifetimeSecs,
                        unsigned int ttlSecs,
                        const std::string& keyName)
{
  //! @todo
  return false;
}

CVariant CIPFS::GetDAG(const std::string& cid)
{
  // Generic DAG JSON helper. File import/export uses CUnixFSJson and requires
  // the UnixFS JSON node shape.
  if (!m_blockStore)
    return CVariant{};

  DATASTORE::CCID ccid;
  if (!DATASTORE::CCID::FromString(cid, ccid))
    return CVariant{};

  DATASTORE::CBlock block;
  if (!m_blockStore->Get(ccid, block))
    return CVariant{};

  std::vector<uint8_t> jsonData;
  switch (ccid.Codec())
  {
    case DATASTORE::CIDCodec::DAG_JSON:
      if (!block.Empty())
        jsonData.assign(block.Data(), block.Data() + block.Size());
      break;
    case DATASTORE::CIDCodec::DAG_JSON_ZSTD:
      if (!IPFS::DecompressZstd(block.Data(), block.Size(), jsonData))
        return CVariant{};
      break;
    default:
      return CVariant{};
  }

  std::string json;
  if (!jsonData.empty())
    json.assign(reinterpret_cast<const char*>(jsonData.data()), jsonData.size());

  CVariant data;
  if (!CJSONVariantParser::Parse(json, data))
    return CVariant{};

  return data;
}

std::string CIPFS::PutDAG(const CVariant& content, DATASTORE::CIDCodec codec)
{
  // Generic DAG JSON helper. File import/export uses CUnixFSJson and requires
  // the UnixFS JSON node shape.
  if (!m_blockStore)
    return "";

  std::string json;
  if (!CJSONVariantWriter::Write(content, json, true))
    return "";

  std::vector<uint8_t> data;
  switch (codec)
  {
    case DATASTORE::CIDCodec::DAG_JSON:
      data.assign(json.begin(), json.end());
      break;
    case DATASTORE::CIDCodec::DAG_JSON_ZSTD:
      if (!IPFS::CompressZstd(reinterpret_cast<const uint8_t*>(json.data()), json.size(), data))
        return "";
      break;
    default:
      return "";
  }

  if (data.empty())
    return "";

  DATASTORE::CCID ccid;
  DATASTORE::CBlock block;
  if (!IPFS::CIPFSBlockUtils::MakeAddressedBlock(codec, data.data(), data.size(), ccid, block))
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

  return IPFS::UNIXFS::CImporter::AddFile(*m_blockStore, data, size, cidString);
}

bool CIPFS::GetFile(const std::string& cid, std::vector<uint8_t>& data)
{
  if (!m_blockStore)
    return false;

  return IPFS::UNIXFS::CResolver::ReadFile(*m_blockStore, cid, data);
}

bool CIPFS::IsDirectory(const std::string& cid)
{
  if (!m_blockStore)
    return false;

  return IPFS::UNIXFS::CDirectory::IsDirectory(*m_blockStore, cid);
}

bool CIPFS::ListDirectory(const std::string& cid, std::vector<CIPFSEntry>& entries)
{
  if (!m_blockStore)
    return false;

  std::vector<IPFS::UNIXFS::DirectoryEntry> unixfsEntries;
  if (!IPFS::UNIXFS::CDirectory::ListDirectory(*m_blockStore, cid, unixfsEntries))
    return false;

  std::vector<CIPFSEntry> output;
  output.reserve(unixfsEntries.size());
  for (const auto& entry : unixfsEntries)
  {
    CIPFSEntry ipfsEntry;
    ipfsEntry.name = entry.name;
    ipfsEntry.cid = entry.cid;
    ipfsEntry.size = entry.totalSize;

    switch (entry.type)
    {
      case IPFS::UNIXFS::DirectoryEntryType::Directory:
        ipfsEntry.type = CIPFSEntryType::Directory;
        break;
      case IPFS::UNIXFS::DirectoryEntryType::File:
      case IPFS::UNIXFS::DirectoryEntryType::Symlink:
        ipfsEntry.type = CIPFSEntryType::File;
        break;
      case IPFS::UNIXFS::DirectoryEntryType::Unknown:
      default:
        ipfsEntry.type = CIPFSEntryType::Unknown;
        break;
    }

    output.emplace_back(std::move(ipfsEntry));
  }

  entries = std::move(output);
  return true;
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
