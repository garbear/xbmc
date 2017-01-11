/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "stddef.h"
#include "stdint.h"

namespace DATASTREAM
{
  /*!
   * \brief Capabilities of the data stream, available before creation
   */
  struct DataStreamCapabilities
  {
    /*!
     * \brief Indicates that the data stream supports in-order message delivery
     *        guarantee
     *
     * This generally has a higher latency than out-of-order delivery.
     */
    bool supportsOrdered;

    /*!
     * \brief Indicates that the data stream supports delivering messages
     *        out-of-order
     *
     * This is usually used for performance reasons.
     */
    bool supportsUnordered;

    /*!
     * \brief Indicates that the data stream can be opened without negotiation
     */
    //bool supportsNegotiation;

    /*!
     * \brief Indicates that the data stream can't be opened without negotiation
     */
    //bool requiresNegotiation;
  };

  /*!
   * \brief Properties set when the data stream is opened
   */
  struct DataStreamProperties
  {
    /*!
     * \brief Returns a string containing the name of the subprotocol in use,
     *        if available
     *
     * The default value is NULL or "".
     */
    const char* protocol;

    /*!
     * \brief Indicates whether the data stream guarantees in-order delivery of
     *        messages
     */
    bool ordered;

    /*!
     * \brief Indicates whether the data stream is negotiated by the stream or
     *        not
     */
    //bool negotiated;

    /*!
     * \brief Get the amount of time, in ms, the stream is allowed to take to
     *        attempt to transmit a message
     *
     * The default value is 0, meaning the data stream will keep attempting to
     * transmit the message.
     */
    unsigned unsigned int maxPacketLifetimeMs;

    /*!
     * \brief Get the maximum number of times the stream will try to transmit
     *        a message before giving up when in unordered mode
     *
     * The default is 0, meaning there is no maximum.
     */
    unsigned unsigned int maxRetransmits;

    /*!
     * \brief Specifies the number of bytes of buffered outgoing data that is
     *        considered "low", or 0 if unknown
     */
    unsigned int bufferedAmountLowThreshold;
  };

  /*!
   * \brief Measured properties about the data stream
   */
  struct DataStreamDiagnostics
  {
    /*!
     * \brief The number of bytes currently queued to be sent over the data
     *        stream, or 0 if unknown or the queue is empty
     */
    unsigned int bufferedAmount;

    /*!
     * \brief The average latency of the stream, in seconds, or 0 if unknown
     */
    float latency;

    /*!
     * \brief The jitter, expressed as the latency's RMS, in seconds, or 0 if
     *        unknown
     *
     * The standard deviation can be calculated as (jitter - latency).
     */
    float jitter;

    /*!
     * \brief The average bandwidth of the stream, in bytes/seconds, or 0 if
     *        unknown
     *
     * The average bandwidth is usually expressed as a (possibly weighted)
     * moving average.
     */
    float bandwidth;

    /*!
     * \brief The stream's estimated maximum bandwidth, in bytes/seconds, or 0
     *        if unknown
     */
    float maxBandwidth;
  };

  /*!
   * \brief Base data stream abstraction
   */
  class IDataStreamBase
  {
  public:
    virtual ~IDataStreamBase() = default;

    /*!
     * \brief Get the current state of the data stream
     *
     * \return True if the stream is open, false otherwise
     */
    virtual bool IsOpen() = 0;

    /*!
     * \brief Get the capabilities offered by the data stream
     *
     * \param caps Set to the data stream's capabilities
     */
    virtual void GetCapabilities(DataStreamCapabilities& caps) = 0;

    /*!
     * \brief Get the properties of an open data stream
     *
     * \param props Set to the data stream's properties
     *
     * \return True if the data stream is open, false otherwise
     */
    virtual bool GetProperties(DataStreamProperties& props) = 0;

    /*!
     * \brief Get the measured diagnostics of an open data stream
     *
     * \param diags Set to the data stream's stats
     *
     * \return True if the data stream is open, false otherwise
     */
    virtual bool GetDiagnostics(DataStreamDiagnostics& diags) = 0;
  };

  /*!
   * \brief Outgoing data stream abstraction
   */
  class IDataStreamOutput : public IDataStreamBase
  {
  public:
    virtual ~IDataStreamOutput() = default;

    /*!
     * \brief Open the data stream
     *
     * \return True if the data stream is open and is ready to start sending
     *         data, false otherwise
     */
    virtual bool Open() = 0;

    /*!
     * \brief Add data to the outgoing stream
     *
     * \return True if the data was submitted, false otherwise (possibly due to
     *         a full queue or closed stream)
     */
    virtual bool Send(const uint8_t* data, size_t size) = 0;

    /*!
     * \brief Close the data stream
     */
    virtual void Close() = 0;
  };

  /*!
   * \brief Incoming data stream abstraction
   */
  class IDataStreamInput: public IDataStreamBase
  {
  public:
    virtual ~IDataStreamInput() = default;

    /*!
     * \brief Called when the data stream is opened
     */
    virtual void OnOpen() = 0;

    /*!
     * \brief Called when the data stream receives data
     */
    virtual void OnReceive(const uint8_t* data, size_t size) = 0;

    /*!
     * \brief Called when the data stream is closed
     *
     * \param error An implementation-defined error code, or 0 for no error
     */
    virtual void OnClose(int error) = 0;
  };
}
