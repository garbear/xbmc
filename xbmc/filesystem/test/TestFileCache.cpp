/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/FileCache.h"
#include "threads/Event.h"

#if !defined(TARGET_WINDOWS)
#include "platform/posix/ConvUtils.h"
#endif

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace XFILE;

namespace
{
class CGatedFileCacheSource : public IFileCacheSource
{
public:
  enum class Operation
  {
    READ,
    SEEK,
  };

  struct SeekOutcome
  {
    int64_t result;
    int error;
  };

  bool Open(const CURL& url, unsigned int flags) override
  {
    m_openAborted = false;
    m_firstRead = true;
    m_positionUncertain = false;
    m_operationRecorded = false;
    m_position = 0;
    m_nextSeekOutcome = 0;
    m_abortCalls = 0;
    m_blockSeek = false;
    m_blockPostSeekRead = false;
    m_sourceSeekCompleted = false;
    m_recordNextOperation = false;
    m_abortEntered.Reset();
    m_openEntered.Set();
    if (m_blockOpen)
      m_allowOpen.Wait();
    if (m_openAborted && !m_openSucceedsAfterAbort)
      return false;
    m_firstReadEntered.Reset();
    m_seekEntered.Reset();
    m_sourceSeekCompletedEvent.Reset();
    m_postSeekReadEntered.Reset();
    m_allowFirstRead.Reset();
    m_allowSeek.Set();
    m_allowPostSeekRead.Set();
    return true;
  }

  void Close() override
  {
    ++m_closeCalls;
    m_allowFirstRead.Set();
    m_allowSeek.Set();
    m_allowPostSeekRead.Set();
  }

  void Abort() override
  {
    ++m_abortCalls;
    m_openAborted = true;
    m_abortEntered.Set();
    if (m_abortReleasesOpen)
      m_allowOpen.Set();
    m_allowFirstRead.Set();
    m_allowSeek.Set();
    m_allowPostSeekRead.Set();
  }

  ssize_t Read(void* buffer, size_t size) override
  {
    if (m_recordNextOperation.exchange(false))
    {
      m_nextOperation.set_value(Operation::READ);
      m_operationRecorded = true;
    }

    if (m_firstRead)
    {
      m_firstRead = false;
      m_firstReadEntered.Set();
      m_allowFirstRead.Wait();
      const size_t count = std::min(size, m_firstReadSize);
      for (size_t index = 0; index < count; ++index)
        static_cast<unsigned char*>(buffer)[index] = static_cast<unsigned char>(index);
      m_position += count;
      return static_cast<ssize_t>(count);
    }

    if (m_blockPostSeekRead && m_sourceSeekCompleted)
    {
      m_postSeekReadEntered.Set();
      m_allowPostSeekRead.Wait();
      return 0;
    }

    if (m_positionUncertain)
    {
      static_cast<unsigned char*>(buffer)[0] = 0xa5;
      ++m_position;
      return 1;
    }

    return 0;
  }

  int64_t Seek(int64_t position, int whence) override
  {
    if (m_recordNextOperation.exchange(false))
    {
      m_nextOperation.set_value(Operation::SEEK);
      m_operationRecorded = true;
    }

    m_seekEntered.Set();
    if (m_blockSeek)
      m_allowSeek.Wait();
    if (m_throwOnSeek)
      throw std::runtime_error{"injected seek failure"};

    int64_t result = position;
    int error = 0;
    if (m_nextSeekOutcome < m_seekOutcomes.size())
    {
      const SeekOutcome outcome = m_seekOutcomes[m_nextSeekOutcome++];
      m_position = position;
      m_positionUncertain = outcome.result != position;
      if (m_positionUncertain && !m_operationRecorded)
        m_recordNextOperation = true;
      result = outcome.result;
      error = outcome.error;
    }
    else
    {
      m_position = position;
      m_positionUncertain = false;
    }

    SetLastError(error);
    m_sourceSeekCompleted = true;
    m_sourceSeekCompletedEvent.Set();
    return result;
  }

  int64_t GetLength() override { return 1024 * 1024; }
  int GetChunkSize() override { return 64 * 1024; }
  int IoControl(IOControl request, void* param) override
  {
    if (request == IOControl::SEEK_POSSIBLE)
    {
      SetLastError(EACCES);
      return 1;
    }
    return 0;
  }
  IFile* GetImplementation() override { return nullptr; }

  void AddSeekOutcome(int64_t result, int error)
  {
    m_seekOutcomes.emplace_back(SeekOutcome{result, error});
  }
  void SetFirstReadSize(size_t size) { m_firstReadSize = size; }
  void BlockOpen(bool abortReleasesOpen = true, bool openSucceedsAfterAbort = false)
  {
    m_blockOpen = true;
    m_abortReleasesOpen = abortReleasesOpen;
    m_openSucceedsAfterAbort = openSucceedsAfterAbort;
    m_allowOpen.Reset();
  }
  void BlockSeek()
  {
    m_blockSeek = true;
    m_allowSeek.Reset();
  }
  void ThrowOnSeek() { m_throwOnSeek = true; }
  void BlockPostSeekRead()
  {
    m_blockPostSeekRead = true;
    m_allowPostSeekRead.Reset();
  }
  std::future<Operation> GetNextOperation() { return m_nextOperation.get_future(); }
  bool WaitForFirstRead(std::chrono::milliseconds timeout)
  {
    return m_firstReadEntered.Wait(timeout);
  }
  bool WaitForOpen(std::chrono::milliseconds timeout) { return m_openEntered.Wait(timeout); }
  bool WaitForAbort(std::chrono::milliseconds timeout) { return m_abortEntered.Wait(timeout); }
  bool WaitForSeek(std::chrono::milliseconds timeout) { return m_seekEntered.Wait(timeout); }
  bool WaitForSourceSeekCompletion(std::chrono::milliseconds timeout)
  {
    return m_sourceSeekCompletedEvent.Wait(timeout);
  }
  bool WaitForPostSeekRead(std::chrono::milliseconds timeout)
  {
    return m_postSeekReadEntered.Wait(timeout);
  }
  void AllowFirstRead() { m_allowFirstRead.Set(); }
  void AllowOpen() { m_allowOpen.Set(); }
  void AllowSeek() { m_allowSeek.Set(); }
  void AllowPostSeekRead() { m_allowPostSeekRead.Set(); }
  unsigned int GetAbortCalls() const { return m_abortCalls; }
  unsigned int GetCloseCalls() const { return m_closeCalls; }

private:
  bool m_firstRead{true};
  bool m_positionUncertain{false};
  bool m_operationRecorded{false};
  int64_t m_position{0};
  size_t m_firstReadSize{1};
  size_t m_nextSeekOutcome{0};
  std::vector<SeekOutcome> m_seekOutcomes;
  std::atomic<bool> m_blockOpen{false};
  std::atomic<bool> m_openAborted{false};
  std::atomic<bool> m_abortReleasesOpen{true};
  std::atomic<bool> m_openSucceedsAfterAbort{false};
  std::atomic<bool> m_blockSeek{false};
  std::atomic<bool> m_throwOnSeek{false};
  std::atomic<bool> m_blockPostSeekRead{false};
  std::atomic<bool> m_sourceSeekCompleted{false};
  std::atomic<unsigned int> m_abortCalls{0};
  std::atomic<unsigned int> m_closeCalls{0};
  std::atomic<bool> m_recordNextOperation{false};
  std::promise<Operation> m_nextOperation;
  CEvent m_openEntered{true};
  CEvent m_allowOpen{true};
  CEvent m_abortEntered{true};
  CEvent m_firstReadEntered{true};
  CEvent m_allowFirstRead{true};
  CEvent m_seekEntered{true};
  CEvent m_allowSeek{true};
  CEvent m_sourceSeekCompletedEvent{true};
  CEvent m_postSeekReadEntered{true};
  CEvent m_allowPostSeekRead{true};
};

class TestFileCache : public CFileCache
{
public:
  TestFileCache(unsigned int flags, std::unique_ptr<IFileCacheSource> source)
    : CFileCache(flags, std::move(source))
  {
  }
};

struct SeekResult
{
  int64_t position;
  DWORD error;
};

SeekResult SeekWithError(CFileCache& cache, int64_t position)
{
  const int64_t result = cache.Seek(position, SEEK_SET);
  return {result, GetLastError()};
}
} // namespace

TEST(TestFileCache, AbortCancelsBlockedSourceOpen)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->BlockOpen();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  auto openResult =
      std::async(std::launch::async, [&]() { return cache.Open(CURL{"mock://server/movie.mkv"}); });
  const bool openEntered = sourcePtr->WaitForOpen(5s);
  const bool openWasWaiting = openResult.wait_for(0ms) == std::future_status::timeout;

  cache.Abort();
  const bool openCanceled = openResult.wait_for(1s) == std::future_status::ready;
  const bool openReady = openResult.wait_for(5s) == std::future_status::ready;
  bool opened = true;
  if (openReady)
    opened = openResult.get();
  cache.Close();

  EXPECT_TRUE(openEntered);
  EXPECT_TRUE(openWasWaiting);
  EXPECT_TRUE(openCanceled);
  ASSERT_TRUE(openReady);
  EXPECT_FALSE(opened);
  EXPECT_EQ(1U, sourcePtr->GetAbortCalls());
  EXPECT_FALSE(cache.IsRunning());
}

TEST(TestFileCache, CloseCancelsRacingSuccessfulSourceOpen)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->BlockOpen(false, true);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  auto openResult = std::async(std::launch::async,
                               [&]()
                               {
                                 const bool opened = cache.Open(CURL{"mock://server/movie.mkv"});
                                 return std::pair{opened, GetLastError()};
                               });
  const bool openEntered = sourcePtr->WaitForOpen(5s);
  auto closeResult = std::async(std::launch::async, [&]() { cache.Close(); });
  const bool abortEntered = sourcePtr->WaitForAbort(5s);
  const bool openWasWaiting = openResult.wait_for(0ms) == std::future_status::timeout;
  const bool closeWasWaiting = closeResult.wait_for(0ms) == std::future_status::timeout;

  sourcePtr->AllowOpen();
  const bool openReady = openResult.wait_for(5s) == std::future_status::ready;
  const bool closeReady = closeResult.wait_for(5s) == std::future_status::ready;
  std::pair<bool, DWORD> result{true, 0};
  if (openReady)
    result = openResult.get();
  if (closeReady)
    closeResult.get();

  EXPECT_TRUE(openEntered);
  EXPECT_TRUE(abortEntered);
  EXPECT_TRUE(openWasWaiting);
  EXPECT_TRUE(closeWasWaiting);
  ASSERT_TRUE(openReady);
  ASSERT_TRUE(closeReady);
  EXPECT_FALSE(result.first);
  EXPECT_EQ(ECANCELED, result.second);
  EXPECT_EQ(1U, sourcePtr->GetAbortCalls());
  EXPECT_GE(sourcePtr->GetCloseCalls(), 1U);
  EXPECT_FALSE(sourcePtr->WaitForFirstRead(0ms));
  EXPECT_FALSE(cache.IsRunning());
}

TEST(TestFileCache, NormalSourceSeekReturnsRequestedPosition)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto seekResult =
      std::async(std::launch::async, [&]() { return cache.Seek(256 * 1024, SEEK_SET); });
  sourcePtr->AllowFirstRead();

  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(seekEntered);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(256 * 1024, seekResult.get());
}

TEST(TestFileCache, CloseAllowsLaterOpen)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/first.mkv"}));
  ASSERT_TRUE(sourcePtr->WaitForFirstRead(5s));
  sourcePtr->AllowFirstRead();
  cache.Close();

  ASSERT_TRUE(cache.Open(CURL{"mock://server/second.mkv"}));
  ASSERT_TRUE(sourcePtr->WaitForFirstRead(5s));
  sourcePtr->AllowFirstRead();
  cache.Close();

  EXPECT_FALSE(cache.IsRunning());
}

TEST(TestFileCache, FailedSourceSeekPropagatesErrorAndQuarantinesReads)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  auto nextOperation = sourcePtr->GetNextOperation();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto firstSeek =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();

  const bool firstSeekEntered = sourcePtr->WaitForSeek(5s);
  const bool firstSeekReady = firstSeek.wait_for(5s) == std::future_status::ready;
  SeekResult firstResult{};
  if (firstSeekReady)
    firstResult = firstSeek.get();
  else
  {
    cache.Close();
    ASSERT_TRUE(firstSeekReady);
  }

  auto secondSeek = std::async(std::launch::async, [&]() { return cache.Seek(0, SEEK_SET); });
  const bool operationReady = nextOperation.wait_for(5s) == std::future_status::ready;
  CGatedFileCacheSource::Operation operation{CGatedFileCacheSource::Operation::READ};
  if (operationReady)
    operation = nextOperation.get();
  const bool secondSeekReady = secondSeek.wait_for(5s) == std::future_status::ready;
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(firstSeekEntered);
  ASSERT_TRUE(firstSeekReady);
  EXPECT_EQ(-1, firstResult.position);
  EXPECT_EQ(ECONNRESET, firstResult.error);
  ASSERT_TRUE(operationReady);
  EXPECT_EQ(CGatedFileCacheSource::Operation::SEEK, operation);
  ASSERT_TRUE(secondSeekReady);
  EXPECT_EQ(0, secondSeek.get());
}

TEST(TestFileCache, WrongPositiveSourceSeekResultIsFailure)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->AddSeekOutcome(256 * 1024 + 1, 0);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();

  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  SeekResult result{};
  if (seekReady)
    result = seekResult.get();
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(seekEntered);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(-1, result.position);
  EXPECT_EQ(EIO, result.error);
}

TEST(TestFileCache, FailedSourceSeekLeavesCacheReadPositionUnchanged)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->SetFirstReadSize(64);
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto firstSeek =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();
  const bool firstSeekReady = firstSeek.wait_for(5s) == std::future_status::ready;
  SeekResult firstResult{};
  if (firstSeekReady)
    firstResult = firstSeek.get();
  else
  {
    cache.Close();
    ASSERT_TRUE(firstSeekReady);
  }

  auto secondSeek = std::async(std::launch::async, [&]() { return SeekWithError(cache, 32); });
  const bool secondSeekReady = secondSeek.wait_for(5s) == std::future_status::ready;
  SeekResult secondResult{};
  if (secondSeekReady)
    secondResult = secondSeek.get();
  else
  {
    cache.Close();
    ASSERT_TRUE(secondSeekReady);
  }

  unsigned char value = 0xff;
  const ssize_t bytesRead = cache.Read(&value, 1);
  const int64_t position = cache.GetPosition();
  cache.Close();

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(firstSeekReady);
  ASSERT_TRUE(secondSeekReady);
  EXPECT_EQ(-1, firstResult.position);
  EXPECT_EQ(-1, secondResult.position);
  EXPECT_EQ(1, bytesRead);
  EXPECT_EQ(0, value);
  EXPECT_EQ(1, position);
}

TEST(TestFileCache, ReadFailsPromptlyAfterQuarantinedCacheDrains)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->AddSeekOutcome(-1, ECONNRESET);
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  const bool readEntered = sourcePtr->WaitForFirstRead(5s);
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  if (!seekReady)
  {
    cache.Close();
    ASSERT_TRUE(seekReady);
  }

  unsigned char value = 0xff;
  const ssize_t cachedRead = cache.Read(&value, 1);
  auto failedRead = std::async(std::launch::async,
                               [&]()
                               {
                                 unsigned char nextValue{};
                                 const ssize_t result = cache.Read(&nextValue, 1);
                                 return std::pair{result, GetLastError()};
                               });
  const bool readFailedPromptly = failedRead.wait_for(1s) == std::future_status::ready;
  cache.Close();
  const bool failedReadReady = failedRead.wait_for(5s) == std::future_status::ready;

  ASSERT_TRUE(readEntered);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(-1, seekResult.get().position);
  EXPECT_EQ(1, cachedRead);
  EXPECT_EQ(0, value);
  ASSERT_TRUE(readFailedPromptly);
  ASSERT_TRUE(failedReadReady);
  const auto [readResult, readError] = failedRead.get();
  EXPECT_EQ(-1, readResult);
  EXPECT_EQ(ECONNRESET, readError);
}

TEST(TestFileCache, CloseCancelsBlockedSourceSeek)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  ASSERT_TRUE(sourcePtr->WaitForFirstRead(5s));
  sourcePtr->BlockSeek();
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();
  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool callerWasWaiting = seekResult.wait_for(0ms) == std::future_status::timeout;

  auto closeResult = std::async(std::launch::async, [&]() { cache.Close(); });
  const bool seekCanceled = seekResult.wait_for(1s) == std::future_status::ready;
  const bool closeFinished = closeResult.wait_for(1s) == std::future_status::ready;

  sourcePtr->AllowSeek();
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  const bool closeReady = closeResult.wait_for(5s) == std::future_status::ready;
  SeekResult result{};
  if (seekReady)
    result = seekResult.get();
  if (closeReady)
    closeResult.get();

  EXPECT_TRUE(seekEntered);
  EXPECT_TRUE(callerWasWaiting);
  EXPECT_TRUE(seekCanceled);
  EXPECT_TRUE(closeFinished);
  ASSERT_TRUE(seekReady);
  ASSERT_TRUE(closeReady);
  EXPECT_EQ(-1, result.position);
  EXPECT_EQ(ECANCELED, result.error);
  EXPECT_EQ(1U, sourcePtr->GetAbortCalls());
  EXPECT_FALSE(cache.IsRunning());

  cache.Close();
}

TEST(TestFileCache, AbortWakesSeekWaitingForPostSeekData)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  ASSERT_TRUE(sourcePtr->WaitForFirstRead(5s));
  sourcePtr->BlockPostSeekRead();
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 1024 * 1024 - 1); });
  sourcePtr->AllowFirstRead();

  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool sourceSeekCompleted = sourcePtr->WaitForSourceSeekCompletion(5s);
  const bool postSeekReadEntered = sourcePtr->WaitForPostSeekRead(5s);
  const bool callerWasWaiting = seekResult.wait_for(0ms) == std::future_status::timeout;

  const auto abortStart = std::chrono::steady_clock::now();
  cache.Abort();
  cache.Abort();
  const auto abortDuration = std::chrono::steady_clock::now() - abortStart;
  const bool seekCanceled = seekResult.wait_for(1s) == std::future_status::ready;

  sourcePtr->AllowPostSeekRead();
  const bool seekReady = seekResult.wait_for(5s) == std::future_status::ready;
  SeekResult result{};
  if (seekReady)
    result = seekResult.get();
  cache.Close();
  cache.Close();

  EXPECT_TRUE(seekEntered);
  EXPECT_TRUE(sourceSeekCompleted);
  EXPECT_TRUE(postSeekReadEntered);
  EXPECT_TRUE(callerWasWaiting);
  EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(abortDuration).count(), 1000);
  EXPECT_TRUE(seekCanceled);
  ASSERT_TRUE(seekReady);
  EXPECT_EQ(-1, result.position);
  EXPECT_EQ(ECANCELED, result.error);
  EXPECT_EQ(1U, sourcePtr->GetAbortCalls());
  EXPECT_FALSE(cache.IsRunning());
}

TEST(TestFileCache, SeekReturnsWhenWorkerExitsUnexpectedly)
{
  using namespace std::chrono_literals;

  auto source = std::make_unique<CGatedFileCacheSource>();
  auto* sourcePtr = source.get();
  sourcePtr->ThrowOnSeek();
  TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

  ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
  ASSERT_TRUE(sourcePtr->WaitForFirstRead(5s));
  auto seekResult =
      std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
  sourcePtr->AllowFirstRead();

  const bool seekEntered = sourcePtr->WaitForSeek(5s);
  const bool seekFinishedPromptly = seekResult.wait_for(1s) == std::future_status::ready;
  if (!seekFinishedPromptly)
    cache.Abort();
  const bool seekFinished = seekResult.wait_for(5s) == std::future_status::ready;
  SeekResult result{};
  if (seekFinished)
    result = seekResult.get();
  cache.Close();

  EXPECT_TRUE(seekEntered);
  EXPECT_TRUE(seekFinishedPromptly);
  ASSERT_TRUE(seekFinished);
  EXPECT_EQ(-1, result.position);
  EXPECT_EQ(EIO, result.error);
  EXPECT_FALSE(cache.IsRunning());
}

TEST(TestFileCache, CloseRacingSuccessfulSeekCompletionIsSafe)
{
  using namespace std::chrono_literals;

  for (unsigned int iteration = 0; iteration < 25; ++iteration)
  {
    auto source = std::make_unique<CGatedFileCacheSource>();
    auto* sourcePtr = source.get();
    TestFileCache cache{READ_AUDIO_VIDEO, std::move(source)};

    ASSERT_TRUE(cache.Open(CURL{"mock://server/movie.mkv"}));
    ASSERT_TRUE(sourcePtr->WaitForFirstRead(5s));
    sourcePtr->BlockSeek();
    auto seekResult =
        std::async(std::launch::async, [&]() { return SeekWithError(cache, 256 * 1024); });
    sourcePtr->AllowFirstRead();
    const bool seekEntered = sourcePtr->WaitForSeek(5s);

    CEvent raceStart{true};
    auto allowSeek = std::async(std::launch::async,
                                [&]()
                                {
                                  raceStart.Wait();
                                  sourcePtr->AllowSeek();
                                });
    auto closeResult = std::async(std::launch::async,
                                  [&]()
                                  {
                                    raceStart.Wait();
                                    cache.Close();
                                  });
    raceStart.Set();

    const bool allowReady = allowSeek.wait_for(1s) == std::future_status::ready;
    const bool seekReady = seekResult.wait_for(1s) == std::future_status::ready;
    const bool closeReady = closeResult.wait_for(1s) == std::future_status::ready;
    sourcePtr->AllowSeek();
    if (allowReady)
      allowSeek.get();

    const bool seekFinished = seekResult.wait_for(5s) == std::future_status::ready;
    const bool closeFinished = closeResult.wait_for(5s) == std::future_status::ready;
    SeekResult result{};
    if (seekFinished)
      result = seekResult.get();
    if (closeFinished)
      closeResult.get();

    EXPECT_TRUE(seekEntered) << "iteration " << iteration;
    EXPECT_TRUE(allowReady) << "iteration " << iteration;
    EXPECT_TRUE(seekReady) << "iteration " << iteration;
    EXPECT_TRUE(closeReady) << "iteration " << iteration;
    ASSERT_TRUE(seekFinished) << "iteration " << iteration;
    ASSERT_TRUE(closeFinished) << "iteration " << iteration;
    if (result.position == -1)
      EXPECT_EQ(ECANCELED, result.error) << "iteration " << iteration;
    else
      EXPECT_EQ(256 * 1024, result.position) << "iteration " << iteration;
    EXPECT_FALSE(cache.IsRunning()) << "iteration " << iteration;
  }
}
