// This sketch shows how to interface with ILI9314 TFT-LCD
// Guide: https://www.amebaiot.com/en/amebapro2-arduino-spi-lcd

#include "SPI.h"
#include "AmebaILI9341.h"
#include <string.h>

#include "muse_charm_240x320_RGB565.hpp"
#include "neutral_smile_240x320_RGB565.hpp"
#include "happy_smile_240x320_RGB565.hpp"
#include "waving_wink_240x320_RGB565.hpp"
#include "cheering_240x320_RGB565.hpp"
#include "shy_smile_240x320_RGB565.hpp"
#include "waving_240x320_RGB565.hpp"


#define TFT_RESET       5
#define TFT_DC          4
#define TFT_CS          SPI_SS

AmebaILI9341 tft = AmebaILI9341(TFT_CS, TFT_DC, TFT_RESET);

#define ILI9341_SPI_FREQUENCY 20000000

void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const unsigned short *color)
{
    tft.drawBitmap(x, y, w, h, color);
    delay(500);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("init ILI9341_TFTLCD...");

    SPI.setDefaultFrequency(ILI9341_SPI_FREQUENCY);

    tft.begin();
    tft.clr(); 
    Serial.println("ILI9341 TFT-LCD initialized !!!");        
}

void loop()
{
    Serial.println("draw Bitmap muse_charm_240x320_RGB565");   
    drawBitmap(0, 0, 240, 320, muse_charm_240x320_RGB565); 
    Serial.println("draw Bitmap neutral_smile_240x320_RGB565");   
    drawBitmap(0, 0, 240, 320, neutral_smile_240x320_RGB565);   
    Serial.println("draw Bitmap happy_smile_240x320_RGB565");   
    drawBitmap(0, 0, 240, 320, happy_smile_240x320_RGB565);
    Serial.println("draw Bitmap waving_wink_240x320_RGB565");
    drawBitmap(0, 0, 240, 320, waving_wink_240x320_RGB565);
    Serial.println("draw Bitmap cheerng_wink_240x320_RGB565");
    drawBitmap(0, 0, 240, 320, cheering_240x320_RGB565);
    Serial.println("draw Bitmap shy_smile_240x320_RGB565");
    drawBitmap(0, 0, 240, 320, shy_smile_240x320_RGB565);
    Serial.println("draw Bitmap waving_240x320_RGB565");
    drawBitmap(0, 0, 240, 320, waving_240x320_RGB565);
}
