#include <DHT.h>

#define TRIG_A 8
#define ECHO_A 9
#define TRIG_B 10
#define ECHO_B 11
#define RED_LED 12
#define DHTPIN 7       // DHT11 data pin connected here
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

int peopleCount = 0;
const int safeLimit = 10;

long readUltrasonic(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH) / 58;  // convert to cm
}

void setup() {
  pinMode(TRIG_A, OUTPUT);
  pinMode(ECHO_A, INPUT);
  pinMode(TRIG_B, OUTPUT);
  pinMode(ECHO_B, INPUT);
  pinMode(RED_LED, OUTPUT);

  Serial.begin(9600);   // TX → ESP32 RX2
  dht.begin();
}

void loop() {
  int distA = readUltrasonic(TRIG_A, ECHO_A);
  int distB = readUltrasonic(TRIG_B, ECHO_B);

  // Simple entry-exit detection
  if (distA < 20) { 
    peopleCount++; 
    delay(600); // debounce
  }
  if (distB < 20 && peopleCount > 0) { 
    peopleCount--; 
    delay(600); 
  }

  // LED Warning
  if (peopleCount > safeLimit) digitalWrite(RED_LED, HIGH);
  else digitalWrite(RED_LED, LOW);

  // Get DHT11 data
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    temp = -1;
    hum = -1;
  }

  // Send JSON-like string to ESP32
  Serial.print(peopleCount);
  Serial.print(",");
  Serial.print(temp);
  Serial.print(",");
  Serial.println(hum);

  delay(500);
}
