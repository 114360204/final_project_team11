#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h> 
#include "cJSON.h" 
#include <curl/curl.h>

// --- 定義資料結構 (展現 Struct 巢狀結構技巧) ---

// 1. 最底層：城市
typedef struct {
    char nameTW[50];   // 顯示名稱 (台北)
    char nameAPI[50];  // 查詢名稱 (Taipei)
} City;

// 2. 上層：國家 (包含一個城市的陣列)
typedef struct {
    char countryName[50]; // 國家名稱 (台灣)
    City cities[10];      // 該國家底下的城市列表 (假設最多10個)
    int cityCount;        // 該國家目前有多少個城市
} Country;

// --- 天氣資訊結構 ---
typedef struct {
    char city[50];
    double temperature;
    int humidity;
    char description[100];
} WeatherInfo;

struct string {
    char *ptr;
    size_t len;
};

// --- 基礎工具函式 ---
void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) exit(EXIT_FAILURE);
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) exit(EXIT_FAILURE);
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size * nmemb;
}

// ★記得填入你的 API Key
const char* API_KEY = "2fa80dc7ec8be3fac297f88afd028de9"; 

int getWeather(char* searchName, char* displayName, WeatherInfo* info) {
    CURL *curl;
    CURLcode res;
    struct string s;
    init_string(&s);

    curl = curl_easy_init();
    if(curl) {
        char url[512];
        sprintf(url, "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric&lang=zh_tw", searchName, API_KEY);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            printf("連線失敗，請檢查網路。\n");
            return 0;
        }

        cJSON *json = cJSON_Parse(s.ptr);
        if (json == NULL) return 0;

        cJSON *cod = cJSON_GetObjectItem(json, "cod");
        int statusCode = 0;
        if (cJSON_IsNumber(cod)) statusCode = cod->valueint;
        else if (cJSON_IsString(cod)) statusCode = atoi(cod->valuestring);

        if (statusCode != 200) {
             printf(">> 查詢失敗 (Code: %d) - 請檢查 API Key。\n", statusCode);
             return 0;
        }

        cJSON *main_obj = cJSON_GetObjectItem(json, "main");
        cJSON *temp = cJSON_GetObjectItem(main_obj, "temp");
        cJSON *humidity = cJSON_GetObjectItem(main_obj, "humidity");
        cJSON *weather_arr = cJSON_GetObjectItem(json, "weather");
        cJSON *weather_item = cJSON_GetArrayItem(weather_arr, 0);
        cJSON *desc = cJSON_GetObjectItem(weather_item, "description");

        strcpy(info->city, displayName);
        info->temperature = temp->valuedouble;
        info->humidity = humidity->valueint;
        strcpy(info->description, desc->valuestring);

        cJSON_Delete(json);
        curl_easy_cleanup(curl);
        free(s.ptr);
        return 1;
    }
    return 0;
}

void saveHistory(WeatherInfo info) {
    FILE *fp = fopen("history.txt", "a");
    if (fp != NULL) {
        fprintf(fp, "%s | %.1f°C | %s\n", info.city, info.temperature, info.description);
        fclose(fp);
    }
}

int main() {
    SetConsoleOutputCP(65001); // 設定 UTF-8 輸出

    // ★建立巢狀資料庫：國家 -> 城市
    // 這裡展現了如何用程式碼結構化地管理真實世界資料
    Country worldData[] = {
        // 第一個國家：台灣
        {
            "台灣", 
            {
                {"台北", "Taipei"},
                {"台中", "Taichung"},
                {"高雄", "Kaohsiung"},
                {"台南", "Tainan"},
                {"花蓮", "Hualien"}
            },
            5 // 城市數量
        },
        // 第二個國家：日本
        {
            "日本",
            {
                {"東京", "Tokyo"},
                {"大阪", "Osaka"},
                {"京都", "Kyoto"},
                {"北海道", "Hokkaido"},
                {"沖繩", "Okinawa"}
            },
            5
        },
        // 第三個國家：美國
        {
            "美國",
            {
                {"紐約", "New York"},
                {"洛杉磯", "Los Angeles"},
                {"舊金山", "San Francisco"},
                {"芝加哥", "Chicago"}
            },
            4
        },
        // 第四個國家：歐洲精選
        {
            "歐洲地區",
            {
                {"倫敦 (英國)", "London"},
                {"巴黎 (法國)", "Paris"},
                {"柏林 (德國)", "Berlin"}
            },
            3
        }
    };
    
    int countryCount = sizeof(worldData) / sizeof(worldData[0]);
    int countryChoice, cityChoice;
    WeatherInfo currentData;

    printf("=== 環球天氣觀測系統 (Global Weather System) ===\n");

    while(1) { 
        // --- 第一層選單：選國家 ---
        printf("\n請選擇國家/地區:\n");
        for(int i=0; i<countryCount; i++) {
            printf("%d. %s\n", i+1, worldData[i].countryName);
        }
        printf("0. 離開系統\n");
        printf("輸入選項: ");
        
        if (scanf("%d", &countryChoice) != 1) {
            while(getchar() != '\n');
            continue;
        }

        if (countryChoice == 0) break; // 離開程式

        if (countryChoice > 0 && countryChoice <= countryCount) {
            
            // 取得使用者選的國家 struct
            Country selectedCountry = worldData[countryChoice - 1];

            // --- 第二層選單：選該國家的城市 ---
            while(1) {
                printf("\n-- 您選擇了 [%s] --\n", selectedCountry.countryName);
                printf("請選擇城市:\n");
                for(int j=0; j<selectedCountry.cityCount; j++) {
                    printf("%d. %s\n", j+1, selectedCountry.cities[j].nameTW);
                }
                printf("0. 返回上一頁 (重選國家)\n");
                printf("輸入選項: ");

                if (scanf("%d", &cityChoice) != 1) {
                    while(getchar() != '\n');
                    continue;
                }

                if (cityChoice == 0) break; // 跳出內層迴圈，回到國家選單

                if (cityChoice > 0 && cityChoice <= selectedCountry.cityCount) {
                    // 鎖定最終目標城市
                    City targetCity = selectedCountry.cities[cityChoice - 1];

                    printf("\n正在連線至衛星查詢 %s (%s) ...\n", targetCity.nameTW, selectedCountry.countryName);

                    if (getWeather(targetCity.nameAPI, targetCity.nameTW, &currentData)) {
                        printf("\n============================\n");
                        printf(" 地區: %s - %s\n", selectedCountry.countryName, currentData.city);
                        printf(" 氣溫: %.1f °C\n", currentData.temperature);
                        printf(" 濕度: %d %%\n", currentData.humidity);
                        printf(" 現況: %s\n", currentData.description);
                        printf("============================\n");
                        saveHistory(currentData);
                        
                        printf("\n按 Enter 繼續...");
                        getchar(); getchar(); // 暫停一下讓使用者看結果
                    }
                } else {
                    printf("無效的城市選項。\n");
                }
            } // end of city loop

        } else {
            printf("無效的國家選項。\n");
        }
    } // end of main loop

    printf("系統已關閉。\n");
    return 0;
}