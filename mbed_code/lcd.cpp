#include "mbed.h"
#include "lcd.h"

int lcdlinenum;

void dual_printf(char* a){
    pc.printf(a); 
    pc.printf("\r\n");
    
    lcd_clear_line(lcdlinenum);
  
    lcd_printf(a);
    if(lcdlinenum > 3){
        lcdlinenum = 1;
    }else{
        lcdlinenum++;
    }
}

/********************************************************************
 lcd default
********************************************************************/
void lcd_default()
{
    lcdlinenum = 1;
    lcd.frequency(10000);
    lcd_clear();
    lcd_set_contrast( 40 );
    lcd_backlight_level( 8 );
    lcd_cursor_blink(1);
}
/********************************************************************
 lcd clear
********************************************************************/
void lcd_clear()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x51;
    lcd.write(LCDADDR, data, 2, false);    
    wait_ms(2);
}
/********************************************************************
 lcd home
********************************************************************/
void lcd_home()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x46;
    lcd.write(LCDADDR, data, 2, false);    
    wait_ms(2);
}

void lcd_printf(char* message){
    lcd.write(LCDADDR, message, strlen(message), false);   
}

/********************************************************************
 clear one line of display
********************************************************************/
void lcd_clear_line(unsigned int line)
{
    switch (line){
        case 1:
            lcd_goto( LINE1_BEGIN );
            lcd_printf("                    ");
            lcd_goto( LINE1_BEGIN );
            break;
        case 2:
            lcd_goto( LINE2_BEGIN );
            lcd_printf("                    ");
            lcd_goto( LINE2_BEGIN );
            break;
        case 3:
            lcd_goto( LINE3_BEGIN );
            lcd_printf("                    ");
            lcd_goto( LINE3_BEGIN );
            break;
        case 4:
            lcd_goto( LINE4_BEGIN );
            lcd_printf("                    ");
            lcd_goto( LINE4_BEGIN );
            break;
        default:
            break;
    }
    wait_ms(5);
}

void lcd_cursor_blink(bool blink){
    char data[10];
    data[0] = 0xFE;
    if(blink){
        data[1] = 0x4b;
    }else{
        data[1] = 0x4c;
    }    
    
    lcd.write(LCDADDR, data, 2, false);    
    
    wait_ms(2);
}

/********************************************************************
 lcd cursor goto
********************************************************************/
void lcd_goto(unsigned int position)
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x45;
    data[2] = position;
    lcd.write(LCDADDR, data, 3, false);    
    
    wait_ms(2);
}
/********************************************************************
 lcd set contrast
********************************************************************/
void lcd_set_contrast(unsigned int level)
{
    if(level > 50){ level = 50; }
    
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x52;
    data[2] = level;
    lcd.write(LCDADDR, data, 3, false);  
      
    wait_ms(10);
}
/********************************************************************
 lcd set backlight level
********************************************************************/
void lcd_backlight_level(unsigned int level)
{
    if(level > 8){ level = 8; }
    
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x53;
    data[2] = level;
    lcd.write(LCDADDR, data, 3, false);   
    
    wait_ms(2);
}
/********************************************************************
 lcd set backlight level
********************************************************************/
void lcd_shift_right()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x56;
    lcd.write(LCDADDR, data, 2, false);   
    
    wait_ms(1);
}
/********************************************************************
 lcd set backlight level
********************************************************************/
void lcd_shift_left()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x55;
    lcd.write(LCDADDR, data, 2, false);   
    
    wait_ms(1);
}
/********************************************************************
 lcd set backlight level
********************************************************************/
void lcd_cursor_right()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x4A;
    lcd.write(LCDADDR, data, 2, false);   
    
    wait_ms(1);
}
/********************************************************************
 lcd set backlight level
********************************************************************/
void lcd_cursor_left()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x49;
    lcd.write(LCDADDR, data, 2, false);   
    
    wait_ms(1);
}
/********************************************************************
 lcd set backlight level
********************************************************************/
void lcd_backspace()
{
    char data[10];
    data[0] = 0xFE;
    data[1] = 0x4E;
    lcd.write(LCDADDR, data, 2, false);   

    wait_ms(1);
}
