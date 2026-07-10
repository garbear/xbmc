#pragma once
#include <stddef.h>
#include <stdint.h>

/* Minimal libretro.h stub — only what rc_libretro needs */
#define RETRO_MEMORY_SAVE_RAM    0
#define RETRO_MEMORY_RTC         1
#define RETRO_MEMORY_SYSTEM_RAM  2
#define RETRO_MEMORY_VIDEO_RAM   3

#define RETRO_MEMFLAG_CONST       (1 << 0)
#define RETRO_MEMFLAG_BIGENDIAN   (1 << 1)
#define RETRO_MEMFLAG_ALIGN_4     (1 << 2)
#define RETRO_MEMFLAG_MINSIZE_8   (1 << 3)
#define RETRO_MEMFLAG_MINSIZE_16  (1 << 4)
#define RETRO_MEMFLAG_MINSIZE_32  (1 << 5)

struct retro_memory_descriptor
{
  uint64_t    flags;
  void*       ptr;
  size_t      offset;
  size_t      start;
  size_t      select;
  size_t      disconnect;
  size_t      len;
  const char* addrspace;
};

struct retro_memory_map
{
  const struct retro_memory_descriptor* descriptors;
  unsigned num_descriptors;
};
