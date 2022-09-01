#!/usr/bin/env python3
import threading
import this
import serial

class Fridge_Monitor:
    temperature = ""
    humidity = ""
    should_run = True
    thread = threading.Thread()
        
    def read_values_from_serial(arduino_connection, baudrate, timeout):
        print("started")
        ser = serial.Serial(port=arduino_connection, baudrate=baudrate, timeout=timeout)
        ser.reset_input_buffer()

        while Fridge_Monitor.should_run:
            msg = ser.readline()
            if msg != b'':
                if ser.in_waiting > 0:
                    line = msg.decode('utf-8').rstrip()
                    if "Temperature:" in line:
                        tempValue = line.split(": ")[1].split("\n")[0]
                        print(tempValue)
                        Fridge_Monitor.temperature = tempValue
                    if "Humidity:" in line:
                        humValue = line.split(": ")[1].split("\n")[0]
                        print(humValue)
                        Fridge_Monitor.humidity = humValue
    
    def toggle_should_run(state):
        should_run = state
        
    def start(arduino_con, baudrate, timeout):
        if Fridge_Monitor.thread.is_alive() == False:
            thread = threading.Thread(target=Fridge_Monitor.read_values_from_serial, args=(arduino_con, baudrate, timeout))
            thread.start()
        
            