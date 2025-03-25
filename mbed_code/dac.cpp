#include "dac.h"

using namespace std;

void power_chan(char i2caddr, char chan, float voltage)
{   
    char data[3];
    
    // voltage = 3.3 * Code / 4096
    // Code = voltage * 4096 / 3.3 = voltage * 1241
    unsigned int full_code = 1241 * voltage;
    
    char MSDB, LSDB;
    LSDB = full_code % 0x10; //is this correct? masking one byte should be % 0x100 -Albert
    MSDB = full_code / 0x10; //same for this
    
    data[0]=chan;
    data[1]=MSDB;
    data[2]=LSDB;
    
    power_error_indicator = 1;
    //pc.printf ("%X\r\n", A);
    while (dac_i2c.write(i2caddr,data,3,false)) {
    }
    power_error_indicator = 0;
}

