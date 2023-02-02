/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroEngineGuiBridge.h"

#include "GUIRenderHandle.h"
#include "GUIRenderTarget.h"
#include "GUIRenderTargetFactory.h"

using namespace KODI;
using namespace RETRO_ENGINE;

CRetroEngineGuiBridge::~CRetroEngineGuiBridge() = default;

void CRetroEngineGuiBridge::RegisterRenderer(CGUIRenderTargetFactory& factory)
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

std::shared_ptr<CGUIRenderHandle> CRetroEngineGuiBridge::RegisterControl(
    CGUIGameEngineControl& control)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  // Create handle for camera view control
  std::shared_ptr<CGUIRenderHandle> renderHandle(new CGUIRenderControlHandle(*this, control));

  std::shared_ptr<CGUIRenderTarget> renderTarget;
  if (m_factory != nullptr)
    renderTarget.reset(m_factory->CreateRenderControl(control));

  m_renderTargets.insert(std::make_pair(renderHandle.get(), std::move(renderTarget)));

  return renderHandle;
}

void CRetroEngineGuiBridge::UnregisterHandle(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  m_renderTargets.erase(handle);
}

void CRetroEngineGuiBridge::Render(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->Render();
  }
}

void CRetroEngineGuiBridge::ClearBackground(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->ClearBackground();
  }
}

bool CRetroEngineGuiBridge::IsDirty(CGUIRenderHandle* handle)
{
  std::lock_guard<std::mutex> lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
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
      CGUIRenderHandle* handle = it.first;
      std::shared_ptr<CGUIRenderTarget>& renderTarget = it.second;

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

CGUIRenderTarget* CRetroEngineGuiBridge::CreateRenderTarget(CGUIRenderHandle* handle)
{
  CGUIRenderControlHandle* controlHandle = static_cast<CGUIRenderControlHandle*>(handle);
  return m_factory->CreateRenderControl(controlHandle->GetControl());
}
