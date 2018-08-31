/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JavaInvoker.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <jni.h>

// Time before Java programs are terminated
#define JAVA_TIMEOUT_MS  1000

using namespace JAVA;

CJavaInvoker::CJavaInvoker(ILanguageInvocationHandler *invocationHandler) :
  ILanguageInvoker(invocationHandler)
{
}

CJavaInvoker::~CJavaInvoker()
{
  // Nothing to do for the default invoker used for registration with the
  // CScriptInvocationManager
  if (GetId() < 0)
    return;

  Stop(true);
  pulseGlobalEvent();

  onExecutionFinalized();
}

bool CJavaInvoker::Execute(const std::string &script, const std::vector<std::string> &arguments /* = std::vector<std::string>() */)
{
  if (script.empty())
    return false;

  if (!XFILE::CFile::Exists(script))
  {
    CLog::Log(LOGERROR, "CJavaInvoker(%d): File \"%s\" does not exist", GetId(), CSpecialProtocol::TranslatePath(script).c_str());
    return false;
  }

  if (!onExecutionInitialized())
    return false;

  return ILanguageInvoker::Execute(script, arguments);
}

bool CJavaInvoker::IsStopping() const
{
  bool bStopping;
  {
    CSingleLock lock(m_critical);
    bStopping = m_bStop;
  }

  return bStopping || ILanguageInvoker::IsStopping();
}

bool CJavaInvoker::execute(const std::string &script, const std::vector<std::string> &arguments)
{
  m_sourceFile = script;

  bool bStopping;
  {
    CSingleLock lock(m_critical);
    bStopping = m_bStop;
  }

  if (!bStopping)
  {
    CLog::Log(LOGDEBUG, "CJavaInvoker(%d, %s): Failed to execute script: JVM is stopped", GetId(), m_sourceFile.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG, "CJavaInvoker(%d, %s): Start processing", GetId(), m_sourceFile.c_str());

  /*
  std::string realFilename(CSpecialProtocol::TranslatePath(m_sourceFile));
  std::string sourceDir = URIUtils::GetDirectory(realFilename);
  URIUtils::RemoveSlashAtEnd(sourceDir);

  CLog::Log(LOGDEBUG, "CJavaInvoker(%d, %s): Entering source directory %s", GetId(), m_sourceFile.c_str(), sourceDir.c_str());
  */

  JavaVMInitArgs vm_args{};

  JavaVMOption option{};
  option.optionString = const_cast<char*>("-Djava.class.path=/usr/lib/java");

#if defined(JNI_VERSION_10)
  vm_args.version = JNI_VERSION_10;
#else
#error "Java 10 not found. Edit source to use Java 8"
  vm_args.version = JNI_VERSION_1_8;
#endif
  vm_args.nOptions = 1;
  vm_args.options = &option;
  vm_args.ignoreUnrecognized = false;

  JavaVM *jvm;
  JNIEnv *env;
  if (JNI_CreateJavaVM(&jvm, reinterpret_cast<void**>(&env), &vm_args) == JNI_OK)
  {
    jclass cls = env->FindClass("Main");
    jmethodID mid = env->GetStaticMethodID(cls, "test", "(I)V");
    env->CallStaticVoidMethod(cls, mid, 100);

    jvm->DestroyJavaVM();
  }
  else
  {
    CLog::Log(LOGERROR, "CJavaInvoker(%d): Failed to create JVM", GetId());
    return false;
  }

  return true;
}

bool CJavaInvoker::stop(bool abort)
{
  CSingleLock lock(m_critical);
  m_bStop = true;

  if (!IsRunning())
    return false;

  XbmcThreads::EndTime timeout(JAVA_TIMEOUT_MS);

  while (!m_stoppedEvent.WaitMSec(15))
  {
    if (timeout.IsTimePast())
      CLog::Log(LOGERROR, "CJavaInvoker(%d, %s): Waited %d seconds to terminate JVM - let's kill it", GetId(), m_sourceFile.c_str(), JAVA_TIMEOUT_MS / 1000);

    break;
  }

  if (!timeout.IsTimePast())
    CLog::Log(LOGDEBUG, "CJavaInvoker(%d, %s): JVM termination took %dms", GetId(), m_sourceFile.c_str(), JAVA_TIMEOUT_MS - timeout.MillisLeft());

  return true;
}

void CJavaInvoker::onExecutionDone()
{
  m_stoppedEvent.Set();

  ILanguageInvoker::onExecutionDone();
}

void CJavaInvoker::onError(const std::string &exceptionType /* = "" */, const std::string &exceptionValue /* = "" */, const std::string &exceptionTraceback /* = "" */)
{
  /* TODO
  CSingleLock gc(CServiceBroker::GetWinSystem()->GetGfxContext());

  CGUIDialogKaiToast *pDlgToast = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKaiToast>(WINDOW_DIALOG_KAI_TOAST);
  if (pDlgToast != NULL)
  {
    std::string message;
    if (m_addon && !m_addon->Name().empty())
      message = StringUtils::Format(g_localizeStrings.Get(2102).c_str(), m_addon->Name().c_str());
    else if (m_sourceFile == CSpecialProtocol::TranslatePath("special://profile/autoexec.py"))
      message = StringUtils::Format(g_localizeStrings.Get(2102).c_str(), "autoexec.py");
    else
       message = g_localizeStrings.Get(2103);
    pDlgToast->QueueNotification(CGUIDialogKaiToast::Error, message, g_localizeStrings.Get(2104));
  }
  */
}
