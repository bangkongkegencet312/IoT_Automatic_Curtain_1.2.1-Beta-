#include <Servo.h> // Library untuk mengontrol servo motor
#include <DHT.h>   // Library untuk sensor DHT (suhu dan kelembaban)

#define LDR_PIN A0     // Pin analog untuk sensor LDR (Light Dependent Resistor)
#define SERVO_PIN 9    // Pin digital untuk servo motor
#define DHT_PIN 10     // Pin digital untuk sensor DHT11
#define DHTTYPE DHT11  // Tipe sensor DHT yang digunakan (DHT11)

Servo servo; // Membuat objek Servo
DHT dht(DHT_PIN, DHTTYPE); // Membuat objek DHT dengan pin dan tipe sensor

// PID konstanta
float Kp = 2.08; // Konstanta Proportional
float Ki = 0.1;  // Konstanta Integral
float Kd = 1.00; // Konstanta Derivative
float setpoint = 90.0; // Nilai target intensitas cahaya (misalnya, 90% terang)
float error = 0.0;     // Selisih antara setpoint dan intensitas cahaya aktual
float prevError = 0.0; // Error sebelumnya, digunakan untuk perhitungan derivative
float integral = 0.0;  // Akumulasi error, digunakan untuk perhitungan integral
float derivative = 0.0; // Perubahan error, digunakan untuk perhitungan derivative
int posisiServo = 10;  // Posisi awal servo (derajat)
const float errorThreshold = 5.0; // Batas error di mana PID akan aktif

// Suhu update interval
unsigned long waktuSuhuSebelumnya = 0; // Menyimpan waktu terakhir suhu dibaca
const unsigned long intervalSuhu = 2000; // Interval pembacaan suhu (2000 ms = 2 detik)
float suhu = 0; // Variabel untuk menyimpan nilai suhu

void setup() {
  Serial.begin(9600);    // Memulai komunikasi serial dengan baud rate 9600
                           // Ini akan digunakan untuk mengirim data ke ESP32
  servo.attach(SERVO_PIN); // Mengaitkan objek servo ke pin yang ditentukan
  servo.write(posisiServo); // Mengatur posisi awal servo
  dht.begin(); // Memulai sensor DHT
}

void loop() {
  // --- Pembacaan Sensor LDR dan Perhitungan Intensitas Cahaya ---
  int nilaiLDR = analogRead(LDR_PIN); // Membaca nilai analog dari LDR (0-1023)
  // Mengkonversi nilai LDR menjadi intensitas cahaya dalam persentase (0-100)
  // Diasumsikan nilai LDR 0 adalah paling terang (100% cahaya) dan 1023 paling gelap (0% cahaya)
  float intensitasCahaya = 100 - ((nilaiLDR / 1023.0) * 100.0);

  // --- Kontrol PID untuk Servo ---
  error = setpoint - intensitasCahaya; // Menghitung error
  integral += error;                    // Mengakumulasi error untuk integral
  derivative = error - prevError;       // Menghitung perubahan error untuk derivative
  // Menghitung output PID
  float pidOutput = (Kp * error) + (Ki * integral) + (Kd * derivative);

  // Jika error melebihi ambang batas, sesuaikan posisi servo
  if (abs(error) > errorThreshold) {
    // Menentukan posisi target servo berdasarkan apakah intensitas cahaya kurang dari setpoint
    // Jika cahaya kurang dari setpoint, target servo ke 10 derajat (misal: membuka tirai lebih lebar)
    // Jika cahaya lebih dari setpoint, target servo ke 180 derajat (misal: menutup tirai lebih rapat)
    int posisiTarget = (intensitasCahaya < setpoint) ? 10 : 180;

    // Menyesuaikan posisi servo secara bertahap berdasarkan output PID
    if (posisiServo < posisiTarget) {
      posisiServo += min(int(abs(pidOutput)), 1); // Tambahkan 1 derajat jika pidOutput positif dan lebih dari 0
    } else if (posisiServo > posisiTarget) {
      posisiServo -= min(int(abs(pidOutput)), 1); // Kurangkan 1 derajat jika pidOutput negatif dan kurang dari 0
    }

    // Membatasi posisi servo agar tidak melebihi 0 dan 180 derajat
    posisiServo = constrain(posisiServo, 0, 180);
    servo.write(posisiServo); // Menggerakkan servo ke posisi yang baru
  }

  prevError = error; // Menyimpan error saat ini untuk perhitungan derivative selanjutnya

  // --- Pembacaan Suhu (setiap 2 detik) ---
  // Memeriksa apakah sudah waktunya untuk membaca suhu (setiap 'intervalSuhu' ms)
  if (millis() - waktuSuhuSebelumnya >= intervalSuhu) {
    float suhuBaca = dht.readTemperature(); // Membaca suhu dari sensor DHT
    if (!isnan(suhuBaca)) { // Memastikan pembacaan suhu valid (bukan NaN - Not a Number)
      suhu = suhuBaca;      // Memperbarui variabel suhu
    }
    waktuSuhuSebelumnya = millis(); // Memperbarui waktu terakhir suhu dibaca
  }

  // --- Mengirim Data ke ESP32 via Serial ---
  // Mengirim data dengan format "key:value,key:value,..."
  Serial.print("cahaya:");
  Serial.print(intensitasCahaya);
  Serial.print(",  error:");
  Serial.print(error);
  Serial.print(",  servo:");
  Serial.print(posisiServo);
  Serial.print(",  suhu:");
  Serial.println(suhu); // Menggunakan println agar ada karakter newline (\n) di akhir,
                        // yang akan digunakan ESP32 sebagai penanda akhir data

  delay(50); // Jeda singkat untuk stabilisasi dan mencegah pembacaan terlalu cepat
}