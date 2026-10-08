/*!
	@file   BITMAP.ino
	@author Gavin Lyons
	@brief  Example file for GC9A01_LTSM bitmap tests + FPS bitmap test.
	@note   See USER OPTIONS in SETUP function
	@test
		-# Test 300 Sprite 
		-# Test 301 icons
		-# Test 302 bi-color small image
		-# Test 303 bi-color full screen image 128x128
		-# Test 304 16 bit color image from a data array
		-# Test 305 16 bit color image data from a data array
		-# Test 306 8 bit color image data from a data array
    -# Test 400 16 bit bitmap
		-# Test 601 FPS bitmap results to serial port
*/

// libraries
#include "GC9A01_LTSM.hpp"

// bitmap test data
#include "Bitmap_BlueMarble.hpp"
//#include "Bitmap_MuseCharm.hpp"

/// @cond

//  Test timing related defines
#define TEST_DELAY1 1000
#define TEST_DELAY2 2000
#define TEST_DELAY5 5000

#ifdef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
#pragma message("gll: dislib16_ADVANCED_SCREEN_BUFFER_ENABLE is defined. This example is not for that mode")
#endif

GC9A01_LTSM myTFT;
bool bhardwareSPI = true;  // true for hardware spi, false for software

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  // === USER OPTION 1 SPI_SPEED + TYPE ===
  int8_t DC_TFT = 4;
  int8_t RST_TFT = 6;
  int8_t CS_TFT = 5;
  if (bhardwareSPI == true) {          // hw spi
    uint32_t TFT_SCLK_FREQ = 8000000;  // Spi freq in Hertz
    myTFT.TFTsetupGPIO_SPI(TFT_SCLK_FREQ, RST_TFT, DC_TFT, CS_TFT);
  } else {                        // sw spi
    uint16_t SWSPICommDelay = 0;  // optional SW SPI GPIO delay in uS
    int8_t SDIN_TFT = 13;
    int8_t SCLK_TFT = 15;
    myTFT.TFTsetupGPIO_SPI(SWSPICommDelay, RST_TFT, DC_TFT, CS_TFT, SCLK_TFT, SDIN_TFT);
  }
  // ===
  // === USER OPTION 2 Screen Setup ===
  uint16_t TFT_WIDTH = 240;   // Screen width in pixels
  uint16_t TFT_HEIGHT = 240;  // Screen height in pixels
  myTFT.TFTInitScreenSize(TFT_WIDTH, TFT_HEIGHT);
  // ===
  myTFT.TFTGC9A01Initialize();
  Serial.println("Start");
}

//  MAIN
void loop(void) {
  Test400();
  EndTests();
}
// *** End OF MAIN **

/*!
	@brief  Test400 16 bit color image from a data array
*/
void Test400(void) {
  char teststr1[] = "Test 400";
  Serial.println("Test 400");
  myTFT.writeCharString(55, 55, teststr1);
  delay(TEST_DELAY5);

  myTFT.drawBitmap16Data(0, 0, BlueMarble, 240, 240);
  Serial.println("Test End");   
  delay(TEST_DELAY5);
  myTFT.fillScreen(myTFT.C_BLACK);

}

/*!
	@brief  Stop testing and shutdown the TFT
*/
void EndTests(void) {
  char teststr1[] = "Tests over";
  myTFT.fillScreen(myTFT.C_BLACK);
  myTFT.writeCharString(75, 75, teststr1);
  delay(TEST_DELAY5);
  myTFT.TFTPowerDown();
  Serial.println("Tests Over");
  while (1) {};
}
// *************** EOF ****************

/// @endcond
