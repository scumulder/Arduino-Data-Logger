# Arduino-Data-Logger

This project uses an Arduino UNO and a datalogger shield (like the one found [here](goo.gl/DzUzoR)) to record voltage readings via ADC and save them in a .csv file on a SD card. The data logger also records the date and time corresponding to the readings.  
  
Additional functionality is added to make the logging process more user friendly such as:

- Push button input to start the data logging process
- LCD screen to display the status of the process or any errors along the way
- Easily changeable log time duration in the code

[Here](https://vimeo.com/238146295) is a link to the video demonstrating the process. The voltage input received are from a simple voltage divider, 5V, and GND. The duration is set for 1 minute.


[This](https://learn.adafruit.com/adafruit-data-logger-shield/overview) tutorial by Adafruit was used to become familiar with the hardware. 

