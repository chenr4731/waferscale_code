#include "dac.h"
#include "power.h"

using namespace std;

#define INA_GAIN 200
/*
float current_meas(bool core)
{     
    float ampv = meas_amp * 3.3;
    float vdrop = ampv / INA_GAIN;
    float res = 1;
    if(core){
        res = 0.25;
    }
    return  vdrop / res;
}
*/


void power_core(float core_volt)
{
    float periph_volt = core_volt;
    float mem_volt = periph_volt;
    if(core_volt > 1){ //if desired voltage is too high, set all voltages to 1V
        periph_volt = 1;
        core_volt = 1;
        mem_volt = 1;
    }
    if(core_volt < 0.9){ //peripheral voltage must be at least 0.9V
        periph_volt = 0.9;
    }
    if(core_volt < 0.8){ //memory voltage must be at least 0.05V than desired voltage if desired voltage is too low
        mem_volt = core_volt + 0.05;
    }
    power_chan(PADVDD, periph_volt); //set periph voltage
    wait(POWER_UP_TIME);

    // Core and Memory
    power_chan(COREVDD, core_volt); //set core voltage
    wait(POWER_UP_TIME);
    power_chan(MEM1VDD, mem_volt); //set mem1 voltage
    wait(POWER_UP_TIME);
    power_chan(MEM2VDD, mem_volt); //set mem2 voltage
    wait(POWER_UP_TIME);

    // Clock
    power_chan(CLOCKVDD, periph_volt); //set clock voltage
    wait(POWER_UP_TIME);
    power_chan(PLLAVDD, periph_volt); //set pll voltage
    wait(POWER_UP_TIME);

    // Sensor
    power_chan(SENSORVDD, core_volt);//power_chan(SENSORVDD, 1.0);
    wait(POWER_UP_TIME);
    power_chan(SENSORLOWVDD, 0.35);
    wait(POWER_UP_TIME);
    power_chan(SENSORSTRESSVDD, periph_volt);//power_chan(SENSORSTRESSVDD, 1.0);
    wait(POWER_UP_TIME);
}


void power_up(float core_volt)
{
    // The 1.8V supplies MUST be up whenever the 3.3V ones are
    power_chan(ADVDD2, 1.8);
    wait(POWER_UP_TIME);
    power_chan(DVDD2, 1.8);
    wait(POWER_UP_TIME);
    
    float periph_volt = core_volt;
    
    
    if(core_volt > 1){
        periph_volt = 1;
        core_volt = 1;
    }
    if(core_volt < 0.95){
        periph_volt = 0.95;
    }
    
    float mem_volt = periph_volt;
    
    // Other padring
    power_chan(ADVDD, 3.3);
    wait(POWER_UP_TIME);
    power_chan(DVDD, 3.3);
    wait(POWER_UP_TIME);
    power_chan(PADVDD, 1.0);//power_chan(PADVDD, 1.0);
    wait(POWER_UP_TIME);

    // Core and Memory
    power_chan(COREVDD, core_volt);
    wait(POWER_UP_TIME);
    power_chan(MEM1VDD, mem_volt);//power_chan(MEM1VDD, 1.0);
    wait(POWER_UP_TIME);
    power_chan(MEM2VDD, mem_volt);//power_chan(MEM2VDD, 1.0);
    wait(POWER_UP_TIME);

    // Clock
    power_chan(CLOCKVDD, periph_volt);//power_chan(CLOCKVDD, 1.0);
    wait(POWER_UP_TIME);
    power_chan(PLLAVDD, 1.0);//power_chan(PLLAVDD, 1.0);
    wait(POWER_UP_TIME);
    power_chan(RING_OSC_NBIAS, 0.75);
    wait(POWER_UP_TIME);

    // Sensor Supplies
    power_chan(SENSORVDD, core_volt);//power_chan(SENSORVDD, 1.0);
    wait(POWER_UP_TIME);
    power_chan(SENSORLOWVDD, 0.35);
    wait(POWER_UP_TIME);
    power_chan(SENSORSTRESSVDD, 1.0);//power_chan(SENSORSTRESSVDD, 1.0);
    wait(POWER_UP_TIME);

    power_indicator = 1;
}

void power_down()
{

    // Zero/float all inputs, so they are not above DVDD when you lower it
    PORESETn = 0;
    CORERESETn = 0;
    GPIO1.input();
    GPIO2.input();
    GPIO3.input();
    TCK = 0;
    TMS = 0;
    TDI = 0;
    //scan_data_in = 0;
    scan_phi = 0;
    scan_phi_bar = 0;
    scan_load_chain = 0;
    //scan_load_chip = 0;

    // Core and Memory
    power_chan(COREVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(MEM1VDD, 0);
    wait(POWER_UP_TIME);
    power_chan(MEM2VDD, 0);
    wait(POWER_UP_TIME);

    // Clock
    power_chan(CLOCKVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(PLLAVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(RING_OSC_NBIAS, 0);
    wait(POWER_UP_TIME);

    // Sensor Supplies
    power_chan(SENSORVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(SENSORLOWVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(SENSORSTRESSVDD, 0);
    wait(POWER_UP_TIME);

    // Other padring
    power_chan(ADVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(DVDD, 0);
    wait(POWER_UP_TIME);
    power_chan(PADVDD, 0);
    wait(POWER_UP_TIME);

    // The 1.8V supplies MUST be up whenever the 3.3V ones are
    power_chan(ADVDD2, 0);
    wait(POWER_UP_TIME);
    power_chan(DVDD2, 0);
    wait(POWER_UP_TIME);

    power_indicator = 0;
}

void adjustSRAMVoltage(float voltage)
{
    //assumption is that only SRAM voltage needs to be adjusted, other voltages
    //will not affect SRAM.
    
    float mem_volt = voltage;
    if (mem_volt > 1.0) { //clamp mem_volt at a maximum of 1.5 volt
        //_USB_CONSOLE.printf("Requested memory voltage was %0.02f, clamping to 1.0 V", mem_volt);
        mem_volt = 1.0;
    }
    
    power_chan(MEM1VDD, mem_volt);//power_chan(MEM1VDD, 1.0);
    wait(POWER_UP_TIME);
    power_chan(MEM2VDD, mem_volt);//power_chan(MEM2VDD, 1.0);
    wait(POWER_UP_TIME);
    
}
