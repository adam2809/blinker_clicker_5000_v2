## How does this work?
The program has been designed for the Arduino Nano. The purpuse is to connnect to a host computer and monitor the connection state. The communication porotocol used - Modbus RTU utilises the boards USB port, ch341 UART converter and the AtMega328p UART. A host has to be set up with a running Modbus RTU server at a baudrate of 9600 and device id 1.
On boot the target flashes a red led every 500ms. It will constantly poll the server with the following parameters:

- Device ID 0x1
- Funciton code 0x5 (Write Coil)
- Address 0x0000
- Data 0xFF00

If an expected response from the server is received the target assumes the connection is solid and will turn off the red blinking led and turn on a solid blue led. If the connection is lost the Arduino comes back to its initial state immediately. The star feature of this project is that when a button is pressed a different Modbus request is executed with the following parameters:

- Device ID 0x1
- Funciton code 0x5 (Write Coil)
- Address 0x0001
- Data 0xFF00

Once a response is received the red led will blink three times with a separation of one second. Enjoy!


## Building the Arduino Nano program

Install prerequisites:
```
sudo apt-get install gcc-avr binutils-avr avr-libc gdb-avr
```
To build
```
mkdir build
cd build
cmake ..
make
```

## Flashing the Arduino Nano program
Make sure to build first. Install prerequisites:
```
sudo apt-get install avrdude
```
To flash:
```
cd build
avrdude -c arduino -p m328p -P /dev/ttyUSB0 -b 57600 -v -U flash:w:blinker_clicker_5000_v2.hex
```

## Setting up a Modbus RTU Server
This can be acheived using the pymodbus library and an included example. Firstly make sure you have a python virtual environment set up:
```
sudo apt-get install python3-venv
python3 -m venv venv
chmod +x venv/bin/activate
source venv/bin/activate
```
Now you should see a confirmation on your terminal that you are in a virtual environment. Install pyserial, pymodbus and clone its repository to use an included example:
```
pip install pymodbus==3.12.0
pip install pyserial==3.5
git clone https://github.com/pymodbus-dev/pymodbus.git -b v3.12.0
```
Go to the examples directory and run the server. Make sure to replace the placeholder in the command with the name of the communications port you are using
```
cd pymodbus/examples
./server_async.py --comm serial --framer rtu --log debug --port <com_port> --device_ids 1
```

## Pin connections:
- Red LED: D3 (Arduino Nano) PD3 (AtMega328p)  - with a 510R resistor in series
- Blue LED: D5 (Arduino Nano) PD5 (AtMega328p)  - with a 510R resistor in series
- Button - Active Low: D9 (Arduino Nano) PB1 (AtMega328p)



