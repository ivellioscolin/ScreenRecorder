#ifndef _POSTPROCESSOR_H_
#define _POSTPROCESSOR_H_

#include "CommonTypes.h"
#include "bmp.h"

class POSTPROCESSOR
{
public:
    POSTPROCESSOR();
    ~POSTPROCESSOR();
    void Init(DX_RESOURCES* Data);
    void CleanRefs();
    BOOL ProcessCapture(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys);
    BOOL ProcessEncoding();

private:

    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_DeviceContext;
};

#endif //_POSTPROCESSOR_H_
