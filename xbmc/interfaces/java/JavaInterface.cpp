/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JavaInterface.h"
#include "JavaInvoker.h"

using namespace JAVA;

ILanguageInvoker* CJavaInterface::CreateInvoker()
{
  return new CJavaInvoker(this);
}
