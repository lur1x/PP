#pragma once
#include <Windows.h>
#include <cstdint>

typedef struct
{
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
} BMPINFO;

typedef struct
{
    uint8_t r, g, b, a;
} rgb32;

class Bitmap
{
public:
    Bitmap(const char* path);
    ~Bitmap();

    void Save(const char* path, uint16_t bit_count = 24);

    rgb32* GetPixel(uint32_t x, uint32_t y) const;
    void SetPixel(rgb32* pixel, uint32_t x, uint32_t y);

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint16_t BitCount() const;

private:
    BMPINFO m_bmpInfo;
    uint8_t* m_pixels;
};
