## Minecraft Clock LED display

- Create the following display using a Raspberry Pi and either a 16x16 or 32x16 LED display
- Potentially use extra space to display the time (for 32x16)

![clock](assets/minecraft-clock.gif)

## Setting up the display
- [The LED display to get](https://www.adafruit.com/product/420)
- [Guide for wiring the raspberry pi to the LED display](https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/wiring.md)


## Details of the implementation
The clock should accuractely represent the real world day-night cycle. 

- 'Noon' on the clock should represent real world solar noon
- 'Dusk' on the clock should represent real world dusk
- 'Midnight' on the clock should represent solar midnight
- 'Dawn' on the clock should represent the real world dawn
- All times in between noon and midnight should represent the corresponding solar time

![clock cycles](assets/minecraft-clock-cycles.gif)



## Eventual Upgrades
- [64x64 display](https://www.sparkfun.com/products/14824)
