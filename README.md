# BMP280 sensor example

## Supported sensor interfaces
* SPI 4-wire

## Supported devices
* Jetson Nano
* Jetson Xavier NX
* Raspberry Pi4

## Functions
### instance(device, address)
* **Returns the shared pointer to the instance's sensor. Performs sensor configuration with default settings.**
* *device* - I2C file name to which the sensor is connected. Example */dev/i2c-1*.
* *address* - BMP280 I2C address(default 0x77).

### getTemperature()
* **Returns the temperature in degrees °C.**

### getHumidity()
* **Returns the humidity in percentage.**

### getQfePressure()
* **Returns the QFE pressure in ppa.**

### getQnhPressure(double altitude)
* **Returns the QFE pressure in ppa.**
* *altitude* - Altitude of measuring point.

### paToHg(float ppa)
* **Returns the converted pressure value from pascals to millimeters of mercury.**
* *ppa* - Pressure obtained by functions ***getQfePressure*** and ***getQnhPressure***.

### calcDewpoint(float humidity, float temperature)
* **Returns the dewpoint in degrees °C**
* *humidity* - Humidity in percentage.
* *temperature* - Temperature in degrees °C.

### setConfig(Config config)
* **Set config for sensor**
* *config* - Config.

| Config                           | Description                   | Default value      |
|:---------------------------------|:------------------------------|:-------------------|
| mode                             | SLEEP                         | *NORMAL*           |
|                                  | FORCED                        |                    |
|                                  | NORMAL                        |                    |
| ---                              | ---                           | ---                |
| filter                           | OFF                           | *OFF*              |
|                                  | _2                            |                    |
|                                  | _4                            |                    |
|                                  | _8                            |                    |
|                                  | _16                           |                    |
| ---                              | ---                           | ---                |
| oversamplingPressure             | SKIPPED                       | *STANDARD*         |
|                                  | ULTRA_LOW_POWER               |                    |
|                                  | LOW_POWER                     |                    |
|                                  | STANDARD                      |                    |
|                                  | HIGH_RES                      |                    |
|                                  | ULTRA_HIGH_RES                |                    |
| ---                              | ---                           | ---                |
| oversamplingTemperature          | SKIPPED                       | *STANDARD*         |
|                                  | ULTRA_LOW_POWER               |                    |
|                                  | LOW_POWER                     |                    |
|                                  | STANDARD                      |                    |
|                                  | HIGH_RES                      |                    |
|                                  | ULTRA_HIGH_RES                |                    |
| ---                              | ---                           | ---                |
| oversamplingHumidity             | SKIPPED                       | *STANDARD*         |
|                                  | ULTRA_LOW_POWER               |                    |
|                                  | LOW_POWER                     |                    |
|                                  | STANDARD                      |                    |
|                                  | HIGH_RES                      |                    |
|                                  | ULTRA_HIGH_RES                |                    |
| ---                              | ---                           | ---                |
| standby                          | _05ms                         | *_500ms*           |
|                                  | _62ms                         |                    |
|                                  | _125ms                        |                    |
|                                  | _250ms                        |                    |
|                                  | _500ms                        |                    |
|                                  | _1000ms                       |                    |
|                                  | _2000ms                       |                    |
|                                  | _4000ms                       |                    |
| ---                              | ---                           | ---                |