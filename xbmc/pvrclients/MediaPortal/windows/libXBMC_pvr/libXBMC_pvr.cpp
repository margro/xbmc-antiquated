/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 * margro: Modified version for static linkage
 */

#define USE_DEMUX // enable including of the demuxer packet structure

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "libXBMC_pvr.h"

using namespace std;

cHelper_libXBMC_pvr::cHelper_libXBMC_pvr()
{
  m_Handle      = NULL;
  m_cb          = NULL;
}

bool cHelper_libXBMC_pvr::RegisterMe(void *hdl)
//int PVR_register_me(void *hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me is called with NULL handle !!!\n");
  else
  {
    m_Handle = (AddonCB*) hdl;
    m_cb     = m_Handle->PVRLib_RegisterMe(m_Handle->addonData);
    if (!m_cb)
      fprintf(stderr, "libXBMC_pvr-ERROR: PVRLib_register_me can't get callback table from XBMC !!!\n");
    else
      return true;
  }
  return false;
}

//void PVR_unregister_me()
cHelper_libXBMC_pvr::~cHelper_libXBMC_pvr()
{
  if (m_Handle && m_cb)
    m_Handle->PVRLib_UnRegisterMe(m_Handle->addonData, m_cb);
}

//void PVR_transfer_epg_entry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
void cHelper_libXBMC_pvr::TransferEpgEntry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferEpgEntry(m_Handle->addonData, handle, epgentry);
}

//void PVR_transfer_channel_entry(const PVRHANDLE handle, const PVR_CHANNEL *chan)
void cHelper_libXBMC_pvr::TransferChannelEntry(const PVRHANDLE handle, const PVR_CHANNEL *chan)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferChannelEntry(m_Handle->addonData, handle, chan);
}

//void PVR_transfer_timer_entry(const PVRHANDLE handle, const PVR_TIMERINFO *timer)
void cHelper_libXBMC_pvr::TransferTimerEntry(const PVRHANDLE handle, const PVR_TIMERINFO *timer)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferTimerEntry(m_Handle->addonData, handle, timer);
}

//void PVR_transfer_recording_entry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
void cHelper_libXBMC_pvr::TransferRecordingEntry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording)
{
  if (m_cb == NULL)
    return;

  m_cb->TransferRecordingEntry(m_Handle->addonData, handle, recording);
}

//void PVR_add_menu_hook(PVR_MENUHOOK *hook)
void cHelper_libXBMC_pvr::AddMenuHook(PVR_MENUHOOK *hook)
{
  if (m_cb == NULL)
    return;

  m_cb->AddMenuHook(m_Handle->addonData, hook);
}

//DLLEXPORT void PVR_recording(const char *Name, const char *FileName, bool On)
void cHelper_libXBMC_pvr::Recording(const char *Name, const char *FileName, bool On)
{
  if (m_cb == NULL)
    return;

  m_cb->Recording(m_Handle->addonData, Name, FileName, On);
}

//DLLEXPORT void PVR_trigger_timer_update()
void cHelper_libXBMC_pvr::TriggerTimerUpdate()
{
  if (m_cb == NULL)
    return;

  m_cb->TriggerTimerUpdate(m_Handle->addonData);
}

//DLLEXPORT void PVR_trigger_recording_update()
void cHelper_libXBMC_pvr::TriggerRecordingUpdate()
{
  if (m_cb == NULL)
    return;

  m_cb->TriggerTimerUpdate(m_Handle->addonData);
}

//void DLLEXPORT PVR_free_demux_packet(DemuxPacket* pPacket)
void cHelper_libXBMC_pvr::FreeDemuxPacket(DemuxPacket* pPacket)
{
  if (m_cb == NULL)
    return;

  m_cb->FreeDemuxPacket(m_Handle->addonData, pPacket);
}

//DemuxPacket* DLLEXPORT PVR_allocate_demux_packet(int iDataSize)
DemuxPacket* cHelper_libXBMC_pvr::AllocateDemuxPacket(int iDataSize)
{
  if (m_cb == NULL)
    return NULL;

  return m_cb->AllocateDemuxPacket(m_Handle->addonData, iDataSize);
}

