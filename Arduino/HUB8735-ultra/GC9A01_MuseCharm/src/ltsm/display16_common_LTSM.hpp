// Sketch-local copy, modified 2026-10-07: explicit SPI bus selection, GPIO CS setup, and local includes.
/*!
	@file     display16_common_LTSM.hpp
	@details  This file contains various common elements used by 16-bit arduino display library. Display16_LTSM
			  Common includes , User options, GPIO Abstraction macros, return code enum.     
			  User Options section 
				-# Option 1: dislib16_ADVANCED_GRAPHICS_ENABLE enables advanced graphic functions
				-# Option 2: dislib16_ADVANCED_SCREEN_BUFFER_ENABLE enables screen buffer mode.
				-# Option 3: dislib16_DEBUG_MODE_ENABLE enables debug messages.
 */

#pragma once
 
#if ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif
#include <stdint.h> 

#if defined(__AVR__)
	#include <avr/pgmspace.h>
	#define FLASH_STORAGE PROGMEM
#elif defined(ESP8266) || defined(ESP32)
	#include <pgmspace.h>
	#define FLASH_STORAGE PROGMEM
#else
	#define FLASH_STORAGE
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif

// Section: User options 
// ================================================================
// ===== USER OPTION 1 turns on advanced graphics if commented in
// default is off
//#define dislib16_ADVANCED_GRAPHICS_ENABLE
// ================================================================
// ===== USER OPTION 2 turns on screenBuffer mode if commented in
// default is off
//#define dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
// ================================================================
// ===== USER OPTION 3 turns on debug mode,  Enable debug messages if commented in
// default is off
//#define dislib16_DEBUG_MODE_ENABLE
// ================================================================
// End of user options section

/*! namespace for Error enum*/
namespace DisLib16{
/*! Enum to define a standard return code for most functions that return failures*/
enum Ret_Codes_e : uint8_t
{
	Success = 0,                 /**< Success, Function ran without defined Error*/
	Reserved = 1,                /**< Reserved */
	WrongFont = 2,               /**< Wrong Font selected*/
	CharScreenBounds = 3,        /**< Text Character is out of Screen bounds, Check x and y*/
	CharFontASCIIRange = 4,      /**< Text Character is outside of chosen Fonts ASCII range, Check the selected Fonts ASCII range.*/
	CharArrayNullptr = 5,        /**< Text Character Array is an invalid pointer object*/
	FontPtrNullptr = 6,          /**< Pointer to the font is an invalid pointer object*/
	FontNotEnabled = 7,          /**< Is the font you selected enabled in _font.hpp? */
	BitmapNullptr = 8,           /**< The Bitmap data array is an invalid pointer object*/
	BitmapScreenBounds = 9,      /**< The Bitmap starting point is outside screen bounds, check x and y*/
	BitmapLargerThanScreen = 10, /**< The Bitmap is larger than screen, check  w and h*/
	BitmapVerticalSize = 11,     /**< A vertical Bitmap's height must be divisible by 8.*/
	BitmapHorizontalSize = 12,   /**< A horizontal Bitmap's width must be divisible by 8*/
	BitmapSize = 13,             /**< Size of the Bitmap is incorrect: BitmapSize(vertical)!=(w*(h/8),BitmapSize(horizontal)!=(w/8)*h*/
	BufferSize = 14,             /**< Size of the Buffer is incorrect: BufferSize(vertical)!=(w*(h/8)*/
	BufferNullptr = 15,          /**< The Buffer data array is an invalid pointer object*/
	BufferEmpty = 16,            /**< Empty buffer object */
	ShapeScreenBounds = 17,      /**< Shape is outside screen bounds, check x and y */
	IconScreenWidth = 18,        /**< Icon is greater than screen width , check width w value */
	BitmapDataEmpty = 19,        /**< Empty bitmap span object  */
	GenericError = 20,           /**< Generic Error */
	FontDataEmpty = 21,          /**< There is no data in selected font. */
	MemoryAError = 22            /**<  Memory allocation failure*/
};
}

// GPIO Abstractions for Arduino
#define DISPLAY16_DC_SetHigh        digitalWrite(_display_DC, HIGH)
#define DISPLAY16_DC_SetLow         digitalWrite(_display_DC, LOW)
#define DISPLAY16_RST_SetHigh       digitalWrite(_display_RST, HIGH)
#define DISPLAY16_RST_SetLow        digitalWrite(_display_RST, LOW)
#define DISPLAY16_CS_SetHigh        digitalWrite(_display_CS, HIGH)
#define DISPLAY16_CS_SetLow         digitalWrite(_display_CS, LOW)
#define DISPLAY16_SCLK_SetHigh      digitalWrite(_display_SCLK, HIGH)
#define DISPLAY16_SCLK_SetLow       digitalWrite(_display_SCLK, LOW)
#define DISPLAY16_SDATA_SetHigh     digitalWrite(_display_SDATA, HIGH)
#define DISPLAY16_SDATA_SetLow      digitalWrite(_display_SDATA, LOW)

#define DISPLAY16_DC_SetDigitalOutput    pinMode(_display_DC, OUTPUT)
#define DISPLAY16_RST_SetDigitalOutput   pinMode(_display_RST, OUTPUT)
#define DISPLAY16_CS_SetDigitalOutput    pinMode(_display_CS, OUTPUT)
#define DISPLAY16_SCLK_SetDigitalOutput  pinMode(_display_SCLK, OUTPUT)
#define DISPLAY16_SDATA_SetDigitalOutput pinMode(_display_SDATA, OUTPUT)

// Delays
#define MILLISEC_DELAY delay
#define MICROSEC_DELAY delayMicroseconds
