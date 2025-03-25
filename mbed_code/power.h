#ifndef POWER_H
#define POWER_H

#include "dac.h"

void power_up(float core_volt); //analogous to powerUp in dpcs
void power_core(float core_volt); //analogous to powerAdjust in dpcs

//float current_meas(bool core);
void power_down();

//stuff in power_up.h that is not in power.h
//debugPower
//powerReset
//adjustSRAMVoltage

void adjustSRAMVoltage(float voltage);


#endif
