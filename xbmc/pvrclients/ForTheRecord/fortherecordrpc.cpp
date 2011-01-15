/**
 * \brief  Proof of concept code to access ForTheRecord's REST api using C++ code
 * \author Marcel Groothuis
 ***************************************************************************
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
 ***************************************************************************
 * Depends on:
 * - libcurl: http://curl.haxx.se/
 * - jsoncpp: http://jsoncpp.sourceforge.net/
 *
 * Tested under Windows and Linux
 */

#include <stdio.h>
#include "curl/curl.h"
#include "client.h"
#include "utils.h"
#include "fortherecordrpc.h"

/**
 * \brief CURL callback function that receives the return data from the HTTP get/post calls
 */
static size_t curl_write_data(void *buffer, size_t size, size_t nmemb, void *stream)
{
    //XBMC->Log(LOG_DEBUG, "\nwrite_data size=%i, nmemb=%i\n", size, nmemb, (char*) buffer);

    // Calculate the real size of the incoming buffer
    size_t realsize = size * nmemb;
 
    std::string* response = (std::string*) stream;
    // Dirty... needs some checking
    *response += (char*) buffer;
    
    return realsize;
}

/**
 * \brief Namespace with ForTheRecord related code
 */
namespace ForTheRecord
{
    // The usable urls:
    //http://localhost:49943/ForTheRecord/Control/help
    //http://localhost:49943/ForTheRecord/Scheduler/help
    //http://localhost:49943/ForTheRecord/Guide/help
    //http://localhost:49943/ForTheRecord/Core/help
    //http://localhost:49943/ForTheRecord/Configuration/help
    //http://localhost:49943/ForTheRecord/Log/help

    int ForTheRecordRPC(std::string command, std::string arguments, std::string& json_response)
    {
      CURL *curl;
      CURLcode res;
      std::string url = g_szBaseURL + command;
      
      XBMC->Log(LOG_DEBUG, "URL: %s\n", url.c_str());

      curl = curl_easy_init();
      
      if(curl)
      {
        struct curl_slist *chunk = NULL;
 
        chunk = curl_slist_append(chunk, "Content-type: application/json; charset=UTF-8");
        chunk = curl_slist_append(chunk, "Accept: application/json; charset=UTF-8");

        /* Specify the URL */
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_iConnectTimeout);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        /* Now specify the POST data */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, arguments.c_str());
        /* Define our callback to get called when there's data to be written */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data); 
        /* Set a pointer to our struct to pass to the callback */ 
        json_response = "";
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_response);

        /* Perform the request */
        res = curl_easy_perform(curl);

        /* always cleanup */
        curl_easy_cleanup(curl);
        return 0;
      }
      else
      {
        return E_FAILED;
      }
    }

    int ForTheRecordJSONRPC(std::string command, std::string arguments, Json::Value& json_response)
    {
      std::string response;
      int retval = E_FAILED;
      retval = ForTheRecordRPC(command, arguments, response);

      if (retval == CURLE_OK)
      {
#ifdef DEBUG
        // Print only the first 512 bytes, otherwise XBMC will crash...
        XBMC->Log(LOG_DEBUG, "Response: %s\n", response.substr(0,512).c_str());
#endif
        if (response.length() != 0)
        {
          Json::Reader reader;

          bool parsingSuccessful = reader.parse(response, json_response);

          if ( !parsingSuccessful )
          {
              XBMC->Log(LOG_DEBUG, "Failed to parse %s: \n%s\n", 
              response.c_str(),
              reader.getFormatedErrorMessages().c_str() );
              return E_FAILED;
          }
        }
        else
        {
          XBMC->Log(LOG_DEBUG, "Empty response");
          return E_FAILED;
        }
#ifdef DEBUG
        printValueTree(stdout, json_response);
#endif
      }

      return retval;
    }

    /**
      * \brief Retrieve the TV channels that are in the guide
      */
    int RequestGuideChannelList()
    {
        Json::Value root;
        int retval = E_FAILED;
        
        retval = ForTheRecordJSONRPC("ForTheRecord/Guide/Channels/Television", "", root);
        
        if(retval >= 0)
        {
            if( root.type() == Json::arrayValue)
            {
                int size = root.size();
                
                // parse channel list
                for ( int index =0; index < size; ++index )
                {
                    std::string name = root[index]["Name"].asString();
                    XBMC->Log(LOG_DEBUG, "Found channel %i: %s\n", index, name.c_str());
                }
                return size;
            }
            else
            {
                XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
                return -1;
            }
        }
        else
        {
            XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
        }
        
        return retval;
    }

    /**
     * \brief Get the list with channel groups from 4TR
     * \param channelType The channel type (Television or Radio)
     */
    int RequestChannelGroups(enum ChannelType channelType)
    {
        Json::Value root;
        int retval = -1;
        
        if (channelType == Television)
        {
            retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/ChannelGroups/Television", "?visibleOnly=false", root);
        }
        else if (channelType == Radio)
        {
            retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/ChannelGroups/Radio", "?visibleOnly=false", root);        
        }
        
        if(retval >= 0)
        {
            if( root.type() == Json::arrayValue)
            {
                int size = root.size();
                
                // parse channel group list
                for ( int index =0; index < size; ++index )
                {
                    std::string name = root[index]["GroupName"].asString();
                    std::string guid = root[index]["ChannelGroupId"].asString();
                    if (channelType == Television)
                    {
                        XBMC->Log(LOG_DEBUG, "Found TV channel group %s: %s\n", guid.c_str(), name.c_str());
                    }
                    else if (channelType == Radio)
                    {
                        XBMC->Log(LOG_DEBUG, "Found Radio channel group %s: %s\n", guid.c_str(), name.c_str());
                    }
                }
                return size;
            } else {
                XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
                return -1;
            }
        }
        else
        {
           XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
        }
        
        return retval;
    }
    
    /**
     * \brief Get the list with TV channel groups from 4TR
     */
    int RequestTVChannelGroups()
    {
        return RequestChannelGroups(Television);
    }
    
    /**
     * \brief Get the list with Radio channel groups from 4TR
     */
    int RequestRadioChannelGroups()
    {
        return RequestChannelGroups(Radio);
    }

    /**
     * \brief Get the list with channels from 4TR
     * \param channelType The channel type (Television or Radio)
     */
    int RequestChannels(enum ChannelType channelType)
    {
        Json::Value response;
        int retval = -1;
        
        if (channelType == Television)
        {
            retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/Channels/Television", "?visibleOnly=false", response);
        }
        else if (channelType == Radio)
        {
            retval = ForTheRecordJSONRPC("ForTheRecord/Scheduler/Channels/Radio", "?visibleOnly=false", response);        
        }
        
        if(retval >= 0)
        {           
            if( response.type() == Json::arrayValue)
            {
                int size = response.size();
                
                // parse channel list
                for ( int index =0; index < size; ++index )
                {
                    std::string name = response[index]["DisplayName"].asString();
                    std::string guid = response[index]["ChannelId"].asString();
                    if (channelType == Television)
                    {
                        XBMC->Log(LOG_DEBUG, "Found TV channel %s: %s\n", guid.c_str(), name.c_str());
                    }
                    else if (channelType == Radio)
                    {
                        XBMC->Log(LOG_DEBUG, "Found Radio channel %s: %s\n", guid.c_str(), name.c_str());
                    }
                }
                return size;
            }
            else
            {
                XBMC->Log(LOG_DEBUG, "Unknown response format. Expected Json::arrayValue\n");
                return -1;
            }
        } else
        {
            XBMC->Log(LOG_DEBUG, "RequestChannelList failed. Return value: %i\n", retval);
        }
        
        return retval;
    }

    /**
     * \brief Get the list with TV channel groups from 4TR
     */
    int RequestTVChannels()
    {
        return RequestChannels(Television);
    }
    
    /**
     * \brief Get the list with Radio channel groups from 4TR
     */
    int RequestRadioChannels()
    {
        return RequestChannels(Radio);
    }
    
    /*
     * \brief Ping core service.
     * \param requestedApiVersion The API version the client needs, pass in Constants.ForTheRecordRestApiVersion.
     * \return 0 if client and server are compatible, -1 if the client is too old, +1 if the client is newer than the server and -2 if the connection failed (server down?)
     */
    int Ping(int requestedApiVersion)
    {
      Json::Value response;
      char command[128];
      int version = -2;
      
      snprintf(command, 128, "ForTheRecord/Core/Ping/%i", requestedApiVersion);

      int retval = ForTheRecordJSONRPC(command, "", response);

      if (retval != E_FAILED)
      {
        if (response.type() == Json::intValue)
        {
          version = response.asInt();
        }
      }

      return version;
    }

    int GetLiveStreams()
    {
      Json::Value response;
      int retval = ForTheRecordJSONRPC("ForTheRecord/Control/GetLiveStreams", "", response);

      if (retval != E_FAILED)
      {
        if (response.type() == Json::arrayValue)
        {
          int size = response.size();
          
          // parse live stream list
          for ( int index =0; index < size; ++index )
          {
            printf("Found live stream %i: %s\n", index, response["LiveStream"]["RtspUrl"]);
          }
        }
      }
      return retval;
    }

    //Remember the last LiveStream object to be able to stop the stream again
    Json::Value g_current_livestream;

    int TuneLiveStream(const std::string channel_id, std::string& stream)
    {
      // Send only a channel object in json format, no LiveStream object.
      // FTR will answer with a LiveStream object.
      std::string arguments = "{\"Channel\":{\"BroadcastStart\":\"String content\",\"BroadcastStop\":\"String content\",\"ChannelId\":\"";
      arguments += channel_id;
      arguments += "\",\"ChannelType\":0,\"DefaultPostRecordSeconds\":2147483647,\"DefaultPreRecordSeconds\":2147483647,\"DisplayName\":\"String content\",\"GuideChannelId\":\"1627aea5-8e0a-4371-9022-9b504344e724\",\"LogicalChannelNumber\":2147483647,\"Sequence\":2147483647,\"Version\":2147483647,\"VisibleInGuide\":true}";
      arguments += "}";

      Json::Value response;
      int retval = ForTheRecordJSONRPC("ForTheRecord/Control/TuneLiveStream", arguments, response);

      if (retval != E_FAILED)
      {
          if (response.type() == Json::objectValue)
          {
            //printValueTree(response);
            g_current_livestream = response["LiveStream"];
            stream = g_current_livestream["TimeshiftFile"].asString();
            //stream = g_current_livestream["RtspUrl"].asString();
            XBMC->Log(LOG_DEBUG, "Tuned live stream: %s\n", stream.c_str());
          }
      }
      return retval;
    }


    int StopLiveStream()
    {
      if(!g_current_livestream.empty())
      {
        Json::StyledWriter writer;
        std::string arguments = writer.write(g_current_livestream);

        Json::Value response;
        int retval = ForTheRecordJSONRPC("ForTheRecord/Control/StopLiveStream", arguments, response);

        if (retval != E_FAILED)
        {
          printValueTree(response);
        }
        g_current_livestream.clear();

        return retval;
      } else
        return E_FAILED;
    }


    bool KeepLiveStreamAlive()
    {
      //Example request:
      //{"CardId":"String content","Channel":{"BroadcastStart":"String content","BroadcastStop":"String content","ChannelId":"1627aea5-8e0a-4371-9022-9b504344e724","ChannelType":0,"DefaultPostRecordSeconds":2147483647,"DefaultPreRecordSeconds":2147483647,"DisplayName":"String content","GuideChannelId":"1627aea5-8e0a-4371-9022-9b504344e724","LogicalChannelNumber":2147483647,"Sequence":2147483647,"Version":2147483647,"VisibleInGuide":true},"RecorderTunerId":"1627aea5-8e0a-4371-9022-9b504344e724","RtspUrl":"String content","StreamLastAliveTime":"\/Date(928142400000+0200)\/","StreamStartedTime":"\/Date(928142400000+0200)\/","TimeshiftFile":"String content"}
      //Example response:
      //true
      if(!g_current_livestream.empty())
      {
        Json::StyledWriter writer;
        std::string arguments = writer.write(g_current_livestream);

        Json::Value response;
        int retval = ForTheRecordJSONRPC("ForTheRecord/Control/KeepLiveStreamAlive", arguments, response);

        if (retval != E_FAILED)
        {
          //if (response == "true")
          //{
            return true;
          //}
        }
      }
      
      return false;
    }

    int GetEPGData(std::string guidechannel_id, struct tm epg_start, struct tm epg_end, Json::Value& response)
    {
      if ( guidechannel_id.length() > 0 )
      {
        char command[256];
        
        //Format: ForTheRecord/Guide/Programs/{guideChannelId}/{lowerTime}/{upperTime}
        snprintf(command, 256, "ForTheRecord/Guide/Programs/%s/%i-%02i-%02i/%i-%02i-%02i", 
                 guidechannel_id.c_str(),
                 epg_start.tm_year + 1900, epg_start.tm_mon + 1, epg_start.tm_mday,
                 epg_end.tm_year + 1900, epg_end.tm_mon + 1, epg_end.tm_mday);

        int retval = ForTheRecordJSONRPC(command, "", response);

        return retval;
      }

      return E_FAILED;
    }

    int GetRecordingsForTitle(std::string title, Json::Value& response)
    {
      int retval = E_FAILED;
      CURL *curl;
      
      XBMC->Log(LOG_DEBUG, "GetRecordingsForTitle");

      curl = curl_easy_init();
      
      if(curl)
      {
        std::string command = "ForTheRecord/Control/RecordingsForProgramTitle/Television/";
        char* pch = curl_easy_escape(curl, title.c_str(), 0);
        command += pch;
        curl_free(pch);
        XBMC->Log(LOG_DEBUG, "GetRecordingsForTitle - URL: %s\n", command.c_str());

        retval = ForTheRecord::ForTheRecordJSONRPC(command, "?includeNonExisting=false", response);

        curl_easy_cleanup(curl);
      }
      return retval;
    }

    time_t WCFDateToTimeT(std::string wcfdate, int& offset)
    {
      time_t ticks;
      char offsetc;
      int offsetv;

      if (wcfdate.empty())
      {
        return 0;
      }

      //WCF compatible format "/Date(1290896700000+0100)/" => 2010-11-27 23:25:00
      ticks = atoi(wcfdate.substr(6, 10).c_str()); //only take the first 10 chars (fits in a 32-bit time_t value)
      offsetc = wcfdate[19]; // + or -
      offsetv = atoi(wcfdate.substr(20, 4).c_str());

      offset = (offsetc == '+' ? offsetv : -offsetv);

      return ticks;
    }
}

   
    //TODO: implement all functionality for a XBMC PVR client
    // Misc:
    //------
    // -GetDriveSpace
    //   Return the Total and Free Drive space on the PVR Backend
    // -GetBackendTime
    //   The time at the PVR Backend side
    //
    // EPG
    //-----
    // -RequestEPGForChannel
    //
    // Channels
    // -GetNumChannels
    // -RequestChannelList
    // -DeleteChannel (optional)
    // -RenameChannel (optional)
    // -MoveChannel (optional)
    //
    // Recordings
    //------------
    // -GetNumRecordings
    // -RequestRecordingsList
    // -DeleteRecording
    // -RenameRecording
    // -Cutmark functions  (optional)
    // Playback:
    // -OpenRecordedStream
    // -CloseRecordedStream
    // -ReadRecordedStream
    // -PauseRecordedStream
    //
    // Timers (schedules)
    //--------------------
    // -GetNumTimers 
    // -RequestTimerList
    // -AddTimer
    // -DeleteTimer
    // -RenameTimer
    // -UpdateTimer
    //
    // Live TV/Radio
    // -OpenLiveStream
    // -CloseLiveStream
    // -ReadLiveStream (from TS buffer file)
    // -PauseLiveStream
    // -GetCurrentClientChannel
    // -SwitchChannel
    // -SignalQuality (optional)
    // -GetLiveStreamURL (for RTSP based streaming)

