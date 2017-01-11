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
 * \brief Outgoing data stream abstraction
 */
class IDataStreamOutput
{
public:
  virtual ~IDataStreamOutput() = default;

  /*!
   * \brief Get a buffer for sending zero-copy data
   *
   * \param[out] data The buffer, must be released or sent if this returns True
   * \param[out] size The size of the buffer
   *
   * \return True if a buffer was obtained, false otherwise
   */
  virtual bool GetSendBuffer(uint8_t*& data, size_t& size) { return false; }

  /*!
   * \brief Release a zero-copy buffer obtained via GetSendBuffer()
   *
   * \param data The buffer returned by GetSendBuffer()
   */
  virtual void ReleaseSendBuffer(uint8_t* data) {}

  /*!
   * \brief Add data to the outgoing stream
   *
   * \param data The data
   * \param size The size of the data
   *
   * \return True if the data was submitted, false otherwise (possibly due to
   *         a full queue or closed stream)
   */
  virtual bool Send(const uint8_t* data, size_t size) = 0;
};

/*!
 * \brief Incoming data stream abstraction
 */
class IDataStreamInput
{
public:
  virtual ~IDataStreamInput() = default;

  /*!
   * \brief Get a buffer for receiving zero-copy data
   *
   * \param[out] data The buffer, must be released or received if this returns True
   * \param[out] size The size of the buffer
   *
   * \return True if a buffer was obtained, false otherwise
   */
  virtual bool GetReceiveBuffer(uint8_t*& data, size_t& size) { return false; }

  /*!
   * \brief Release a zero-copy buffer obtained via GetReceiveBuffer()
   *
   * \param data The buffer returned by GetReceiveBuffer()
   */
  virtual void ReleaseReceiveBuffer(uint8_t* data) {}

  /*!
   * \brief Receive data for the incoming stream
   *
   * \param data The data
   * \param size The size of the data
   */
  virtual void OnReceive(const uint8_t* data, size_t size) = 0;
};
} // namespace RETRO
} // namespace KODI
