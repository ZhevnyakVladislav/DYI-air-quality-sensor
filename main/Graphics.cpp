#include "Graphics.h"
#include <cstring>
#include <cmath>
#include <cstdlib> // for abs
#include "esp_heap_caps.h"

Graphics::Graphics(int w, int h) : width(w), height(h) {
    bufferSize = (w * h) / 8;
    buffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_DMA);
    clear();
}

Graphics::~Graphics() {
    if (buffer) free(buffer);
}

void Graphics::clear() {
    memset(buffer, 0xFF, bufferSize);
}

uint8_t* Graphics::getBuffer() { return buffer; }

void Graphics::drawPixel(int x, int y, int color) {
    // 1. Rotate Logical (Landscape) to Hardware (Portrait)
    // Logical: 0..295 (x), 0..127 (y)
    // Hardware: 0..127 (hw_x), 0..295 (hw_y)
    int hw_x = y;
    int hw_y = (296 - 1) - x; // Hardcoded height of 296 for safety

    // Safety check
    if (hw_x < 0 || hw_x >= 128 || hw_y < 0 || hw_y >= 296) return;

    // 2. Calculate correct buffer index
    // The hardware width is 128 pixels (16 bytes)
    // We MUST use 16 (128/8) as the stride, not the logical width!
    int STRIDE = 16; 
    int idx = (hw_x / 8) + (hw_y * STRIDE);

    if (color == 0) buffer[idx] &= ~(0x80 >> (hw_x % 8)); // Black
    else            buffer[idx] |=  (0x80 >> (hw_x % 8)); // White
}

// Bresenham's Line Algorithm
void Graphics::drawLine(int x0, int y0, int x1, int y1, int color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Graphics::drawFastHLine(int x, int y, int w, int color) {
    drawLine(x, y, x + w - 1, y, color);
}

void Graphics::drawFastVLine(int x, int y, int h, int color) {
    drawLine(x, y, x, y + h - 1, color);
}

void Graphics::drawRect(int x, int y, int w, int h, int color) {
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void Graphics::drawBitmap(int x, int y, const uint8_t *bitmap, int w, int h, int color) {
    int byteWidth = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (bitmap[j * byteWidth + i / 8] & (128 >> (i & 7))) {
                drawPixel(x + i, y + j, color);
            }
        }
    }
}

void Graphics::drawChar(int x, int y, char ascii, sFONT *font, int color) {
    ascii -= 32;
    int bytes_per_line = (font->Width + 7) / 8;
    const uint8_t *ptr = &font->table[ascii * font->Height * bytes_per_line];

    for (int row = 0; row < font->Height; row++) {
        for (int col = 0; col < font->Width; col++) {
            if (ptr[row * bytes_per_line + col / 8] & (0x80 >> (col % 8))) {
                drawPixel(x + col, y + row, color);
            }
        }
    }
}

void Graphics::drawString(int x, int y, const char *text, sFONT *font, int color) {
    while (*text) {
        drawChar(x, y, *text, font, color);
        x += font->Width;
        text++;
    }
}

void Graphics::drawStringCentered(int x, int y, int w, int h, const char *text, sFONT *font, int color) {
    int str_len = strlen(text);
    int str_w = str_len * font->Width;
    int start_x = x + (w - str_w) / 2;
    int start_y = y + (h - font->Height) / 2;
    drawString(start_x, start_y, text, font, color);
}
