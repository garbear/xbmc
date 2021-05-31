/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppParamParser.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#if defined(TARGET_LINUX)
namespace
{
std::vector<std::string> availableWindowSystems = CCompileInfo::GetAvailableWindowSystems();
} // namespace
#endif

CAppParamParser::CAppParamParser()
: m_logLevel(LOG_LEVEL_NORMAL),
  m_playlist(new CFileItemList())
{
}

CAppParamParser::CAppParamParser(const CAppParamParser& other)
{
  *this = other;
}

CAppParamParser::~CAppParamParser() = default;

CAppParamParser& CAppParamParser::operator=(const CAppParamParser& rhs)
{
  if (this != &rhs)
  {
    m_logLevel = rhs.m_logLevel;
    m_startFullScreen = rhs.m_startFullScreen;
    m_platformDirectories = rhs.m_platformDirectories;
    m_testmode = rhs.m_testmode;
    m_standAlone = rhs.m_standAlone;
    m_windowing = rhs.m_windowing;
    m_args = rhs.m_args;
    m_settingsFile = rhs.m_settingsFile;
    m_playlist = std::make_unique<CFileItemList>();
    for (int i = 0; i < rhs.m_playlist->Size(); i++)
      m_playlist->Add(rhs.m_playlist->Get(i));
  }

  return *this;
}
void CAppParamParser::Parse(const char* const* argv, int nArgs)
{
  m_args.reserve(nArgs);

  for (int i = 0; i < nArgs; i++)
  {
    m_args.emplace_back(argv[i]);
    if (i > 0)
      ParseArg(argv[i]);
  }

  if (nArgs > 1)
  {
    // testmode is only valid if at least one item to play was given
    if (m_playlist->IsEmpty())
      m_testmode = false;
  }
}

void CAppParamParser::DisplayVersion()
{
  printf("%s Media Center %s\n", CSysInfo::GetVersion().c_str(), CSysInfo::GetAppName().c_str());
  printf("Copyright (C) %s Team %s - http://kodi.tv\n",
         CCompileInfo::GetCopyrightYears(), CSysInfo::GetAppName().c_str());
  exit(0);
}

void CAppParamParser::DisplayHelp()
{
  std::string lcAppName = CSysInfo::GetAppName();
  StringUtils::ToLower(lcAppName);
  printf("Usage: %s [OPTION]... [FILE]...\n\n", lcAppName.c_str());
  printf("Arguments:\n");
  printf("  -fs\t\t\tRuns %s in full screen\n", CSysInfo::GetAppName().c_str());
  printf("  --standalone\t\t%s runs in a stand alone environment without a window \n", CSysInfo::GetAppName().c_str());
  printf("\t\t\tmanager and supporting applications. For example, that\n");
  printf("\t\t\tenables network settings.\n");
  printf("  -p or --portable\t%s will look for configurations in install folder instead of ~/.%s\n", CSysInfo::GetAppName().c_str(), lcAppName.c_str());
  printf("  --debug\t\tEnable debug logging\n");
  printf("  --version\t\tPrint version information\n");
  printf("  --test\t\tEnable test mode. [FILE] required.\n");
  printf("  --settings=<filename>\t\tLoads specified file after advancedsettings.xml replacing any settings specified\n");
  printf("  \t\t\t\tspecified file must exist in special://xbmc/system/\n");
#if defined(TARGET_LINUX)
  printf("  --windowing=<system>\tSelect which windowing method to use.\n");
  printf("  \t\t\t\tAvailable window systems are:");
  for (const auto& windowSystem : availableWindowSystems)
    printf(" %s", windowSystem.c_str());
  printf("\n");
#endif
  exit(0);
}

void CAppParamParser::ParseArg(const std::string &arg)
{
  if (arg == "-fs" || arg == "--fullscreen")
    m_startFullScreen = true;
  else if (arg == "-h" || arg == "--help")
    DisplayHelp();
  else if (arg == "-v" || arg == "--version")
    DisplayVersion();
  else if (arg == "--standalone")
    m_standAlone = true;
  else if (arg == "-p" || arg  == "--portable")
    m_platformDirectories = false;
  else if (arg == "--debug")
    m_logLevel = LOG_LEVEL_DEBUG;
  else if (arg == "--test")
    m_testmode = true;
  else if (arg.substr(0, 11) == "--settings=")
    m_settingsFile = arg.substr(11);
#if defined(TARGET_LINUX)
  else if (arg.substr(0, 12) == "--windowing=")
  {
    if (std::find(availableWindowSystems.begin(), availableWindowSystems.end(), arg.substr(12)) !=
        availableWindowSystems.end())
      m_windowing = arg.substr(12);
    else
    {
      std::cout << "Selected window system not available: " << arg << std::endl;
      std::cout << "    Available window systems:";
      for (const auto& windowSystem : availableWindowSystems)
        std::cout << " " << windowSystem;
      std::cout << std::endl;
      exit(0);
    }
  }
#endif
  else if (arg.length() != 0 && arg[0] != '-')
  {
    const CFileItemPtr item = std::make_shared<CFileItem>(arg);
    item->SetPath(arg);
    m_playlist->Add(item);
  }
}

void CAppParamParser::SetAdvancedSettings(CAdvancedSettings& advancedSettings) const
{
  if (m_logLevel == LOG_LEVEL_DEBUG)
  {
    advancedSettings.m_logLevel = LOG_LEVEL_DEBUG;
    advancedSettings.m_logLevelHint = LOG_LEVEL_DEBUG;
    CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_DEBUG);
  }

  if (!m_settingsFile.empty())
    advancedSettings.AddSettingsFile(m_settingsFile);

  if (m_startFullScreen)
    advancedSettings.m_startFullScreen = true;

  if (m_standAlone)
    advancedSettings.m_handleMounting = true;
}

const CFileItemList& CAppParamParser::GetPlaylist() const
{
  return *m_playlist;
}
