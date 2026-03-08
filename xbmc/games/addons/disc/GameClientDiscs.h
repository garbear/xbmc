/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientSubsystem.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

struct AddonInstance_Game;
class CCriticalSection;

namespace KODI
{
namespace GAME
{

class CGameClient;
class CGameClientDiscModel;
class CGameClientDiscTransport;
class CGameClientDiscXML;

/*!
 * \ingroup games
 */
class CGameClientDiscs : protected CGameClientSubsystem
{
public:
  CGameClientDiscs(CGameClient& gameClient,
                   AddonInstance_Game& addonStruct,
                   CCriticalSection& clientAccess);
  ~CGameClientDiscs() override;

  // Game client capabilities
  bool SupportsDiskControl() const;

  // Disc interface
  void Initialize(const std::string& gamePath);
  void RestoreDiscList();
  void RefreshDiscState();
  const CGameClientDiscModel& GetDiscs() const { return *m_discModel; }
  bool IsEjected() const { return m_isEjected; }
  bool SetEjected(bool ejected);
  bool AddDisc(const std::string& filePath);
  bool RemoveDisc(const std::string& filePath);
  bool RemoveDiscByIndex(size_t index);
  bool InsertDisc(const std::string& filePath);
  bool InsertDiscByIndex(size_t index);

private:
  void LoadModelFromCore(CGameClientDiscModel& model) const;
  void PruneRemovedDiscs(CGameClientDiscModel& model) const;
  void SaveDiscState();

  // Add-on parameters
  std::unique_ptr<CGameClientDiscTransport> m_transport;
  std::unique_ptr<CGameClientDiscXML> m_discXml;

  // Game parameters
  std::unique_ptr<CGameClientDiscModel> m_discModel;
  std::atomic<bool> m_isEjected{false};
};
} // namespace GAME
} // namespace KODI
