#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once
#include "gui3d.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"

#include "stdstring.h"
using namespace std;

class CGraphicContext
{
public:
  CGraphicContext(void);
  ~CGraphicContext(void);
  LPDIRECT3DDEVICE8			Get3DDevice();
  void									Set(LPDIRECT3DDEVICE8 p3dDevice, int iWidth, int iHeight, bool bWideScreen=false);
  int										GetWidth() const;
  int										GetHeight() const;
  void									SendMessage(CGUIMessage& message);
  void									setMessageSender(IMsgSenderCallback* pCallback);
  DWORD									GetNewID();
  const CStdString&     GetMediaDir() const;
  void									SetMediaDir(const CStdString& strMediaDir);
	bool									IsWidescreen() const;
	VOID									Correct(FLOAT& fCoordinateX, FLOAT& fCoordinateY, FLOAT& fCoordinateX2, FLOAT& fCoordinateY2) const;
	void									SetViewPort(float fx, float fy , float fwidth, float fheight);
	void									RestoreViewPort();
protected:
  IMsgSenderCallback*     m_pCallback;
  LPDIRECT3DDEVICE8       m_pd3dDevice;
  int                     m_iScreenHeight;
  int                     m_iScreenWidth;
  DWORD                   m_dwID;
	bool										m_bWidescreen;
  CStdString              m_strMediaDir;
	D3DVIEWPORT8						m_oldviewport;
};

extern CGraphicContext g_graphicsContext;
#endif