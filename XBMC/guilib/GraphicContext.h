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

/*!
\file GraphicContext.h
\brief
*/

#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even whith optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif


#include <vector>
#include <stack>
#include <map>
#include "utils/CriticalSection.h"  // base class
#include "TransformMatrix.h"        // for the members m_guiTransform etc.
#include "Geometry.h"               // for CRect/CPoint
#include "gui3d.h"
#include "StdString.h"

namespace Surface { class CSurface; }

// forward definitions
class IMsgSenderCallback;
class CGUIMessage;

/*!
 \ingroup graphics
 \brief
 */
enum RESOLUTION {
  INVALID = -1,
  HDTV_1080i = 0,
  HDTV_720p = 1,
  HDTV_480p_4x3 = 2,
  HDTV_480p_16x9 = 3,
  NTSC_4x3 = 4,
  NTSC_16x9 = 5,
  PAL_4x3 = 6,
  PAL_16x9 = 7,
  PAL60_4x3 = 8,
  PAL60_16x9 = 9,
  AUTORES = 10,
  WINDOW = 11,
  DESKTOP = 12,
  CUSTOM = 13
};

enum VSYNC {
  VSYNC_DISABLED = 0,
  VSYNC_VIDEO = 1,
  VSYNC_ALWAYS = 2,
  VSYNC_DRIVER = 3
};

enum VIEW_TYPE { VIEW_TYPE_NONE = 0,
                 VIEW_TYPE_LIST,
                 VIEW_TYPE_ICON,
                 VIEW_TYPE_BIG_LIST,
                 VIEW_TYPE_BIG_ICON,
                 VIEW_TYPE_WIDE,
                 VIEW_TYPE_BIG_WIDE,
                 VIEW_TYPE_WRAP,
                 VIEW_TYPE_BIG_WRAP,
                 VIEW_TYPE_AUTO,
                 VIEW_TYPE_MAX };

/*!
 \ingroup graphics
 \brief
 */
struct OVERSCAN
{
  int left;
  int top;
  int right;
  int bottom;
};

/*!
 \ingroup graphics
 \brief
 */
struct RESOLUTION_INFO
{
  OVERSCAN Overscan;
  int iScreen;
  int iWidth;
  int iHeight;
  int iSubtitles;
  DWORD dwFlags;
  float fPixelRatio;
  float fRefreshRate;
  char strMode[48];
  char strOutput[32];
  char strId[16];
};

/*!
 \ingroup graphics
 \brief
 */
class CGraphicContext : public CCriticalSection
{
public:
  CGraphicContext(void);
  virtual ~CGraphicContext(void);

  std::string  GetRenderVendor();
  std::string  GetRenderRenderer();
  void         GetRenderVersion(int& maj, int& min);

  inline void setScreenSurface(Surface::CSurface* surface) XBMC_FORCE_INLINE { m_screenSurface = surface; }
  inline Surface::CSurface* getScreenSurface() XBMC_FORCE_INLINE { return m_screenSurface; }


  virtual bool ValidateSurface(Surface::CSurface* dest=NULL) = 0;
  virtual Surface::CSurface* InitializeSurface() = 0;
  virtual void ReleaseThreadSurface() = 0;

  // the following two functions should wrap any
  // GL calls to maintain thread safety
  virtual void BeginPaint(Surface::CSurface* dest=NULL, bool lock=true) = 0;
  virtual void EndPaint(Surface::CSurface* dest=NULL, bool lock=true) = 0;
  virtual void ReleaseCurrentContext(Surface::CSurface* dest=NULL) = 0;
  virtual void AcquireCurrentContext(Surface::CSurface* dest=NULL) = 0;
  virtual void DeleteThreadContext();

  int GetWidth() const { return m_iScreenWidth; }
  int GetHeight() const { return m_iScreenHeight; }
  float GetFPS() const;
  bool SendMessage(CGUIMessage& message);
  bool SendMessage(DWORD message, DWORD senderID, DWORD destID, DWORD param1 = 0, DWORD param2 = 0);
  void setMessageSender(IMsgSenderCallback* pCallback);
  DWORD GetNewID();
  const CStdString& GetMediaDir() const { return m_strMediaDir; }
  void SetMediaDir(const CStdString& strMediaDir);
  bool IsWidescreen() const { return m_bWidescreen; }
  bool SetViewPort(float fx, float fy , float fwidth, float fheight, bool intersectPrevious = false);
  void RestoreViewPort();
  const RECT& GetViewWindow() const;
  void SetViewWindow(float left, float top, float right, float bottom);
  void SetFullScreenViewWindow(RESOLUTION &res);
  bool IsFullScreenRoot() const;
  bool ToggleFullScreenRoot();
  void SetFullScreenRoot(bool fs = true);
  void ClipToViewWindow();
  void SetFullScreenVideo(bool bOnOff);
  bool IsFullScreenVideo() const;
  bool IsCalibrating() const;
  void SetCalibrating(bool bOnOff);
  void GetAllowedResolutions(std::vector<RESOLUTION> &res, bool bAllowPAL60 = false);
  bool IsValidResolution(RESOLUTION res);
  void SetVideoResolution(RESOLUTION &res, BOOL NeedZ = FALSE, bool forceClear = false);
  RESOLUTION GetVideoResolution() const;
  void ResetOverscan(RESOLUTION res, OVERSCAN &overscan);
  void ResetOverscan(RESOLUTION_INFO &resinfo);
  void ResetScreenParameters(RESOLUTION res);
  void Lock() { EnterCriticalSection(*this);  }
  void Unlock() { LeaveCriticalSection(*this); }
  float GetPixelRatio(RESOLUTION iRes) const;
  void CaptureStateBlock();
  void ApplyStateBlock();
  void Clear();

  // output scaling
  void SetRenderingResolution(RESOLUTION res, float posX, float posY, bool needsScaling);  ///< Sets scaling up for rendering
  void SetScalingResolution(RESOLUTION res, float posX, float posY, bool needsScaling);    ///< Sets scaling up for skin loading etc.
  float GetScalingPixelRatio() const;
  void Flip();
  void InvertFinalCoords(float &x, float &y) const;
  inline float ScaleFinalXCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformXCoord(x, y, 0); }
  inline float ScaleFinalYCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformYCoord(x, y, 0); }
  inline float ScaleFinalZCoord(float x, float y) const XBMC_FORCE_INLINE { return m_finalTransform.TransformZCoord(x, y, 0); }
  inline void ScaleFinalCoords(float &x, float &y, float &z) const XBMC_FORCE_INLINE { m_finalTransform.TransformPosition(x, y, z); }
  bool RectIsAngled(float x1, float y1, float x2, float y2) const;

  inline float GetGUIScaleX() const XBMC_FORCE_INLINE { return m_guiScaleX; }
  inline float GetGUIScaleY() const XBMC_FORCE_INLINE { return m_guiScaleY; }
  inline DWORD MergeAlpha(DWORD color) const XBMC_FORCE_INLINE
  {
    DWORD alpha = m_finalTransform.TransformAlpha((color >> 24) & 0xff);
    if (alpha > 255) alpha = 255;
    return ((alpha << 24) & 0xff000000) | (color & 0xffffff);
  }

  void SetOrigin(float x, float y);
  void RestoreOrigin();
  void SetCameraPosition(const CPoint &camera);
  void RestoreCameraPosition();
  bool SetClipRegion(float x, float y, float w, float h);
  void RestoreClipRegion();
  void ApplyHardwareTransform();
  void RestoreHardwareTransform();
  void NotifyAppFocusChange(bool bGaining);
  void ClipRect(CRect &vertex, CRect &texture, CRect *diffuse = NULL);
  inline void ResetWindowTransform()
  {
    while (m_groupTransform.size())
      m_groupTransform.pop();
    m_groupTransform.push(m_guiTransform);
  }
  inline void AddTransform(const TransformMatrix &matrix)
  {
    ASSERT(m_groupTransform.size());
    if (m_groupTransform.size())
      m_groupTransform.push(m_groupTransform.top() * matrix);
    else
      m_groupTransform.push(matrix);
    UpdateFinalTransform(m_groupTransform.top());
  }
  inline void RemoveTransform()
  {
    ASSERT(m_groupTransform.size() > 1);
    if (m_groupTransform.size())
      m_groupTransform.pop();
    if (m_groupTransform.size())
      UpdateFinalTransform(m_groupTransform.top());
    else
      UpdateFinalTransform(TransformMatrix());
  }

  int GetMaxTextureSize() const { return m_maxTextureSize; };
protected:
  IMsgSenderCallback* m_pCallback;
  Surface::CSurface* m_screenSurface;

  std::stack<GLint*> m_viewStack;
  std::map<Uint32, Surface::CSurface*> m_surfaces;
  CCriticalSection m_surfaceLock;

  int m_iScreenHeight;
  int m_iScreenWidth;
  int m_iFullScreenHeight;
  int m_iFullScreenWidth;
  int m_iBackBufferCount;
  DWORD m_dwID;
  bool m_bWidescreen;
  CStdString m_strMediaDir;
  RECT m_videoRect;
  bool m_bFullScreenRoot;
  bool m_bFullScreenVideo;
  bool m_bCalibrating;
  RESOLUTION m_Resolution;

private:
  void UpdateCameraPosition(const CPoint &camera);
  void UpdateFinalTransform(const TransformMatrix &matrix);
  RESOLUTION m_windowResolution;
  float m_guiScaleX;
  float m_guiScaleY;
  std::stack<CPoint> m_cameras;
  std::stack<CPoint> m_origins;
  std::stack<CRect>  m_clipRegions;

  std::string s_RenderVendor;
  std::string s_RenderRenderer;
  std::string s_RenderxExt;
  int s_RenderMajVer;
  int s_RenderMinVer;

  TransformMatrix m_guiTransform;
  TransformMatrix m_finalTransform;
  std::stack<TransformMatrix> m_groupTransform;

  int   m_maxTextureSize;
};

/*!
 \ingroup graphics
 \brief
 */

#endif
