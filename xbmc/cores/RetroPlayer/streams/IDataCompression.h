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

namespace DATASTREAM
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
     * \brief
     */
    virtual void AddData(const uint8_t* inputData, size_t inputSize, uint8_t*& outputData, size_t& outputSize) = 0;

    /*!
     * \brief
     */
    virtual void FreeData(uint8_t* outputData, size_t outputSize) = 0;

    /*!
     * \brief
     */
    void GetStats(CompressionStats& stats) = 0;
  };
}
