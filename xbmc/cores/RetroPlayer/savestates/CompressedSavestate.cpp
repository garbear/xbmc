/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CompressedSavestate.h"

#include "XBDateTime.h"

#include <lz4.h>

using namespace KODI;
using namespace RETRO;

CCompressedSavestate::CCompressedSavestate()
{
  Reset();
}

CCompressedSavestate::~CCompressedSavestate() = default;

void CCompressedSavestate::Reset()
{
}

bool CCompressedSavestate::Serialize(const uint8_t*& data, size_t& size) const
{
  return true;
}

SAVE_TYPE CCompressedSavestate::Type() const
{
  return SAVE_TYPE::UNKNOWN;
}

void CCompressedSavestate::SetType(SAVE_TYPE type)
{
}

uint8_t CCompressedSavestate::Slot() const
{
  return 0;
}

void CCompressedSavestate::SetSlot(uint8_t slot)
{
}

std::string CCompressedSavestate::Label() const
{
  std::string label;

  return label;
}

void CCompressedSavestate::SetLabel(const std::string& label)
{
}

std::string CCompressedSavestate::Caption() const
{
  std::string caption;

  return caption;
}

void CCompressedSavestate::SetCaption(const std::string& caption)
{
}

CDateTime CCompressedSavestate::Created() const
{
  CDateTime createdUTC;

  return createdUTC;
}

void CCompressedSavestate::SetCreated(const CDateTime& createdUTC)
{
}

std::string CCompressedSavestate::GameFileName() const
{
  std::string gameFileName;

  return gameFileName;
}

void CCompressedSavestate::SetGameFileName(const std::string& gameFileName)
{
}

uint64_t CCompressedSavestate::TimestampFrames() const
{
  return 0;
}

void CCompressedSavestate::SetTimestampFrames(uint64_t timestampFrames)
{
}

double CCompressedSavestate::TimestampWallClock() const
{
  return 0.0;
}

void CCompressedSavestate::SetTimestampWallClock(double timestampWallClock)
{
}

std::string CCompressedSavestate::GameClientID() const
{
  std::string gameClientId;

  return gameClientId;
}

void CCompressedSavestate::SetGameClientID(const std::string& gameClientId)
{
}

std::string CCompressedSavestate::GameClientVersion() const
{
  std::string gameClientVersion;

  return gameClientVersion;
}

void CCompressedSavestate::SetGameClientVersion(const std::string& gameClientVersion)
{
}

AVPixelFormat CCompressedSavestate::GetPixelFormat() const
{
  return AV_PIX_FMT_NONE;
}

void CCompressedSavestate::SetPixelFormat(AVPixelFormat pixelFormat)
{
}

unsigned int CCompressedSavestate::GetNominalWidth() const
{
  return 0;
}

void CCompressedSavestate::SetNominalWidth(unsigned int nominalWidth)
{
}

unsigned int CCompressedSavestate::GetNominalHeight() const
{
  return 0;
}

void CCompressedSavestate::SetNominalHeight(unsigned int nominalHeight)
{
}

unsigned int CCompressedSavestate::GetMaxWidth() const
{
  return 0;
}

void CCompressedSavestate::SetMaxWidth(unsigned int maxWidth)
{
}

unsigned int CCompressedSavestate::GetMaxHeight() const
{
  return 0;
}

void CCompressedSavestate::SetMaxHeight(unsigned int maxHeight)
{
}

float CCompressedSavestate::GetPixelAspectRatio() const
{
  return 0.0f;
}

void CCompressedSavestate::SetPixelAspectRatio(float pixelAspectRatio)
{
}

const uint8_t* CCompressedSavestate::GetVideoData() const
{
  return nullptr;
}

size_t CCompressedSavestate::GetVideoSize() const
{
  return 0;
}

uint8_t* CCompressedSavestate::GetVideoBuffer(size_t size)
{
  uint8_t* videoBuffer = nullptr;

  return videoBuffer;
}

unsigned int CCompressedSavestate::GetVideoWidth() const
{
  return 0;
}

void CCompressedSavestate::SetVideoWidth(unsigned int videoWidth)
{
}

unsigned int CCompressedSavestate::GetVideoHeight() const
{
  return 0;
}

void CCompressedSavestate::SetVideoHeight(unsigned int videoHeight)
{
}

unsigned int CCompressedSavestate::GetRotationDegCCW() const
{
  return 0;
}

void CCompressedSavestate::SetRotationDegCCW(unsigned int rotationCCW)
{
}

const uint8_t* CCompressedSavestate::GetMemoryData() const
{
  return nullptr;
}

size_t CCompressedSavestate::GetMemorySize() const
{
  return 0;
}

uint8_t* CCompressedSavestate::GetMemoryBuffer(size_t size)
{
  uint8_t* memoryBuffer = nullptr;

  return memoryBuffer;
}

void CCompressedSavestate::Finalize()
{
}

bool CCompressedSavestate::Deserialize(std::vector<uint8_t> data)
{
  return false;
}
