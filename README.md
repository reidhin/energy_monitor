# energy_monitor

Calculating energy usage by using the led indicator on the energy meter.

The led indicator presumably flashes every time 1 Wh of energy is used (https://en.wikipedia.org/wiki/Electricity_meter).
By detecting the led flashes and measuring the time between two flashes the energy usage rate (or power) can be calculated.
Power = 3600 / (time between flashes [sec]) [Watts]

// https://github.com/mruettgers/SMLReader
// https://www.bitsandparts.nl/Lichtsensor-Fototransistor-BPW40-p115130
// BPW40
// flat side = collector
// pijl gaat naar emitter

// https://subscription.packtpub.com/book/iot-and-hardware/9781785888564/3/ch03lvl1sec27/reading-and-counting-pulses-with-arduino
// https://roboticsbackend.com/arduino-pulsein-with-interrupts/
// https://forum.arduino.cc/t/counting-pulse-of-energy-meter/550830/8

// follow this first: https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

// peak detection
// https://github.com/WorldFamousElectronics/PulseSensorPlayground/blob/master/src/utility/PulseSensor.cpp
// https://lastminuteengineers.com/pulse-sensor-arduino-tutorial/
// https://github.com/leandcesar/PeakDetection/blob/master/PeakDetection.cpp
// https://microcontrollerslab.com/pulse-sensor-arduino-tutorial/

// graph in html
// https://www.highcharts.com/demo/highcharts/combo-regression

// interrupts: https://gammon.com.au/interrupts