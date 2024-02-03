# Sensirion I²C SHT4X Arduino Library

This is the Sensirion SHT4X library for Arduino allowing you to 
communicate with a sensor of the SHT4X family over I²C.

<img src="images/SHT4x.png" width="300px">

Click [here](https://sensirion.com/products/catalog/SEK-SHT40/) to learn more about the Sensirion SHT4X sensor family.



## Supported sensor types

| Sensor name   | I²C Addresses  |
| ------------- | -------------- |
|[SHT40](https://sensirion.com/products/catalog/SHT40/)| **0x44**, 0x45|
|[SHT41](https://sensirion.com/products/catalog/SHT41/)| **0x44**, 0x45|
|[SHT45](https://sensirion.com/products/catalog/SHT45/)| **0x44**, 0x45|

The following instructions and examples use a *SHT40*.



## Installation of the library

This library can be installed using the Arduino Library manager:
Start the [Arduino IDE](http://www.arduino.cc/en/main/software) and open
the Library Manager via

`Sketch` ➔ `Include Library` ➔ `Manage Libraries...`

Search for the `Sensirion I2C SHT4X` library in the `Filter
your search...` field and install it by clicking the `install` button.

If you cannot find it in the library manager, download the latest release as .zip file 
and add it to your [Arduino IDE](http://www.arduino.cc/en/main/software) via

`Sketch` ➔ `Include Library` ➔ `Add .ZIP Library...`

Don't forget to **install the dependencies** listed below the same way via library 
manager or `Add .ZIP Library`

#### Dependencies
* [Sensirion Core](https://github.com/Sensirion/arduino-core)

## Sensor wiring

Use the following pin description to connect your SHT4X to the standard I²C bus of your Arduino board:

<img src="images/SHT40_pinout.png" width="300px">

| *Pin* | *Cable Color* | *Name* | *Description*  | *Comments* |
|-------|---------------|:------:|----------------|------------|
| 1 | green | SDA | I2C: Serial data input / output | 
| 2 | black | GND | Ground | 
| 3 | yellow | SCL | I2C: Serial clock input | 
| 4 | red | VDD | Supply Voltage | 1.1V to 3.6V

The recommended voltage is 3.3V.

Please refer to the datasheet for proper circuit setup. There are 3rd party boards for easy connection of the SHT4x sensor to your Arduino Board.

### Board specific wiring
You will find pinout schematics for recommended board models below:



<details><summary>Arduino Uno</summary>
<p>

| *SHT4X* | *SHT4X Pin* | *Cable Color* | *Board Pin* |
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

| *SHT4X* | *SHT4X Pin* | *Cable Color* | *Board Pin* |
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

| *SHT4X* | *SHT4X Pin* | *Cable Color* | *Board Pin* |
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

| *SHT4X* | *SHT4X Pin* | *Cable Color* | *Board Pin* |
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

| *SHT4X* | *SHT4X Pin* | *Cable Color* | *Board Pin* |
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

2. Connect the SHT4X sensor to your Arduino as explained in [Sensor wiring](#sensor-wiring)

3. Open the `exampleUsage` sample project within the Arduino IDE:

   `File` ➔ `Examples` ➔ `Sensirion I2C SHT4X` ➔ `exampleUsage`

  
   The provided example is working with a SHT40, I²C address 0x44.
   In order to use the code with another product or I²C address you need to change it in the code of `exampleUsage`. 
   You find the list with pre-defined addresses in `src/SensirionI2CSht4x.h`.


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
