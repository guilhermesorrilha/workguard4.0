#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int trigPin = 5;
const int echoPin = 18;
const int ldrPin = 34;

const int ledRed = 25;
const int ledGreen = 14;
const int ledBlue = 32;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "futuro_trabalho/workguard/dados";

WiFiClient espClient;
PubSubClient client(espClient);

long duration;
int distance;
unsigned long lastMsg = 0;
bool userPresent = false;

void setup_wifi() {
  delay(10);
  lcd.setCursor(0,0);
  lcd.print("Conectando WiFi");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.setCursor(0,1);
    lcd.print(".");
  }
  lcd.clear();
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32WorkGuardClient")) {
      digitalWrite(ledBlue, HIGH); 
      delay(200); 
      digitalWrite(ledBlue, LOW);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);
  
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.backlight();
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int lightLevel = analogRead(ldrPin); 
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  userPresent = (distance < 50);

  bool badLight = (lightLevel < 500); 
  bool badTemp = (temp > 30.0 || temp < 15.0);
  
  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, LOW);

  String lcdLine1 = "";
  String lcdLine2 = "";
  String statusMsg = "";

  if (!userPresent) {
    digitalWrite(ledBlue, HIGH);
    lcdLine1 = "Status: AUSENTE";
    lcdLine2 = "Modo Eco Ativo";
    statusMsg = "Ausente";
  } 
  else if (badTemp || badLight) {
    digitalWrite(ledRed, HIGH);
    lcdLine1 = "ALERTA SAUDE!";
    statusMsg = "Risco:";
    
    if(badTemp) {
      lcdLine2 = "Temp Ruim: " + String((int)temp) + "C";
      statusMsg += " Temperatura";
    } else {
      lcdLine2 = "Luz Baixa!";
      statusMsg += " Iluminacao";
    }
  } 
  else {
    digitalWrite(ledGreen, HIGH);
    lcdLine1 = "Ambiente: BOM";
    lcdLine2 = "T:" + String((int)temp) + "C " + "LuzOK";
    statusMsg = "Produtivo";
  }

  lcd.setCursor(0, 0);
  lcd.print(lcdLine1 + "    ");
  lcd.setCursor(0, 1);
  lcd.print(lcdLine2 + "    ");

  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    
    String payload = "{";
    payload += "\"temperatura\":" + String(temp) + ",";
    payload += "\"usuario_presente\":" + String(userPresent ? "true" : "false") + ",";
    payload += "\"luminosidade\":" + String(lightLevel) + ",";
    payload += "\"status_trabalho\":\"" + statusMsg + "\"";
    payload += "}";

    client.publish(mqtt_topic, payload.c_str());
  }
  
  delay(200); 
}