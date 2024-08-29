import serial
import os

os.system("cls")

# Configure the serial port (replace with your port and baudrate)
ser = serial.Serial('COM3', 9600, timeout=1)

def save_data():
    current_file = None
    data_buffer = []

    while True:
        line = ser.readline().decode('utf-8').strip()
        

        if "sent" in line.lower():
            if current_file and data_buffer:
                with open(current_file, 'w') as file:
                    file.write("\n".join(data_buffer))
                    print(f"-> {line}")
                print(f"{current_file} saved.")
            current_file = None
            data_buffer = []
    
        elif current_file is None or (line[:4]=="file"):
            current_file = line
            data_buffer = []   
            print(f"file -> {line}") 
        else:
            data_buffer.append(line)
            print(f"data -> {line}")

if __name__ == "__main__":
    line = ser.readline().decode('utf-8').strip()
    print(f"{line}")
    try:
        save_data()
    except KeyboardInterrupt:
        ser.close()
