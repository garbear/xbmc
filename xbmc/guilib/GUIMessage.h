/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMessage.h
\brief
*/

constexpr const int GUI_MSG_WINDOW_INIT = 1; // Initialize window
constexpr const int GUI_MSG_WINDOW_DEINIT = 2; // Deinit window
constexpr const int GUI_MSG_WINDOW_RESET = 27; // Reset window to initial state

constexpr const int GUI_MSG_SETFOCUS = 3; // Set focus to control param1=up/down/left/right
constexpr const int GUI_MSG_LOSTFOCUS = 4; // Control lost focus

constexpr const int GUI_MSG_CLICKED = 5; // Control has been clicked

constexpr const int GUI_MSG_VISIBLE = 6; // Set control visible
constexpr const int GUI_MSG_HIDDEN = 7; // Set control hidden

constexpr const int GUI_MSG_ENABLED = 8; // Enable control
constexpr const int GUI_MSG_DISABLED = 9; // Disable control

constexpr const int GUI_MSG_SET_SELECTED = 10; // Control = selected
constexpr const int GUI_MSG_SET_DESELECTED = 11; // Control = not selected

// Add label control (for controls supporting more then 1 label)
constexpr const int GUI_MSG_LABEL_ADD = 12;

// Set the label of a control
constexpr const int GUI_MSG_LABEL_SET = 13;

// Clear all labels of a control // add label control (for controls supporting more then 1 label)
constexpr const int GUI_MSG_LABEL_RESET = 14;

constexpr const int GUI_MSG_ITEM_SELECTED = 15; // Ask control 2 return the selected item
constexpr const int GUI_MSG_ITEM_SELECT = 16; // Ask control 2 select a specific item
constexpr const int GUI_MSG_LABEL2_SET = 17;
constexpr const int GUI_MSG_SHOWRANGE = 18;

constexpr const int GUI_MSG_FULLSCREEN = 19; // Should go to fullscreen window (vis or video)
constexpr const int GUI_MSG_EXECUTE = 20; // User has clicked on a button with <execute> tag

// Message will be send to all active and inactive(!) windows, all active modal
// and modeless dialogs dwParam1 must contain an additional message the windows
// should react on
constexpr const int GUI_MSG_NOTIFY_ALL = 21;

// Message is sent to all windows to refresh all thumbs
constexpr const int GUI_MSG_REFRESH_THUMBS = 22;

// Message is sent to the window from the base control class when it's been
// asked to move. dwParam1 contains direction.
constexpr const int GUI_MSG_MOVE = 23;

// Bind label control (for controls supporting more then 1 label)
constexpr const int GUI_MSG_LABEL_BIND = 24;

constexpr const int GUI_MSG_FOCUSED = 26; // A control has become focused

constexpr const int GUI_MSG_PAGE_CHANGE = 28; // A page control has changed the page number

// Message sent to all listing controls telling them to refresh their item layouts
constexpr const int GUI_MSG_REFRESH_LIST = 29;

constexpr const int GUI_MSG_PAGE_UP = 30; // Page up
constexpr const int GUI_MSG_PAGE_DOWN = 31; // Page down

// Instruct the control to MoveUp or MoveDown by offset amount
constexpr const int GUI_MSG_MOVE_OFFSET = 32;

constexpr const int GUI_MSG_SET_TYPE = 33; ///< Instruct a control to set it's type appropriately

/*!
 * \brief Message indicating the window has been resized
 *
 * Any controls that keep stored sizing information based on aspect ratio or
 * window size should recalculate sizing information.
 */
constexpr const int GUI_MSG_WINDOW_RESIZE = 34;

/*!
 * \brief Message indicating loss of renderer, prior to reset
 *
 * Any controls that keep shared resources should free them on receipt of this
 * message, as the renderer is about to be reset.
 */
constexpr const int GUI_MSG_RENDERER_LOST = 35;

/*!
 * \brief Message indicating regain of renderer, after reset
 *
 * Any controls that keep shared resources may reallocate them now that the
 * renderer is back.
 */
constexpr const int GUI_MSG_RENDERER_RESET = 36;

/*!
 * \brief A control wishes to have (or release) exclusive access to mouse actions
 */
constexpr const int GUI_MSG_EXCLUSIVE_MOUSE = 37;

/*!
 * \brief A request for supported gestures is made
 */
constexpr const int GUI_MSG_GESTURE_NOTIFY = 38;

/*!
 * \brief A request to add a control
 */
constexpr const int GUI_MSG_ADD_CONTROL = 39;

/*!
 * \brief A request to remove a control
 */
constexpr const int GUI_MSG_REMOVE_CONTROL = 40;

/*!
 * \brief A request to unfocus all currently focused controls
 */
constexpr const int GUI_MSG_UNFOCUS_ALL = 41;

constexpr const int GUI_MSG_SET_TEXT = 42;

constexpr const int GUI_MSG_WINDOW_LOAD = 43;

constexpr const int GUI_MSG_VALIDITY_CHANGED = 44;

/*!
 * \brief Check whether a button is selected
 */
constexpr const int GUI_MSG_IS_SELECTED = 45;

/*!
 * \brief Bind a set of labels to a spin (or similar) control
 */
constexpr const int GUI_MSG_SET_LABELS = 46;

/*!
 * \brief Set the filename for an image control
 */
constexpr const int GUI_MSG_SET_FILENAME = 47;

/*!
 * \brief Get the filename of an image control
 */
constexpr const int GUI_MSG_GET_FILENAME = 48;

/*!
 * \brief The user interface is ready for usage
 */
constexpr const int GUI_MSG_UI_READY = 49;

/*!
 * \brief Called every 500ms to allow time dependent updates
 */
constexpr const int GUI_MSG_REFRESH_TIMER = 50;

/*!
 * \brief Called if state has changed which could lead to GUI changes
 */
constexpr const int GUI_MSG_STATE_CHANGED = 51;

/*!
 * \brief Called when a subtitle download has finished
 */
constexpr const int GUI_MSG_SUBTITLE_DOWNLOADED = 52;

constexpr const int GUI_MSG_USER = 1000;

/*!
 * \brief Complete to get codingtable page
 */
constexpr const int GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED = 65000;

/*!
 * \ingroup winmsg
 */
#define CONTROL_SELECT(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_SELECTED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define CONTROL_DESELECT(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_DESELECTED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define CONTROL_ENABLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_ENABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define CONTROL_DISABLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_DISABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define CONTROL_ENABLE_ON_CONDITION(controlID, bCondition) \
  do \
  { \
    CGUIMessage _msg(bCondition ? GUI_MSG_ENABLED : GUI_MSG_DISABLED, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define CONTROL_SELECT_ITEM(controlID, iItem) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_ITEM_SELECT, GetID(), controlID, iItem); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 * \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 * \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL_THREAD_SAFE(controlID, label) \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    if (CServiceBroker::GetAppMessenger()->IsProcessThread()) \
      OnMessage(_msg); \
    else \
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(_msg, GetID()); \
  }

/*!
 * \ingroup winmsg
 * \brief Set the second label of the current control
 */
#define SET_CONTROL_LABEL2(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_LABEL2_SET, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 * \brief Set a bunch of labels on the given control
 */
#define SET_CONTROL_LABELS(controlID, defaultValue, labels) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_LABELS, GetID(), controlID, defaultValue); \
    _msg.SetPointer(labels); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 * \brief Set the label of the current control
 */
#define SET_CONTROL_FILENAME(controlID, label) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SET_FILENAME, GetID(), controlID); \
    _msg.SetLabel(label); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define SET_CONTROL_HIDDEN(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_HIDDEN, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define SET_CONTROL_FOCUS(controlID, dwParam) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_SETFOCUS, GetID(), controlID, dwParam); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 */
#define SET_CONTROL_VISIBLE(controlID) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_VISIBLE, GetID(), controlID); \
    OnMessage(_msg); \
  } while (0)

#define SET_CONTROL_SELECTED(dwSenderId, controlID, bSelect) \
  do \
  { \
    CGUIMessage _msg(bSelect ? GUI_MSG_SET_SELECTED : GUI_MSG_SET_DESELECTED, dwSenderId, \
                     controlID); \
    OnMessage(_msg); \
  } while (0)

/*!
 * \ingroup winmsg
 * \brief Click message sent from controls to windows.
 */
#define SEND_CLICK_MESSAGE(id, parentID, action) \
  do \
  { \
    CGUIMessage _msg(GUI_MSG_CLICKED, id, parentID, action); \
    SendWindowMessage(_msg); \
  } while (0)

#include <memory>
#include <string>
#include <vector>

// forwards
class CGUIListItem;
typedef std::shared_ptr<CGUIListItem> CGUIListItemPtr;
class CFileItemList;

/*!
 * \ingroup winmsg
 */
class CGUIMessage final
{
public:
  CGUIMessage(int dwMsg, int senderID, int controlID, int param1 = 0, int param2 = 0);
  CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, CFileItemList* item);
  CGUIMessage(
      int msg, int senderID, int controlID, int param1, int param2, const CGUIListItemPtr& item);
  CGUIMessage(const CGUIMessage& msg);
  ~CGUIMessage(void);
  CGUIMessage& operator=(const CGUIMessage& msg);

  int GetControlId() const;
  int GetMessage() const;
  void* GetPointer() const;
  CGUIListItemPtr GetItem() const;
  int GetParam1() const;
  int GetParam2() const;
  int GetSenderId() const;
  void SetParam1(int param1);
  void SetParam2(int param2);
  void SetPointer(void* pointer);
  void SetLabel(const std::string& strLabel);
  void SetLabel(int iString); // for convenience - looks up in strings.po
  const std::string& GetLabel() const;
  void SetStringParam(const std::string& strParam);
  void SetStringParams(const std::vector<std::string>& params);
  const std::string& GetStringParam(size_t param = 0) const;
  size_t GetNumStringParams() const;
  void SetItem(CGUIListItemPtr item);

private:
  std::string m_strLabel;
  std::vector<std::string> m_params;
  int m_senderID;
  int m_controlID;
  int m_message;
  void* m_pointer;
  int m_param1;
  int m_param2;
  CGUIListItemPtr m_item;

  static std::string empty_string;
};
