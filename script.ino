#include <WiFi.h>
#include <PubSubClient.h>

// CONFIGURAÇÃO INICIAL

const char* WIFI_SSID = "Inteli.Iot";
const char* WIFI_PASS = "%(Yk(sxGMtvFEs.3";

const char* MQTT_TOKEN = "BBUS-fT4R8TqG8CFojsJ0R6bIy0dZT8rNha";
const char* DEVICE_NAME = "esp32-signal";

// Ubidots MQTT
const char* MQTT_HOST = "industrial.api.ubidots.com";
const uint16_t MQTT_PORT = 1883;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL_MS = 2000; // publicar a cada 2s

void connectWiFi() {
  Serial.print("Conectando WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long startAttemptTime = millis();
  // tenta por 20s (ajuste se quiser)
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Falha ao conectar WiFi. Verifique credenciais.");
  }
}

bool connectMQTT() {
  if (client.connected()) return true;

  Serial.print("Conectando MQTT na Ubidots... ");
  String clientId = String(DEVICE_NAME) + "-" + String(random(0xffff), HEX);

  // Ubidots espera token como username. password pode ficar vazio.
  if (client.connect(clientId.c_str(), MQTT_TOKEN, "")) {
    Serial.println("Conectado ao MQTT.");
    return true;
  } else {
    Serial.print("Erro ao conectar MQTT, rc=");
    Serial.println(client.state());
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  randomSeed(micros());

  connectWiFi();

  client.setServer(MQTT_HOST, MQTT_PORT);
}

void publishRSSI() {
  // mede o RSSI (dBm)
  long rssi = WiFi.RSSI(); // valor em dBm (ex: -57)

  // imprime no Serial
  Serial.print("RSSI (dBm): ");
  Serial.println(rssi);

  // Monta payload JSON conforme Ubidots aceita: {"rssi": -57}
  char payload[64];
  snprintf(payload, sizeof(payload), "{\"rssi\": %ld}", rssi);

  // Tópico para publicar em device: /v1.6/devices/<DEVICE_LABEL>
  // Ubidots vai criar variáveis automaticamente com as chaves do JSON (rssi)
  char topic[128];
  snprintf(topic, sizeof(topic), "/v1.6/devices/%s", DEVICE_NAME);

  bool ok = client.publish(topic, payload);
  if (!ok) {
    Serial.println("Falha ao publicar no MQTT.");
  } else {
    Serial.print("Publicado em ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // tenta reconectar WiFi
    connectWiFi();
  }

  if (!client.connected()) {
    // tenta reconectar MQTT (com backoff simples)
    connectMQTT();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    publishRSSI();
  }
}
