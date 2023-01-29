/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISavestate.h"

#include <memory>
#include <vector>

namespace KODI
{
namespace RETRO
{
class CCompressedSavestate : public ISavestate
{
public:
  CCompressedSavestate();
  ~CCompressedSavestate() override;

  // Implementation of ISavestate
  void Reset() override;
  bool Serialize(const uint8_t*& data, size_t& size) const override;
  SAVE_TYPE Type() const override;
  uint8_t Slot() const override;
  std::string Label() const override;
  std::string Caption() const override;
  CDateTime Created() const override;
  std::string GameFileName() const override;
  uint64_t TimestampFrames() const override;
  double TimestampWallClock() const override;
  std::string GameClientID() const override;
  std::string GameClientVersion() const override;
  AVPixelFormat GetPixelFormat() const override;
  unsigned int GetNominalWidth() const override;
  unsigned int GetNominalHeight() const override;
  unsigned int GetMaxWidth() const override;
  unsigned int GetMaxHeight() const override;
  float GetPixelAspectRatio() const override;
  const uint8_t* GetVideoData() const override;
  size_t GetVideoSize() const override;
  unsigned int GetVideoWidth() const override;
  unsigned int GetVideoHeight() const override;
  unsigned int GetRotationDegCCW() const override;
  const uint8_t* GetMemoryData() const override;
  size_t GetMemorySize() const override;
  void SetType(SAVE_TYPE type) override;
  void SetSlot(uint8_t slot) override;
  void SetLabel(const std::string& label) override;
  void SetCaption(const std::string& caption) override;
  void SetCreated(const CDateTime& createdUTC) override;
  void SetGameFileName(const std::string& gameFileName) override;
  void SetTimestampFrames(uint64_t timestampFrames) override;
  void SetTimestampWallClock(double timestampWallClock) override;
  void SetGameClientID(const std::string& gameClient) override;
  void SetGameClientVersion(const std::string& gameClient) override;
  void SetPixelFormat(AVPixelFormat pixelFormat) override;
  void SetNominalWidth(unsigned int nominalWidth) override;
  void SetNominalHeight(unsigned int nominalHeight) override;
  void SetMaxWidth(unsigned int maxWidth) override;
  void SetMaxHeight(unsigned int maxHeight) override;
  void SetPixelAspectRatio(float pixelAspectRatio) override;
  uint8_t* GetVideoBuffer(size_t size) override;
  void SetVideoWidth(unsigned int videoWidth) override;
  void SetVideoHeight(unsigned int videoHeight) override;
  void SetRotationDegCCW(unsigned int rotationCCW) override;
  uint8_t* GetMemoryBuffer(size_t size) override;
  void Finalize() override;
  bool Deserialize(std::vector<uint8_t> data) override;

private:
  /*!
   * \brief System memory storage (for deserializing savestates)
   *
   * This memory is used when deserializing from a vector.
   */
  std::vector<uint8_t> m_compressedData;

  /*!
   * \brief Underlying savestate to store uncompressed data
   */
  std::unique_ptr<ISavestate> m_savestate;
};
} // namespace RETRO
} // namespace KODI
