/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CAdvancedSettings;
class CFileItemList;

class CAppParamParser
{
public:
  CAppParamParser();
  CAppParamParser(const CAppParamParser& other);
  ~CAppParamParser();

  CAppParamParser& operator=(const CAppParamParser& rhs);

  void Parse(const char* const* argv, int nArgs);
  void SetAdvancedSettings(CAdvancedSettings& advancedSettings) const;

  /*!
   * \brief Get an unmodified string of all command line arguments
   */
  const std::vector<std::string>& GetArgs() const { return m_args; }

  const CFileItemList& GetPlaylist() const;

  int m_logLevel;
  bool m_startFullScreen = false;
  bool m_platformDirectories = true;
  bool m_testmode = false;
  bool m_standAlone = false;
  std::string m_windowing;

private:
  void ParseArg(const std::string &arg);
  void DisplayHelp();
  void DisplayVersion();

  std::vector<std::string> m_args;
  std::string m_settingsFile;
  std::unique_ptr<CFileItemList> m_playlist;
};
