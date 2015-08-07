/*
 *      Copyright (C) 2014-2015 Team XBMC
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
#ifndef KODI_GAME_CALLBACKS_H_
#define KODI_GAME_CALLBACKS_H_

#include "kodi_game_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CB_GameLib
{
  // --- Game callbacks --------------------------------------------------------

  /*!
   * \brief Requests the frontend to stop the current game
   */
  void (*CloseGame)(void* addonData);

  /*!
   * \brief Render a frame
   *
   * \param data The frame's data
   * \param size The size of the data
   * \param width The width, in pixels
   * \param height The height, in pixels
   * \param format The format of the frame being rendered
   */
  void (*VideoFrame)(void* addonData, const uint8_t* data, unsigned int size, unsigned int width, unsigned int height, GAME_RENDER_FORMAT format);

  /*!
   * \brief Render a H264 frame
   *
   * \param data The frame's data
   * \param size The size of the data
   * \param width The width, in pixels
   * \param height The height, in pixels
   */
  bool (*VideoFrameH264)(void* addonData, const uint8_t* data, unsigned int size, unsigned int width, unsigned int height);

  /*!
   * \brief Render a chunk of audio data
   *
   * \param data The audio data
   * \param size The size of the data
   * \param frames The number of frames; a left and right sample make one frame
   * \param format The format of the audio data
   */
  void (*AudioFrames)(void* addonData, const uint8_t* data, unsigned int size, unsigned int frames, GAME_AUDIO_FORMAT format);

  // -- Hardware rendering callbacks -------------------------------------------

  /*!
   * \brief Set info for hardware rendering
   *
   * \param hw_info A struct of properties for the hardware rendering system
   */
  void (*HwSetInfo)(void* addonData, const game_hw_info* hw_info);

  /*!
   * \brief Get the framebuffer for rendering
   *
   * \return The framebuffer
   */
  uintptr_t (*HwGetCurrentFramebuffer)(void* addonData);

  /*!
   * \brief Get a symbol from the hardware context
   *
   * \param symbol The symbol's name
   *
   * \return A function pointer for the specified symbol
   */
  game_proc_address_t (*HwGetProcAddress)(void* addonData, const char* symbol);

  // --- Input callbacks -------------------------------------------------------

  /*!
   * \brief Begin reporting events for the specified port
   *
   * \param port The zero-indexed port number
   */
  bool (*OpenPort)(void* addonData, unsigned int port);

  /*!
   * \brief End reporting events for the specified port
   *
   * \param port The port number passed to OpenPort()
   */
  void (*ClosePort)(void* addonData, unsigned int port);

  /*!
   * \brief Set the rumble state of a controller
   *
   * \param port The number passed when opening the port
   * \param effect Apply the rumble to the strong motor or the weak motor
   * \param strength The magnitude of the feedback in the closed interval [0.0, 1.0]
   */
  void (*RumbleSetState)(void* addonData, unsigned int port, GAME_RUMBLE_EFFECT effect, float strength);

} CB_GameLib;

#ifdef __cplusplus
}
#endif

#endif // KODI_GAME_CALLBACKS_H_
