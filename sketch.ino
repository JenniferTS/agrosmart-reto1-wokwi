/*
 * AgroSmart · Reto 1 (Wokwi) — completo
 * ESP32 + LCD 20x4 + Servo riego (GPIO 18) + LED ventilador (GPIO 23)
 *
 *  - Serial: 4 preguntas → JSON MQTT → Node-RED → Gemini
 *  - LCD: muestra diagnóstico (tópico agro/<user>/ia)
 *  - Servo/LED: obedecen MQTT agro/<user>/servo y .../ventilador
 *    y también se mueven según REGAR:/VENT: del texto de la IA
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- secrets (all-in-one) ---
// Wokwi: la Wi-Fi del simulador es siempre Wokwi-GUEST (sin clave).
// El bróker MQTTS es el real del grupo (puerto 8883).

#define WIFI_SSID   "Wokwi-GUEST"
#define WIFI_PASS   ""

#define MQTT_HOST   "172-214-154-175.sslip.io"
#define MQTT_PORT   8883
#define MQTT_USER   "sharon"
#define MQTT_PASS   "tecsup2026"

// En Wokwi la validación del certificado CA a veces se cuelga en "MQTT TLS...".
// 1 = cifrar sin verificar el cert (OK para la simulación). En placa real podés poner 0.
#define MQTT_TLS_INSECURE 1

// --- root CA ---
// Certificado raíz de Let's Encrypt (ISRG Root X1), válido hasta 2035.
// Con él el ESP32 verifica el certificado del bróker MQTTS.

static const char ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";


LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiClientSecure net;
PubSubClient mqtt(net);

const int PIN_VENTILADOR = 23;
const int PIN_SERVO      = 18;
const int ANGULO_CERRADO = 0;
const int ANGULO_ABIERTO = 90;

const char* T_DATOS  = "agro/" MQTT_USER "/datos";
const char* T_STATUS = "agro/" MQTT_USER "/status";
const char* T_IA     = "agro/" MQTT_USER "/ia";
const char* T_SERVO  = "agro/" MQTT_USER "/servo";
const char* T_VENT   = "agro/" MQTT_USER "/ventilador";
const char* T_SERVO_EST = "agro/" MQTT_USER "/servo/estado";
const char* T_VENT_EST  = "agro/" MQTT_USER "/ventilador/estado";

volatile bool iaLista = false;
char iaTexto[220];

void moverServo(int grados) {
  grados = constrain(grados, 0, 180);
  uint32_t us = 500 + (uint32_t)grados * 2000 / 180;
  uint32_t duty = (uint32_t)((uint64_t)us * 16383 / 20000);
  ledcWrite(PIN_SERVO, duty);
}

void setRiego(bool abrir) {
  moverServo(abrir ? ANGULO_ABIERTO : ANGULO_CERRADO);
  mqtt.publish(T_SERVO_EST, abrir ? "ABIERTO" : "CERRADO", true);
  Serial.printf("[ACT] Servo riego %s\n", abrir ? "ABIERTO" : "CERRADO");
}

void setVentilador(bool on) {
  digitalWrite(PIN_VENTILADOR, on ? HIGH : LOW);
  mqtt.publish(T_VENT_EST, on ? "ON" : "OFF", true);
  Serial.printf("[ACT] Ventilador %s\n", on ? "ON" : "OFF");
}

void lcdClearLine(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print("                    ");
}

void lcdTitulo(const char* l0, const char* l1 = "", const char* l2 = "", const char* l3 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l0);
  lcd.setCursor(0, 1); lcd.print(l1);
  lcd.setCursor(0, 2); lcd.print(l2);
  lcd.setCursor(0, 3); lcd.print(l3);
}

void lcdLinea(uint8_t row, const char* txt) {
  lcdClearLine(row);
  lcd.setCursor(0, row);
  char buf[21];
  snprintf(buf, sizeof(buf), "%-20.20s", txt ? txt : "");
  lcd.print(buf);
}

void mostrarIaEnLcd(const char* texto) {
  char copia[220];
  strlcpy(copia, texto ? texto : "", sizeof(copia));
  const char* lineas[4] = { "", "", "", "" };
  uint8_t n = 0;
  char* start = copia;
  for (char* p = copia; n < 4; p++) {
    if (*p == '\n') {
      *p = 0;
      lineas[n++] = start;
      start = p + 1;
    } else if (*p == 0) {
      lineas[n++] = start;
      break;
    }
  }
  lcd.clear();
  for (uint8_t i = 0; i < 4; i++) lcdLinea(i, lineas[i]);
  Serial.println("[LCD] Diagnostico IA:");
  Serial.println(texto);

  // Mueve actuadores según la 1.ª línea: "REGAR:SI VENT:ON"
  if (strstr(texto, "REGAR:SI")) setRiego(true);
  else if (strstr(texto, "REGAR:NO")) setRiego(false);
  if (strstr(texto, "VENT:ON")) setVentilador(true);
  else if (strstr(texto, "VENT:OFF")) setVentilador(false);
}

void lcdPregunta(uint8_t n, uint8_t total, const char* titulo) {
  lcd.clear();
  char cab[21];
  snprintf(cab, sizeof(cab), "Dato %u/%u", n, total);
  lcd.setCursor(0, 0); lcd.print(cab);
  lcd.setCursor(0, 1); lcd.print(titulo);
  lcd.setCursor(0, 2); lcd.print("Escribe en Serial");
  lcd.setCursor(0, 3); lcd.print("y pulsa Enter...");
}

void onMqttMsg(char* topic, byte* payload, unsigned int len) {
  char msg[96];
  if (len >= sizeof(msg)) len = sizeof(msg) - 1;
  memcpy(msg, payload, len);
  msg[len] = 0;

  if (strcmp(topic, T_IA) == 0) {
    if (len >= sizeof(iaTexto)) len = sizeof(iaTexto) - 1;
    memcpy(iaTexto, payload, len);
    iaTexto[len] = 0;
    iaLista = true;
    Serial.printf("[MQTT] IA recibida (%u bytes)\n", len);
    return;
  }

  String v = String(msg);
  v.trim();
  v.toUpperCase();
  if (strcmp(topic, T_SERVO) == 0) {
    if (v == "ABRIR") setRiego(true);
    else if (v == "CERRAR") setRiego(false);
  } else if (strcmp(topic, T_VENT) == 0) {
    if (v == "ON") setVentilador(true);
    else if (v == "OFF") setVentilador(false);
  }
}

void mqttSuscribir() {
  mqtt.subscribe(T_IA, 1);
  mqtt.subscribe(T_SERVO, 1);
  mqtt.subscribe(T_VENT, 1);
  Serial.println("[MQTT] sub ia + servo + ventilador");
}

void mqttTick() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqtt.connected()) {
    String id = "wokwi-agro-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS, T_STATUS, 1, true, "OFFLINE")) {
      mqtt.publish(T_STATUS, "ONLINE", true);
      mqttSuscribir();
      Serial.println("[MQTT] reconectado");
    }
  }
  mqtt.loop();
}

String leerLineaSerial() {
  String linea;
  for (;;) {
    mqttTick();
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (linea.length()) {
          Serial.println();
          return linea;
        }
      } else if (c >= 32 && c < 127) {
        linea += c;
        Serial.print(c);
        lcdClearLine(3);
        lcd.setCursor(0, 3);
        String eco = ">" + linea;
        if (eco.length() > 20) eco = eco.substring(eco.length() - 20);
        lcd.print(eco);
      }
    }
    delay(10);
  }
}

bool pedirNumero(uint8_t n, const char* tituloLcd, const char* preguntaSerial,
                 float minimo, float maximo, float& valor) {
  for (;;) {
    lcdPregunta(n, 4, tituloLcd);
    Serial.printf("%s: ", preguntaSerial);
    String s = leerLineaSerial();
    s.replace(',', '.');
    s.trim();
    char* fin = nullptr;
    float v = strtof(s.c_str(), &fin);
    if (fin != s.c_str() && *fin == '\0' && v >= minimo && v <= maximo) {
      valor = v;
      lcdClearLine(2);
      lcdClearLine(3);
      lcd.setCursor(0, 2); lcd.print("OK:");
      lcd.setCursor(4, 2); lcd.print(v, 1);
      delay(150);
      return true;
    }
    Serial.printf("  Invalido. Usa un numero entre %.0f y %.0f.\n", minimo, maximo);
    lcdClearLine(2);
    lcd.setCursor(0, 2); lcd.print("Valor invalido");
    delay(400);
  }
}

String pedirTexto(uint8_t n, const char* tituloLcd, const char* preguntaSerial) {
  for (;;) {
    lcdPregunta(n, 4, tituloLcd);
    Serial.printf("%s: ", preguntaSerial);
    String s = leerLineaSerial();
    s.trim();
    s.replace("\"", "");
    s.replace("\\", "");
    if (s.length() >= 2 && s.length() <= 20) {
      lcdClearLine(2);
      lcdClearLine(3);
      lcd.setCursor(0, 2); lcd.print("OK:");
      lcd.setCursor(4, 2); lcd.print(s);
      delay(150);
      return s;
    }
    Serial.println("  Escribe entre 2 y 20 letras (ej. tomate).");
    lcdClearLine(2);
    lcd.setCursor(0, 2); lcd.print("Texto invalido");
    delay(400);
  }
}

void conectarWiFi() {
  lcdTitulo("AgroSmart completo", "WiFi Wokwi...", "", "");
  Serial.printf("[WiFi] Conectando a %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    lcd.setCursor(i % 20, 3); lcd.print(".");
    i++;
  }
  Serial.printf("\n[WiFi] OK IP %s\n", WiFi.localIP().toString().c_str());
  lcdTitulo("WiFi OK", WiFi.localIP().toString().c_str(), "Conectando MQTT...", "");
}

void conectarMQTT() {
  uint8_t intento = 0;
  while (!mqtt.connected()) {
    if (WiFi.status() != WL_CONNECTED) conectarWiFi();
    intento++;
    String id = "wokwi-agro-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.printf("[MQTT] intento %u → %s:%d ...\n", intento, MQTT_HOST, MQTT_PORT);
    lcdClearLine(2);
    lcdClearLine(3);
    lcd.setCursor(0, 2); lcd.print("MQTT TLS...");
    lcd.setCursor(0, 3); lcd.print("intento ");
    lcd.print(intento);
    mqtt.setSocketTimeout(15);
    bool ok = mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS, T_STATUS, 1, true, "OFFLINE");
    if (ok) {
      Serial.println("[MQTT] OK (TLS)");
      mqtt.publish(T_STATUS, "ONLINE", true);
      mqttSuscribir();
      setRiego(false);
      setVentilador(false);
      lcdTitulo("MQTT OK", "LCD+Servo+LED", "Serial: 4 datos", "Enter cada uno");
      return;
    }
    int rc = mqtt.state();
    Serial.printf("[MQTT] fallo state=%d\n", rc);
    lcdClearLine(2);
    lcd.setCursor(0, 2); lcd.print("MQTT fallo rc=");
    lcd.print(rc);
    lcdClearLine(3);
    lcd.setCursor(0, 3); lcd.print("Reintentando...");
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== AgroSmart Wokwi · LCD + Servo + LED ===");

  pinMode(PIN_VENTILADOR, OUTPUT);
  digitalWrite(PIN_VENTILADOR, LOW);
  ledcAttach(PIN_SERVO, 50, 14);
  moverServo(ANGULO_CERRADO);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcdTitulo("AgroSmart", "LCD + Servo + LED", "Iniciando...", "");

#if MQTT_TLS_INSECURE
  net.setInsecure();
  Serial.println("[TLS] modo insecure (simulador Wokwi)");
#else
  net.setCACert(ROOT_CA);
#endif
  net.setTimeout(20000);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMsg);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(20);
  mqtt.setSocketTimeout(15);

  conectarWiFi();
  conectarMQTT();
  for (int i = 0; i < 4; i++) { mqtt.loop(); delay(30); }
}

void loop() {
  mqttTick();

  float temperatura, humAmbiente, humSuelo;
  Serial.println("\n=== Ingreso de datos (4) ===");
  pedirNumero(1, "Temperatura C", "1/4 Temperatura (C)", -10, 60, temperatura);
  pedirNumero(2, "Humedad aire %", "2/4 Humedad ambiental (%)", 0, 100, humAmbiente);
  pedirNumero(3, "Humedad suelo %", "3/4 Humedad de suelo (%)", 0, 100, humSuelo);
  String cultivo = pedirTexto(4, "Tipo de cultivo", "4/4 Tipo de cultivo (ej. tomate)");

  char json[192];
  snprintf(json, sizeof(json),
           "{\"temperatura\":%.1f,\"humedad_ambiente\":%.1f,\"humedad_suelo\":%.1f,\"cultivo\":\"%s\"}",
           temperatura, humAmbiente, humSuelo, cultivo.c_str());

  Serial.printf("\n[JSON] %s\n", json);
  iaLista = false;
  iaTexto[0] = 0;

  lcdTitulo("Enviando datos...", cultivo.c_str(), "Luego espera IA", "+ mueve servo");
  bool ok = mqtt.publish(T_DATOS, json, false);
  Serial.printf("[MQTT] publish datos %s\n", ok ? "OK" : "FALLO");
  if (!ok) {
    lcdTitulo("Error al publicar", "Revisa MQTT/WiFi", "", "");
    delay(1200);
    return;
  }

  lcdTitulo("Esperando Gemini", "Servo/LED listos", "LCD <- .../ia", "");
  unsigned long t0 = millis();
  while (!iaLista && (millis() - t0) < 45000UL) {
    mqttTick();
    lcd.setCursor((millis() / 300) % 20, 3);
    lcd.print('.');
    delay(20);
  }

  if (iaLista) {
    mostrarIaEnLcd(iaTexto);
    Serial.println("Mira el SERVO y el LED en Wokwi. Enter para otra ronda...");
    t0 = millis();
    while ((millis() - t0) < 8000UL) {
      mqttTick();
      if (Serial.available()) break;
      delay(20);
    }
    while (Serial.available()) Serial.read();
  } else {
    lcdTitulo("Sin respuesta IA", "Timeout 45s", "Mira /agro/web", "");
    delay(1500);
  }

  lcdTitulo("Otra ronda?", "Serial: 4 datos", "Servo sigue MQTT", "desde /agro/web");
  delay(400);
}