/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Summary.h"

using namespace KODI;
using namespace RETRO;

Summary CSummaryEngine::LoadSummary(const CVariant &summaryData)
{
  Summary summary;


}


std::string CSummaryEngine::GetSummary(const Summary &summary, const uint8_t* state, size_t stateSize)
{
  std::string ret;

  for (const Phrase &phrase : summary.phrases)
  {
    ret = GetPhrase(phrase.template_, summary.concepts);
    if (!ret.empty())
      break;
  }

  return ret;
}

std::string CSummaryEngine::GetPhrase(const Template &template_, const std::vector<Concept> &concepts)
{
  std::string phrase;

  std::string lookupName = template_.lookupName;

  auto it = std::find_if(concepts.begin(), concepts.end(),
    [&lookupName](const Concept &concept)
    {
      return concept.symbol == lookupName
    });

  if (it != concepts.end())
  {
    const Concept &concept = *it;

    if (static)
  }

}

std::string InstantiateUnits(const ConceptPtr &value, UnitType units, const uint8_t* state);
