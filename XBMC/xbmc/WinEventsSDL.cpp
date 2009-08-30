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
#include "stdafx.h"
#include "WinEvents.h"
#include "Application.h"
#include "XBMC_vkeys.h"
#ifdef HAS_SDL_JOYSTICK
#include "common/SDLJoystick.h"
#endif

#ifdef HAS_SDL

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

bool CWinEventsSDL::MessagePump()
{ 
  SDL_Event event;
  
  while (SDL_PollEvent(&event))
  {
    switch(event.type)
    {    
    case SDL_QUIT:
      if (!g_application.m_bStop) g_application.getApplicationMessenger().Quit();
      break;
      
#ifdef HAS_SDL_JOYSTICK
    case SDL_JOYBUTTONUP:
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYAXISMOTION:
    case SDL_JOYBALLMOTION:
    case SDL_JOYHATMOTION:
      g_Joystick.Update(event);
      return true;
      break;
#endif
      
    case SDL_ACTIVEEVENT:
      //If the window was inconified or restored
      if( event.active.state & SDL_APPACTIVE )
      {
        g_application.m_AppActive = event.active.gain != 0;
        if (g_application.m_AppActive) g_application.Minimize(false);
      }
      else if (event.active.state & SDL_APPINPUTFOCUS)
      {
        g_application.m_AppFocused = event.active.gain != 0;
        g_graphicsContext.NotifyAppFocusChange(g_application.m_AppFocused);
      }
      break;
      
    case SDL_KEYDOWN:
    {
      // process any platform specific shortcuts before handing off to XBMC
#ifdef __APPLE__      
      if (ProcessOSXShortcuts(event))
      {
        return true;
      }
#endif      
      
      XBMC_Event newEvent;
      newEvent.type = XBMC_KEYDOWN;
      newEvent.key.keysym.scancode = event.key.keysym.scancode;
      newEvent.key.keysym.sym = (XBMCKey) event.key.keysym.sym;
      newEvent.key.keysym.mod =(XBMCMod) event.key.keysym.mod;
      newEvent.key.keysym.unicode = event.key.keysym.unicode;
      newEvent.key.state = event.key.state;
      newEvent.key.type = event.key.type;
      newEvent.key.which = event.key.which;

      // don't handle any more messages in the queue until we've handled keydown,
      // if a keyup is in the queue it will reset the keypress before it is handled.
      return g_application.OnEvent(newEvent);
    }
      
    case SDL_KEYUP:
    {
      XBMC_Event newEvent;
      newEvent.type = XBMC_KEYUP;
      newEvent.key.keysym.scancode = event.key.keysym.scancode;
      newEvent.key.keysym.sym = (XBMCKey) event.key.keysym.sym;
      newEvent.key.keysym.mod =(XBMCMod) event.key.keysym.mod;
      newEvent.key.keysym.unicode = event.key.keysym.unicode;
      newEvent.key.state = event.key.state;
      newEvent.key.type = event.key.type;
      newEvent.key.which = event.key.which;
      
      return g_application.OnEvent(newEvent);
    }
    
    case SDL_MOUSEBUTTONDOWN:
    {
      XBMC_Event newEvent;
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      newEvent.button.button = event.button.button;
      newEvent.button.state = event.button.state;
      newEvent.button.type = event.button.type;
      newEvent.button.which = event.button.which;
      newEvent.button.x = event.button.x;
      newEvent.button.y = event.button.y;

      return g_application.OnEvent(newEvent);
    }

    case SDL_MOUSEBUTTONUP:
    {
      XBMC_Event newEvent;
      newEvent.type = XBMC_MOUSEBUTTONUP;
      newEvent.button.button = event.button.button;
      newEvent.button.state = event.button.state;
      newEvent.button.type = event.button.type;
      newEvent.button.which = event.button.which;
      newEvent.button.x = event.button.x;
      newEvent.button.y = event.button.y;

      return g_application.OnEvent(newEvent);
    }

    case SDL_MOUSEMOTION:
    {
      XBMC_Event newEvent;
      newEvent.type = XBMC_MOUSEMOTION;
      newEvent.motion.xrel = event.motion.xrel;
      newEvent.motion.yrel = event.motion.yrel;
      newEvent.motion.state = event.motion.state;
      newEvent.motion.type = event.motion.type;
      newEvent.motion.which = event.motion.which;
      newEvent.motion.x = event.motion.x;
      newEvent.motion.y = event.motion.y;

      return g_application.OnEvent(newEvent);
    }
    
    }
  }
  
  return false;
}

#ifdef __APPLE__
bool CWinEventsSDL::ProcessOSXShortcuts(SDL_Event& event)
{
  static bool shift = false, cmd = false;
  static CAction action;

  cmd   = !!(SDL_GetModState() & (KMOD_LMETA  | KMOD_RMETA ));
  shift = !!(SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT));

  if (cmd && event.key.type == SDL_KEYDOWN)
  {
    switch(event.key.keysym.sym)
    {
    case SDLK_q:  // CMD-q to quit
    case SDLK_w:  // CMD-w to quit
      if (!g_application.m_bStop)
        g_application.getApplicationMessenger().Quit();
      return true;

    case SDLK_f: // CMD-f to toggle fullscreen
      action.wID = ACTION_TOGGLE_FULLSCREEN;
      g_application.OnAction(action);
      return true;

    case SDLK_s: // CMD-3 to take a screenshot
      action.wID = ACTION_TAKE_SCREENSHOT;
      g_application.OnAction(action);
      return true;

    case SDLK_h: // CMD-h to hide (but we minimize for now)
    case SDLK_m: // CMD-m to minimize
      g_application.getApplicationMessenger().Minimize();
      return true;

    default:
      return false;
    }
  }
  
  return false;  
}

#elif defined(_LINUX)

bool CWinEventsSDL::ProcessLinuxShortcuts(SDL_Event& event)
{
  bool alt = false;

  alt = !!(SDL_GetModState() & (XBMCKMOD_LALT  | XBMCKMOD_RALT));

  if (alt && event.key.type == SDL_KEYDOWN)
  {
    switch(event.key.keysym.sym)
    {
      case SDLK_TAB:  // ALT+TAB to minimize/hide
        g_application.Minimize();
        return true;
      default:
        break;
    }
  }
  return false;
}
#endif

#endif
