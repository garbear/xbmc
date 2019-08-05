/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "achievements/Achievement.h"
#include "achievements/Condition.h"
#include "achievements/Constant.h"
#include "achievements/Expression.h"
#include "achievements/LeaderBoard.h"
#include "achievements/Term.h"
#include "achievements/Variable.h"
#include "file/File.h"
#include "media/Game.h"
#include "summarization/Annotation.h"
#include "summarization/Concept.h"
#include "summarization/Instance.h"
#include "summarization/LanguageString.h"
#include "summarization/Molecule.h"
#include "summarization/Phrase.h"
#include "summarization/Summary.h"

using namespace KODI;
using namespace RETRO;

void GetConcepts(std::vector<std::shared_ptr<CConcept>>& concepts)
{
  concepts.emplace_back(new CDictionaryConcept);
}

void GetMolecules(std::vector<std::shared_ptr<CMolecule>>& molecules)
{
  molecules.emplace_back(new CMolecule);
}

void GetPhraseTemplate(std::shared_ptr<CLanguageString>& phraseTemplate)
{
  phraseTemplate.reset(new CLanguageString);
  phraseTemplate->language = "en";
  phraseTemplate->string = "Level @Money(0) Track @Money(1) | @Gear(2) engine; @Gear(3) shocks; @Gear(4) tires; @Gear(5) Armor | @Money(6) Mine(s); @Money(7) Nitrous Oxide Cannister(s); @Money(8) Gallon(s) of Oil | @Money(9)@Money(10).@Money(11)@Money(12)@Money(13).@Money(14)@Money(15)@Money(16)$";
}

void GetPhrases(std::vector<std::shared_ptr<CPhrase>>& phrases)
{
  std::shared_ptr<CPhrase> phrase(new CPhrase);

  //phrase->condition.reset(new CCondition);
  GetMolecules(phrase->molecules);
  GetPhraseTemplate(phrase->phraseTemplate);
}

void Test()
{
  std::shared_ptr<CSummary> summary(new CSummary);

  GetConcepts(summary->concepts);
  summary->license = "CC-BY-NC-SA-4.0";
  GetPhrases(summary->phrases);
}
