#include <stdio.h>
#include "PostProcessor.h"

using namespace DirectX;

POSTPROCESSOR::POSTPROCESSOR() : m_Device(nullptr),
                                 m_DeviceContext(nullptr)
{
}

POSTPROCESSOR::~POSTPROCESSOR()
{
    CleanRefs();
}

void POSTPROCESSOR::Init(DX_RESOURCES* Data)
{
    m_Device = Data->Device;
    m_DeviceContext = Data->Context;
    m_Device->AddRef();
    m_DeviceContext->AddRef();
}

void POSTPROCESSOR::CleanRefs()
{
    if (m_DeviceContext)
    {
        m_DeviceContext->Release();
        m_DeviceContext = nullptr;
    }

    if (m_Device)
    {
        m_Device->Release();
        m_Device = nullptr;
    }
}

BOOL POSTPROCESSOR::ProcessCapture(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys)
{
    BOOL isSuccess = TRUE;
    BYTE* pBits = NULL;
    DWORD pitch = 0;

    D3D11_TEXTURE2D_DESC Desc;
    Data->Frame->GetDesc(&Desc);

    IDXGISurface2* pCaptureSurf = nullptr;
    ID3D11Texture2D* pCaptureTex = nullptr;

    // Map
    if (pMappedSys)
    {
        pBits = pMappedSys->pBits;
        pitch = pMappedSys->Pitch;
    }
    else
    {
        D3D11_TEXTURE2D_DESC DescCapture;
        DescCapture = Desc;
        DescCapture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        DescCapture.Usage = D3D11_USAGE_STAGING;
        DescCapture.MiscFlags = 0;
        DescCapture.BindFlags = 0;

        HRESULT hr = m_Device->CreateTexture2D(&DescCapture, nullptr, &pCaptureTex);

        if (!FAILED(hr))
        {
            m_DeviceContext->CopyResource(pCaptureTex, SharedSurf);
            pCaptureTex->QueryInterface(__uuidof(IDXGISurface2), (reinterpret_cast<void**>(&pCaptureSurf)));

            DXGI_MAPPED_RECT rawDesktop;
            RtlZeroMemory(&rawDesktop, sizeof(DXGI_MAPPED_RECT));
            pCaptureSurf->Map(&rawDesktop, DXGI_MAP_READ);

            pBits = rawDesktop.pBits;
            pitch = rawDesktop.Pitch;
        }
        else
        {
            isSuccess = FALSE;
        }
    }

    // Write
    if (isSuccess == TRUE)
    {
        SYSTEMTIME curTime = { 0 };
        GetSystemTime(&curTime);

        BITMAP_FILE outBmp;
        RtlZeroMemory(&outBmp, sizeof(BITMAP_FILE));
        outBmp.bitmapheader.bfType = 0x4D42;
        outBmp.bitmapheader.bfSize = sizeof(outBmp.bitmapheader)
            + sizeof(outBmp.bitmapinfoheader)
            + Desc.Width * Desc.Height * 4;
        outBmp.bitmapheader.bfOffBits = sizeof(outBmp.bitmapheader)
            + sizeof(outBmp.bitmapinfoheader);

        outBmp.bitmapinfoheader.biSize = sizeof(BITMAPINFOHEADER);
        outBmp.bitmapinfoheader.biWidth = Desc.Width;
        outBmp.bitmapinfoheader.biHeight = -((int)Desc.Height);// RGB raw data is inversted in BMP
        outBmp.bitmapinfoheader.biPlanes = 1;
        outBmp.bitmapinfoheader.biBitCount = 32;//Should always be DXGI_FORMAT_B8G8R8A8_UNORM
        outBmp.bitmapinfoheader.biCompression = 0;
        outBmp.bitmapinfoheader.biSizeImage = Desc.Width * Desc.Height * 4;
        outBmp.bitmapinfoheader.biXPelsPerMeter = 0xED8;
        outBmp.bitmapinfoheader.biYPelsPerMeter = 0xED8;
        outBmp.bitmapinfoheader.biClrUsed = 0;
        outBmp.bitmapinfoheader.biClrImportant = 0;

        wchar_t buf[MAX_PATH * 2];
        ZeroMemory(buf, MAX_PATH * 2 * sizeof(TCHAR));
        GetCurrentDirectory(MAX_PATH * 2, buf);

        swprintf_s(buf, MAX_PATH * 2, L"%s\\Desktop-%04d%02d%02d-%02d-%02d-%02d-%03d.bmp",
            buf, curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour, curTime.wMinute, curTime.wSecond, curTime.wMilliseconds);

        FILE *pBmpFile = NULL;
        if (_wfopen_s(&pBmpFile, buf, L"wb") == 0)
        {
            // Write header
            fwrite((void*)&outBmp, sizeof(outBmp.bitmapheader) + sizeof(outBmp.bitmapinfoheader), 1, pBmpFile);

            // Write raw data
            if (Desc.Width * 4 == pitch) // Write in burst
            {
                fwrite(pBits, Desc.Width * Desc.Height * 4, 1, pBmpFile);
            }
            else // Write line by line
            {
                for (unsigned int line = 0; line < Desc.Height; line++)
                {
                    fwrite(pBits + line * pitch, Desc.Width * 4, 1, pBmpFile);
                }
            }
            fclose(pBmpFile);
        }
        else
        {
            isSuccess = FALSE;
        }
    }

    // Unmap
    if (pMappedSys)
    {
        // Unmap in DuplMgr
    }
    else
    {
        pCaptureSurf->Unmap();

        pCaptureSurf->Release();
        pCaptureSurf = nullptr;



        if (pCaptureTex)
        {
            pCaptureTex->Release();
        }
    }

    return isSuccess;
}

BOOL POSTPROCESSOR::ProcessEncoding()
{
    return TRUE;
}