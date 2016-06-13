#include <stdio.h>
#include "PostProcessor.h"

using namespace DirectX;
extern LONGLONG* g_SampleTime;
extern LARGE_INTEGER* g_LastTime;

// Format constants
const UINT32 VIDEO_FPS = 30;
const UINT32 VIDEO_BIT_RATE = 800000;
const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_H264;
//const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_HEVC;
const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_ARGB32;

POSTPROCESSOR::POSTPROCESSOR() : m_InstanceID(0xFFFFFFFF),
                                 m_Width(0),
                                 m_Height(0),
                                 m_Device(nullptr),
                                 m_DeviceContext(nullptr),
                                 m_pSinkWriter(nullptr),
                                 m_StreamIndex(0),
                                 m_MFStarted(FALSE),
                                 m_MFReady(FALSE),
								 m_pBuffer(nullptr)

{
    //m_InstanceID = instance;
}

POSTPROCESSOR::~POSTPROCESSOR()
{
    m_InstanceID = 0xFFFFFFFF;
    m_MFStarted = FALSE;
    CleanRefs();
}

void POSTPROCESSOR::Init(UINT instance, DX_RESOURCES* Data, RECT* pDeskSize)
{
    m_InstanceID = instance;
    m_Width = (DWORD)abs(pDeskSize->left - pDeskSize->right);
    m_Height = (DWORD)abs(pDeskSize->bottom - pDeskSize->top);
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
        GetLocalTime(&curTime);

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

        swprintf_s(buf, MAX_PATH * 2, L"%s\\Desktop%d_%04d%02d%02d_%02d%02d%02d-%03d.bmp",
            buf, m_InstanceID,curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour, curTime.wMinute, curTime.wSecond, curTime.wMilliseconds);

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

BOOL POSTPROCESSOR::StartEncoding()
{
    BOOL isSuccess = FALSE;
	g_SampleTime[m_InstanceID] = 0;
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        if (SUCCEEDED(MFStartup(MF_VERSION)))
        {
            m_MFStarted = TRUE;

            SYSTEMTIME curTime = { 0 };
            GetLocalTime(&curTime);

            wchar_t buf[MAX_PATH * 2];
            ZeroMemory(buf, MAX_PATH * 2 * sizeof(TCHAR));
            GetCurrentDirectory(MAX_PATH * 2, buf);

            swprintf_s(buf, MAX_PATH * 2, L"%s\\Desktop%d_%04d%02d%02d_%02d%02d%02d-%03d.mp4",
                buf, m_InstanceID, curTime.wYear, curTime.wMonth, curTime.wDay, curTime.wHour, curTime.wMinute, curTime.wSecond, curTime.wMilliseconds);

            IMFSinkWriter *pSinkWriter = NULL;
            IMFMediaType  *pMediaTypeOut = NULL;
            IMFMediaType  *pMediaTypeIn = NULL;
            DWORD         streamIndex;

            HRESULT hr = S_OK;
            hr = MFCreateSinkWriterFromURL(buf, NULL, NULL, &pSinkWriter);

            // Set the output media type.
            if (SUCCEEDED(hr))
            {
                hr = MFCreateMediaType(&pMediaTypeOut);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, VIDEO_ENCODING_FORMAT);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            }
            if (SUCCEEDED(hr))
            {
                hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, m_Width, m_Height);
            }
            if (SUCCEEDED(hr))
            {
                hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
            }
            if (SUCCEEDED(hr))
            {
                hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            }
            if (SUCCEEDED(hr))
            {
                hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);
            }

            // Set the input media type.
            if (SUCCEEDED(hr))
            {
                hr = MFCreateMediaType(&pMediaTypeIn);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);
            }
            if (SUCCEEDED(hr))
            {
                hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            }
            if (SUCCEEDED(hr))
            {
                hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, m_Width, m_Height);
            }
            if (SUCCEEDED(hr))
            {
                hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
            }
            if (SUCCEEDED(hr))
            {
                hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            }
            if (SUCCEEDED(hr))
            {
                hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL);
            }

            // Tell the sink writer to start accepting data.
            if (SUCCEEDED(hr))
            {
                hr = pSinkWriter->BeginWriting();
            }

			// Create a new memory buffer.
			if (SUCCEEDED(hr))
			{
				hr = MFCreateMemoryBuffer(m_Width * m_Height * 4, &m_pBuffer);
			}

            // Return the pointer to the caller.
            if (SUCCEEDED(hr))
            {
                m_pSinkWriter = pSinkWriter;
                (m_pSinkWriter)->AddRef();
                m_StreamIndex = streamIndex;

                m_MFReady = TRUE;
                isSuccess = TRUE;
            }

            SafeRelease(&pSinkWriter);
            SafeRelease(&pMediaTypeOut);
            SafeRelease(&pMediaTypeIn);
        }
    }
    return isSuccess;
}

void POSTPROCESSOR::StopEncoding()
{
    if(m_MFStarted)
    {
        m_MFStarted = FALSE;
        m_MFReady = FALSE;
		SafeRelease(&m_pBuffer);
        m_pSinkWriter->Finalize();
        SafeRelease(&m_pSinkWriter);
        MFShutdown();
    }
    CoUninitialize();
}

BOOL POSTPROCESSOR::ProcessEncoding(_In_ FRAME_DATA* Data, _In_ ID3D11Texture2D* SharedSurf, DXGI_MAPPED_RECT* pMappedSys)
{
	BOOL isSuccess = TRUE;
	BYTE* pBits = NULL;
	DWORD pitch = 0;

	D3D11_TEXTURE2D_DESC Desc;
	Data->Frame->GetDesc(&Desc);

	IDXGISurface2* pCaptureSurf = nullptr;
	ID3D11Texture2D* pCaptureTex = nullptr;

	HRESULT hr = S_OK;

	LARGE_INTEGER CurrentTime, Frequency, ElapsedMicroseconds;
	LONGLONG Elapsed100Nanoseconds;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&CurrentTime);
	ElapsedMicroseconds.QuadPart = CurrentTime.QuadPart - g_LastTime[m_InstanceID].QuadPart;
	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

	Elapsed100Nanoseconds = ElapsedMicroseconds.QuadPart * 10;

	RtlCopyMemory(&g_LastTime[m_InstanceID], &CurrentTime, sizeof(LARGE_INTEGER));

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

		hr = m_Device->CreateTexture2D(&DescCapture, nullptr, &pCaptureTex);

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
		IMFSample *pSample = NULL;
		BYTE *pData = NULL;
		hr = E_FAIL;

		if(m_pBuffer)
		{
			hr = m_pBuffer->Lock(&pData, NULL, NULL);
		}

		if (SUCCEEDED(hr))
		{
			hr = MFCopyImage(
					pData,          // Destination buffer.
					m_Width * 4,	// Destination stride.
					pBits,			// First row in source image.
					pitch,          // Source stride.
					m_Width * 4,    // Image width in bytes.
					m_Height        // Image height in pixels.
			);
			m_pBuffer->Unlock();
		}

		// Set the data length of the buffer.
		if (SUCCEEDED(hr))
		{
			hr = m_pBuffer->SetCurrentLength(m_Width * m_Height * 4);
		}

		// Create a media sample and add the buffer to the sample.
		if (SUCCEEDED(hr))
		{
			hr = MFCreateSample(&pSample);
		}

		if (SUCCEEDED(hr))
		{
			hr = pSample->AddBuffer(m_pBuffer);
		}

		// Set the time stamp and the duration.
		if (SUCCEEDED(hr))
		{
			hr = pSample->SetSampleTime(g_SampleTime[m_InstanceID]);
		}
		if (SUCCEEDED(hr))
		{
			hr = pSample->SetSampleDuration(Elapsed100Nanoseconds);
		}

		// Send the sample to the Sink Writer.
		if (SUCCEEDED(hr))
		{
			hr = m_pSinkWriter->WriteSample(m_StreamIndex, pSample);
			g_SampleTime[m_InstanceID] += Elapsed100Nanoseconds;
		}

		SafeRelease(&pSample);
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