/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "AddonDll.h"
#include "include/kodi_imageenc_types.h"
#include "guilib/iimage.h"

typedef DllAddon<ImageEncoder, IMAGEENC_PROPS> DllImageEncoder;
namespace ADDON
{
  typedef CAddonDll<DllImageEncoder,
                    ImageEncoder, IMAGEENC_PROPS> ImageEncoderDll;

  class CImageEncoder : public ImageEncoderDll,
                        public IImage
  {
  public:
    CImageEncoder(const AddonProps &props)
      : ImageEncoderDll(props)
    {};
    CImageEncoder(const cp_extension_t *ext);
    CImageEncoder(const CImageEncoder& from);
    virtual ~CImageEncoder();
    virtual AddonPtr Clone() const;

    bool Create(const std::string& mimetype);

    bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                             unsigned int width, unsigned int height);
    bool Decode(unsigned char* const pixels, unsigned int width,
                unsigned int height, unsigned int pitch,
                unsigned int format);
    bool CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width,
                                    unsigned int height, unsigned int format,
                                    unsigned int pitch, const std::string& destFile,
                                    unsigned char*& bufferout, unsigned int& bufferoutSize);

    const std::string& GetMimetypes() const { return m_mimetype; }
  protected:
    void* m_image = nullptr;
    std::string m_mimetype;
  };

} /*namespace ADDON*/
