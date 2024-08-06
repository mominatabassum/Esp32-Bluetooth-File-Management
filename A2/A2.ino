#include <Wire.h>
#include <BluetoothSerial.h>
#include <SPIFFS.h>

#define IMU 0x68
#define PWR_MGMT_1_REG 0x6B

#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43

#define SDA_PIN 21
#define SCL_PIN 22

#define SAMPLES 10000

struct imu_data {
  float accelX;     
  float accelY;
  float accelZ; 

  float gyroX;      
  float gyroY; 
  float gyroZ; 
};

imu_data imu;

BluetoothSerial SerialBT;
float radius, speed;
bool isRecording = false;
String filename;

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.begin(9600);

  // Wake up the MPU9250
  Wire.beginTransmission(IMU);
  Wire.write(PWR_MGMT_1_REG);
  Wire.write(0);
  Wire.endTransmission();

  // Start Bluetooth serial
  SerialBT.begin("IMU_device"); // Name of your Bluetooth device
  Serial.println("IMU Device is Ready to Pair");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  SerialBT.println("SPIFFS mounted successfully");
}

void loop() {
  // Taking input from Bluetooth
  if (SerialBT.available()) {
    String input = SerialBT.readString();
    input.trim();
    if (input.length() > 0) {
      int commaIndex = input.indexOf(',');
      if (commaIndex != -1  && !isRecording) {
        radius = input.substring(0, commaIndex).toFloat();
        speed = input.substring(commaIndex + 1).toFloat();
        filename = "/file_" + String(radius) + "_" + String(speed) + ".txt";
        
        SerialBT.print("Radius: ");
        SerialBT.println(radius);
        SerialBT.print("Speed: ");
        SerialBT.println(speed);
      } else {
        if (input == "start" && !isRecording) {
          isRecording = true;
          SerialBT.print("recording started");

        } else if (input == "stop") {
          isRecording = false;
           SerialBT.print("recording stopped");

        }else if(input=="list" && !isRecording){
            listFiles();

        } else if (input=="delete" && !isRecording) {
          deleteFile(filename);
          
        } else if (input=="write" && !isRecording) {
          sendFileToSerial(filename);

        }
      }
    }
  }
      
  // Read accelerometer data
  imu.accelX = (readRegister16(IMU, ACCEL_XOUT_H)-102) / 16370.0; // Adjust scale
  imu.accelY = (readRegister16(IMU, ACCEL_XOUT_H + 2)-168) / 16390.0;
  imu.accelZ = (readRegister16(IMU, ACCEL_XOUT_H + 4)+1483) / 16290.0;

  // Read gyroscope data
  imu.gyroX = readRegister16(IMU, GYRO_XOUT_H); // Adjust scale
  imu.gyroY = readRegister16(IMU, GYRO_XOUT_H + 2);
  imu.gyroZ = readRegister16(IMU, GYRO_XOUT_H + 4);

  if (isRecording) {
    saveToFile();
  }

  delay(100); // Adjust delay as needed
}

// Read MPU registers
int16_t readRegister16(int addr, int reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(addr, 2, true);
  
  int16_t value = Wire.read() << 8 | Wire.read();
  return value;
}

void saveToFile() { 
  File file = SPIFFS.open(filename, FILE_APPEND);
  if (!file) {
    SerialBT.println("Failed to open file for writing");
    createFile(filename, "0,0,0,0,0,0");
    return;
  }
  file.printf("AccelX: %.2f, AccelY: %.2f, AccelZ: %.2f, GyroX: %.2f, GyroY: %.2f, GyroZ: %.2f\n",
              imu.accelX, imu.accelY, imu.accelZ,
              imu.gyroX, imu.gyroY, imu.gyroZ);
  file.close();
}

void listFiles() {
        File root = SPIFFS.open("/");
        if (!root || !root.isDirectory()) {
          SerialBT.println("Failed to open directory");
          return;
        }

        File file = root.openNextFile();
        while (file) {
          String fileName = file.name();
          size_t fileSize = file.size();
          file.close(); // Close file after accessing its size

          SerialBT.print("Found file: ");
          SerialBT.print(fileName);
          SerialBT.print(" Size: ");
          SerialBT.print(fileSize);
          SerialBT.println(" bytes");

          file = root.openNextFile(); // Move to the next file
        }
}

void createFile(String path, const char *message) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    SerialBT.println("Failed to open file for writing");
    return;
  }

  if (file.print(message)) {
    SerialBT.println("File written successfully");
  } else {
    SerialBT.println("Failed to write to file");
  }

  file.close(); // Close the file to save changes
}

void deleteFile(String path) {
  if (SPIFFS.remove(path)) {
    Serial.print("File deleted: ");
    Serial.println(path);
  } else {
    Serial.print("Failed to delete file: ");
    Serial.println(path);
  }
}


void sendFileToSerial(String filename) {
  // Initialize SPIFFS
  if (!SPIFFS.begin()) { // Replace with LittleFS.begin() if using LittleFS
    SerialBT.println("Failed to mount file system");
    return;
  }

  File file = SPIFFS.open(filename, "r"); // Replace with LittleFS.open() if using LittleFS
  if (!file) {
    SerialBT.println("Failed to open file");
    return;
  }

  SerialBT.println("Sending file...");
  while (file.available()) {
    Serial.write(file.read());
  }
  SerialBT.println("File sent");
  file.close();
}


