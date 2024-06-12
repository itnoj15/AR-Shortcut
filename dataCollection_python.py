import serial
import csv
import time

class SensorManager:
    def __init__(self, port):
        self.serial = serial.Serial(port, 115200)

    def read_sensor(self):
        line = self.serial.readline()
        data = line.decode('utf-8').strip().split(",")
        float_data = [float(item) for item in data]
        return float_data
    
def main():
    sensor_manager = SensorManager('/dev/cu.usbmodem1301') 

    GESTURE = 3 # Gesture class - 0: copy / 1: paste / 2: undo / 3: yes / 4: no / 5: unknown / 6: not moving
    DATA_ID = 0

    with open('data_mj.csv', 'a', newline='') as file:
        writer = csv.writer(file)
        # writer.writerow(['gesture','id','Ax', 'Ay', 'Az', 'Gx', 'Gy', 'Gz', 'AEx', 'AEy', 'AEz', 'GEx', 'GEy', 'GEz'])
        time.sleep(2)
        print("start:", DATA_ID)
        lastTime = time.time()
        while True:
            if DATA_ID == 101:
                quit()
            if time.time() - lastTime >= 1.5:
                DATA_ID += 1
                print("start:", DATA_ID)
                lastTime = time.time()
            
            line = [GESTURE, DATA_ID] + sensor_manager.read_sensor()
            if 1 <= DATA_ID <= 100:
                writer.writerow(line)

if __name__ == "__main__":
    main()