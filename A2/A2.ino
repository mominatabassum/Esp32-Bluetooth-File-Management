#include <Wire.h>
#include <BluetoothSerial.h>
#include <SPIFFS.h>

#define IMU 0x68
#define PWR_MGMT_1_REG 0x6B

#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43

#define SDA_PIN 21
#define SCL_PIN 22

#define SAMPLES 500
#define fileCount 24

struct imu_data {
  float accelX;     
  float accelY;
  float accelZ; 

  float gyroX;      
  float gyroY; 
  float gyroZ; 
};

imu_data imu;
String data_array[500];
File file;
String fileList[fileCount];

BluetoothSerial SerialBT;
float radius, speed;
bool isRecording = false;
bool isChecked = false;
String curentFileName;
String axis;
int count = 0;

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.begin(9600);

  // Wake up the MPU9250
  Wire.beginTransmission(IMU);
  Wire.write(PWR_MGMT_1_REG);
  Wire.write(0);
  Wire.endTransmission();

  // Start Bluetooth serial
  SerialBT.begin("Final_IMU"); // Name of your Bluetooth device
  Serial.println("IMU Device is Ready to Pair");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  SerialBT.println("SPIFFS mounted successfully");
}

unsigned long pTime = 0;
unsigned long cTime;
unsigned long loopTime;

void loop() {
  // Taking input from Bluetooth
  if (SerialBT.available()) {
    String input = SerialBT.readString();
    input.trim();
    if (input.length() > 0) {
      int commaIndex = input.indexOf(',');
      if (commaIndex != -1 && !isRecording) {
        int secondCommaIndex = input.indexOf(',', commaIndex + 1);
        radius = input.substring(0, commaIndex).toFloat();
        speed = input.substring(commaIndex + 1, secondCommaIndex).toFloat();
        curentFileName = "/file_" + String(radius) + "_" + String(speed) + ".txt";
        if (secondCommaIndex != -1) {
          axis = input.substring(secondCommaIndex + 1);
          curentFileName = "/file_" + String(radius) + "" + String(speed) + "" + axis + ".txt";
        }
        SerialBT.print("Radius: ");
        SerialBT.println(radius);
        SerialBT.print("Speed: ");
        SerialBT.println(speed);
      } else {
        if (input == "start" && !isRecording) {
          isRecording = true;
          SerialBT.print("recording started");
          file = SPIFFS.open(curentFileName, FILE_APPEND);
          count=0;

        } else if (input == "stop") {
          isRecording = false;
           SerialBT.print("recording stopped");
           file.close();

        }else if(input=="list" && !isRecording){
            listFiles();

        } else if (input=="delete" && !isRecording) {
          deleteFile();
          
        } else if (input=="write" && !isRecording) {
          sendFileToSerial(curentFileName);

        } else if (input=="info" && !isRecording) {
          SerialBT.print(curentFileName);

        } else if (input=="transfer" && !isRecording) {
          transferFiles();

        } else if (input=="toggle") {
          isChecked=!isChecked;

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
    char buffer[100];  // Adjust the size based on your data
    sprintf(buffer, "AX: %.2f, AY: %.2f, AZ: %.2f, GX: %.2f, GY: %.2f, GZ: %.2f\n",
                    imu.accelX, imu.accelY, imu.accelZ,
                    imu.gyroX, imu.gyroY, imu.gyroZ);
    data_array[count] = String(buffer);
        SerialBT.printf("AX: %.2f, AY: %.2f, AZ: %.2f, GX: %.2f, GY: %.2f, GZ: %.2f\n",
              imu.accelX, imu.accelY, imu.accelZ,
              imu.gyroX, imu.gyroY, imu.gyroZ);
    count++;
    if (count==SAMPLES) {
      isRecording = false;
      SerialBT.print("recording stopped");
      saveToFile();
      file.close();
      count=0;
    }
  }

  if (isChecked){
         
             SerialBT.printf("AX: %.2f, AY: %.2f, AZ: %.2f, GX: %.2f, GY: %.2f, GZ: %.2f\n",
             imu.accelX, imu.accelY, imu.accelZ,
             imu.gyroX, imu.gyroY, imu.gyroZ);

  }
  cTime = millis();
  loopTime=cTime-pTime;
  pTime = cTime;
  Serial.println(loopTime);
  delay(1); // Adjust delay as needed
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
  if (!file) {
    SerialBT.println("Failed to open file for writing");
    createFile(curentFileName, "0,0,0,0,0,0");
    return;
  }
  
  // Assuming you want to write all elements of data_array to the file
  for (int i = 0; i < SAMPLES; i++) {
    file.println(data_array[i]);
  }
}


void listFiles() {
    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory()) {
        SerialBT.println("Failed to open directory");
        return;
    }

    int j = 0;
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        size_t fileSize = file.size();
        file.close(); // Close file after accessing its size

        SerialBT.print("Found file: ");
        SerialBT.print(fileName);
        fileList[j] = fileName;
        SerialBT.print(" Size: ");
        SerialBT.print(fileSize);
        SerialBT.println(" bytes");

        file = root.openNextFile(); // Move to the next file
        j++; // Increment the file count
    }

    SerialBT.print("Total number of files: ");
    SerialBT.println(j);
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

void deleteFile() {
  if (SPIFFS.remove(curentFileName)) {
    SerialBT.print("File deleted: ");
    SerialBT.println(curentFileName);
  } else {
    SerialBT.print("Failed to delete file: ");
    SerialBT.println(curentFileName);
  }
}


void sendFileToSerial(String x) {
    file = SPIFFS.open(x, "r"); 
    if (!file) {
      SerialBT.println("Failed to open file:  " + x);
      return;
    }

    //SerialBT.println("Sending file..." + x);
    Serial.println(removeLeadingSlash(x));
    while (file.available()) {
      Serial.write(file.read());
    }
    SerialBT.println("File sent");
    Serial.println("File Sent");
    file.close();
}


void transferFiles() {
  listFiles();
  String x;
  for (int i = 0; i < fileCount; i++) {
    x = fileList[i];
    SerialBT.println("transfering file: " + x); 
    Serial.println(x);
    sendFileToSerial("/" + x);    
    Serial.println("File Sent");
  }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   

String removeLeadingSlash(String str) {
    if (str.startsWith("/")) {
        str = str.substring(1);
    }
    return str;
}
