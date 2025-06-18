// index.js

// --- Konfigurasi Firebase ---
const admin = require('firebase-admin');
// GANTI INI: Sesuaikan dengan nama file kunci akun layanan Anda yang sebenarnya
const serviceAccount = require('./esp32-automatic-curtain-firebase-adminsdk-fbsvc-1937b3cdd0.json'); // <<<<<<< BARIS INI YANG BENAR // <<<<<<< UBAH INI

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  // GANTI INI: Sesuaikan dengan databaseURL Realtime Database Anda
  databaseURL: 'https://esp32-automatic-curtain-default-rtdb.asia-southeast1.firebasedatabase.app/' // <<<<<<< UBAH INI
});

const db = admin.database();
// Anda bisa mengubah 'sensor_data' jika ingin menyimpan di path lain di database
const ref = db.ref('sensor_data'); 

// --- Konfigurasi MQTT ---
const mqtt = require('mqtt');

// Pastikan ini cocok dengan konfigurasi broker HiveMQ Cloud Anda
const MQTT_BROKER_URL = 'mqtts://9575f087603642b38802e20db41742bf.s1.eu.hivemq.cloud:8883';
const MQTT_USERNAME = 'tetomiku'; // Username MQTT Anda
const MQTT_PASSWORD = 'TetoMiku1'; // Password MQTT Anda
const MQTT_TOPIC = 'sensor/arduino'; // Topik yang dipublikasikan oleh ESP32

const mqttClientOptions = {
  username: MQTT_USERNAME,
  password: MQTT_PASSWORD,
  protocol: 'mqtts', // Penting: gunakan mqtts untuk port 8883
  port: 8883,
  // Untuk produksi, `rejectUnauthorized: true` adalah default dan direkomendasikan.
  // Hanya jika Anda mengalami masalah sertifikat yang tidak terduga, bisa set `rejectUnauthorized: false`
  // Namun, untuk HiveMQ Cloud, `rejectUnauthorized: true` (default) harusnya berfungsi.
};

const client = mqtt.connect(MQTT_BROKER_URL, mqttClientOptions);

// --- Penanganan Event Koneksi MQTT ---
client.on('connect', () => {
  console.log('BERHASIL! Terhubung ke MQTT Broker!');
  client.subscribe(MQTT_TOPIC, (err) => {
    if (!err) {
      console.log(`Berhasil berlangganan topik: ${MQTT_TOPIC}`);
    } else {
      console.error(`Gagal berlangganan topik: ${MQTT_TOPIC}, error: ${err}`);
    }
  });
});

client.on('error', (err) => {
  console.error('TERJADI ERROR MQTT:', err);
});

client.on('close', () => {
  console.log('Koneksi MQTT ditutup.');
});

client.on('offline', () => {
    console.log('Klien MQTT offline.');
});

client.on('reconnect', () => {
    console.log('Mencoba menyambung ulang ke MQTT.');
});

// --- Penanganan Pesan MQTT ---
client.on('message', (topic, message) => {
  console.log(`Menerima pesan dari topik: ${topic}`);
  const payload = message.toString(); // Ubah buffer pesan menjadi string
  console.log('Payload mentah:', payload);

  try {
    const data = JSON.parse(payload); // Parse string JSON menjadi objek JavaScript

    // Tambahkan timestamp dari server Firebase saat data diterima
    data.timestamp = admin.database.ServerValue.TIMESTAMP; 

    // Simpan data ke Firebase Realtime Database
    // .push() akan membuat kunci unik (ID) otomatis untuk setiap data
    ref.push(data) 
      .then(() => {
        console.log('Data berhasil disimpan ke Firebase!');
      })
      .catch((error) => {
        console.error('Gagal menyimpan data ke Firebase:', error);
      });
  } catch (e) {
    console.error('Error saat parsing JSON atau menyimpan ke Firebase:', e);
  }
});

console.log('Aplikasi Jembatan MQTT ke Firebase dimulai...');