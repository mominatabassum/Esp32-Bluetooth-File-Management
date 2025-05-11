**ESP32 IMU Data Logger with Bluetooth and SPIFFS**
This project turns your ESP32 into a powerful data logger, capturing IMU (accelerometer and gyroscope) data from an MPU9250 sensor. The beauty of this setup is that all data is saved locally on the ESP32 using the SPIFFS file system, which means no worries about data loss while the system is in motion.

1. File Operations:
2. Start/Stop Recording: Start or stop recording data with simple Bluetooth commands.
3. File Listing: Want to know what’s saved? You can easily list all files stored on the ESP32, including their sizes.
4. File Deletion: Need to clear some space? Files can be deleted remotely through Bluetooth commands.
5. File Transfer: Transfer your recorded files to your computer for detailed analysis, all done wirelessly.


We created this while analyzing data at various speeds and radii using one of the motors in the lab. The first value in the ex_data represents the radius at which the IMU was mounted, and the second value indicates the speed of the universal motor.
