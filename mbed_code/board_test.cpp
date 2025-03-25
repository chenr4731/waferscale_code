#include "board_test.h"


void mem_test(JTAG &jtag)
{

    jtag.PowerupDAP();
    unsigned int base = 0x10000000;
    unsigned int size = 2000;
    unsigned int* data;
    data = new unsigned int[size];
    
    for(unsigned int i=0; i<size; i++){
        data[i] = i;
    }
    
    pc.printf("Performing memWrite\r\n");
    jtag.memWrite(base, data, size);

    pc.printf("Checking memWrite\r\n");
    for(unsigned int i=0; i<size; i++){
        unsigned int d = jtag.readMemory(base + i*4);
        //pc.printf("MEM[%X] = %X\r\n", i, d);
        if(d != i){
            pc.printf("MEMERROR: MEM[%X] = %X\r\n", i, d);
        }
    }

    jtag.writeMemory(base + 4*20, 0xdeadbeef);

    pc.printf("Performing memRead\r\n");
    jtag.memRead(base, data, size, true, true);
    
    pc.printf("Mem test done\r\n");
    delete data;
    
}

void lcd_test()
{

    lcd_default();
    wait(1);
    
    lcd_printf("Initial Power On");
    wait(1);
    lcd_clear( );
    wait(1);
 
    lcd_goto( LINE4_BEGIN );
    lcd_printf("Line 4");
    wait(1);
    lcd_goto( LINE2_BEGIN );
    lcd_printf("Line 2");
    wait(1);
    
    lcd_clear_line( 4);
    lcd_home();
    lcd_printf("Line 1");
    lcd_clear();
    wait(1);

    lcd_printf("ABCDEF");
    wait(1);
    lcd_backspace(); lcd_backspace(); lcd_backspace();
    wait(1);
    lcd_cursor_left(); lcd_cursor_left(); lcd_cursor_left();
    lcd_printf("def");
    wait(1);
    lcd_cursor_right(); lcd_cursor_right(); lcd_cursor_right();
    lcd_printf("abc");
    wait(1);
    lcd_clear();
    wait(1);
    
    lcd_goto( LINE1_BEGIN ); lcd_printf("LINE1");
    lcd_goto( LINE2_BEGIN ); lcd_printf("LINE2");
    lcd_goto( LINE3_BEGIN ); lcd_printf("LINE3");
    lcd_goto( LINE4_BEGIN ); lcd_printf("LINE4");
    wait(1);
    
    int i=0;
    while(1) {
        lcd_set_contrast( (5*i)%50 );
        lcd_backlight_level( i%8 );
        
        lcd_shift_right(); wait_ms(50); 
        lcd_shift_right(); wait_ms(50);
        lcd_shift_right(); wait_ms(50);
        lcd_shift_right(); wait_ms(50);
        lcd_shift_right(); wait_ms(50);
        lcd_shift_right(); wait_ms(50);
        lcd_shift_right(); wait_ms(50);
        
        lcd_shift_left(); wait_ms(50);
        lcd_shift_left(); wait_ms(50);
        lcd_shift_left(); wait_ms(50);
        lcd_shift_left(); wait_ms(50);
        lcd_shift_left(); wait_ms(50);
        lcd_shift_left(); wait_ms(50);
        lcd_shift_left(); wait_ms(50);
        
        i++;
        wait_ms(100);
    }
}


void i2ctest(){

    char data[4] = "abc";
    int A = 0;
    for (A = 0; A<=0xFF; A++) {
        if (lcd.write(A,data,3,false)==0) {
            pc.printf (" GOT LCD %X\r\n", A);
        }
    }
        
}

void analog_test()
{
    
    power_chan(ADVDD, 0.10);
    wait(POWER_UP_TIME);
    power_chan(PADVDD, 0.15);
    wait(POWER_UP_TIME);
    power_chan(DVDD, 0.20);
    wait(POWER_UP_TIME);
    power_chan(ADVDD2, 0.25);
    wait(POWER_UP_TIME);
    power_chan(COREVDD, 0.30);
    wait(POWER_UP_TIME);
    power_chan(DVDD2, 0.35);
    wait(POWER_UP_TIME);
    power_chan(SENSORVDD, 0.40);
    wait(POWER_UP_TIME);
    power_chan(SENSORLOWVDD, 0.45);
    wait(POWER_UP_TIME);
    power_chan(SENSORSTRESSVDD, 0.50);
    wait(POWER_UP_TIME);
    power_chan(CLOCKVDD, 0.55);
    wait(POWER_UP_TIME);
    power_chan(MEM1VDD, 0.60);
    wait(POWER_UP_TIME);
    power_chan(MEM2VDD, 0.65);
    wait(POWER_UP_TIME);
    power_chan(PLLAVDD, 0.70);
    wait(POWER_UP_TIME);
    power_chan(RING_OSC_NBIAS, 0.75);
    power_indicator = 1;
}

void scan_set_test()
{

    rotate_chain();
    rotate_chain();
    int g = get_scan_bits("PLL_LFTUNE");
    pc.printf("initially %d\r\n", g); // should be 0
    
    set_scan_bits("PLL_LFTUNE", 27);
    rotate_chain();
    load_chip();
    
    //load_chain();
    rotate_chain();
    
    
    g = get_scan_bits("PLL_LFTUNE");
    pc.printf("then %d\r\n", g); // should be 27
 
    pc.printf("The 13th line of range is %d\r\n", get_binline_by_num("/local/Prange.txt", 13));
 
}

void jtag_test()
{
    JTAG jtag;
    for(char ii=0x0; ii<=0xf; ii++){
    jtag.setIR(ii);

    //jtag.leaveState();
    jtag.setState('d');
        
    int rett = jtag.shiftBits(0xdeadbeef >> 1, 32);
    jtag.leaveState();
    pc.printf("ii %x, rett %x\r\n", ii, rett);
     
}     

}
void pll_test()
{
    scan_pll(1, 1, 126, 1, 12, 32);
    unsigned long long freq = calc_pll_freqs(1, 1, 126, 1, 12, 32);
    freq = freq / 1000000; // in MHz
    int fMHz = freq;
    
    pc.printf("pll freq: %d MHz", fMHz);
    
    set_scan_bits("use_scan", 1);
    set_scan_bits("PLL_RESET", 1);
    set_scan_bits("intclk_source", 2);
    set_scan_bits("extclk_source", 1);
    set_scan_bits("ext_div_by", 10);
    rotate_chain();
    load_chip();

    set_scan_bits("PLL_RESET", 0);
    rotate_chain();
    load_chip();
    
    PORESETn = 1;
    CORERESETn = 1;
    set_scan_bits("RESET_scan", 1);
    set_scan_bits("CORERESET_scan", 1);

}
