// Sketch-local copy, modified 2026-10-07: explicit SPI bus selection, GPIO CS setup, and local includes.
/*!
	@file    display16_font_LTSM.hpp
	@author  Gavin Lyons
	@brief   header file for font class, 16-bit arduino display library. Display16_LTSM
*/

#pragma once

// Includes
#include "display16_common_LTSM.hpp"
#include "fonts_LTSM/FontDefault_LTSM.hpp" // Only needed to set defualt font

/*! @brief Font class to hold font data object  */
class display_Fonts
{
	public:
		display_Fonts();
		~display_Fonts(){};
		
		DisLib16::Ret_Codes_e  setFont(const uint8_t* font);
		void setInvertFont(bool invertStatus);
		bool getInvertFont(void);

	protected:
		const uint8_t*  _FontSelect = FontDefault;  /**< Pointer to the active font,  Fonts Stored are Const */
		uint8_t _Font_X_Size = 0x08; /**< Width Size of a Font character */
		uint8_t _Font_Y_Size = 0x08; /**< Height Size of a Font character */
		uint8_t _FontOffset = 0x20; /**< Offset in the ASCII table 0x00 to 0xFF, where font begins */
		uint8_t _FontNumChars = 0x5F; /**< Number of characters in font (0x00 to 0xFE) -1 */
	private:
		bool _FontInverted = false; /**< display font inverted? , False = no invert , true = invert*/
};
