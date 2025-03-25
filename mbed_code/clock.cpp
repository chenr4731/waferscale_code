#include "clock.h"

float ext_freq(DigitalIn* clk_pin)
{
    bool last = false;
    unsigned int count = 0;
    
    Timer t;
    t.start();
    for(int i=0; i<100000; i++){
        bool clkval = *clk_pin;
        if(clkval && !last){
            count++;
        }
        last = clkval;
    }
    float time = t.read();
    t.stop();
    if(count < 1000){
        dual_printf("Clk read inaccurate");
        pc.printf("Clk count %d\r\n", count);
    }
    //pc.printf("T: %f\r\nC: %i\r\n", time, count);
    
    return count / time;
}
