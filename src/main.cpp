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
}

void loop() {
  server.handleClient();

  // Ler sensores
  double temps[5];
  temps[0] = random(0, 1000);  // substituir por thermocouple1.readCelsius();
  temps[1] = random(0, 1000);
  temps[2] = random(0, 1000);
  temps[3] = random(0, 1000);
  temps[4] = random(0, 1000);

  // Montar JSON
  String json = "{";
  json += "\"board\":\"Placa de Teste\",";
  json += "\"location\":\"" + boardLocation + "\",";
  json += "\"sensors\":[";
  for (int i = 0; i < 5; i++) {
    bool status = !isnan(temps[i]);
    json += "{";
    json += "\"name\":\"Sensor " + String(i+1) + "\",";
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
  } else {
    Serial.println("Erro: Wi-Fi desconectado!");
  }

  delay(sendDelay);
}
