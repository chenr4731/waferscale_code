#include "scan.h"
#include "lcd.h"


void scan_clear()
{
    dual_printf("Clearing Scan Bits");
    //scan_data_in = 0;
    scan_load_chain = 0;
    scan_phi = 1;
    scan_phi_bar = 1;
    //scan_load_chip = 1;

    // 0 all inputs again
    scan_phi = 0;
    scan_phi_bar = 0;
    //scan_load_chip = 0;
}

void scan_init()
{
    scan_clear();
    
    //pc.printf("Opening Scan File\r\n");
    FILE *fp = fopen("/local/scan.txt", "r");
    if(!fp){
        dual_printf("ERROR: scan file not found");
    }
    //pc.printf("Scan File Open\r\n");
    {
        unsigned int high = 0;
        unsigned int low = 0;
        char sig_name[255];

        // First pass through file to find number of variables, length of chain, and longest var name
        int n = 0;
        while (!feof(fp)) {
            fscanf(fp, "%d:%d %s", &high, &low, sig_name);
            n++;
        }
        rewind(fp);
        scan_num_vars = n;
        scan_length = high + 1;
        pc.printf("Scan chain length %d, %d variables\r\n", scan_length, scan_num_vars);

        // Declare dynamic memory using sizes just found
        scan_bits  = new char[scan_length];
        scan_out   = new char[scan_length];
        for(int i=0; i<scan_length; i++) {
            scan_bits[i] = 0;
            scan_out[i] = 0;
        }

        scan_highs = new int[scan_num_vars];
        scan_lows  = new int[scan_num_vars];
        scan_names = new char*[scan_num_vars];

        n = 0;
        while (!feof(fp)) {
            fscanf(fp, "%d:%d %s", &high, &low, sig_name);
            
            scan_highs[n] = high;
            scan_lows[n] = low;
            scan_names[n] = new char[strlen(sig_name)+1];
            strcpy(scan_names[n], sig_name);
            //pc.printf("Read line signal %s, h=%i, l=%i\r\n", scan_names[n], scan_highs[n], scan_lows[n]);
            n++;
        }
    }
    //pc.printf("Closing Scan File\r\n");
    fclose(fp);
    //pc.printf("File Scan Closed\r\n");
}

int get_var_idx(char* varname)
{
    for(int i=0; i<scan_num_vars; i++) {
        if(!strcmp(varname, scan_names[i])) {
            return i;
        }
    }
    dual_printf("ERROR - INVALID SCAN VARIABLE");
    dual_printf(varname);
    return -1;
}

void set_scan_bits(char* varname, unsigned int value)
{
    int idx = get_var_idx(varname);

    int j=0;
    for(int i=scan_lows[idx]; i<=scan_highs[idx]; i++) {
        scan_bits[i] = (value & (1 << j)) >> j;
        j++;
    }
}

unsigned int get_scan_bits(char* varname){
    int idx = get_var_idx(varname);
    unsigned int value = 0;
    
    int j = 0;
    for(int i=scan_lows[idx]; i<=scan_highs[idx]; i++) {
        value = value | (scan_out[i] << j);
        j++;
    }
    
    return value;
}

void load_chip()
{
    //scan_load_chip = 1;
    //scan_load_chip = 0;
}

void load_chain()
{
    scan_load_chain = 1;
    scan_phi = 1;
    scan_phi = 0;
    scan_phi_bar = 1;
    scan_phi_bar = 0;
    scan_load_chain = 0;
}

void rotate_chain()
{
    for(int i=0; i<scan_length; i++) {
        //scan_data_in = scan_bits[i];
        scan_out[i] = scan_data_out;
        scan_phi = 1;
        scan_phi = 0;
        scan_phi_bar = 1;
        scan_phi_bar = 0;
    }
}
