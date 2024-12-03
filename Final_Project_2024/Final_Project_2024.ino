#define BLYNK_TEMPLATE_NAME "FINAL PROJECT KELOMPOK 5 MYSKILL SIB 2024"
#define BLYNK_AUTH_TOKEN "isx12PjIqNqtAy4V4jRgB-kf4xTzs4j6"
#define BLYNK_TEMPLATE_ID "TMPL6MYl1pHuP"

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MQUnifiedsensor.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Konfigurasi pin
#define DHTPIN 27            // Pin untuk sensor DHT22
#define DHTTYPE DHT22        // Tipe sensor DHT22
#define MQ_PIN 34           // Pin analog untuk MQ-135
#define RELAY_PIN 15         // Pin untuk relay kipas
#define BUZZER_PIN 26        // Pin untuk buzzer
#define RED_LED 16           // Pin untuk LED merah
#define GREEN_LED 17         // Pin untuk LED hijau
#define BLUE_LED 5           // Pin untuk LED biru

// Konfigurasi OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Konfigurasi MQ-135
#define placa "ESP32"
#define Voltage_Resolution 3.3
#define ADC_Bit_Resolution 12
#define type "MQ-135"
#define RatioMQ135CleanAir 3.6 // RS / R0 = 3.6 ppm
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, MQ_PIN, type);

// Inisialisasi sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Threshold level untuk kualitas udara
float safeThreshold = 30;    // Udara aman
float warningThreshold = 50; // Udara tidak aman
float dangerThreshold = 101; // Udara berbahaya

// Konfigurasi WiFi dan Blynk
char auth[] = "isx12PjIqNqtAy4V4jRgB-kf4xTzs4j6";  // Token Blynk
char ssid[] = "Ayamma";       // Nama WiFi
char pass[] = "Pastibisaa";   // Password WiFi
bool powerStatus = true;        // Status power ESP

void setLED(bool red, bool green, bool blue) {
  digitalWrite(RED_LED, red);
  digitalWrite(GREEN_LED, green);
  digitalWrite(BLUE_LED, blue);
}

void playTone(int frequency, int duration) {
  int delayValue = 1000000 / frequency / 2; // Menghitung waktu delay (microdetik)
  long numCycles = frequency * duration / 1000; // Jumlah siklus yang diperlukan

  for (long i = 0; i < numCycles; i++) {
    digitalWrite(BUZZER_PIN, HIGH); // Nyalakan buzzer
    delayMicroseconds(delayValue);
    digitalWrite(BUZZER_PIN, LOW);  // Matikan buzzer
    delayMicroseconds(delayValue);
  }
  
}

// Fungsi untuk mematikan ESP
BLYNK_WRITE(V3) {
  int pinValue = param.asInt();
  if (pinValue == 0) {
    powerStatus = false; // Matikan ESP
  }
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin output
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // Inisialisasi DHT22
  dht.begin();

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED tidak ditemukan, cek koneksi!"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Inisialisasi MQ-135
  MQ135.setRegressionMethod(1);
  MQ135.setA(605.18);
  MQ135.setB(-3.937);
  MQ135.init();

  // Kalibrasi MQ-135
  float calcR0 = 0;
  for (int i = 0; i < 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    delay(100);
  }
  MQ135.setR0(calcR0 / 10);
  Serial.println("Kalibrasi selesai!");

  // Inisialisasi Blynk
  Blynk.begin(auth, ssid, pass);
  Serial.println("Blynk siap!");
}

void loop() {
  if (!powerStatus) {
    Serial.println("ESP dimatikan");
    ESP.restart();
  }

  Blynk.run();

  // Membaca nilai dari MQ-135
  MQ135.update();
  float airQuality = MQ135.readSensor(); // Menggunakan perpustakaan MQUnifiedsensor

  // Membaca suhu dan kelembaban dari DHT22
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Kirim data ke Blynk
  Blynk.virtualWrite(V0, airQuality);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);

  // Menampilkan data di Serial Monitor
  Serial.print("CO : ");
  Serial.print(airQuality);
  Serial.println(" PPM");
  Serial.print("Temperature : ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity : ");
  Serial.print(humidity);
  Serial.println(" %");
  delay(500);
  
  // Tampilkan data pada OLED
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(2, 0);
  display.print("ECO BREATH");
  display.setTextSize(1);
  display.setCursor(4, 30);
  display.print("Gas: ");
  display.print(airQuality);
  display.print(" PPM");
  display.setCursor(4, 40);
  display.print("Suhu: ");
  display.print(temperature);
  display.print(" C");
  display.setCursor(4, 50);
  display.print("Humidity : ");
  display.print(humidity);
  display.print(" %");
  display.display();
  delay(1000);

  // Logika kontrol LED, buzzer, dan relay
  if (airQuality <= safeThreshold) {
    setLED(LOW, HIGH, LOW);       // LED hijau menyala
    digitalWrite(BUZZER_PIN, LOW); 
    digitalWrite(RELAY_PIN, LOW); 
    Serial.println("Status: Aman");
  } 
  else if (airQuality <= warningThreshold) {
    setLED(LOW, LOW, HIGH);       // LED biru menyala
    digitalWrite(BUZZER_PIN, HIGH); 
    digitalWrite(RELAY_PIN, HIGH); 
    Serial.println("Status: Peringatan");

    // Nada sedang putus-putus saat LED biru menyala
    playTone(800, 200);  // Nada 800 Hz, durasi 100 ms, jeda 100 ms
    playTone(800, 200);  // Nada 800 Hz, durasi 100 ms, jeda 100 ms
  } 
  else if (airQuality > warningThreshold) {
    setLED(HIGH, LOW, LOW);       // LED merah menyala
    Blynk.logEvent("unhealthy_air", "Udara Tidak Sehat! Gas PPM: " + String(airQuality));
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH); 
    Serial.println("Status: Bahaya");

    // Nada tinggi panjang saat LED merah menyala
    playTone(1500, 2500);  // Nada 1500 Hz, durasi 1000 ms, jeda 500 ms
  }

  delay(500); // Delay untuk pembacaan ulang
}
