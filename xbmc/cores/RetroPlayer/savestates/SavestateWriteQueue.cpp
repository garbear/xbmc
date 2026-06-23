/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateWriteQueue.h"

#include "ServiceBroker.h"
#include "cores/RetroPlayer/guibridge/GUIGameMessenger.h"
#include "cores/RetroPlayer/rendering/RPRenderManager.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "utils/log.h"

#include <condition_variable>
#include <mutex>
#include <utility>

using namespace KODI;
using namespace RETRO;
using namespace KODI::MESSAGING;

namespace KODI::RETRO
{
struct SavestateWriteTracker
{
  std::mutex mutex;
  std::condition_variable cv;
  unsigned int pendingWrites{0};
  bool acceptingWrites{true};
};
} // namespace KODI::RETRO

namespace
{
void RefreshSavestateCallback(void* userptr);

struct SavestateRefreshMessage
{
  SavestateRefreshMessage(CGUIGameMessenger& guiMessenger, std::string savePath)
    : guiMessenger(guiMessenger),
      savePath(std::move(savePath))
  {
    callback.callback = &RefreshSavestateCallback;
    callback.userptr = this;
  }

  ThreadMessageCallback callback{};
  CGUIGameMessenger& guiMessenger;
  std::string savePath;
};

void RefreshSavestateCallback(void* userptr)
{
  std::unique_ptr<SavestateRefreshMessage> message(static_cast<SavestateRefreshMessage*>(userptr));

  message->guiMessenger.RefreshSavestates(message->savePath);
}

void DispatchSavestateRefresh(CGUIGameMessenger& guiMessenger, const std::string& savePath)
{
  auto message = std::make_unique<SavestateRefreshMessage>(guiMessenger, savePath);
  ThreadMessageCallback* callback = &message->callback;

  // The posted ThreadMessageCallback is embedded in SavestateRefreshMessage. Its userptr points
  // back to the owning message, and RefreshSavestateCallback runs on the application thread and
  // deletes that owner. SendMsg is synchronous, so the worker waits until refresh dispatch returns.
  message.release();
  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_CALLBACK, -1, -1, static_cast<void*>(callback));
}

class CSavestateWriteCompletion
{
public:
  explicit CSavestateWriteCompletion(std::shared_ptr<SavestateWriteTracker> tracker)
    : m_tracker(std::move(tracker))
  {
  }

  ~CSavestateWriteCompletion()
  {
    if (!m_tracker || !m_armed)
      return;

    std::unique_lock lock(m_tracker->mutex);
    if (m_tracker->pendingWrites > 0)
      --m_tracker->pendingWrites;
    m_tracker->cv.notify_all();
  }

  void Arm() { m_armed = true; }

private:
  std::shared_ptr<SavestateWriteTracker> m_tracker;
  bool m_armed{false};
};

class CSavestateWriteJob : public CJob
{
public:
  CSavestateWriteJob(SavestateWritePayload payload,
                     CGUIGameMessenger& guiMessenger,
                     std::shared_ptr<SavestateWriteTracker> tracker)
    : m_payload(std::move(payload)),
      m_guiMessenger(guiMessenger),
      m_completion(std::move(tracker))
  {
  }

  bool DoWork() override
  {
    CSavestateDatabase database;
    const bool success = CSavestateWriter::WritePayload(m_payload, database);
    if (success)
      DispatchSavestateRefresh(m_guiMessenger, m_payload.savePath);

    return success;
  }

  void ArmCompletion() { m_completion.Arm(); }

  const char* GetType() const override { return "savestate-write"; }

private:
  SavestateWritePayload m_payload;

  // CReversiblePlayback::Deinitialize() waits for all queued savestate writes before playback
  // teardown, and CGUIGameMessenger is owned by CRetroPlayer, which outlives CReversiblePlayback.
  // The worker must not outlive that wait.
  CGUIGameMessenger& m_guiMessenger;

  // Completion is queued job accounting, not write success. It intentionally runs when a
  // successfully queued job is destroyed, including normal completion or cancellation, so shutdown
  // cannot block forever.
  CSavestateWriteCompletion m_completion;
};

class CSavestateThumbnailWriteJob : public CJob
{
public:
  CSavestateThumbnailWriteJob(SavestateThumbnailPayload payload,
                              std::shared_ptr<SavestateWriteTracker> tracker)
    : m_payload(std::move(payload)),
      m_completion(std::move(tracker))
  {
  }

  bool DoWork() override { return CRPRenderManager::WriteThumbnailPayload(m_payload); }

  void ArmCompletion() { m_completion.Arm(); }

  const char* GetType() const override { return "savestate-thumbnail-write"; }

private:
  SavestateThumbnailPayload m_payload;

  CSavestateWriteCompletion m_completion;
};
} // namespace

CSavestateWriteQueue::CSavestateWriteQueue(CGUIGameMessenger& guiMessenger)
  : m_guiMessenger(guiMessenger),
    m_tracker(std::make_shared<SavestateWriteTracker>())
{
}

CSavestateWriteQueue::~CSavestateWriteQueue()
{
  Wait();
}

void CSavestateWriteQueue::QueueSavestateWrite(SavestateWritePayload payload)
{
  auto tracker = m_tracker;

  {
    std::unique_lock lock(tracker->mutex);
    if (!tracker->acceptingWrites)
    {
      CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Refusing to queue savestate write during shutdown");
      return;
    }

    ++tracker->pendingWrites;
  }

  auto* job = new CSavestateWriteJob(std::move(payload), m_guiMessenger, tracker);
  job->ArmCompletion();

  // Normal save/autosave writes are serialized by this one-at-a-time FIFO queue.
  // TODO: Coordinate savestate rename/delete operations with this queue as well.
  // CJobQueue::AddJob() owns the passed job on success and deletes it before returning on failure.
  if (!m_savestateWriteQueue.AddJob(job))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to queue savestate write job");
    return;
  }
}

void CSavestateWriteQueue::QueueThumbnailWrite(SavestateThumbnailPayload payload)
{
  auto tracker = m_tracker;

  {
    std::unique_lock lock(tracker->mutex);
    if (!tracker->acceptingWrites)
    {
      CLog::Log(LOGERROR,
                "RetroPlayer[SAVE]: Refusing to queue savestate thumbnail write during shutdown");
      return;
    }

    ++tracker->pendingWrites;
  }

  auto* job = new CSavestateThumbnailWriteJob(std::move(payload), tracker);
  job->ArmCompletion();

  // Thumbnail writes use a separate serialized queue so they can run in parallel with .sav writes.
  // CJobQueue::AddJob() owns the passed job on success and deletes it before returning on failure.
  if (!m_thumbnailWriteQueue.AddJob(job))
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to queue savestate thumbnail write job");
    return;
  }
}

void CSavestateWriteQueue::Wait()
{
  auto tracker = m_tracker;

  std::unique_lock lock(tracker->mutex);
  tracker->acceptingWrites = false;
  tracker->cv.wait(lock, [tracker]() { return tracker->pendingWrites == 0; });
}
