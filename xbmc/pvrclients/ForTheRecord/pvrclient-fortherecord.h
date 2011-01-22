#pragma once
/*
 *      Copyright (C) 2010 Marcel Groothuis
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

/* Master defines for client control */
#include "../../addons/include/xbmc_pvr_types.h"

#include "channel.h"
#include "recording.h"
#include "guideprogram.h"
#include "os-dependent.h"

#ifdef TSREADER
#include "TSReader.h"
#include "KeepAliveThread.h"
#endif

class cPVRClientForTheRecord
{
public:
  /* Class interface */
  cPVRClientForTheRecord();
  ~cPVRClientForTheRecord();

  /* Server handling */
  bool Connect();
  void Disconnect();
  bool IsUp();

  /* General handling */
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);

  /* EPG handling */
  PVR_ERROR RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start = NULL, time_t end = NULL);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio = 0);

  /* Record handling **/
  int GetNumRecordings(void);
  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname);

  /* Timer handling */
  int GetNumTimers(void);
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force = false);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);

  /* Live stream handling */
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char* buf, int buf_size);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char* buf, int buf_size);
  long long SeekRecordedStream(long long pos, int whence=SEEK_SET);
  long long LengthRecordedStream(void);

  /* Used for rtsp streaming */
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);

private:
  cChannel* FetchChannel(int channel_uid);
  void Close();
  bool FetchRecordingDetails(const Json::Value& data, cRecording& recording);
  bool FetchGuideProgramDetails(std::string Id, cGuideProgram& guideprogram);

  //int                     m_iCurrentChannel;
  bool                    m_bConnected;
  //bool                    m_bStop;
  bool                    m_bTimeShiftStarted;
  std::string             m_BackendName;
  int                     m_BackendVersion;
  time_t                  m_BackendUTCoffset;
  time_t                  m_BackendTime;

  std::vector<cChannel>   m_Channels; // Local channel cache list needed for id to guid conversion
  int                     m_channel_id_offset;
  int                     m_epg_id_offset;
//  CURL*                   m_curl;
#ifdef TSREADER
  CTsReader*              m_tsreader;
  CKeepAliveThread        m_keepalive;
#endif //TSREADER

};
