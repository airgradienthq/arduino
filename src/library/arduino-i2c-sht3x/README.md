# Sensirion I²C SHT3X Arduino Library

This is the Sensirion SHT3X library for Arduino allowing you to 
communicate with a sensor of the SHT3X family over I²C.

<img src="images/SHT3x.png" width="300px">

Click [here](https://sensirion.com/products/catalog/SHT30-DIS-B) to learn more about the Sensirion SHT3X sensor family.


Not all sensors of this driver family support all measurements.
In case a measurement is not supported by all sensors, the products that
support it are listed in the API description.



## Supported sensor types

| Sensor name   | I²C Addresses  |
| ------------- | -------------- |
|[SHT30A](https://sensirion.com/products/catalog/SHT30A-DIS-B)| **0x44**, 0x45|
|[SHT30](https://sensirion.com/products/catalog/SHT30-DIS-B)| **0x44**, 0x45|
|[SHT31A](https://sensirion.com/products/catalog/SHT31A-DIS-B)| **0x44**, 0x45|
|[SHT31](https://sensirion.com/products/catalog/SHT31-DIS-B)| **0x44**, 0x45|
|[SHT33](https://sensirion.com/products/catalog/SHT33-DIS)| **0x44**, 0x45|
|[SHT35A](https://sensirion.com/products/catalog/SHT35A-DIS-B)| **0x44**, 0x45|
|[SHT35](https://sensirion.com/products/catalog/SHT35-DIS-B)| **0x44**, 0x45|
|[SHT85](https://sensirion.com/sht85)| **0x44** |

The following instructions and examples use a *SHT30*.



## Installation of the library

This library can be installed using the Arduino Library manager:
Start the [Arduino IDE](http://www.arduino.cc/en/main/software) and open
the Library Manager via

`Sketch` ➔ `Include Library` ➔ `Manage Libraries...`

Search for the `Sensirion I2C SHT3X` library in the `Filter
your search...` field and install it by clicking the `install` button.

If you cannot find it in the library manager, download the latest release as .zip file 
and add it to your [Arduino IDE](http://www.arduino.cc/en/main/software) via

`Sketch` ➔ `Include Library` ➔ `Add .ZIP Library...`

Don't forget to **install the dependencies** listed below the same way via library 
manager or `Add .ZIP Library`

#### Dependencies
* [Sensirion Core](https://github.com/Sensirion/arduino-core)

## Sensor wiring

Use the following pin description to connect your SHT3X to the standard I²C bus of your Arduino board:

<img src="images/SHT3x_pinout.png" width="300px">

| *Pin* | *Cable Color* | *Name* | *Description*  | *Comments* |
|-------|---------------|:------:|----------------|------------|
| 1 | green | SDA | I2C: Serial data input / output | 
| 2 | black | GND | Ground | 
| 3 | yellow | SCL | I2C: Serial clock input | 
| 4 | red | VDD | Supply Voltage | 2.15V to 5.5V




The recommended voltage is 3.3V.

### Board specific wiring
You will find pinout schematics for recommended board models below:



<details><summary>Arduino Uno</summary>
<p>

| *SHT3X* | *SHT3X Pin* | *Cable Color* | *Board Pin* |
| :---: | --- | --- | --- |
| SDA | 1 | green | D18/SDA |
| GND | 2 | black | GND |
| SCL | 3 | yellow | D19/SCL |
| VDD | 4 | red | 3.3V |



<img src="images/Arduino-Uno-Rev3-i2c-pinout-3.3V.png" width="600px">
</p>
</details>



<details><summary>Arduino Nano</summary>
<p>

| *SHT3X* | *SHT3X Pin* | *Cable Color* | *Board Pin* |
| :---: | --- | --- | --- |
| SDA | 1 | green | A4 |
| GND | 2 | black | GND |
| SCL | 3 | yellow | A5 |
| VDD | 4 | red | 3.3V |



<img src="images/Arduino-Nano-i2c-pinout-3.3V.png" width="600px">
</p>
</details>



<details><summary>Arduino Micro</summary>
<p>

| *SHT3X* | *SHT3X Pin* | *Cable Color* | *Board Pin* |
| :---: | --- | --- | --- |
| SDA | 1 | green | D2/SDA |
| GND | 2 | black | GND |
| SCL | 3 | yellow | ~D3/SCL |
| VDD | 4 | red | 3.3V |



<img src="images/Arduino-Micro-i2c-pinout-3.3V.png" width="600px">
</p>
</details>



<details><summary>Arduino Mega 2560</summary>
<p>

| *SHT3X* | *SHT3X Pin* | *Cable Color* | *Board Pin* |
| :---: | --- | --- | --- |
| SDA | 1 | green | D20/SDA |
| GND | 2 | black | GND |
| SCL | 3 | yellow | D21/SCL |
| VDD | 4 | red | 3.3V |



<img src="images/Arduino-Mega-2560-Rev3-i2c-pinout-3.3V.png" width="600px">
</p>
</details>



<details><summary>ESP32 DevKitC</summary>
<p>

| *SHT3X* | *SHT3X Pin* | *Cable Color* | *Board Pin* |
| :---: | --- | --- | --- |
| SDA | 1 | green | GPIO 21 |
| GND | 2 | black | GND |
| SCL | 3 | yellow | GPIO 22 |
| VDD | 4 | red | 3V3 |



<img src="images/esp32-devkitc-i2c-pinout-3.3V.png" width="600px">
</p>
</details>


## Quick Start

1. Install the libraries and dependencies according to [Installation of the library](#installation-of-the-library)

2. Connect the SHT3X sensor to your Arduino as explained in [Sensor wiring](#sensor-wiring)

3. Open the `exampleUsage` sample project within the Arduino IDE:

   `File` ➔ `Examples` ➔ `Sensirion I2C SHT3X` ➔ `exampleUsage`

  
   The provided example is working with a SHT30, I²C address 0x44.
   In order to use the code with another product or I²C address you need to change it in the code of `exampleUsage`. 
   You find the list with pre-defined addresses in `src/SensirionI2CSht3x.h`.


5. Click the `Upload` button in the Arduino IDE or `Sketch` ➔ `Upload`

4. When the upload process has finished, open the `Serial Monitor` or `Serial
   Plotter` via the `Tools` menu to observe the measurement values. Note that
   the `Baud Rate` in the used tool has to be set to `115200 baud`.

## Contributing

**Contributions are welcome!**

We develop and test this driver using our company internal tools (version
control, continuous integration, code review etc.) and automatically
synchronize the master branch with GitHub. But this doesn't mean that we don't
respond to issues or don't accept pull requests on GitHub. In fact, you're very
welcome to open issues or create pull requests :)

This Sensirion library uses
[`clang-format`](https://releases.llvm.org/download.html) to standardize the
formatting of all our `.cpp` and `.h` files. Make sure your contributions are
formatted accordingly:

The `-i` flag will apply the format changes to the files listed.

```bash
clang-format -i src/*.cpp src/*.h
```

Note that differences from this formatting will result in a failed build until
they are fixed.


## License

See [LICENSE](LICENSE).
