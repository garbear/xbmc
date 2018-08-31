/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/generic/ILanguageInvoker.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

#include <string>
#include <vector>

namespace JAVA
{
class CJavaInvoker : public ILanguageInvoker
{
public:
  explicit CJavaInvoker(ILanguageInvocationHandler *invocationHandler);
  ~CJavaInvoker() override;

  // Implementation of ILanguageInvoker
  bool Execute(const std::string &script, const std::vector<std::string> &arguments = std::vector<std::string>()) override;
  bool IsStopping() const override;

protected:
  // Implementation of ILanguageInvoker
  bool execute(const std::string &script, const std::vector<std::string> &arguments) override;
  bool stop(bool abort) override;
  void onExecutionDone() override;

private:
  void onError(const std::string &exceptionType = "", const std::string &exceptionValue = "", const std::string &exceptionTraceback = "");

  std::string m_sourceFile;
  bool m_bStop = false;

  // Synchronization parameters
  mutable CCriticalSection m_critical;
  CEvent m_stoppedEvent;
};
}
