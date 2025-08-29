#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include "max6675.h"

// --- Configurações Wi-Fi ---
const char* ssid = "iPhone";
const char* password = "0011223344";

// --- URL do servidor ---
const char* serverURL = "http://carvao.imoveldiretocomodono.com.br/recebeDados/placa";

// --- Pinos MAX6675 ---
#define CLK_PIN 5
#define DO_PIN 19
#define CS1_PIN 33
#define CS2_PIN 25
#define CS3_PIN 26
#define CS4_PIN 27
#define CS5_PIN 13

// --- Instâncias MAX6675 ---
MAX6675 thermocouple1(CLK_PIN, CS1_PIN, DO_PIN);
MAX6675 thermocouple2(CLK_PIN, CS2_PIN, DO_PIN);
MAX6675 thermocouple3(CLK_PIN, CS3_PIN, DO_PIN);
MAX6675 thermocouple4(CLK_PIN, CS4_PIN, DO_PIN);
MAX6675 thermocouple5(CLK_PIN, CS5_PIN, DO_PIN);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Conectando ao Wi-Fi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Ler temperaturas
  double temps[5];
  temps[0] = thermocouple1.readCelsius();
  temps[1] = thermocouple2.readCelsius();
  temps[2] = thermocouple3.readCelsius();
  temps[3] = thermocouple4.readCelsius();
  temps[4] = thermocouple5.readCelsius();

  // Montar JSON
  String json = "{";
  json += "\"board\":\"Placa de Teste\",";
  json += "\"location\":\"Forno 1\",";
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

  // --- Enviar POST HTTP ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(json);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    http.end();
  } else {
    Serial.println("Erro: Wi-Fi desconectado!");
  }

  delay(15000); // enviar a cada 15 segundos
}
