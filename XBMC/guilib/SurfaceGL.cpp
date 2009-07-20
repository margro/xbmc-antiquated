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
\file Surface.cpp
\brief
*/
#include "include.h"
#include "SurfaceGL.h"
#ifdef __APPLE__
#include "CocoaInterface.h"
#endif
#include <string>
#include "Settings.h"

#ifdef _WIN32
#include "VideoReferenceClock.h"
#endif

using namespace Surface;

#include <SDL/SDL_syswm.h>
#include "XBVideoConfig.h"

bool CSurfaceGL::b_glewInit = 0;

#ifdef HAS_GLX
Display* CSurface::s_dpy = 0;

static Bool WaitForNotify(Display *dpy, XEvent *event, XPointer arg)
{
  return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
}
static Bool    (*_glXGetSyncValuesOML)(Display* dpy, GLXDrawable drawable, int64_t* ust, int64_t* msc, int64_t* sbc);
static int64_t (*_glXSwapBuffersMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor,int64_t remainder);

#endif

#include "GraphicContext.h"

static __int64 abs(__int64 a)
{
  if(a < 0) return -a;
  else      return  a;
}

#ifdef _WIN32
static BOOL (APIENTRY *_wglSwapIntervalEXT)(GLint) = 0;
static int (APIENTRY *_wglGetSwapIntervalEXT)() = 0;
#elif !defined(__APPLE__)
static int (*_glXGetVideoSyncSGI)(unsigned int*) = 0;
static int (*_glXWaitVideoSyncSGI)(int, int, unsigned int*) = 0;
static int (*_glXSwapIntervalSGI)(int) = 0;
static int (*_glXSwapIntervalMESA)(int) = 0;
#endif

CSurfaceGL::CSurfaceGL(CSurface* src) : CSurface(src)
{
  *this = *src;
  m_pShared = src;
}


CSurfaceGL::CSurfaceGL(int width, int height, bool doublebuffer, CSurface* shared,
                       CSurface* window, XBMC::SurfacePtr parent, bool fullscreen,
                   bool pixmap, bool pbuffer, int antialias)
                   : CSurface(width, height, doublebuffer, shared, window, 
                   parent, fullscreen, pixmap, pbuffer, antialias)
{
#ifdef __APPLE__
  m_glContext = 0;
#endif

#ifdef _WIN32
  m_glDC = NULL;
  m_glContext = NULL;
#endif
  s_glxExt ="";
#ifdef HAS_GLX
  m_glWindow = 0;
  m_parentWindow = 0;
  m_glContext = 0;
  m_glPBuffer = 0;

  GLXFBConfig *fbConfigs = 0;
  bool mapWindow = false;
  XVisualInfo *vInfo = NULL;
  int num = 0;

  int singleVisAttributes[] =
  {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, m_iRedSize,
    GLX_GREEN_SIZE, m_iGreenSize,
    GLX_BLUE_SIZE, m_iBlueSize,
    GLX_ALPHA_SIZE, m_iAlphaSize,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    None
  };

  int doubleVisAttributes[] =
  {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, m_iRedSize,
    GLX_GREEN_SIZE, m_iGreenSize,
    GLX_BLUE_SIZE, m_iBlueSize,
    GLX_ALPHA_SIZE, m_iAlphaSize,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_DOUBLEBUFFER, True,
    None
  };

  int doubleVisAttributesAA[] =
  {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_RED_SIZE, m_iRedSize,
    GLX_GREEN_SIZE, m_iGreenSize,
    GLX_BLUE_SIZE, m_iBlueSize,
    GLX_ALPHA_SIZE, m_iAlphaSize,
    GLX_DEPTH_SIZE, 8,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_DOUBLEBUFFER, True,
    GLX_SAMPLE_BUFFERS, 1,
    None
  };

  // are we using an existing window to create the context?
  if (window)
  {
    // obtain the GLX window associated with the CSurface object
    m_glWindow = window->GetWindow();
    CLog::Log(LOGINFO, "GLX Info: Using destination window");
  } else {
    m_glWindow = 0;
    CLog::Log(LOGINFO, "GLX Info: NOT Using destination window");
  }

  if (!s_dpy)
  {
    // try to open root display
    s_dpy = XOpenDisplay(0);
    if (!s_dpy)
    {
      CLog::Log(LOGERROR, "GLX Error: No Display found");
      return;
    }
  }

  if (pbuffer)
  {
    MakePBuffer();
    return;
  }

  if (doublebuffer)
  {
    if (antialias)
    {
      // query compatible framebuffers based on double buffered AA attributes
      fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributesAA, &num);
      if (!fbConfigs)
      {
        CLog::Log(LOGWARNING, "GLX: No Multisample buffers available, FSAA disabled");
        fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
      }
    }
    else
    {
      // query compatible framebuffers based on double buffered attributes
      fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
    }
  }
  else
  {
    // query compatible framebuffers based on single buffered attributes (not used currently)
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);
  }
  if (fbConfigs==NULL)
  {
    CLog::Log(LOGERROR, "GLX Error: No compatible framebuffers found");
    return;
  }

  for (int i=0;i<num;i++)
  {
    // obtain the xvisual from the first compatible framebuffer
    vInfo = glXGetVisualFromFBConfig(s_dpy, fbConfigs[i]);
    if (vInfo->depth == 24)
    {
      CLog::Log(LOGNOTICE,"Using fbConfig[%i]",i);
      break;
    }
  }

  if (!vInfo) {
    CLog::Log(LOGERROR, "GLX Error: vInfo is NULL!");
    return;
  }

  // if no window is specified, create a window because a GL context needs to be
  // associated to a window
  if (!m_glWindow)
  {
    XSetWindowAttributes  swa;
    int swaMask;
    Window p;

    m_Surface = parent;

    // check if a parent was passed
    if (parent)
    {
      SDL_SysWMinfo info;

      SDL_VERSION(&info.version);
      SDL_GetWMInfo(&info);
      m_parentWindow = p = info.info.x11.window;
      swaMask = 0;
      CLog::Log(LOGINFO, "GLX Info: Using parent window");
    }
    else
    {
      // create a window with the desktop as the parent
      p = RootWindow(s_dpy, vInfo->screen);
      swaMask = CWBorderPixel;
    }
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    swa.colormap = XCreateColormap(s_dpy, p, vInfo->visual, AllocNone);
    swaMask |= (CWColormap | CWEventMask);
    m_glWindow = XCreateWindow(s_dpy, p, 0, 0, m_iWidth, m_iHeight,
      0, vInfo->depth, InputOutput, vInfo->visual,
      swaMask, &swa );
    XSync(s_dpy, False);
    mapWindow = true;

    // success?
    if (!m_glWindow)
    {
      XFree(fbConfigs);
      CLog::Log(LOGERROR, "GLX Error: Could not create window");
      return;
    }
  }

  // now create the actual context
  if (shared)
  {
    CLog::Log(LOGINFO, "GLX Info: Creating shared context");
    m_glContext = glXCreateContext(s_dpy, vInfo, shared->GetContext(), True);
  } else {
    CLog::Log(LOGINFO, "GLX Info: Creating unshared context");
    m_glContext = glXCreateContext(s_dpy, vInfo, NULL, True);
  }
  XFree(fbConfigs);
  XFree(vInfo);

  // map the window and wait for notification that it was successful (X-specific)
  if (mapWindow)
  {
    XEvent event;
    XMapWindow(s_dpy, m_glWindow);
    XIfEvent(s_dpy, &event, WaitForNotify, (XPointer)m_glWindow);
  }

  // success?
  if (m_glContext)
  {
    // make this context current
    glXMakeCurrent(s_dpy, m_glWindow, m_glContext);

    // initialize glew only once (b_glewInit is static)
    if (!b_glewInit)
    {
      if (glewInit()!=GLEW_OK)
      {
        CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
      }
      else
      {
        b_glewInit = true;
      }
    }
    m_bOK = true;
  }
  else
  {
    CLog::Log(LOGERROR, "GLX Error: Could not create context");
  }
#elif defined(HAS_SDL_OPENGL)
#ifdef __APPLE__
  // We only want to call SDL_SetVideoMode if it's not shared, otherwise we'll create a new window.
  if (shared == 0)
  {
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   m_iRedSize);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, m_iGreenSize);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  m_iBlueSize);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, m_iAlphaSize);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, m_bDoublebuffer ? 1:0);
#ifdef __APPLE__
    // Enable vertical sync to avoid any tearing.
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    // We always want to use a shared context as we jump around in resolution,
    // otherwise we lose all our textures. However, contexts do not share correctly
    // between fullscreen and non-fullscreen.
    //
    shared = g_graphicsContext.getScreenSurface();

    // If we're coming from or going to fullscreen do NOT share.
    if (g_graphicsContext.getScreenSurface() != 0 &&
      (fullscreen == false && g_graphicsContext.getScreenSurface()->m_bFullscreen == true ||
      fullscreen == true  && g_graphicsContext.getScreenSurface()->m_bFullscreen == false))
    {
      shared =0;
    }

    // Make sure we DON'T call with SDL_FULLSCREEN, because we need a view!
    m_Surface = SDL_SetVideoMode(m_iWidth, m_iHeight, 0, SDL_OPENGL);

    // the context SDL creates isn't full screen compatible, so we create new one
    m_glContext = Cocoa_GL_ReplaceSDLWindowContext();
#else
    // We use the RESIZABLE flag or else the SDL wndproc won't let us ResizeSurface(), the
    // first sizing will replace the borderstyle to prevent allowing abritrary resizes
    int options = SDL_OPENGL | SDL_RESIZABLE | (fullscreen?SDL_NOFRAME:0);
    m_Surface = SDL_SetVideoMode(m_iWidth, m_iHeight, 0, options);
    m_glDC = wglGetCurrentDC();
    m_glContext = wglGetCurrentContext();
#endif
    if (m_Surface)
    {
      m_bOK = true;
    }

    if (!b_glewInit)
    {
      if (glewInit()!=GLEW_OK)
      {
        CLog::Log(LOGERROR, "GL: Critical Error. Could not initialise GL Extension Wrangler Library");
      }
      else
      {
        b_glewInit = true;
      }
    }

#ifdef __APPLE__
  }
  else
  {
    // Take the shared context.
    m_glContext = shared->m_glContext;
    MakeCurrent();
    m_bOK = true;
  }

#endif

#else
  int options = SDL_HWSURFACE | SDL_DOUBLEBUF;
  if (fullscreen)
  {
    options |= SDL_FULLSCREEN;
  }
  s_glVendor   = "Unknown";
  s_glRenderer = "Unknown";
  m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 32, options);
  if (m_SDLSurface)
  {
    m_bOK = true;
  }
#endif
}

CSurfaceGL::~CSurfaceGL()
{

}

CSurfaceGL& CSurfaceGL::operator =(const CSurface &base)
{
  CSurface* tmp = dynamic_cast<CSurface *>(this);
  *tmp = base;
  return *this;
}

void CSurfaceGL::EnableVSync(bool enable)
{
#ifdef HAS_SDL_OPENGL
  if (m_bVSync==enable && m_bVsyncInit == true)
    return;

  if (!m_bOK)
    return;

  if (enable)
    CLog::Log(LOGINFO, "GL: Enabling VSYNC");
  else
    CLog::Log(LOGINFO, "GL: Disabling VSYNC");

  // Nvidia cards: See Appendix E. of NVidia Linux Driver Set README
  CStdString strVendor = g_graphicsContext.GetRenderVendor();
  CStdString strRenderer = g_graphicsContext.GetRenderRenderer();
  //CStdString strVendor(s_RenderVendor), strRenderer(s_RenderRenderer);
  strVendor.ToLower();
  strRenderer.ToLower();

  m_iVSyncMode = 0;
  m_iVSyncErrors = 0;
  m_iSwapRate  = 0;
  m_bVSync     = enable;
  m_bVsyncInit = true;

#ifdef HAS_GLX
  // Obtain function pointers
  if (!_glXGetSyncValuesOML  && ExtensionIsSupported("GLX_OML_sync_control"))
    _glXGetSyncValuesOML = (Bool (*)(Display*, GLXDrawable, int64_t*, int64_t*, int64_t*))glXGetProcAddress((const GLubyte*)"glXGetSyncValuesOML");
  if (!_glXSwapBuffersMscOML && ExtensionIsSupported("GLX_OML_sync_control"))
    _glXSwapBuffersMscOML = (int64_t (*)(Display*, GLXDrawable, int64_t, int64_t, int64_t))glXGetProcAddress((const GLubyte*)"glXSwapBuffersMscOML");
  if (!_glXWaitVideoSyncSGI  && ExtensionIsSupported("GLX_SGI_video_sync"))
    _glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  if (!_glXGetVideoSyncSGI   && ExtensionIsSupported("GLX_SGI_video_sync"))
    _glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!_glXSwapIntervalSGI   && ExtensionIsSupported("GLX_SGI_swap_control") )
    _glXSwapIntervalSGI = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalSGI");
  if (!_glXSwapIntervalMESA  && ExtensionIsSupported("GLX_MESA_swap_control"))
    _glXSwapIntervalMESA = (int (*)(int))glXGetProcAddress((const GLubyte*)"glXSwapIntervalMESA");
#elif defined (_WIN32)
  if (!_wglSwapIntervalEXT)
    _wglSwapIntervalEXT = (BOOL (APIENTRY *)(GLint))wglGetProcAddress("wglSwapIntervalEXT");
  if (!_wglGetSwapIntervalEXT )
    _wglGetSwapIntervalEXT = (int (APIENTRY *)())wglGetProcAddress("wglGetSwapIntervalEXT");
#endif

#ifdef HAS_GLX
  if (_glXSwapIntervalSGI)
    _glXSwapIntervalSGI(0);
  if (_glXSwapIntervalMESA)
    _glXSwapIntervalMESA(0);
#elif defined (_WIN32)
  if (_wglSwapIntervalEXT)
    _wglSwapIntervalEXT(0);
#elif defined (__APPLE__)
  Cocoa_GL_EnableVSync(false);
#endif

  if (enable)
  {
#ifdef HAS_GLX
    // now let's see if we have some system to do specific vsync handling
    if (_glXGetSyncValuesOML && _glXSwapBuffersMscOML && !m_iVSyncMode)
    {
      int64_t ust, msc, sbc;
      if(_glXGetSyncValuesOML(s_dpy, m_glWindow, &ust, &msc, &sbc))
        m_iVSyncMode = 5;
      else
        CLog::Log(LOGWARNING, "%s - glXGetSyncValuesOML failed", __FUNCTION__);
    }
    if (_glXWaitVideoSyncSGI && _glXGetVideoSyncSGI && !m_iVSyncMode && strVendor.find("nvidia") == std::string::npos)
    {
      unsigned int count;
      if(_glXGetVideoSyncSGI(&count) == 0)
        m_iVSyncMode = 3;
      else
        CLog::Log(LOGWARNING, "%s - glXGetVideoSyncSGI failed, glcontext probably not direct", __FUNCTION__);
    }
    if (_glXSwapIntervalSGI && !m_iVSyncMode)
    {
      if(_glXSwapIntervalSGI(1) == 0)
        m_iVSyncMode = 2;
      else
        CLog::Log(LOGWARNING, "%s - glXSwapIntervalSGI failed", __FUNCTION__);
    }
    if (_glXSwapIntervalMESA && !m_iVSyncMode)
    {
      if(_glXSwapIntervalMESA(1) == 0)
        m_iVSyncMode = 2;
      else
        CLog::Log(LOGWARNING, "%s - glXSwapIntervalMESA failed", __FUNCTION__);
    }
#elif defined (_WIN32)
    if (_wglSwapIntervalEXT && !m_iVSyncMode)
    {
      if(_wglSwapIntervalEXT(1))
      {
        if(_wglGetSwapIntervalEXT() == 1)
          m_iVSyncMode = 2;
        else
          CLog::Log(LOGWARNING, "%s - wglGetSwapIntervalEXT didn't return the set swap interval", __FUNCTION__);
      }
      else
        CLog::Log(LOGWARNING, "%s - wglSwapIntervalEXT failed", __FUNCTION__);
    }
#elif defined (__APPLE__)
    if (!m_iVSyncMode)
    {
      Cocoa_GL_EnableVSync(true);
      m_iVSyncMode = 10;
    }
#endif

    if(g_advancedSettings.m_ForcedSwapTime != 0.0)
    {
      /* some hardware busy wait on swap/glfinish, so we must manually sleep to avoid 100% cpu */
      double rate = g_graphicsContext.GetFPS();
      if (rate <= 0.0 || rate > 1000.0)
      {
        CLog::Log(LOGWARNING, "Unable to determine a valid horizontal refresh rate, vsync workaround disabled %.2g", rate);
        m_iSwapRate = 0;
      }
      else
      {
        __int64 freq;
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        m_iSwapRate   = (__int64)((double)freq / rate);
        m_iSwapTime   = (__int64)(0.001 * g_advancedSettings.m_ForcedSwapTime * freq);
        m_iSwapStamp  = 0;
        CLog::Log(LOGINFO, "GL: Using artificial vsync sleep with rate %f", rate);
        if(!m_iVSyncMode)
          m_iVSyncMode = 1;
      }
    }

    if(!m_iVSyncMode)
      CLog::Log(LOGERROR, "GL: Vertical Blank Syncing unsupported");
    else
      CLog::Log(LOGINFO, "GL: Selected vsync mode %d", m_iVSyncMode);
  }
#endif
}

void CSurfaceGL::Flip()
{
  if (m_bOK && m_bDoublebuffer)
  {
#ifdef _WIN32
    int priority;
    DWORD_PTR affinity;
#endif
    if (m_iVSyncMode && m_iSwapRate != 0)
    {
#ifdef HAS_SDL_OPENGL
      glFlush();
#endif
#ifdef _WIN32
      priority = GetThreadPriority(GetCurrentThread());
      affinity = SetThreadAffinityMask(GetCurrentThread(), 1);
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

      __int64 curr, diff, freq;
      QueryPerformanceCounter((LARGE_INTEGER*)&curr);
      QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

      if(m_iSwapStamp == 0)
        m_iSwapStamp = curr;

      /* calculate our next swap timestamp */
      diff = curr - m_iSwapStamp;
      diff = m_iSwapRate - diff % m_iSwapRate;
      m_iSwapStamp = curr + diff;

      /* sleep as close as we can before, assume 1ms precision of sleep *
      * this should always awake so that we are guaranteed the given   *
      * m_iSwapTime to do our swap                                     */
      diff = (diff - m_iSwapTime) * 1000 / freq;
      if(diff > 0)
        Sleep((DWORD)diff);
    }

#ifdef HAS_GLX
    if (m_iVSyncMode == 3)
    {
      glFinish();
      unsigned int before = 0, after = 0;
      if(_glXGetVideoSyncSGI(&before) != 0)
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

      glXSwapBuffers(s_dpy, m_glWindow);
      glFinish();

      if(_glXGetVideoSyncSGI(&after) != 0)
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

      if(after == before)
      {
        CLog::Log(LOGINFO, "GL: retrace count didn't change after buffer swap, switching to vsync mode 4");
        m_iVSyncMode = 4;
      }
    }
    else if (m_iVSyncMode == 4)
    {
      glFinish();
      unsigned int before = 0, swap = 0, after = 0;
      if(_glXGetVideoSyncSGI(&before) != 0)
      {
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);
        EnableVSync(true);
      }

      if(_glXWaitVideoSyncSGI(2, (before+1)%2, &swap) != 0)
        CLog::Log(LOGERROR, "%s - glXWaitVideoSyncSGI - Returned error", __FUNCTION__);

      glXSwapBuffers(s_dpy, m_glWindow);
      glFinish();

      if(_glXGetVideoSyncSGI(&after) != 0)
        CLog::Log(LOGERROR, "%s - glXGetVideoSyncSGI - Failed to get current retrace count", __FUNCTION__);

      if(after == before)
        CLog::Log(LOGERROR, "%s - glXWaitVideoSyncSGI - Woke up early", __FUNCTION__);

      if(after > before + 1)
        m_iVSyncErrors++;
      else
        m_iVSyncErrors = 0;

      if(m_iVSyncErrors > 30)
      {
        CLog::Log(LOGINFO, "GL: retrace count seems to be changing due to the swapbuffers call, switching to vsync mode 3");
        m_iVSyncMode = 3;
      }
    }
    else if (m_iVSyncMode == 5)
    {
      int64_t ust, msc, sbc;
      if(_glXGetSyncValuesOML(s_dpy, m_glWindow, &ust, &msc, &sbc))
        _glXSwapBuffersMscOML(s_dpy, m_glWindow, msc, 0, 0);
      else
      {
        CLog::Log(LOGERROR, "%s - glXSwapBuffersMscOML - Failed to get current retrace count", __FUNCTION__);
        EnableVSync(true);
      }
      CLog::Log(LOGINFO, "%s - ust:%"PRId64", msc:%"PRId64", sbc:%"PRId64"", __FUNCTION__, ust, msc, sbc);
    }
    else
      glXSwapBuffers(s_dpy, m_glWindow);
#elif defined(__APPLE__)
    Cocoa_GL_SwapBuffers(m_glContext);
#elif defined(HAS_SDL_OPENGL)
    SDL_GL_SwapBuffers();
#elif defined(HAS_SDL)
    SDL_Flip(m_SDLSurface);
#endif

    if (m_iVSyncMode && m_iSwapRate != 0)
    {
      __int64 curr, diff;
      QueryPerformanceCounter((LARGE_INTEGER*)&curr);

#ifdef _WIN32
      SetThreadPriority(GetCurrentThread(), priority);
      SetThreadAffinityMask(GetCurrentThread(), affinity);
#endif

      diff = curr - m_iSwapStamp;
      m_iSwapStamp = curr;
      if (abs(diff - m_iSwapRate) < abs(diff))
        CLog::Log(LOGDEBUG, "%s - missed requested swap",__FUNCTION__);
    }
  }
#ifdef HAS_SDL_OPENGL
  else
  {
    glFlush();
  }
#endif
}

bool CSurfaceGL::MakeCurrent()
{
  if (m_pShared)
    return m_pShared->MakeCurrent();

#ifdef HAS_SDL_OPENGL
  if (!m_glContext)
    return false;
#endif

#ifdef HAS_GLX
  GLXDrawable drawable = None;
  if (m_glWindow)
    drawable = m_glWindow;
  else if(m_glPBuffer)
    drawable = m_glPBuffer;
  else
    return false;

  //attempt up to 10 times
  int i = 0;
  while (i<=10 && !glXMakeCurrent(s_dpy, drawable, m_glContext))
  {
    Sleep(5);
    i++;
  }
  if (i==10)
    return false;
  return true;
#endif

#ifdef __APPLE__
  Cocoa_GL_MakeCurrentContext(m_glContext);
  return true;
#endif

#ifdef _WIN32
#ifdef HAS_SDL // TODO:DIRECTX
  if(wglGetCurrentContext() == m_glContext)
    return true;
  else
    return (wglMakeCurrent(m_glDC, m_glContext) == TRUE);
#endif
#endif
  return false;
}

void CSurfaceGL::RefreshCurrentContext()
{
#ifdef HAS_GLX
  ReleaseContext();
  MakeCurrent();
#endif

#ifdef __APPLE__
  m_glContext = Cocoa_GL_GetCurrentContext();
#endif
}

void CSurfaceGL::ReleaseContext()
{
#ifdef HAS_GLX
  {
    CLog::Log(LOGINFO, "GL: ReleaseContext");
    glXMakeCurrent(s_dpy, None, NULL);
  }
#endif
#ifdef __APPLE__
  // Nothing?
#endif
#ifdef _WIN32
  if (IsShared())
    m_pShared->ReleaseContext();
#ifdef HAS_SDL // TODO:DIRECTX
  else if (m_glContext)
    wglMakeCurrent(NULL, NULL);
#endif
#endif
}

bool CSurfaceGL::ResizeSurface(int newWidth, int newHeight)
{
  CLog::Log(LOGDEBUG, "Asking to resize surface to %d x %d", newWidth, newHeight);
#ifdef HAS_GLX
  if (m_parentWindow)
  {
    XLockDisplay(s_dpy);
    XResizeWindow(s_dpy, m_parentWindow, newWidth, newHeight);
    XResizeWindow(s_dpy, m_glWindow, newWidth, newHeight);
    glXWaitX();
    XUnlockDisplay(s_dpy);
  }
#endif
#ifdef __APPLE__
  m_glContext = Cocoa_GL_ResizeWindow(m_glContext, newWidth, newHeight);
  // If we've resized, we likely lose the vsync settings so get it back.
  if (m_bVSync)
  {
    Cocoa_GL_EnableVSync(m_bVSync);
  }
#endif
#ifdef _WIN32
#ifdef HAS_SDL // TODO:DIRECTX
  SDL_SysWMinfo sysInfo;
  SDL_VERSION(&sysInfo.version);
  if (SDL_GetWMInfo(&sysInfo))
  {
    RECT rBounds;
    HMONITOR hMonitor;
    MONITORINFOEX mi;
    HWND hwnd = sysInfo.window;

    // Start by getting the current window rect and centering the new window on
    // the monitor that window is on
    GetWindowRect(hwnd, &rBounds);
    hMonitor = MonitorFromRect(&rBounds, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    g_VideoReferenceClock.SetMonitor(mi); //let the videoreferenceclock know which monitor we're on

    rBounds.left = mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - newWidth) / 2;
    rBounds.top = mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top - newHeight) / 2;
    rBounds.right = rBounds.left + newWidth;
    rBounds.bottom = rBounds.top + newHeight;

    // if this covers the screen area top to bottom, remove the window borders and caption bar
    m_bCoversScreen = (mi.rcMonitor.top + newHeight == mi.rcMonitor.bottom);
    CLog::Log(LOGDEBUG, "New size %s the screen", (m_bCoversScreen ? "covers" : "does not cover"));

    DWORD styleOut, styleIn;
    DWORD swpOptions = SWP_NOCOPYBITS | SWP_SHOWWINDOW;
    HWND hInsertAfter;

    styleIn = styleOut = GetWindowLong(hwnd, GWL_STYLE);
    // We basically want 2 styles, one that is our maximized borderless
    // and one with a caption and non-resizable frame
    if (m_bCoversScreen)
    {
      styleOut = WS_VISIBLE | WS_CLIPSIBLINGS | WS_POPUP;
      LockSetForegroundWindow(LSFW_LOCK);
    }
    else
    {
      styleOut = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
      LockSetForegroundWindow(LSFW_UNLOCK);
    }

    if (IsOnTop())
    {
      hInsertAfter = HWND_TOPMOST;
    }
    else
    {
      hInsertAfter = HWND_NOTOPMOST;
    }

    if (styleIn != styleOut)
    {
      SetWindowLong(hwnd, GWL_STYLE, styleOut);
      // if the style is changing, we are either adding or removing a frame so need to notify
      swpOptions |= SWP_FRAMECHANGED;
    }

    // Now adjust the size of the window so that the client rect is the requested size
    AdjustWindowRectEx(&rBounds, styleOut, false, 0); // there is never a menu

    // finally, move and resize the window
    SetWindowPos(hwnd, hInsertAfter, rBounds.left, rBounds.top,
      rBounds.right - rBounds.left, rBounds.bottom - rBounds.top,
      swpOptions);

    SetForegroundWindow(hwnd);

    SDL_SetWidthHeight(newWidth, newHeight);

    return true;
  }
#endif
#endif
  return false;
}

bool CSurfaceGL::glxIsSupported(const char* extension)
{
  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

#ifdef HAS_GLX
  if (s_glxExt.length()==0)
  {
    s_glxExt  = " ";
    s_glxExt += (const char*)glXQueryExtensionsString(s_dpy, DefaultScreen(s_dpy));
    s_glxExt += " ";
  }
#endif

  return s_glxExt.find(name) != std::string::npos;
}

/*
void CSurfaceGL::GetRenderVersion(int& maj, int& min)
{
#ifdef HAS_SDL_OPENGL
  if (s_RenderMajVer==0)
  {
    const char* ver = (const char*)glGetString(GL_VERSION);
    if (ver != 0)
      sscanf(ver, "%d.%d", &s_RenderMajVer, &s_RenderMinVer);
  }

  if (s_RenderVendor.length()==0)
  {
    s_RenderVendor   = (const char*)glGetString(GL_VENDOR);
    s_RenderRenderer = (const char*)glGetString(GL_RENDERER);
  }
#ifdef HAS_GLX
  if (s_RenderExt.length()==0)
  {
    s_RenderExt  = " ";
    s_RenderExt += (const char*)glXQueryExtensionsString(s_dpy, DefaultScreen(s_dpy));
    s_RenderExt += " ";
  }
#endif

#endif
  maj = s_RenderMajVer;
  min = s_RenderMinVer;
}
*/


void* CSurfaceGL::GetRenderWindow()
{
  SDL_SysWMinfo sysInfo;
  SDL_VERSION(&sysInfo.version);

  if (SDL_GetWMInfo(&sysInfo))
  {
    HWND hwnd = sysInfo.window;
    if(hwnd != NULL)
      return hwnd;
  }

  return NULL;
}