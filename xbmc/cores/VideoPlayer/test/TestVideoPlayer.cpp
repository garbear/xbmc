/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/IPlayerCallback.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStream.h"
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStreamFile.h"
#include "cores/VideoPlayer/VideoPlayer.h"
#include "filesystem/File.h"
#include "filesystem/IFileTypes.h"
#include "jobs/JobManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/Event.h"

#if !defined(TARGET_WINDOWS)
#include "platform/posix/ConvUtils.h"
#endif

#include <atomic>
#include <cerrno>
#include <future>
#include <memory>
#include <stdexcept>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

class CFileItem;

class CTestPlayerCallback : public IPlayerCallback
{
public:
  void OnPlayBackEnded() override {}
  void OnPlayBackStarted(const CFileItem& file) override {}
  void OnPlayBackStopped() override {}
  void OnPlayBackError() override {}
  void OnQueueNextItem() override {}
};

enum class TestSeekStep
{
  NORMAL,
  LARGE,
};

class CTestVideoPlayer : public CVideoPlayer
{
public:
  explicit CTestVideoPlayer(IPlayerCallback& c) : CVideoPlayer(c) {}
  virtual ~CTestVideoPlayer() {}

  int InvokeGetPreviousBookmark(std::chrono::milliseconds ts) { return GetPreviousBookmark(ts); }
  int InvokeGetNextBookmark(std::chrono::milliseconds ts) { return GetNextBookmark(ts); }
  std::optional<std::chrono::milliseconds> InvokeGetBookmarkPos(int idx)
  {
    return GetBookmarkPos(idx);
  }

  void SetCurrentVideoId(int id) { m_CurrentVideo.id = id; }
  void SetCurrentAudioId(int id) { m_CurrentAudio.id = id; }
  void SetHasVideo(bool hasVideo) { m_HasVideo = hasVideo; }
  void SetHasAudio(bool hasAudio) { m_HasAudio = hasAudio; }
  bool GetHasVideo() const { return m_HasVideo; }
  bool GetHasAudio() const { return m_HasAudio; }
  void InvokeUpdateHasVideoAudio() { UpdateHasVideoAudio(); }

  constexpr static SeekStep ConvertTestSeekStep(TestSeekStep step)
  {
    if (step == TestSeekStep::NORMAL)
      return SeekStep::NORMAL;
    else if (step == TestSeekStep::LARGE)
      return SeekStep::LARGE;
    throw std::out_of_range("missing mapping");
  }

  static int64_t InvokeCalcTimeOrPercentSeekTarget(int64_t time,
                                                   int64_t maxTime,
                                                   Direction direction,
                                                   TestSeekStep step)
  {
    return CalcTimeOrPercentSeekTarget(time, maxTime, direction, ConvertTestSeekStep(step));
  }
};

struct CShutdownState
{
  CEvent seekEntered{true};
  CEvent releaseSeek{true};
  CEvent abortEntered{true};
  CEvent allowAbortReturn{true};
  CEvent abortReturned{true};
  CEvent processReturned{true};
  CEvent onExitEntered{true};
  CEvent playerExited{true};
  CEvent inputDestroyed{true};
  std::atomic<unsigned int> sequence{0};
  std::atomic<unsigned int> abortCalls{0};
  std::atomic<unsigned int> abortOrder{0};
  std::atomic<unsigned int> abortReturnOrder{0};
  std::atomic<unsigned int> stopOrder{0};
  std::atomic<unsigned int> destroyOrder{0};
  std::atomic<int64_t> seekResult{0};
};

struct CFactoryShutdownState
{
  CEvent factoryEntered{true};
  CEvent allowFactoryReturn{true};
  CEvent processReturned{true};
  std::atomic<unsigned int> inputOpenCalls{0};
  std::atomic<bool> openResult{true};
};

class CFactoryInputStream : public CDVDInputStream
{
public:
  CFactoryInputStream(const CFileItem& item, std::shared_ptr<CFactoryShutdownState> state)
    : CDVDInputStream(DVDSTREAM_TYPE_FILE, item),
      m_state(std::move(state))
  {
  }

  bool Open() override
  {
    ++m_state->inputOpenCalls;
    return true;
  }
  int Read(uint8_t* buf, int bufSize) override { return 0; }
  int64_t Seek(int64_t offset, int whence) override { return offset; }
  int64_t GetLength() override { return 0; }
  bool IsEOF() override { return false; }

private:
  std::shared_ptr<CFactoryShutdownState> m_state;
};

class CFactoryShutdownVideoPlayer : public CVideoPlayer
{
public:
  CFactoryShutdownVideoPlayer(IPlayerCallback& callback,
                              std::shared_ptr<CFactoryShutdownState> state)
    : CVideoPlayer(callback),
      m_state(std::move(state))
  {
  }

  void InvokeAbortInputStream() { AbortInputStream(); }

protected:
  void OnStartup() override {}
  void Process() override
  {
    m_state->openResult = OpenInputStream();
    m_state->processReturned.Set();
  }
  std::shared_ptr<CDVDInputStream> CreateInputStream() override
  {
    m_state->factoryEntered.Set();
    m_state->allowFactoryReturn.Wait();
    return std::make_shared<CFactoryInputStream>(CFileItem{"mock://server/movie.mkv", false},
                                                 m_state);
  }

private:
  std::shared_ptr<CFactoryShutdownState> m_state;
};

class CBlockingInputStream : public CDVDInputStream
{
public:
  CBlockingInputStream(const CFileItem& item, std::shared_ptr<CShutdownState> state)
    : CDVDInputStream(DVDSTREAM_TYPE_FILE, item),
      m_state(std::move(state))
  {
  }

  ~CBlockingInputStream() override
  {
    auto state = m_state;
    state->destroyOrder = ++state->sequence;
    state->inputDestroyed.Set();
  }

  int Read(uint8_t* buf, int bufSize) override { return 0; }
  int64_t Seek(int64_t offset, int whence) override
  {
    m_state->seekEntered.Set();
    m_state->releaseSeek.Wait();
    if (m_aborted)
    {
      SetLastError(ECANCELED);
      return -1;
    }
    return offset;
  }
  int64_t GetLength() override { return 1024 * 1024; }
  bool IsEOF() override { return false; }
  void Abort() override
  {
    auto state = m_state;
    m_aborted = true;
    ++state->abortCalls;
    state->abortOrder = ++state->sequence;
    state->releaseSeek.Set();
    state->abortEntered.Set();
    state->allowAbortReturn.Wait();
    state->abortReturnOrder = ++state->sequence;
    state->abortReturned.Set();
  }

private:
  std::shared_ptr<CShutdownState> m_state;
  std::atomic<bool> m_aborted{false};
};

class CShutdownVideoPlayer : public CVideoPlayer
{
public:
  CShutdownVideoPlayer(IPlayerCallback& callback, std::shared_ptr<CShutdownState> state)
    : CVideoPlayer(callback),
      m_state(std::move(state))
  {
  }

  void SetInputStream(std::shared_ptr<CDVDInputStream> input)
  {
    std::unique_lock lock(m_inputStreamSync);
    m_pInputStream = std::move(input);
  }

  void StopThread(bool wait = true) override
  {
    m_state->stopOrder = ++m_state->sequence;
    m_state->releaseSeek.Set();
    CThread::StopThread(wait);
  }

protected:
  void OnStartup() override {}
  void Process() override
  {
    m_state->seekResult = m_pInputStream->Seek(256 * 1024, SEEK_SET);
    m_state->processReturned.Set();
  }
  void OnExit() override
  {
    m_state->onExitEntered.Set();
    std::unique_lock lock(m_inputStreamSync);
    m_pInputStream.reset();
    m_state->playerExited.Set();
  }

private:
  std::shared_ptr<CShutdownState> m_state;
};

class TestVideoPlayer : public testing::Test
{
protected:
  static void SetUpTestSuite()
  {
    CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());
  }
  static void TearDownTestSuite() { CServiceBroker::UnregisterJobManager(); }
};

TEST_F(TestVideoPlayer, GetPreviousBookmark)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  std::vector<std::chrono::milliseconds> bookmarks{100s, 200s};
  player.SetBookmarks(bookmarks);

  std::chrono::milliseconds ts{0s};
  int idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(-1, idx);
  ts = 100s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(-1, idx);
  // 5-second grade delay
  ts = 105s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(-1, idx);
  ts = 106s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(0, idx);
  ts = 200s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(0, idx);
  // 5-second grade delay
  ts = 205s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(0, idx);
  ts = 206s;
  idx = player.InvokeGetPreviousBookmark(ts);
  EXPECT_EQ(1, idx);
}

TEST_F(TestVideoPlayer, GetNextBookmark)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  std::vector<std::chrono::milliseconds> bookmarks{100s, 200s};
  player.SetBookmarks(bookmarks);

  std::chrono::milliseconds ts{0s};
  int idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(0, idx);
  ts = 100s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(1, idx);
  ts = 101s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(1, idx);
  ts = 200s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(-1, idx);
  ts = 201s;
  idx = player.InvokeGetNextBookmark(ts);
  EXPECT_EQ(-1, idx);
}

TEST_F(TestVideoPlayer, GetBookmarkPos)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  std::vector<std::chrono::milliseconds> bookmarks{100s, 200s};
  player.SetBookmarks(bookmarks);

  std::optional<std::chrono::milliseconds> pos = player.InvokeGetBookmarkPos(-1);

  EXPECT_FALSE(pos.has_value());

  pos = player.InvokeGetBookmarkPos(0);
  EXPECT_TRUE(pos.has_value());
  if (pos.has_value())
  {
    // braces to quiet clang warning
    EXPECT_EQ(100s, pos.value());
  }

  pos = player.InvokeGetBookmarkPos(1);
  EXPECT_TRUE(pos.has_value());
  if (pos.has_value())
  {
    // braces to quiet clang warning
    EXPECT_EQ(200s, pos.value());
  }

  pos = player.InvokeGetBookmarkPos(2);
  EXPECT_FALSE(pos.has_value());
}

TEST_F(TestVideoPlayer, UpdateHasVideoAudioClearsStaleVideoFlag)
{
  // simulates a mixed playlist transition from a music video to an audio-only
  // track: the previous item left m_HasVideo true, but the new item has no
  // video stream (m_CurrentVideo.id < 0), so the stale flag must be cleared
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  player.SetHasVideo(true);
  player.SetHasAudio(true);
  player.SetCurrentVideoId(-1);
  player.SetCurrentAudioId(0);

  player.InvokeUpdateHasVideoAudio();

  EXPECT_FALSE(player.GetHasVideo());
  EXPECT_TRUE(player.GetHasAudio());
}

TEST_F(TestVideoPlayer, UpdateHasVideoAudioKeepsFlagsWhenStreamsOpen)
{
  CTestPlayerCallback playercallback;
  CTestVideoPlayer player(playercallback);

  player.SetHasVideo(true);
  player.SetHasAudio(true);
  player.SetCurrentVideoId(0);
  player.SetCurrentAudioId(0);

  player.InvokeUpdateHasVideoAudio();

  EXPECT_TRUE(player.GetHasVideo());
  EXPECT_TRUE(player.GetHasAudio());
}

TEST_F(TestVideoPlayer, CalcTimeOrPercentSeekTargetCompat)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);

  // Back compatibility mode
  // time based jumps allowed
  advancedSettings->m_videoSmoothPercentToTimeSeeking = false;
  advancedSettings->m_videoUseTimeSeeking = true;

  // ensure video long enough to engage time jumps
  int64_t maxTime = 2000 * advancedSettings->m_videoTimeSeekForwardBig + 1000;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));

  // video not long enough => percent based jumps
  maxTime = 2000 * advancedSettings->m_videoTimeSeekForwardBig - 1000;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));
}

TEST_F(TestVideoPlayer, CalcTimeOrPercentSeekTargetPercent)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);

  // Percent based only
  advancedSettings->m_videoSmoothPercentToTimeSeeking = false;
  advancedSettings->m_videoUseTimeSeeking = false;

  // duration that would have engaged time based jumps otherwise
  int64_t maxTime = 2000 * advancedSettings->m_videoTimeSeekForwardBig + 1000;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));
}

TEST_F(TestVideoPlayer, CalcTimeOrPercentSeekTargetSmooth)
{
  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  ASSERT_TRUE(advancedSettings != nullptr);

  // Smooth percent to time based jumps
  advancedSettings->m_videoSmoothPercentToTimeSeeking = true;

  // Tests pattern: find the threshold between percent-based and time-based using
  // the advanced settings, then try a maxTime under and over the threshold

  int64_t threshold = advancedSettings->m_videoTimeSeekForwardBig * 1000 * 100 /
                      advancedSettings->m_videoPercentSeekForwardBig;

  // percent based for small durations
  int64_t maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));
  // time based for large durations
  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::LARGE));

  // Repeat for the other types of jumps
  threshold = advancedSettings->m_videoTimeSeekForward * 1000 * 100 /
              advancedSettings->m_videoPercentSeekForward;

  maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekForward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekForward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::FORWARD,
                                                                TestSeekStep::NORMAL));

  threshold = advancedSettings->m_videoTimeSeekBackwardBig * 1000 * 100 /
              advancedSettings->m_videoPercentSeekBackwardBig;

  maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackwardBig / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackwardBig * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::LARGE));

  threshold = advancedSettings->m_videoTimeSeekBackward * 1000 * 100 /
              advancedSettings->m_videoPercentSeekBackward;

  maxTime = threshold / 2;

  EXPECT_EQ(maxTime * advancedSettings->m_videoPercentSeekBackward / 100,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));

  maxTime = threshold * 2;

  EXPECT_EQ(advancedSettings->m_videoTimeSeekBackward * 1000,
            CTestVideoPlayer::InvokeCalcTimeOrPercentSeekTarget(0, maxTime, Direction::BACKWARD,
                                                                TestSeekStep::NORMAL));
}

TEST_F(TestVideoPlayer, FileInputAbortReachesFileCache)
{
  const std::string path{"special://temp/TestVideoPlayer-file-cache-abort.mkv"};
  XFILE::CFile::Delete(path);

  XFILE::CFile output;
  ASSERT_TRUE(output.OpenForWrite(path, true));
  const uint8_t contents[4]{0, 1, 2, 3};
  ASSERT_EQ(4, output.Write(contents, sizeof(contents)));
  output.Close();

  CFileItem item{path, false};
  item.SetMimeType("video/x-matroska");
  CDVDInputStreamFile input{item, XFILE::READ_CACHED};
  ASSERT_TRUE(input.Open());

  input.Abort();

  EXPECT_EQ(-1, input.Seek(0, SEEK_SET));
  EXPECT_EQ(ECANCELED, GetLastError());
  input.Close();
  EXPECT_TRUE(XFILE::CFile::Delete(path));
}

TEST_F(TestVideoPlayer, CloseFileKeepsInputAliveUntilAbortReturns)
{
  auto state = std::make_shared<CShutdownState>();
  CTestPlayerCallback callback;
  CShutdownVideoPlayer player{callback, state};
  CFileItem item{"mock://server/movie.mkv", false};
  player.SetInputStream(std::make_shared<CBlockingInputStream>(item, state));
  player.Create(false);

  const bool seekEntered = state->seekEntered.Wait(5s);
  auto closeResult = std::async(std::launch::async, [&]() { return player.CloseFile(); });
  const bool abortEntered = state->abortEntered.Wait(5s);
  const bool processReturned = state->processReturned.Wait(5s);
  const bool onExitEntered = state->onExitEntered.Wait(5s);
  const bool inputDestroyedDuringAbort = state->inputDestroyed.Signaled();
  const bool closeWaitedForAbort = closeResult.wait_for(0ms) == std::future_status::timeout;

  state->allowAbortReturn.Set();
  state->releaseSeek.Set();
  const bool closeFinished = closeResult.wait_for(5s) == std::future_status::ready;
  bool closeSucceeded = false;
  if (closeFinished)
    closeSucceeded = closeResult.get();

  EXPECT_TRUE(seekEntered);
  EXPECT_TRUE(abortEntered);
  EXPECT_TRUE(processReturned);
  EXPECT_TRUE(onExitEntered);
  EXPECT_FALSE(inputDestroyedDuringAbort);
  EXPECT_TRUE(closeWaitedForAbort);
  ASSERT_TRUE(closeFinished);
  EXPECT_TRUE(closeSucceeded);
  EXPECT_TRUE(state->abortReturned.Signaled());
  EXPECT_TRUE(state->playerExited.Signaled());
  EXPECT_TRUE(state->inputDestroyed.Signaled());
  EXPECT_EQ(1U, state->abortCalls);
  EXPECT_EQ(1U, state->abortOrder);
  EXPECT_GT(state->abortReturnOrder, state->abortOrder);
  EXPECT_GT(state->stopOrder, state->abortOrder);
  EXPECT_GT(state->destroyOrder, state->abortReturnOrder);
  EXPECT_EQ(-1, state->seekResult);
}

TEST_F(TestVideoPlayer, CloseFileCanCancelInputFactoryPublication)
{
  auto state = std::make_shared<CFactoryShutdownState>();
  CTestPlayerCallback callback;
  CFactoryShutdownVideoPlayer player{callback, state};
  player.Create(false);

  const bool factoryEntered = state->factoryEntered.Wait(5s);
  auto abortResult = std::async(std::launch::async, [&]() { player.InvokeAbortInputStream(); });
  const bool abortReadyBeforeFactoryReturn = abortResult.wait_for(1s) == std::future_status::ready;

  state->allowFactoryReturn.Set();
  const bool processReturned = state->processReturned.Wait(5s);
  if (abortResult.wait_for(5s) == std::future_status::ready)
    abortResult.get();
  const bool closeSucceeded = player.CloseFile();

  EXPECT_TRUE(factoryEntered);
  EXPECT_TRUE(abortReadyBeforeFactoryReturn);
  EXPECT_TRUE(processReturned);
  EXPECT_TRUE(closeSucceeded);
  EXPECT_FALSE(state->openResult);
  EXPECT_EQ(0U, state->inputOpenCalls);
}
