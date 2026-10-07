# GC9A01 MuseCharm on HUB8735 Ultra SPI1

Open `GC9A01_MuseCharm.ino` in Arduino IDE and select **HUB8735 Ultra**.
Keep `bhardwareSPI = true` to use hardware SPI1 at 8 MHz.

| Display signal | Arduino pin |
| --- | --- |
| SCLK / SCL | 19 (`SPI1_SCLK`) |
| MOSI / SDA / DIN | 20 (`SPI1_MOSI`) |
| CS | 22 |
| DC | 23 |
| RST | 24 |

The board SPI1 implementation also initializes MISO 18 and hardware SS 21.
Leave those pins unconnected to the display; the driver controls display CS 22
as a GPIO. Supply the display and backlight according to the module requirements
and connect a common ground.

The original libraries use the global `SPI` object for initialization,
transactions, transfers, and shutdown. Calling `SPI1.begin()` in the sketch
alone therefore does not select SPI1 for the display. The sketch now calls
`myTFT.TFTsetSPIBus(SPI1)` before display initialization. The local driver also
configures the display CS pin as an output in hardware mode.

`src/ltsm/` contains the required sources from the locally installed
GC9A01_LTSM 1.1.1 and display16_LTSM 1.0.2, including the default font and
original licenses. All SPI operations use the selected bus; the default remains
`SPI`. Includes resolve within the sketch so Arduino compiles this corrected
copy without requiring changes to shared installed libraries. Keep the `src`
folder with the sketch. Original projects:
https://github.com/gavinlyonsrepo/GC9A01_LTSM and
https://github.com/gavinlyonsrepo/display16_LTSM.

Validation: compiled and linked for
`ideasHatch:AmebaPro2:Ameba_HUB-8735_ultra` with the installed 4.1.1-Release
board package. A host simulation of the actual sketch and drivers confirmed
SPI1 initialization, bitmap transfers, balanced 8 MHz mode-0 transactions,
GPIO CS handling, and SPI1 shutdown with the default SPI bus untouched.
Physical display operation still requires testing on the board.
