/*  OLED Driver for SSD1306 I2C units
    Nefastor
    Warning : hard-configured for 128 x 32 displays, will not work correctly on other sizes.
    Warning : this was coded in a hurry to have "something that just works".
    Warning : originally coded in C, this C++ port was also done in a hurry.
*/

#ifndef __SSD1306_OLED_DRIVER__
#define __SSD1306_OLED_DRIVER__

// The code was originally written for STM32, quick porting requires some typedefs
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

class OLED
{
    public:
        int file_i2c;                   // I2C bus handle
        uint8_t frame_buffer[513];      // 512 bytes of pixel data for 128x32. Additional byte for the "C/D" bit

    public:
        // OLED(); // default constructor
        void refresh ();
        void init (int handle);     // takes an I2C bus file handle and initializes the display on that bus
        void print_character (char c, uint8_t col, uint8_t line);
        void set_pixel (uint16_t x, uint16_t y, uint8_t c);	        // X, Y coordinates and color (1 or 0)
        void print (char* s, uint8_t col, uint8_t line);
};


#endif // __SSD1306_OLED_DRIVER__
