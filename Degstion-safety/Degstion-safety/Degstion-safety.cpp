#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <cstdlib>

typedef struct
{
    FLOAT h;
    FLOAT s;
    FLOAT l;
} HSL;

namespace Colors
{
    HSL rgb2hsl(RGBQUAD rgb)
    {
        HSL hsl;
        BYTE r = rgb.rgbRed;
        BYTE g = rgb.rgbGreen;
        BYTE b = rgb.rgbBlue;
        FLOAT _r = (FLOAT)r / 255.f;
        FLOAT _g = (FLOAT)g / 255.f;
        FLOAT _b = (FLOAT)b / 255.f;
        FLOAT rgbMin = min(min(_r, _g), _b);
        FLOAT rgbMax = max(max(_r, _g), _b);
        FLOAT fDelta = rgbMax - rgbMin;
        FLOAT deltaR;
        FLOAT deltaG;
        FLOAT deltaB;
        FLOAT h = 0.f;
        FLOAT s = 0.f;
        FLOAT l = (FLOAT)((rgbMax + rgbMin) / 2.f);
        if (fDelta != 0.f)
        {
            s = l < .5f ? (FLOAT)(fDelta / (rgbMax + rgbMin)) : (FLOAT)(fDelta / (2.f - rgbMax - rgbMin));
            deltaR = (FLOAT)(((rgbMax - _r) / 6.f + (fDelta / 2.f)) / fDelta);
            deltaG = (FLOAT)(((rgbMax - _g) / 6.f + (fDelta / 2.f)) / fDelta);
            deltaB = (FLOAT)(((rgbMax - _b) / 6.f + (fDelta / 2.f)) / fDelta);
            if (_r == rgbMax)      h = deltaB - deltaG;
            else if (_g == rgbMax) h = (1.f / 3.f) + deltaR - deltaB;
            else if (_b == rgbMax) h = (2.f / 3.f) + deltaG - deltaR;
            if (h < 0.f)           h += 1.f;
            if (h > 1.f)           h -= 1.f;
        }
        hsl.h = h;
        hsl.s = s;
        hsl.l = l;
        return hsl;
    }

    RGBQUAD hsl2rgb(HSL hsl)
    {
        RGBQUAD rgb;
        FLOAT r = hsl.l;
        FLOAT g = hsl.l;
        FLOAT b = hsl.l;
        FLOAT h = hsl.h;
        FLOAT sl = hsl.s;
        FLOAT l = hsl.l;
        FLOAT v = (l <= .5f) ? (l * (1.f + sl)) : (l + sl - l * sl);
        FLOAT m;
        FLOAT sv;
        FLOAT fract;
        FLOAT vsf;
        FLOAT mid1;
        FLOAT mid2;
        INT sextant;
        if (v > 0.f)
        {
            m = l + l - v;
            sv = (v - m) / v;
            h *= 6.f;
            sextant = (INT)h;
            fract = h - sextant;
            vsf = v * sv * fract;
            mid1 = m + vsf;
            mid2 = v - vsf;
            switch (sextant)
            {
            case 0: r = v; g = mid1; b = m; break;
            case 1: r = mid2; g = v; b = m; break;
            case 2: r = m; g = v; b = mid1; break;
            case 3: r = m; g = mid2; b = v; break;
            case 4: r = mid1; g = m; b = v; break;
            case 5: r = v; g = m; b = mid2; break;
            }
        }
        rgb.rgbRed = (BYTE)(r * 255.f);
        rgb.rgbGreen = (BYTE)(g * 255.f);
        rgb.rgbBlue = (BYTE)(b * 255.f);
        return rgb;
    }
}

volatile int activePhase = 0;
int red, green, blue;

COLORREF Hue(int length) {
    if (red != length) { red++; return RGB(red, 0, 0); }
    else if (green != length) { green++; return RGB(length, green, 0); }
    else if (blue != length) { blue++; return RGB(0, length, blue); }
    else { red = 0; green = 0; blue = 0; return RGB(0, 0, 0); }
}

void RefreshScreenClean() {
    InvalidateRect(NULL, NULL, TRUE);
    UpdateWindow(GetDesktopWindow());
}
DWORD WINAPI Sound1(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 11025 * 45;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>(t & 0xFFF) != 0 && (t & 0x3FF) != 0 ? ((((t >> 1 & 0x4000 ^ t) >> 14 & 3) + 1) * (((t >> 11) ^ (t >> 10)) % 5) * t % 99) + (((((((t >> 14 & 3) - (t >> 16 & 1)) + 3) / 3 | 0) * t % 99) & 0x40) | (2000 / (t & 0x3FF) | 0) | (40000 / (t & 0xFFF) | 0)) : 0;
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}

DWORD WINAPI Sound2(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 22050, 22050, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 22050 * 45;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>(t >> 4 | t * (t >> 5 & t >> 7) | t * (t >> 4 & t >> 5 | t >> 9) * t * (t + t >> t));
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}

DWORD WINAPI Sound3(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 11025 * 30;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>((t >> 10 ^ t >> 11) % 5 * ((t >> 14 & 3 ^ t >> 15 & 1) + 1) * t % 99 + ((3 + (t >> 14 & 3) - (t >> 16 & 1)) / 3 * t % 99 & 64));
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}

DWORD WINAPI Sound4(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 11025 * 30;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>((t >> 10 | t * 5) & (t >> 8 | t * 4) & (t >> 4 | t * 6));
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}

DWORD WINAPI Sound5(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 8000 * 30;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>(t * t / (1 + (t >> 9 & t >> 8)) & 128);
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}

DWORD WINAPI Sound6(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 22050, 22050, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 22050 * 30;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>(t + (t & t >> 12) * t >> 8);
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}

DWORD WINAPI Sound7(LPVOID lpParam) {
    HWAVEOUT hWaveOut = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
    waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int bufSize = 8000 * 30;
    char* buffer = new char[bufSize];
    for (DWORD t = 0; t < (DWORD)bufSize; ++t) {
        buffer[t] = static_cast<char>(t * ((t & 4096 ? t % 65536 < 59392 ? 7 : t & 7 : 16) + (1 & t >> 14)) >> (3 & -(long)t >> (t & 2048 ? 2 : 10)) | t >> (t & 16384 ? t & 4096 ? 10 : 3 : 2));
    }
    WAVEHDR header = { buffer, (DWORD)bufSize, 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR)); waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
    Sleep(30000); waveOutReset(hWaveOut); waveOutClose(hWaveOut); delete[] buffer;
    return 0;
}
DWORD WINAPI Payload1(LPVOID lpParam) {
    int iconIndexes[] = { 3, 4, 15, 27, 47, 77, 93, 109, 131, 161, 238, 240 };
    int totalIcons = sizeof(iconIndexes) / sizeof(iconIndexes);
    int w = GetSystemMetrics(SM_CXSCREEN); int h = GetSystemMetrics(SM_CYSCREEN);
    srand(GetTickCount()); DWORD startTime = GetTickCount();
    while (activePhase == 1 && (GetTickCount() - startTime < 15000)) {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            HDC hdcMem = CreateCompatibleDC(hdc); HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);
            BitBlt(hdcMem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
            bool invert = (rand() % 2 == 0);
            for (int y = 0; y < h; y += 4) {
                for (int x = 0; x < w; x += 4) {
                    int colorVal = (x ^ y) & 0xFF;
                    if (invert) colorVal = 255 - colorVal;
                    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, colorVal));
                    RECT r = { x, y, x + 4, y + 4 }; FillRect(hdcMem, &r, hBrush); DeleteObject(hBrush);
                }
            }
            for (int i = 0; i < 5; i++) {
                HICON hIcon = ExtractIconW(GetModuleHandle(NULL), L"shell32.dll", iconIndexes[rand() % totalIcons]);
                if (hIcon) { DrawIcon(hdcMem, rand() % w, rand() % h, hIcon); DestroyIcon(hIcon); }
            }
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, 128, 0 }; AlphaBlend(hdc, 0, 0, w, h, hdcMem, 0, 0, w, h, bf);
            SelectObject(hdcMem, hOldBm); DeleteObject(hbmMem); DeleteDC(hdcMem); ReleaseDC(NULL, hdc);
        }
        Sleep(50);
    }
    RefreshScreenClean(); return 0;
}

DWORD WINAPI payload2(LPVOID lpParam) {
    int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    while (activePhase == 2) {
        HDC hdc = GetDC(0); int rx = rand() % sw; int ry = rand() % sh;
        BitBlt(hdc, rx, 10, 100, sh, hdc, rx, 0, SRCCOPY); BitBlt(hdc, rx, -10, -100, sh, hdc, rx, 0, SRCCOPY);
        BitBlt(hdc, 10, ry, sw, 96, hdc, 0, ry, SRCCOPY); BitBlt(hdc, -10, ry, sw, -96, hdc, 0, ry, SRCCOPY);
        BitBlt(hdc, -2, -2, sw, sh, hdc, 1, 1, SRCCOPY); Sleep(10); ReleaseDC(0, hdc);
    }
    RefreshScreenClean(); return 0;
}

DWORD WINAPI playload3(LPVOID lpParam) {
    int y = GetSystemMetrics(SM_CYSCREEN); int x = GetSystemMetrics(SM_CXSCREEN);
    int yfull = GetSystemMetrics(SM_CYFULLSCREEN); int xfull = GetSystemMetrics(SM_CXFULLSCREEN);
    int inc = round(x / 100);
    while (activePhase == 3) {
        HDC hdc = GetDC(0); int rainbow = x - rand() % x - (x / 150 - 112) % 149;
        for (int yp = 0; yp < y && activePhase == 3; ++yp) {
            for (int xp = 0; xp < x && activePhase == 3; ++xp) {
                int xa = inc ^ xp;
                SetPixel(hdc, xp, yp, RGB(xa, xa, xa)); SetPixel(hdc, yp, rainbow, RGB(yfull, xa, xfull));
                SetPixel(hdc, rainbow, rainbow, RGB(rainbow, rainbow, rainbow)); SetPixel(hdc, rainbow, yp, RGB(xfull, yfull, xfull));
            }
        }
        ReleaseDC(0, hdc);
    }
    RefreshScreenClean(); return 0;
}

DWORD WINAPI payload4(LPVOID lpParam) {
    HDC hdc = GetDC(NULL); HDC hdcCopy = CreateCompatibleDC(hdc);
    int w = GetSystemMetrics(0); int h = GetSystemMetrics(1);
    BITMAPINFO bmpi = { 0 }; HBITMAP bmp;
    bmpi.bmiHeader.biSize = sizeof(bmpi); bmpi.bmiHeader.biWidth = w; bmpi.bmiHeader.biHeight = h;
    bmpi.bmiHeader.biPlanes = 1; bmpi.bmiHeader.biBitCount = 32; bmpi.bmiHeader.biCompression = BI_RGB;
    RGBQUAD* rgbquad = NULL; HSL hslcolor;
    bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, (void**)&rgbquad, NULL, 0); SelectObject(hdcCopy, bmp);
    INT i = 0;
    while (activePhase == 4) {
        hdc = GetDC(NULL); StretchBlt(hdcCopy, 0, 0, w, h, hdc, 0, 0, w, h, SRCCOPY);
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                int index = y * w + x;
                int fx = (int)((4 * i) + ((4 * i) * sin((x + i) / 16.0)) + (4 * i) + ((4 * i) * sin((y - i) / 8.0)));
                hslcolor = Colors::rgb2hsl(rgbquad[index]);
                hslcolor.h = fmod(fx / 200.f + y / h * .2f, 1.f); hslcolor.l = fmod(hslcolor.l + 0.1f * cos(i / 10.0f), 1.f);
                rgbquad[index] = Colors::hsl2rgb(hslcolor);
            }
        }
        i++; StretchBlt(hdc, 0, 0, w, h, hdcCopy, 0, 0, w, h, SRCCOPY); ReleaseDC(NULL, hdc);
    }
    RefreshScreenClean(); DeleteDC(hdcCopy); DeleteObject(bmp); return 0;
}

DWORD WINAPI Playload5(LPVOID lpvd) {
    HDC hdc = GetDC(NULL); HDC hdcCopy = CreateCompatibleDC(hdc);
    int w = GetSystemMetrics(0); int h = GetSystemMetrics(1);
    BITMAPINFO bmpi = { 0 }; HBITMAP bmp;
    bmpi.bmiHeader.biSize = sizeof(bmpi); bmpi.bmiHeader.biWidth = w; bmpi.bmiHeader.biHeight = h;
    bmpi.bmiHeader.biPlanes = 1; bmpi.bmiHeader.biBitCount = 32; bmpi.bmiHeader.biCompression = BI_RGB;
    RGBQUAD* rgbquad = NULL; HSL hslcolor;
    bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, (void**)&rgbquad, NULL, 0); SelectObject(hdcCopy, bmp);
    INT i = 0;
    while (activePhase == 5) {
        hdc = GetDC(NULL); StretchBlt(hdcCopy, 0, 0, w, h, hdc, 0, 0, w, h, SRCCOPY);
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                int index = y * w + x; int fx = (int)((4 * i) + ((4 * i) * sin(x / 32.0)) + (4 * i) + ((4 * i) * sin(y / 24.0)));
                hslcolor = Colors::rgb2hsl(rgbquad[index]); float currentHueDegrees = hslcolor.h * 360.f;
                if ((currentHueDegrees >= 0.f && currentHueDegrees <= 20.f) || (currentHueDegrees >= 340.f && currentHueDegrees <= 360.f) || (currentHueDegrees >= 40.f && currentHueDegrees <= 65.f)) {
                    hslcolor.h = fmod(fx / 300.f + y / h * .1f, 1.f); rgbquad[index] = Colors::hsl2rgb(hslcolor);
                }
            }
        }
        i++; StretchBlt(hdc, 0, 0, w, h, hdcCopy, 0, 0, w, h, SRCCOPY); ReleaseDC(NULL, hdc);
    }
    RefreshScreenClean(); DeleteDC(hdcCopy); DeleteObject(bmp); return 0;
}

DWORD WINAPI payload6(LPVOID lpParam) {
    int w = GetSystemMetrics(0); int h = GetSystemMetrics(1);
    while (activePhase == 6) {
        HDC hdc = GetDC(NULL); if (!hdc) continue;
        StretchBlt(hdc, -10, -10, w + 20, h + 20, hdc, 0, 0, w, h, SRCCOPY);
        HPEN hPen = CreatePen(PS_SOLID, rand() % 10 + 2, RGB(rand() % 256, rand() % 256, rand() % 256));
        HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, rand() % w, rand() % h, NULL);
        POINT points[3] = { {rand() % w, rand() % h}, {rand() % w, rand() % h}, {rand() % w, rand() % h} };
        PolyBezierTo(hdc, points, 3);
        SelectObject(hdc, oldPen); DeleteObject(hPen); ReleaseDC(NULL, hdc); Sleep(10);
    }
    RefreshScreenClean(); return 0;
}

DWORD WINAPI Payload7(LPVOID lpParam) {
    int w = GetSystemMetrics(SM_CXSCREEN); int h = GetSystemMetrics(SM_CYSCREEN);
    srand(GetTickCount()); DWORD startTime = GetTickCount();
    while (activePhase == 7 && (GetTickCount() - startTime < 15000)) {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            HBRUSH brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush); PatBlt(hdc, 0, 0, w, h, PATINVERT);
            SelectObject(hdc, oldBrush); DeleteObject(brush);
            StretchBlt(hdc, w + ((rand() % 21) - 10), (rand() % 21) - 10, -w + ((rand() % 21) - 10), h + ((rand() % 21) - 10),
                hdc, (rand() % 21) - 10, (rand() % 21) - 10, w + ((rand() % 21) - 10), h + ((rand() % 21) - 10), SRCCOPY);
            ReleaseDC(NULL, hdc);
        }
        Sleep(10);
    }
    RefreshScreenClean(); return 0;
}
DWORD WINAPI Cur(LPVOID lpParam) {
    int iconIndexes[] = { 3, 4, 15, 27, 47, 77, 93, 109, 131, 161, 238, 240 };
    int totalIcons = sizeof(iconIndexes) / sizeof(iconIndexes);
    HMODULE hShell = LoadLibraryW(L"shell32.dll");
    srand(GetTickCount());
    DWORD lastIconChange = GetTickCount();
    HICON hIcon = ExtractIconW(GetModuleHandle(NULL), L"shell32.dll", iconIndexes[0]);
    HCURSOR hCurrentCursor = LoadCursor(NULL, IDC_ARROW);
    while (true) {
        DWORD currentTick = GetTickCount();
        if (currentTick - lastIconChange >= 2000) {
            int randomIndex = rand() % totalIcons;
            HICON hNewIcon = ExtractIconW(GetModuleHandle(NULL), L"shell32.dll", iconIndexes[randomIndex]);
            if (hNewIcon) {
                if (hIcon) DestroyIcon(hIcon);
                hIcon = hNewIcon;
                ICONINFO ii;
                if (GetIconInfo(hIcon, &ii)) {
                    ii.fIcon = FALSE; ii.xHotspot = 0; ii.yHotspot = 0;
                    hCurrentCursor = CreateIconIndirect(&ii);
                    DeleteObject(ii.hbmMask); DeleteObject(ii.hbmColor);
                }
            }
            lastIconChange = currentTick;
        }
        SetCursor(hCurrentCursor);
        POINT p;
        if (GetCursorPos(&p)) {
            int offsetX = (rand() % 31) - 15; int offsetY = (rand() % 31) - 15;
            int newX = p.x + offsetX; int newY = p.y + offsetY;
            SetCursorPos(newX, newY);
            if (hIcon) {
                HDC hdc = GetDC(NULL);
                if (hdc) { DrawIcon(hdc, newX, newY, hIcon); ReleaseDC(NULL, hdc); }
            }
        }
        Sleep(30);
    }
    if (hIcon) DestroyIcon(hIcon);
    if (hShell) FreeLibrary(hShell);
    return 0;
}

DWORD WINAPI texts(LPVOID lpvd) {
    int x = GetSystemMetrics(0); int y = GetSystemMetrics(1);
    LPCSTR text[] = { "Degstion.exe", "AMOGUS!!!", "pangey1", "Enjoy your new PC!!!", "THERE IS NO MERCY!!!", "IS THIS OHIO?!", "WHAT IS HAPPENING?!", "R.I.P PC!!!", "SYSTEM OVERLOADED!!!" };
    while (1) {
        HDC hdc = GetDC(0); SetBkMode(hdc, 0);
        for (int i = 0; i < 9; ++i) {
            SetTextColor(hdc, RGB(rand() % 255, rand() % 255, rand() % 255));
            TextOutA(hdc, rand() % x, rand() % y, text[i], strlen(text[i]));
            Sleep(10);
        }
        ReleaseDC(0, hdc);
    }
}

DWORD WINAPI Ball(LPVOID lpParam) {
    int w = GetSystemMetrics(0); int h = GetSystemMetrics(1);
    int signX = 1, signY = 1, incrementor = 10, x = 10, y = 10; float hueCounter = 0.0f;
    while (1) {
        HDC hdc = GetDC(0);
        if (hdc) {
            int top_x = x, top_y = y, bottom_x = 100 + x, bottom_y = 100 + y;
            x += incrementor * signX; y += incrementor * signY;
            hueCounter += 0.01f; if (hueCounter >= 1.0f) hueCounter = 0.0f;
            HSL hsl = { hueCounter, 1.0f, 0.5f }; RGBQUAD rgb = Colors::hsl2rgb(hsl);
            HBRUSH brush = CreateSolidBrush(RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
            Ellipse(hdc, top_x, top_y, bottom_x, bottom_y);
            if (y >= h - 100) signY = -1; if (x >= w - 100) signX = -1;
            if (y <= 0) signY = 1; if (x <= 0) signX = 1;
            SelectObject(hdc, oldBrush); DeleteObject(brush); ReleaseDC(0, hdc);
        }
        Sleep(10);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    if (MessageBoxW(NULL, L"Run safety?", L"Degstion.exe", MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST) != IDYES) return 0;
    if (MessageBoxW(NULL, L"Are you sure?", L"Warning", MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST) != IDYES) return 0;
    Sleep(500);

    CreateThread(NULL, 0, Ball, NULL, 0, NULL);
    CreateThread(NULL, 0, Cur, NULL, 0, NULL);

    // Фаза 1
    activePhase = 1;
    HANDLE hThread1 = CreateThread(NULL, 0, Payload1, NULL, 0, NULL);
    HANDLE hSnd1 = CreateThread(NULL, 0, Sound1, NULL, 0, NULL);
    if (hSnd1 != NULL) { WaitForSingleObject(hSnd1, 15000); CloseHandle(hSnd1); }
    activePhase = 0; if (hThread1 != NULL) { WaitForSingleObject(hThread1, 5000); CloseHandle(hThread1); }
    RefreshScreenClean(); Sleep(1000);

    // Фаза 2
    activePhase = 2;
    HANDLE hThread2 = CreateThread(NULL, 0, payload2, NULL, 0, NULL);
    HANDLE hSnd2 = CreateThread(NULL, 0, Sound2, NULL, 0, NULL);
    if (hSnd2 != NULL) { WaitForSingleObject(hSnd2, 15000); CloseHandle(hSnd2); }
    activePhase = 0; if (hThread2 != NULL) { WaitForSingleObject(hThread2, 5000); CloseHandle(hThread2); }
    RefreshScreenClean(); Sleep(1000);

    // Фаза 3
    activePhase = 3;
    CreateThread(NULL, 0, texts, NULL, 0, NULL);
    HANDLE hThread3 = CreateThread(NULL, 0, playload3, NULL, 0, NULL);
    HANDLE hSnd3 = CreateThread(NULL, 0, Sound3, NULL, 0, NULL);
    if (hSnd3 != NULL) { WaitForSingleObject(hSnd3, 15000); CloseHandle(hSnd3); }
    activePhase = 0; if (hThread3 != NULL) { WaitForSingleObject(hThread3, 5000); CloseHandle(hThread3); }
    RefreshScreenClean(); Sleep(1000);

    // Фаза 4
    activePhase = 4;
    HANDLE hThread4 = CreateThread(NULL, 0, payload4, NULL, 0, NULL);
    HANDLE hSnd4 = CreateThread(NULL, 0, Sound4, NULL, 0, NULL);
    if (hSnd4 != NULL) { WaitForSingleObject(hSnd4, 15000); CloseHandle(hSnd4); }
    activePhase = 0; if (hThread4 != NULL) { WaitForSingleObject(hThread4, 5000); CloseHandle(hThread4); }
    RefreshScreenClean(); Sleep(1000);

    // Фаза 5
    activePhase = 5;
    HANDLE hThread5 = CreateThread(NULL, 0, Playload5, NULL, 0, NULL);
    HANDLE hSnd5 = CreateThread(NULL, 0, Sound5, NULL, 0, NULL);
    if (hSnd5 != NULL) { WaitForSingleObject(hSnd5, 15000); CloseHandle(hSnd5); }
    activePhase = 0; if (hThread5 != NULL) { WaitForSingleObject(hThread5, 5000); CloseHandle(hThread5); }
    RefreshScreenClean(); Sleep(1000);

    // Фаза 6
    activePhase = 6;
    HANDLE hThread6 = CreateThread(NULL, 0, payload6, NULL, 0, NULL);
    HANDLE hSnd6 = CreateThread(NULL, 0, Sound6, NULL, 0, NULL);
    if (hSnd6 != NULL) { WaitForSingleObject(hSnd6, 15000); CloseHandle(hSnd6); }
    activePhase = 0; if (hThread6 != NULL) { WaitForSingleObject(hThread6, 5000); CloseHandle(hThread6); }
    RefreshScreenClean(); Sleep(1000);

    // Фаза 7
    activePhase = 7;
    HANDLE hThread7 = CreateThread(NULL, 0, Payload7, NULL, 0, NULL);
    HANDLE hSnd7 = CreateThread(NULL, 0, Sound7, NULL, 0, NULL);
    if (hSnd7 != NULL) { WaitForSingleObject(hSnd7, 15000); CloseHandle(hSnd7); }
    activePhase = 0; if (hThread7 != NULL) { WaitForSingleObject(hThread7, 5000); CloseHandle(hThread7); }
    RefreshScreenClean();

    return 0;
}
