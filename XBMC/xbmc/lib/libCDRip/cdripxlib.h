/*
** Copyright (C) 2003 WiSo (www.wisonauts.org)
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef CDRIPX_UTIL_INCLUDED
#define CDRIPX_UTIL_INCLUDED


#include <xtl.h>
#include <stdio.h>

#ifdef _WITHENC
#include "cdencoder.h"
#include <vorbis\vorbisenc.h>
#endif

//-----------------------------------------------------------------------------
// Name: class CCDRipX
// Desc: CDDA Player
//
// Init()
//   initialize and reads TOC
// GetNumTocEntries()
//   returns the Number of Tracks
// GetTrackInfo(6)
//   returns track 6 in minutes,seconds and frames
// playTrack(7)
//   plays track 7
// Process( &pTimeplayed  )
//   processes the audio data. need to be called in a loop. calling DirectSoundDoWork() 
//   afterwards is mandatory. pTimeplayed returns the time played in seconds
// Pause(DSSTREAMPAUSE_PAUSE)
//   paused the stream. DSSTREAMPAUSE_RESUME resumes it
// Stop()
//   stops playing and clears memory (hopefully)
//
//
// Encoder (only included when _WITHENC is defined in CDPlayX and CDRipXlib
//
// RipToOgg(6,"Track06",nPercent,nPeakValue,nJitterErrors,nJitterPos)
//    rips Track 6, encodes it do ogg and save it as Track06
//    nPercent: shows how many is done
//
// See sample app CDPlayX for example (www.wisonauts.org)
//-----------------------------------------------------------------------------

// Define the maximum amount of packets we will ever submit to the renderer
const DWORD WAVSTRM_PACKET_COUNT = 2;

#define TRACKSPERSEC 75
#define CB_CDDASECTOR 2352
#define TRACK_LIST 100

#define CDRIPX_OK		0
#define CDRIPX_ERR		1
#define CDRIPX_DONE		2


struct cdtoc {
	int	min;
	int	sec;
	int	frame;
	bool bIsAudio;
};

class CCDRipX
{
protected:
	
	LONG					nNumBytesRead;
	LONG					nTotalBytes;
	IDirectSoundStream*     m_pDestXMO;
	WAVEFORMATEX            m_wfxSourceFormat; 
	BYTE*	                m_pvSourceBuffer[WAVSTRM_PACKET_COUNT]; 
	HRESULT					hr;
	HRESULT                 m_hrOpenResult; 
	DWORD                   m_adwStatus[2];
	BYTE*					pbtStream;
	LONG					nBufferSize;
	BOOL					m_init;
	int						m_nNumTracks;

	HRESULT					pGetTrackInfo();
	unsigned long			CalculateDiscID();
	int						Ripperinit(int ntrack);
	int						Playerinit();
	int						CreateStream();
	HRESULT					ProcessSound( DWORD dwPacketIndex );
	BOOL					FindFreePacket( DWORD* pdwPacketIndex );
	cdtoc					CDCon[TRACK_LIST];
	
#ifdef _WITHENC
	BOOL					rip_in_progress;
	CCDEnc					*enc;
#endif

public:
	cdtoc					oCDCon[TRACK_LIST];
	bool					IsAudioTrack( int nTrack );

	HRESULT					Init();
	int						GetNumTocEntries();
	cdtoc					GetTrackInfo(int ntrack);
	HRESULT					playTrack(int nTrack);
	int						Process( DWORD* pTimeplayed = NULL );
	void					Pause(DWORD dwPause);
	void					Stop();

#ifdef _WITHENC
	int						RipToOgg(	int				ntrack,
										char*			file,
										int&			nPercent,
										int&			nPeakValue,
										int&			nJitterErrors,
										int&			nJitterPos );
#endif
		
	CCDRipX();
	~CCDRipX();

};
void DPf(const char* pzFormat, ...);


#endif