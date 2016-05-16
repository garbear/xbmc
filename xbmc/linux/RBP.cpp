/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "RBP.h"
#if defined(TARGET_RASPBERRY_PI)

#include <assert.h>
#include "settings/Settings.h"
#include "utils/log.h"

#include "cores/omxplayer/OMXImage.h"

#include <sys/ioctl.h>
#include "rpi/rpi_user_vcsm.h"
#include "utils/TimeUtils.h"

#define MAJOR_NUM 100
#define IOCTL_MBOX_PROPERTY _IOWR(MAJOR_NUM, 0, char *)
#define DEVICE_FILE_NAME "/dev/vcio"

static int mbox_open();
static void mbox_close(int file_desc);

CRBP::CRBP()
{
  m_initialized     = false;
  m_omx_initialized = false;
  m_DllBcmHost      = new DllBcmHost();
  m_OMX             = new COMXCore();
  m_display = DISPMANX_NO_HANDLE;
  m_mb = mbox_open();
  vcsm_init();
  m_vsync_count = 0;
  m_vsync_time = 0;
}

CRBP::~CRBP()
{
  Deinitialize();
  delete m_OMX;
  delete m_DllBcmHost;
}

bool CRBP::Initialize()
{
  CSingleLock lock(m_critSection);
  if (m_initialized)
    return true;

  m_initialized = m_DllBcmHost->Load();
  if(!m_initialized)
    return false;

  m_DllBcmHost->bcm_host_init();

  m_omx_initialized = m_OMX->Initialize();
  if(!m_omx_initialized)
    return false;

  char response[80] = "";
  m_arm_mem = 0;
  m_gpu_mem = 0;
  m_codec_mpg2_enabled = false;
  m_codec_wvc1_enabled = false;

  if (vc_gencmd(response, sizeof response, "get_mem arm") == 0)
    vc_gencmd_number_property(response, "arm", &m_arm_mem);
  if (vc_gencmd(response, sizeof response, "get_mem gpu") == 0)
    vc_gencmd_number_property(response, "gpu", &m_gpu_mem);

  if (vc_gencmd(response, sizeof response, "codec_enabled MPG2") == 0)
    m_codec_mpg2_enabled = strcmp("MPG2=enabled", response) == 0;
  if (vc_gencmd(response, sizeof response, "codec_enabled WVC1") == 0)
    m_codec_wvc1_enabled = strcmp("WVC1=enabled", response) == 0;

  if (m_gpu_mem < 128)
    setenv("V3D_DOUBLE_BUFFER", "1", 1);

  m_gui_resolution_limit = CSettings::GetInstance().GetInt("videoscreen.limitgui");
  if (!m_gui_resolution_limit)
    m_gui_resolution_limit = m_gpu_mem < 128 ? 720:1080;

  g_OMXImage.Initialize();
  m_omx_image_init = true;
  return true;
}

void CRBP::LogFirmwareVerison()
{
  char  response[1024];
  m_DllBcmHost->vc_gencmd(response, sizeof response, "version");
  response[sizeof(response) - 1] = '\0';
  CLog::Log(LOGNOTICE, "Raspberry PI firmware version: %s", response);
  CLog::Log(LOGNOTICE, "ARM mem: %dMB GPU mem: %dMB MPG2:%d WVC1:%d", m_arm_mem, m_gpu_mem, m_codec_mpg2_enabled, m_codec_wvc1_enabled);
  m_DllBcmHost->vc_gencmd(response, sizeof response, "get_config int");
  response[sizeof(response) - 1] = '\0';
  CLog::Log(LOGNOTICE, "Config:\n%s", response);
  m_DllBcmHost->vc_gencmd(response, sizeof response, "get_config str");
  response[sizeof(response) - 1] = '\0';
  CLog::Log(LOGNOTICE, "Config:\n%s", response);
}

static void vsync_callback_static(DISPMANX_UPDATE_HANDLE_T u, void *arg)
{
  CRBP *rbp = reinterpret_cast<CRBP*>(arg);
  rbp->VSyncCallback();
}

DISPMANX_DISPLAY_HANDLE_T CRBP::OpenDisplay(uint32_t device)
{
  CSingleLock lock(m_critSection);
  if (m_display == DISPMANX_NO_HANDLE)
  {
    m_display = vc_dispmanx_display_open( 0 /*screen*/ );
    int s = vc_dispmanx_vsync_callback(m_display, vsync_callback_static, (void *)this);
    assert(s == 0);
  }
  return m_display;
}

void CRBP::CloseDisplay(DISPMANX_DISPLAY_HANDLE_T display)
{
  CSingleLock lock(m_critSection);
  assert(display == m_display);
  int s = vc_dispmanx_vsync_callback(m_display, NULL, NULL);
  assert(s == 0);
  vc_dispmanx_display_close(m_display);
  m_display = DISPMANX_NO_HANDLE;
}

void CRBP::GetDisplaySize(int &width, int &height)
{
  CSingleLock lock(m_critSection);
  DISPMANX_MODEINFO_T info;
  if (m_display != DISPMANX_NO_HANDLE && vc_dispmanx_display_get_info(m_display, &info) == 0)
  {
    width = info.width;
    height = info.height;
  }
  else
  {
    width = 0;
    height = 0;
  }
}

unsigned char *CRBP::CaptureDisplay(int width, int height, int *pstride, bool swap_red_blue, bool video_only)
{
  DISPMANX_RESOURCE_HANDLE_T resource;
  VC_RECT_T rect;
  unsigned char *image = NULL;
  uint32_t vc_image_ptr;
  int stride;
  uint32_t flags = 0;

  if (video_only)
    flags |= DISPMANX_SNAPSHOT_NO_RGB|DISPMANX_SNAPSHOT_FILL;
  if (swap_red_blue)
    flags |= DISPMANX_SNAPSHOT_SWAP_RED_BLUE;
  if (!pstride)
    flags |= DISPMANX_SNAPSHOT_PACK;

  stride = ((width + 15) & ~15) * 4;

  CSingleLock lock(m_critSection);
  if (m_display != DISPMANX_NO_HANDLE)
  {
    image = new unsigned char [height * stride];
    resource = vc_dispmanx_resource_create( VC_IMAGE_RGBA32, width, height, &vc_image_ptr );

    vc_dispmanx_snapshot(m_display, resource, (DISPMANX_TRANSFORM_T)flags);

    vc_dispmanx_rect_set(&rect, 0, 0, width, height);
    vc_dispmanx_resource_read_data(resource, &rect, image, stride);
    vc_dispmanx_resource_delete( resource );
  }
  if (pstride)
    *pstride = stride;
  return image;
}

void CRBP::VSyncCallback()
{
  CSingleLock lock(m_vsync_lock);
  m_vsync_count++;
  m_vsync_time = CurrentHostCounter();
  m_vsync_cond.notifyAll();
}

uint32_t CRBP::WaitVsync(uint32_t target)
{
  CSingleLock vlock(m_vsync_lock);
  DISPMANX_DISPLAY_HANDLE_T display = m_display;
  XbmcThreads::EndTime delay(50);
  if (target == ~0U)
    target = m_vsync_count+1;
  while (!delay.IsTimePast())
  {
    if ((signed)(m_vsync_count - target) >= 0)
      break;
    if (!m_vsync_cond.wait(vlock, delay.MillisLeft()))
      break;
  }
  if (m_vsync_count < target)
    CLog::Log(LOGDEBUG, "CRBP::%s no  vsync %d/%d display:%x(%x) delay:%d", __FUNCTION__, m_vsync_count, target, m_display, display, delay.MillisLeft());

  return m_vsync_count;
}

uint32_t CRBP::LastVsync(int64_t &time)
{
  CSingleLock lock(m_vsync_lock);
  time = m_vsync_time;
  return m_vsync_count;
}

uint32_t CRBP::LastVsync()
{
  int64_t time = 0;
  return LastVsync(time);
}

void CRBP::Deinitialize()
{
  if (m_omx_image_init)
    g_OMXImage.Deinitialize();

  if(m_omx_initialized)
    m_OMX->Deinitialize();

  if (m_display)
    CloseDisplay(m_display);

  m_DllBcmHost->bcm_host_deinit();

  if(m_initialized)
    m_DllBcmHost->Unload();

  m_omx_image_init  = false;
  m_initialized     = false;
  m_omx_initialized = false;
  if (m_mb)
    mbox_close(m_mb);
  m_mb = 0;
  vcsm_exit();
}

static int mbox_property(int file_desc, void *buf)
{
   int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);

   if (ret_val < 0)
   {
     CLog::Log(LOGERROR, "%s: ioctl_set_msg failed:%d", __FUNCTION__, ret_val);
   }
   return ret_val;
}

static int mbox_open()
{
   int file_desc;

   // open a char device file used for communicating with kernel mbox driver
   file_desc = open(DEVICE_FILE_NAME, 0);
   if (file_desc < 0)
     CLog::Log(LOGERROR, "%s: Can't open device file: %s (%d)", __FUNCTION__, DEVICE_FILE_NAME, file_desc);

   return file_desc;
}

static void mbox_close(int file_desc)
{
  close(file_desc);
}

static unsigned mem_lock(int file_desc, unsigned handle)
{
   int i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000d; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i*sizeof *p; // actual size

   mbox_property(file_desc, p);
   return p[5];
}

unsigned mem_unlock(int file_desc, unsigned handle)
{
   int i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000e; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i*sizeof *p; // actual size

   mbox_property(file_desc, p);
   return p[5];
}

CGPUMEM::CGPUMEM(unsigned int numbytes, bool cached)
{
  m_numbytes = numbytes;
  m_vcsm_handle = vcsm_malloc_cache(numbytes, cached ? VCSM_CACHE_TYPE_HOST : VCSM_CACHE_TYPE_NONE, (char *)"CGPUMEM");
  assert(m_vcsm_handle);
  m_vc_handle = vcsm_vc_hdl_from_hdl(m_vcsm_handle);
  assert(m_vc_handle);
  m_arm = vcsm_lock(m_vcsm_handle);
  assert(m_arm);
  m_vc = mem_lock(g_RBP.GetMBox(), m_vc_handle);
  assert(m_vc);
}

CGPUMEM::~CGPUMEM()
{
  mem_unlock(g_RBP.GetMBox(), m_vc_handle);
  vcsm_unlock_ptr(m_arm);
  vcsm_free(m_vcsm_handle);
}

// Call this to clean and invalidate a region of memory
void CGPUMEM::Flush()
{
  struct vcsm_user_clean_invalid_s iocache = {};
  iocache.s[0].handle = m_vcsm_handle;
  iocache.s[0].cmd = 3; // clean+invalidate
  iocache.s[0].addr = (int) m_arm;
  iocache.s[0].size  = m_numbytes;
  vcsm_clean_invalid( &iocache );
}

#include "cores/VideoPlayer/DVDClock.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"

extern "C"
{
  #include "libswscale/swscale.h"
  #include "libavutil/imgutils.h"
  #include "libavcodec/avcodec.h"
}

CPixelConverter::CPixelConverter() :
  m_width(0),
  m_height(0),
  m_swsContext(nullptr),
  m_buf(nullptr),
  m_processInfo(CProcessInfo::CreateInstance())
{
}

bool CPixelConverter::Open(AVPixelFormat pixfmt, AVPixelFormat targetfmt, unsigned int width, unsigned int height, void *opaque)
{
  CLog::Log(LOGDEBUG, "CPixelConverter::%s: pixfmt:%d tarfgetfmt:%d %dx%d opaque:%p", __FUNCTION__, pixfmt, targetfmt, width, height, opaque);
  if (pixfmt == targetfmt || width == 0 || height == 0)
    return false;

  m_vcffmpeg = new CDVDVideoCodecFFmpeg(*m_processInfo);
  m_decoder = new MMAL::CDecoder;

  memset(&m_avctx, 0, sizeof m_avctx);
  m_avctx.pix_fmt = AV_PIX_FMT_YUV420P;
  m_avctx.opaque = (void *)m_vcffmpeg;
  CDVDStreamInfo hints = {};
  hints.codec = AV_CODEC_ID_H264; // dummy
  CDVDCodecOptions options =  {};
  if (!m_vcffmpeg->Open(hints, options))
    CLog::Log(LOGERROR, "%s: Unable to open DVDVideoCodecFFMpeg", __FUNCTION__);

  m_avctx.hwaccel_context = opaque;
  if (m_decoder->Open(&m_avctx, &m_avctx, m_avctx.pix_fmt, 1))
    m_vcffmpeg->SetHardware(m_decoder);
  else
    CLog::Log(LOGERROR, "%s: Unable to open MMALFFmpeg", __FUNCTION__);

  m_width = width;
  m_height = height;

  m_swsContext = sws_getContext(width, height, pixfmt,
                                width, height, targetfmt,
                                SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (!m_swsContext)
  {
    CLog::Log(LOGERROR, "%s: Failed to create swscale context", __FUNCTION__);
    return false;
  }

  return true;
}

// allocate a new picture (AV_PIX_FMT_YUV420P)
AVFrame* CPixelConverter::AllocatePicture(int iWidth, int iHeight)
{
  AVFrame* frame = new AVFrame;
  if (frame)
  {
    frame->width = iWidth;
    frame->height = iHeight;
    frame->format = m_avctx.pix_fmt;
    m_decoder->FFGetBuffer(&m_avctx, frame, 0);
  }
  return frame;
}

void CPixelConverter::FreePicture(AVFrame* frame)
{
  assert(frame);
  AVBufferRef *buf = frame->buf[0];
  CGPUMEM *gmem = (CGPUMEM *)av_buffer_get_opaque(buf);
  m_decoder->FFReleaseBuffer(gmem, nullptr);
  delete frame;
}

void CPixelConverter::Dispose()
{
  delete m_vcffmpeg;
  m_vcffmpeg = nullptr;

  m_decoder->Close();
  m_decoder = nullptr;

  delete m_processInfo;
  m_processInfo = nullptr;

  if (m_swsContext)
  {
    sws_freeContext(m_swsContext);
    m_swsContext = nullptr;
  }

  if (m_buf)
  {
    FreePicture(m_buf);
    m_buf = nullptr;
  }
}

bool CPixelConverter::Decode(const uint8_t* pData, unsigned int size)
{
  if (pData == nullptr || size == 0 || m_swsContext == nullptr)
    return false;

  if (m_buf)
    FreePicture(m_buf);
  m_buf = AllocatePicture(m_width, m_height);
  if (!m_buf)
  {
    CLog::Log(LOGERROR, "%s: Failed to allocate picture of dimensions %dx%d", __FUNCTION__, m_width, m_height);
    return false;
  }

  uint8_t* dataMutable = const_cast<uint8_t*>(pData);

  const int stride = size / m_height;

  uint8_t* src[] =       { dataMutable,         0,                   0,                   0 };
  int      srcStride[] = { stride,              0,                   0,                   0 };
  uint8_t* dst[] =       { m_buf->data[0],      m_buf->data[1],      m_buf->data[2],      0 };
  int      dstStride[] = { m_buf->linesize[0],  m_buf->linesize[1],  m_buf->linesize[2],  0 };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);

  return true;
}

void CPixelConverter::GetPicture(DVDVideoPicture& dvdVideoPicture)
{
  if (!m_decoder->GetPicture(&m_avctx, m_buf, &dvdVideoPicture))
    CLog::Log(LOGERROR, "CPixelConverter::AllocatePicture, failed to GetPicture.");

  dvdVideoPicture.dts            = DVD_NOPTS_VALUE;
  dvdVideoPicture.pts            = DVD_NOPTS_VALUE;

  dvdVideoPicture.iFlags         = 0; // *not* DVP_FLAG_ALLOCATED
  dvdVideoPicture.color_matrix   = 4; // CONF_FLAGS_YUVCOEF_BT601
  dvdVideoPicture.color_range    = 0; // *not* CONF_FLAGS_YUV_FULLRANGE
  dvdVideoPicture.iWidth         = m_width;
  dvdVideoPicture.iHeight        = m_height;
  dvdVideoPicture.iDisplayWidth  = m_width; // TODO: Update if aspect ratio changes
  dvdVideoPicture.iDisplayHeight = m_height;
}

#endif
