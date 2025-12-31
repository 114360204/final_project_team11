#define _CRT_SECURE_NO_WARNINGS 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h> 
#include "cJSON.h" 
#include <curl/curl.h>
#include "WeatherEngine.h"
#include "WeatherUI.h" 

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define COLOR_BG RGB(26, 26, 46) 
#define COLOR_TEXT RGB(255, 255, 255) 

HBRUSH hBrushBg;
const char* API_KEY = "2fa80dc7ec8be3fac297f88afd028de9";

// 引用 WeatherUI.c 裡面的全域變數 函式庫
extern int langMode;
extern Country worldData[];
extern int currentCountryIdx;

// --- 網路功能實作 ---
struct string { char* ptr; size_t len; };
void init_string(struct string* s) { s->len = 0; s->ptr = malloc(1); s->ptr[0] = '\0'; }

size_t writefunc(void* ptr, size_t size, size_t nmemb, struct string* s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0'; s->len = new_len;
    return size * nmemb;
}

void UTF8ToANSI(char* src, char* dest, int destSize) {
    wchar_t wStr[512];
    MultiByteToWideChar(CP_UTF8, 0, src, -1, wStr, 512);
    WideCharToMultiByte(CP_ACP, 0, wStr, -1, dest, destSize, NULL, NULL);
}

int getAQI(double lat, double lon) {
    CURL* curl; struct string s; init_string(&s); curl = curl_easy_init();
    char url[512]; sprintf(url, "http://api.openweathermap.org/data/2.5/air_pollution?lat=%f&lon=%f&appid=%s", lat, lon, API_KEY);
    curl_easy_setopt(curl, CURLOPT_URL, url); curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc); curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    int aqi = -1; if (curl_easy_perform(curl) == CURLE_OK) {
        cJSON* json = cJSON_Parse(s.ptr); if (json) {
            cJSON* list = cJSON_GetObjectItem(json, "list"); cJSON* first = cJSON_GetArrayItem(list, 0);
            aqi = cJSON_GetObjectItem(cJSON_GetObjectItem(first, "main"), "aqi")->valueint; cJSON_Delete(json);
        }
    }
    curl_easy_cleanup(curl); free(s.ptr); return aqi;
}

int getWeather(char* searchName, char* displayName, WeatherInfo* info) {
    CURL* curl; struct string s; init_string(&s); curl = curl_easy_init();
    if (!curl) return 0;

    char encodedName[100]; strcpy(encodedName, searchName);
    for (int i = 0; encodedName[i]; i++) if (encodedName[i] == ' ') encodedName[i] = '+';

    const char* langCodes[] = { "zh_tw", "en" };

    char url[512]; sprintf(url, "http://api.openweathermap.org/data/2.5/forecast?q=%s&appid=%s&units=metric&lang=%s", encodedName, API_KEY, langCodes[langMode]);
    curl_easy_setopt(curl, CURLOPT_URL, url); curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc); curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    if (curl_easy_perform(curl) != CURLE_OK) { free(s.ptr); curl_easy_cleanup(curl); return 0; }
    cJSON* json = cJSON_Parse(s.ptr); if (!json) { free(s.ptr); curl_easy_cleanup(curl); return 0; }
    cJSON* list = cJSON_GetObjectItem(json, "list"); cJSON* cur = cJSON_GetArrayItem(list, 0);
    cJSON* city_obj = cJSON_GetObjectItem(json, "city"); cJSON* coord = cJSON_GetObjectItem(city_obj, "coord");
    double lat = cJSON_GetObjectItem(coord, "lat")->valuedouble; double lon = cJSON_GetObjectItem(coord, "lon")->valuedouble;

    info->temperature = cJSON_GetObjectItem(cJSON_GetObjectItem(cur, "main"), "temp")->valuedouble;
    info->humidity = cJSON_GetObjectItem(cJSON_GetObjectItem(cur, "main"), "humidity")->valueint;
    strcpy(info->city, displayName);
    UTF8ToANSI(cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(cur, "weather"), 0), "description")->valuestring, info->description, 100);

    double t_max = -99, t_min = 99; char tomorrow_desc[100] = ""; int count = 0, h_sum = 0;
    for (int i = 8; i < 16; i++) {
        cJSON* item = cJSON_GetArrayItem(list, i); if (!item) break;
        double t = cJSON_GetObjectItem(cJSON_GetObjectItem(item, "main"), "temp")->valuedouble;
        int h = cJSON_GetObjectItem(cJSON_GetObjectItem(item, "main"), "humidity")->valueint;
        if (t > t_max) t_max = t; if (t < t_min) t_min = t;
        h_sum += h; count++;
        if (i == 12) UTF8ToANSI(cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(item, "weather"), 0), "description")->valuestring, tomorrow_desc, 100);
    }
    info->aqi = getAQI(lat, lon);
    sprintf(info->tomorrow_brief, "  - %s: %.1f ~ %.1f C\r\n  - %s: %d %%\r\n  - %s: %s\r\n  - %s: AQI %d [%s]",
        GetUIText(14), t_min, t_max, GetUIText(15), (count ? h_sum / count : 0), GetUIText(4), tomorrow_desc, GetUIText(5), info->aqi, info->aqi <= 2 ? GetUIText(11) : GetUIText(12));

    if (info->aqi <= 2) strcpy(info->suggestion, langMode == 0 ? "空氣品質良好，非常適合戶外活動。" : "Air quality is good. Great for outdoors.");
    else strcpy(info->suggestion, langMode == 0 ? "空氣品質警戒，建議減少外出並佩戴口罩。" : "Air quality warning. Please wear a mask.");

    cJSON_Delete(json); curl_easy_cleanup(curl); free(s.ptr); return 1;
}

// --- 報告視窗邏輯 ---
LRESULT CALLBACK ReportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND && LOWORD(wParam) == 888) { DestroyWindow(hwnd); return 0; }
    if (msg == WM_CLOSE) { DestroyWindow(hwnd); return 0; }
    if (msg == WM_CTLCOLORSTATIC) {
        SetTextColor((HDC)wParam, RGB(0, 0, 0));
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void ShowWeatherReport(HWND owner, WeatherInfo info) {
    char report[2048]; char timeStr[100]; time_t now = time(NULL); strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    sprintf(report, "【 %s - %s 】\r\n\r\n 現在狀態:\r\n ------------------------------------------\r\n  > %s: %.1f C\r\n  > %s: %d %%\r\n  > %s: %s\r\n  > %s: AQI %d\r\n\r\n [ %s ]\r\n ------------------------------------------\r\n%s\r\n\r\n [ %s ]\r\n ------------------------------------------\r\n  %s\r\n\r\n %s: %s",
        info.city, (langMode == 0 ? "衛星觀測" : "Satellite Obv"), GetUIText(2), info.temperature, GetUIText(3), info.humidity, GetUIText(4), info.description, GetUIText(5), info.aqi, GetUIText(6), info.tomorrow_brief, GetUIText(16), info.suggestion, GetUIText(8), timeStr);

    WNDCLASSA rwc = { 0 }; rwc.lpfnWndProc = ReportWndProc; rwc.hInstance = GetModuleHandle(NULL); rwc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); rwc.lpszClassName = "FinalReportWinV12"; RegisterClassA(&rwc);
    HWND hPop = CreateWindowA("FinalReportWinV12", GetUIText(0), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 920, 850, owner, NULL, NULL, NULL);

    char weatherStatus[50], aqiStatus[50];
    sprintf(weatherStatus, "--- %s ---", info.description);
    sprintf(aqiStatus, "%s: %s", GetUIText(13), info.aqi <= 2 ? GetUIText(11) : GetUIText(12));

    // --- 圖片載入邏輯 ---
    const char* weatherImgFile = "cloud.bmp";
    if (strstr(info.description, "晴")) weatherImgFile = "sun.bmp";
    else if (strstr(info.description, "雨")) weatherImgFile = "rain.bmp";

    const char* aqiImgFile = (info.aqi <= 2) ? "safe.bmp" : "warning.bmp";

    int imgW = 110;
    int imgH = 110;
    HANDLE hBmpWeather = LoadImageA(NULL, weatherImgFile, IMAGE_BITMAP, imgW, imgH, LR_LOADFROMFILE);
    HANDLE hBmpAQI = LoadImageA(NULL, aqiImgFile, IMAGE_BITMAP, imgW, imgH, LR_LOADFROMFILE);

    // [除錯程式碼] 找不到圖片時會跳出視窗
    if (hBmpWeather == NULL) {
        char path[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, path);
        char msg[1024];
        sprintf(msg, "找不到天氣圖片: %s\n\n程式正在這個路徑找圖片:\n%s\n\n錯誤代碼: %d", weatherImgFile, path, GetLastError());
        MessageBoxA(owner, msg, "圖片讀取錯誤", MB_OK | MB_ICONERROR);
    }
    if (hBmpAQI == NULL) {
        // 如果 AQI 圖也找不到，這裡可以再跳一次，或不管它
    }

    HWND hWImg = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE, 720, 50, imgW, imgH, hPop, NULL, NULL, NULL);
    SendMessage(hWImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpWeather);

    HWND hAImg = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE, 720, 260, imgW, imgH, hPop, NULL, NULL, NULL);
    SendMessage(hAImg, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmpAQI);

    HWND hWText = CreateWindowA("STATIC", weatherStatus, WS_CHILD | WS_VISIBLE | SS_CENTER, 680, 165, 200, 30, hPop, NULL, NULL, NULL);
    SendMessage(hWText, WM_SETFONT, (WPARAM)CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);
    HWND hAText = CreateWindowA("STATIC", aqiStatus, WS_CHILD | WS_VISIBLE | SS_CENTER, 680, 375, 200, 30, hPop, NULL, NULL, NULL);
    SendMessage(hAText, WM_SETFONT, (WPARAM)CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);

    HWND hEdit = CreateWindowA("EDIT", report, WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 30, 30, 620, 640, hPop, NULL, NULL, NULL);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)CreateFont(22, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);
    HWND hBack = CreateWindowA("BUTTON", GetUIText(7), WS_CHILD | WS_VISIBLE | BS_FLAT, 30, 690, 620, 65, hPop, (HMENU)888, NULL, NULL);
    SendMessage(hBack, WM_SETFONT, (WPARAM)CreateFont(24, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);
}

// --- 主視窗程序 ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    const int MIN_W = 850;
    const int MIN_H = 880;
    const double ASPECT_RATIO = (double)MIN_W / MIN_H;

    switch (msg) {
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = MIN_W;
        mmi->ptMinTrackSize.y = MIN_H;
        return 0;
    }
    case WM_SIZING: {
        RECT* pRect = (RECT*)lParam;
        int width = pRect->right - pRect->left;
        int height = pRect->bottom - pRect->top;

        switch (wParam) {
        case WMSZ_LEFT: case WMSZ_RIGHT:
            height = (int)(width / ASPECT_RATIO); pRect->bottom = pRect->top + height; break;
        case WMSZ_TOP: case WMSZ_BOTTOM:
            width = (int)(height * ASPECT_RATIO); pRect->right = pRect->left + width; break;
        case WMSZ_TOPLEFT: case WMSZ_TOPRIGHT: case WMSZ_BOTTOMLEFT: case WMSZ_BOTTOMRIGHT:
            height = (int)(width / ASPECT_RATIO); pRect->bottom = pRect->top + height; break;
        }
        return TRUE;
    }
    case WM_SIZE:
        UpdateLayout(hwnd, LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_CREATE:
        hBrushBg = CreateSolidBrush(COLOR_BG);
        InitUI(hwnd);
        SetTimer(hwnd, 1, 1000, NULL);
        ShowCountryMenu(hwnd);
        break;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)hBrushBg;
    }
    case WM_TIMER: {
        char fullTime[100]; time_t now = time(NULL);
        strftime(fullTime, sizeof(fullTime), "UTC+8: %Y-%m-%d %H:%M:%S", localtime(&now));
        SetWindowTextA(hTimeLabel, fullTime); break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 700 || id == 701) { langMode = id - 700; ShowCountryMenu(hwnd); }
        else if (id == 702) PostQuitMessage(0);
        else if (id == 999) ShowCountryMenu(hwnd);
        else if (id >= 100 && id < 104) ShowCityMenu(hwnd, id - 100);
        else if (id >= 200 && currentCountryIdx != -1) {
            City c = worldData[currentCountryIdx].cities[id - 200];
            WeatherInfo info;
            if (getWeather(c.nameEN, (char*)(langMode ? c.nameEN : c.nameTW), &info))
                ShowWeatherReport(hwnd, info);
        }
        break;
    }
    case WM_DESTROY: DeleteObject(hBrushBg); PostQuitMessage(0); break;
    default: return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(COLOR_BG);
    wc.lpszClassName = "FinalUnifiedWeatherV12";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("FinalUnifiedWeatherV12", "Satellite Weather System", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 850, 880, NULL, NULL, hInstance, NULL);
    MSG msg; while (GetMessageA(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return (int)msg.wParam;
}