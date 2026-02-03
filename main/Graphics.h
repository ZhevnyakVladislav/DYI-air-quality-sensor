#pragma once
#include <cstdint>
#include "fonts.h"

class Graphics {
public:
    Graphics(int width, int height);
    ~Graphics();

    void clear();
    void drawPixel(int x, int y, int color);
    
    // New Drawing Primitives
    void drawLine(int x0, int y0, int x1, int y1, int color);
    void drawFastHLine(int x, int y, int w, int color);
    void drawFastVLine(int x, int y, int h, int color);
    void drawRect(int x, int y, int w, int h, int color);
    void drawBitmap(int x, int y, const uint8_t *bitmap, int w, int h, int color);

    void drawChar(int x, int y, char ascii, sFONT *font, int color);
    void drawString(int x, int y, const char *text, sFONT *font, int color);
    void drawStringCentered(int x, int y, int w, int h, const char *text, sFONT *font, int color);

    uint8_t* getBuffer();

private:
    int width, height;
    uint8_t* buffer;
    int bufferSize;
};