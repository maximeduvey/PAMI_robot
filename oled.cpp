/* OLED Test

    NOTE : I don't know the speed of the I2C bus ! Seems slow.

    Uses the following libs:
    * https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI (C++ lib, rather complex to turn into C)
    * https://www.airspayce.com/mikem/bcm2835/ (dependency of the above)
    Other libs:
    * https://www.stealthylabs.com/blog/2020/02/19/oled-ssd1306-library.html (this one is C, but it's garbage)
    So the first option is the only one that is viable.

    All those libs are trash, I'll need to roll-out mine.

    Based on the datasheet, I should be able to port my existing library for the SPI version to I2C.
    Note that my code is for the SSD1309, but for I2C we're dealing with the SSD1306.

    The I2C interface uses a special byte before address and data called the "control byte", with two bits:
    * D/C which corresponds to the data/command signal in SPI mode
    * Co which is "continuation"
    Co exists mainly so that the control byte doesn't need to be sent for each byte of useful payload.
    My understanding is that the control byte is necessary for every command byte being sent.
    Then, when switching to data, set Co to zero and "burst" the pixel data bytes.

    Note : the init sequence for this display is different. It was helpful to read:
    https://github.com/greiman/SSD1306Ascii/blob/master/src/SSD1306init.h

    Ultimately, I managed to get the OLED working with very little fuss.

    I wonder if I need to add the following for the PAMI :
    * Large font (16-pixel high)
    * Blinking support, maybe ?
*/

// Includes =============================================================

#include "main.hpp"			// Project header
#include "oled.hpp"
#include <bcm2835.h>



// Structures ===========================================================

// Constants ============================================================

// Constant : initialization parameters to be sent to the OLED only once at the start of operation
// Note : some are default value according to the datasheet but must be sent anyway or the display won't work
static const uint8_t oled_param[] = {
        // ************** TO DO - add I2C-specific command bytes *************************
        0x00,       // Co == 0, D/C == 0 => 0000_0000
		0xAE,		// Display off during initialization
		0xD5,		// Display clock divider :
		0x80,		// the suggested ratio 0x80
		0xA8,		// MUX mode
		// 0x3F,       // value is 63, corresponding to 64, and it's the number of pixel rows of the display
        0x1F,       // Change to 31 for a 32-row display
		0xD3,		// Display offset
		0x00,
		0x40,		// Start line
		0x8D,		// Charge pump settings (doesn't appear to be in the datasheet)
		0x14,
		0x20,		// Memory mode :
		0x00,		// Horizontal mode
		0x21,		// Column indices
		0x00,			// First column
		0x7F,		// Last column (127)
		0xC0,		// Vertical orientation : C0 or C8 to flip the screen vertically. Combine with A0/A1 respectively.
		0xA0,		// Horizontal orientation : A0 or A1 to flip the screen left-right.
		0xDA,		// Directions remapping
		0x02, // 0x12,  // For some reason this is necessary on SSD1306 with 128x32 panel vs. SSD1309 with 128x64
		0x81,		// Set brightness (0-255). Note : this works, but the effect is almost imperceptible.
		0x7F,		// 0x7F is the default value
		0xD9,		// Precharge settings : leave to default
	    0xF1,
		0xDB,		// "Set Vcomm Deselect Level" (don't touch that) :
		0x40,		// appears to correspond to a value close to VCC ??? (undocumented)
		0xA4,		// Resume internal RAM to OLED operation
		0xA6,		// Normal or video-inverse display (A6 or A7)
		0xAF		// Turn on OLED panel
};

// Standard ASCII 5x7 font. Stored in Flash to save RAM
// Defines pixel maps for ASCII characters 0x20 to 0x7F (32-127)
static const uint8_t Font5x7[] = {
	0x00, 0x00, 0x00, 0x00, 0x00,// (space)
	0x00, 0x00, 0x5F, 0x00, 0x00,// !
	0x00, 0x07, 0x00, 0x07, 0x00,// "
	0x14, 0x7F, 0x14, 0x7F, 0x14,// #
	0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
	0x23, 0x13, 0x08, 0x64, 0x62,// %
	0x36, 0x49, 0x55, 0x22, 0x50,// &
	0x00, 0x05, 0x03, 0x00, 0x00,// '
	0x00, 0x1C, 0x22, 0x41, 0x00,// (
	0x00, 0x41, 0x22, 0x1C, 0x00,// )
	0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
	0x08, 0x08, 0x3E, 0x08, 0x08,// +
	0x00, 0x50, 0x30, 0x00, 0x00,// ,
	0x08, 0x08, 0x08, 0x08, 0x08,// -
	0x00, 0x60, 0x60, 0x00, 0x00,// .
	0x20, 0x10, 0x08, 0x04, 0x02,// /
	0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
	0x00, 0x42, 0x7F, 0x40, 0x00,// 1
	0x42, 0x61, 0x51, 0x49, 0x46,// 2
	0x21, 0x41, 0x45, 0x4B, 0x31,// 3
	0x18, 0x14, 0x12, 0x7F, 0x10,// 4
	0x27, 0x45, 0x45, 0x45, 0x39,// 5
	0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
	0x01, 0x71, 0x09, 0x05, 0x03,// 7
	0x36, 0x49, 0x49, 0x49, 0x36,// 8
	0x06, 0x49, 0x49, 0x29, 0x1E,// 9
	0x00, 0x36, 0x36, 0x00, 0x00,// :
	0x00, 0x56, 0x36, 0x00, 0x00,// ;
	0x00, 0x08, 0x14, 0x22, 0x41,// <
	0x14, 0x14, 0x14, 0x14, 0x14,// =
	0x41, 0x22, 0x14, 0x08, 0x00,// >
	0x02, 0x01, 0x51, 0x09, 0x06,// ?
	0x32, 0x49, 0x79, 0x41, 0x3E,// @
	0x7E, 0x11, 0x11, 0x11, 0x7E,// A
	0x7F, 0x49, 0x49, 0x49, 0x36,// B
	0x3E, 0x41, 0x41, 0x41, 0x22,// C
	0x7F, 0x41, 0x41, 0x22, 0x1C,// D
	0x7F, 0x49, 0x49, 0x49, 0x41,// E
	0x7F, 0x09, 0x09, 0x01, 0x01,// F
	0x3E, 0x41, 0x41, 0x51, 0x32,// G
	0x7F, 0x08, 0x08, 0x08, 0x7F,// H
	0x00, 0x41, 0x7F, 0x41, 0x00,// I
	0x20, 0x40, 0x41, 0x3F, 0x01,// J
	0x7F, 0x08, 0x14, 0x22, 0x41,// K
	0x7F, 0x40, 0x40, 0x40, 0x40,// L
	0x7F, 0x02, 0x04, 0x02, 0x7F,// M
	0x7F, 0x04, 0x08, 0x10, 0x7F,// N
	0x3E, 0x41, 0x41, 0x41, 0x3E,// O
	0x7F, 0x09, 0x09, 0x09, 0x06,// P
	0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
	0x7F, 0x09, 0x19, 0x29, 0x46,// R
	0x46, 0x49, 0x49, 0x49, 0x31,// S
	0x01, 0x01, 0x7F, 0x01, 0x01,// T
	0x3F, 0x40, 0x40, 0x40, 0x3F,// U
	0x1F, 0x20, 0x40, 0x20, 0x1F,// V
	0x7F, 0x20, 0x18, 0x20, 0x7F,// W
	0x63, 0x14, 0x08, 0x14, 0x63,// X
	0x03, 0x04, 0x78, 0x04, 0x03,// Y
	0x61, 0x51, 0x49, 0x45, 0x43,// Z
	0x00, 0x00, 0x7F, 0x41, 0x41,// [
	0x02, 0x04, 0x08, 0x10, 0x20,// "\"
	0x41, 0x41, 0x7F, 0x00, 0x00,// ]
	0x04, 0x02, 0x01, 0x02, 0x04,// ^
	0x40, 0x40, 0x40, 0x40, 0x40,// _
	0x00, 0x01, 0x02, 0x04, 0x00,// `
	0x20, 0x54, 0x54, 0x54, 0x78,// a
	0x7F, 0x48, 0x44, 0x44, 0x38,// b
	0x38, 0x44, 0x44, 0x44, 0x20,// c
	0x38, 0x44, 0x44, 0x48, 0x7F,// d
	0x38, 0x54, 0x54, 0x54, 0x18,// e
	0x08, 0x7E, 0x09, 0x01, 0x02,// f
	0x08, 0x14, 0x54, 0x54, 0x3C,// g
	0x7F, 0x08, 0x04, 0x04, 0x78,// h
	0x00, 0x44, 0x7D, 0x40, 0x00,// i
	0x20, 0x40, 0x44, 0x3D, 0x00,// j
	0x00, 0x7F, 0x10, 0x28, 0x44,// k
	0x00, 0x41, 0x7F, 0x40, 0x00,// l
	0x7C, 0x04, 0x18, 0x04, 0x78,// m
	0x7C, 0x08, 0x04, 0x04, 0x78,// n
	0x38, 0x44, 0x44, 0x44, 0x38,// o
	0x7C, 0x14, 0x14, 0x14, 0x08,// p
	0x08, 0x14, 0x14, 0x18, 0x7C,// q
	0x7C, 0x08, 0x04, 0x04, 0x08,// r
	0x48, 0x54, 0x54, 0x54, 0x20,// s
	0x04, 0x3F, 0x44, 0x40, 0x20,// t
	0x3C, 0x40, 0x40, 0x20, 0x7C,// u
	0x1C, 0x20, 0x40, 0x20, 0x1C,// v
	0x3C, 0x40, 0x30, 0x40, 0x3C,// w
	0x44, 0x28, 0x10, 0x28, 0x44,// x
	0x0C, 0x50, 0x50, 0x50, 0x3C,// y
	0x44, 0x64, 0x54, 0x4C, 0x44,// z
	0x00, 0x08, 0x36, 0x41, 0x00,// {
	0x00, 0x00, 0x7F, 0x00, 0x00,// |
	0x00, 0x41, 0x36, 0x08, 0x00,// }
	0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
	0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
};



// I2C functions ========================================================

// Write frame buffer to the OLED
void OLED::refresh()
{
	// Acquire the slave (OLED default address is 0x3C)
    if (ioctl(file_i2c, I2C_SLAVE, 0x3C) < 0)
		mvprintw(5, 0, "Failed to acquire bus access and/or talk to slave.\n");	//ERROR HANDLING; you can check errno to see what went wrong
    write(file_i2c, frame_buffer, 513);   // Not 1025 because I'm using a 128 x 32 display only
}

// Initialize the OLED
void OLED::init(int handle)
{
	// Save the file handle
	file_i2c = handle;
    // Frame buffer init
    frame_buffer[0] = 0x40; // Co == 0, D/C == 1 => 0100_0000
    for (int i = 1; i < 1025; i++)
        frame_buffer[i] = 0;    // clear the frame buffer
	// Acquire the slave (OLED default address is 0x3C)
    if (ioctl(file_i2c, I2C_SLAVE, 0x3C) < 0)
		mvprintw(5, 0, "Failed to acquire bus access and/or talk to slave.\n");	//ERROR HANDLING; you can check errno to see what went wrong
    // Display init
    write(file_i2c, oled_param, sizeof(oled_param));
	// First image (empty framebuffer)
    this->refresh();		// To avoid confusion with ncurses' refresh function
}

// OLED functions =======================================================

// Print a single ASCII character (7 x 5 pixel font) :
// The location parameters are in character units, not pixels : "col" goes from 0 to 20, "line" goes from 0 to 7.
// Very fast : exploits the layout of the frame buffer, where each byte represents a column of 8 pixels.
// The font used in this library also represents characters using columns of 8 pixels. Couldn't be simpler.
void OLED::print_character (char c, uint8_t col, uint8_t line)
{
	// Convert the ASCII code to an index in the font array:
	c -= 32;				// The font starts at ASCII code 32
	uint16_t f = c * 5;		// There are five bytes for each character (note : Cortex-M3 has single-cycle multiply : it's OK to use it)

	// convert col and line to a frame buffer index
	// uint16_t i = (line * 128) + (col * 6) + 1;  // Why is there a +1 in the SPI version ?
    uint16_t i = (line * 128) + (col * 6) + 2;      // I2C version : shifted one byte to account for the control byte

	// write the character (structures and pointers might make it faster and nicer to read):
	frame_buffer[i++] = Font5x7[f++];
	frame_buffer[i++] = Font5x7[f++];
	frame_buffer[i++] = Font5x7[f++];
	frame_buffer[i++] = Font5x7[f++];
	frame_buffer[i++] = Font5x7[f++];
	frame_buffer[i] = 0;	// empty column to the right
}

// Heavily-optimized function to set a single pixel on a 128 x 64 display.
// It exploits the layout of the frame buffer, where each byte represents a column of 8 pixels.
void OLED::set_pixel (uint16_t x, uint16_t y, uint8_t c)	// X, Y coordinates and color (1 or 0)
{
	uint16_t i = ((y & 0x38) << 4) | x;	// offset by X to get the address of the byte of interest
	uint8_t m = 0x1 << (y & 0x7);	// 8 bits in a byte represent 8 lines, so this is obvious

    // "+ 1" to indices are I2C-specific to account for the control byte
	if (c == 1)
		frame_buffer[i + 1] |= m;	// sets one bit in the byte
	else
		frame_buffer[i + 1] &= ~m;	// clears. Note : XOR would toggle a bit
}

// Function to print a C string. It just loops through the string and prints each character.
void OLED::print (char* s, uint8_t col, uint8_t line)
{
	int k = 0;
	int len = strlen (s);

	while (k < len)
	{
		print_character (s[k], col + k, line);
		k++;
	}
}


// Main function ========================================================

void main_oled ()
{
/*

    // YOUR STUFF HERE
    businit (); // initialize I2C for the OLED
    oledinit ();
    // Print something
    char str2[] = "Hello world";
    print (str2, 0, 0);
    //set_pixel (1, 1, 1);
    oledrefresh ();

    char str[10];
    for (int k = 0; k < 1000; k++)
    {
        sprintf(str, "%04i", k);
        print(str, 0, 1);
        oledrefresh();
    }
*/


/*
    // test pattern
    for (int z = 0; z < 128; z++)
        frame_buffer[z + 1] = z;
    oledrefresh ();
*/
/*
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 128; x++)
        {
            set_pixel(x, y, 1);
            oledrefresh ();
        }
    }
*/

}