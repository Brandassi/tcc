#include <Adafruit_LiquidCrystal.h>
#include <SoftwareSerial.h>

Adafruit_LiquidCrystal lcd(0);
SoftwareSerial BT(10, 11);

const int ledVerm = 7;
const int ledVerde = 4;
const int botao = 13;
const int relePin = 8;

// Se true -> módulo aciona com sinal LOW. Se false -> aciona com HIGH.
const bool RELAY_ACTIVE_LOW = true;

bool estadoSistema = false;
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setRelay(bool on) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(relePin, on ? LOW : HIGH);
  } else {
    digitalWrite(relePin, on ? HIGH : LOW);
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16,2);
  pinMode(botao, INPUT_PULLUP);
  pinMode(ledVerm, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(relePin, OUTPUT);

  // Estado inicial: desligado
  setRelay(false);
  digitalWrite(ledVerm, HIGH);
  digitalWrite(ledVerde, LOW);

  lcd.setBacklight(1);
  lcd.setCursor(0,0);
  lcd.print("Dessalinizador:");
  lcd.setCursor(0,1);
  lcd.print("Desligado");
  Serial.println("Setup pronto");
}

void loop() {
  int reading = digitalRead(botao);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    static int stableState = HIGH;
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) { // borda de descida: botão pressionado (pull-up)
        estadoSistema = !estadoSistema;
        if (estadoSistema) {
          // Ligar sistema e bomba via relé
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Dessalinizador:");
          lcd.setCursor(0,1);
          lcd.print("Ligado");
          digitalWrite(ledVerm, LOW);
          digitalWrite(ledVerde, HIGH);
          setRelay(true);
          Serial.println("BOMBA LIGADA (relé acionado)");
        } else {
          // Desligar sistema e bomba
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Dessalinizador:");
          lcd.setCursor(0,1);
          lcd.print("Desligado");
          digitalWrite(ledVerde, LOW);
          setRelay(false);
          Serial.println("BOMBA DESLIGADA (relé aberto)");
        }
      }
    }
  }

  lastButtonReading = reading;
}
