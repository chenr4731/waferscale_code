#include "mbed.h"
#include "pinout.h"

#ifndef SCAN_H
#define SCAN_H

static char* scan_bits;
static char* scan_out;
static int   scan_length;

static int* scan_highs;
static int* scan_lows;
static char** scan_names;
static int scan_num_vars;

void scan_clear();
void scan_init();
int get_var_idx(char* varname);
void set_scan_bits(char* varname, unsigned int value);
unsigned int get_scan_bits(char* varname);
void load_chip();
void load_chain();
void rotate_chain();

#endif
