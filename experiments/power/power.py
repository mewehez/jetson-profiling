#!/usr/bin/env python3
import pathlib
import signal
import time
from contextlib import ExitStack

exit_program = False

def sigint_handler(sig, frame):
    global exit_program
    if(not exit_program):
        exit_program = True

def read_rail_name(path, rail):
    with open(path, "r") as file:
        rail["name"] = file.readline().rstrip('\n')


I2C_BASE_PATH = pathlib.Path("/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/")

rails_path = {}
rails = {}
for i in range(3):
    rails_path[i] = {
        "name":    I2C_BASE_PATH / f"rail_name_{i}",
        "current": I2C_BASE_PATH / f"in_current{i}_input",
        "voltage": I2C_BASE_PATH / f"in_voltage{i}_input",
        "power":   I2C_BASE_PATH / f"in_power{i}_input"
    }
    rails[i] = {"name": "rail0", "current": "", "voltage": "", "power": ""}

if __name__ == "__main__":
    # attach the signal handler
    signal.signal(signal.SIGINT, sigint_handler)

    for i in range(3):
        read_rail_name(rails_path[i]["name"], rails[i])
        print(f"Rail {i}: {rails[i]['name']}")
    with ExitStack() as stack:  
        files = {}
        keys = ["current", "voltage", "power"]
        for i in range(3):
            files[i] = {}
            for key in keys:
                files[i][key] = stack.enter_context(open(rails_path[i][key]))
        
        while(not exit_program):
            start_t = time.time()
            for i in range(3):
                for key, val in files[i].items():
                    rails[i][key] = val.readline().rstrip('\n')
            
            # print(f'[{rails[0]["name"]}] Current: {rails[0]["current"]}mA -- Voltage: {rails[0]["voltage"]}mV -- Power: {rails[0]["power"]}mW')
            # print(f'[{rails[1]["name"]}] Current: {rails[1]["current"]}mA -- Voltage: {rails[1]["voltage"]}mV -- Power: {rails[1]["power"]}mW')
            # print(f'[{rails[2]["name"]}] Current: {rails[2]["current"]}mA -- Voltage: {rails[2]["voltage"]}mV -- Power: {rails[2]["power"]}mW')
            
            for i in range(3):
                for _, val in files[i].items():
                    val.seek(0)
            
            end_t   = time.time()
            print(f"End - Start = {end_t - start_t}")
    print("SIGINT catched. Closing program...")
