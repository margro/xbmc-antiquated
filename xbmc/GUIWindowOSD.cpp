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

#include "GUIWindowOSD.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "MouseStat.h"

CGUIWindowOSD::CGUIWindowOSD(void)
    : CGUIDialog(WINDOW_OSD, "VideoOSD.xml")
{
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{
}

void CGUIWindowOSD::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_bRelativeCoords = true;
}

void CGUIWindowOSD::FrameMove()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.IsActive() || g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CHANNELS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_GUIDE)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_DIRECTOR)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CUTTER)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_OSD_TELETEXT))
      SetAutoClose(100); // enough for 10fps
  }
  CGUIDialog::FrameMove();
}

bool CGUIWindowOSD::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_NEXT_ITEM || action.GetID() == ACTION_PREV_ITEM)
  {
    // these could indicate next chapter if video supports it
    if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action))
      return true;
  }

  return CGUIDialog::OnAction(action);
}

EVENT_RESULT CGUIWindowOSD::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_FORWARD, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_BACK, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }

  return CGUIDialog::OnMouseEvent(point, event);
}

bool CGUIWindowOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_VIDEO_MENU_STARTED:
    {
      // We have gone to the DVD menu, so close the OSD.
      Close();
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning())
        pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_GUIDE);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_DIRECTOR);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_CUTTER);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_OSD_TELETEXT);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

