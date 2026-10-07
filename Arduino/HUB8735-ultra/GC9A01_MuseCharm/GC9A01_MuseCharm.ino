/*!
	@file   GC9A01_MuseCharm.ino
	@author Richard Kuo
	@brief  Example file for GC9A01_LTSM bitmap test
	@note   See USER OPTIONS in SETUP function
*/

// libraries
#include "src/ltsm/GC9A01_LTSM.hpp"

// bitmap test data
#include "bitmap_cheering.hpp"
#include "bitmap_happy_smile.hpp"
#include "bitmap_neutral_smile.hpp"
#include "bitmap_shy_smile.hpp"
#include "bitmap_waving.hpp"
#include "bitmap_waving_wink.hpp"

//  Test timing related defines
#define DRAW_DELAY 200

#ifdef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
#pragma message("gll: dislib16_ADVANCED_SCREEN_BUFFER_ENABLE is defined. This example is not for that mode")
#endif

GC9A01_LTSM myTFT;
bool bhardwareSPI = true;  // true for hardware spi, false for software

void setup(void) {
  Serial.begin(115200);

  // === USER OPTION 1 SPI_SPEED + TYPE ===
  // HUB8735 Ultra wiring: SCLK=19, MOSI/SDA=20, CS=22, DC=23, RST=24.
  // SPI1 also reserves MISO=18 and hardware SS=21; leave them unconnected.
  int8_t CS_TFT = 22; 
  int8_t DC_TFT = 23;
  int8_t RST_TFT = 24;  
  if (bhardwareSPI == true) {          // hw spi
    uint32_t TFT_SCLK_FREQ = 8000000;  // Spi freq in Hertz
    myTFT.TFTsetupGPIO_SPI(TFT_SCLK_FREQ, RST_TFT, DC_TFT, CS_TFT);
    myTFT.TFTsetSPIBus(SPI1); // Initialization and every transfer use SPI1.
  } else {                        // sw spi
    uint16_t SWSPICommDelay = 0;  // optional SW SPI GPIO delay in uS
    int8_t SDIN_TFT = SPI1_MOSI; // 20
    int8_t SCLK_TFT = SPI1_SCLK; // 19
    myTFT.TFTsetupGPIO_SPI(SWSPICommDelay, RST_TFT, DC_TFT, CS_TFT, SCLK_TFT, SDIN_TFT);
  }
  // === USER OPTION 2 Screen Setup ===
  uint16_t TFT_WIDTH = 240;   // Screen width in pixels
  uint16_t TFT_HEIGHT = 240;  // Screen height in pixels
  myTFT.TFTInitScreenSize(TFT_WIDTH, TFT_HEIGHT);

  myTFT.TFTGC9A01Initialize();
  Serial.println("TFT init done.");

}

void Draw_Bitmap() {
  Serial.println("Draw MuseCharm");
  myTFT.drawBitmap16Data(0, 0, neutral_smile, 240, 240); 
  delay(DRAW_DELAY);  
  myTFT.drawBitmap16Data(0, 0, happy_smile, 240, 240); 
  delay(DRAW_DELAY);
  myTFT.drawBitmap16Data(0, 0, shy_smile, 240, 240); 
  delay(DRAW_DELAY);  
  myTFT.drawBitmap16Data(0, 0, waving, 240, 240); 
  delay(DRAW_DELAY);
  myTFT.drawBitmap16Data(0, 0, waving_wink, 240, 240); 
  delay(DRAW_DELAY);
  myTFT.drawBitmap16Data(0, 0, cheering, 240, 240); 
  delay(DRAW_DELAY);
  Serial.println("Done");   
}

void loop(void) {
  Draw_Bitmap();  
}
