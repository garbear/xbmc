/*
 *  Copyright (C) 2022 Team Kodi
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
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

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
  return static_cast<bool>(m_blockStore);
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

  //! @todo
  std::vector<uint8_t> multihash{cid.begin(), cid.end()};

  // Get classed CID
  const DATASTORE::CCID ccid{DATASTORE::CIDCodec::DAG_JSON, std::move(multihash)};

  DATASTORE::CBlock block;
  if (!m_blockStore->Get(ccid, block))
    return CVariant{};

  //! @todo lz4-decompress block
  std::string json{block.Data().begin(), block.Data().end()};

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

  //! @todo lz4-compress data
  std::vector<uint8_t> data{json.begin(), json.end()};
  if (data.empty())
    return "";

  std::vector<uint8_t> multihash = {}; //! @todo MakeMultihash(data);

  //! Get classed CID
  DATASTORE::CCID ccid{DATASTORE::CIDCodec::DAG_JSON, multihash};

  DATASTORE::CBlock block{std::move(ccid), std::move(data)};

  if (!m_blockStore->Put(block))
    return "";

  //! @todo
  std::string cid{multihash.begin(), multihash.end()};
  return cid;
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
