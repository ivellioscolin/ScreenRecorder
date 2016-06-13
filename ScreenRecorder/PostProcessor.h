#ifndef _POSTPROCESSOR_H_
#define _POSTPROCESSOR_H_

#include "CommonTypes.h"
#include "bmp.h"
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

class POSTPROCESSOR
{
public:
    //POSTPROCESSOR(UINT instance);
    POSTPROCESSOR();
    ~POSTPROCESSOR();
    void Init(UINT instance, DX_RESOURCES* Data, RECT* pDeskSize);
    void CleanRefs();
	BOOL IsCaptureEnabled() { return m_CaptureOn; }
	void EnableCapture() { m_CaptureOn = TRUE; }
	void DisableCapture() { m_CaptureOn = FALSE; }
	BOOL IsRecordingEnabled() { return m_RecordOn; }
	void EnableRecording() { m_RecordOn = TRUE; }
	void DisableRecording() { m_RecordOn = FALSE; }
    BOOL ProcessCapture(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys);
    BOOL StartEncoding(void);
    void StopEncoding(void);
    BOOL ProcessEncoding(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys);
	double GetEncodingFPS(void);

private:

	BOOL IsMFStarted(void) { return m_MFStarted; }
	BOOL IsMFReady(void) { return m_MFReady; }

	BOOL m_CaptureOn;
	BOOL m_RecordOn;
    UINT m_InstanceID;
    DWORD m_Width;
    DWORD m_Height;
	QWORD m_FrameCount;
	LONGLONG m_SampleTime;
	LARGE_INTEGER m_LastTime;
    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_DeviceContext;
    IMFSinkWriter* m_pSinkWriter;
    DWORD m_StreamIndex;
    BOOL m_MFStarted;
    BOOL m_MFReady;
	IMFMediaBuffer *m_pBuffer;
};

#endif //_POSTPROCESSOR_H_
