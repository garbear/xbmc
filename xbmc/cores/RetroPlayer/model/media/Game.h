/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class CAchievement;
  class CAnnotation;
  class CLanguageString;
  class CLeaderBoard;
  class CMediaStream;
  class CSummary;

  class CGame
  {
  public:
    CGame() = default;

    std::unique_ptr<CLanguageString> title;

    std::string author;

    uint64_t created = 0; // Unix time

    uint64_t released = 0; // Unix time

    std::shared_ptr<CLanguageString> description;

    std::vector<std::string> platforms;

    std::vector<std::string> genres;

    std::string developer;

    std::string publisher;

    std::string rights;

    std::string discussion;

    std::vector<std::shared_ptr<CAchievement>> achievements;

    std::vector<std::shared_ptr<CLeaderBoard>> leaderBoards;

    std::shared_ptr<CSummary> summary;

    std::vector<std::shared_ptr<CAnnotation>> annotations;

    std::vector<std::shared_ptr<CMediaStream>> mediaStreams;
  };
}
}
