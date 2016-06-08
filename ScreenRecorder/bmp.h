#pragma once

#ifndef __BMP_FILE_H__
#define __BMP_FILE_H__

#include <wingdi.h>
//#include <stdio.h>

#pragma pack(push)
#pragma pack(1)

typedef struct tagBITMAP_FILE {

    BITMAPFILEHEADER bitmapheader;
    BITMAPINFOHEADER bitmapinfoheader;
    PALETTEENTRY     palette[256];
    BYTE             *buffer;
}BITMAP_FILE, FAR *LPBITMAP_FILE, *PBITMAP_FILE;

#pragma pack(pop)
#endif /* __BMP_FILE_H__ */