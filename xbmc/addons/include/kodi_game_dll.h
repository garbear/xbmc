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
#ifndef KODI_GAME_DLL_H_
#define KODI_GAME_DLL_H_

#include "kodi_game_types.h"

/*! Functions that the game client add-on must implement */
#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Get the GAME_API_VERSION_STRING that was used to compile this game client.
 * Used to check if the implementation is compatible with the frontend.
 */
const char* GetGameAPIVersion(void);

/*!
 * Get the GAME_MIN_API_VERSION_STRING that was used to compile this game client.
 * Used to check if the implementation is compatible with the frontend.
 */
const char* GetMininumGameAPIVersion(void);

/*! Loads a game */
GAME_ERROR LoadGame(const char* url);

/*! Loads a "special" kind of game. Should not be used except in extreme cases */
GAME_ERROR LoadGameSpecial(GAME_TYPE type, const char** urls, size_t num_urls);

/*! Unloads a currently loaded game */
GAME_ERROR UnloadGame(void);

/*!
 * Runs the game for one video frame.
 *
 * If a frame is not rendered for reasons where a game "dropped" a frame, this
 * still counts as a frame, and Run() should explicitly dupe a frame if
 * GET_CAN_DUPE returns true. In this case, the video callback can take a NULL
 * argument for data.
 */
GAME_ERROR Run(void);

/*! Resets the current game */
GAME_ERROR Reset(void);

/*!
 * Set the status of an open port.
 */
void UpdatePort(unsigned int port, bool port_connected);

/*!
 * Called in response to an input event on an open port.
 */
void InputEvent(unsigned int port, game_input_event* event);

/*!
 * Gets information about system audio/video timings and geometry. Can be
 * called only after Load() has successfully completed.
 * NOTE: The implementation of this function might not initialize every
 * variable if needed. E.g. geom.aspect_ratio might not be initialized if
 * core doesn't desire a particular aspect ratio.
 */
GAME_ERROR GetSystemAVInfo(struct game_system_av_info *info);

/*!
 * Returns the amount of data the implementation requires to serialize
 * internal state (save states). Between calls to Load() and
 * Unload(), the returned size is never allowed to be larger than
 * a previous returned value, to ensure that the frontend can allocate a save
 * state buffer once.
 */
size_t SerializeSize(void);

/*!
 * Serializes internal state. It should return GAME_ERROR_FAILED if failed
 * or size is lower than SerializeSize(), and GAME_ERROR_NO_ERROR on
 * success.
 */
GAME_ERROR Serialize(void *data, size_t size);
GAME_ERROR Deserialize(const void *data, size_t size);

GAME_ERROR CheatReset(void);
GAME_ERROR CheatSet(unsigned index, bool enabled, const char *code);

/*! Gets the region of the game */
GAME_REGION GetRegion(void);

/*! Gets region of memory */
void* GetMemoryData(GAME_MEMORY id);
size_t GetMemorySize(GAME_MEMORY id);

/*!
 * Interface to eject and insert disk images. This is used for games which
 * consist of multiple images and must be manually swapped out by the user
 * (e.g. PSX) during runtime. If the implementation can do this
 * automatically, it should strive to do so. However, there are cases where
 * the user must manually do so.
 *
 * Overview: To swap a disk image, eject the disk image with
 * DiskControlSetEjectState(true). Set the disk index with
 * DiskControlSetImageIndex(index). Insert the disk again with
 * DiskControlSetEjectState(false).
 *
 * Replaces RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE.
 */

/*!
 * If ejected is true, "ejects" the virtual disk tray. When ejected, the
 * disk image index can be set.
 */
GAME_ERROR DiskSetEjectState(GAME_EJECT_STATE ejected);

/*! Gets current eject state. The initial state is not ejected. */
GAME_EJECT_STATE DiskGetEjectState(void);

/*!
 * Gets current disk index. First disk is index 0. If return value
 * is >= DiskControlGetNumImages(), no disk is currently inserted.
 */
unsigned DiskGetImageIndex(void);

/*!
 * Sets image index. Can only be called when disk is ejected. The
 * implementation supports setting "no disk" (empty tray) by setting index to
 * GAME_NO_DISK.
 */
GAME_ERROR DiskSetImageIndex(unsigned index);

/*! Gets total number of images which are available to use */
unsigned DiskGetNumImages(void);

/*!
 * Replaces the disk image associated with index. Arguments to pass in info
 * have same requirements as Load(). Virtual disk tray must be
 * ejected when calling this. Replacing a disk image with info = NULL will
 * remove the disk image from the internal list. As a result, calls to
 * DiskControlGetImageIndex() can change.
 *
 * E.g. DiskControlReplaceImageIndex(1, NULL), and previous
 * DiskControlGetImageIndex() returned 4 before. Index 1 will be
 * removed, and the new index is 3.
 */
GAME_ERROR DiskReplaceImageIndex(unsigned index, const char* url);

/*!
 * Adds a new valid index (DiskControlGetNumImages()) to the internal disk list.
 * This will increment subsequent return values from DiskControlGetNumImages() by
 * 1. This image index cannot be used until a disk image has been set with
 * replace_image_index.
 */
GAME_ERROR DiskAddImageIndex(void);

/*!
 * Invalidates the current HW context. Any GL state is lost, and must not
 * be deinitialized explicitly. If explicit deinitialization is desired by
 * the game client, it should implement context_destroy callback. If
 * called, all GPU resources must be reinitialized. Usually called when
 * frontend reinits video driver. Also called first time video driver is
 * initialized, allowing the client to init resources.
 */
GAME_ERROR HwContextReset(void);

/*!
 * Called before the context is destroyed. Resources can be deinitialized
 * at this step.
 */
GAME_ERROR HwContextDestroy(void);

/*!
 * Interface which is used to notify a game client about audio being
 * available for writing. The function can be called from any thread, so
 * a core using this must have a thread safe audio implementation. It is
 * intended for games where audio and video are completely asynchronous
 * and audio can be generated on the fly. This interface is not recommended
 * for use with emulators which have highly synchronous audio.
 *
 * The function only notifies about writability; the game client still
 * has to call the normal audio functions to write audio. The audio
 * functions must be called from within the notification function. The
 * amount of audio data to write is up to the implementation. Generally,
 * the audio function will be called continuously in a loop.
 *
 * Due to thread safety guarantees and lack of sync between audio and
 * video, a frontend can selectively disallow this interface based on
 * internal configuration. A core using this interface must also implement
 * the "normal" audio interface.
 *
 * Replaces RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK.
 */
GAME_ERROR AudioAvailable(void);

/*!
 * If enabled is true, audio driver in frontend is active, and callback is
 * expected to be called regularly. If enabled is false, audio driver in
 * frontend is paused or inactive. Audio callback will not be called until
 * audio_set_state() has been called with true. Initial state is false
 * (inactive).
 */
GAME_ERROR AudioSetState(bool enabled);

/*!
 * Frame time interface. Replaces RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK.
 *
 * Notifies a game client of time spent since last invocation of
 * Run() in microseconds. It will be called right before Run()
 * every frame.The frontend can tamper with the timing to
 * support cases like fast-forward, slow-motion, frame stepping, etc. In this case the
 * delta time will use the reference value in frame_time_callback.
 */
GAME_ERROR FrameTimeNotify(game_usec_t usec);

/*!
 * Signals when the camera driver is initialized. CameraStart() can be called
 * from the client.
 */
GAME_ERROR CameraInitialized(void);

/*! Signals when the camera driver is deinitialized */
GAME_ERROR CameraDeinitialized(void);

/*!
 * A function for raw framebuffer data. buffer points to an XRGB8888 buffer.
 * width, height and pitch are similar to VideoRefresh(). First pixel is top-
 * left origin.
 */
GAME_ERROR CameraFrameRawBuffer(const uint32_t *buffer, unsigned width, unsigned height, size_t pitch);

/*!
 * A function for when OpenGL textures are used.
 *
 * texture_id is a texture owned by camera driver. Its state or content
 * should be considered immutable, except for things like texture filtering
 * and clamping.
 *
 * texture_target is the texture target for the GL texture. These can
 * include e.g. GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE, and possibly more
 * depending on extensions.
 *
 * affine points to a packed 3x3 column-major matrix used to apply an
 * affine transform to texture coordinates.
 * (affine_matrix * vec3(coord_x, coord_y, 1.0))
 * After transform, normalized texture coord (0, 0) should be bottom-left
 * and (1, 1) should be top-right (or (width, height) for RECTANGLE).
 *
 * GL-specific typedefs are avoided here to avoid relying on gl.h in the
 * API definition.
 */
GAME_ERROR CameraFrameOpenglTexture(unsigned texture_id, unsigned texture_target, const float *affine);

/*!
 * Called by Kodi to assign the function pointers of this add-on to pClient.
 * Note that get_addon() is defined here, so it will be available in all
 * compiled game clients.
 */
void __declspec(dllexport) get_addon(struct GameClient* pClient)
{
  pClient->GetGameAPIVersion        = GetGameAPIVersion;
  pClient->GetMininumGameAPIVersion = GetMininumGameAPIVersion;
  pClient->LoadGame                 = LoadGame;
  pClient->LoadGameSpecial          = LoadGameSpecial;
  pClient->UnloadGame               = UnloadGame;
  pClient->Run                      = Run;
  pClient->Reset                    = Reset;
  pClient->UpdatePort               = UpdatePort;
  pClient->InputEvent               = InputEvent;
  pClient->GetSystemAVInfo          = GetSystemAVInfo;
  pClient->SerializeSize            = SerializeSize;
  pClient->Serialize                = Serialize;
  pClient->Deserialize              = Deserialize;
  pClient->CheatReset               = CheatReset;
  pClient->CheatSet                 = CheatSet;
  pClient->GetRegion                = GetRegion;
  pClient->GetMemoryData            = GetMemoryData;
  pClient->GetMemorySize            = GetMemorySize;
  pClient->DiskSetEjectState        = DiskSetEjectState;
  pClient->DiskGetEjectState        = DiskGetEjectState;
  pClient->DiskGetImageIndex        = DiskGetImageIndex;
  pClient->DiskSetImageIndex        = DiskSetImageIndex;
  pClient->DiskGetNumImages         = DiskGetNumImages;
  pClient->DiskReplaceImageIndex    = DiskReplaceImageIndex;
  pClient->DiskAddImageIndex        = DiskAddImageIndex;
  pClient->HwContextReset           = HwContextReset;
  pClient->HwContextDestroy         = HwContextDestroy;
  pClient->AudioAvailable           = AudioAvailable;
  pClient->AudioSetState            = AudioSetState;
  pClient->FrameTimeNotify          = FrameTimeNotify;
  pClient->CameraInitialized        = CameraInitialized;
  pClient->CameraDeinitialized      = CameraDeinitialized;
  pClient->CameraFrameRawBuffer     = CameraFrameRawBuffer;
  pClient->CameraFrameOpenglTexture = CameraFrameOpenglTexture;
};

#ifdef __cplusplus
};
#endif

#endif // KODI_GAME_DLL_H_
