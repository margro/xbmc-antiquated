// Visualisation.h: interface for the CVisualisation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <xtl.h>

struct Visualisation
{
public:
	void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice);
	void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample);
	void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
	void (__cdecl* Render) ();
	void (__cdecl* Stop)();
};

class CVisualisation
{
public:
	CVisualisation(struct Visualisation* pVisz);
	virtual ~CVisualisation();

	// Things that MUST be supplied by the child classes
	void Create();
	void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample);
	void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
	void Render();
	void Stop();
private:
	struct Visualisation* m_pVisz;
};


#endif // !defined(AFX_Visualisation_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
