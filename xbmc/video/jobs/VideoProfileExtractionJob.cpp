/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoProfileExtractionJob.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationPlayer.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "jobs/JobManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "threads/SystemClock.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <cstring>
#include <mutex>
#include <unordered_set>

using namespace std::chrono_literals;

namespace
{
std::mutex g_stateMutex;
std::unordered_set<int> g_pendingFileIds;
std::unordered_set<int> g_attemptedFileIds;

std::string GetLogPath(const CFileItem& item)
{
  if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->m_strFileNameAndPath.empty())
    return CURL::GetRedacted(item.GetVideoInfoTag()->m_strFileNameAndPath);

  return CURL::GetRedacted(item.GetDynPath());
}

bool IsApplicationStopping()
{
  return g_application.IsStopping();
}

void ClearPending(int fileId)
{
  std::unique_lock lock(g_stateMutex);
  g_pendingFileIds.erase(fileId);
}

void MarkFileAttempted(int fileId)
{
  std::unique_lock lock(g_stateMutex);
  g_pendingFileIds.erase(fileId);
  g_attemptedFileIds.insert(fileId);
}

bool IsMediaPlaying()
{
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  return appPlayer && appPlayer->IsPlaying();
}

class CVideoProfileExtractionQueueGuard
{
public:
  explicit CVideoProfileExtractionQueueGuard(int fileId) : m_fileId(fileId) {}
  ~CVideoProfileExtractionQueueGuard()
  {
    if (m_markAttempted)
      MarkFileAttempted(m_fileId);
    else
      ClearPending(m_fileId);
  }

  void MarkAttempted() { m_markAttempted = true; }

private:
  int m_fileId;
  bool m_markAttempted{false};
};

class CVideoDatabaseAutoClose
{
public:
  explicit CVideoDatabaseAutoClose(CVideoDatabase& db) : m_db(db) {}
  ~CVideoDatabaseAutoClose() { m_db.Close(); }

private:
  CVideoDatabase& m_db;
};
} // namespace

CVideoProfileExtractionJob::CVideoProfileExtractionJob(const CFileItem& item, int fileId)
  : m_item(item),
    m_fileId(fileId)
{
}

bool CVideoProfileExtractionJob::IsNeeded(const CFileItem& item, int fileId)
{
  if (fileId < 0 || ::IsApplicationStopping() || !item.HasVideoInfoTag())
    return false;

  if (!item.GetVideoInfoTag()->m_streamDetails.HasUnscannedVideoProfile())
    return false;

  return CDVDFileInfo::CanExtract(item);
}

bool CVideoProfileExtractionJob::ShouldAbort() const
{
  return ::IsApplicationStopping();
}

const char* CVideoProfileExtractionJob::GetAbortReason() const
{
  return "application is shutting down";
}

bool CVideoProfileExtractionJob::WaitUntilIdleOrAbort() const
{
  bool loggedWaiting{false};

  while (IsMediaPlaying())
  {
    if (ShouldAbort())
      return false;

    if (!loggedWaiting)
    {
      CLog::LogF(LOGDEBUG,
                 "Lazy video profile extraction for file id {} waiting because playback is active",
                 m_fileId);
      loggedWaiting = true;
    }

    KODI::TIME::Sleep(100ms);
  }

  if (loggedWaiting)
  {
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction for file id {} resumed after playback stopped",
               m_fileId);
  }

  return !ShouldAbort();
}

bool CVideoProfileExtractionJob::PrepareHeavyPhase(const char* phase) const
{
  if (!WaitUntilIdleOrAbort())
  {
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction for file id {} aborted before {} because {}",
               m_fileId, phase, GetAbortReason());
    return false;
  }

  return true;
}

bool CVideoProfileExtractionJob::Queue(const CFileItem& item, int fileId)
{
  if (!IsNeeded(item, fileId))
    return false;

  {
    std::unique_lock lock(g_stateMutex);
    if (g_pendingFileIds.contains(fileId))
    {
      CLog::LogF(LOGDEBUG,
                 "Skipped lazy video profile extraction for file id {} because it is already "
                 "queued ({})",
                 fileId, GetLogPath(item));
      return false;
    }

    if (g_attemptedFileIds.contains(fileId))
    {
      CLog::LogF(LOGDEBUG,
                 "Skipped lazy video profile extraction for file id {} because it was already "
                 "attempted ({})",
                 fileId, GetLogPath(item));
      return false;
    }

    g_pendingFileIds.insert(fileId);
  }

  const unsigned int jobId = CServiceBroker::GetJobManager()->AddJob(
      new CVideoProfileExtractionJob(item, fileId), nullptr, CJob::PRIORITY_LOW_PAUSABLE);
  if (jobId == 0)
  {
    ClearPending(fileId);
    return false;
  }

  CLog::LogF(LOGDEBUG, "Queued lazy video profile extraction for file id {} ({})", fileId,
             GetLogPath(item));

  return true;
}

bool CVideoProfileExtractionJob::QueueIfNeeded(const CFileItem& item, int fileId)
{
  if (!IsNeeded(item, fileId))
    return false;

  if (!Queue(item, fileId))
    return false;

  return true;
}

bool CVideoProfileExtractionJob::Equals(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const auto* profileJob = dynamic_cast<const CVideoProfileExtractionJob*>(job);
  if (profileJob == nullptr)
    return false;

  return m_fileId == profileJob->m_fileId;
}

bool CVideoProfileExtractionJob::DoWork()
{
  CVideoProfileExtractionQueueGuard queueGuard(m_fileId);

  if (ShouldAbort())
  {
    CLog::LogF(LOGDEBUG, "Lazy video profile extraction for file id {} aborted because {}",
               m_fileId, GetAbortReason());
    return false;
  }

  CFileItem scannedItem(m_item);
  if (!PrepareHeavyPhase("media probing"))
    return false;

  if (ShouldAbort())
  {
    CLog::LogF(
        LOGDEBUG,
        "Lazy video profile extraction for file id {} aborted before media probing because {}",
        m_fileId, GetAbortReason());
    return false;
  }

  if (!CDVDFileInfo::GetFileStreamDetails(&scannedItem))
  {
    queueGuard.MarkAttempted();
    CLog::LogF(LOGDEBUG, "Lazy video profile extraction failed for file id {} ({})", m_fileId,
               GetLogPath(m_item));
    return false;
  }

  if (ShouldAbort())
  {
    CLog::LogF(
        LOGDEBUG,
        "Lazy video profile extraction for file id {} aborted after media probing because {}",
        m_fileId, GetAbortReason());
    return false;
  }

  const CVideoInfoTag* scannedInfo = scannedItem.GetVideoInfoTag();
  if (!scannedInfo)
  {
    queueGuard.MarkAttempted();
    return false;
  }

  CVideoDatabase db;
  if (!PrepareHeavyPhase("opening video database"))
    return false;

  if (!db.Open())
  {
    queueGuard.MarkAttempted();
    return false;
  }
  CVideoDatabaseAutoClose closeDb(db);

  if (ShouldAbort())
  {
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction for file id {} aborted after opening video database "
               "because {}",
               m_fileId, GetAbortReason());
    return false;
  }

  CVideoInfoTag currentInfo;
  currentInfo.m_iFileId = m_fileId;
  if (!PrepareHeavyPhase("reading stream details"))
    return false;

  if (!db.GetStreamDetails(currentInfo))
  {
    queueGuard.MarkAttempted();
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction could not load current stream details for file id {}",
               m_fileId);
    return false;
  }

  if (ShouldAbort())
  {
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction for file id {} aborted before merging stream details "
               "because {}",
               m_fileId, GetAbortReason());
    return false;
  }

  if (!currentInfo.m_streamDetails.UpdateMissingVideoProfilesFrom(scannedInfo->m_streamDetails))
  {
    queueGuard.MarkAttempted();
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction completed for file id {} but no DB update was needed",
               m_fileId);
    return true;
  }

  if (!PrepareHeavyPhase("writing stream details"))
    return false;

  if (!db.SetStreamDetailsForFileId(currentInfo.m_streamDetails, m_fileId))
  {
    queueGuard.MarkAttempted();
    CLog::LogF(LOGDEBUG, "Lazy video profile extraction failed to update DB for file id {}",
               m_fileId);
    return false;
  }

  if (ShouldAbort())
  {
    CLog::LogF(LOGDEBUG,
               "Lazy video profile extraction for file id {} aborted after updating DB because {}",
               m_fileId, GetAbortReason());
    return false;
  }

  queueGuard.MarkAttempted();
  CLog::LogF(LOGDEBUG, "Lazy video profile extraction updated DB for file id {} ({})", m_fileId,
             GetLogPath(m_item));

  return true;
}
