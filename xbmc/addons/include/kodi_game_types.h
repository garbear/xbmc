/*
 *      Copyright (C) 2014 Team XBMC
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
#ifndef KODI_GAME_TYPES_H_
#define KODI_GAME_TYPES_H_

/* current game API version */
#define GAME_API_VERSION                "1.0.0"

/* min. game API version */
#define GAME_MIN_API_VERSION            "1.0.0"

/* magic number for empty tray */
#define GAME_NO_DISK                   ((unsigned)-1)

#include <stddef.h>
#include <stdint.h>

#ifdef TARGET_WINDOWS
  #include <windows.h>
#else
  #ifndef __cdecl
    #define __cdecl
  #endif
  #ifndef __declspec
    #define __declspec(X)
  #endif
#endif

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
  #if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
    #define ATTRIBUTE_PACKED __attribute__ ((packed))
    #define PRAGMA_PACK 0
  #endif
#endif

#if !defined(ATTRIBUTE_PACKED)
  #define ATTRIBUTE_PACKED
  #define PRAGMA_PACK 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! Game add-on error codes */
typedef enum GAME_ERROR
{
  GAME_ERROR_NO_ERROR                =  0, // no error occurred
  GAME_ERROR_UNKNOWN                 = -1, // an unknown error occurred
  GAME_ERROR_NOT_IMPLEMENTED         = -2, // the method that the frontend called is not implemented
  GAME_ERROR_REJECTED                = -3, // the command was rejected by the game client
  GAME_ERROR_INVALID_PARAMETERS      = -4, // the parameters of the method that was called are invalid for this operation
  GAME_ERROR_FAILED                  = -5, // the command failed
} GAME_ERROR;

typedef enum GAME_RENDER_FORMAT
{
  GAME_RENDER_FMT_NONE = 0,
  GAME_RENDER_FMT_YUV420P,
  GAME_RENDER_FMT_0RGB8888,
  GAME_RENDER_FMT_RGB565,
  GAME_RENDER_FMT_0RGB1555,
} GAME_RENDER_FORMAT;

typedef enum GAME_AUDIO_FORMAT
{
  GAME_AUDIO_FMT_NONE = 0,
  GAME_AUDIO_FMT_S16NE,
} GAME_AUDIO_FORMAT;

typedef enum GAME_INPUT_EVENT_SOURCE
{
  GAME_INPUT_EVENT_DIGITAL_BUTTON,
  GAME_INPUT_EVENT_ANALOG_BUTTON,
  GAME_INPUT_EVENT_ANALOG_STICK,
  GAME_INPUT_EVENT_ACCELEROMETER,
  GAME_INPUT_EVENT_KEY,
  GAME_INPUT_EVENT_MOUSE,
} GAME_INPUT_EVENT_SOURCE;

/*! Returned from game_get_region() (TODO) */
typedef enum GAME_REGION
{
  GAME_REGION_NTSC                   = 0,
  GAME_REGION_PAL                    = 1,
} GAME_REGION;

typedef enum GAME_MEMORY
{
  /*!
    * Passed to game_get_memory_data/size(). If the memory type doesn't apply
    * to the implementation NULL/0 can be returned.
    */
  GAME_MEMORY_MASK                   = 0xff,

  /*!
    * Regular save ram. This ram is usually found on a game cartridge, backed
    * up by a battery. If save game data is too complex for a single memory
    * buffer, the SYSTEM_DIRECTORY environment callback can be used.
    */
  GAME_MEMORY_SAVE_RAM               = 0,

  /*!
    * Some games have a built-in clock to keep track of time. This memory is
    * usually just a couple of bytes to keep track of time.
    */
  GAME_MEMORY_RTC                    = 1,

  /*! System ram lets a frontend peek into a game systems main RAM */
  GAME_MEMORY_SYSTEM_RAM             = 2,

  /*! Video ram lets a frontend peek into a game systems video RAM (VRAM) */
  GAME_MEMORY_VIDEO_RAM              = 3,

  /*! Special memory types */
  GAME_MEMORY_SNES_BSX_RAM           = ((1 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_BSX_PRAM          = ((2 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_SUFAMI_TURBO_A_RAM= ((3 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_SUFAMI_TURBO_B_RAM= ((4 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_GAME_BOY_RAM      = ((5 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_GAME_BOY_RTC      = ((6 << 8) | GAME_MEMORY_RTC)
} GAME_MEMORY;

/*!
  * Special game types passed into game_load_game_special(). Only used when
  * multiple ROMs are required.
  */
typedef enum GAME_TYPE
{
  GAME_TYPE_BSX                      = 0x101,
  GAME_TYPE_BSX_SLOTTED              = 0x102,
  GAME_TYPE_SUFAMI_TURBO             = 0x103,
  GAME_TYPE_SUPER_GAME_BOY           = 0x104
} GAME_TYPE;

typedef enum GAME_KEY_MOD
{
  GAME_KEY_MOD_NONE                  = 0x0000,

  GAME_KEY_MOD_SHIFT                 = 0x01,
  GAME_KEY_MOD_CTRL                  = 0x02,
  GAME_KEY_MOD_ALT                   = 0x04,
  GAME_KEY_MOD_META                  = 0x08,

  GAME_KEY_MOD_NUMLOCK               = 0x10,
  GAME_KEY_MOD_CAPSLOCK              = 0x20,
  GAME_KEY_MOD_SCROLLOCK             = 0x40,
} GAME_KEY_MOD;

typedef enum GAME_LOG_LEVEL /* retro_log_level in libretro */
{
  GAME_LOG_DEBUG                     = 0,
  GAME_LOG_INFO                      = 1,
  GAME_LOG_WARN                      = 2,
  GAME_LOG_ERROR                     = 3
} GAME_LOG_LEVEL;

/*! ID values for SIMD CPU features */
typedef enum GAME_SIMD
{
  GAME_SIMD_SSE                      = (1 << 0),
  GAME_SIMD_SSE2                     = (1 << 1),
  GAME_SIMD_VMX                      = (1 << 2),
  GAME_SIMD_VMX128                   = (1 << 3),
  GAME_SIMD_AVX                      = (1 << 4),
  GAME_SIMD_NEON                     = (1 << 5),
  GAME_SIMD_SSE3                     = (1 << 6),
  GAME_SIMD_SSSE3                    = (1 << 7),
  GAME_SIMD_MMX                      = (1 << 8),
  GAME_SIMD_MMXEXT                   = (1 << 9),
  GAME_SIMD_SSE4                     = (1 << 10),
  GAME_SIMD_SSE42                    = (1 << 11),
  GAME_SIMD_AVX2                     = (1 << 12),
  GAME_SIMD_VFPU                     = (1 << 13),
} GAME_SIMD;

typedef enum GAME_CAMERA_BUFFER /* retro_camera_buffer in libretro */
{
  GAME_CAMERA_BUFFER_OPENGL_TEXTURE  = 0,
  GAME_CAMERA_BUFFER_RAW_FRAMEBUFFER = 1
} GAME_CAMERA_BUFFER;


typedef enum GAME_RUMBLE_EFFECT /* retro_rumble_effect in libretro */
{
  GAME_RUMBLE_STRONG                 = 0,
  GAME_RUMBLE_WEAK                   = 1
} GAME_RUMBLE_EFFECT;

// TODO
typedef enum GAME_HW_FRAME_BUFFER
{
  GAME_HW_FRAME_BUFFER_VALID         = -1, // Pass this to game_video_refresh if rendering to hardware
  GAME_HW_FRAME_BUFFER_DUPLICATE     = 0,  // Passing NULL to game_video_refresh is still a frame dupe as normal
  GAME_HW_FRAME_BUFFER_RENDER        = 1,
} GAME_HW_FRAME_BUFFER;

typedef enum GAME_HW_CONTEXT_TYPE /* retro_hw_context_type in libretro */
{
  GAME_HW_CONTEXT_NONE               = 0,
  GAME_HW_CONTEXT_OPENGL             = 1, // OpenGL 2.x. Latest version available before 3.x+. Driver can choose to use latest compatibility context
  GAME_HW_CONTEXT_OPENGLES2          = 2, // GLES 2.0
  GAME_HW_CONTEXT_OPENGL_CORE        = 3, // Modern desktop core GL context. Use major/minor fields to set GL version
  GAME_HW_CONTEXT_OPENGLES3          = 4, // GLES 3.0
} GAME_HW_CONTEXT_TYPE;

typedef enum GAME_ROTATION
{
  GAME_ROTATION_0_CW                 = 0,
  GAME_ROTATION_90_CW                = 1,
  GAME_ROTATION_180_CW               = 2,
  GAME_ROTATION_270_CW               = 3
} GAME_ROTATION;

typedef enum GAME_EJECT_STATE
{
  GAME_NOT_EJECTED,
  GAME_EJECTED
} GAME_EJECT_STATE;

typedef struct game_input_device_caps
{
  unsigned int digital_button_count;
  unsigned int analog_button_count;
  unsigned int analog_stick_count;
  unsigned int accelerometer_count;
  unsigned int key_count;
  unsigned int mouse_count;
} ATTRIBUTE_PACKED game_input_device_caps;

typedef struct game_input_event
{
  GAME_INPUT_EVENT_SOURCE type;
  int                     port;
  unsigned int            source_index;
  union
  {
    struct
    {
      bool         pressed;
    } digital_button;

    struct
    {
      float        magnitude;
    } analog_button;

    struct
    {
      float        x;
      float        y;
    } analog_stick;

    struct
    {
      float        x;
      float        y;
      float        z;
    } accelerometer;

    struct
    {
      bool         pressed;
      uint32_t     character; // text character of the pressed key (UTF-32)
      GAME_KEY_MOD modifiers;
    } key;

    struct
    {
      int          x;
      int          y;
    } mouse;
  };
} ATTRIBUTE_PACKED game_input_event;

struct game_geometry
{
  unsigned base_width;          // Nominal video width of game
  unsigned base_height;         // Nominal video height of game
  unsigned max_width;           // Maximum possible width of game
  unsigned max_height;          // Maximum possible height of game
  float    aspect_ratio;        // Nominal aspect ratio of game. If aspect_ratio is <= 0.0,
                                // an aspect ratio of base_width / base_height is assumed.
                                // A frontend could override this setting if desired.
};

struct game_system_timing
{
  double fps;                   // FPS of video content.
  double sample_rate;           // Sampling rate of audio.
};

struct game_system_av_info
{
  struct game_geometry geometry;
  struct game_system_timing timing;
};

typedef uint64_t  game_perf_tick_t;
typedef int64_t   game_time_t;
typedef int64_t   game_usec_t;
typedef void      (*game_proc_address_t)(void);

struct game_perf_counter
{
  const char       *ident;
  game_perf_tick_t start;
  game_perf_tick_t total;
  game_perf_tick_t call_cnt;
  bool              registered;
};

struct game_camera_info
{
  uint64_t caps;                // Set by game client. Example bitmask: caps = (1 << GAME_CAMERA_BUFFER_OPENGL_TEXTURE) | (1 << GAME_CAMERA_BUFFER_RAW_FRAMEBUFFER).
  unsigned width;               // Desired resolution for camera. Is only used as a hint.
  unsigned height;
};

struct game_hw_info
{
  enum GAME_HW_CONTEXT_TYPE context_type; // Which API to use. Set by game client.
  bool     depth;               // Set if render buffers should have depth component attached.
  bool     stencil;             // Set if stencil buffers should be attached.
                                // If depth and stencil are true, a packed 24/8 buffer will be added. Only attaching stencil is invalid and will be ignored.
  bool     bottom_left_origin;  // Use conventional bottom-left origin convention. Is false, standard top-left origin semantics are used.
  unsigned version_major;       // Major version number for core GL context.
  unsigned version_minor;       // Minor version number for core GL context.
  bool     cache_context;       // If this is true, the frontend will go very far to avoid resetting context in scenarios like toggling fullscreen, etc.
                                // The reset callback might still be called in extreme situations such as if the context is lost beyond recovery.
                                // For optimal stability, set this to false, and allow context to be reset at any time.
  bool     debug_context;       // Creates a debug context.
};

/*! Properties passed to the ADDON_Create() method of a game client */
typedef struct game_client_properties
{
  /*!
   * Path to the game client's shared library being loaded. Replaces
   * RETRO_ENVIRONMENT_GET_LIBRETRO_PATH.
   */
  const char* library_path;

  /*!
   * The "system" directory of the frontend. This directory can be used to
   * store system-specific ROMs such as BIOSes, configuration data, etc.
   * Replaces RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY.
   */
  const char* system_directory;

  /*!
    * The "content" directory of the frontend. This directory can be used to
    * store specific assets that the core relies upon, such as art assets, input
    * data, etc. Replaces RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY.
    */
  const char* content_directory;

  /*!
    * The "save" directory of the frontend. This directory can be used to store
    * SRAM, memory cards, high scores, etc, if the game client cannot use the
    * regular memory interface, GetMemoryData(). Replaces
    * RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY.
    */
  const char* save_directory;
} game_client_properties;

/*! Structure to transfer the methods from kodi_game_dll.h to Kodi */
typedef struct GameClient
{
  const char* (__cdecl* GetGameAPIVersion)(void);
  const char* (__cdecl* GetMininumGameAPIVersion)(void);
  GAME_ERROR  (__cdecl* LoadGame)(const char* url);
  GAME_ERROR  (__cdecl* LoadGameSpecial)(GAME_TYPE type, const char** urls, size_t num_urls);
  GAME_ERROR  (__cdecl* UnloadGame)(void);
  GAME_ERROR  (__cdecl* Run)(void);
  GAME_ERROR  (__cdecl* Reset)(void);
  void        (__cdecl* UpdatePort)(unsigned int port, bool port_connected);
  void        (__cdecl* InputEvent)(unsigned int port, game_input_event* event);
  GAME_ERROR  (__cdecl* GetSystemAVInfo)(struct game_system_av_info *info);
  size_t      (__cdecl* SerializeSize)(void);
  GAME_ERROR  (__cdecl* Serialize)(void *data, size_t size);
  GAME_ERROR  (__cdecl* Deserialize)(const void *data, size_t size);
  GAME_ERROR  (__cdecl* CheatReset)(void);
  GAME_ERROR  (__cdecl* CheatSet)(unsigned index, bool enabled, const char *code);
  GAME_REGION (__cdecl* GetRegion)(void);
  void*       (__cdecl* GetMemoryData)(GAME_MEMORY id);
  size_t      (__cdecl* GetMemorySize)(GAME_MEMORY id);
  GAME_ERROR  (__cdecl* DiskSetEjectState)(GAME_EJECT_STATE ejected);
  GAME_EJECT_STATE (__cdecl* DiskGetEjectState)(void);
  unsigned    (__cdecl* DiskGetImageIndex)(void);
  GAME_ERROR  (__cdecl* DiskSetImageIndex)(unsigned index);
  unsigned    (__cdecl* DiskGetNumImages)(void);
  GAME_ERROR  (__cdecl* DiskReplaceImageIndex)(unsigned index, const char* url);
  GAME_ERROR  (__cdecl* DiskAddImageIndex)(void);
  GAME_ERROR  (__cdecl* HwContextReset)(void);
  GAME_ERROR  (__cdecl* HwContextDestroy)(void);
  GAME_ERROR  (__cdecl* AudioAvailable)(void);
  GAME_ERROR  (__cdecl* AudioSetState)(bool enabled);
  GAME_ERROR  (__cdecl* FrameTimeNotify)(game_usec_t usec);
  GAME_ERROR  (__cdecl* CameraInitialized)(void);
  GAME_ERROR  (__cdecl* CameraDeinitialized)(void);
  GAME_ERROR  (__cdecl* CameraFrameRawBuffer)(const uint32_t *buffer, unsigned width, unsigned height, size_t pitch);
  GAME_ERROR  (__cdecl* CameraFrameOpenglTexture)(unsigned texture_id, unsigned texture_target, const float *affine);
} GameClient;

#ifdef __cplusplus
}
#endif

#endif // KODI_GAME_TYPES_H_
