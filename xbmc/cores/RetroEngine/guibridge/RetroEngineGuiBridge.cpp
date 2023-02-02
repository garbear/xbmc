/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineGuiBridge.h"

#include "GUIRERenderHandle.h"
#include "GUIRERenderTarget.h"
#include "GUIRERenderTargetFactory.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineGuiBridge::~CRetroEngineGuiBridge() = default;

void CRetroEngineGuiBridge::RegisterRenderer(CGUIRERenderTargetFactory& factory)
{
  // Set factory
  {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    m_factory = &factory;
    UpdateRenderTargets();
  }
}

void CRetroEngineGuiBridge::UnregisterRenderer()
{
  // Reset factory
  {
    std::lock_guard<std::mutex> lock(m_targetMutex);
    m_factory = nullptr;
    UpdateRenderTargets();
  }
}

std::shared_ptr<CGUIRERenderHandle> CRetroEngineGuiBridge::RegisterControl(
    CGUIGameEngineControl& control)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  // Create handle for RetroEngine control
  std::shared_ptr<CGUIRERenderHandle> renderHandle(new CGUIRERenderControlHandle(*this, control));

  std::shared_ptr<CGUIRERenderTarget> renderTarget;
  if (m_factory != nullptr)
    renderTarget.reset(m_factory->CreateRenderControl(control));

  m_renderTargets.insert(std::make_pair(renderHandle.get(), std::move(renderTarget)));

  return renderHandle;
}

void CRetroEngineGuiBridge::UnregisterHandle(CGUIRERenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  m_renderTargets.erase(handle);
}

void CRetroEngineGuiBridge::Render(CGUIRERenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRERenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->Render();
  }
}

void CRetroEngineGuiBridge::ClearBackground(CGUIRERenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRERenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->ClearBackground();
  }
}

bool CRetroEngineGuiBridge::IsDirty(CGUIRERenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRERenderTarget>& renderTarget = it->second;
    if (renderTarget)
      return renderTarget->IsDirty();
  }

  return false;
}

void CRetroEngineGuiBridge::UpdateRenderTargets()
{
  if (m_factory != nullptr)
  {
    for (auto& it : m_renderTargets)
    {
      CGUIRERenderHandle* handle = it.first;
      std::shared_ptr<CGUIRERenderTarget>& renderTarget = it.second;

      if (!renderTarget)
        renderTarget.reset(CreateRenderTarget(handle));
    }
  }
  else
  {
    for (auto& it : m_renderTargets)
      it.second.reset();
  }
}

CGUIRERenderTarget* CRetroEngineGuiBridge::CreateRenderTarget(CGUIRERenderHandle* handle)
{
  CGUIRERenderControlHandle* controlHandle = static_cast<CGUIRERenderControlHandle*>(handle);
  return m_factory->CreateRenderControl(controlHandle->GetControl());
}
