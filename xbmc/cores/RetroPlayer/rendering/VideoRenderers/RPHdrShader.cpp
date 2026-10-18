/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPHdrShader.h"

#include "rendering/MatrixGL.h"

#include <algorithm>

using namespace KODI;
using namespace RETRO;

CRPHdrShader::CRPHdrShader(const std::string& prefix)
{
#if defined(HAS_GL)
  VertexShader()->LoadSource("gl_retroplayer_composite.vert", prefix);
  PixelShader()->LoadSource("gl_retroplayer_composite.frag", prefix);
#else
  VertexShader()->LoadSource("gles_retroplayer_composite.vert", prefix);
  PixelShader()->LoadSource("gles_retroplayer_composite.frag", prefix);
#endif
}

void CRPHdrShader::SetLuminance(unsigned int paperWhite, unsigned int peak)
{
  m_paperWhite = paperWhite / 10000.0f;
  m_peakRatio = std::max(static_cast<float>(peak) / std::max(paperWhite, 1u), 1.0f);
}

void CRPHdrShader::SetQuad(const CPoint& origin,
                           const CPoint& right,
                           const CPoint& down,
                           const CRect& texCoords)
{
  m_quad = {right.x - origin.x, right.y - origin.y, 0.0f, 0.0f,
            down.x - origin.x, down.y - origin.y, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            origin.x, origin.y, 0.0f, 1.0f};
  m_texQuad = {texCoords.x2 - texCoords.x1, 0.0f, 0.0f, 0.0f,
               0.0f, texCoords.y2 - texCoords.y1, 0.0f, 0.0f,
               0.0f, 0.0f, 1.0f, 0.0f,
               texCoords.x1, texCoords.y1, 0.0f, 1.0f};
}

void CRPHdrShader::OnCompiledAndLinked()
{
  m_hPos = glGetAttribLocation(ProgramHandle(), "a_pos");
  m_hSamp = glGetUniformLocation(ProgramHandle(), "u_samp");
  m_hMatrix = glGetUniformLocation(ProgramHandle(), "u_matrix");
  m_hQuad = glGetUniformLocation(ProgramHandle(), "u_quad");
  m_hTexQuad = glGetUniformLocation(ProgramHandle(), "u_texQuad");
  m_hPaperWhite = glGetUniformLocation(ProgramHandle(), "u_paperWhite");
  m_hPeakRatio = glGetUniformLocation(ProgramHandle(), "u_peakRatio");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "u_alpha");

  glUseProgram(ProgramHandle());
  glUniform1i(m_hSamp, 0);
  glUseProgram(0);
}

bool CRPHdrShader::OnEnabled()
{
  CMatrixGL matrix = glMatrixProject.Get();
  matrix.MultMatrixf(glMatrixModview.Get());
  glUniformMatrix4fv(m_hMatrix, 1, GL_FALSE, matrix);
  glUniformMatrix4fv(m_hQuad, 1, GL_FALSE, m_quad.data());
  glUniformMatrix4fv(m_hTexQuad, 1, GL_FALSE, m_texQuad.data());
  glUniform1f(m_hPaperWhite, m_paperWhite);
  glUniform1f(m_hPeakRatio, m_peakRatio);
  glUniform1f(m_hAlpha, m_alpha);

  return true;
}
