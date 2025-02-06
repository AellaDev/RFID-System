#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN 8

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {5, 4, 7, 6};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

struct User {
  String cardID;
  String pin;
  bool isLocked;
  int attempts;
};

User users[] = {
  {"62:80:A8:51", "1122AABB", false, 0},
  {"D3:DC:BB:2E", "5566CCDD", false, 0},
  {"53:E6:35:34", "1155AACC", false, 0}
};
const int totalUsers = sizeof(users) / sizeof(users[0]);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  showReadyState();
}

void loop() {
  rfid.PCD_Init();
  
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(50); 
    return;
  }
  
  if (!rfid.PICC_ReadCardSerial()) {
    delay(50);  
    return;
  }

  handleCard();
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  delay(100);
  
  showReadyState();
}

void showReadyState() {
  lcd.clear();
  lcd.print("Ready for Card");
}

void handleCard() {
  String cardID = getCardID();
  lcd.clear();
  lcd.print("Card: ");
  lcd.setCursor(0, 1);
  lcd.print(cardID);
  delay(2000);

  User* user = findUser(cardID);

  if (user == nullptr || user->isLocked) {
    lcd.clear();
    lcd.print("Access Denied");
    if (user != nullptr && user->isLocked) {
      lcd.setCursor(0, 1);
      lcd.print("Card Locked");
    }
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Enter Password:");
  String enteredPin = getPassword();

  if (enteredPin == user->pin) {
    grantAccess();
    user->attempts = 0;
  } else {
    user->attempts++;
    if (user->attempts >= 3) {
      user->isLocked = true;
      lcd.clear();
      lcd.print("Card Locked");
      lcd.setCursor(0, 1);
      lcd.print("Contact Admin");
    } else {
      lcd.clear();
      lcd.print("Wrong Password");
      lcd.setCursor(0, 1);
      lcd.print(3 - user->attempts);
      lcd.print(" tries left");
    }
    delay(2000);
  }
}

String getCardID() {
  String cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      cardID += "0";
    }
    cardID += String(rfid.uid.uidByte[i], HEX);
    if (i != rfid.uid.size - 1) {
      cardID += ":";
    }
  }
  cardID.toUpperCase();
  return cardID;
}

User* findUser(String cardID) {
  for (int i = 0; i < totalUsers; i++) {
    if (users[i].cardID == cardID) {
      return &users[i];
    }
  }
  return nullptr;
}

String getPassword() {
  String password = "";
  lcd.setCursor(0, 1);
  while (password.length() < 8) {
    char key = keypad.getKey();
    if (key) {
      password += key;
      lcd.print("*");
      delay(200);
    }
  }
  return password;
}

void grantAccess() {
  lcd.clear();
  lcd.print("Access Granted");
  digitalWrite(RELAY_PIN, HIGH);
  delay(5000);
  digitalWrite(RELAY_PIN, LOW);
}
