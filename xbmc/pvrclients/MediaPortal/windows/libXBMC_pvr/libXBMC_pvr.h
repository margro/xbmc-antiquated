#pragma once
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xbmc_pvr_types.h"
#include "AddonHelpers_local.h"

class cHelper_libXBMC_pvr
{
public:
  cHelper_libXBMC_pvr();
  ~cHelper_libXBMC_pvr();

  bool RegisterMe(void *Handle);

  void TransferEpgEntry(const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
  void TransferChannelEntry(const PVRHANDLE handle, const PVR_CHANNEL *chan);
  void TransferTimerEntry(const PVRHANDLE handle, const PVR_TIMERINFO *timer);
  void TransferRecordingEntry(const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);
  void AddMenuHook(PVR_MENUHOOK *hook);
  void Recording(const char *Name, const char *FileName, bool On);
  void TriggerTimerUpdate();
  void TriggerRecordingUpdate();

#ifdef USE_DEMUX
  void FreeDemuxPacket(DemuxPacket* pPacket);
  DemuxPacket* AllocateDemuxPacket(int iDataSize);
#endif

private:
  AddonCB    *m_Handle;
  CB_PVRLib  *m_cb;
};
