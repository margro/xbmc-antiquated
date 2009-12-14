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

#include "system.h"
#include "AudioRendererFactory.h"
#include "GUISettings.h"
#include "log.h"
#include "NullDirectSound.h"

#ifdef HAS_PULSEAUDIO
#include "PulseAudioDirectSound.h"
#endif

#ifdef _WIN32
#include "Win32DirectSound.h"
#endif
#ifdef __APPLE__
#include "CoreAudioRenderer.h"
#elif defined(_LINUX)
#include "ALSADirectSound.h"
#endif

#define ReturnOnValidInitialize()          \
{                                          \
  if (audioSink->Initialize(pCallback, device, iChannels, uiSamplesPerSec, uiBitsPerSample, bResample, strAudioCodec, bIsMusic, bPassthrough))  \
    return audioSink;                      \
  else                                     \
  {                                        \
    audioSink->Deinitialize();             \
    delete audioSink;                      \
    audioSink = NULL;                      \
  }                                        \
}\

IAudioRenderer* CAudioRendererFactory::Create(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
  IAudioRenderer* audioSink = NULL;

  CStdString deviceString, device;
  if (bPassthrough)
  {
    deviceString = g_guiSettings.GetString("audiooutput.passthroughdevice");
    if (deviceString.Equals("custom"))
      deviceString = g_guiSettings.GetString("audiooutput.custompassthrough");
      
    // some platforms (osx) do not have a separate passthroughdevice setting.
    if (deviceString.IsEmpty())
      deviceString = g_guiSettings.GetString("audiooutput.audiodevice");
  }
  else
  {
    deviceString = g_guiSettings.GetString("audiooutput.audiodevice");
    if (deviceString.Equals("custom"))
      deviceString = g_guiSettings.GetString("audiooutput.customdevice");
  } 
  int iPos = deviceString.Find(":");
  if (iPos > 0)
  {
    audioSink = CreateFromUri(deviceString.Left(iPos));
    if (audioSink)
    {
      device = deviceString.Right(deviceString.length() - iPos - 1);
      ReturnOnValidInitialize();

      audioSink = new CNullDirectSound();
      audioSink->Initialize(pCallback, device, iChannels, uiSamplesPerSec, uiBitsPerSample, bResample, strAudioCodec, bIsMusic, bPassthrough);
      return audioSink;
    }
  }
  CLog::Log(LOGINFO, "AudioRendererFactory: %s not a explicit device, trying to autodetect.", device.c_str());

  device = deviceString;

/* First pass creation */
#ifdef HAS_PULSEAUDIO
  audioSink = new CPulseAudioDirectSound();
  ReturnOnValidInitialize();
#endif

/* incase none in the first pass was able to be created, fall back to os specific */
#ifdef WIN32
  audioSink = new CWin32DirectSound();
  ReturnOnValidInitialize();
#endif
#ifdef __APPLE__
  audioSink = new CCoreAudioRenderer();
  ReturnOnValidInitialize();
#elif defined(_LINUX)
  audioSink = new CALSADirectSound();
  ReturnOnValidInitialize();
#endif

  audioSink = new CNullDirectSound();
  audioSink->Initialize(pCallback, device, iChannels, uiSamplesPerSec, uiBitsPerSample, bResample, strAudioCodec, bIsMusic, bPassthrough);
  return audioSink;
}

void CAudioRendererFactory::EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough)
{
#ifdef HAS_PULSEAUDIO
  CPulseAudioDirectSound::EnumerateAudioSinks(vAudioSinks, passthrough);
  if (vAudioSinks.size() > 0)
    return;
#endif

#ifdef WIN32
  CWin32DirectSound::EnumerateAudioSinks(vAudioSinks, passthrough);
#endif

#ifdef __APPLE__
  CCoreAudioRenderer::EnumerateAudioSinks(vAudioSinks, passthrough);
#elif defined(_LINUX)
  CALSADirectSound::EnumerateAudioSinks(vAudioSinks, passthrough);
#endif
}

IAudioRenderer *CAudioRendererFactory::CreateFromUri(const CStdString &soundsystem)
{
#ifdef HAS_PULSEAUDIO
  if (soundsystem.Equals("pulse"))
    return new CPulseAudioDirectSound();
#endif

#ifdef WIN32
  if (soundsystem.Equals("directsound"))
    return new CWin32DirectSound();
#endif

#ifdef __APPLE__
  if (soundsystem.Equals("coreaudio"))
    return new CCoreAudioRenderer();
#elif defined(_LINUX)
  if (soundsystem.Equals("alsa"))
    return new CALSADirectSound();
#endif

  if (soundsystem.Equals("null"))
    return new CNullDirectSound();

  return NULL;
}