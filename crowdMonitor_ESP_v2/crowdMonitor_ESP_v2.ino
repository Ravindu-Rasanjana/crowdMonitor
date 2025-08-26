#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>

// WiFi & MQTT
const char* ssid = "Galaxy Note20 Ultra 5G";
const char* password = "nqsx8387";
const char* mqtt_server = "broker.hivemq.com"; 

WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

int peopleCount = 0;
String status = "SAFE";
float temperature = 0.0;
float humidity = 0.0;

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17 (connected to Arduino TX)

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Web Dashboard
  server.on("/", []() {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="UTF-8">
        <title>IoT Crowd & Environment Monitor</title>
        <style>
          body { font-family: Arial, sans-serif; background:#f4f4f9; text-align:center; }
          .card { margin:50px auto; padding:30px; max-width:500px;
                  background:white; border-radius:20px; box-shadow:0 4px 15px rgba(0,0,0,0.2);}
          h1 { color:#333; }
          .count { font-size:48px; font-weight:bold; color:#007BFF; }
          .status { font-size:28px; font-weight:bold; }
          .safe { color:green; }
          .unsafe { color:red; }
          .env { font-size:22px; margin-top:15px; }
        </style>
        <script>
          async function fetchData() {
            let response = await fetch("/data");
            let json = await response.json();
            document.getElementById("count").innerText = json.count;
            let statusEl = document.getElementById("status");
            statusEl.innerText = json.status;
            statusEl.className = "status " + (json.status === "SAFE" ? "safe" : "unsafe");
            document.getElementById("temp").innerText = json.temperature + " °C";
            document.getElementById("hum").innerText = json.humidity + " %";
          }
          setInterval(fetchData, 1000);
          window.onload = fetchData;
        </script>
      </head>
      <body>
        <div class="card">
          <h1>IoT Crowd & Environment Monitor</h1>
          <p>People inside:</p>
          <p id="count" class="count">0</p>
          <p>Status:</p>
          <p id="status" class="status safe">SAFE</p>
          <p class="env">🌡️ Temp: <span id="temp">--</span></p>
          <p class="env">💧 Humidity: <span id="hum">--</span></p>
        </div>
      </body>
      </html>
    )rawliteral";
    server.send(200, "text/html", html);
  });

  // JSON endpoint for AJAX
  server.on("/data", []() {
    String json = "{";
    json += "\"count\":" + String(peopleCount) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"humidity\":" + String(humidity);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  if (!client.connected()) client.connect("ESP32CrowdClient");
  client.loop();
  server.handleClient();

  // Read from Arduino (format: count,temp,hum)
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    data.trim();
    int first = data.indexOf(',');
    int second = data.indexOf(',', first + 1);
    if (first > 0 && second > 0) {
      peopleCount = data.substring(0, first).toInt();
      temperature = data.substring(first + 1, second).toFloat();
      humidity = data.substring(second + 1).toFloat();
      status = (peopleCount > 10) ? "UNSAFE" : "SAFE";

      // Publish to MQTT
      client.publish("crowd/count", String(peopleCount).c_str());
      client.publish("crowd/status", status.c_str());
      client.publish("crowd/temp", String(temperature).c_str());
      client.publish("crowd/hum", String(humidity).c_str());
    }
  }
}
