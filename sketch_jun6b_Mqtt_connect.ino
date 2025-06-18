#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h> // Tambahkan library untuk sinkronisasi waktu

// --- Konfigurasi WiFi ---
const char* ssid = "Wifi lemot jangan di pake";
const char* password = "panpakapan_arisu";

// --- Konfigurasi HiveMQ Cloud (MQTT Broker) ---
const char* mqtt_server = "9575f087603642b38802e20db41742bf.s1.eu.hivemq.cloud";
const int mqtt_port = 8883; // Port TLS
const char* mqtt_user = "tetomiku";
const char* mqtt_pass = "TetoMiku1";

// --- Objek Klien untuk Koneksi ---
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

String inputData = "";

// Tambahkan konfigurasi NTP untuk sinkronisasi waktu
const char* ntpServer = "pool.ntp.org"; // Server NTP umum
const long gmtOffset_sec = 7 * 3600;   // Offset GMT (misal: WIB = GMT+7)
const int daylightOffset_sec = 0;      // Tidak ada daylight saving

void setup_wifi() {
  delay(10);
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi terhubung!");

  // Inisialisasi dan sinkronisasi waktu dari NTP server setelah WiFi terhubung
  // Sangat penting untuk koneksi TLS/SSL
  Serial.print("Sinkronisasi waktu dari NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println(" Gagal mendapatkan waktu");
    // Anda mungkin ingin menambahkan delay atau retry di sini
  } else {
    Serial.println(" Waktu tersinkronisasi");
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    // ClientID harus unik. Gunakan MAC Address atau ID unik lain untuk ClientID jika ada banyak perangkat
    // String clientId = "ESP32Client-" + WiFi.macAddress(); // Contoh ClientID unik
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) { // Menggunakan ClientID statis
      Serial.println("Terhubung!");
      // client.subscribe("topic/test");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state()); // Menampilkan kode error MQTT (harus -2)
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

// Fungsi untuk mengubah string data dari Arduino menjadi format JSON
String convertToJson(String data) {
  String cahaya = getValue(data, "cahaya");
  String error = getValue(data, "error");
  String servo = getValue(data, "servo");
  String suhu = getValue(data, "suhu");

  return "{\"cahaya\":" + cahaya + ",\"error\":" + error + ",\"servo\":" + servo + ",\"suhu\":" + suhu + "}";
}

// Fungsi helper untuk mengambil nilai dari string berdasarkan key
String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "0";

  start += key.length() + 1;
  int end = data.indexOf(",", start);
  if (end == -1) end = data.length();

  return data.substring(start, end);
}

void setup() {
  Serial.begin(9600); // Inisialisasi Serial Monitor ESP32 untuk debugging (UART0)
  Serial1.begin(9600, SERIAL_8N1, 16, 17); // Inisialisasi Serial1 untuk komunikasi dengan Arduino Uno

  setup_wifi(); // Panggil fungsi untuk menghubungkan ke WiFi dan sinkronisasi waktu

  // Konfigurasi server MQTT dengan port TLS/SSL (8883)
  client.setServer(mqtt_server, mqtt_port);

  // **Penting untuk TLS:** Menginstruksikan WiFiClientSecure untuk tidak memvalidasi sertifikat CA
  // Ini adalah cara cepat untuk mengatasi masalah sertifikat untuk development/testing.
  // **Namun, SANGAT TIDAK DISARANKAN untuk PRODUKSI karena mengurangi keamanan.**
  // Untuk produksi, Anda harus menyertakan sertifikat CA dari broker Anda.
  wifiClient.setInsecure(); // Ini akan mengizinkan koneksi TLS tanpa verifikasi sertifikat CA.
                           // Lebih aman: wifiClient.setCACert(const char* root_ca); jika Anda punya sertifikat root.
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      inputData.trim();
      String jsonPayload = convertToJson(inputData);
      client.publish("sensor/arduino", jsonPayload.c_str());
      Serial.println(jsonPayload);
      inputData = "";
    } else {
      inputData += c;
    }
  }
  delay(10);
}