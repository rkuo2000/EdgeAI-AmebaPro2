// Sketch-local copy, modified 2026-10-07: explicit SPI bus selection, GPIO CS setup, and local includes.
/*!
	@file     display16_graphics_LTSM.cpp
	@author   Gavin Lyons
	@brief    Source file for graphics class. 16-bit arduino display library. Display16_LTSM
*/

#include "display16_graphics_LTSM.hpp"

/*!
	@brief Construct a new  graphics object
 */
display16_graphics_LTSM::display16_graphics_LTSM() {}

/*!
	@brief Draw a pixel to screen
	@param x  Column co-ord
	@param y  row co-ord
	@param color 565 16-bit
	@details  By default uses spiWriteDataBuffer method to write each row as a buffer for speed.
			Much faster than pixel by pixel spi byte writes
			If dislib16_ADVANCED_SCREEN_BUFFER_ENABLE is defined then the function 
			will use the screen buffer to draw the pixel into VRAM.
			Will return early if x or y are out of bounds.
*/
void display16_graphics_LTSM ::drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= _width) || (y >= _height))
		return;
#ifdef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
	// Calculate the index in the buffer
	size_t index = (y * _width + x) * 2; // 2 bytes per pixel for RGB565
	// Write the color to the buffer
	_screenBuffer[index] = (uint8_t)(color >> 8);     // High byte
	_screenBuffer[index + 1] = (uint8_t)(color & 0xFF); // Low byte
#else
	setAddrWindow(x, y, x + 1, y + 1);
	uint8_t TransmitBuffer[2]{(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
	spiWriteDataBuffer(TransmitBuffer, 2);
#endif
}

/*!
	@brief fills a rectangle starting from coordinates (x,y) with width of w and height of h.
	@param x x coordinate
	@param y y coordinate
	@param w width of the rectangle
	@param h height of the rectangle
	@param color color to fill rectangle 565 16-bit
	@return
		-# Display_Success for success
		-# Display_ShapeScreenBounds out of screen bounds
	@note  uses spiWriteBuffer method in non-frame buffer mode
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::fillRectBuffer(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	// Check bounds
	if ((x >= _width) || (y >= _height))
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error fillRectBuffer: Out of screen bounds");
		#endif
		return DisLib16::ShapeScreenBounds;
	}
	if ((x + w - 1) >= _width)
		w = _width - x;
	if ((y + h - 1) >= _height)
		h = _height - y;

	// Convert color to bytes
	uint8_t hi = color >> 8;
	uint8_t lo = color;
	// Row buffer for one row of the rectangle
	uint8_t rowBuffer[w * 2]; // Each pixel is 2 bytes (16-bit color)
	// Fill the row buffer with the color
	for (uint32_t i = 0; i < w; i++)
	{
		rowBuffer[2 * i] = hi;
		rowBuffer[2 * i + 1] = lo;
	}
	// Draw the rectangle row by row
	for (uint16_t j = 0; j < h; j++)
	{
		// Set the address window for the current row
		setAddrWindow(x, y + j, x + w - 1, y + j);
		// Write the row buffer to the display
		spiWriteDataBuffer(rowBuffer, w * 2);
	}
	return DisLib16::Success;

}

/*!
	@brief Fills the whole screen with a given color.
	@param  color 565 16-bit
*/
void display16_graphics_LTSM ::fillScreen(uint16_t color)
{
	fillRectBuffer(0, 0, _width, _height, color);
}

/*!
	@brief Draws a vertical line starting at (x,y) with height h.
	@param x The starting x coordinate
	@param y The starting y coordinate
	@param h The height of the line
	@param color The color of the line 565 16 Bit color
*/
void display16_graphics_LTSM ::drawFastVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
	if ((x >= _width) || (y >= _height))
		return;
	if ((y + h - 1) >= _height)
		h = _height - y;

#ifdef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
	for (uint16_t i = 0; i < h; i++)
	{
		drawPixel(x, y + i, color);
	}
#else
	uint8_t hi, lo;
	hi = color >> 8;
	lo = color;
	setAddrWindow(x, y, x, y + h - 1);
	DISPLAY16_DC_SetHigh;
	spiStartTransaction();
	while (h--)
	{
		spiWrite(hi);
		spiWrite(lo);
	}
	spiEndTransaction();
#endif
}

/*!
	@brief Draws a horizontal line starting at (x,y) with width w.
	@param x The starting x coordinate
	@param y The starting y coordinate
	@param w The width of the line
	@param color The color of the line 565 16 Bit color
*/
void display16_graphics_LTSM ::drawFastHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{

	if ((x >= _width) || (y >= _height))
		return;
	if ((x + w - 1) >= _width)
		w = _width - x;
#ifdef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
	for (uint16_t i = 0; i < w; i++)
	{
		drawPixel(x+i, y, color);
	}
#else
	uint8_t hi, lo;
	hi = color >> 8;
	lo = color;
	setAddrWindow(x, y, x + w - 1, y);
	DISPLAY16_DC_SetHigh;
	spiStartTransaction();
	while (w--)
	{
		spiWrite(hi);
		spiWrite(lo);
	}
	spiEndTransaction();
#endif
}

/*!
	@brief draws a circle where (x0,y0) are center coordinates an r is circle radius.
	@param centerX circle center x position
	@param centerY circle center y position
	@param radius radius of circle
	@param color The color of the circle , 565 16 Bit color
*/
void display16_graphics_LTSM ::drawCircle(int16_t centerX, int16_t centerY, int16_t radius, uint16_t color)
{
	// Initial decision parameter for the circle drawing algorithm
	int16_t decisionParam = 1 - radius;
	// Differences for circle drawing in the x and y directions
	int16_t deltaX = 1;
	int16_t deltaY = -2 * radius;
	// Starting coordinates
	int16_t x = 0;
	int16_t y = radius;
	// Draw the initial points on the circle (4 points)
	drawPixel(centerX, centerY + radius, color);
	drawPixel(centerX, centerY - radius, color);
	drawPixel(centerX + radius, centerY, color);
	drawPixel(centerX - radius, centerY, color);
	// Apply the circle drawing algorithm to plot points around the circle
	while (x < y)
	{
		// If the decision parameter is positive or zero, adjust y and deltaY
		if (decisionParam >= 0)
		{
			y--;
			deltaY += 2;
			decisionParam += deltaY;
		}
		// Always adjust x and deltaX
		x++;
		deltaX += 2;
		decisionParam += deltaX;
		// Draw the 8 symmetrical points of the circle for each iteration
		drawPixel(centerX + x, centerY + y, color);
		drawPixel(centerX - x, centerY + y, color);
		drawPixel(centerX + x, centerY - y, color);
		drawPixel(centerX - x, centerY - y, color);
		drawPixel(centerX + y, centerY + x, color);
		drawPixel(centerX - y, centerY + x, color);
		drawPixel(centerX + y, centerY - x, color);
		drawPixel(centerX - y, centerY - x, color);
	}
}

///@cond

/*!
	@brief Internal helper function used by drawRoundRect to draw parts of a circle.
	@param centerX The x-coordinate of the circle's center.
	@param centerY The y-coordinate of the circle's center.
	@param radius The radius of the circle.
	@param cornerFlags A bitmask indicating which corners of the circle to draw.
	@param color The color of the circle.
*/
void display16_graphics_LTSM::drawCircleHelper(int16_t centerX, int16_t centerY,
										   int16_t radius, uint8_t cornerFlags, uint16_t color)
{
	// Initial decision parameter for the circle drawing algorithm
	int16_t decisionParam = 1 - radius;
	// Differences for circle drawing in the x and y directions
	int16_t deltaX = 1;
	int16_t deltaY = -2 * radius;
	// Starting coordinates
	int16_t x = 0;
	int16_t y = radius;
	// Apply the circle drawing algorithm to plot points in the specified corners
	while (x < y)
	{
		// If the decision parameter is positive or zero, adjust y and deltaY
		if (decisionParam >= 0)
		{
			y--;
			deltaY += 2;
			decisionParam += deltaY;
		}
		// Always adjust x and deltaX
		x++;
		deltaX += 2;
		decisionParam += deltaX;
		// Draw the points for each corner based on the cornerFlags
		if (cornerFlags & 0x4) // Top-right corner
		{
			drawPixel(centerX + x, centerY + y, color);
			drawPixel(centerX + y, centerY + x, color);
		}
		if (cornerFlags & 0x2) // Bottom-right corner
		{
			drawPixel(centerX + x, centerY - y, color);
			drawPixel(centerX + y, centerY - x, color);
		}
		if (cornerFlags & 0x8) // Top-left corner
		{
			drawPixel(centerX - y, centerY + x, color);
			drawPixel(centerX - x, centerY + y, color);
		}
		if (cornerFlags & 0x1) // Bottom-left corner
		{
			drawPixel(centerX - y, centerY - x, color);
			drawPixel(centerX - x, centerY - y, color);
		}
	}
}

/*!
	@brief Internal helper function used by fillCircle and fillRoundRect to fill parts of a circle.
	@param centerX The x-coordinate of the circle's center.
	@param centerY The y-coordinate of the circle's center.
	@param radius The radius of the circle.
	@param cornerFlags A bitmask indicating which parts of the circle to fill.
	@param verticalOffset An additional vertical offset to adjust the line length.
	@param color The color to fill the circle with.
*/
void display16_graphics_LTSM::fillCircleHelper(int16_t centerX, int16_t centerY, int16_t radius,
											  uint8_t cornerFlags, int16_t verticalOffset, uint16_t color)
{
	// Initial decision parameter for the circle filling algorithm
	int16_t decisionParam = 1 - radius;
	// Differences for circle drawing in the x and y directions
	int16_t deltaX = 1;
	int16_t deltaY = -2 * radius;
	// Starting coordinates
	int16_t x = 0;
	int16_t y = radius;
	// Apply the circle filling algorithm to plot vertical lines at the specified points
	while (x < y)
	{
		// If the decision parameter is positive or zero, adjust y and deltaY
		if (decisionParam >= 0)
		{
			y--;
			deltaY += 2;
			decisionParam += deltaY;
		}
		// Always adjust x and deltaX
		x++;
		deltaX += 2;
		decisionParam += deltaX;
		// Draw the vertical lines for each part of the circle based on the cornerFlags
		if (cornerFlags & 0x1) // Bottom-right corner
		{
			drawFastVLine(centerX + x, centerY - y, 2 * y + 1 + verticalOffset, color);
			drawFastVLine(centerX + y, centerY - x, 2 * x + 1 + verticalOffset, color);
		}
		if (cornerFlags & 0x2) // Bottom-left corner
		{
			drawFastVLine(centerX - x, centerY - y, 2 * y + 1 + verticalOffset, color);
			drawFastVLine(centerX - y, centerY - x, 2 * x + 1 + verticalOffset, color);
		}
	}
}

///@endcond

/*!
	@brief fills a circle where (x0,y0) are center coordinates an r is circle radius.
	@param x0 circle center x position
	@param y0 circle center y position
	@param r radius of circle
	@param color color of the circle , 565 16 Bit color
*/
void display16_graphics_LTSM::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
}

/*!
	@brief draws rectangle at (x,y) where h is height and w is width of the rectangle.
	@param x x start coordinate
	@param y y start coordinate
	@param w width of the rectangle
	@param h height of the rectangle
	@param color color to fill  rectangle 565 16-bit
*/
void display16_graphics_LTSM ::drawRectWH(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	drawFastHLine(x, y, w, color);
	drawFastHLine(x, y + h - 1, w, color);
	drawFastVLine(x, y, h, color);
	drawFastVLine(x + w - 1, y, h, color);
}

/*!
	@brief draws a line from (x0,y0) to (x1,y1).
	@param x0 x start coordinate
	@param y0 y start coordinate
	@param x1 x end coordinate
	@param y1 y end coordinate
	@param color color to fill  rectangle 565 16-bit
*/
void display16_graphics_LTSM ::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	int16_t steep, dx, dy, err, ystep;
	steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		swapint16t(x0, y0);
		swapint16t(x1, y1);
	}
	if (x0 > x1)
	{
		swapint16t(x0, x1);
		swapint16t(y0, y1);
	}
	dx = x1 - x0;
	dy = abs(y1 - y0);

	err = dx / 2;
	if (y0 < y1)
	{
		ystep = 1;
	}else{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			drawPixel(y0, x0, color);
		}else{
			drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

/*!
	@brief fills a rectangle at (x,y) where h is height and w is width of the rectangle.
	@param x x start coordinate
	@param y y start coordinate
	@param w width of the rectangle
	@param h height of the rectangle
	@param color color to fill  rectangle 565 16-bit
*/
void display16_graphics_LTSM ::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint16_t i;
	for (i = x; i < x + w; i++)
	{
		drawFastVLine(i, y, h, color);
	}
}

/*!
	@brief draws a rectangle with rounded edges
	@param x x start coordinate
	@param y y start coordinate
	@param w width of the rectangle
	@param h height of the rectangle
	@param r r: radius of the rounded edges
	@param color color to fill  rectangle 565 16-bit
*/
void display16_graphics_LTSM ::drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
	drawFastHLine(x + r, y, w - 2 * r, color);
	drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
	drawFastVLine(x, y + r, h - 2 * r, color);
	drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
	drawCircleHelper(x + r, y + r, r, 1, color);
	drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

/*!
	@brief Fills a rectangle with rounded edges
	@param x x start coordinate
	@param y y start coordinate
	@param w width of the rectangle
	@param h height of the rectangle
	@param r r: radius of the rounded edges
	@param color color to fill  rectangle 565 16-bit
*/
void display16_graphics_LTSM ::fillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
	fillRect(x + r, y, w - 2 * r, h, color);
	fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

/*!
	@brief draws a triangle of coordinates (x0,y0), (x1,y1) and (x2,y2).
	@param x0 x start coordinate point 1
	@param y0 y start coordinate point 1
	@param x1 x start coordinate point 2
	@param y1 y start coordinate point 2
	@param x2 x start coordinate point 3
	@param y2 y start coordinate point 3
	@param color color to draw triangle 565 16-bit
*/
void display16_graphics_LTSM ::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
	drawLine(x0, y0, x1, y1, color);
	drawLine(x1, y1, x2, y2, color);
	drawLine(x2, y2, x0, y0, color);
}

/*!
	@brief Fills a triangle of coordinates (x0,y0), (x1,y1) and (x2,y2).
	@param x0 x start coordinate point 1
	@param y0 y start coordinate point 1
	@param x1 x start coordinate point 2
	@param y1 y start coordinate point 2
	@param x2 x start coordinate point 3
	@param y2 y start coordinate point 3
	@param color color to fill , 565 16-bit
*/
void display16_graphics_LTSM ::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
	// Temporary variables for line drawing
	int16_t leftX, rightX, y, lastY;
	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1)
	{
		swapint16t(y0, y1);
		swapint16t(x0, x1);
	}
	if (y1 > y2)
	{
		swapint16t(y2, y1);
		swapint16t(x2, x1);
	}
	if (y0 > y1)
	{
		swapint16t(y0, y1);
		swapint16t(x0, x1);
	}
	// If the triangle is flat (top and bottom vertices are the same y-coordinate)
	if (y0 == y2)
	{
		leftX = rightX = x0;
		if (x1 < leftX)
			leftX = x1;
		else if (x1 > rightX)
			rightX = x1;
		if (x2 < leftX)
			leftX = x2;
		else if (x2 > rightX)
			rightX = x2;
		drawFastHLine(leftX, y0, rightX - leftX + 1, color);
		return;
	}
	int16_t dx01 = x1 - x0,
			dy01 = y1 - y0,
			dx02 = x2 - x0,
			dy02 = y2 - y0,
			dx12 = x2 - x1,
			dy12 = y2 - y1;
	// Accumulated error terms for drawing the triangle
	int32_t sa = 0, sb = 0;
	if (y1 == y2)
		lastY = y1;
	else
		lastY = y1 - 1;
	for (y = y0; y <= lastY; y++)
	{
		leftX = x0 + sa / dy01;
		rightX = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		if (leftX > rightX)
			swapint16t(leftX, rightX);
		drawFastHLine(leftX, y, rightX - leftX + 1, color);
	}
	// Reset error terms for the lower part of the triangle
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	// Draw the lower part of the triangle
	for (; y <= y2; y++)
	{
		leftX = x1 + sa / dy12;
		rightX = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		if (leftX > rightX)
			swapint16t(leftX, rightX);
		drawFastHLine(leftX, y, rightX - leftX + 1, color);
	}
}

/*!
	@brief turn on or off screen wrap of the text
	@param w TRUE wrap on, false warp off 
*/
void display16_graphics_LTSM ::setTextWrap(bool w) { _textwrap = w; }

/*!
	@brief Write 1 character on Display
	@param  x character starting position on x-axis.
	@param  y character starting position on y-axis.
	@param  value Character to be written.
	@note uses spiWriteDataBuffer method to write each character as a row by row buffer for speed.
			Much faster than pixel by pixel spi byte writes,
			if _textCharPixelOrBuffer = false. 
	@return Will return DisLib16::Ret_Codes_e enum
		-# DisLib16::Success  success
		-# DisLib16::CharScreenBounds co-ords out of bounds check x and y
		-# DisLib16::CharFontASCIIRange Character out of ASCII Font bounds, check Font range
 */
DisLib16::Ret_Codes_e display16_graphics_LTSM::writeChar(uint16_t x, uint16_t y, char value)
{

	// 1. Check for screen out of  bounds
	if ((x >= _width) || (y >= _height))
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error 1: writeChar : Co-ordinates out of bounds");
		#endif
		return DisLib16::CharScreenBounds;
	}
	// 2. Check for character out of font range bounds
	if (value < _FontOffset || value >= (_FontOffset + _FontNumChars + 1))
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.print("Error 2: writeChar : Character out of Font bounds: ");
			Serial.println(value);
		#endif
		return DisLib16::CharFontASCIIRange;
	}

	uint16_t ltextcolor = 0;
	uint16_t ltextbgcolor = 0;
	if (getInvertFont() == true)
	{
		ltextbgcolor = _textcolor;
		ltextcolor = _textbgcolor;
	}
	else
	{
		ltextbgcolor = _textbgcolor;
		ltextcolor = _textcolor;
	}
	// Locate font bitmap
	uint16_t fontIndex = ((value - _FontOffset) * ((_Font_X_Size * _Font_Y_Size) / 8)) + 4;
	if (_textCharPixelOrBuffer) // Pixel-by-pixel drawing mode 
	{
		for (int16_t cy = 0; cy < _Font_Y_Size; cy++)
		{ // Process row first
			for (int16_t cx = 0; cx < _Font_X_Size; cx++)
			{ // Then process each column in the row
				int byteIndex = fontIndex + (cy * (_Font_X_Size / 8)) + (cx / 8);
				int bitIndex = 7 - (cx % 8); // Bit index within the byte (MSB first)
				if (pgm_read_byte(&_FontSelect[byteIndex]) & (1 << bitIndex))
				{
					drawPixel(x + cx, y + cy, ltextcolor);
				}else{
					drawPixel(x + cx, y + cy, ltextbgcolor);
				}
			}
		}
	}
	else // Buffered mode
	{
		uint8_t buffer[_Font_X_Size * _Font_Y_Size * 2]; // local buffer to hold char.
		int16_t colByte = pgm_read_byte(&_FontSelect[fontIndex]);
		int16_t colbit = 7;
		uint32_t bufferIndex = 0;
		for (int16_t cx = 0; cx < _Font_X_Size; cx++)
		{
			for (int16_t cy = 0; cy < _Font_Y_Size; cy++)
			{
				if ((colByte & (1 << colbit)) != 0)
				{
					buffer[bufferIndex++] = (ltextcolor >> 8) & 0xFF; // High byte
					buffer[bufferIndex++] = ltextcolor & 0xFF;		  // Low byte
				}else{
					buffer[bufferIndex++] = (ltextbgcolor >> 8) & 0xFF; // High byte
					buffer[bufferIndex++] = ltextbgcolor & 0xFF;		// Low byte
				}
				colbit--;
				if (colbit < 0)
				{
					colbit = 7;
					fontIndex++;
					colByte = pgm_read_byte(&_FontSelect[fontIndex]);
				}
			}
		}
		// Set window and write buffer
		setAddrWindow(x, y, x + _Font_X_Size - 1, y + _Font_Y_Size - 1);
		spiWriteDataBuffer(buffer, bufferIndex);
	}
	return DisLib16::Success;
}


/*!
	@brief Write Text character array on display
	@param  x character starting position on x-axis.
	@param  y character starting position on y-axis.
	@param  pText Pointer to the array of the text to be written.
	@return Will return
		-# DisLib16::Success Success
		-# DisLib16::CharArrayNullptr String pText Array invalid pointer object
		-# Failure code from  writeChar method upstream
 */
DisLib16::Ret_Codes_e display16_graphics_LTSM::writeCharString(uint16_t x, uint16_t y, char *pText)
{
	uint8_t count = 0;
	uint8_t MaxLength = 0;
	// Check for null pointer
	if (pText == nullptr)
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error: writeCharString16 1 :String array is not valid pointer");
		#endif
		return DisLib16::CharArrayNullptr;
	}
	DisLib16::Ret_Codes_e DrawCharReturnCode;
	while (*pText != '\0')
	{
		// check if text has reached end of screen
		if ((x + (count * _Font_X_Size)) > _width - _Font_X_Size)
		{
			y = y + _Font_Y_Size;
			x = 0;
			count = 0;
		}
		DrawCharReturnCode = writeChar(x + (count * (_Font_X_Size)), y, *pText++);
		if (DrawCharReturnCode != DisLib16::Success)
			return DrawCharReturnCode;
		count++;
		MaxLength++;
		if (MaxLength >= 250)
			break; // 2nd way out of loop, safety check
	}
	return DisLib16::Success;
}

/*!
	@brief write method used in the print class when user calls print
	@param character the character to print
	@return Will return
		-# 1. success
		-# DisLib16::Ret_Codes_e enum error code,  An error in the writeChar method upstream
*/
size_t display16_graphics_LTSM::write(uint8_t character)
{
	DisLib16::Ret_Codes_e DrawCharReturnCode = DisLib16::Success;
	switch (character)
	{
	case '\n':
		_cursorY += _Font_Y_Size;
		_cursorX = 0;
		break;
	case '\r':
		break;
	default:
		DrawCharReturnCode = writeChar(_cursorX, _cursorY, character);
		if (DrawCharReturnCode != DisLib16::Success)
		{
			// Set the write error based on the result of the drawing operation
			setWriteError(DrawCharReturnCode); // Set error flag to non-zero value}
			break;
		}
		_cursorX += (_Font_X_Size);
		if (_textwrap && (static_cast<uint16_t>(_cursorX) > (_width - (_Font_X_Size))))
		{
			_cursorY += _Font_Y_Size;
			_cursorX = 0;
		}
		break;
	} // end of switch
	return 1;
}


/*!
	@brief: Draws an bi-color bitmap to screen
	@param x X coordinate
	@param y Y coordinate
	@param w width of the bitmap in pixels
	@param h height of the bitmap in pixels
	@param color bitmap foreground colors ,is bi-color
	@param bgcolor bitmap background colors ,is bi-color
	@param bitmap  an array of uint8_t containing bitmap data horizontally addressed.
	@return
		-# Display_Success=success
		-# Display_BitmapNullptr=invalid pointer object
		-# Display_BitmapScreenBounds=Co-ordinates out of bounds,
		-# Display_BitmapHorizontalSize=bitmap wrong size
	@note A horizontal Bitmap's w must be divisible by 8. For a bitmap with w=88 & h=48.
		  Bitmap excepted size = (88/8) * 48 = 528 bytes.
		  	If dislib16_ADVANCED_SCREEN_BUFFER_ENABLE is defined then the function 
			will write to screen Buffer instead of VRAM.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::drawBitmap(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t color,uint16_t bgcolor,
	const uint8_t* bitmap) {
	int16_t byteWidth = (w + 7) / 8;
	uint8_t byte = 0;
	// Check for null bitmap
	if (bitmap == nullptr) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error: drawBitmap: Bitmap pointer is null");
		#endif
		return DisLib16::BitmapDataEmpty;
	}
	// Check horizontal size
	if (w % 8 != 0) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.print("Error: drawBitmap: Horizontal Bitmap size incorrect: Width must be divisible by 8: ");
			Serial.println(w);
		#endif
		return DisLib16::BitmapHorizontalSize;
	}
	// Check bounds
	if ((static_cast<uint16_t>(x) >= _width) || (static_cast<uint16_t>(y) >= _height)) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error: drawBitmap: Out of screen bounds, check x & y");
		#endif
		return DisLib16::BitmapScreenBounds;
	}
	// Clip width/height if needed
	if (static_cast<uint16_t>(x + w - 1) >= _width)
		w = _width - x;
	if (static_cast<uint16_t>(y + h - 1) >= _height)
		h = _height - y;

#ifndef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
	uint16_t mycolor = 0;
	// Buffer for one row of pixels (16-bit per pixel split into bytes)
	uint8_t rowBuffer[w * 2];
	// Draw row by row
	for (int16_t j = 0; j < h; j++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				byte <<= 1;
			else
				byte = pgm_read_byte(bitmap + (j * byteWidth + i / 8));
			mycolor = (byte & 0x80) ? color : bgcolor;
			rowBuffer[2 * i]     = mycolor >> 8;
			rowBuffer[2 * i + 1] = mycolor & 0xFF;
		}
		setAddrWindow(x, y + j, x + w - 1, y + j);
		spiWriteDataBuffer(rowBuffer, w * 2);
	}
#else
	for (int16_t j = 0; j < h; j++, y++) {
		for (int16_t i = 0; i < w; i++) {
			if (i & 7)
				byte <<= 1;
			else
				byte = pgm_read_byte(bitmap + (j * byteWidth + i / 8));
			drawPixel(x + i, y, (byte & 0x80) ? color : bgcolor);
		}
	}
#endif
	return DisLib16::Success;
}


/*!
	@brief Draws an 8-bit color bitmap (RRRGGGBB format) to the screen.
		This function reads an 8-bit bitmap stored in RRRGGGBB format, converts each
		pixel to 16-bit RGB565, and writes it to the display.
	@param x X coordinate of the top-left corner of the bitmap.
	@param y Y coordinate of the top-left corner of the bitmap.
	@param bitmap span to the 8-bit bitmap data array.
	@param w Width of the bitmap in pixels.
	@param h Height of the bitmap in pixels.
	@return Display status code:
			-# DisLib16::Success on success.
			-# DisLib16::BitmapDataEmpty if bitmap is empty.
			-# DisLib16::BitmapScreenBounds if the coordinates are out of screen bounds.
	@note 	If dislib16_ADVANCED_SCREEN_BUFFER_ENABLE is defined then the function 
			will write to screen Buffer instead of VRAM.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::drawBitmap8Data(uint16_t x, uint16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h)
{
	if (bitmap == nullptr) // 1. Check for empty bitmap
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error drawBitmap8 1: Bitmap array is empty");
		#endif
		return DisLib16::BitmapDataEmpty;
	}
	if ((x >= _width) || (y >= _height)) // 2. Check bounds
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error drawBitmap8 2: Out of screen bounds");
		#endif
		return DisLib16::BitmapScreenBounds;
	}

	if ((x + w - 1) >= _width)
		w = _width - x;
	if ((y + h - 1) >= _height)
		h = _height - y;

#ifndef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
	uint8_t rowBuffer[w * 2]; // Allocate space for 16-bit per pixel row buffer
	uint16_t j = 0;
	uint16_t color = 0;
	const uint8_t* bitmapIter = bitmap;

	// Process bitmap data row-by-row
	for (j = 0; j < h; j++)
	{
		// Convert 8-bit colors to 16-bit RGB565
		for (uint16_t i = 0; i < w; i++)
		{
			uint8_t pixelVal = pgm_read_byte(bitmapIter);
			color = convert8bitTo16bit(pixelVal);
			rowBuffer[2 * i] = color >> 8;
			rowBuffer[2 * i + 1] = color & 0xFF;
			++bitmapIter;
		}
		setAddrWindow(x, y + j, x + w - 1, y + j);
		spiWriteDataBuffer(rowBuffer, w * 2);
	}
#else
	uint16_t color = 0;
	const uint8_t* bitmapIter = bitmap;

	for (uint16_t j = 0; j < h; j++)
	{
		for (uint16_t i = 0; i < w; i++)
		{
			uint8_t pixelVal = pgm_read_byte(bitmapIter);
			color = convert8bitTo16bit(pixelVal);
			drawPixel(x + i, y + j, color);
			++bitmapIter;
		}
	}
#endif

	return DisLib16::Success;
}


/*!
	@brief: Draws a 16-bit color bitmap to the screen from a data array
	@param x X coordinate
	@param y Y coordinate
	@param bitmap span to data array
	@param w width of the bitmap in pixels
	@param h height of the bitmap in pixels
	@return Display status code:
			-# DisLib16::Success on success.
			-# DisLib16::BitmapDataEmpty if bitmap is empty.
			-# DisLib16::BitmapScreenBounds if the coordinates are out of screen bounds.
	@note 	If dislib16_ADVANCED_SCREEN_BUFFER_ENABLE is defined then the function 
			will write to screen Buffer instead of VRAM.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::drawBitmap16Data(
	uint16_t x, uint16_t y,
	const uint8_t *bitmap,
	uint16_t w, uint16_t h
) {
	if (bitmap == nullptr) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error drawBitmap16 1: Bitmap array is null");
		#endif
		return DisLib16::BitmapDataEmpty;
	}

	if ((x >= _width) || (y >= _height)) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error drawBitmap16 2: Out of screen bounds");
		#endif
		return DisLib16::BitmapScreenBounds;
	}

	if ((x + w - 1) >= _width)
		w = _width - x;
	if ((y + h - 1) >= _height)
		h = _height - y;

#ifndef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
	const uint8_t *bitmapIter = bitmap;
	for (uint16_t j = 0; j < h; j++) {
		setAddrWindow(x, y + j, x + w - 1, y + j);
		// Prepare a local buffer for one row
		uint16_t rowBuffer[w];
		for (uint16_t i = 0; i < w; i++) {
			rowBuffer[i] = pgm_read_word(bitmapIter);
			bitmapIter += 2;
		}
		spiWriteDataBuffer(reinterpret_cast<uint8_t *>(rowBuffer), w * sizeof(uint16_t));
	}
#else
	const uint8_t* bitmapIter = bitmap;
	uint16_t colour;
	for (uint16_t j = 0; j < h; j++) {
		for (uint16_t i = 0; i < w; i++) {
			// Read two bytes (MSB first) from the bitmap (PROGMEM-safe)
			uint8_t hi = pgm_read_byte(bitmapIter);       // high byte
			uint8_t lo = pgm_read_byte(bitmapIter + 1);   // low byte
			colour = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
			bitmapIter += 2;
			//TFTdrawPixel(x+i, y + h -1 -j, color);
			//drawPixel(x + i - 1, y + j - 1, color);
			drawPixel(x + i - 1, y + j - 1, colour);
		}
	}
#endif

	return DisLib16::Success;
}


/*!
	@brief: Draws an 16 bit color sprite bitmap to screen from a data array with transparent background
	@param x X coordinate
	@param y Y coordinate
	@param bitmap pointer to data array
	@param w width of the sprite in pixels
	@param h height of the sprite in pixels
	@param backgroundColor the background color of sprite (16 bit 565) this will be made transparent
	@param printBg  if true print the background color, if false sprite mode.
	@note  Does not use buffer or malloc, just draw pixel
	@return Display status code:
			-# DisLib16::Success on success.
			-# DisLib16::BitmapDataEmpty if bitmap is empty.
			-# DisLib16::BitmapScreenBounds if the coordinates are out of screen bounds.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::drawSpriteData(uint16_t x,uint16_t y,const uint8_t* bitmap,uint16_t w,
	uint16_t h,uint16_t backgroundColor,bool printBg) 
{
	// 1. Null check
	if (bitmap == nullptr) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error drawSprite 1: Sprite array is nullptr");
		#endif
		return DisLib16::BitmapDataEmpty;
	}
	// 2. Screen bounds check
	if ((x >= _width) || (y >= _height)) {
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error drawSprite 2: Sprite out of screen bounds");
		#endif
		return DisLib16::BitmapScreenBounds;
	}
	if ((x + w - 1) >= _width)
		w = _width - x;
	if ((y + h - 1) >= _height)
		h = _height - y;
	const uint8_t* bitmapIter = bitmap;
	uint16_t colour;
	for (uint16_t j = 0; j < h; j++) {
		for (uint16_t i = 0; i < w; i++) {
			// Read two bytes (MSB first) from the bitmap (PROGMEM-safe)
			uint8_t hi = pgm_read_byte(bitmapIter);       // high byte
			uint8_t lo = pgm_read_byte(bitmapIter + 1);   // low byte
			colour = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
			bitmapIter += 2;
			if (!printBg) {
				if (colour != backgroundColor) {
					drawPixel(x + i - 1, y + j - 1, colour);
				}
			} else {
				drawPixel(x + i - 1, y + j - 1, colour);
			}
		}
	}
	return DisLib16::Success;
}


/*!
	@brief : Write an SPI command to TFT
	@param command byte to send
*/
void display16_graphics_LTSM::writeCommand(uint8_t command)
{
	DISPLAY16_DC_SetLow;
	spiStartTransaction();
	spiWrite(command);
	spiEndTransaction();
}

/*!
	@brief Write an SPI data byte to device
	@param dataByte byte to send
*/
void display16_graphics_LTSM ::writeData(uint8_t dataByte)
{
	DISPLAY16_DC_SetHigh;
	spiStartTransaction();
	spiWrite(dataByte);
	spiEndTransaction();
}

/*!
	@brief  Write byte to SPI
	@param spiData byte to write
*/
void display16_graphics_LTSM::spiWrite(uint8_t spiData)
{
	if (_hardwareSPI == false)
	{
		spiWriteSoftware(spiData);
	}else{
		_spi->transfer(spiData);
	}
}

/*!
	@brief Write a byte to SPI using software SPI
	@param spiData byte to send
	@note uses _SWSPIGPIODelay to slowdown software SPI if CPU frequency too fast
*/
void display16_graphics_LTSM::spiWriteSoftware(uint8_t spiData)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		DISPLAY16_SDATA_SetLow;
		if (spiData & 0x80)
			DISPLAY16_SDATA_SetHigh;
		DISPLAY16_SCLK_SetHigh;
		if (_SWSPIGPIODelay > 0)
			MICROSEC_DELAY(_SWSPIGPIODelay);
		spiData <<= 1;
		DISPLAY16_SCLK_SetLow;
		if (_SWSPIGPIODelay > 0)
			MICROSEC_DELAY(_SWSPIGPIODelay);
	}
}

/*!
	@brief  Write a buffer to SPI, both Software and hardware SPI supported
	@param spiData to send
	@param len length of buffer
*/
void display16_graphics_LTSM::spiWriteDataBuffer(uint8_t *spiData, uint32_t len)
{
#if defined(ESP8266)
	// ESP8266 needs a periodic yield() call to avoid watchdog reset.
	yield();
#endif
	DISPLAY16_DC_SetHigh;
	spiStartTransaction();
	if (_hardwareSPI == false)
	{
		for (uint32_t i = 0; i < len; i++)
		{
			spiWriteSoftware(spiData[i]);
		}
	}else{
		for (uint32_t i = 0; i < len; i++)
		{
			_spi->transfer(spiData[i]);
		}
	}
	spiEndTransaction();
}

/*!
	@brief Set the Cursor Position on screen
	@param x the x co-ord of the cursor position
	@param y the y co-ord of the cursor position
*/
void display16_graphics_LTSM::setCursor(int16_t x, int16_t y)
{
	_cursorX = x;
	_cursorY = y;
}

/*!
	@brief Set text color
	@param c  text color , Color definitions 16-Bit Color Values R5G6B5
*/
void display16_graphics_LTSM::setTextColor(uint16_t c)
{
	_textcolor = _textbgcolor = c;
}

/*!
	@brief Set text color foreground and background
	@param c text foreground color , Color definitions 16-Bit Color Values R5G6B5
	@param b text background color , Color definitions 16-Bit Color Values R5G6B5
*/
void display16_graphics_LTSM::setTextColor(uint16_t c, uint16_t b)
{
	_textcolor = c;
	_textbgcolor = b;
}

/*!
	@brief convert 8 bit color to 16 bit color 565
	@param RRRGGGBB a byte of 8bit color
	@details RRRGGGBB to RRRRRGGGGGGBBBBB
	@return a uint16_t 565 color value
*/
uint16_t display16_graphics_LTSM::convert8bitTo16bit(uint8_t RRRGGGBB)
{
	uint16_t red = (RRRGGGBB >> 5) & 0x07;
	uint16_t green = (RRRGGGBB >> 2) & 0x07;
	uint16_t blue = RRRGGGBB & 0x03;
	red = (red * 255 / 7) >> 3;		// Scale 3-bit red (0-7) to 5-bit (0-31)
	green = (green * 255 / 7) >> 2; // Scale 3-bit green (0-7) to 6-bit (0-63)
	blue = (blue * 255 / 3) >> 3;	// Scale 2-bit blue (0-3) to 5-bit (0-31)
	return (red << 11) | (green << 5) | blue;
}

/*!
	@brief Set the text rendering mode to either buffered or pixel-by-pixel.
	@param mode If true, characters are drawn pixel by pixel; if false, characters are drawn using a buffer.
 */
void display16_graphics_LTSM::setTextCharPixelOrBuffer(bool mode)
{
	_textCharPixelOrBuffer = mode;
}

/*!
	@brief Get the current text rendering mode.
	@return if true characters are, drawn pixel by pixel, 
	if false, characters are drawn using a buffered write.
 */
bool display16_graphics_LTSM::getTextCharPixelOrBuffer() const
{
	return _textCharPixelOrBuffer;
}


/*!
	@brief Begin an SPI transaction for the display.
 */
void display16_graphics_LTSM::spiStartTransaction(void)
{
	//There is a pre-defined macro SPI_HAS_TRANSACTION in SPI library for checking 
	//whether the firmware of the Arduino board supports _spi->beginTransaction().
	if (_hardwareSPI)
	{
		#ifdef SPI_HAS_TRANSACTION
			_spi->beginTransaction(SPISettings(_speedSPIHz, MSBFIRST, SPI_MODE0)); 
		#else // SPI transactions likewise not present in MCU or lib
			_spi->setClockDivider(SPI_CLOCK_DIV8); // 72/8 = 9Mhz
		#endif
	}
	DISPLAY16_CS_SetLow;
}

/*!
	@brief End an SPI transaction for the display.
 */
void display16_graphics_LTSM::spiEndTransaction(void)
{
	DISPLAY16_CS_SetHigh;
	//There is a pre-defined macro SPI_HAS_TRANSACTION in SPI library for checking 
	//whether the firmware of the Arduino board supports _spi->endTransaction().
	if (_hardwareSPI)
	{
		#ifdef SPI_HAS_TRANSACTION
			_spi->endTransaction(); 
		#else // SPI transactions likewise not present in MCU or lib
			// Blank
		#endif
	}
	
}


//==================================================
// *** Advanced Graphics Functions ***
#ifdef dislib16_ADVANCED_GRAPHICS_ENABLE

/*!
	@brief Get the current maximum angle of the arc.
	@return The current maximum angle in degrees.
*/
float display16_graphics_LTSM::getArcAngleMax() const
{
	return _arcAngleMax;
}
/*!
	@brief Set a new maximum angle for the arc.
	@param arcAngleMax The new maximum angle in degrees (should be positive).
*/
void display16_graphics_LTSM::setArcAngleMax(float arcAngleMax)
{
	if (arcAngleMax > 0)
	{ // Ensure the max angle is positive
		_arcAngleMax = arcAngleMax;
	}
}
/*!
	@brief Get the current angle offset.
	@return The current angle offset in degrees.
*/
int display16_graphics_LTSM::getArcAngleOffset() const
{
	return _arcAngleOffset;
}
/*!
	@brief Set a new angle offset.
	@param arcAngleOffset The new angle offset in degrees.
*/
void display16_graphics_LTSM::setArcAngleOffset(int arcAngleOffset)
{
	_arcAngleOffset = arcAngleOffset;
}

/*!
	@brief Draws a polygon with a specified number of sides, diameter, rotation, and color.
	This function draws a regular polygon by connecting points equally spaced around a circle,
	with each point having a distance defined by the diameter. The polygon is rotated by the given
	angle (in degrees) before being drawn. The number of sides is enforced to be at least 3.
	@param x The x-coordinate of the center of the polygon.
	@param y The y-coordinate of the center of the polygon.
	@param sides The number of sides the polygon will have. Must be at least 3.
	@param diameter The diameter of the circle inscribed by the polygon.
	@param rotation The angle (in degrees) by which to rotate the polygon.
	@param fill if false draw ,if true fill
	@param color The color of the polygon edges.
	@returns error code  GenericError , if user inputs incorrect sides value
 */
DisLib16::Ret_Codes_e display16_graphics_LTSM::drawPolygon(int16_t x, int16_t y, uint8_t sides, int16_t diameter, float rotation, bool fill, uint16_t color)
{
	static constexpr uint8_t MAX_SIDES = 12;
	if ((sides < 3) || (sides > MAX_SIDES))
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error : drawPolygon : sides incorrect value 3-12");
		#endif
		return DisLib16::GenericError;
	}
	const float degreesToRadians = _myPI / 180.0;
	const float angleBetweenPoints = 360.0 / sides;
	int16_t vx[MAX_SIDES];
	int16_t vy[MAX_SIDES];
	int16_t intersectX[MAX_SIDES];	// max intersections = sides
	// Calculate polygon vertex positions
	for (uint8_t i = 0; i < sides; i++)
	{
		vx[i] = x + (sin((i * angleBetweenPoints + rotation) * degreesToRadians) * diameter);
		vy[i] = y + (cos((i * angleBetweenPoints + rotation) * degreesToRadians) * diameter);
	}
	if (!fill) // If not filling, just draw the polygon outline
	{
		for (uint8_t i = 0; i < sides; i++)
		{
			uint8_t j = (i + 1) % sides;
			drawLine(vx[i], vy[i], vx[j], vy[j], color);
		}
	}else{ // If filling, use scanline algorithm to fill the polygon
		int16_t minY = vy[0], maxY = vy[0];
		for (uint8_t i = 1; i < sides; i++)
		{
			if (vy[i] < minY) minY = vy[i];
			if (vy[i] > maxY) maxY = vy[i];
		}
		for (int16_t scanY = minY; scanY <= maxY; scanY++) // Loop through scanlines
		{
			uint8_t intersections = 0;
			for (uint8_t i = 0; i < sides; i++)
			{
				uint8_t j = (i + 1) % sides;
				if ((vy[i] <= scanY && vy[j] > scanY) || (vy[j] <= scanY && vy[i] > scanY))
				{ // Compute intersection using linear interpolation
					float t = (float)(scanY - vy[i]) / (vy[j] - vy[i]);
					intersectX[intersections++] = vx[i] + t * (vx[j] - vx[i]);
				}
			}
			// Bubble sort intersections
			for (uint8_t i = 0; i < intersections - 1; i++)
			{
				for (uint8_t j = i + 1; j < intersections; j++)
				{
					if (intersectX[i] > intersectX[j])
					{
						swapint16t(intersectX[i], intersectX[j]);
					}
				}
			}
			// Draw horizontal lines between pairs of intersections
			for (uint8_t i = 0; i < intersections; i += 2)
			{ 
				if (i + 1 < intersections)
				{
					drawFastHLine(intersectX[i], scanY, intersectX[i + 1] - intersectX[i] + 1, color);
				}
			}
		}
	}
	return DisLib16::Success;
}


/*!
	@brief Draws a grid of dots on the screen starting from the given coordinates.
		This function draws a grid of pixels with a specified gap between them.
		It checks that the provided coordinates and dimensions are within the screen bounds
		and adjusts them if necessary. It also validates The grid of dots gap value and defaults it to 2 if invalid.
	@param x The x-coordinate of the top-left corner where The grid of dots will start.
	@param y The y-coordinate of the top-left corner where The grid of dots will start.
	@param w The width of the area to draw The grid of dots, from the starting x-coordinate.
	@param h The height of the area to draw The grid of dots, from the starting y-coordinate.
	@param DotGridGap The gap between each dot gap line, controlling the spacing between drawn pixels.
	@param color The color of the pixels to draw in The grid of dots.
	@return DisLib16::Ret_Codes_e Returns a status code indicating success or failure. Possible return values:
	- DisLib16::Success: The grid of dots was successfully drawn.
	- DisLib16::ShapeScreenBounds: The provided coordinates or dimensions are out of screen bounds.
 */
DisLib16::Ret_Codes_e display16_graphics_LTSM::drawDotGrid(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t DotGridGap, uint16_t color)
{
	// User input handling
	if ((x >= _width) || (y >= _height))
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error: drawDotGrid: Out of screen bounds");
		#endif
		return DisLib16::ShapeScreenBounds;
	}
	if (DotGridGap < 2 || DotGridGap > 20)
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Warning : drawDotGrid: Invalid Dot Gap value(2-20) , setting to 2");
		#endif
		DotGridGap = 2;
	}
	if ((x + w - 1) >= _width)
		w = _width - x;
	if ((y + h - 1) >= _height)
		h = _height - y;
	// Swap coordinates if the width or height are smaller than the starting point
	int16_t dotGapWidth, dotGapHeight;
	if (w < x)
	{
		dotGapWidth = w;
		w = x;
		x = dotGapWidth;
	}
	if (h < y)
	{
		dotGapHeight = h;
		h = y;
		y = dotGapHeight;
	}

	// Draw the grid of pixels
	for (uint16_t row = y; row <= h; row += DotGridGap)
	{
		for (uint16_t col = x; col <= w; col += DotGridGap)
		{
			drawPixel(col, row, color);
		}
	}
	return DisLib16::Success;
}

/*!
	@brief Draws a line using an angle and length as parameters.
	This function draws a line starting from `(x, y)`, extending in the direction
	specified by `angle`, with a given `length`. The function also allows applying
	an `offset` to the angle before computing the line’s end coordinates.
	@param x The starting x-coordinate (horizontal position).
	@param y The starting y-coordinate (vertical position).
	@param angle The angle (in degrees) at which the line is drawn.
	@param start calculates a new starting position by moving start units along the given angle.
	@param length The length of the line.
	@param offset An additional offset applied to the angle before calculating the endpoint.
	@param color The color of the line (RGB565 format).
 */
void display16_graphics_LTSM::drawLineAngle(int16_t x, int16_t y, int angle, uint8_t start, uint8_t length, int offset, uint16_t color)
{
	if (start == 0)
	{
		if (length < 2)
		{
			drawPixel(x, y, color);
		}else{
			drawLine(x, y,
					 x + length * cosineFromDegrees(angle + offset),
					 y + length * sineFromDegrees(angle + offset), color);
		}
	}
	else
	{
		if (start - length < 2)
		{
			drawPixel(x, y, color);
		}else{
			drawLine(
				x + start * cosineFromDegrees(angle + offset),
				y + start * sineFromDegrees(angle + offset),
				x + (start + length) * cosineFromDegrees(angle + offset),
				y + (start + length) * sineFromDegrees(angle + offset),
				color);
		}
	}
}

/*!
	@brief Draws a quadrilateral (four-sided polygon) by connecting four points with lines.
	This function draws a quadrilateral by drawing four lines between the given vertices.
	The lines are drawn in the order: (x0, y0) to (x1, y1), (x1, y1) to (x2, y2),
	(x2, y2) to (x3, y3), and finally (x3, y3) back to (x0, y0).
	@param x0 The x-coordinate of the first vertex.
	@param y0 The y-coordinate of the first vertex.
	@param x1 The x-coordinate of the second vertex.
	@param y1 The y-coordinate of the second vertex.
	@param x2 The x-coordinate of the third vertex.
	@param y2 The y-coordinate of the third vertex.
	@param x3 The x-coordinate of the fourth vertex.
	@param y3 The y-coordinate of the fourth vertex.
	@param color The 565 color used to draw the lines of the quadrilateral.
 */
void display16_graphics_LTSM::drawQuadrilateral(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
	drawLine(x0, y0, x1, y1, color); // low 1
	drawLine(x1, y1, x2, y2, color); // high 1
	drawLine(x2, y2, x3, y3, color); // high 2
	drawLine(x3, y3, x0, y0, color); // low 2
}

/*!
	@brief Fills a quadrilateral with the specified color using triangles.
	This function fills a quadrilateral by dividing it into triangles and filling them individually.
	@param x0 The x-coordinate of the first vertex.
	@param y0 The y-coordinate of the first vertex.
	@param x1 The x-coordinate of the second vertex.
	@param y1 The y-coordinate of the second vertex.
	@param x2 The x-coordinate of the third vertex.
	@param y2 The y-coordinate of the third vertex.
	@param x3 The x-coordinate of the fourth vertex.
	@param y3 The y-coordinate of the fourth vertex.
	@param color The color used to fill the quadrilateral.

 */
void display16_graphics_LTSM::fillQuadrilateral(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint16_t color)
{
	// Compute the cross product of vectors (x1-x0, y1-y0) and (x2-x0, y2-y0)
	// to determine convexity
	int32_t crossProduct = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
	if (crossProduct >= 0)
	{
		// Convex case (or degenerate, treat as convex)
		fillTriangle(x0, y0, x1, y1, x2, y2, color);
		fillTriangle(x2, y2, x3, y3, x0, y0, color);
	}
	else
	{
		// Concave case: Choose the alternative diagonal
		fillTriangle(x1, y1, x2, y2, x3, y3, color);
		fillTriangle(x3, y3, x0, y0, x1, y1, color);
	}
}

/*!
	@brief Draw an ellipse on the display.
	This function uses the midpoint ellipse algorithm to efficiently draw an
	ellipse centered at (cx, cy) with the given semi-major (horizontal) and
	semi-minor (vertical) axes.
	@param cx X-coordinate of the ellipse center.
	@param cy Y-coordinate of the ellipse center.
	@param semiMajorAxis Length of the semi-major axis (horizontal radius).
	@param semiMinorAxis Length of the semi-minor axis (vertical radius).
	@param fill if false draw Ellipse , if true fill it!
	@param color 16-bit color value for the ellipse.
 */
void display16_graphics_LTSM::drawEllipse(int16_t cx, int16_t cy, int16_t semiMajorAxis, int16_t semiMinorAxis, bool fill, uint16_t color)
{
	int16_t x, y;
	// int16_t twiceMajorAxisSquared, twiceMinorAxisSquared; //stores 2 * (a*a) &  2 * (b*b)
	int32_t stopXThreshold, stopYThreshold; // ndicates when iteration stops in X & Y direction.
	int32_t deltaX, deltaY;					// Represents the X & Y direction error change.
	int32_t decisionParam;
	// Precompute squared values for efficiency
	const int32_t twiceMajorAxisSquared = 2 * (semiMajorAxis * semiMajorAxis);
	const int32_t twiceMinorAxisSquared = 2 * (semiMinorAxis * semiMinorAxis);
	// Region 1: Upper half
	x = semiMajorAxis;
	y = 0;
	deltaX = (semiMinorAxis * semiMinorAxis) * (1 - (2 * semiMajorAxis)); // Initial error term for X
	deltaY = (semiMajorAxis * semiMajorAxis);							  // Initial error term for Y
	decisionParam = 0;
	stopXThreshold = (twiceMinorAxisSquared * semiMajorAxis); // Stopping condition for region 1
	stopYThreshold = 0;
	// First region: X decreases faster than Y increases
	while (stopXThreshold >= stopYThreshold)
	{
		if (fill)
		{
			drawFastHLine(cx - x, cy + y, 2 * x + 1, color); // Fill horizontal line
			if (y != 0)
				drawFastHLine(cx - x, cy - y, 2 * x + 1, color); // Mirror bottom half
		}
		else
		{
			ellipseHelper(cx, cy, x, y, color);
		}
		y++;
		stopYThreshold += twiceMajorAxisSquared;
		decisionParam += deltaY;
		deltaY += twiceMajorAxisSquared;
		// Check for boundary error and adjust X
		if ((2 * decisionParam) + deltaX > 0)
		{
			x--;
			stopXThreshold -= twiceMinorAxisSquared;
			decisionParam += deltaX;
			deltaX += twiceMinorAxisSquared;
		}
	}
	// Region 2: Lower half
	x = 0;
	y = semiMinorAxis;
	deltaX = (semiMinorAxis * semiMinorAxis);							  // Reset X error term
	deltaY = (semiMajorAxis * semiMajorAxis) * (1 - (2 * semiMinorAxis)); // Initial error term for Y
	decisionParam = 0;
	stopXThreshold = 0;
	stopYThreshold = (twiceMajorAxisSquared * semiMinorAxis); // Stopping condition for region 2
	// Second region: Y decreases faster than X increases
	while (stopXThreshold <= stopYThreshold)
	{
		if (fill)
		{
			drawFastHLine(cx - x, cy + y, 2 * x + 1, color); // Fill horizontal line
			if (y != 0)
				drawFastHLine(cx - x, cy - y, 2 * x + 1, color); // Mirror bottom half
		}
		else
		{
			ellipseHelper(cx, cy, x, y, color);
		}
		x++;
		stopXThreshold += twiceMinorAxisSquared;
		decisionParam += deltaX;
		deltaX += twiceMinorAxisSquared;
		// Check for boundary error and adjust Y
		if (((2 * decisionParam) + deltaY) > 0)
		{
			y--;
			stopYThreshold -= twiceMajorAxisSquared;
			decisionParam += deltaY;
			deltaY += twiceMajorAxisSquared;
		}
	}
}

/// @cond

/*!
	@brief Plots four symmetric points of an ellipse.
	This function takes advantage of the symmetry of ellipses, plotting the
	four points in each quadrant to minimize calculations.
	@param cx X-coordinate of the ellipse center.
	@param cy Y-coordinate of the ellipse center.
	@param x Current X offset.
	@param y Current Y offset.
	@param color 16-bit color value.
 */
void display16_graphics_LTSM::ellipseHelper(uint16_t cx, uint16_t cy, uint16_t x, uint16_t y, uint16_t color)
{
	drawPixel(cx + x, cy + y, color);
	if (x != 0)
		drawPixel(cx - x, cy + y, color);
	if (y != 0)
		drawPixel(cx + x, cy - y, color);
	if (x != 0 && y != 0)
		drawPixel(cx - x, cy - y, color);
}

/*!
	@brief helps drawArc draw an Arc on screen
	@param centerX X-coordinate of the center of the arc
	@param centerY Y-coordinate of the center of the arc
	@param radius The radius of the arc
	@param thickness the thickness of the arc
	@param start Starting angle of arc
	@param end End angle of arc
	@param color The color of the arc.
*/
void display16_graphics_LTSM::drawArcHelper(uint16_t centerX, uint16_t centerY, uint16_t radius, uint16_t thickness, float start, float end, uint16_t color)
{
	// Define bounding box variables
	int16_t minX = 65535;
	int16_t maxX = -32767;
	int16_t minY = 32767;
	int16_t maxY = -32767;
	// Trigonometric values
	float cosStart, sinStart, cosEnd, sinEnd;
	float outerRadius, tempValue;
	float startAngle, endAngle;
	// Squared radius values for comparison
	int16_t innerRadiusSquared, outerRadiusSquared;
	// Loop variables
	int16_t x, y, xSquared, ySquared;
	int16_t y1Start, y2End, y2Start;
	// Slope calculations
	float startSlope, endSlope;
	// Boolean flags for arc filling logic
	bool y1StartFound, y2StartFound, y1EndFound, y2EndSearching;
	// Convert arc angles to degrees from the normalized input range
	startAngle = (start / _arcAngleMax) * 360;
	endAngle = (end / _arcAngleMax) * 360;
	// Normalize angles to stay within the 0-360 range
	while (startAngle < 0)
		startAngle += 360;
	while (endAngle < 0)
		endAngle += 360;
	while (startAngle > 360)
		startAngle -= 360;
	while (endAngle > 360)
		endAngle -= 360;
	// Handle cases where the arc wraps around 0 degrees
	if (startAngle > endAngle)
	{
		drawArcHelper(centerX, centerY, radius, thickness, ((startAngle / 360.0) * _arcAngleMax), _arcAngleMax, color);
		drawArcHelper(centerX, centerY, radius, thickness, 0, ((endAngle / 360.0) * _arcAngleMax), color);
	}
	else
	{ // Compute trigonometric values for start and end angles
		cosStart = cosineFromDegrees(startAngle);
		sinStart = sineFromDegrees(startAngle);
		cosEnd = cosineFromDegrees(endAngle);
		sinEnd = sineFromDegrees(endAngle);
		// Determine the bounding box of the arc
		outerRadius = radius;
		tempValue = outerRadius * cosStart;
		if (tempValue < minX)
			minX = tempValue;
		if (tempValue > maxX)
			maxX = tempValue;
		tempValue = outerRadius * sinStart;
		if (tempValue < minY)
			minY = tempValue;
		if (tempValue > maxY)
			maxY = tempValue;

		tempValue = outerRadius * cosEnd;
		if (tempValue < minX)
			minX = tempValue;
		if (tempValue > maxX)
			maxX = tempValue;
		tempValue = outerRadius * sinEnd;
		if (tempValue < minY)
			minY = tempValue;
		if (tempValue > maxY)
			maxY = tempValue;
		// Adjust bounding box for inner arc
		outerRadius = radius - thickness;
		tempValue = outerRadius * cosStart;
		if (tempValue < minX)
			minX = tempValue;
		if (tempValue > maxX)
			maxX = tempValue;
		tempValue = outerRadius * sinStart;
		if (tempValue < minY)
			minY = tempValue;
		if (tempValue > maxY)
			maxY = tempValue;

		tempValue = outerRadius * cosEnd;
		if (tempValue < minX)
			minX = tempValue;
		if (tempValue > maxX)
			maxX = tempValue;
		tempValue = outerRadius * sinEnd;
		if (tempValue < minY)
			minY = tempValue;
		if (tempValue > maxY)
			maxY = tempValue;
		// Special cases for quarter-circle boundary adjustments
		if ((startAngle < 90) && (endAngle > 90))
			maxY = radius;
		if ((startAngle < 180) && (endAngle > 180))
			minX = -radius;
		if ((startAngle < 270) && (endAngle > 270))
			minY = -radius;
		// Calculate slopes for boundary conditions
		startSlope = (float)cosStart / (float)sinStart;
		endSlope = (float)cosEnd / (float)sinEnd;
		if (endAngle == 360)
			endSlope = -1000000; // Force slope to an extreme value
		innerRadiusSquared = (radius - thickness) * (radius - thickness);
		outerRadiusSquared = radius * radius;
		// Scan through bounding box to determine which pixels to fill
		for (x = minX; x <= maxX; x++)
		{
			y1StartFound = false;
			y2StartFound = false;
			y1EndFound = false;
			y2EndSearching = false;
			y1Start = 0;
			y2End = 0;
			y2Start = 0;
			for (y = minY; y <= maxY; y++)
			{
				xSquared = x * x;
				ySquared = y * y;
				// Check if pixel is within the arc boundaries
				if (
					(xSquared + ySquared < outerRadiusSquared && xSquared + ySquared >= innerRadiusSquared) && ((y > 0 && startAngle < 180 && x <= y * startSlope) || (y < 0 && startAngle > 180 && x >= y * startSlope) || (y < 0 && startAngle <= 180) || (y == 0 && startAngle <= 180 && x < 0) || (y == 0 && startAngle == 0 && x > 0)) && ((y > 0 && endAngle < 180 && x >= y * endSlope) || (y < 0 && endAngle > 180 && x <= y * endSlope) || (y > 0 && endAngle >= 180) || (y == 0 && endAngle >= 180 && x < 0) || (y == 0 && startAngle == 0 && x > 0)))
				{ // Find start and end points for vertical line drawing
					if (!y1StartFound)
					{
						y1StartFound = true;
						y1Start = y;
					}
					else if (y1EndFound && !y2StartFound)
					{
						y2StartFound = true;
						y2Start = y;
						y += y2End - y1Start - 1;
						if (y > maxY - 1)
						{
							y = y2Start;
							y2EndSearching = true;
						}
					}
					else if (y2StartFound && !y2EndSearching)
					{
						y2EndSearching = true;
					}
				}
				else
				{
					if (y1StartFound && !y1EndFound)
					{
						y1EndFound = true;
						y2End = y - 1;
						drawFastVLine(centerX + x, centerY + y1Start, y - y1Start, color);
						if (y < 0)
						{
							y = abs(y);
						}
						else
						{
							break;
						}
					}
					else if (y2StartFound)
					{
						if (y2EndSearching)
						{
							drawFastVLine(centerX + x, centerY + y2Start, y - y2Start, color);
							y2EndSearching = false;
							break;
						}
						else
						{
							y = y2Start;
							y2EndSearching = true;
						}
					}
				}
			}
			if (y1StartFound && !y1EndFound)
			{
				y2End = maxY;
				drawFastVLine(centerX + x, centerY + y1Start, y2End - y1Start + 1, color);
			}
			else if (y2StartFound && y2EndSearching)
			{
				drawFastVLine(centerX + x, centerY + y2Start, maxY - y2Start + 1, color);
			}
		}
	}
}

/// @endcond

/*!
	@brief Draw an arc on the TFT display.
	This function draws an arc between two angles (start and end) on a circle with a given radius.
	@param cx X-coordinate of the center of the circle.
	@param cy Y-coordinate of the center of the circle.
	@param radius The radius of the circle.
	@param thickness the thickness of the arc
	@param startAngle The starting angle of the arc (in degrees).
	@param endAngle The ending angle of the arc (in degrees).
	@param color The color of the arc.
	@details variables _arcAngleMax and  _arcAngleOffset which can be set by user with setters
		to define behaviour of arc.
		_arcAngleOffset is zero default, means 0 degree is positive X axis , Arc is drawn counterclockwise
		XXX 270 XXX
		180 XXX 000
		XXX 090 XXX
		This function uses a modified midpoint circle algorithm combined with scanline filling
		to efficiently draw an arc with a specified thickness. It calculates pixel positions using
		trigonometric boundary checks and slopes for accurate rendering.
		For more information, see: https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 */
void display16_graphics_LTSM::drawArc(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t thickness, float startAngle, float endAngle, uint16_t color)
{
	if (radius == 0)
		radius = 1;
	if (thickness == 0)
		thickness = 1;
	// Check if the arc should be a full circle (0 to 360 degrees)
	if (startAngle == 0 && endAngle == _arcAngleMax)
	{
		drawArcHelper(cx, cy, radius, thickness, 0, _arcAngleMax, color);
	}
	else
	{
		// Apply the offset and adjust the start/end angles
		drawArcHelper(
			cx, cy, radius, thickness,
			startAngle + (_arcAngleOffset / (float)360) * _arcAngleMax, // Adjust start angle with the offset
			endAngle + (_arcAngleOffset / (float)360) * _arcAngleMax,	// Adjust end angle with the offset
			color);
	}
}


/*!
 * @brief Computes the cosine of an angle given in degrees.
 * This function converts the input angle from degrees to radians and then calculates
 * the cosine of that angle using the standard C++ `cos` function.
 * @param angle The angle in degrees.
 * @return The cosine value of the given angle.
 */
float display16_graphics_LTSM::cosineFromDegrees(float angle)
{
	float radians = angle / 360.0f * 2.0f * _myPI;
	return cos(radians);
}

/*!
 * @brief Computes the sine of an angle given in degrees.
 * This function converts the input angle from degrees to radians and then calculates
 * the sine of that angle using the standard C++ `sin` function.
 * @param angle The angle in degrees.
 * @return The sine value of the given angle.
 */
float display16_graphics_LTSM::sineFromDegrees(float angle)
{
	float radians = angle / 360.0f * 2.0f * _myPI;
	return sin(radians);
}
#endif

//***************** Buffer mode functions *****************//
#ifdef dislib16_ADVANCED_SCREEN_BUFFER_ENABLE
/*!
	@brief Allocates memory for the screen buffer based on display resolution.
		The buffer size is calculated as width × height × 2 bytes (RGB565 format).
		If allocation fails, it returns a memory allocation error.
		In debug mode, it prints the allocated buffer size.
	@return DisLib16::Success on successful allocation,
			DisLib16::MemoryAError if allocation fails.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::setBuffer(void)
{
	// Allocate memory for the buffer
	_screenBuffer.resize(_width * _height * 2);
	if (_screenBuffer.empty() || (_screenBuffer.size() != static_cast<size_t>(_width) * _height * 2))
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error: setBuffer: Memory allocation failed");
		#endif
		return DisLib16::MemoryAError;
	}
	#ifdef dislib16_DEBUG_MODE_ENABLE
		Serial.print("Buffer size set bytes: ");
		Serial.println(_screenBuffer.size());
	#endif
	return DisLib16::Success;
}

/*!
	@brief Clears the screen buffer by filling it with a given color.
		The buffer is filled in RGB565 format. This function assumes the buffer
		has already been allocated via setBuffer().
	@param color The 16-bit RGB565 color to fill the buffer with.
	@return DisLib16::Success on completion.
			DisLib16::BufferEmpty if the buffer is empty.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::clearBuffer(uint16_t color)
{
	if (_screenBuffer.empty())
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE	
			Serial.println("Error: clearBuffer: Buffer is empty");
		#endif
		return DisLib16::BufferEmpty;
	}
	uint8_t high = color >> 8;
	uint8_t low  = color & 0xFF;
	for (size_t i = 0; i < _screenBuffer.size(); i += 2) {
		_screenBuffer[i] = high;
		_screenBuffer[i + 1] = low;
	}
	return DisLib16::Success;
}

/*!
	@brief Writes the contents of the screen buffer to the display.
		This function assumes the buffer has already been allocated and filled.
		It sets the address window for the entire screen and writes the buffer data.
	@param bufferMode If 0(default), writes the entire buffer at once; otherwise, writes row by row.
			Row-by-row writing is used by displays like SSD1331 that require it.
	@return DisLib16::Success on completion.
			DisLib16::BufferEmpty if the buffer is empty.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::writeBuffer(uint8_t bufferMode )
{
	if (_screenBuffer.empty())
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Error: writeBuffer: Buffer is empty");
		#endif
		return DisLib16::BufferEmpty;
	}
	if (bufferMode == 0){
		//  write the entire buffer at once, default mode
		setAddrWindow(0, 0, _width-1 , _height);
		spiWriteDataBuffer(const_cast<uint8_t *>(_screenBuffer.data()),_screenBuffer.size());
	} else {
		// Write the buffer row by row
		for (uint16_t row = 0; row < _height; ++row)
		{
			setAddrWindow(0, row, _width - 1, row);
			const uint8_t* rowPtr = _screenBuffer.data() + row * _width * 2;
			spiWriteDataBuffer(const_cast<uint8_t*>(rowPtr), _width * 2);
		}
	}
	return DisLib16::Success;
}

/*!
	@brief Destroys the screen buffer by resizing it to zero.
		This function checks if the buffer has been 
		successfully destroyed and prints debug information.
	@return DisLib16::Success on successful destruction,
			DisLib16::MemoryAError if destruction fails.
*/
DisLib16::Ret_Codes_e display16_graphics_LTSM::destroyBuffer(void)
{
	_screenBuffer.resize(0);
	if (_screenBuffer.size() == 0)
	{
		#ifdef dislib16_DEBUG_MODE_ENABLE
			Serial.println("Buffer has been successfully destroyed");
		#endif
		return DisLib16::Success;
	}else {
			#ifdef dislib16_DEBUG_MODE_ENABLE
				Serial.println("Error: destroyBuffer : Buffer destruction failed.");
			#endif
		return DisLib16::MemoryAError;
	}
	return DisLib16::Success;
}
#endif
//**************** EOF *****************
