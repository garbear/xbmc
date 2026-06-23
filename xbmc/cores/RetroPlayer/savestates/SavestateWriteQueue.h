/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SavestateThumbnail.h"
#include "SavestateWriter.h"
#include "jobs/JobQueue.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
class CGUIGameMessenger;
struct SavestateWriteTracker;

class CSavestateWriteQueue
{
public:
  explicit CSavestateWriteQueue(CGUIGameMessenger& guiMessenger);
  ~CSavestateWriteQueue();

  void QueueSavestateWrite(SavestateWritePayload payload);
  void QueueThumbnailWrite(SavestateThumbnailPayload payload);
  void Wait();

private:
  CGUIGameMessenger& m_guiMessenger;
  CJobQueue m_savestateWriteQueue{false, 1, CJob::PRIORITY_LOW};
  CJobQueue m_thumbnailWriteQueue{false, 1, CJob::PRIORITY_LOW};
  std::shared_ptr<SavestateWriteTracker> m_tracker;
};
} // namespace RETRO
} // namespace KODI
