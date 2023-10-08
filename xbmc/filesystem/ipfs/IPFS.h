/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IIPFS.h"

#include <memory>

namespace KODI
{
namespace DATASTORE
{
class CBlockStore;
class CDataStore;
} // namespace DATASTORE
} // namespace KODI

namespace XFILE
{

class CIPFS : public IIPFS
{
public:
  CIPFS();
  ~CIPFS() override;

  // Implementation of IIPFS
  bool Initialize(const std::string& dataStoreRoot) override;
  void Deinitialize() override;
  bool IsOnline() override;
  CVariant GetDAG(const std::string& cid) override;
  std::string PutDAG(const CVariant& content) override;

private:
  std::unique_ptr<KODI::DATASTORE::CDataStore> m_dataStore;
  std::unique_ptr<KODI::DATASTORE::CBlockStore> m_blockStore;
};

} // namespace XFILE
