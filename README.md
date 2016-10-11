# GoldenAnt
Personal repository for my Arduino projects. Please visit the [wiki](../../wiki) page for extended information.

## Soil moisture sensor

Basic tool which blinks a green led when the soil moisture is still enough and a red light when the soil is too dry.

### Material

  - 1x Arduino Uno R3 
  - 3x Resistor 1K
  - 2x Resistor 10K
  - 1x Buzzer (Piezoelectric)
  - 1x Red LED
  - 1x Green LED
  - 1x LDR (Light Dependant Resistor)
  - 1x Pushbutton
  - 1x Soil moisture sensor*

\*The soil moisture sensor must be compatible with Arduino and can be easily found in several webs like eBay or GearBest. The model which appears on the wiring diagram is the version with 4 pins (Vcc, Gnd, AO and DO) but the model with 3 pins is also useful because the DO pin is not used in this design. The AO pin provides a voltage between 0V and 5V depending of the moisture (the drier is the soil, the higher is the voltage). And the D0 is a digital pin which becomes HIGH when a certain moisture value, set by a potentiometer, is reached.

### Wiring

![Soil moisture wiring](https://github.com/franciscoalario/GoldenAnt/blob/master/soil_moisture/resources/Soil_moisture.jpg)
The wiring has been designed using Proteus 8:

- [JPG Image](../blob/master/soil_moisture/resources/Soil_moisture.jpg)
- [Vectorial image](../blob/master/soil_moisture/resources/Soil_moisture.SVG)
- [Proteus 8 file](../blob/master/soil_moisture/resources/Soil_moisture.pdsprj)

### Features

The device measures the soil moisture and provides a different output depending the moisture level. The signal provided by the moisture sensor is an analog signal which varies from 0V for a completely wet soil to 5V for a dry soil. Arduino translates this voltage to an interval between 0 (0V) and 1023 (5V) and the software does the same conversion to 0%-100% to ease the setting of moisture levels, so 0% is a dry floor and 100% is completely wet.

Actually, the lower value doesn't reach 0 when the sensor is fully dipped in water, the minimum value for my design is 300, so my threshold for the soil moisture varies from 300 (100% wet) to 1023 (0% wet).

`100-map(value, 300, 1024, 0, 100)`

There are several outputs depending of the soil moisture. If the moisture is appropriate a green led blinks every ten minutes with a duration between 500ms and 1500ms depending on the moisture level.
`doBlink(GREEN_LED_PIN, map(val, MINIMUM_MOIST, 100, 500, 1500), 1);`

If the moisture level is lower than the appropriate level a red led blinks every minute with a duration between 250ms and 500ms and between 1 and 5 times depending on how dry is the soil.

`doLoudBlink(RED_LED_PIN, map(val, 0, MINIMUM_MOIST, 250, 500), 5-map(val, 0, MINIMUM_MOIST, 0, 4), 2000);`

In addition, if the environmental light is greater than a 60%, an alarm sounds joining the led blink.

`doBlink(RED_LED_PIN, map(val, 0, MINIMUM_MOIST, 250, 500), 5-map(val, 0, MINIMUM_MOIST, 0, 4));`

By default the minimum level for an appropriate soil moisture is 40% but can be changed using the button on the device.

The functionality of the button is as follows:
* **Push once**: This action forces the device to make a measurement of the soil. A green blink and a buzz will occur and then the proper led will become ON to indicate if the moisture is good or not.
* Keep pushing more than **2 seconds**: A single buzz indicates that this mode has been selected. Release the button and the current soil moisture will be set as the minimum proper moisture. Note that this design has not storage module, so when the devices is turned off this value is erased.
* Keep pushing more than **5 seconds**: A two-times buzz indicates that this mode has been selected. Release the button and the minimum proper moisture is set to the default value, 40%. This is the preset mode when the device is turned on.
* Keep pushing more than **7 seconds**: A three-times buzz indicates that this mode has been selected. Release the button and the test mode is activated. The devices test the both lights and the buzzer combining blinks and sounds.
