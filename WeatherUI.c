// WeatherUI.c
#include "WeatherUI.h"
#include <stdio.h>

// --- 全域變數定義 ---
HWND hLabel, hButtons[50], hLangBtns[3], hTimeLabel, hGroupInfo;
int currentCountryIdx = -1;
int langMode = 0; // 0: TW, 1: EN

// --- 城市數據庫 (搬移至此) ---
Country worldData[] = {
    {"台灣 (Taiwan)", "Taiwan", {
        {"基隆市", "Keelung"},{"台北市", "Taipei"},{"新北市", "New Taipei"},{"桃園市", "Taoyuan"},
        {"新竹市", "Hsinchu"},{"新竹縣", "Zhubei"},{"苗栗縣", "Miaoli"},{"台中市", "Taichung"},
        {"彰化縣", "Changhua"},{"南投縣", "Nantou"},{"雲林縣", "Douliu"},{"嘉義市", "Chiayi"},
        {"嘉義縣", "Taibao"},{"台南市", "Tainan"},{"高雄市", "Kaohsiung"},{"屏東縣", "Pingtung"},
        {"宜蘭縣", "Yilan"},{"花蓮縣", "Hualien"},{"台東縣", "Taitung"},{"澎湖縣", "Magong"},
        {"金門縣", "Jincheng"},{"連江縣 (馬祖)", "Nangan"}
    }, 22},
    {"日本 (Japan)", "Japan", {
        {"東京", "Tokyo"},{"大阪", "Osaka"},{"京都", "Kyoto"},{"北海道 (札幌)", "Sapporo"},
        {"沖繩 (那霸)", "Naha"},{"福岡", "Fukuoka"},{"名古屋", "Nagoya"}
    }, 7},
    {"美國 (USA)", "USA", {
        {"紐約", "New York"},{"洛杉磯", "Los Angeles"},{"舊金山", "San Francisco"},
        {"西雅圖", "Seattle"},{"芝加哥", "Chicago"},{"波士頓", "Boston"}
    }, 6},
    {"歐洲 (Europe)", "Europe", {
        {"倫敦 (英國)", "London"},{"巴黎 (法國)", "Paris"},{"柏林 (德國)", "Berlin"},
        {"羅馬 (義大利)", "Rome"},{"馬德里 (西班牙)", "Madrid"},{"阿姆斯特丹 (荷蘭)", "Amsterdam"}
    }, 6}
};

// --- 文字資源 ---
const char* GetUIText(int id) {
    const char* texts[2][18] = {
        {"衛星天氣觀測系統", "數據同步中...", "現在氣溫", "相對濕度", "天氣狀況", "空氣品質", "明日氣象預報趨勢", "關閉報告並返回選單", "觀測時間", "<- 返回上一層", "請選擇區域", "良好", "警戒", "狀態", "預測溫度", "預測濕度", "當天外出建議", "退出系統"},
        {"Weather System", "Syncing...", "Temperature", "Humidity", "Condition", "Air Quality", "Tomorrow's Forecast", "Close and Return", "Observed at", "<- Back", "Select Region", "Good", "Warning", "Status", "Forecast Temp", "Forecast Humid", "Outdoor Suggestion", "Exit"}
    };
    return texts[langMode][id];
}

void ClearButtons() {
    for (int i = 0; i < 50; i++) {
        if (hButtons[i]) { DestroyWindow(hButtons[i]); hButtons[i] = NULL; }
    }
}

void ShowCountryMenu(HWND hwnd) {
    ClearButtons(); currentCountryIdx = -1;
    SetWindowTextA(hLabel, GetUIText(10));
    HFONT hFontBtn = CreateFont(22, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI");
    for (int i = 0; i < 4; i++) {
        const char* name = (langMode == 0) ? worldData[i].countryTW : worldData[i].countryEN;
        hButtons[i] = CreateWindowA("BUTTON", name, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_FLAT, 100, 80 + (i * 70), 600, 50, hwnd, (HMENU)(100 + i), NULL, NULL);
        SendMessage(hButtons[i], WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    }
    ShowWindow(hGroupInfo, SW_SHOW);
}

void ShowCityMenu(HWND hwnd, int countryIdx) {
    ClearButtons(); currentCountryIdx = countryIdx;
    ShowWindow(hGroupInfo, SW_HIDE);
    HFONT hFontBtn = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI");
    for (int i = 0; i < worldData[countryIdx].cityCount; i++) {
        const char* name = (langMode == 0) ? worldData[countryIdx].cities[i].nameTW : worldData[countryIdx].cities[i].nameEN;
        hButtons[i] = CreateWindowA("BUTTON", name, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_FLAT, 50 + (i % 3) * 235, 80 + (i / 3) * 65, 220, 50, hwnd, (HMENU)(200 + i), NULL, NULL);
        SendMessage(hButtons[i], WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    }
    hButtons[49] = CreateWindowA("BUTTON", GetUIText(9), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_FLAT, 100, 750, 600, 55, hwnd, (HMENU)999, NULL, NULL);
    SendMessage(hButtons[49], WM_SETFONT, (WPARAM)hFontBtn, TRUE);
}

// 處理初始化靜態元件 (標題、時間、語系按鈕)
void InitUI(HWND hwnd) {
    hLabel = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 20, 800, 40, hwnd, NULL, NULL, NULL);
    SendMessage(hLabel, WM_SETFONT, (WPARAM)CreateFont(32, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);

    hGroupInfo = CreateWindowA("BUTTON", "開發小組資訊 / Team Info", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 100, 380, 600, 230, hwnd, NULL, NULL, NULL);
    SendMessage(hGroupInfo, WM_SETFONT, (WPARAM)CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);

    const char* teamDetail = "小組 11\r\n\r\n114360204 柯冠義\r\n114360218 吳智揚\r\n114360240 謝宇豐";
    HWND hTeamText = CreateWindowA("STATIC", teamDetail, WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 45, 560, 170, hGroupInfo, NULL, NULL, NULL);
    SendMessage(hTeamText, WM_SETFONT, (WPARAM)CreateFont(26, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI"), TRUE);

    HFONT hFontBtn = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft JhengHei UI");
    hLangBtns[0] = CreateWindowA("BUTTON", "繁體中文", WS_CHILD | WS_VISIBLE | BS_FLAT, 150, 650, 150, 45, hwnd, (HMENU)700, NULL, NULL);
    hLangBtns[1] = CreateWindowA("BUTTON", "English", WS_CHILD | WS_VISIBLE | BS_FLAT, 320, 650, 150, 45, hwnd, (HMENU)701, NULL, NULL);
    hLangBtns[2] = CreateWindowA("BUTTON", "Exit / 退出", WS_CHILD | WS_VISIBLE | BS_FLAT, 490, 650, 150, 45, hwnd, (HMENU)702, NULL, NULL);
    for (int i = 0; i < 3; i++) SendMessage(hLangBtns[i], WM_SETFONT, (WPARAM)hFontBtn, TRUE);

    hTimeLabel = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 715, 800, 30, hwnd, NULL, NULL, NULL);
    SendMessage(hTimeLabel, WM_SETFONT, (WPARAM)CreateFont(22, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 0, 0, "Consolas"), TRUE);
}

// 處理視窗縮放邏輯 (RWD)
void UpdateLayout(HWND hwnd, int newWidth, int newHeight) {
    if (hLabel) MoveWindow(hLabel, 0, 20, newWidth, 40, TRUE);

    int btnWidth = 600;
    if (newWidth < 600) btnWidth = newWidth - 100;
    int startX = (newWidth - btnWidth) / 2;

    for (int i = 0; i < 4; i++) {
        if (hButtons[i] && currentCountryIdx == -1) {
            MoveWindow(hButtons[i], startX, 80 + (i * 70), btnWidth, 50, TRUE);
        }
    }

    if (currentCountryIdx != -1) {
        int cityBtnW = (newWidth - 100) / 3;
        for (int i = 0; i < 30; i++) {
            if (hButtons[i]) {
                MoveWindow(hButtons[i], 50 + (i % 3) * (cityBtnW + 15), 80 + (i / 3) * 65, cityBtnW, 50, TRUE);
            }
        }
        if (hButtons[49]) MoveWindow(hButtons[49], startX, newHeight - 130, btnWidth, 55, TRUE);
    }
    if (hGroupInfo) MoveWindow(hGroupInfo, startX, 380, btnWidth, 230, TRUE);

    int langBtnX = (newWidth - 490) / 2;
    for (int i = 0; i < 3; i++) {
        if (hLangBtns[i]) MoveWindow(hLangBtns[i], langBtnX + (i * 170), newHeight - 230, 150, 45, TRUE);
    }
    if (hTimeLabel) MoveWindow(hTimeLabel, 0, newHeight - 165, newWidth, 30, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
}