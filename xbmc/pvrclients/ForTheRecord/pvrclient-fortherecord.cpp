/*
 *      Copyright (C) 2010 Marcel Groothuis
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "client.h"
//#include "timers.h"
#include "channel.h"
#include "recordinggroup.h"
#include "recordingsummary.h"
#include "recording.h"
#include "epg.h"
#include "utils.h"
#include "pvrclient-fortherecord.h"
#include "fortherecordrpc.h"

using namespace std;

/************************************************************/
/** Class interface */

cPVRClientForTheRecord::cPVRClientForTheRecord()
{
  m_bConnected             = false;
  //m_bStop                  = true;
  m_bTimeShiftStarted      = false;
  m_BackendUTCoffset       = 0;
  m_BackendTime            = 0;
  m_tsreader               = NULL;
  m_channel_id_offset      = 0;
  m_epg_id_offset          = 0;
}

cPVRClientForTheRecord::~cPVRClientForTheRecord()
{
  XBMC->Log(LOG_DEBUG, "->~cPVRClientForTheRecord()");
}


bool cPVRClientForTheRecord::Connect()
{
  string result;
  char buffer[256];

  snprintf(buffer, 256, "http://%s:%i/", g_szHostname.c_str(), g_iPort);
  g_szBaseURL = buffer;

  XBMC->Log(LOG_INFO, "Connect() - Connecting to %s", g_szBaseURL.c_str());

  int version = ForTheRecord::Ping(FTR_REST_API_VERSION);

  switch (version)
  {
  case -1:
    XBMC->Log(LOG_NOTICE, "Ping Ok. The client is too old.\n");
    break;
  case 0:
    XBMC->Log(LOG_INFO, "Ping Ok. The client and server are compatible.\n");
    break;
  case 1:
    XBMC->Log(LOG_INFO, "Ping Ok. The client is newer than the server.\n");
    break;
  default:
    XBMC->Log(LOG_ERROR, "Ping failed... No connection to ForTheRecord.\n");
    return false;
  }

  m_bConnected = true;
  return true;
}

void cPVRClientForTheRecord::Disconnect()
{
  string result;

  XBMC->Log(LOG_INFO, "Disconnect");

  if (m_bTimeShiftStarted)
  {
    //TODO: tell ForTheRecord that it should stop streaming
  }

  m_bConnected = false;
}

/************************************************************/
/** General handling */

// Used among others for the server name string in the "Recordings" view
const char* cPVRClientForTheRecord::GetBackendName()
{
  XBMC->Log(LOG_DEBUG, "->GetBackendName()");

  if(m_BackendName.length() == 0)
  {
    m_BackendName = "ForTheRecord (";
    m_BackendName += g_szHostname.c_str();
    m_BackendName += ")";
  }

  return m_BackendName.c_str();
}

const char* cPVRClientForTheRecord::GetBackendVersion()
{
  // Don't know how to fetch this from ForTheRecord
  return "0.0";
}

const char* cPVRClientForTheRecord::GetConnectionString()
{
  XBMC->Log(LOG_DEBUG, "->GetConnectionString()");

  return g_szBaseURL.c_str();
}

PVR_ERROR cPVRClientForTheRecord::GetDriveSpace(long long *total, long long *used)
{
  *total = 0;
  *used = 0;

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::GetBackendTime(time_t *localTime, int *gmtOffset)
{
  return PVR_ERROR_SERVER_ERROR;
}

/************************************************************/
/** EPG handling */

PVR_ERROR cPVRClientForTheRecord::RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start, time_t end)
{
  XBMC->Log(LOG_DEBUG, "->RequestEPGForChannel(%i)", channel.number);

  cChannel* ftrchannel = FetchChannel(channel.uid);

  struct tm* convert = localtime(&start);
  struct tm tm_start = *convert;
  convert = localtime(&end);
  struct tm tm_end = *convert;

  if(ftrchannel)
  {
    Json::Value response;
    int retval;

    retval = ForTheRecord::GetEPGData(ftrchannel->GuideChannelID(), tm_start, tm_end, response);

    if (retval != E_FAILED)
    {
      XBMC->Log(LOG_DEBUG, "GetEPGData returned %i, response.type == %i, response.size == %i.", retval, response.type(), response.size());
      if( response.type() == Json::arrayValue)
      {
        int size = response.size();
        PVR_PROGINFO proginfo;
        cEpg epg;
        
        // parse channel list
        for ( int index =0; index < size; ++index )
        {
          if (epg.Parse(response[index]))
          {
            m_epg_id_offset++;
            proginfo.channum         = channel.number;
            proginfo.uid             = m_epg_id_offset;
            proginfo.title           = epg.Title();
            proginfo.subtitle        = epg.Subtitle();
            proginfo.description     = "";
            proginfo.starttime       = epg.StartTime();
            proginfo.endtime         = epg.EndTime();
            proginfo.genre_type      = 0;
            proginfo.genre_sub_type  = 0;
            proginfo.genre_text    = "";
            proginfo.parental_rating = 0;
            PVR->TransferEpgEntry(handle, &proginfo);
          }
          epg.Reset();
        }
      }
    }
    else
    {
      XBMC->Log(LOG_ERROR, "GetEPGData failed for channel id:%i", channel.number);
    }
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Channel (%i) did not return a channel class.", channel.number);
  }

  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Channel handling */

int cPVRClientForTheRecord::GetNumChannels()
{
  // Not directly possible in ForTheRecord
  // Just return a non-zero value to XBMC otherwise
  // XBMC won't call RequestChannelList
  return 1; //0 = no channels available
}

PVR_ERROR cPVRClientForTheRecord::RequestChannelList(PVRHANDLE handle, int radio)
{
  Json::Value response;
  int retval = -1;

  if (!radio)
  {
    retval = ForTheRecord::ForTheRecordJSONRPC("ForTheRecord/Scheduler/Channels/Television", "?visibleOnly=false", response);
  }
  else
  {
    retval = ForTheRecord::ForTheRecordJSONRPC("ForTheRecord/Scheduler/Channels/Radio", "?visibleOnly=false", response);        
  }

  if(retval >= 0)
  {           
    if( response.type() == Json::arrayValue)
    {
      int size = response.size();

      // parse channel list
      for ( int index = 0; index < size; ++index )
      {

        cChannel channel;
        if( channel.Parse(response[index]) )
        {
          PVR_CHANNEL tag;
          memset(&tag, 0 , sizeof(tag));
          //Hack: assumes that the order of the channel list is fixed.
          //      We can't use the ForTheRecord channel id's. They are GUID strings (128 bit int).       
          tag.number =  m_channel_id_offset + 1;
          //if (channel.LCN())
          //  tag.uid = channel.LCN();
          //else
          tag.uid = tag.number;
          tag.name = channel.Name();
          tag.callsign = channel.Name(); //Used for automatic channel icon search
          tag.iconpath = "";
          tag.encryption = 0; //How to fetch this from ForTheRecord??
          tag.radio = (channel.Type() == ForTheRecord::Radio ? true : false);
          tag.hide = false;
          tag.recording = false;
          tag.bouquet = 0;
          tag.multifeed = false;
          //Use OpenLiveStream to read from the timeshift .ts file or an rtsp stream
          tag.stream_url = "";
          tag.input_format = "mpegts";

          if (!tag.radio)
          {
            XBMC->Log(LOG_DEBUG, "Found TV channel: %s\n", channel.Name());
          }
          else
          {
            XBMC->Log(LOG_DEBUG, "Found Radio channel: %s\n", channel.Name());
          }
          channel.SetID(tag.uid);
          m_Channels.push_back(channel); //Local cache...
          PVR->TransferChannelEntry(handle, &tag);
          m_channel_id_offset++;
        }
      }

      return PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
      return PVR_ERROR_UNKNOWN;
    }
  }
  else
  {
    XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
  }

  return PVR_ERROR_SERVER_ERROR;


}

/************************************************************/
/** Record handling **/

int cPVRClientForTheRecord::GetNumRecordings(void)
{
  Json::Value response;
  int retval = -1;
  int iNumRecordings = 0;

  XBMC->Log(LOG_DEBUG, "GetNumRecordings()");
  retval = ForTheRecord::GetRecordingGroupByTitle(response);
  if (retval >= 0)
  {
    int size = response.size();

    // parse channelgroup list
    for ( int index = 0; index < size; ++index )
    {
      cRecordingGroup recordinggroup;
      if (recordinggroup.Parse(response[index]))
      {
        iNumRecordings += recordinggroup.RecordingsCount();
      }
    }
  }
  return iNumRecordings;
}

PVR_ERROR cPVRClientForTheRecord::RequestRecordingsList(PVRHANDLE handle)
{
  Json::Value recordinggroupresponse;
  int retval = -1;
  int iNumRecordings = 0;

  retval = ForTheRecord::GetRecordingGroupByTitle(recordinggroupresponse);
  if(retval >= 0)
  {           
    // process list of recording groups
    int size = recordinggroupresponse.size();
    for ( int recordinggroupindex = 0; recordinggroupindex < size; ++recordinggroupindex )
    {
      cRecordingGroup recordinggroup;
      if (recordinggroup.Parse(recordinggroupresponse[recordinggroupindex]))
      {
        Json::Value recordingsbytitleresponse;
        retval = ForTheRecord::GetRecordingsForTitle(recordinggroup.ProgramTitle(), recordingsbytitleresponse);
        if (retval >= 0)
        {
          // process list of recording summaries for this group
          int nrOfRecordings = recordingsbytitleresponse.size();
          for (int recordingindex = 0; recordingindex < nrOfRecordings; recordingindex++)
          {
            cRecording recording;
            if (FetchRecordingDetails(recordingsbytitleresponse[recordingindex], recording))
            {
              PVR_RECORDINGINFO tag;
              memset(&tag, 0 , sizeof(tag));
              tag.index           = iNumRecordings;
              tag.channel_name    = recording.ChannelDisplayName();
              tag.lifetime        = MAXLIFETIME; //TODO: recording.Lifetime();
              tag.priority        = 0; //TODO? recording.Priority();
              tag.recording_time  = recording.RecordingStartTime();
              tag.duration        = recording.RecordingStopTime() - recording.RecordingStartTime();
              tag.description     = recording.Description();;
              tag.title           = recording.Title();
              tag.subtitle        = recording.SubTitle();
              tag.directory       = recordinggroup.ProgramTitle().c_str(); //used in XBMC as directory structure below "Server X - hostname"
              tag.stream_url      = recording.RecordingFileName();
              PVR->TransferRecordingEntry(handle, &tag);
              iNumRecordings++;
            }
          }
        }
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}

bool cPVRClientForTheRecord::FetchRecordingDetails(const Json::Value& data, cRecording& recording)
{ 
  bool fRc = false;
  Json::Value recordingresponse;

  cRecordingSummary recordingsummary;
  if (recordingsummary.Parse(data))
  {
    int retval = ForTheRecord::GetRecordingById(recordingsummary.RecordingId(), recordingresponse);
    if (retval >= 0)
    {
      if (recordingresponse.type() == Json::objectValue)
      {
        fRc = recording.Parse(recordingresponse);
      }
    }
  }
  return fRc;
}

PVR_ERROR cPVRClientForTheRecord::DeleteRecording(const PVR_RECORDINGINFO &recinfo)
{
  return PVR_ERROR_NOT_DELETED;
  //return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname)
{
  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Timer handling */

int cPVRClientForTheRecord::GetNumTimers(void)
{
  return 0;
}

PVR_ERROR cPVRClientForTheRecord::RequestTimerList(PVRHANDLE handle)
{
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag)
{
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::AddTimer(const PVR_TIMERINFO &timerinfo)
{
  return PVR_ERROR_NOT_SAVED;
  //return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force)
{
  return PVR_ERROR_NOT_DELETED;
  //return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname)
{
  return PVR_ERROR_NOT_SAVED;
  //return PVR_ERROR_NO_ERROR;
}

PVR_ERROR cPVRClientForTheRecord::UpdateTimer(const PVR_TIMERINFO &timerinfo)
{
  return PVR_ERROR_NOT_SAVED;
  //return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Live stream handling */
cChannel* cPVRClientForTheRecord::FetchChannel(int channel_uid)
{
  // Search for this channel in our local channel list to find the original ChannelID back:
  vector<cChannel>::iterator it;

  for ( it=m_Channels.begin(); it < m_Channels.end(); it++ )
  {
    if (it->ID() == channel_uid)
    {
      return &*it;
      break;
    }
  }

  return NULL;
}


bool cPVRClientForTheRecord::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_DEBUG, "->OpenLiveStream(%i)", channelinfo.number);

  cChannel* channel = FetchChannel(channelinfo.uid);

  if (channel)
  {
    std::string filename;
    XBMC->Log(LOG_INFO, "Tune XBMC channel: %i", channelinfo.number);
    XBMC->Log(LOG_INFO, "Corresponding ForTheRecord channel: %s", channel->Guid().c_str());
    ForTheRecord::TuneLiveStream(channel->Guid(), filename);

    if (filename.length() == 0)
    {
      XBMC->Log(LOG_ERROR, "Could not start the timeshift for channel %i (%s)", channelinfo.number, channel->Guid().c_str());
      return false;
    }

    XBMC->Log(LOG_INFO, "Live stream file: %s", filename.c_str());
    m_bTimeShiftStarted = true;

#ifdef TSREADER
    if (m_tsreader != NULL)
    {
      m_keepalive.StopThread(0);
      m_tsreader->Close();
      delete m_tsreader;
      m_tsreader = new CTsReader();
    } else {
      m_tsreader = new CTsReader();
    }

    // Open Timeshift buffer
    // TODO: rtsp support
    m_tsreader->Open(filename.c_str());
    m_keepalive.StartThread();
#endif
    return true;
  }

  return false;
}

int cPVRClientForTheRecord::ReadLiveStream(unsigned char* buf, int buf_size)
{
#ifdef TSREADER
  unsigned long read_wanted = buf_size;
  unsigned long read_done   = 0;
  static int read_timeouts  = 0;
  unsigned char* bufptr = buf;

  //XBMC->Log(LOG_DEBUG, "->ReadLiveStream(buf_size=%i)", buf_size);

  while (read_done < (unsigned long) buf_size)
  {
    read_wanted = buf_size - read_done;
    if (!m_tsreader)
      return -1;

    if (m_tsreader->Read(bufptr, read_wanted, &read_wanted) > 0)
    {
      usleep(20000);
      read_timeouts++;
      return read_wanted;
    }
    read_done += read_wanted;

    if ( read_done < (unsigned long) buf_size )
    {
      if (read_timeouts > 50)
      {
        XBMC->Log(LOG_INFO, "No data in 1 second");
        read_timeouts = 0;
        return read_done;
      }
      bufptr += read_wanted;
      read_timeouts++;
      usleep(20000);
    }
  }
  read_timeouts = 0;
  return read_done;
#else
  return 0;
#endif //TSREADER
}

void cPVRClientForTheRecord::CloseLiveStream()
{
  string result;

  if (m_bTimeShiftStarted)
  {
#ifdef TSREADER
    if (m_tsreader)
    {
      m_tsreader->Close();
      delete_null(m_tsreader);
    }
    m_keepalive.StopThread();
#endif
    ForTheRecord::StopLiveStream();
    XBMC->Log(LOG_INFO, "CloseLiveStream");
    m_bTimeShiftStarted = false;
  } else {
    XBMC->Log(LOG_DEBUG, "CloseLiveStream: Nothing to do.");
  }
}


bool cPVRClientForTheRecord::SwitchChannel(const PVR_CHANNEL &channelinfo)
{
  return OpenLiveStream(channelinfo);
}


int cPVRClientForTheRecord::GetCurrentClientChannel()
{
  return 0;
}

PVR_ERROR cPVRClientForTheRecord::SignalQuality(PVR_SIGNALQUALITY &qualityinfo)
{
  return PVR_ERROR_NO_ERROR;
}


/************************************************************/
/** Record stream handling */
bool cPVRClientForTheRecord::OpenRecordedStream(const PVR_RECORDINGINFO &recinfo)
{
  XBMC->Log(LOG_DEBUG, "->OpenRecordedStream(index=%i)", recinfo.index);

  return false;
}


void cPVRClientForTheRecord::CloseRecordedStream(void)
{
}

int cPVRClientForTheRecord::ReadRecordedStream(unsigned char* buf, int buf_size)
{
  return -1;
}

/*
 * \brief Request the stream URL for live tv/live radio.
 */
const char* cPVRClientForTheRecord::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  return false;
}
