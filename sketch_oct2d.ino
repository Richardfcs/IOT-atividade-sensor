#include <WiFi.h>
#include "DHT.h"
#include <PubSubClient.h> // Para MQTT
#include <ArduinoJson.h>    // Para criar o JSON de forma segura

// =========== Configurações de Wi-Fi =============
const char* ssid = "[Nome da Rede WI-Fi]";      // Coloque o nome da sua rede Wi-Fi
const char* password = "[Senha do seu Wi-fi]"; // Coloque a senha da sua rede Wi-Fi

// =========== Configurações do MQTT =============
const char* mqtt_server = "broker.hivemq.com"; // Broker MQTT público e gratuito
const int mqtt_port = 1883;
// Tópico para onde os dados serão enviados. Pense nisso como um "canal".
const char* mqtt_topic = "br/com/meuprojeto/esp32c3/sensor"; 
// ID único para seu dispositivo. Mude se tiver mais de um.
const char* client_id = "esp32-c3-supermini-richard";

// =========== Configurações do Sensor DHT11 =============
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// =========== Lógica de Filtro/Média (Sua lógica original, que é ótima) =============
const int MAX_SAMPLES = 10;
float lastTemps[MAX_SAMPLES];
int sampleCount = 0;
const float MIN_TEMP = 0.0;
const float MAX_TEMP = 50.0;

// =========== Controle de Tempo =============
const unsigned long INTERVAL = 5000;
unsigned long lastSendTime = 0;

// =========== Objetos de Rede =============
WiFiClient espClient;
PubSubClient client(espClient);

// Função para conectar ao Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Função para reconectar ao Broker MQTT se a conexão cair
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT Broker...");
    if (client.connect(client_id)) {
      Serial.println("Conectado!");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

// Função para calcular a média (Sua função original)
float calcAverage() {
  float sum = 0.0;
  int cnt = 0;
  for (int i = 0; i < MAX_SAMPLES; i++) {
    if (!isnan(lastTemps[i])) {
      sum += lastTemps[i];
      cnt++;
    }
  }
  return (cnt > 0) ? (sum / cnt) : NAN;
}


void setup() {
  Serial.begin(115200);
  
  // Inicializar buffer de temperatura
  for (int i = 0; i < MAX_SAMPLES; i++) {
    lastTemps[i] = NAN;
  }
  
  dht.begin();
  setup_wifi();
  
  // Aponta o cliente MQTT para o servidor/broker
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  // Garante que estamos sempre conectados ao Broker
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop(); // Essencial para manter a conexão MQTT viva

  unsigned long now = millis();
  if (now - lastSendTime < INTERVAL) {
    return;
  }
  lastSendTime = now;

  // --- Lógica de Leitura e Filtro (Sua lógica original) ---
  float tempC = dht.readTemperature();
  float hum = dht.readHumidity();
  float usedTemp = NAN;
  float usedHum = NAN;

  if (isnan(tempC) || isnan(hum)) {
    Serial.println("Falha ao ler sensor. Usando média se disponível.");
    usedTemp = calcAverage();
    usedHum = NAN; // Não temos como calcular média da umidade ainda
  } else if (tempC < MIN_TEMP || tempC > MAX_TEMP) {
    Serial.print("Leitura de temperatura fora dos limites: ");
    Serial.println(tempC);
    usedTemp = calcAverage();
    usedHum = hum; // Podemos usar a umidade, pois apenas a temp estava fora
  } else {
    // Leitura boa
    usedTemp = tempC;
    usedHum = hum;
    lastTemps[sampleCount % MAX_SAMPLES] = tempC;
    sampleCount++;
  }

  // --- Envio dos Dados via MQTT ---
  if (!isnan(usedTemp) && !isnan(usedHum)) {
    // Cria o objeto JSON
    JsonDocument doc;
    doc["temperature"] = usedTemp;
    doc["humidity"] = usedHum;

    // Converte o JSON para uma string
    char json_output[128];
    serializeJson(doc, json_output);

    // Publica a string no tópico MQTT
    Serial.print("Publicando no tópico ");
    Serial.print(mqtt_topic);
    Serial.print(": ");
    Serial.println(json_output);
    
    client.publish(mqtt_topic, json_output);
  } else {
    Serial.println("Dados inválidos, pulando envio.");
  }
}