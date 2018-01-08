# ESP singlecore logger: air_sensor.ino
Simple Temperature and Humidity logger using AM2301 sensor, SD card and I2C OLED. 

Running on ESP32 (DOIT devkit v1.0), but is meant for ESP8266, hence the single core utilization.

Connections:

OLED:<br>
GND - GND<br>
VCC - 3V3<br>
SCL - D22<br>
SDA - D21<br>

Sensor:<br>
B - GND<br>
R - 3V3<br>
Y - D15<br>

SD-card:<br>
GND&nbsp; - GND<br>
VCC&nbsp;&nbsp; - Vin<br>
MOSO - D19<br>
MOSI - D23<br>
SCK&nbsp;  - D18<br>
CS &nbsp;&nbsp;  - D5

# ESP singlecore logger: plotTandH.py
Very basic python script to plot the data. 
