/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
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
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "RenderSystem.h"

CRenderSystemBase::CRenderSystemBase()
{
  m_bRenderCreated = false;
  m_bVSync = true;
  m_maxTextureSize = 2048;
  m_RenderVerdenVersionMajor = 0;
  m_RenderVerdenVersionMinor = 0;
  m_NeedPower2Texture = false;
}

CRenderSystemBase::~CRenderSystemBase()
{

}

void CRenderSystemBase::GetRenderVersion(unsigned int& major, unsigned int& minor)
{
  major = m_RenderVerdenVersionMajor;
  minor = m_RenderVerdenVersionMinor;
}
