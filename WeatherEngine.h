#ifndef WEATHER_ENGINE_H
#define WEATHER_ENGINE_H

#include <windows.h>

// --- 資料結構定義 ---
typedef struct { char nameTW[50]; char nameEN[50]; } City;
typedef struct { 
    char countryTW[50]; 
    char countryEN[50]; 
    City cities[30]; 
    int cityCount; 
} Country;

typedef struct { 
    char city[50]; 
    double temperature; 
    int humidity; 
    char description[100]; 
    char tomorrow_brief[512]; 
    int aqi; 
    char suggestion[256]; 
} WeatherInfo;

// --- 全域變數宣告 ---
extern Country worldData[];
extern int langMode;

// --- 函式介面 ---
void SetEngineLanguage(int mode);        // 設定語言 (0: TW, 1: EN)
const char* GetUIText(int id);           // 取得介面文字
void UTF8ToANSI(char* src, char* dest, int destSize); // 編碼轉換
int getWeather(char* searchName, char* displayName, WeatherInfo* info); // 抓取天氣

#endif