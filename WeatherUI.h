// WeatherUI.h
#ifndef WEATHER_UI_H
#define WEATHER_UI_H

#include <windows.h>
#include "WeatherEngine.h" // 為了認識 Country 和 City 結構

// --- 共用全域變數宣告 (extern) ---
// 告訴 main.c 這些變數存在於 WeatherUI.c 中
extern HWND hLabel, hButtons[50], hLangBtns[3], hTimeLabel, hGroupInfo;
extern int currentCountryIdx;
extern int langMode;
extern Country worldData[]; // 讓主程式也能讀取國家資料

// --- 介面文字與設定 ---
const char* GetUIText(int id);

// --- 介面操作函式 ---
void InitUI(HWND hwnd); // 初始化建立靜態元件
void ClearButtons();
void ShowCountryMenu(HWND hwnd);
void ShowCityMenu(HWND hwnd, int countryIdx);
void UpdateLayout(HWND hwnd, int width, int height); // 處理視窗縮放

#endif