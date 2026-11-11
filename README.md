# AgroSmart Reto 1 · Wokwi (ESP32 + LCD 20×4)

## Abrir en Wokwi
1. Entra a https://wokwi.com/projects/new/esp32
2. En el diagrama: borra lo que haya → pega el contenido de `diagram.json` (o arma ESP32 + LCD2004 I2C: SDA→21, SCL→22, VCC→3V3, GND→GND).
3. Pega `sketch.ino` en el editor.
4. Crea archivos adicionales con el botón “+” / library files:
   - `secrets.h` (ya trae Wi‑Fi `Wokwi-GUEST` y MQTT de sharon)
   - `root_ca.h`
5. En Library Manager agrega: **PubSubClient** y **LiquidCrystal I2C**.
6. **Start simulation** → abre el **Serial Monitor** (115200, Newline).
7. Esperá `WiFi OK` y luego `MQTT OK (TLS)`. Si se queda en "MQTT TLS...", asegurate de haber pegado el `secrets.h` nuevo (`MQTT_TLS_INSECURE 1`) y el `sketch.ino` actualizado; Stop → Start de nuevo.
8. Responde las 4 preguntas con Enter.

## Qué mirar en Node-RED
Tras enviar el JSON:
- https://agro-sharon.172-214-154-175.sslip.io/agro/prompt  → prompt + respuesta Gemini  
  (usuario `sharon` / tu clave)
- https://agro-sharon.172-214-154-175.sslip.io/ui/ → gauges

Tópico: `agro/sharon/datos`
