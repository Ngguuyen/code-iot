#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// RFID
#define RST_PIN 9
#define SS_PIN 10
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Keypad
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {2, 5, 4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo
Servo servo;
int servoPin = 3;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Buzzer và LED
int buzzerPin = 6;
int ledPin = 7;

// Mật khẩu và thẻ RFID hợp lệ
String password = "1234";
String inputPassword = "";
byte validUID[] = {0xA3, 0xBE, 0xC8, 0x26}; // UID hợp lệ của thẻ RFID

void setup() {
  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // Servo
  servo.attach(servoPin);
  servo.write(0); // Cửa khóa ban đầu

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("He thong bat dau");

  // Buzzer và LED
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Serial Monitor
  Serial.begin(9600);
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Quet the hoac MK");
}

void loop() {
  // Kiểm tra thẻ RFID
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (checkRFID()) {
      openDoor("The RFID hop le");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("The RFID sai!");
      Serial.println("The RFID khong hop le!");
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
      delay(2000);
      digitalWrite(buzzerPin, LOW);
      digitalWrite(ledPin, LOW);
    }
    mfrc522.PICC_HaltA(); // Dừng giao tiếp với thẻ
  }

  // Kiểm tra nhập mật khẩu
  char key = keypad.getKey();
  if (key) {
    if (key == '#') { // Nhấn # để kiểm tra mật khẩu
      if (inputPassword == password) {
        openDoor("Mat khau dung");
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sai mat khau!");
        Serial.println("Sai mat khau!");
        digitalWrite(ledPin, HIGH);
        digitalWrite(buzzerPin, HIGH);
        delay(2000);
        digitalWrite(buzzerPin, LOW);
        digitalWrite(ledPin, LOW);
      }
      inputPassword = ""; // Reset mật khẩu sau khi kiểm tra
    } else if (key == '*') { // Nhấn * để xóa mật khẩu
      inputPassword = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Nhap lai mat khau");
    } else {
      inputPassword += key;
      lcd.setCursor(0, 1);
      lcd.print("MK: ");
      lcd.print(inputPassword);
    }
  }
}

// Hàm mở cửa
void openDoor(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);
  Serial.println(message);
  digitalWrite(ledPin, HIGH); // Bật LED báo trạng thái
  servo.write(90);           // Mở khóa
  digitalWrite(buzzerPin, HIGH); // Buzzer kêu
  delay(2000);
  digitalWrite(buzzerPin, LOW);  // Tắt buzzer
  delay(5000); // Giữ cửa mở trong 5 giây
  servo.write(0);             // Đóng khóa
  digitalWrite(ledPin, LOW);  // Tắt LED
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Quet the hoac MK");
}

// Hàm kiểm tra thẻ RFID hợp lệ
bool checkRFID() {
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != validUID[i]) {
      return false;
    }
  }
  return true;
}
