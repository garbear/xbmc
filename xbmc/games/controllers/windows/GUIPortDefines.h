/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Duration to wait for input from the user
#define COUNTDOWN_DURATION_SEC 6

// Warn the user that time is running out after this duration
#define WAIT_TO_WARN_SEC 2

#define CONTROL_PORT_DIALOG_LABEL 2

// GUI Control IDs
#define CONTROL_PORT_LIST 3
#define CONTROL_PORT_BUTTON_TEMPLATE 10

// GUI button IDs
#define CONTROL_CLOSE_BUTTON 18
#define CONTROL_RESET_BUTTON 19

#define MAX_PORT_COUNT 100 // large enough

#define CONTROL_PORT_BUTTONS_START 100
#define CONTROL_PORT_BUTTONS_END (CONTROL_PORT_BUTTONS_START + MAX_PORT_COUNT)

#define PORT_DIALOG_XML "DialogGameControllers.xml"
