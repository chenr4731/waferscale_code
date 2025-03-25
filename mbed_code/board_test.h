#include "mbed.h"
#include "dac.h"
#include "pinout.h"
#include "scan.h"
#include "pll.h"
#include "lcd.h"
#include "jtag.h"

void analog_test();
void scan_set_test();
void lcd_test();
void mem_test(JTAG &jtag);
