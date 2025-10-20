#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include "max6675.h"
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>

// --- Botão para reset Wi-Fi ---
#define RESET_BUTTON 0   // GPIO0

// --- Pinos MAX6675 ---
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

MAX6675 thermocouple1(CLK_PIN, CS1_PIN, DO_PIN);
MAX6675 thermocouple2(CLK_PIN, CS2_PIN, DO_PIN);
MAX6675 thermocouple3(CLK_PIN, CS3_PIN, DO_PIN);
MAX6675 thermocouple4(CLK_PIN, CS4_PIN, DO_PIN);
MAX6675 thermocouple5(CLK_PIN, CS5_PIN, DO_PIN);

WiFiManager wm;
WebServer server(80);
Preferences preferences;

// --- Parâmetros configuráveis ---
String serverURL;
unsigned long sendDelay;
String boardLocation;

// --- Carregar parâmetros salvos ---
void loadParams() {
  preferences.begin("settings", true);  // somente leitura
  serverURL = preferences.getString("serverURL", "http://carvao.imoveldiretocomodono.com.br/recebeDados/placa");
  sendDelay = preferences.getUInt("sendDelay", 15000);
  boardLocation = preferences.getString("boardLocation", "Forno 1");
  preferences.end();
}

// --- Salvar parâmetros ---
void saveParams(String url, unsigned long delayMs, String location) {
  preferences.begin("settings", false);
  preferences.putString("serverURL", url);
  preferences.putUInt("sendDelay", delayMs);
  preferences.putString("boardLocation", location);
  preferences.end();
}

// --- Página web de configuração ---
const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>Configuração ESP32</title></head>
<body>
<h2>Configurações do ESP32</h2>
<form action="/save" method="POST">
  Server URL:<br>
  <input type="text" name="serverURL" value="%SERVERURL%"><br>
  Delay (ms):<br>
  <input type="number" name="sendDelay" value="%DELAY%"><br>
  Localização da placa:<br>
  <input type="text" name="boardLocation" value="%LOCATION%"><br><br>
  <input type="submit" value="Salvar">
</form>
</body>
</html>
)rawliteral";

// --- Rotas do servidor ---
void handleRoot() {
  String page = htmlForm;
  page.replace("%SERVERURL%", serverURL);
  page.replace("%DELAY%", String(sendDelay));
  page.replace("%LOCATION%", boardLocation);
  server.send(200, "text/html", page);
}

void handleSave() {
  if (server.hasArg("serverURL") && server.hasArg("sendDelay") && server.hasArg("boardLocation")) {
    serverURL = server.arg("serverURL");
    sendDelay = server.arg("sendDelay").toInt();
    boardLocation = server.arg("boardLocation");
    saveParams(serverURL, sendDelay, boardLocation);
    server.send(200, "text/html", "<h2>Configurações salvas!</h2><a href='/'>Voltar</a>");
  } else {
    server.send(400, "text/html", "Parâmetros inválidos");
  }
}
#define BUTTON_PIN 32

MAX6675 t1(CLK_PIN, CS1_PIN, DO_PIN);
MAX6675 t2(CLK_PIN, CS2_PIN, DO_PIN);
MAX6675 t3(CLK_PIN, CS3_PIN, DO_PIN);
MAX6675 t4(CLK_PIN, CS4_PIN, DO_PIN);
MAX6675 t5(CLK_PIN, CS5_PIN, DO_PIN);

void setup() {
  Serial.begin(115200);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  delay(500);

  Serial.println("Iniciando WiFiManager...");

  if (digitalRead(RESET_BUTTON) == LOW) {
    Serial.println("Botão pressionado durante boot: resetando Wi-Fi...");
    wm.resetSettings();
    delay(1000);
  }

  if (!wm.autoConnect("ESP32_Config")) {
    Serial.println("Falha ao conectar! Reiniciando...");
    ESP.restart();
  }

  Serial.println("Wi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Carregar parâmetros salvos
  loadParams();

  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Servidor web iniciado!");
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

void loop() {
  server.handleClient();

  // Ler sensores
// ------------------------------
// Função de envio HTTP
// ------------------------------
bool sendData() {
  double temps[5];
  temps[0] = random(0, 1000);  // substituir por thermocouple1.readCelsius();
  temps[1] = random(0, 1000);
  temps[2] = random(0, 1000);
  temps[3] = random(0, 1000);
  temps[4] = random(0, 1000);

  // Montar JSON
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
  json += "\"board\":\"Placa de Teste\",";
  json += "\"location\":\"" + boardLocation + "\",";
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

  Serial.println("Enviando JSON:");
  Serial.println(json);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setReuse(false);
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(json);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
    delay(50);
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
    Serial.println("Erro: Wi-Fi desconectado!");
  }

  delay(sendDelay);
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
