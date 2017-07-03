#pragma once

/*
 *      Copyright (C) 2007-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAS_DX
#include <string>
#include <vector>
#include <tuple>

#include "../../guilib/Geometry.h"
#include "../RPWinRenderer.h"
#include "ShaderFormats.h"
#include "cores/IPlayer.h"
#include "guilib/D3DResource.h"
#include <DirectXMath.h>

using namespace DirectX;

struct YUVBufferRP;

class CRPYUV2RGBMatrix
{
public:
  CRPYUV2RGBMatrix();
  void SetParameters(float contrast, float blacklevel, unsigned int flags, ERPShaderFormat format);
  XMFLOAT4X4* Matrix();

private:
  bool         m_NeedRecalc;
  float        m_contrast;
  float        m_blacklevel;
  unsigned int m_flags;
  bool         m_limitedRange;
  ERPShaderFormat m_format;
  XMFLOAT4X4   m_mat;
  bool         m_isNearest;
};

class CRPWinShader
{
protected:
  CRPWinShader() :
    m_vbsize(0),
    m_vertsize(0),
    m_inputLayout(nullptr)
  {}
  virtual ~CRPWinShader();
  virtual bool CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize);
  virtual bool LockVertexBuffer(void **data);
  virtual bool UnlockVertexBuffer();
  virtual bool LoadEffect(const std::string& filename, DefinesMap* defines);
  virtual bool Execute(std::vector<ID3D11RenderTargetView*> *vecRT, unsigned int vertexIndexStep);
  virtual void SetStepParams(UINT stepIndex) { }
  virtual bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC *layout, unsigned numElements);

  CD3DEffect   m_effect;

protected:
  static const float* getColorRange(bool isLimitedRange)
  {
    static const float fullRange[] = { 0.f, 1.f };
    static const float limitedRange[] = { 16.f / 255.f, (235.f - 16.f) / 255.f };
    static const float *ranges[] = { fullRange, limitedRange };

    return ranges[isLimitedRange];
  }

private:
  CD3DBuffer          m_vb;
  CD3DBuffer          m_ib;
  unsigned int        m_vbsize;
  unsigned int        m_vertsize;
  ID3D11InputLayout*  m_inputLayout;
};

class CRPYUV2RGBShader : public CRPWinShader
{
public:
  virtual bool Create(unsigned int sourceWidth,
    unsigned int sourceHeight,
    ERPShaderFormat fmt,
    ESCALINGMETHOD scalingMethod = VS_SCALINGMETHOD_LINEAR);
  virtual void Render(CRect sourceRect,
    CPoint dest[],
    float contrast,
    float brightness,
    unsigned int flags,
    YUVBufferRP* YUVbuf,
    ESCALINGMETHOD scalingMethod = VS_SCALINGMETHOD_LINEAR);
  CRPYUV2RGBShader() :
    m_sourceWidth(0),
    m_sourceHeight(0),
    m_format(RP_SHADER_NONE),
    m_isNearest(false)
  {
    memset(&m_texSteps, 0, sizeof(m_texSteps));
  }

  virtual ~CRPYUV2RGBShader();

protected:
  virtual void PrepareParameters(CRect sourceRect,
    CPoint dest[],
    float contrast,
    float brightness,
    unsigned int flags);
  virtual void SetShaderParameters(YUVBufferRP* YUVbuf);
  void HandleScalerChange(ESCALINGMETHOD escalingMethod);

private:
  CRPYUV2RGBMatrix      m_matrix;
  unsigned int        m_sourceWidth, m_sourceHeight;
  CRect               m_sourceRect;
  CPoint              m_dest[4];
  ERPShaderFormat       m_format;
  float               m_texSteps[2];
  bool                m_isNearest;

  struct CUSTOMVERTEX {
    FLOAT x, y, z;
    FLOAT tu, tv;   // Y Texture coordinates
    FLOAT tu2, tv2; // U and V Textures coordinates
  };
};

class CRPConvolutionShader : public CRPWinShader
{
public:
  virtual bool Create(ESCALINGMETHOD method) = 0;
  virtual void Render(CD3DTexture &sourceTexture,
    unsigned int sourceWidth, unsigned int sourceHeight,
    unsigned int destWidth, unsigned int destHeight,
    CRect sourceRect,
    CRect destRect,
    bool useLimitRange) = 0;
  CRPConvolutionShader() : CRPWinShader() {}
  virtual ~CRPConvolutionShader();

protected:
  virtual bool ChooseKernelD3DFormat();
  virtual bool CreateHQKernel(ESCALINGMETHOD method);

  CD3DTexture         m_HQKernelTexture;
  DXGI_FORMAT         m_KernelFormat;
  bool                m_floattex;
  bool                m_rgba;

  struct CUSTOMVERTEX {
    FLOAT x, y, z;
    FLOAT tu, tv;
  };
};

class CRPConvolutionShader1Pass : public CRPConvolutionShader
{
public:
  virtual bool Create(ESCALINGMETHOD method);
  virtual void Render(CD3DTexture &sourceTexture,
    unsigned int sourceWidth, unsigned int sourceHeight,
    unsigned int destWidth, unsigned int destHeight,
    CRect sourceRect,
    CRect destRect,
    bool useLimitRange);
  CRPConvolutionShader1Pass() : CRPConvolutionShader(), m_sourceWidth(0), m_sourceHeight(0) {}

protected:
  virtual void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
    CRect sourceRect,
    CRect destRect);
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount, bool useLimitRange);


private:
  unsigned int  m_sourceWidth, m_sourceHeight;
  CRect         m_sourceRect, m_destRect;
};

class CRPConvolutionShaderSeparable : public CRPConvolutionShader
{
public:
  CRPConvolutionShaderSeparable();
  virtual bool Create(ESCALINGMETHOD method);
  virtual void Render(CD3DTexture &sourceTexture,
    unsigned int sourceWidth, unsigned int sourceHeight,
    unsigned int destWidth, unsigned int destHeight,
    CRect sourceRect,
    CRect destRect,
    bool useLimitRange);
  virtual ~CRPConvolutionShaderSeparable();

protected:
  virtual bool ChooseIntermediateD3DFormat();
  virtual bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height);
  virtual bool ClearIntermediateRenderTarget();
  virtual void PrepareParameters(unsigned int sourceWidth, unsigned int sourceHeight,
    unsigned int destWidth, unsigned int destHeight,
    CRect sourceRect,
    CRect destRect);
  virtual void SetShaderParameters(CD3DTexture &sourceTexture, float* texSteps, int texStepsCount, bool useLimitRange);
  virtual void SetStepParams(UINT stepIndex);

private:
  CD3DTexture   m_IntermediateTarget;
  DXGI_FORMAT   m_IntermediateFormat;
  unsigned int  m_sourceWidth, m_sourceHeight;
  unsigned int  m_destWidth, m_destHeight;
  CRect         m_sourceRect, m_destRect;
  ID3D11RenderTargetView* m_oldRenderTarget = nullptr;
};

class CRPTestShader : public CRPWinShader
{
public:
  virtual bool Create();
};

#endif