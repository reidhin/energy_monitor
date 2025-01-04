# Energy Monitor

Calculating home energy usage by using the led indicator on the energy meter.

The led indicator presumably flashes every time 1 Wh of energy is used (https://en.wikipedia.org/wiki/Electricity_meter).
By detecting the led flashes and measuring the time between two flashes the energy usage rate (or power) can be calculated as follows:
Power = 3600 / (time between flashes [sec]) [Watts]

## Ingredients

- [ESP8266](https://en.wikipedia.org/wiki/ESP8266) - the brain of the monitor;
- [GL5516 LDR](https://en.wikipedia.org/wiki/Photoresistor) Light-dependent resistor;
- Resistor of 10k Ohm

## Wiring

The light dependent resistor (LDR) is connected with a 10k Ohm resistor in series to divide the voltage. 
When the environment is dark, the LDR has a high resistance and the measured voltage tends to zero (ground). 
When the environment is light, the LDR has a low resistance and the measured voltage tends to 3.3 Volts.

The used wiring is shown below.

## Instructions

The codes tries to connect to the local Wifi network using the credentials listed in `arduino_secrets.h`. 
This file is not added to the repository; an example on how to structure this file can be seen in `arduino_secrets_example.h`.
The Wifi connection is needed to display the raw data and calculated energy usage in graphs.

## Code

### Peak detection

The code regulary reads the anologue value from connection A0. 
It does so by polling the value every 25 milliseconds, which results in 40 readings per second. 
One may consider higher or lower reading frequencies. 
However, if the reading frequency is too low, the light pulses may not be adequately captured. 
If, on the other hand, the reading frequency is too high, the ESP8266 may not be able to keep up.
In my case a reading frequency of 40 Hz proved to be fine.

The peak detection is done by monitoring when the read value exceeds a threshold above a calculated mean.
The mean is updated in every step using a certain lag. 
This approach allows for varying peak heights and varying environmental light conditions.

The edges of the peak and top of the peak are calculated.

Since the chosen approach does not lead to clear 1/0 signals, 
I have decided not to follow the path of interrupts like was done here: https://forum.arduino.cc/t/counting-pulse-of-energy-meter/550830/8
See this link for more information on interrupts: https://gammon.com.au/interrupts

The peak detection is losely based on:
- https://github.com/WorldFamousElectronics/PulseSensorPlayground/blob/master/src/utility/PulseSensor.cpp
- https://lastminuteengineers.com/pulse-sensor-arduino-tutorial/
- https://github.com/leandcesar/PeakDetection/blob/master/PeakDetection.cpp
- https://microcontrollerslab.com/pulse-sensor-arduino-tutorial/

### Storage in vectors

I would like to show a bit of history of previous peak detections and the raw data in a web interface. 
Therefore I would like to store, say, the last 200 peaks and measurements.
This can be done in arrays. 
To avoid constant resuffling of arrays every time a new value comes in, I use a trick with modulo.
I keep track of the total amount of peaks measured. The first 200 peaks are added to the array. Then the array is full.
The next peak will be peak number 201. This will be stored in index 201 % 200 = 1 which overwrites the oldest value.
And so on.

### Web server 

The web server uses two techniques to update the data on the web interface:

1. The raw data is regularly updated (every 5 seconds - which at 40 Hz holds 200 datapoints).
The update is initiated by the javascript.

2. Whenever a new peak is detected, a new event is created which is sent to all connected clients. 
This results in a data update of the energy graph.

### Graph drawing using highcharts.

The graph is drawn in the web interface using [Highcharts](https://www.highcharts.com/). 
See this example how this can be used in combination with ESP8266: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

## To do

An LDR is rather slow to react to sudden light changes. One may consider to use phototransisors instead, for example BPW40. 
The flat side = collector; the arrow points to the emittor. See:
- https://github.com/mruettgers/SMLReader
- https://www.bitsandparts.nl/Lichtsensor-Fototransistor-BPW40-p115130


## Project structure

```
├── arduino_secrets_example.h     <- example on how to structure arduino_secrets.h
├── arduino_secrets.h             <- file containin Wifi credentials
├── energy_monitor.ino          <- Main arduino code
├── data
│   ├── favicon.png               <- image with favicon
│   ├── index.css                 <- css file to style the html webinterface
│   ├── index.html                <- html file for the webinterface
│   └── index.js                  <- javascript handling of the graphs and updating of data
├── LICENSE.md                    <- License
└── README.md                     <- readme file with explanation
```

## License

<a rel="license" href="https://creativecommons.org/publicdomain/zero/1.0/">
<img alt="Creative Commons-Licentie" style="border-width:0" src="https://licensebuttons.net/l/publicdomain/88x31.png" />
</a>
<br />This work is subject to <a rel="license" href="https://creativecommons.org/publicdomain/zero/1.0/">Creative Commons CC0 1.0 Universal</a>.
