/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include <mmreg.h>
#include "XBMCFFmpegSplitter.h"


#include "DVDPlayer/DVDDemuxers/DVDDemux.h"
#include "DVDPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "DVDPlayer/DVDClock.h"

//
// CXBMCFFmpegSplitter
//

CXBMCFFmpegSplitter::CXBMCFFmpegSplitter(LPUNKNOWN pUnk, HRESULT* phr)
  : CBaseSplitterFilter(NAME("CXBMCFFmpegSplitter"), pUnk, phr, __uuidof(this))
  , m_timeformat(TIME_FORMAT_MEDIA_TIME)
{
  m_pDemuxer = NULL;
  m_pInputStream = NULL;
}

CXBMCFFmpegSplitter::~CXBMCFFmpegSplitter()
{
}

STDMETHODIMP CXBMCFFmpegSplitter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
  CheckPointer(ppv, E_POINTER);

  *ppv = NULL;

  return 
    __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CXBMCFFmpegSplitter::CreateOutputs(IAsyncReader* pAsyncReader)
{
  CheckPointer(pAsyncReader, E_POINTER);

  HRESULT hr = E_FAIL;
  m_filenameA = DShowUtil::WToA(CStdStringW(m_fn));
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, m_filenameA, "");
  if (!m_pInputStream)
  {
    CLog::Log(LOGERROR, "%s: Error creating stream for %s", __FUNCTION__, m_filenameA.c_str());
    return E_FAIL;
  }
  m_pInputStream->Open(m_filenameA.c_str(),"");
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);
  try
  {
    m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
    if(!m_pDemuxer)
    {
      delete m_pInputStream;
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return E_FAIL;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (m_pDemuxer)
      delete m_pDemuxer;
    delete m_pInputStream;
    return E_FAIL;
  }

  m_rtNewStart = m_rtCurrent = 0;
  m_rtNewStop = m_rtStop = m_rtDuration = (DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) * 10);//directshow basetime is 100 nanosec
  //m_rtNewStop = m_rtStop = m_rtDuration = m_pFile->GetTotalTime();

  bool fHasIndex = false;
  for (int iStream=0; iStream < m_pDemuxer->GetNrOfStreams(); iStream++)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
    CDSStreamInfo hint(*pStream, true);
    CMediaType mt;
    vector<CMediaType> mts;
    mts.push_back(hint.mtype);
    std::string tname;
    pStream->GetStreamName(tname);
    CStdStringW tnameW = DShowUtil::AToW(tname);
    auto_ptr<CBaseSplitterOutputPin> pPinOut(DNew CXBMCFFmpegOutputPin(mts, tnameW.c_str(), this, this, &hr));
    
    AddOutputPin((DWORD)iStream, pPinOut);
  }

  return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CXBMCFFmpegSplitter::DemuxInit()
{
  if(!m_pDemuxer) 
    return(false);

  // reindex if needed

#if 0
  bool fReIndex = false;

  for(int i = 0; i < (int)m_pFile->m_avih.dwStreams && !fReIndex; i++)
  {
    if(m_pFile->m_strms[i]->cs.size() == 0 && GetOutputPin(i)) 
      fReIndex = true;
  }

  if(fReIndex)
  {
    m_pFile->EmptyIndex();

    m_fAbort = false;
    m_nOpenProgress = 0;

    m_rtDuration = 0;

    Com::SmartAutoVectorPtr<UINT64> pSize;
    pSize.Allocate(m_pFile->m_avih.dwStreams);
    memset((UINT64*)pSize, 0, sizeof(UINT64)*m_pFile->m_avih.dwStreams);
    m_pFile->Seek(0);
        ReIndex(m_pFile->GetLength(), pSize);

    if(m_fAbort) m_pFile->EmptyIndex();

    m_fAbort = false;
  }
#endif
  m_nOpenProgress = 100;
  return(true);
}

void CXBMCFFmpegSplitter::DemuxSeek(REFERENCE_TIME rt)
{
  int rt_sec;
  int64_t rt_dvd = rt / 10;
  rt_sec = DVD_TIME_TO_MSEC(rt_dvd);
  double start = DVD_NOPTS_VALUE;
  if (m_pDemuxer && m_pDemuxer->SeekTime(rt_sec, false, &start))
    CLog::Log(LOGERROR,"%s failed to seek",__FUNCTION__);
}

bool CXBMCFFmpegSplitter::DemuxLoop()
{
  HRESULT hr = S_OK;

  int nTracks = m_pDemuxer->GetNrOfStreams();//(int)m_pFile->m_strms.size();

  vector<BOOL> fDiscontinuity;
  fDiscontinuity.resize(nTracks);
  memset(&fDiscontinuity[0], 0, nTracks*sizeof(bool));

  do//while(SUCCEEDED(hr) && !CheckRequest(NULL))
  {
    DemuxPacket* pPacket = NULL;
    CDemuxStream *pStream = NULL;
    ReadPacket(pPacket, pStream);
    //ReadPacket(pPacket, pStream);
    if (pPacket && !pStream)
    {
      /* probably a empty DsPacket, just free it and move on */
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      continue;
    }
    std::auto_ptr<Packet> p(new Packet());
    p->TrackNumber = (DWORD)pPacket->iStreamId;
    p->resize(pPacket->iSize);
    memcpy(&p->at(0), pPacket->pData,pPacket->iSize);

    
    if (pPacket->dts != DVD_NOPTS_VALUE)
      p->rtStart = (pPacket->dts * 10);
    else
      p->rtStart = m_rtCurrent;
    p->bSyncPoint = (pPacket->duration > 0) ? 1 : 0;
    
    p->rtStop = p->rtStart + ((pPacket->duration > 0) ? (pPacket->duration * 10) : 1);
    
    hr = DeliverPacket(p);

  }
  while(SUCCEEDED(hr) && !CheckRequest(NULL));
    
    

#if 0
    int minTrack = nTracks;
    UINT64 minFilePos = _I64_MAX;

    for(int i = 0; i < nTracks; i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();

      DWORD f = m_tFrame[i];
      if(f >= (DWORD)s->cs.size()) 
        continue;

      bool fUrgent = s->IsRawSubtitleStream();

      if(fUrgent || s->cs[f].filepos < minFilePos)
      {
        minTrack = i;
        minFilePos = s->cs[f].filepos;
      }

      if(fUrgent) 
        break;
    }

    if(minTrack == nTracks)
      break;

    DWORD& f = m_tFrame[minTrack];

    do
    {
      CAviFile::strm_t* s = m_pFile->m_strms[minTrack].get();

      m_pFile->Seek(s->cs[f].filepos);

      DWORD size = 0;

      if(s->cs[f].fChunkHdr)
      {
        DWORD id = 0;
        if(S_OK != m_pFile->Read(id) || id == 0 || minTrack != TRACKNUM(id)
        || S_OK != m_pFile->Read(size))
        {
          fDiscontinuity[minTrack] = true;
          break;
        }

        UINT64 expectedsize = -1;
        expectedsize = f < (DWORD)s->cs.size()-1
          ? s->cs[f+1].size - s->cs[f].size
          : s->totalsize - s->cs[f].size;

        if(expectedsize != s->GetChunkSize(size))
        {
          fDiscontinuity[minTrack] = true;
          // ASSERT(0);
          break;
        }
      }
      else
      {
        size = s->cs[f].orgsize;
      }

      auto_ptr<Packet> p(DNew Packet());

      p->TrackNumber = minTrack;
      p->bSyncPoint = (BOOL)s->cs[f].fKeyFrame;
      p->bDiscontinuity = fDiscontinuity[minTrack];
      p->rtStart = s->GetRefTime(f, s->cs[f].size);
      p->rtStop = s->GetRefTime(f+1, f+1 < (DWORD)s->cs.size() ? s->cs[f+1].size : s->totalsize);
      
      p->resize(size);
      if(S_OK != (hr = m_pFile->ByteRead(&p->at(0), p->size()))) 
        return(true); // break;
/*
      DbgLog((LOG_TRACE, 0, _T("%d (%d): %I64d - %I64d, %I64d - %I64d (size = %d)"), 
        minTrack, (int)p->bSyncPoint,
        (p->rtStart)/10000, (p->rtStop)/10000, 
        (p->rtStart-m_rtStart)/10000, (p->rtStop-m_rtStart)/10000,
        size));
*/
      hr = DeliverPacket(p);

      fDiscontinuity[minTrack] = false;
    }
    while(0);

    f++;
  }
#endif
  return(true);
}

bool CXBMCFFmpegSplitter::ReadPacket(DemuxPacket*& DsPacket, CDemuxStream*& stream)
{
  if(m_pDemuxer)
    DsPacket = m_pDemuxer->Read();
  if (DsPacket)
  {
    stream = m_pDemuxer->GetStream(DsPacket->iStreamId);
    if (!stream)
    {
      CLog::Log(LOGERROR, "%s - Error demux DsPacket doesn't belong to a valid stream", __FUNCTION__);
      return false;
    }
    return true;
  }
  
  return false;
}

// IMediaSeeking

STDMETHODIMP CXBMCFFmpegSplitter::GetDuration(LONGLONG* pDuration)
{
  CheckPointer(pDuration, E_POINTER);
  CheckPointer(m_pDemuxer, VFW_E_NOT_CONNECTED);

  if(m_timeformat == TIME_FORMAT_FRAME)
  {
    *pDuration = DVD_MSEC_TO_TIME(m_pDemuxer->GetStreamLength()) * 10;
    return S_OK;
    /*for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();
      if(s->strh.fccType == FCC('vids'))
      {
        *pDuration = s->cs.size();
        return S_OK;
      }
    }*/

    return E_UNEXPECTED;
  }

  return __super::GetDuration(pDuration);
}

//

STDMETHODIMP CXBMCFFmpegSplitter::IsFormatSupported(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  HRESULT hr = __super::IsFormatSupported(pFormat);
  if(S_OK == hr) return hr;
  return *pFormat == TIME_FORMAT_FRAME ? S_OK : S_FALSE;
}

STDMETHODIMP CXBMCFFmpegSplitter::GetTimeFormat(GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  *pFormat = m_timeformat;
  return S_OK;
}

STDMETHODIMP CXBMCFFmpegSplitter::IsUsingTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  return *pFormat == m_timeformat ? S_OK : S_FALSE;
}

STDMETHODIMP CXBMCFFmpegSplitter::SetTimeFormat(const GUID* pFormat)
{
  CheckPointer(pFormat, E_POINTER);
  if(S_OK != IsFormatSupported(pFormat)) return E_FAIL;
  m_timeformat = *pFormat;
  return S_OK;
}

STDMETHODIMP CXBMCFFmpegSplitter::GetStopPosition(LONGLONG* pStop)
{
  CheckPointer(pStop, E_POINTER);
  if(FAILED(__super::GetStopPosition(pStop))) return E_FAIL;
  if(m_timeformat == TIME_FORMAT_MEDIA_TIME) return S_OK;
  LONGLONG rt = *pStop;
  if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, rt, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  return S_OK;
}

STDMETHODIMP CXBMCFFmpegSplitter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
  return E_NOTIMPL;
  CheckPointer(pTarget, E_POINTER);
#if 0
  const GUID& SourceFormat = pSourceFormat ? *pSourceFormat : m_timeformat;
  const GUID& TargetFormat = pTargetFormat ? *pTargetFormat : m_timeformat;
  
  if(TargetFormat == SourceFormat)
  {
    *pTarget = Source; 
    return S_OK;
  }
  else if(TargetFormat == TIME_FORMAT_FRAME && SourceFormat == TIME_FORMAT_MEDIA_TIME)
  {
    for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();
      if(s->strh.fccType == FCC('vids'))
      {
        *pTarget = s->GetFrame(Source);
        return S_OK;
      }
    }
  }
  else if(TargetFormat == TIME_FORMAT_MEDIA_TIME && SourceFormat == TIME_FORMAT_FRAME)
  {
    for(int i = 0; i < (int)m_pFile->m_strms.size(); i++)
    {
      CAviFile::strm_t* s = m_pFile->m_strms[i].get();
      if(s->strh.fccType == FCC('vids'))
      {
        if(Source < 0 || Source >= s->cs.size()) return E_FAIL;
        CAviFile::strm_t::chunk& c = s->cs[(int)Source];
        *pTarget = s->GetRefTime((DWORD)Source, c.size);
        return S_OK;
      }
    }
  }
#endif
  
  return E_FAIL;
}

STDMETHODIMP CXBMCFFmpegSplitter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
  HRESULT hr;
  if(FAILED(hr = __super::GetPositions(pCurrent, pStop)) || m_timeformat != TIME_FORMAT_FRAME)
    return hr;

  if(pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_FRAME, *pCurrent, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;
  if(pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_FRAME, *pStop, &TIME_FORMAT_MEDIA_TIME))) return E_FAIL;

  return S_OK;
}

HRESULT CXBMCFFmpegSplitter::SetPositionsInternal(void* id, LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
  if(m_timeformat != TIME_FORMAT_FRAME)
    return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

  if(!pCurrent && !pStop
  || (dwCurrentFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning 
    && (dwStopFlags&AM_SEEKING_PositioningBitsMask) == AM_SEEKING_NoPositioning)
    return S_OK;

  REFERENCE_TIME 
    rtCurrent = m_rtCurrent,
    rtStop = m_rtStop;

  if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  && FAILED(ConvertTimeFormat(&rtCurrent, &TIME_FORMAT_FRAME, rtCurrent, &TIME_FORMAT_MEDIA_TIME))) 
    return E_FAIL;
  if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
  && FAILED(ConvertTimeFormat(&rtStop, &TIME_FORMAT_FRAME, rtStop, &TIME_FORMAT_MEDIA_TIME)))
    return E_FAIL;

  if(pCurrent)
  switch(dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  {
  case AM_SEEKING_NoPositioning: break;
  case AM_SEEKING_AbsolutePositioning: rtCurrent = *pCurrent; break;
  case AM_SEEKING_RelativePositioning: rtCurrent = rtCurrent + *pCurrent; break;
  case AM_SEEKING_IncrementalPositioning: rtCurrent = rtCurrent + *pCurrent; break;
  }

  if(pStop)
  switch(dwStopFlags&AM_SEEKING_PositioningBitsMask)
  {
  case AM_SEEKING_NoPositioning: break;
  case AM_SEEKING_AbsolutePositioning: rtStop = *pStop; break;
  case AM_SEEKING_RelativePositioning: rtStop += *pStop; break;
  case AM_SEEKING_IncrementalPositioning: rtStop = rtCurrent + *pStop; break;
  }

  if((dwCurrentFlags&AM_SEEKING_PositioningBitsMask)
  && pCurrent)
    if(FAILED(ConvertTimeFormat(pCurrent, &TIME_FORMAT_MEDIA_TIME, rtCurrent, &TIME_FORMAT_FRAME))) return E_FAIL;
  if((dwStopFlags&AM_SEEKING_PositioningBitsMask)
  && pStop)
    if(FAILED(ConvertTimeFormat(pStop, &TIME_FORMAT_MEDIA_TIME, rtStop, &TIME_FORMAT_FRAME))) return E_FAIL;

  return __super::SetPositionsInternal(id, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
}

//
// CXBMCFFmpegSourceFilter
//

CXBMCFFmpegSourceFilter::CXBMCFFmpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
  : CXBMCFFmpegSplitter(pUnk, phr)
{
  m_clsid = __uuidof(this);
  m_pInput.release();
  
}
