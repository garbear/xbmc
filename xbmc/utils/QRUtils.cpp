/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "QRUtils.h"

#include <qrencode.h>

using namespace KODI;
using namespace UTILS;

bool CQRUtils::EncodeString(const std::string& data, std::vector<bool>& pixels, unsigned int& width)
{
  QRcode* qrcode = QRcode_encodeString(data.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1);
  if (qrcode != nullptr)
  {
    pixels.resize(static_cast<size_t>(qrcode->width * qrcode->width));
    TranslateData(pixels, qrcode->data, qrcode->width);
    width = qrcode->width;
    QRcode_free(qrcode);

    return true;
  }

  return false;
}

bool CQRUtils::EncodeData(const std::vector<uint8_t>& data,
                          std::vector<bool>& pixels,
                          unsigned int& width)
{
  QRcode* qrcode = QRcode_encodeData(data.size(), data.data(), 0, QR_ECLEVEL_H);
  if (qrcode != nullptr)
  {
    pixels.resize(static_cast<size_t>(qrcode->width * qrcode->width));
    TranslateData(pixels, qrcode->data, qrcode->width);
    width = qrcode->width;
    QRcode_free(qrcode);

    return true;
  }

  return false;
}

void CQRUtils::TranslateData(std::vector<bool>& pixels, const uint8_t* data, unsigned int width)
{
  for (unsigned int i = 0; i < width; ++i)
  {
    for (unsigned int j = 0; j < width; ++j)
    {
      const unsigned int index = i * width + j;
      pixels[index] = (data[index] & 1);
    }
  }
}
