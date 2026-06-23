/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "jobs/Job.h"

class CVideoProfileExtractionJob : public CJob
{
public:
  static bool IsNeeded(const CFileItem& item, int fileId);
  static bool Queue(const CFileItem& item, int fileId);
  static bool QueueIfNeeded(const CFileItem& item, int fileId);

  CVideoProfileExtractionJob(const CFileItem& item, int fileId);

  bool DoWork() override;
  const char* GetType() const override { return "CVideoProfileExtractionJob"; }
  bool Equals(const CJob* job) const override;

private:
  bool ShouldAbort() const;
  const char* GetAbortReason() const;
  bool WaitUntilIdleOrAbort() const;
  bool PrepareHeavyPhase(const char* phase) const;

  CFileItem m_item;
  int m_fileId{-1};
};
