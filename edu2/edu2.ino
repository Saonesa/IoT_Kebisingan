#define BLYNK_TEMPLATE_ID "TMPL6otlzkypY"
#define BLYNK_TEMPLATE_NAME "IOT Edu"
#define BLYNK_AUTH_TOKEN "12gSDR0UGjPcqzIIr_1ZODU_XiwB_hQs"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h> // Tambahkan library HTTPClient
#include <WiFiClient.h> // Tambahkan library WiFiClient

char auth[] = BLYNK_AUTH_TOKEN; // Blynk Auth Token
char ssid[] = "Saone";          // WiFi SSID
char pass[] = "30082004";       // WiFi Password

// Pin Definitions
#define SENSOR_PIN A0
#define BUZZER_PIN D1
#define SAMPLE_WINDOW 50

BlynkTimer timer;
unsigned int sample;

// Noise Level Thresholds

int highThreshold = 55;
int db = 0;

// Function to read sound levels and handle logic
void readSoundLevel() {
    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;

    // Collect data for SAMPLE_WINDOW duration
    while (millis() - startMillis < SAMPLE_WINDOW) {
        sample = analogRead(SENSOR_PIN);
        if (sample < 1024) {
            if (sample > signalMax) {
                signalMax = sample;
            } else if (sample < signalMin) {
                signalMin = sample;
            }
        }
    }

    // Calculate peak-to-peak and convert to decibels
    float peakToPeak = signalMax - signalMin;
    db = map(peakToPeak, 0, 450, 50, 90);

    // Log data to Serial Monitor
    Serial.print("Peak-to-Peak: ");
    Serial.print(peakToPeak);
    Serial.print(" dB: ");
    Serial.println(db);

    // Send noise level to Blynk
    Blynk.virtualWrite(V1, db);

    // Handle noise levels and control buzzer
    if (db > highThreshold) {
        digitalWrite(BUZZER_PIN, HIGH);
        Serial.println("Noise level HIGH, buzzer ON!");
        Blynk.logEvent("noise_alert", "Kebisingan tinggi terdeteksi!");
    } else {
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Noise level normal, buzzer OFF.");
    }
}


void sendToSpreadsheet(int noiseLevel) {
    // URL Google Apps Script Webhook
    const char* webhookUrl = "https://script.google.com/macros/s/AKfycbyBMXcY3vmMJeNqKUKelamTBzNuFaExcJ8sRhNKmeHR4lcs3-N0flhAsuL0ciWPh5EB/exec"; // Ganti dengan URL webhook Anda

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure(); // Nonaktifkan verifikasi sertifikat (opsional jika Anda tidak memiliki sertifikat SSL spesifik)

        HTTPClient http;

        // Inisialisasi koneksi HTTPS
        if (http.begin(client, webhookUrl)) {
            // Format data dalam JSON
            String jsonPayload = "{\"value\":" + String(noiseLevel) + "}";

            // Set header untuk request HTTP POST
            http.addHeader("Content-Type", "application/json");

            // Kirim data melalui HTTP POST
            int httpResponseCode = http.POST(jsonPayload);

            // Cetak respons HTTP di Serial Monitor
            if (httpResponseCode > 0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                Serial.print("Response: ");
                Serial.println("Data Berhasil Dikirim");
            } else {
                Serial.print("Error in sending POST: ");
                Serial.println(httpResponseCode);
            }

            // Tutup koneksi
            http.end();
        } else {
            Serial.println("Unable to connect to server");
        }
    } else {
        Serial.println("WiFi not connected");
    }
}

// Send data to Google Spreadsheet
void sendDataToSpreadSheet(){
    sendToSpreadsheet(db);
}

// Virtual pin function to manually control the buzzer
BLYNK_WRITE(V2) {
    int buttonState = param.asInt(); // Get button state
    digitalWrite(BUZZER_PIN, buttonState);
}

// Setup function
void setup() {
    // Initialize Serial Monitor
    Serial.begin(115200);

    // Initialize pins
    pinMode(SENSOR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Connect to Blynk
    Blynk.begin(auth, ssid, pass);

    // Timer to monitor noise level every second
    timer.setInterval(1000L, readSoundLevel);
    timer.setInterval(10000L, sendDataToSpreadSheet);
}

// Main loop
void loop() {
    Blynk.run();
    timer.run();
}
