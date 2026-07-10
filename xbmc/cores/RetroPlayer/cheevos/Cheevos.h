/*
 *  Copyright (C) 2020-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <mutex>
#include <thread>
#include <vector>

// rc_client forward declarations — keeps rcheevos headers out of
// every translation unit that includes this header
struct rc_client_t;
struct rc_client_event_t;
struct rc_api_request_t;
struct rc_api_server_response_t;
struct rc_libretro_core_memory_info_t;
typedef void (*rc_client_server_callback_t)(const struct rc_api_server_response_t*,
                                             void*);

namespace KODI
{
namespace GAME
{
class CGameClient;
} // namespace GAME

namespace RETRO
{

class CCheevos
{
public:
  CCheevos(GAME::CGameClient* gameClient,
           const std::string& userName,
           const std::string& loginToken);
  ~CCheevos();

  // Called by RetroPlayer on game start
  void EnableRichPresence();

  // Called every emulator frame from ReversiblePlayback
  void DoFrame();

  // Called by ReversiblePlayback for savestate metadata
  std::string GetRichPresenceEvaluation();

  // Exchanges password for RA token
  bool RCLogin(const std::string& password);

private:
  // rc_client static callbacks
  static uint32_t RcheevosReadMemory(uint32_t address, uint8_t* buffer,
                                      uint32_t num_bytes, rc_client_t* client);
  static void RcheevosServerCall(const rc_api_request_t* request,
                                  rc_client_server_callback_t callback,
                                  void* callback_data, rc_client_t* client);
  static void RcheevosEventHandler(const rc_client_event_t* event,
                                    rc_client_t* client);
  static void RcheevosLoginCallback(int result, const char* errorMessage,
                                     rc_client_t* client, void* userdata);
  static void RcheevosGameLoadCallback(int result, const char* errorMessage,
                                        rc_client_t* client, void* userdata);
  static void RcheevosGetCoreMemoryInfo(uint32_t id,
                                         rc_libretro_core_memory_info_t* info);

  void RichPresencePingThread();

  // rc_libretro_memory_regions_t stored as void* pimpl so callers of
  // Cheevos.h don't need the rcheevos headers on their include path.
  void* m_rcMemoryRegions{nullptr};

  GAME::CGameClient* m_gameClient;
  std::string m_userName;
  std::string m_loginToken;

  rc_client_t* m_rcClient{nullptr};

  std::atomic<bool> m_richPresenceRunning{false};
  std::thread m_richPresenceThread;
  mutable std::mutex m_credentialsMutex; ///< Guards m_userName/m_loginToken

  std::vector<std::thread> m_downloadThreads;
  std::mutex m_downloadThreadsMutex;


  static thread_local CCheevos* s_initializingCheevos;
};

} // namespace RETRO
} // namespace KODI
