// CẤU HÌNH CHO BLYNK IOT
#define BLYNK_TEMPLATE_ID "TMPL6NukCryOl"
#define BLYNK_TEMPLATE_NAME "IoT"
#define BLYNK_AUTH_TOKEN "O0dvP_Ys58x4oIkNwysD788xo-w8Crgx" 

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Thông số WiFi (ESP32 chỉ hỗ trợ 2.4GHz)
char ssid[] = "Zz";       
char pass[] = "TTT667788";  

// THƯ VIỆN VÀ CẤU HÌNH CHO HỆ THỐNG
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// RFID
#define RST_PIN         2    
#define SS_PIN          5    
MFRC522 mfrc522(SS_PIN, RST_PIN);

// LCD (Địa chỉ I2C thường là 0x27 hoặc 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Cảm biến DHT
#define DHT_PIN         13         
#define DHT_TYPE        DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Servo
#define SERVO_PIN       12
Servo myServo;

// Các nút bấm
#define BTN_UP_PIN      26
#define BTN_DOWN_PIN    27
#define BTN_MENU_PIN    25

// Kích thước UID của thẻ RFID (thường là 4 bytes)
#define UID_SIZE 4

unsigned long doorOpenTime = 0;
bool doorIsOpen = false;

// --- Hàm so sánh và in UID ---
bool compareUID(byte *uid1, byte *uid2, byte size = UID_SIZE) {
  for (byte i = 0; i < size; i++) {
    if (uid1[i] != uid2[i])
      return false;
  }
  return true;
}

void printUID(byte *uid, byte size = UID_SIZE) {
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10)
      Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < size - 1)
      Serial.print(":");
  }
}

byte masterUID[UID_SIZE];
bool masterSaved = false;

#define MAX_MEMBERS 10
struct Card {
  byte uid[UID_SIZE];
};
Card members[MAX_MEMBERS];
int memberCount = 0;

bool isMember(byte *uid) {
  for (int i = 0; i < memberCount; i++) {
    if (compareUID(uid, members[i].uid))
      return true;
  }
  return false;
}

bool addMember(byte *uid) {
  if (memberCount >= MAX_MEMBERS)
    return false;
  if (isMember(uid) || compareUID(uid, masterUID))
    return false;
  memcpy(members[memberCount].uid, uid, UID_SIZE);
  memberCount++;
  return true;
}

bool deleteMember(byte *uid) {
  for (int i = 0; i < memberCount; i++) {
    if (compareUID(uid, members[i].uid)) {
      for (int j = i; j < memberCount - 1; j++) {
        memcpy(members[j].uid, members[j + 1].uid, UID_SIZE);
      }
      memberCount--;
      return true;
    }
  }
  return false;
}

// --- Định nghĩa trạng thái hệ thống ---
enum State {
  STATE_INIT,
  STATE_WAIT_AUTH,
  STATE_MENU_ACTIVE,
  STATE_ADD_CARD,
  STATE_DELETE_CARD,
  STATE_ALARM_SETTING    // Dành cho cấu hình cảnh báo nhiệt
};

State currentState = STATE_INIT;
State lastState = STATE_INIT;
bool currentUserIsMaster = false;
bool firstAuthCompleted = false;

uint8_t menuGroup = 0;
uint8_t menuItem = 0;
bool isDHTShowing = false;

// Biến cài đặt cảnh báo nhiệt độ
int alarmThreshold = 30;
bool alarmConfigured = false;  // Mặc định chưa cấu hình cảnh báo

// --- Hàm hiển thị menu chính ---
void displayMenu() {
  lcd.clear();
  if (!currentUserIsMaster) {
    menuGroup = 0;
  }
  if (menuGroup == 0) {
    if (menuItem == 0) {
      lcd.setCursor(0, 0);
      lcd.print(">hien thi do am");
      lcd.setCursor(0, 1);
      lcd.print(" mo cua        ");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print(" hien thi do am");
      lcd.setCursor(0, 1);
      lcd.print(">mo cua        ");
    }
  }
  else if (menuGroup == 1) {
    if (menuItem == 0) {
      lcd.setCursor(0, 0);
      lcd.print(">them the");
      lcd.setCursor(0, 1);
      lcd.print(" xoa the ");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print(" them the");
      lcd.setCursor(0, 1);
      lcd.print(">xoa the ");
    }
  }
  else if (menuGroup == 3) {
    if (menuItem == 0) {
      lcd.setCursor(0, 0);
      lcd.print(">cai canh bao");
      lcd.setCursor(0, 1);
      lcd.print(" xoa canh bao");
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print(" cai canh bao");
      lcd.setCursor(0, 1);
      lcd.print(">xoa canh bao");
    }
  }
}

// --- Giao diện cấu hình cảnh báo ---
void displayAlarmSetting() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarm: ");
  lcd.print(alarmThreshold);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("UP:+ DOWN:- MENU:OK");
}

// --- Xử lý nút DOWN ---
void processButtonDown() {
  if (!currentUserIsMaster) {
    menuGroup = 0;
    menuItem = (menuItem + 1) % 2;
  }
  else {
    if (menuGroup == 0) {
      if (menuItem == 0)
        menuItem = 1;
      else {
        menuGroup = 1;
        menuItem = 0;
      }
    }
    else if (menuGroup == 1) {
      if (menuItem == 0)
        menuItem = 1;
      else {
        menuGroup = 3;
        menuItem = 0;
      }
    }
    else if (menuGroup == 3) {
      if (menuItem == 0)
        menuItem = 1;
      else {
        menuGroup = 0;
        menuItem = 0;
      }
    }
  }
  displayMenu();
}

// --- Xử lý nút UP ---
void processButtonUp() {
  if (!currentUserIsMaster) {
    menuGroup = 0;
    menuItem = (menuItem + 1) % 2;
  }
  else {
    if (menuGroup == 0) {
      if (menuItem == 0) {
        menuGroup = 3;
        menuItem = 1;
      }
      else {
        menuItem = 0;
      }
    }
    else if (menuGroup == 1) {
      if (menuItem == 0) {
        menuGroup = 0;
        menuItem = 1;
      }
      else {
        menuItem = 0;
      }
    }
    else if (menuGroup == 3) {
      if (menuItem == 0) {
        menuGroup = 1;
        menuItem = 1;
      }
      else {
        menuItem = 0;
      }
    }
  }
  displayMenu();
}

void showMessage(const char* msg, unsigned long duration = 1500) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  delay(duration);
  if (currentState == STATE_MENU_ACTIVE)
    displayMenu();
  else
    updateLCDStatus();
}

void updateLCDStatus() {
  lcd.clear();
  switch (currentState) {
    case STATE_INIT:
      lcd.setCursor(0, 0);
      lcd.print("Quet Master");
      lcd.setCursor(0, 1);
      lcd.print("de luu");
      break;
    case STATE_WAIT_AUTH:
      lcd.setCursor(0, 0);
      lcd.print("Quet the de");
      lcd.setCursor(0, 1);
      lcd.print("mo menu");
      break;
    case STATE_MENU_ACTIVE:
      displayMenu();
      break;
    case STATE_ADD_CARD:
      lcd.setCursor(0, 0);
      lcd.print("Them the:");
      lcd.setCursor(0, 1);
      lcd.print("Quet the moi");
      break;
    case STATE_DELETE_CARD:
      lcd.setCursor(0, 0);
      lcd.print("Xoa the:");
      lcd.setCursor(0, 1);
      lcd.print("Quet the xoa");
      break;
    case STATE_ALARM_SETTING:
      displayAlarmSetting();
      break;
  }
}

void displayDHT() {
  lcd.clear();
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    lcd.print("Doc loi DHT11");
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Nhiet: ");
    lcd.print(t);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Do am: ");
    lcd.print(h);
    lcd.print("%");
  }
}

bool readCard(byte *uidOut) {
  if (!mfrc522.PICC_IsNewCardPresent())
    return false;
  if (!mfrc522.PICC_ReadCardSerial())
    return false;
  memcpy(uidOut, mfrc522.uid.uidByte, UID_SIZE);
  mfrc522.PICC_HaltA();
  return true;
}

bool isButtonPressed(uint8_t pin) {
  if (digitalRead(pin) == LOW) {
    delay(50);
    if (digitalRead(pin) == LOW)
      return true;
  }
  return false;
}

int menuIndex = 0;

// -----------------------
// Setup: Khởi tạo các thành phần
// -----------------------
void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID - Bat dau");
  
  lcd.init();
  lcd.backlight();
  updateLCDStatus();
  
  dht.begin();
  
  myServo.attach(SERVO_PIN);
  myServo.write(90);
  
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_MENU_PIN, INPUT_PULLUP);
  
  // KHỞI TẠO KẾT NỐI BLYNK
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  // Chạy Blynk để xử lý các sự kiện từ cloud
  Blynk.run();
  
  // --- Gửi dữ liệu cảm biến DHT lên Blynk ---
  // Gửi dữ liệu mỗi 2 giây lên virtual pin V1 (nhiệt độ) và V2 (độ ẩm)
  static unsigned long lastBlynkUpdate = 0;
  if (millis() - lastBlynkUpdate > 2000) {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) {
      Blynk.virtualWrite(V1, temp);
      Blynk.virtualWrite(V2, hum);
    }
    lastBlynkUpdate = millis();
  }
  
  // --- Các xử lý khác của hệ thống ---
  byte currentUID[UID_SIZE];
  
  if (currentState != lastState) {
    updateLCDStatus();
    lastState = currentState;
  }
  
  // Kiểm tra điều kiện cảnh báo nhiệt (nếu đã cấu hình) và không trong chế độ thêm/xóa thẻ
  if (alarmConfigured && currentState != STATE_ADD_CARD && currentState != STATE_DELETE_CARD) {
    float currentTemp = dht.readTemperature();
    if (!isnan(currentTemp) && currentTemp > alarmThreshold) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CANH BAO!");
      lcd.setCursor(0, 1);
      lcd.print("NH: " + String(currentTemp) + "C");
      delay(1000);
      return;
    }
  }
  
  // Tự động đóng cửa nếu đã mở quá 10 giây
  if (doorIsOpen && (millis() - doorOpenTime > 10000)) {
    myServo.write(0);
    doorIsOpen = false;
    Serial.println("Cua da dong");
  }
  
  if (currentState == STATE_INIT) {
    if (readCard(currentUID)) {
      memcpy(masterUID, currentUID, UID_SIZE);
      masterSaved = true;
      Serial.print("Master luu: ");
      printUID(masterUID);
      Serial.println();
      showMessage("Master da luu", 1000);
      currentState = STATE_WAIT_AUTH;
    }
    return;
  }
  
  if (currentState == STATE_WAIT_AUTH) {
    if (readCard(currentUID)) {
      if (!firstAuthCompleted) {
        if (compareUID(currentUID, masterUID)) {
          currentUserIsMaster = true;
          firstAuthCompleted = true;
          Serial.println("Xac thuc master thanh cong");
          showMessage("Master OK", 800);
          currentState = STATE_MENU_ACTIVE;
        }
        else {
          showMessage("Khong phai Master", 800);
        }
      }
      else {
        if (compareUID(currentUID, masterUID) || isMember(currentUID)) {
          currentUserIsMaster = compareUID(currentUID, masterUID);
          Serial.print("Mo menu voi the: ");
          printUID(currentUID);
          Serial.println();
          showMessage("The hop le", 800);
          currentState = STATE_MENU_ACTIVE;
        }
        else {
          showMessage("The chua luu", 800);
        }
      }
    }
    return;
  }
  
  if (currentState == STATE_MENU_ACTIVE) {
    if (readCard(currentUID)) {
      Serial.println("Dong menu do quet the");
      showMessage("Menu dong", 800);
      currentState = STATE_WAIT_AUTH;
      isDHTShowing = false;
      return;
    }
    if (isButtonPressed(BTN_DOWN_PIN)) {
      processButtonDown();
      delay(200);
    }
    if (isButtonPressed(BTN_UP_PIN)) {
      processButtonUp();
      delay(200);
    }
    if (isButtonPressed(BTN_MENU_PIN)) {
      if (menuGroup == 0) {
        if (menuItem == 0) {
          if (!isDHTShowing) {
            isDHTShowing = true;
            displayDHT();
          }
          else {
            isDHTShowing = false;
            displayMenu();
          }
        }
        else {
          myServo.write(0);
          doorOpenTime = millis();
          doorIsOpen = true;
          delay(5000);
          myServo.write(90);
          displayMenu();
        }
      }
      else if (menuGroup == 1) {
        if (currentUserIsMaster) {
          if (menuItem == 0) {
            currentState = STATE_ADD_CARD;
            updateLCDStatus();
          }
          else {
            currentState = STATE_DELETE_CARD;
            updateLCDStatus();
          }
        }
        else {
          showMessage("Khong du quyen", 1000);
        }
      }
      else if (menuGroup == 3) {
        if (menuItem == 0) {
          currentState = STATE_ALARM_SETTING;
          updateLCDStatus();
        }
        else {
          alarmConfigured = false;
          showMessage("Xoa canh bao OK", 1000);
        }
      }
      delay(300);
      return;
    }
    
    if (isDHTShowing) {
      static unsigned long lastUpdate = 0;
      if (millis() - lastUpdate > 2000) {
        displayDHT();
        lastUpdate = millis();
      }
    }
  }
  
  // Xử lý chế độ thêm thẻ
  if (currentState == STATE_ADD_CARD) {
    if (readCard(currentUID)) {
      if (!compareUID(currentUID, masterUID) && !isMember(currentUID)) {
        if (addMember(currentUID)) {
          Serial.print("Them the thanh cong: ");
          printUID(currentUID);
          Serial.println();
          showMessage("Them the OK", 1000);
        }
        else {
          showMessage("Them the that bai", 1000);
        }
      }
      else {
        showMessage("The bi trung", 1000);
      }
      currentState = STATE_MENU_ACTIVE;
      displayMenu();
    }
    return;
  }
  
  // Xử lý chế độ xóa thẻ
  if (currentState == STATE_DELETE_CARD) {
    if (readCard(currentUID)) {
      if (!compareUID(currentUID, masterUID) && isMember(currentUID)) {
        if (deleteMember(currentUID)) {
          Serial.print("Xoa the thanh cong: ");
          printUID(currentUID);
          Serial.println();
          showMessage("Xoa the OK", 1000);
        }
        else {
          showMessage("Xoa that bai", 1000);
        }
      }
      else {
        showMessage("The khong hop le", 1000);
      }
      currentState = STATE_MENU_ACTIVE;
      displayMenu();
    }
    return;
  }
  
  // --- Xử lý chế độ cấu hình cảnh báo ---
  if (currentState == STATE_ALARM_SETTING) {
    displayAlarmSetting();
    if (isButtonPressed(BTN_UP_PIN)) {
      alarmThreshold++;
      displayAlarmSetting();
      delay(200);
    }
    if (isButtonPressed(BTN_DOWN_PIN)) {
      alarmThreshold--;
      displayAlarmSetting();
      delay(200);
    }
    if (isButtonPressed(BTN_MENU_PIN)) {
      alarmConfigured = true;
      showMessage(("Alarm: " + String(alarmThreshold) + "C").c_str(), 1000);
      currentState = STATE_MENU_ACTIVE;
      displayMenu();
      delay(300);
    }
    return;
  }
}
