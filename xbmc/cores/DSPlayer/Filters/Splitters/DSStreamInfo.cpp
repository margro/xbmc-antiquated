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
 */

#include "DSStreamInfo.h"

#include "DVDPlayer/DVDCodecs/DVDCodecs.h"
#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DSGuidHelper.h"
#include "MMReg.h"

#include "moreuuids.h"
extern "C"
{
  #include "libavformat/avformat.h"
}
CDSStreamInfo::CDSStreamInfo()                                                     { extradata = NULL; Clear(); }
CDSStreamInfo::CDSStreamInfo(const CDSStreamInfo &right, bool withextradata )     { extradata = NULL; Clear(); Assign(right, withextradata); }
CDSStreamInfo::CDSStreamInfo(const CDemuxStream &right, bool withextradata )       { extradata = NULL; Clear(); Assign(right, withextradata); }

CDSStreamInfo::~CDSStreamInfo()
{
  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;
}


void CDSStreamInfo::Clear()
{
  
  codec = CODEC_ID_NONE;
  type = STREAM_NONE;
  software = false;

  if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;

  fpsscale = 0;
  fpsrate  = 0;
  height   = 0;
  width    = 0;
  aspect   = 0.0;
  vfr      = false;
  stills   = false;

  channels   = 0;
  samplerate = 0;
  blockalign = 0;
  bitrate    = 0;
  bitspersample = 0;

  identifier = 0;
}

bool CDSStreamInfo::Equal(const CDSStreamInfo& right, bool withextradata)
{
  if( codec != right.codec
  ||  type != right.type ) return false;

  if( withextradata )
  {
    if( extrasize != right.extrasize ) return false;
    if( extrasize )
    {
      if( memcmp(extradata, right.extradata, extrasize) != 0 ) return false;
    }
  }

  // VIDEO
  if( fpsscale != right.fpsscale
  ||  fpsrate  != right.fpsrate
  ||  height   != right.height
  ||  width    != right.width
  ||  stills   != right.stills
  ||  vfr      != right.vfr) return false;

  // AUDIO
  if( channels      != right.channels
  ||  samplerate    != right.samplerate
  ||  blockalign    != right.blockalign
  ||  bitrate       != right.bitrate
  ||  bitspersample != right.bitspersample ) return false;

  // SUBTITLE
  if( identifier != right.identifier ) return false;

  return true;
}

bool CDSStreamInfo::Equal(const CDemuxStream& right, bool withextradata)
{
  CDSStreamInfo info;
  info.Assign(right, withextradata);
  return Equal(info, withextradata);
}


// ASSIGNMENT
void CDSStreamInfo::Assign(const CDSStreamInfo& right, bool withextradata)
{
  codec = right.codec;
  type = right.type;

  if( extradata && extrasize ) free(extradata);

  if( withextradata && right.extrasize )
  {
    extrasize = right.extrasize;
    extradata = malloc(extrasize);
    memcpy(extradata, right.extradata, extrasize);
  }
  else
  {
    extrasize = 0;
    extradata = 0;
  }

  // VIDEO
  fpsscale = right.fpsscale;
  fpsrate  = right.fpsrate;
  height   = right.height;
  width    = right.width;
  aspect   = right.aspect;
  stills   = right.stills;

  // AUDIO
  channels      = right.channels;
  samplerate    = right.samplerate;
  blockalign    = right.blockalign;
  bitrate       = right.bitrate;
  bitspersample = right.bitspersample;

  // SUBTITLE
  identifier = right.identifier;
}

void CDSStreamInfo::Assign(const CDemuxStream& right, bool withextradata)
{
  Clear();
  mtype.InitMediaType();
  codec = right.codec;
  type = right.type;

  if( withextradata && right.ExtraSize )
  {
    extrasize = right.ExtraSize;
    extradata = malloc(extrasize);
    memcpy(extradata, right.ExtraData, extrasize);
  }
  /****************/
  /* AUDIO STREAM */
  /****************/
  if( right.type == STREAM_AUDIO )
  {
    const CDemuxStreamAudio *stream = static_cast<const CDemuxStreamAudio*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);

    
    
    
    //mtype.subtype = FOURCCMap(avstream->codec->codec_tag);
    //TODO convert audio codecid into wave format ex: ac3 is WAVE_FORMAT_DOLBY_AC3 0x2000
    
    
    
    
    mtype = g_GuidHelper.initAudioType(avstream->codec->codec_id);
    
    if (mtype.subtype == GUID_NULL)
      mtype.subtype = FOURCCMap(avstream->codec->codec_tag);
    WAVEFORMATEX* wvfmt = (WAVEFORMATEX*)mtype.AllocFormatBuffer(sizeof(WAVEFORMATEX)+ avstream->codec->extradata_size);
    
    wvfmt->wFormatTag = mtype.subtype.Data1;//avstream->codec->codec_tag;
    wvfmt->nChannels = avstream->codec->channels;
    wvfmt->nSamplesPerSec= avstream->codec->sample_rate;
    
    wvfmt->wBitsPerSample= avstream->codec->bits_per_coded_sample;
    wvfmt->nBlockAlign = (WORD)((wvfmt->nChannels * wvfmt->wBitsPerSample) / 8);// avstream->codec->block_align ? avstream->codec->block_align : 1;
    wvfmt->nAvgBytesPerSec= wvfmt->nSamplesPerSec * wvfmt->nBlockAlign;
    //mtype.SetSampleSize(256000)
    if (avstream->codec->extradata > 0)
    {
      wvfmt->cbSize = avstream->codec->extradata_size;
      memcpy(wvfmt + 1, avstream->codec->extradata, avstream->codec->extradata_size);
    }
    mtype.SetSampleSize(channels * (bitrate / 8));

    
    //wvfmt->wFormatTag
    //MPEGLAYER3WAVEFORMAT *
    
    channels      = stream->iChannels;
    samplerate    = stream->iSampleRate;
    blockalign    = stream->iBlockAlign;
    bitrate       = stream->iBitRate;
    bitspersample = stream->iBitsPerSample;
  }
  /****************/
  /* VIDEO STREAM */
  /****************/
  else if(  right.type == STREAM_VIDEO )
  {
    const CDemuxStreamVideo *stream = static_cast<const CDemuxStreamVideo*>(&right);
    AVStream* avstream = static_cast<AVStream*>(stream->pPrivate);
    
    /* FormatType */
    /* VideoInfo */
    /* VideoInfo2 */
    /* MPEGVideo */
    /* MPEG2Video */
    mtype = g_GuidHelper.initVideoType(avstream->codec->codec_id);
    
    //Most used type
    if (1)//for (const FOURCC *fcc=fccs;*fcc;fcc++)
    {
      if (mtype.subtype == FOURCCMap(FOURCC_AVC1))
      {
        MPEG2VIDEOINFO *mpeginfo = (MPEG2VIDEOINFO*)mtype.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + avstream->codec->extradata_size - 7);
        //VideoFps = 10000000.0 / AvgTimePerFrame;
        mpeginfo->hdr.AvgTimePerFrame = (REFERENCE_TIME)(10000000 / (stream->iFpsRate / stream->iFpsScale));
        //ff_h264_decode_seq_parameter_set from libavcodec is actually the best function to get those info
        mpeginfo->hdr.dwBitErrorRate = 0;
        
        mpeginfo->hdr.dwBitRate = avstream->codec->bit_rate;
        //mpeginfo->hdr.dwInterlaceFlags
        RECT empty_tagrect = {0,0,0,0};
        mpeginfo->hdr.rcSource = empty_tagrect;
        mpeginfo->hdr.rcTarget = empty_tagrect;
        
        mpeginfo->hdr.dwPictAspectRatioX = (DWORD)stream->iWidth;
        mpeginfo->hdr.dwPictAspectRatioY = (DWORD)stream->iHeight;
        mpeginfo->hdr.bmiHeader.biSize = sizeof(mpeginfo->hdr.bmiHeader);
        mpeginfo->hdr.bmiHeader.biWidth= stream->iWidth;
        mpeginfo->hdr.bmiHeader.biHeight= stream->iHeight;
        
        mpeginfo->hdr.bmiHeader.biBitCount= 24;
        mpeginfo->hdr.bmiHeader.biSizeImage = stream->iWidth * stream->iHeight * avstream->codec->bits_per_coded_sample / 8;
        mpeginfo->hdr.bmiHeader.biCompression = mtype.subtype.Data1;
        mpeginfo->hdr.bmiHeader.biPlanes = 1;
        mpeginfo->hdr.bmiHeader.biClrUsed = 0;
        mpeginfo->hdr.bmiHeader.biClrImportant = 0;
        mpeginfo->hdr.bmiHeader.biYPelsPerMeter = 0;
        mpeginfo->hdr.bmiHeader.biXPelsPerMeter = 0;
        
        //mtype.pbFormat =
        //from ffmpeg full info
        //ex: 01 64 00 1f ff e1 00 19 67 64 00 1f ac 34 e4 01 40 16 ec 04 40 00 00 fa 40 00 2e e0 23 c6 0c 64 80 01 00 05 68 ee b2 c8 b0
        //from directshow
        //ex: 00 19 67 64 00 1f ac 34  e4 01 40 16 ec 04 40 00 00 fa 40 00 2e e0 23 c6  0c 64 80 00 05 68 ee b2 c8 b0 
        //based on those test the extradata from ffmpeg required for the mpeginfo header is starting at index 7
        std::vector<BYTE> extravect;
        extravect.resize(avstream->codec->extradata_size);
        memcpy(&extravect.at(0), avstream->codec->extradata, avstream->codec->extradata_size);

        mpeginfo->dwStartTimeCode = 0;// is there any case where it wouldnt be 0?????
        mpeginfo->dwProfile = avstream->codec->profile;
        mpeginfo->dwLevel = avstream->codec->level;
        mpeginfo->dwFlags = (extravect.at(4) & 3) + 1;
        //drop before 6 and the byte 33(no clue about why i have to remove this one)
        extravect.erase(extravect.begin() + 33);
        extravect.erase(extravect.begin(),extravect.begin() + 6);        
        
        mpeginfo->cbSequenceHeader = extravect.size();
        memcpy(mpeginfo->dwSequenceHeader, &extravect.at(0) , extravect.size());
        return;
      }
  
    }
    
    //typedef struct SPS in h264.h from ffmpeg source might have all info we need for the MPEG2VIDEOINFO
    if (mtype.formattype == GUID_NULL)
    {
      mtype.formattype = FORMAT_VideoInfo;
    }
  
    mtype.bTemporalCompression = 0;
    
    mtype.bFixedSizeSamples = stream->bVFR ? 0 : 1; //hummm is it the right value???

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)mtype.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
  
  
  //Still to be verified in real time but the mathematics make sense
  // 23.97 fps = 417166;
  //so fps = 10000000.0 / AvgTimePerFrame
  //or 
  //AvgTimePerFrame =  10000000 / fps 
  pvi->AvgTimePerFrame = (REFERENCE_TIME)(10000000 / (stream->iFpsRate / stream->iFpsScale));
  
  pvi->dwBitErrorRate = 0;
  
  pvi->dwBitRate = avstream->codec->bit_rate;
  pvi->bmiHeader.biSize = sizeof(pvi->bmiHeader);
  pvi->bmiHeader.biWidth= stream->iWidth;
  pvi->bmiHeader.biHeight= stream->iHeight;
  
  pvi->bmiHeader.biBitCount= avstream->codec->bits_per_coded_sample;
  pvi->bmiHeader.biSizeImage = stream->iWidth * stream->iHeight * avstream->codec->bits_per_coded_sample / 8;
  pvi->bmiHeader.biCompression= stream->codec_fourcc;
  //TOFIX The bitplanes is depending on the subtype
  pvi->bmiHeader.biPlanes = 1;
  pvi->bmiHeader.biClrUsed = 0;
  pvi->bmiHeader.biClrImportant = 0;
  pvi->bmiHeader.biYPelsPerMeter = 0;
  pvi->bmiHeader.biXPelsPerMeter = 0;
  
  //This still need to be verified only tested xvid
  mtype.subtype = MEDIATYPE_Video;
  mtype.subtype.Data1 = pvi->bmiHeader.biCompression;

  mtype.SetFormat((PBYTE)pvi,sizeof(VIDEOINFOHEADER));
  //Not sure if its really working but in case the other way dont work will try this one
  
 
      
  
    
    fpsscale  = stream->iFpsScale;
    fpsrate   = stream->iFpsRate;
    height    = stream->iHeight;
    width     = stream->iWidth;
    aspect    = stream->fAspect;
    vfr       = stream->bVFR;
  }
  else if(  right.type == STREAM_SUBTITLE )
  {
    const CDemuxStreamSubtitle *stream = static_cast<const CDemuxStreamSubtitle*>(&right);
    identifier = stream->identifier;
  }
}
