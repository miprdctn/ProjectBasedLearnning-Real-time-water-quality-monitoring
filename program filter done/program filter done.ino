#define BLYNK_TEMPLATE_ID "TMPL6SfkU0Ns4"
#define BLYNK_TEMPLATE_NAME "miprdctn"
#define BLYNK_AUTH_TOKEN "U0wcdO6wMoD_RpAZy1iH5pLl4gplelc2"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int oneWireBus = 25; // GPIO where the DS18B20 is connected to
#define TdsSensorPin 35    // Inisiasi pin data TDS
#define relay_pin1 13      // Inisiasi pin relay pengisian
#define relay_pin2 12    // Inisiasi pin relay pengosongan

#define VREF 3.3           // analog reference voltage(Volt) of the ADC
#define SCOUNT 30          // sum of sample point 

int analogBuffer[SCOUNT];  // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0;
float temperature = 0;

// WiFi credentials
char ssid[] = "qwerty";
char pass[] = "eprom2019";

OneWire oneWire(oneWireBus);    // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);    // Pass our oneWire reference to Dallas Temperature sensor

BlynkTimer timer; // Membuat objek timer

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  pinMode(TdsSensorPin, INPUT); // Pin data sensor TDS
  pinMode(relay_pin1, OUTPUT);  // Relay pengisian
  pinMode(relay_pin2, OUTPUT);  // Relay pengosongan
  sensors.begin();

  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timer.setInterval(1000L, tdssensor);  // Jadwalkan pembacaan sensor TDS setiap detik
}
float calibrationFactor = 1.0; // Nilai faktor kalibrasi yang dihitung

void loop() {
  Blynk.run();
  timer.run();
}

void tdssensor() {
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex >= SCOUNT) {
      analogBufferIndex = 0; // Resets the index to prevent overflow
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];

    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 4096.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage 
                - 255.86 * compensationVoltage * compensationVoltage 
                + 857.39 * compensationVoltage) * 0.5 * calibrationFactor;

    Serial.print("TDS Value:");
    Serial.print(tdsValue, 0);
    Serial.println(" ppm");

    Serial.print("Temperature:");
    Serial.print(temperature);
    Serial.println("ºC");

    Blynk.virtualWrite(V0, tdsValue);
    Blynk.virtualWrite(V3, temperature);
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i]; 
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  
  return bTemp;
}

BLYNK_WRITE(V1) {
  int value = param.asInt();
  if (value == 0) {
    digitalWrite(relay_pin1, LOW);
    Serial.println("Relay OFF");
  } else {
    digitalWrite(relay_pin1, HIGH);
    Serial.println("Relay ON");
  }
}

BLYNK_WRITE(V2) {
  int value1 = param.asInt();
  if (value1 == 0) {
    digitalWrite(relay_pin2, LOW);
    Serial.println("Relay OFF");
  } else {
    digitalWrite(relay_pin2, HIGH);
    Serial.println("Relay ON");
  }
  Blynk.virtualWrite(V2, value1);
}
