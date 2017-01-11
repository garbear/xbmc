/*
*  Copyright (C) 2017-2023 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include <stddef.h>
#include <stdint.h>
#include <vector>

class CVariant;

namespace KODI
{
namespace RETRO
{

struct DataStreamCapabilities;
struct DataStreamDiagnostics;
struct DataStreamProperties;

enum class ECompressionType
{
  NONE = 0x0,
  DELTA = 0x1,
  LOSSLESS = 0x2,
  LOSSY = 0x4,
  DELTA_LOSSLESS = DELTA | LOSSLESS,
  DELTA_LOSSY = DELTA | LOSSY,
};

/*!
 * \brief Measured properties about the time series stream
 */
struct TimeSeriesDiagnostics
{
  /*!
   * \brief Compression type; determines which of the following fields are used
   */
  ECompressionType type;

  /*!
   * \brief The average ratio of a standalone frame to a delta frame
   */
  float deltaCompressionRatio;

  /*!
   * \brief The average lossless/lossy compression ratio
   */
  float compressionRatio;

  /*!
   * \brief The average quality of lossy compression
   */
  float quality;
};

template<typename DATA_TYPE>
class IFrame
{
  virtual ~IFrame() = default;

  /*!
   * \brief Get a pointer to the frame's data
   *
   * This function must have no side effects. The reference is valid until a
   * new frame is begun.
   *
   * \return A const reference to the frame's data
   */
  virtual const DATA_TYPE& Data() const = 0;

  /*!
   * \brief Get a reference to the frame's data that can be written
   *
   * \return A writable reference to the frame's data
   */
  virtual DATA_TYPE& Begin() = 0;

  /*!
   * \brief Indicate that data has been written to the reference returned
   *        from BeginFrame()
   */
  virtual void Submit() = 0;
};

using DataFrame = IFrame<std::vector<uint8_t>>;

using VariantFrame = IFrame<CVariant>;

/*!
 * \brief Stream of time series frames
 */
template<typename FRAME_TYPE>
class ITimeSeriesStream
{
public:
  virtual ~ITimeSeriesStream() = default;

  /*!
   * \brief Open stream
   */
  virtual bool Open() = 0;

  /*!
   * \brief Free any resources used by this stream
   */
  virtual void Close() = 0;

  /*!
   * \brief Return the timestamp of the current frame
   */
  virtual uint64_t Timestamp() const = 0;

  /*!
   * \brief Get a pointer to which a frame can be written
   *
   * \return The buffer, or nullptr if a new frame can't be provided
   */
  virtual FRAME_TYPE BeginFrame(size_t frameSize) = 0;

  /*!
   * \brief Indicate that a frame of the correct size has been written to the
   *        location returned from BeginFrame()
   */
  virtual void SubmitFrame() = 0;

  /*!
   * \brief Get a pointer to the current frame
   *
   * This function must have no side effects. The pointer is valid until the
   * stream is modified (a non-const function is called).
   *
   * \return A buffer of size CurrentFrameSize(), or nullptr if the stream is empty
   */
  virtual const uint8_t* CurrentFrame() const = 0;

  /*!
   * \brief Get the size of the current frame, or 0 if the stream is empty
   */
  virtual size_t CurrentFrameSize() const = 0;

  /*!
   * \brief Seek in reverse the specified number of frames
   *
   * \param frameCount the number of frames to rewind
   *
   * \return The number of frames rewound
   */
  virtual int64_t SeekReverseFrames(uint64_t frameCount) = 0;

  /*!
   * \brief Get the number of frames between the specified timestamp and the
   *        closest key frame
   */
  virtual uint64_t InterstertialFrames(uint64_t timestamp) const = 0;
};

/*!
 * \brief Nonlinear time series stream
 */
template<typename FRAME_TYPE>
class INonLinearTimeSeriesStream : public ITimeSeriesStream<FRAME_TYPE>
{
  virtual ~INonLinearTimeSeriesStream() = default;

  /*!
   * \brief Return the number of frames ahead of the current frame
   *
   * If the stream supports forward seeking, frames that are passed over
   * during a reverse seek operation can be recovered again.
   */
  virtual uint64_t MaxTimestamp() const = 0;

  /*!
   * \brief Seek ahead the specified number of frames
   *
   * \param frameCount positive for future frames, negative for past frames
   *
   * \return The number of frames advanced
   */
  virtual int64_t SeekForwardFrames(uint64_t frameCount) = 0;
};

} // namespace RETRO
} // namespace KODI
