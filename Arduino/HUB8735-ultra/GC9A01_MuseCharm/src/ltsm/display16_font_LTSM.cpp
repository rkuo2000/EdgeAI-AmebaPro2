// Sketch-local copy, modified 2026-10-07: explicit SPI bus selection, GPIO CS setup, and local includes.
/*!
	@file    display16_font_LTSM.cpp
	@author  Gavin Lyons
	@brief   Source file for font class, 16-bit arduino display library. Display16_LTSM
*/

#include "display16_font_LTSM.hpp"

// === Font class implementation ===
/*!
	@brief init the font class object constructor,
		   Set the default font on initialization
 */
display_Fonts::display_Fonts(){setFont(FontDefault); };

/*!
	@brief SetFont 
	@param font Selected Font Name,  pass the font name
	@return	Will return rdlib::Return_Codes_e  enum
		-# Pass rdlib::Success
		-# Error 1 rdlib::FontPtrNullptr
 */
DisLib16::Ret_Codes_e  display_Fonts::setFont(const uint8_t* font) {

	if (font == nullptr)
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error setFont, Font data is nullptr");
		#endif
		return DisLib16::FontPtrNullptr;
	}
	_FontSelect   = font;
	_Font_X_Size  = _FontSelect[0];
	_Font_Y_Size  = _FontSelect[1];
	_FontOffset   = _FontSelect[2];
	_FontNumChars = _FontSelect[3];
	_FontInverted = false;

	return DisLib16::Success;
}

/*!
	@brief setInvertFont
	@param invertStatus set the invert status flag of font ,false = off.
*/
void display_Fonts::setInvertFont(bool invertStatus) {
	 _FontInverted = invertStatus;
}

/*!
	@brief getInvertFont
	@return invert status flag of font ,false = off.
*/
bool display_Fonts::getInvertFont()
{ return _FontInverted; }

// === End of Font class implementation ===
