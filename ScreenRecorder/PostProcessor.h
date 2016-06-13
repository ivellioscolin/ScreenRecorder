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
    BOOL ProcessCapture(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys);
    BOOL StartEncoding(void);
    void StopEncoding(void);
    BOOL ProcessEncoding(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys);

private:

    UINT m_InstanceID;
    DWORD m_Width;
    DWORD m_Height;
    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_DeviceContext;
    IMFSinkWriter* m_pSinkWriter;
    DWORD m_StreamIndex;
    BOOL m_MFStarted;
    BOOL m_MFReady;
	IMFMediaBuffer *m_pBuffer;
};

#endif //_POSTPROCESSOR_H_
