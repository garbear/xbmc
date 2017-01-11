/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Measured properties about the time series stream
 */
struct CompressionStats
{
  /*!
   * \brief True for lossless compression, false for lossy compression
   */
  bool lossless;

  /*!
   * \brief The average lossless/lossy compression ratio
   */
  float comrpessionRatio;

  /*!
   * \brief The average quality of lossy compression, or unused for lossless
   *        compression
   */
  float quality;

  /*!
   * \brief The average time it takes to compress a packet, in seconds
   */
  double compressionTime;
};

/*!
 * \brief Incoming data stream abstraction
 */
class ILosslessCompression
{
public:
  virtual ~ILosslessCompression() = default;

  /*!
   * \brief Open the data stream
   *
   * \return True if the data stream is open and is ready to start sending
   *         data, false otherwise
   */
  virtual bool Open() = 0;

  /*!
   * \brief Close the data stream
   */
  virtual void Close() = 0;

  /*!
   * \brief Add data to the stream
   */
  virtual void AddData(const uint8_t* inputData,
                       size_t inputSize,
                       uint8_t*& outputData,
                       size_t& outputSize) = 0;

  /*!
   * \brief Free allocated data
   */
  virtual void FreeData(uint8_t* outputData, size_t outputSize) = 0;

  /*!
   * \brief Get statistics about the data stream
   */
  virtual void GetStats(CompressionStats& stats) = 0;
};

} // namespace RETRO
} // namespace KODI
