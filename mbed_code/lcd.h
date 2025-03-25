    //*******************************************************************
    // * October, 2012
    // * Definitions for I2C LCD program
    // * 
    // *
    // * 
    /********************************************************************
     function protocol:
     0xFE, 0x41 - display on            
     0xFE, 0x42 - display off            
     0xFE, 0x45 - set cursor position      
     0xFE, 0x46 - home cursor            
     0xFE, 0x47 - underline cursor on      
     0xFE, 0x48 - underline cursor off      
     0xFE, 0x49 - move cursor left 1 space   
     0xFE, 0x4A - move cursor right 1 space   
     0xFE, 0x4B - blinking cursor on      
     0xFE, 0x4C - blinking curson off      
     0xFE, 0x4E - back space.
     0xFE, 0x51 - clear screen            
     0xFE, 0x52 - set contrast   (1-50)      
     0xFE, 0x53 - set backlight brightness    (1-16)
     0xFE, 0x54 - load custom characters   
     
     0xFE, 0x55 - move screen left 1 space   
     0xFE, 0x56 - move screen right 1 space   

     0xFE, 0x61 - change BAUD rate (1 - 8)   
     0xFE, 0x62 - change I2C address (0-255)
     0xFE, 0x70 - display version number   
     0xFE, 0x71 - display RS232 BAUD rate   
     0xFE, 0x72 - display I2C address      
     0xFE, 0xFE - send next byte to command register   
    ********************************************************************/
    #include "mbed.h"
    #include "pinout.h"
    
    // I2C Addr Pin
    #define LCDADDR 0x50

    #define LINE1_BEGIN 0x00
    #define LINE2_BEGIN 0x40
    #define LINE3_BEGIN 0x14
    #define LINE4_BEGIN 0x54
    
    #define LINE1_END 0x13
    #define LINE2_END 0x53
    #define LINE3_END 0x27
    #define LINE4_END 0x67

    // proto type functions
    void dual_printf(char* a);
    void lcd_default();
    void lcd_clear();
    void lcd_home();
    void lcd_clear_line(unsigned int line);
    void lcd_goto(unsigned int position);
    void lcd_set_contrast(unsigned int level);
    void lcd_backlight_level(unsigned int level);
    void lcd_shift_right();
    void lcd_shift_left();
    void lcd_cursor_right();
    void lcd_cursor_left();
    void lcd_backspace();
    void lcd_cursor_blink(bool blink);
    void lcd_printf(char* message);
