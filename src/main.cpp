#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include "max6675.h"
#include <WiFiManager.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ------------------------------
// Pinos e objetos
// ------------------------------
#define CLK_PIN 5
#define DO_PIN 19
#define CS1_PIN 33
#define CS2_PIN 25
#define CS3_PIN 26
#define CS4_PIN 27
#define CS5_PIN 13
#define BUTTON_PIN 32

MAX6675 t1(CLK_PIN, CS1_PIN, DO_PIN);
MAX6675 t2(CLK_PIN, CS2_PIN, DO_PIN);
MAX6675 t3(CLK_PIN, CS3_PIN, DO_PIN);
MAX6675 t4(CLK_PIN, CS4_PIN, DO_PIN);
MAX6675 t5(CLK_PIN, CS5_PIN, DO_PIN);

Preferences prefs;
WiFiManager wm;

// ------------------------------
// Variáveis configuráveis
// ------------------------------
String serverURL = "http://carvao.imoveldiretocomodono.com.br/recebeDados/placa";
String boardName = "Placa de Teste";
String location  = "Forno 1";
unsigned long sendInterval = 15000;

// ------------------------------
// Funções auxiliares
// ------------------------------
void savePreferences() {
  prefs.begin("config", false);
  prefs.putString("serverURL", serverURL);
  prefs.putString("board", boardName);
  prefs.putString("location", location);
  prefs.putULong("interval", sendInterval);
  prefs.end();
}

void loadPreferences() {
  prefs.begin("config", true);
  serverURL = prefs.getString("serverURL", "http://carvao.imoveldiretocomodono.com.br/recebeDados/placa");
  boardName = prefs.getString("board", "Placa de Teste");
  location  = prefs.getString("location", "Forno 1");
  sendInterval = prefs.getULong("interval", 15000);
  prefs.end();
}

// ------------------------------
// Função de envio HTTP
// ------------------------------
bool sendData() {
  double temps[5];
  temps[0] = t1.readCelsius();
  temps[1] = t2.readCelsius();
  temps[2] = t3.readCelsius();
  temps[3] = t4.readCelsius();
  temps[4] = t5.readCelsius();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado, tentando reconectar...");
    WiFi.reconnect();
    delay(2000);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Falha ao reconectar Wi-Fi!");
      return false;
    }
  }

  // Monta o JSON no formato original
  String json = "{";
  json += "\"board\":\"" + boardName + "\",";
  json += "\"location\":\"" + location + "\",";
  json += "\"sensors\":[";

  for (int i = 0; i < 5; i++) {
    bool status = !isnan(temps[i]);
    json += "{";
    json += "\"name\":\"Sensor " + String(i + 1) + "\",";
    json += "\"status\":" + String(status ? "true" : "false") + ",";
    json += "\"temp\":\"" + String(status ? temps[i] : 0.0, 4) + "\"";
    json += "}";
    if (i < 4) json += ",";
  }

  json += "]}";

  // Envia via HTTP POST
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");
  int response = http.POST(json);

  Serial.println("Enviando dados...");
  Serial.println(json);
  Serial.printf("HTTP Response: %d\n", response);

  http.end();
  return response == 200;
}

// ------------------------------
// Wi-Fi com botão de configuração
// ------------------------------
void setupWiFi() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(100);

  WiFiManager wm; // cria o gerenciador local

  // Carrega preferências salvas
  loadPreferences();

  // Cria os campos personalizados
  WiFiManagerParameter custom_server("server", "Server URL", serverURL.c_str(), 100);
  WiFiManagerParameter custom_board("board", "Board Name", boardName.c_str(), 32);
  WiFiManagerParameter custom_loc("location", "Location", location.c_str(), 32);
  WiFiManagerParameter custom_interval("interval", "Interval (ms)", String(sendInterval).c_str(), 10);

  // Adiciona os parâmetros antes de iniciar o portal
  wm.addParameter(&custom_server);
  wm.addParameter(&custom_board);
  wm.addParameter(&custom_loc);
  wm.addParameter(&custom_interval);

  // Verifica se o botão foi pressionado para forçar o modo AP
  bool startAP = false;
  unsigned long pressStart = millis();

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(100);
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
      if (millis() - pressStart > 1000) {
        startAP = true;
        break;
      }
    }
  }

  if (startAP) {
    Serial.println("Modo de configuração Wi-Fi iniciado...");
    wm.resetSettings();
    bool res = wm.startConfigPortal("ConfigurarPlaca", "12345678");

    if (!res) {
      Serial.println("Falha no portal. Reiniciando...");
      delay(2000);
      ESP.restart();
    }

    // Salva as novas configurações personalizadas
    serverURL = custom_server.getValue();
    boardName = custom_board.getValue();
    location  = custom_loc.getValue();
    sendInterval = String(custom_interval.getValue()).toInt();

    savePreferences();

    Serial.println("Configurações salvas:");
    Serial.println("Server URL: " + serverURL);
    Serial.println("Board: " + boardName);
    Serial.println("Location: " + location);
    Serial.println("Interval: " + String(sendInterval));

  } else {
    // Tenta conectar usando credenciais salvas
    Serial.println("Conectando com credenciais salvas...");
    if (!wm.autoConnect("ConfigurarPlaca", "12345678")) {
      Serial.println("Falha ao conectar. Reiniciando...");
      delay(2000);
      ESP.restart();
    }
  }

  Serial.println("Wi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


// ------------------------------
// Setup e Loop
// ------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Iniciando...");

  loadPreferences();
  setupWiFi();
}

void loop() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();
    sendData();
  }

  delay(100);
}
