// Wokwi: la Wi-Fi del simulador es siempre Wokwi-GUEST (sin clave).
// El bróker MQTTS es el real del grupo (puerto 8883).
#pragma once

#define WIFI_SSID   "Wokwi-GUEST"
#define WIFI_PASS   ""

#define MQTT_HOST   "172-214-154-175.sslip.io"
#define MQTT_PORT   8883
#define MQTT_USER   "sharon"
#define MQTT_PASS   "tecsup2026"

// En Wokwi la validación del certificado CA a veces se cuelga en "MQTT TLS...".
// 1 = cifrar sin verificar el cert (OK para la simulación). En placa real podés poner 0.
#define MQTT_TLS_INSECURE 1
