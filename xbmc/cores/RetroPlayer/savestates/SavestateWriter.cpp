/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateWriter.h"

#include "ISavestate.h"
#include "SavestateDatabase.h"
#include "utils/log.h"

#include <cstring>
#include <memory>

using namespace KODI;
using namespace RETRO;

bool CSavestateWriter::WritePayload(const SavestateWritePayload& payload,
                                    CSavestateDatabase& database)
{
  if (payload.savePath.empty())
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Refusing to write savestate with empty save path");
    return false;
  }

  if (payload.gamePath.empty())
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Refusing to write savestate with empty game path");
    return false;
  }

  if (payload.memoryData.empty())
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Refusing to write savestate with empty memory data");
    return false;
  }

  std::unique_ptr<ISavestate> savestate = CSavestateDatabase::AllocateSavestate();
  savestate->SetType(payload.type);
  savestate->SetSlot(payload.slot);
  savestate->SetLabel(payload.label);
  savestate->SetCaption(payload.caption);
  savestate->SetCreated(payload.created);
  savestate->SetGameFileName(payload.gameFileName);
  savestate->SetTimestampFrames(payload.timestampFrames);
  savestate->SetTimestampWallClock(payload.timestampWallClock);
  savestate->SetGameClientID(payload.gameClientId);
  savestate->SetGameClientVersion(payload.gameClientVersion);

  savestate->SetPixelFormat(payload.pixelFormat);
  savestate->SetNominalWidth(payload.nominalWidth);
  savestate->SetNominalHeight(payload.nominalHeight);
  savestate->SetNominalDisplayAspectRatio(payload.nominalDisplayAspectRatio);
  savestate->SetMaxWidth(payload.maxWidth);
  savestate->SetMaxHeight(payload.maxHeight);

  savestate->SetVideoWidth(payload.videoWidth);
  savestate->SetVideoHeight(payload.videoHeight);
  savestate->SetDisplayAspectRatio(payload.displayAspectRatio);
  savestate->SetRotationDegCCW(payload.rotationCCW);
  if (!payload.videoData.empty())
  {
    uint8_t* const videoData = savestate->GetVideoBuffer(payload.videoData.size());
    if (videoData != nullptr)
      std::memcpy(videoData, payload.videoData.data(), payload.videoData.size());
  }

  uint8_t* const memoryData = savestate->GetMemoryBuffer(payload.memoryData.size());
  if (memoryData == nullptr)
  {
    CLog::Log(LOGERROR, "RetroPlayer[SAVE]: Failed to allocate savestate memory buffer");
    return false;
  }

  std::memcpy(memoryData, payload.memoryData.data(), payload.memoryData.size());

  savestate->Finalize();

  return database.AddSavestate(payload.savePath, payload.gamePath, *savestate);
}
