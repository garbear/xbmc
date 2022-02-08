/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VarInt.h"

#include "utils/log.h"

using namespace KODI;
using namespace CRYPTO;

bool VarInt::Decode(const uint8_t* data, size_t dataLength, uint64_t& result)
{
  result = 0;
  size_t shift = 0;
  size_t bytesRead = 0;
  std::string errorLog;

  // Maximum possible iterations for a uint64_t
  const size_t maxIterations = sizeof(uint64_t) * 8 / 7 + 1;

  for (size_t i = 0; i < maxIterations; ++i)
  {
    if (bytesRead >= dataLength)
    {
      errorLog = "Data is too short for decoding";
      break;
    }

    const uint8_t byte = data[bytesRead];

    // Check if we can safely shift without overflowing the result.
    if (shift < 64)
    {
      // Mask out the most significant bit and shift the relevant data into result.
      result |= static_cast<uint64_t>(byte & 0x7F) << shift;
    }
    else
    {
      // We've shifted too far, the data isn't a valid varint
      errorLog = "Excessive shifts detected, malformed varint";
      break;
    }

    ++bytesRead;

    // If the most significant bit is 0, this is the last byte in the varint.
    if ((byte & 0x80) == 0)
      break;

    shift += 7;
  }

  if (shift >= 64 && errorLog.empty())
    errorLog = "Data does not represent a valid varint";

  if (!errorLog.empty())
  {
    CLog::Log(LOGERROR, "VarInt::Decode - {}. bytesRead: {}, data size: {}", errorLog, bytesRead,
              dataLength);
    return false;
  }

  return true;
}

bool VarInt::EncodeTo(uint64_t value, uint8_t* target, size_t targetLength)
{
  const uint64_t originalValue = value; // For logging purposes
  size_t offset = 0;

  while (value >= 0x80)
  {
    if (offset >= targetLength)
    {
      CLog::Log(LOGERROR,
                "VarInt::EncodeTo - target buffer is too short for value {}. current offset: {}",
                originalValue, offset);
      return false;
    }

    // Encode the lower 7 bits of value and mark the next byte as needed by setting the 8th bit.
    target[offset++] = static_cast<uint8_t>(value) | 0x80;

    // Right shift the value by 7 bits to process the next set of 7 bits.
    value >>= 7;
  }

  if (offset >= targetLength)
  {
    CLog::Log(LOGERROR,
              "VarInt::EncodeTo - target buffer is too short for encoding the final byte of value "
              "{}. Initial offset: {}",
              originalValue, offset);
    return false;
  }

  target[offset] = static_cast<uint8_t>(value);
  return true;
}

size_t VarInt::EncodingLength(uint64_t value)
{
  size_t length = 0;

  do
  {
    ++length;
    value >>= 7;
  } while (value != 0);

  return length;
}
