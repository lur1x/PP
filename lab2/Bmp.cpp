#include "Bmp.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdexcept>

Bitmap::Bitmap(const char* path)
    : m_bmpInfo()
    , m_pixels(nullptr)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file) 
    {
        throw std::runtime_error("Failed to open file: " + std::string(path));
    }

    file.read(reinterpret_cast<char*>(&m_bmpInfo.bfh), sizeof(m_bmpInfo.bfh));
    file.read(reinterpret_cast<char*>(&m_bmpInfo.bih), sizeof(m_bmpInfo.bih));

    if (m_bmpInfo.bfh.bfType != 0x4D42) 
    { 
        throw std::runtime_error("Not a valid BMP file");
    }

    int32_t height = m_bmpInfo.bih.biHeight;
    if (height == 0) 
    {
        throw std::runtime_error("Invalid BMP: height is zero");
    }

    bool isBottomUp = (height > 0);
    uint32_t absHeight = static_cast<uint32_t>(std::abs(height));
    uint32_t width = m_bmpInfo.bih.biWidth;

    if (m_bmpInfo.bih.biBitCount != 24 && m_bmpInfo.bih.biBitCount != 32) 
    {
        throw std::runtime_error("Only 24-bit and 32-bit BMP are supported");
    }

    uint32_t bytesPerPixel = m_bmpInfo.bih.biBitCount / 8;
    uint32_t rowSize = ((width * bytesPerPixel + 3) / 4) * 4; 
    uint32_t pixelDataSize = rowSize * absHeight;

    if (m_bmpInfo.bih.biSizeImage == 0) 
    {
        m_bmpInfo.bih.biSizeImage = pixelDataSize;
    }

    file.seekg(m_bmpInfo.bfh.bfOffBits, std::ios::beg);

    m_pixels = new uint8_t[pixelDataSize];
    file.read(reinterpret_cast<char*>(m_pixels), pixelDataSize);

    if (file.gcount() != pixelDataSize) 
    {
        delete[] m_pixels;
        m_pixels = nullptr;
        throw std::runtime_error("Failed to read pixel data");
    }

    uint32_t convertedSize = width * absHeight * sizeof(rgb32);
    uint8_t* temp = new uint8_t[convertedSize];
    rgb32* out = reinterpret_cast<rgb32*>(temp);

    uint8_t* in = m_pixels;
    int32_t rowPadding = rowSize - (width * bytesPerPixel);

    for (uint32_t row = 0; row < absHeight; ++row) 
    {
        uint32_t sourceRow = isBottomUp ? (absHeight - 1 - row) : row;
        uint8_t* rowIn = m_pixels + (sourceRow * rowSize);

        for (uint32_t col = 0; col < width; ++col) 
        {
            if (m_bmpInfo.bih.biBitCount == 24) 
            {
                out->b = rowIn[0];
                out->g = rowIn[1];
                out->r = rowIn[2];
                out->a = 0xFF; 
                rowIn += 3;
            }
            else if (m_bmpInfo.bih.biBitCount == 32) 
            {
                out->b = rowIn[0];
                out->g = rowIn[1];
                out->r = rowIn[2];
                out->a = rowIn[3];
                rowIn += 4;
            }
            ++out;
        }

    }

    m_bmpInfo.bih.biBitCount = 32;
    m_bmpInfo.bih.biSizeImage = convertedSize;
    m_bmpInfo.bfh.bfSize = sizeof(m_bmpInfo.bfh) + sizeof(m_bmpInfo.bih) + convertedSize;
    m_bmpInfo.bih.biHeight = absHeight; 

    delete[] m_pixels;
    m_pixels = temp;
}

Bitmap::~Bitmap()
{
    delete[] m_pixels;
}

void Bitmap::Save(const char* path, uint16_t bit_count)
{
    if (bit_count != 24 && bit_count != 32) 
    {
        throw std::runtime_error("Only 24-bit and 32-bit output supported");
    }

    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file) 
    {
        throw std::runtime_error("Failed to create output file");
    }

    BMPINFO savedInfo = m_bmpInfo;

    m_bmpInfo.bih.biBitCount = bit_count;
    uint32_t bytesPerPixel = bit_count / 8;
    uint32_t rowSize = ((m_bmpInfo.bih.biWidth * bytesPerPixel + 3) / 4) * 4;
    uint32_t imageSize = rowSize * m_bmpInfo.bih.biHeight;

    m_bmpInfo.bih.biSizeImage = imageSize;
    m_bmpInfo.bfh.bfSize = m_bmpInfo.bfh.bfOffBits + imageSize;

    m_bmpInfo.bih.biHeight = std::abs(m_bmpInfo.bih.biHeight);

    file.write(reinterpret_cast<char*>(&m_bmpInfo.bfh), sizeof(m_bmpInfo.bfh));
    file.write(reinterpret_cast<char*>(&m_bmpInfo.bih), sizeof(m_bmpInfo.bih));

    file.seekp(m_bmpInfo.bfh.bfOffBits, std::ios::beg);

    rgb32* in = reinterpret_cast<rgb32*>(m_pixels);
    uint32_t width = m_bmpInfo.bih.biWidth;
    uint32_t height = m_bmpInfo.bih.biHeight;

    uint8_t* outBuffer = new uint8_t[imageSize];
    uint8_t* out = outBuffer;
    int32_t rowPadding = rowSize - (width * bytesPerPixel);

    for (int row = height - 1; row >= 0; --row) 
    {
        rgb32* rowIn = in + (row * width);

        for (uint32_t col = 0; col < width; ++col) 
        {
            if (bit_count == 24) 
            {
                *out++ = rowIn->b;
                *out++ = rowIn->g;
                *out++ = rowIn->r;
            }
            else if (bit_count == 32)
            {
                *out++ = rowIn->b;
                *out++ = rowIn->g;
                *out++ = rowIn->r;
                *out++ = rowIn->a;
            }
            ++rowIn;
        }

        for (int32_t i = 0; i < rowPadding; ++i) 
        {
            *out++ = 0;
        }
    }

    file.write(reinterpret_cast<char*>(outBuffer), imageSize);
    delete[] outBuffer;

    m_bmpInfo = savedInfo;
}

rgb32* Bitmap::GetPixel(uint32_t x, uint32_t y) const
{
    if (x >= GetWidth() || y >= GetHeight()) 
    {
        throw std::out_of_range("Pixel coordinates out of range");
    }

    rgb32* pixels = reinterpret_cast<rgb32*>(m_pixels);
    return &pixels[y * GetWidth() + x];
}

void Bitmap::SetPixel(rgb32* pixel, uint32_t x, uint32_t y)
{
    if (x >= GetWidth() || y >= GetHeight()) 
    {
        throw std::out_of_range("Pixel coordinates out of range");
    }

    rgb32* target = GetPixel(x, y);
    *target = *pixel;
}

uint32_t Bitmap::GetWidth() const
{
    return m_bmpInfo.bih.biWidth;
}

uint32_t Bitmap::GetHeight() const
{
    return static_cast<uint32_t>(std::abs(m_bmpInfo.bih.biHeight));
}

uint16_t Bitmap::BitCount() const
{
    return m_bmpInfo.bih.biBitCount;
}