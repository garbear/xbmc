/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Skin XML files
constexpr auto SELECT_AVATAR_DIALOG_XML = "DialogSelect.xml";
constexpr auto DIALOG_AGENT_CONTROLLER_XML = "DialogSelect.xml";

// Name of list item property for savestate captions
constexpr auto SAVESTATE_LABEL = "savestate.label";
constexpr auto SAVESTATE_CAPTION = "savestate.caption";
constexpr auto SAVESTATE_GAME_CLIENT = "savestate.gameclient";
constexpr auto SAVESTATE_GAME_CLIENT_VERSION = "savestate.gameclientversion";

// String of list item property "game.videofilter" when no filter is set
constexpr auto PROPERTY_NO_VIDEO_FILTER = "";

// Control IDs for game dialogs
constexpr unsigned int CONTROL_VIDEO_HEADING = 10810;
constexpr unsigned int CONTROL_VIDEO_THUMBS = 10811;
constexpr unsigned int CONTROL_VIDEO_DESCRIPTION = 10812;
constexpr unsigned int CONTROL_SAVES_HEADING = 10820;
constexpr unsigned int CONTROL_SAVES_DETAILED_LIST = 3; // Select dialog defaults to this control ID
constexpr unsigned int CONTROL_SAVES_DESCRIPTION = 10822;
constexpr unsigned int CONTROL_SAVES_EMULATOR_NAME = 10823;
constexpr unsigned int CONTROL_SAVES_EMULATOR_ICON = 10824;
constexpr unsigned int CONTROL_SAVES_NEW_BUTTON = 10825;
constexpr unsigned int CONTROL_SAVES_CANCEL_BUTTON = 10826;
constexpr unsigned int CONTROL_NUMBER_OF_ITEMS = 10827;
constexpr unsigned int CONTROL_SAVES_EMULATOR_VERSION = 10828;

// Control IDs for avatar dialogs
constexpr unsigned int CONTROL_SELECT_AVATAR_DETAILED_LIST = 3;
constexpr unsigned int CONTROL_SELECT_AVATAR_READY_PLAYER_LABEL = 4;

// Control IDs for agent controller dialog
constexpr unsigned int CONTROL_AGENT_CONTROLLER_HEADER = 2;
constexpr unsigned int CONTROL_AGENT_CONTROLLER_DETAILED_LIST = 3;
constexpr unsigned int CONTROL_AGENT_CONTROLLER_OK_BUTTON = 18;
constexpr unsigned int CONTROL_AGENT_CONTROLLER_SETTINGS_BUTTON = 19;
constexpr unsigned int CONTROL_AGENT_CONTROLLER_RESET_BUTTON = 20;
