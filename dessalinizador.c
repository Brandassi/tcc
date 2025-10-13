#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BT_RX = 10;
const int BT_TX = 11;
SoftwareSerial BT(BT_RX, BT_TX);

const int ledVerm = 7;
const int ledVerde = 4;
const int botao = 13;
const int relePin = 8;

const bool RELAY_ACTIVE_LOW = true;

bool estadoSistema = false;

int lastButtonReading = HIGH;
int stableState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

const long BT_BAUD = 9600;
String btBuffer = "";
unsigned long lastBtCharTime = 0;
const unsigned long BT_READ_TIMEOUT = 50;

void setRelay(bool on) {
  if (RELAY_ACTIVE_LOW) digitalWrite(relePin, on ? LOW : HIGH);
  else digitalWrite(relePin, on ? HIGH : LOW);
}

void printStatusToLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dessalinizador:");
  lcd.setCursor(0, 1);
  if (estadoSistema) lcd.print("Ligado  ");
  else lcd.print("Desligado");
}

String sanitizeCmd(const String &s) {
  String out = s;
  out.trim();
  String filtered = "";
  for (unsigned int i = 0; i < out.length(); ++i) {
    char c = out.charAt(i);
    if (c >= 32 && c <= 126) filtered += c;
  }
  return filtered;
}

void setup() {
  Serial.begin(9600);
  BT.begin(BT_BAUD);
  lcd.init();
  lcd.backlight();

  pinMode(botao, INPUT_PULLUP);
  pinMode(ledVerm, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(relePin, OUTPUT);

  estadoSistema = false;
  setRelay(false);
  digitalWrite(ledVerm, HIGH);
  digitalWrite(ledVerde, LOW);

  printStatusToLCD();
  Serial.println("Setup pronto");
}

void ligarPorBotao() {
  estadoSistema = true;
  digitalWrite(ledVerm, LOW);
  digitalWrite(ledVerde, HIGH);
  setRelay(true);
  Serial.println("BOMBA LIGADA (botao)");
  printStatusToLCD();
}

void desligarPorBotao() {
  estadoSistema = false;
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVerm, HIGH);
  setRelay(false);
  Serial.println("BOMBA DESLIGADA (botao)");
  printStatusToLCD();
}

void ligarPorBT_a() {
  estadoSistema = true;
  digitalWrite(ledVerm, LOW);
  digitalWrite(ledVerde, HIGH);
  setRelay(true);
  Serial.println("BOMBA LIGADA (Recebido  via BT- 'a')");
  printStatusToLCD();
}

void desligarPorBT_b() {
  estadoSistema = false;
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledVerm, HIGH);
  setRelay(false);
  Serial.println("BOMBA DESLIGADA (Recebido via BT - 'b')");
  printStatusToLCD();
}

void processBTCommand(const String &raw) {
  String cmd = sanitizeCmd(raw);
  if (cmd.length() == 0) return;

  char first = cmd.charAt(0);

  if (first == 'a' || first == 'A') {
    if (!estadoSistema) {
      ligarPorBT_a();
      BT.println("ACK:ON");
    } else {
      BT.println("ACK:ALREADY_ON");
    }
    return;
  } else if (first == 'b' || first == 'B') {
    if (estadoSistema) {
      desligarPorBT_b();
      BT.println("ACK:OFF");
    } else {
      BT.println("ACK:ALREADY_OFF");
    }
    return;
  }

  String up = cmd;
  up.toUpperCase();
  if (up == "ON" || up == "LIGAR" || up == "1") {
    if (!estadoSistema) {
      ligarPorBT_a();
      BT.println("ACK:ON");
    } else {
      BT.println("ACK:ALREADY_ON");
    }
  } else if (up == "OFF" || up == "DESLIGAR" || up == "0") {
    if (estadoSistema) {
      desligarPorBT_b();
      BT.println("ACK:OFF");
    } else {
      BT.println("ACK:ALREADY_OFF");
    }
  } else if (up == "STATUS") {
    BT.println(estadoSistema ? "STATUS:ON" : "STATUS:OFF");
  } else {
    BT.print("ERR:CMD?");
    BT.println(cmd);
  }
}

void loop() {
  int reading = digitalRead(botao);
  if (reading != lastButtonReading) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) {
        if (!estadoSistema) ligarPorBotao();
        else desligarPorBotao();
      }
    }
  }
  lastButtonReading = reading;

  while (BT.available()) {
    char c = BT.read();
    btBuffer += c;
    lastBtCharTime = millis();
    if (c == '\n') {
      processBTCommand(btBuffer);
      btBuffer = "";
    }
  }

  if (btBuffer.length() > 0 && (millis() - lastBtCharTime) > BT_READ_TIMEOUT) {
    processBTCommand(btBuffer);
    btBuffer = "";
  }
}
