#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h> 
#include "cJSON.h" 
#include <curl/curl.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// --- 資料結構 ---
typedef struct { char nameTW[50]; char nameAPI[50]; } City;
typedef struct { char countryName[50]; City cities[30]; int cityCount; } Country;

typedef struct {
    char city[50];
    double temperature;
    int humidity;
    char description[100];
    char tomorrow_brief[512];
    int aqi;
} WeatherInfo;

// --- 輔助函式 ---
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

const char* API_KEY = "2fa80dc7ec8be3fac297f88afd028de9";

// --- API 邏輯 ---
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

const char* getAQIDesc(int aqi) {
    switch (aqi) {
    case 1: return "優"; case 2: return "普通"; case 3: return "輕度污染";
    case 4: return "中度污染"; case 5: return "重度污染"; default: return "未知";
    }
}

int getWeather(char* searchName, char* displayName, WeatherInfo* info) {
    CURL* curl; struct string s; init_string(&s); curl = curl_easy_init();
    if (!curl) return 0;
    char* encodedName = curl_easy_escape(curl, searchName, 0);
    char url[512]; sprintf(url, "http://api.openweathermap.org/data/2.5/forecast?q=%s&appid=%s&units=metric&lang=zh_tw", encodedName, API_KEY);
    curl_free(encodedName);
    curl_easy_setopt(curl, CURLOPT_URL, url); curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc); curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    if (curl_easy_perform(curl) != CURLE_OK) { free(s.ptr); return 0; }
    cJSON* json = cJSON_Parse(s.ptr); if (!json) { free(s.ptr); return 0; }
    cJSON* city_obj = cJSON_GetObjectItem(json, "city"); cJSON* coord = cJSON_GetObjectItem(city_obj, "coord");
    double lat = cJSON_GetObjectItem(coord, "lat")->valuedouble; double lon = cJSON_GetObjectItem(coord, "lon")->valuedouble;
    cJSON* list = cJSON_GetObjectItem(json, "list"); cJSON* cur = cJSON_GetArrayItem(list, 0);

    info->temperature = cJSON_GetObjectItem(cJSON_GetObjectItem(cur, "main"), "temp")->valuedouble;
    info->humidity = cJSON_GetObjectItem(cJSON_GetObjectItem(cur, "main"), "humidity")->valueint;
    strcpy(info->city, displayName);
    UTF8ToANSI(cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(cur, "weather"), 0), "description")->valuestring, info->description, 100);

    double t_max = -99, t_min = 99, h_sum = 0; char tomorrow_desc[100] = ""; int count = 0;
    for (int i = 8; i < 16; i++) {
        cJSON* item = cJSON_GetArrayItem(list, i); if (!item) break;
        cJSON* m = cJSON_GetObjectItem(item, "main");
        double t = cJSON_GetObjectItem(m, "temp")->valuedouble; int h = cJSON_GetObjectItem(m, "humidity")->valueint;
        if (t > t_max) t_max = t; if (t < t_min) t_min = t;
        h_sum += h; count++;
        if (i == 12) UTF8ToANSI(cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(item, "weather"), 0), "description")->valuestring, tomorrow_desc, 100);
    }
    info->aqi = getAQI(lat, lon);
    sprintf(info->tomorrow_brief, "  - 預測溫度  : %.1f ~ %.1f C\r\n  - 預測濕度  : 約 %.0f %%\r\n  - 天氣狀況  : %s\r\n  - 環境預估  : 空氣品質 [%s]",
        t_min, t_max, (count > 0 ? h_sum / count : 0), tomorrow_desc, getAQIDesc(info->aqi));

    cJSON_Delete(json); curl_easy_cleanup(curl); free(s.ptr); return 1;
}

// --- 大視窗報告介面 ---
void ShowWeatherReport(HWND owner, WeatherInfo info) {
    char report[2048];
    char timeStr[100];
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", t);

    sprintf(report, "【 %s - 全方位氣象觀測報告 】\r\n\r\n"
        " [ 當前環境即時數據 ]\r\n"
        " ------------------------------------------\r\n"
        "  > 現在氣溫  : %.1f C\r\n"
        "  > 相對濕度  : %d %%\r\n"
        "  > 天氣狀況  : %s\r\n"
        "  > 空氣品質  : %s (AQI %d)\r\n\r\n"
        " [ 明日氣象預報趨勢 ]\r\n"
        " ------------------------------------------\r\n"
        "%s\r\n\r\n"
        " [ 系統觀測提示 ]\r\n"
        " ------------------------------------------\r\n"
        "  觀測時間 : %s\r\n"
        "  %s",
        info.city, info.temperature, info.humidity, info.description, getAQIDesc(info.aqi), info.aqi, info.tomorrow_brief,
        timeStr,
        (info.aqi > 2 ? " [!] 提醒: 偵測到空氣品質不佳，建議佩戴口罩。" : " [+] 提示: 目前環境良好，適合進行戶外活動。"));

    HWND hPop = CreateWindowA("STATIC", "衛星大數據氣象報告", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 920, 800, owner, NULL, NULL, NULL);

    // 1. 天氣圖形 (ASCII)
    char asciiWeather[512], weatherStatus[100];
    if (strstr(info.description, "晴") || strstr(info.description, "Clear")) {
        sprintf(asciiWeather, "      \\   /    \r\n       .-.     \r\n    -- (   ) -- \r\n       `-`     \r\n      /   \\    ");
        sprintf(weatherStatus, "--- 晴 朗 ---");
    }
    else if (strstr(info.description, "雨") || strstr(info.description, "Rain")) {
        sprintf(asciiWeather, "     .--.      \r\n    (    ).    \r\n   (___(__)    \r\n    ' ' ' '    \r\n   ' ' ' '     ");
        sprintf(weatherStatus, "--- 下 雨 ---");
    }
    else if (strstr(info.description, "雲") || strstr(info.description, "陰") || strstr(info.description, "Clouds")) {
        sprintf(asciiWeather, "      .--.     \r\n   .-(    ).   \r\n  (___.__)__)  \r\n               \r\n               ");
        sprintf(weatherStatus, "--- 多 雲 ---");
    }
    else {
        sprintf(asciiWeather, "               \r\n      .-.      \r\n     (   )     \r\n      `-`      \r\n               ");
        sprintf(weatherStatus, "--- 觀測中 ---");
    }

    HWND hWText = CreateWindowA("STATIC", asciiWeather, WS_CHILD | WS_VISIBLE | SS_LEFT, 680, 60, 200, 110, hPop, NULL, NULL, NULL);
    SendMessage(hWText, WM_SETFONT, (WPARAM)CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Courier New"), TRUE);
    HWND hWStatus = CreateWindowA("STATIC", weatherStatus, WS_CHILD | WS_VISIBLE | SS_CENTER, 680, 175, 180, 30, hPop, NULL, NULL, NULL);
    SendMessage(hWStatus, WM_SETFONT, (WPARAM)CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei"), TRUE);

    // 2. 空品圖形 (去除外框簡潔版)
    char asciiAQI[512], aqiStatus[100];
    if (info.aqi <= 2) {
        sprintf(asciiAQI, "      SAFE     \r\n               \r\n     (^_^)     \r\n               \r\n      CLEAN    ");
        sprintf(aqiStatus, "狀態: 良好 (+)");
    }
    else {
        sprintf(asciiAQI, "     WARNING   \r\n               \r\n     (!_!)     \r\n               \r\n      POLLUTED ");
        sprintf(aqiStatus, "狀態: 警戒 (!)");
    }

    HWND hAText = CreateWindowA("STATIC", asciiAQI, WS_CHILD | WS_VISIBLE | SS_LEFT, 680, 280, 200, 110, hPop, NULL, NULL, NULL);
    SendMessage(hAText, WM_SETFONT, (WPARAM)CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Courier New"), TRUE);
    HWND hAStatus = CreateWindowA("STATIC", aqiStatus, WS_CHILD | WS_VISIBLE | SS_CENTER, 680, 395, 180, 30, hPop, NULL, NULL, NULL);
    SendMessage(hAStatus, WM_SETFONT, (WPARAM)CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei"), TRUE);

    // 3. 文字內容區
    HWND hEdit = CreateWindowA("EDIT", report, WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 30, 30, 620, 680, hPop, NULL, NULL, NULL);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)CreateFont(24, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei"), TRUE);
}

// --- 完整的城市清單資料庫 ---
Country worldData[] = {
    {"台灣 (Taiwan)", {
        {"基隆市", "Keelung"},{"台北市", "Taipei"},{"新北市", "New Taipei"},{"桃園市", "Taoyuan"},
        {"新竹市", "Hsinchu"},{"新竹縣", "Zhubei"},{"苗栗縣", "Miaoli"},{"台中市", "Taichung"},
        {"彰化縣", "Changhua"},{"南投縣", "Nantou"},{"雲林縣", "Douliu"},{"嘉義市", "Chiayi"},
        {"嘉義縣", "Taibao"},{"台南市", "Tainan"},{"高雄市", "Kaohsiung"},{"屏東縣", "Pingtung"},
        {"宜蘭縣", "Yilan"},{"花蓮縣", "Hualien"},{"台東縣", "Taitung"},{"澎湖縣", "Magong"},
        {"金門縣", "Jincheng"},{"連江縣 (馬祖)", "Nangan"}
    }, 22},
    {"日本 (Japan)", {
        {"東京", "Tokyo"},{"大阪", "Osaka"},{"京都", "Kyoto"},{"北海道 (札幌)", "Sapporo"},
        {"沖繩 (那霸)", "Naha"},{"福岡", "Fukuoka"},{"名古屋", "Nagoya"}
    }, 7},
    {"美國 (USA)", {
        {"紐約", "New York"},{"洛杉磯", "Los Angeles"},{"舊金山", "San Francisco"},
        {"西雅圖", "Seattle"},{"芝加哥", "Chicago"},{"波士頓", "Boston"}
    }, 6},
    {"歐洲 (Europe)", {
        {"倫敦 (英國)", "London"},{"巴黎 (法國)", "Paris"},{"柏林 (德國)", "Berlin"},
        {"羅馬 (義大利)", "Rome"},{"馬德里 (西班牙)", "Madrid"},{"阿姆斯特丹 (荷蘭)", "Amsterdam"}
    }, 6}
};

int countryCount = 4; HWND hLabel; HWND hButtons[50]; int currentCountryIdx = -1;

void ClearButtons() { for (int i = 0; i < 50; i++) { if (hButtons[i]) { DestroyWindow(hButtons[i]); hButtons[i] = NULL; } } }
void ShowCountryMenu(HWND hwnd) {
    ClearButtons(); currentCountryIdx = -1; SetWindowTextA(hLabel, "衛星全方位天氣觀測系統 - 請選擇區域");
    for (int i = 0; i < countryCount; i++) { hButtons[i] = CreateWindowA("BUTTON", worldData[i].countryName, WS_TABSTOP | WS_VISIBLE | WS_CHILD, 100, 80 + (i * 70), 600, 50, hwnd, (HMENU)(100 + i), NULL, NULL); }
}
void ShowCityMenu(HWND hwnd, int countryIdx) {
    ClearButtons(); currentCountryIdx = countryIdx;
    for (int i = 0; i < worldData[countryIdx].cityCount; i++) {
        hButtons[i] = CreateWindowA("BUTTON", worldData[countryIdx].cities[i].nameTW, WS_TABSTOP | WS_VISIBLE | WS_CHILD, 50 + (i % 3) * 235, 80 + (i / 3) * 65, 220, 50, hwnd, (HMENU)(200 + i), NULL, NULL);
    }
    hButtons[49] = CreateWindowA("BUTTON", "<- 返回上一層", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 100, 750, 600, 55, hwnd, (HMENU)999, NULL, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hLabel = CreateWindowA("STATIC", "歡迎使用衛星大數據觀測站", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 20, 800, 40, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel, WM_SETFONT, (WPARAM)CreateFont(28, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei"), TRUE);
        ShowCountryMenu(hwnd); break;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 999) ShowCountryMenu(hwnd);
        else if (id >= 100 && id < 100 + countryCount) ShowCityMenu(hwnd, id - 100);
        else if (id >= 200 && currentCountryIdx != -1) {
            City c = worldData[currentCountryIdx].cities[id - 200];
            WeatherInfo info; SetWindowTextA(hLabel, "數據同步中...");
            if (getWeather(c.nameAPI, c.nameTW, &info)) { ShowWeatherReport(hwnd, info); SetWindowTextA(hLabel, "觀測報告完成。"); }
        }
        break;
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main() {
    WNDCLASSA wc = { 0 }; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandle(NULL); wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); wc.lpszClassName = "WeatherFinal"; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("WeatherFinal", "衛星全方位天氣觀測系統", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 850, 880, NULL, NULL, wc.hInstance, NULL);
    MSG msg; while (GetMessageA(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return 0;
}