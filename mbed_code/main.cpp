
#include "mbed.h"
#include "string"
#include "dac.h"
#include "board_test.h"
#include "scan.h"
#include "power.h"
#include "pinout.h"
#include "pll.h"
#include "lcd.h"
#include "jtag.h"
#include "mmap.h"
#include "clock.h"
#include "EasyBMP.h"
#include "main.h"
#include "elf.h"
//#include "mnist_input.h"


//port from dpcs
Serial _USB_CONSOLE(USBTX, USBRX);
//JTAG* _JTAG;
//FILE* _FP; //PORT THIS FUNCTIONALITY FROM DPCS


DigitalOut myled(LED4);

//#define TEST_DMEM                             // uncomment this to test Data Memory, currently post-processing and march test functions do not support data memory

#ifdef WRITE_RESULTS
//LocalFileSystem local("local");               // Create the local filesystem under the name "local"
#endif

#define BYTE_MASK 0x000000FF
#define BYTE0_SHIFT 0
#define BYTE1_SHIFT 8
#define BYTE2_SHIFT 16
#define BYTE3_SHIFT 24
#define ERROR_BYTE0 1
#define ERROR_BYTE1 2
#define ERROR_BYTE2 4
#define ERROR_BYTE3 8
#define ZERO 0x00000000
#define ONE 0xFFFFFFFF

//defining the memory address of all the config regs could be useful.

#define OUTPUT 0x20000100

void tckTicks(unsigned int c)
{
    unsigned int i;
    TCK = 0;
    for (i=0; i<c; i++) {
        TCK = 0;
        //wait_us(500);
        TCK = 1;
        //wait_us(500);
    }
    TCK = 1;
}

/*
 * Function Name: getNum
 * Arguments: none/void
 * Description: This function get a single digit number from terminal,
 *              and return the number
 * Comments: Now this function only support signal digit number.
 */
int getNum() /* only support single digit */
{
    int num;
    char tmp;
    while (1) {
        pc.printf("Type in number, press enter to enter\r\n");
        pc.scanf("%c", &tmp);
        num = (int)tmp - 48;
        pc.printf("You type in %d, \r\nTo confirm, press space bar; To re-enter, press any other key\r\n", num);
            
        tmp = pc.getc();      
        if (tmp == 0x20) {
            break;
        } else {
            continue;
        }
    }
    return num;
}

/*
 * Function Name: reg_write
 * Arguments: broadcast, whole_tile, block_tile, tile_core, addr, value,  
 *            curr_tile_y, active_tiles[], x_tiles, 
 *            cm3_count, core_count[][]
 * Description: This function does a config register write
 * Comments: This function is a specialized version of memAccess.
 *           See memAccess for details.
 */
int reg_write(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int **core_count)
{
    //DigitalOut FINISH_FLAG (LED3);
    //_USB_CONSOLE.baud(9600);

    _USB_CONSOLE.printf("[reg_write]** ");

    int num_cores;
    int same_prog_tmp;
    int *core_op;
    
    if (broadcast) {
        SAMEPROG = 1;
        same_prog_tmp = 1;
        num_cores = active_tiles[curr_tile_y];
        core_op = new int[num_cores];
        for (int i=0; i<num_cores; i++) {
            core_op[i] = 1;
        }
        pc.printf("Broadcast to whole chain, curr_tile_y = %d\r\n", curr_tile_y);
    } else if (whole_tile) {
        SAMEPROG = 1;
        same_prog_tmp = 1;
        num_cores = active_tiles[curr_tile_y];
        core_op = new int[num_cores];
        for (int i=0; i<num_cores; i++) {
            if (i==block_tile) {
                core_op[i] = 1;
            } else {
                core_op[i] = 0;
            }
        }
        pc.printf("Broadcast to a tile, x:%d y:%d\r\n", block_tile, curr_tile_y);
    } else {
        SAMEPROG = 0;
        same_prog_tmp = 0;
        num_cores = cm3_count;
        core_op = new int[num_cores];
        int core_op_loc = 0;
        for (int x=0; x<block_tile; x++) {
            core_op_loc += core_count[x][curr_tile_y];
        }
        core_op_loc += tile_core;
        for (int i=0; i<num_cores; i++) {
            //if (i==(block_tile*core_count + tile_core)) {
            if (i == core_op_loc) {
                core_op[i] = 1;
            } else {
                core_op[i] = 0;
            }
        }
        pc.printf("To a single core: x:%d y:%d core:%d\r\n", block_tile, curr_tile_y, tile_core);
    }

    pc.printf("register write to addr:%x with val:%x\r\n", addr, value);

    JTAG* _JTAG = new JTAG;
    _JTAG->setCurrTileID(curr_tile_y);
    _JTAG->setNumCores(num_cores);
    _JTAG->setNumTiles(active_tiles[curr_tile_y]);

    for (int k=0; k<num_cores; k=k+1) {
        if (core_op[k]) _JTAG->setCore(k);
    }

    _JTAG->getConfig();
    pc.printf("[JTAG config] SAMEPROG = %d\r\n", same_prog_tmp);

    pc.printf("[reg_write]** Verify Chip ID...\r\n");
    int idcode = _JTAG->readID();
    if(idcode != 0x4ba00477) {//0x4ba00477
        pc.printf("ERROR: IDCode %X, exiting program.\r\n", idcode);
        wait(2);
        //power_down();
        //delete _JTAG;
        return -1;
    }
    pc.printf("IDCode %X\r\n", idcode);
    _JTAG->reset();
    _JTAG->leaveState();
    //_JTAG->PowerupDAP();
    while (_JTAG->PowerupDAP() != 0) {
        _JTAG->reset();
        _JTAG->leaveState();
    }

    _JTAG->writeMemory(addr, value);
    tckTicks(150);

    pc.printf("[reg_write]** reading memory.\r\n");
    unsigned int read_val = _JTAG->readMemory(addr);

    if (read_val != value) {
        pc.printf("[reg_write] read back value wrong:%x, expected:%x\r\n", read_val, value);
        delete _JTAG;
        return -1;
    } else {
        pc.printf("[reg_write] read back correct: read_val = %x\r\n", read_val);
    }
    //_JTAG->reset();
    //_JTAG->leaveState();
    delete _JTAG;

    return 0;
}

/*
 * Function Name: memAccess
 * Arguments: broadcast, whole_tile, block_tile, tile_core, addr, value, RW, 
 *            halt, curr_tile_y, active_tiles[], x_tiles, 
 *            cm3_count, core_count[][]
 * Description: This function does memory access.
 * Comments: 0) This is the generic function / API to access memory addr space.
 *              Before calling this function, certain variables need to set up
 *              properly, mainly the arguments.
 *           1) There are 3 modes of access, determined by the first 4 arguments
 *              i)   Broadcast to the whole chain. This mode is used 
 *                   if the first argument (bool broadcast) is set to true, 
 *                   and the following 3 arguments will be ignored.
 *              ii)  Broadcast to a tile. This mode is used 
 *                   if the first argument (bool broadcast) is set to false, 
 *                   and the secoon argument (bool whole_tile) is set to true. 
 *                   The tile that will be  addressed is determined by 
 *                   the third argument (block_tile).
 *                   The fourth argument (tile_core) will be ignored
 *              iii) Access a single core. This mode is used if the first 2
 *                   arguments (bool broadcast and bool whole_tile) are both
 *                   set to false. The core that will be address is determined
 *                   by the third (block_tile) and fourth (tile_core) arguments,
 *                   with block_tile indicating which tile the core is in,
 *                   and tile_core indicating which core inside the said tile.
 *           2) The next three arguments (addr, value, RW) indicate
 *              the access being a read (0 for RW) or a write (1 for RW), 
 *              the addr, and value (if a write, the value is used to do a 
 *              read back check).
 *           3) bool halt indicates whether this memory access 
 *              will do a haltCore. 
 *              Generally, accessing a register does not required a haltCore,
 *              while accessing SRAM requires a haltCore.
 *           4) The following arguments are the general configuration setups:
 *              curr_tile_y indicates the current chain,
 *              active_tiles[] has the number of active tiles of all chains,
 *              x_tiles indicates the number of tiles in the current chain,
 *              cm3_count indicates the maximum number of accessible cores 
 *              in the current chain (one tile counts as one accessible core
 *              when doing broadcasts),
 *              core_count[][] has the number of cores in every tile
 */
int memAccess(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int RW, bool halt, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int **core_count)
{
    _USB_CONSOLE.printf("[memAccess]** ");

    int num_cores;
    int same_prog_tmp;
    
    if (broadcast) {
        SAMEPROG = 1;
        same_prog_tmp = 1;
        num_cores = active_tiles[curr_tile_y];
        //pc.printf("Broadcast to whole chain, curr_tile_y = %d\r\n", curr_tile_y);
    } else if (whole_tile) {
        SAMEPROG = 1;
        same_prog_tmp = 1;
        num_cores = active_tiles[curr_tile_y];
       // pc.printf("Broadcast to a tile, x:%d y:%d\r\n", block_tile, curr_tile_y);
    } else {
        SAMEPROG = 0;
        same_prog_tmp = 0;
        num_cores = cm3_count;
        //pc.printf("To a single core: x:%d y:%d core:%d\r\n", block_tile, curr_tile_y, tile_core);
    }

    int core_op[num_cores];

    if (broadcast) {
        for (int i=0; i<num_cores; i++) {
            core_op[i] = 1;
        }
    } else if (whole_tile) {
        for (int i=0; i<num_cores; i++) {
            if (i==block_tile) core_op[i] = 1;
            else core_op[i] = 0;
        }
    } else {
        int core_op_loc = 0;
        for (int x=0; x<block_tile; x++) {
            core_op_loc += core_count[x][curr_tile_y];
        }
        core_op_loc += tile_core;
        for (int i=0; i<num_cores; i++) {
            //if (i==(block_tile*core_count + tile_core)) {
            if (i == core_op_loc) {
                core_op[i] = 1;
            } else {
                core_op[i] = 0;
            }
        }
    }

    if (halt) {
        pc.printf("memory (halt) ");
        CORERESETn = 1;
        tckTicks(8192);//ensure synchronous reset
    } else pc.printf("register (no halt) ");

    if (RW) pc.printf("write to addr:%x with val:%x\r\n", addr, value);
    else pc.printf("read addr:%x\r\n", addr);

    JTAG* _JTAG = new JTAG;
    _JTAG->setCurrTileID(curr_tile_y);
    _JTAG->setNumCores(num_cores);
    _JTAG->setNumTiles(active_tiles[curr_tile_y]);

    for (int k=0; k<num_cores; k=k+1) {
        if (core_op[k]) _JTAG->setCore(k);
    }

    //_JTAG->getConfig();
    //pc.printf("[JTAG config] SAMEPROG = %d\r\n", same_prog_tmp);

    //pc.printf("[memAccess]** Verify Chip ID...\r\n");
    int idcode = _JTAG->readID();
    if(idcode != 0x4ba00477) {//0x4ba00477
        pc.printf("ERROR: IDCode %X, exiting program.\r\n", idcode);
        wait(2);
        //power_down();
        delete _JTAG;
        return -1;
    }
    //pc.printf("IDCode %X\r\n", idcode);
    _JTAG->reset();
    _JTAG->leaveState();
    //_JTAG->PowerupDAP();
    while (_JTAG->PowerupDAP() != 0) {
        _JTAG->reset();
        _JTAG->leaveState();
    }

    if (halt) {
        if (_JTAG->haltCore() != 0) {
            pc.printf("ERROR: Could not halt core, exiting program.\r\n");
            delete _JTAG;
            return -1;
        }
        pc.printf("core halted.\r\n");
    }

    if (RW) {
        pc.printf("[memAccess]** writing addr:%x value:%x *****\r\n", addr, value);
        _JTAG->writeMemory(addr, value);
        tckTicks(4096);
    }
    // the following loop is just for read testing
    /*for (int k=0; k<num_cores; k=k+1) {
        _JTAG->setCore(k);
    }
    _JTAG->getConfig();*/
    pc.printf("[memAccess]** reading memory.\r\n");
    unsigned int read_val = _JTAG->readMemory(addr);

    if (RW) {
        if (read_val != value) {
            pc.printf("[write] read back value wrong:%x, expected:%x\r\n", read_val, value);
            delete _JTAG;
            return -1;
        } else {
            pc.printf("[write] read back correct: read_val = %x\r\n", read_val);
        }
    } else {
        pc.printf("[read] read_val = %x\r\n", read_val);
    }
   
    
    delete _JTAG;
    delete [] core_op;

    return read_val;//0;
}

/*
 * Function Name: loopRead
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][], addr
 * Description: loop through all the tiles and read the addr
 * Comments: The outer loop goes through all the chain, 
 *           the inner loop goes throught all the tiles(and cores) in one chain.
 *           Then call memAccess to read addr w/o haltCore
 */
int loopRead(int x_tiles, int y_tiles, int *active_tiles, int **core_count, unsigned int addr)
{
    int cm3_count;
    for (int y=0; y<y_tiles; y=y+1) {
        switch(y) {
            case 0: 
                CHAIN_SELECT = 1; 
                CHAIN_SELECT_1 = 0;
                break;
            case 1: 
                CHAIN_SELECT = 0; 
                CHAIN_SELECT_1 = 1; 
                break;
            default:
                CHAIN_SELECT = 0;
                CHAIN_SELECT_1 = 0;
                pc.printf("[loopRead] bad CHAIN_SELECT: curr_tile_y = %d\r\n", y);
                break;
        }
        //cm3_count = core_count * active_tiles[y];
        cm3_count = 0;
        int curr_active_tiles = active_tiles[y];
        for (int x=0; x<curr_active_tiles; x++) {
            cm3_count += core_count[x][y];
        }
        for (int x=0; x<x_tiles; x=x+1) {
            pc.printf("[loopRead] read tile x:%d y:%d @ addr:%x  ***********\r\n", x, y, addr);
            memAccess(false, true, x, 0, addr, 0x00000000, 0, false, y, active_tiles, x_tiles, cm3_count, core_count);
            //memAccess(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int RW, bool halt, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int core_count)
            tckTicks(150);
        }
    }
    pc.printf("[loopRead] finished****************\r\n");
    return 0;
}

/*
 * Function Name: loopMemWrite
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][], addr, value
 * Description: loop through all the tiles and write to the addr w/ value
 * Comments: The outer loop goes through all the chain, 
 *           the inner loop goes throught all the tiles(and cores) in one chain.
 *           Then call memAccess to write value to addr w/ haltCore
 */
int loopMemWrite(int x_tiles, int y_tiles, int *active_tiles, int **core_count, unsigned int addr, unsigned int value)
{
    int cm3_count;
    for (int y=0; y<y_tiles; y=y+1) {
        switch(y) {
            case 0: 
                CHAIN_SELECT = 1; 
                CHAIN_SELECT_1 = 0;
                break;
            case 1: 
                CHAIN_SELECT = 0; 
                CHAIN_SELECT_1 = 1; 
                break;
            default:
                CHAIN_SELECT = 0;
                CHAIN_SELECT_1 = 0;
                pc.printf("[loopMemWrite] bad CHAIN_SELECT: curr_tile_y = %d\r\n", y);
                break;
        }
        //cm3_count = core_count * active_tiles[y];
        cm3_count = 0;
        int curr_active_tiles = active_tiles[y];
        for (int x=0; x<curr_active_tiles; x++) {
            cm3_count += core_count[x][y];
        }
        for (int x=0; x<x_tiles; x=x+1) {
            int curr_tile_cores = core_count[x][y];
            for (int i=0; i<curr_tile_cores; i++) {
                pc.printf("[loopMemWrite] write to core(%d) in tile(%d,%d) addr:%x, value:%x  ***********\r\n", i, x, y, addr, value);
                memAccess(false, false, x, i, addr, value, 1, true, y, active_tiles, x_tiles, cm3_count, core_count);
            }
            //memAccess(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int RW, bool halt, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int core_count)
            tckTicks(150);
        }
    }
    pc.printf("[loopMemWrite] finished****************\r\n");
    return 0;
}

/*
 * Function Name: tileUnloop
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][]
 * Description: This function write to a config reg and 
 *              returns the updated active_tiles number of the current chain
 * Comments: This function is basically a very specified version of 
 *           memAccess, which do a broadcast write to the last tile of the 
 *           current "visible" chain.
 */
int tileUnloop(int active_tiles, int curr_tile_x, int curr_tile_y, bool last_tile)
{
    //DigitalOut FINISH_FLAG (LED3);
    //_USB_CONSOLE.baud(9600);

    _USB_CONSOLE.printf("[tileUnloop]** curr_tile_x = %d\r\n", curr_tile_x);

    SAMEPROG = 1;
    TILE_TEST = 1;
    int same_prog_tmp = 1;
    int num_cores = active_tiles;
    int *core_op;
    core_op = new int[num_cores];
    for (int i=0; i<num_cores; i++) {//(curr_tile_x+1)
        /*if (i == num_cores-1) core_op[i] = 1;
        else core_op[i] = 0;*/
        if (last_tile) {
            if (i == num_cores-1) {
                core_op[i] = 1;
            } else {
                core_op[i] = 0;
            }
        } else {
            core_op[i] = 1;
        }
    }

    JTAG* _JTAG = new JTAG;
    _JTAG->setCurrTileID(curr_tile_y);
    _JTAG->setNumCores(num_cores);
    _JTAG->setNumTiles(active_tiles);

    
    for (int k=0; k<num_cores; k=k+1) {
        if (core_op[k]) _JTAG->setCore(k);
    }

    _JTAG->getConfig();
    pc.printf("[JTAG config] SAMEPROG = %d\r\n", same_prog_tmp);
    
    pc.printf("\r\n [tileUnloop]** Verify Chip ID...\r\n");
    int idcode = _JTAG->readID();
    pc.printf("IDCODE value is: %x \r\n", idcode);
    if(idcode != 0x4ba00477) {//0x4ba00477
        pc.printf("ERROR: IDCode %X, exiting program.\r\n", idcode);
        while(1) wait(2);
        //power_down();
        //return -1;
    }
    pc.printf("IDCode %X\r\n", idcode);
    

    _JTAG->reset();
    _JTAG->leaveState();
    //_JTAG->PowerupDAP();
    while (_JTAG->PowerupDAP() != 0) {
        _JTAG->reset();
        _JTAG->leaveState();
        while(1) wait(10);
    }

    if (last_tile) {
        pc.printf("[tileUnloop]** writing 0 to tile(%d,%d) reg[15][0]\r\n", curr_tile_x, curr_tile_y);
        _JTAG->writeMemory(0x4000003C, 0x00000000);  
        //memAccess(false, true, 0, 0, addr, data, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    } else {
        pc.printf("[tileUnloop]** writing 1 to tile(%d,%d) reg[15][0]\r\n", curr_tile_x, curr_tile_y);   
        _JTAG->writeMemory(0x4000003C, 0x00000001);
        //_JTAG->writeMemory(0x40000034, 0x00000800);
    }
    //delete _JTAG;
    
    /*
    _JTAG->reset(); //SP
    _JTAG->leaveState();
    
    _JTAG->setState('i');
    
    TDI = 1;
    //_JTAG->clockTicks(128);
    
    for (int ck=0; ck<16; ck++)
    {
        int tdo_test = TDO;
        int tdi_test = TDI;
        _JTAG->clockTicks(1);
        pc.printf("TDI: %d TDO: %d \r\n", tdi_test, tdo_test);
        }
        
    //int output = _JTAG->shiftBits(JTAG_BYPASS, 4);
    //pc.printf("Output1: %x\r\n", output);
    
    //output = _JTAG->shiftBits(JTAG_BYPASS, 4);
    //pc.printf("Output2: %x\r\n", output);
    
    //unsigned int id = _JTAG->shiftBits(JTAG_BYPASS_2, 32);
    //pc.printf("ID: %d", id);
    _JTAG->leaveState();
    
    _JTAG->setState('d');
    
    for(int i=0; i<16; i++)
    {
        int tmp_tdo = TDO;
        int tmp_tdi = TDI;
        pc.printf("Value of TDI: %d TDO: %d \t", tmp_tdi, tmp_tdo);
        pc.printf("Iteration: %d\r\n", i);
        
        _JTAG->clockTicks(1);
        
        if ( (i % 5) == 0) {
                _JTAG->DataHigh();
        } else {
                _JTAG->DataLow();
        }
    }
    */
    //while(1) {wait(10);}
    /*num_cores = active_tiles + 1;
    delete [] core_op;
    core_op = new int[num_cores];
    for (int i=0; i<num_cores; i++) {
        core_op[i] = 1;
    }
    _JTAG = new JTAG;
    _JTAG->setCurrTileID(curr_tile_y);
    _JTAG->setNumCores(num_cores);
    for (int k=0; k<num_cores; k=k+1) {
        if (core_op[k]) _JTAG->setCore(k);
    }
    _JTAG->getConfig();

    _JTAG->reset();
    _JTAG->leaveState();
    //_JTAG->PowerupDAP();
    while (_JTAG->PowerupDAP() != 0) {
        _JTAG->reset();
        _JTAG->leaveState();
    }*/
    /*_JTAG->writeMemory(0x40000000, 0xAAAAAAAA);
    tckTicks(150);
    _JTAG->readMemory(0x40000000);*/

    delete _JTAG;
    delete [] core_op;

    //for (int x=0; x<num_cores; x=x+1) {
        /*core_op = new int[num_cores];
        for (int i=0; i<num_cores; i++) {
            //if (i>=x) core_op[i] = 1;
            if (i==x) core_op[i] = 1;
            else core_op[i] = 0;
        }
        _JTAG = new JTAG;
        _JTAG->setCurrTileID(curr_tile_y);
        _JTAG->setNumCores(num_cores);
        for (int k=0; k<num_cores; k=k+1) {
            if (core_op[k]) _JTAG->setCore(k);
        }
        _JTAG->getConfig();
        _JTAG->reset();
        _JTAG->leaveState();
        while (_JTAG->PowerupDAP() != 0) {
            _JTAG->reset();
            _JTAG->leaveState();
        }*/
        /*_JTAG->writeMemory(0x40000000, 0xAAAAAAAA);
        tckTicks(150);
        unsigned int read_val = _JTAG->readMemory(0x40000000);
        if (read_val != 0xAAAAAAAA) {
            pc.printf("[tileUnloop]** tile (%d,%d) reg[0]: %x != 0xAAAAAAAA\r\n", x, curr_tile_y, read_val);
        }
        _JTAG->writeMemory(0x40000000, 0xFFFFFFFF);
        tckTicks(150);*/
        /*unsigned int reg_unloop = _JTAG->readMemory(0x4000003C);
        if (x < num_cores - 1) {
            if (reg_unloop != 1) pc.printf("[tileUnloop]** tile (%d,%d) reg[15]: %x != 1\r\n", x, curr_tile_y, reg_unloop);
            else pc.printf("[tileUnloop]** tile (%d,%d) reg[15]: %x = 1\r\n", x, curr_tile_y, reg_unloop);
        } else {
            if (reg_unloop != 0) pc.printf("[tileUnloop]** tile (%d,%d) reg[15]: %x != 0\r\n", x, curr_tile_y, reg_unloop);
            else pc.printf("[tileUnloop]** tile (%d,%d) reg[15]: %x = 0\r\n", x, curr_tile_y, reg_unloop);
        }*/
    //}

    /*int value = _JTAG->readMemory(0x4000003C);
    if (value != 1) {
        pc.printf("[tileUnloop]** curr_tile_x = %d FAIL, value = %x\r\n", curr_tile_x, value);
        wait(2);
    }*/
    if (last_tile) {
        return active_tiles;
    } else {
        return active_tiles + 1;
    }
}

/*
 * Function Name: unloopAll
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][]
 * Description: loop through all the chains and unloop the chains
 * Comments: After initial power-on reset, all the tiles are in loop-back
 *           mode. In order to access all the tiles in a chain, unlooping
 *           is needed by writing to a config reg in the last tile of 
 *           the current "visible" chain.
 *           The outer loop goes through all the chain, 
 *           the inner loop goes throught all the tiles in one chain,
 *           Then call memAccess to write to the config reg.
 *           active_tiles array is updated through this process.
 */
int unloopAll(int x_tiles, int y_tiles, int *active_tiles, int **core_count)
{
    for (int y=0; y<y_tiles; y=y+1) {
        switch(y) {
            case 0: 
                CHAIN_SELECT = 1; 
                CHAIN_SELECT_1 = 0;
                break;
            case 1: 
                CHAIN_SELECT = 0; 
                CHAIN_SELECT_1 = 1; 
                break;
            default:
                CHAIN_SELECT = 0;
                CHAIN_SELECT_1 = 0;
                pc.printf("[unloopAll] bad CHAIN_SELECT: curr_tile_y = %d\r\n", y);
                break;
        }
        //for (int x=0; x<x_tiles-1; x=x+1) {
        for (int x=0; x<x_tiles; x=x+1) {
            if (x < x_tiles-1) {
                pc.printf("[unloopAll] unloop tile x:%d y:%d ************\r\n", x, y);
                
                
                int new_active_tiles = tileUnloop(active_tiles[y], x, y, false);
                
                if (new_active_tiles != active_tiles[y] + 1) {
                    pc.printf("[unloopAll] unloop tile x:%d y:%d failed, new_active_tiles = %d\r\n", x, y, new_active_tiles);
                }
                active_tiles[y] = active_tiles[y] + 1;
                tckTicks(4096);
            } else {
                /* this else branch is technically not needed */
                int new_active_tiles = tileUnloop(active_tiles[y], x, y, true);
                if (new_active_tiles != active_tiles[y]) {
                    pc.printf("[unloopAll] unloop tile x:%d y:%d failed, new_active_tiles = %d\r\n", x, y, new_active_tiles);
                }
                active_tiles[y] = active_tiles[y];
                tckTicks(4096);
            }
        }
    }
    pc.printf("[unloopAll] finished****************\r\n");
    return 0;
}

/*
 * Function Name: program_IDs
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][]
 * Description: loop through all the tiles and program tile ID
 * Comments: The outer loop goes through all the chain, 
 *           the inner loop goes throught all the tiles in one chain.
 *           Then call memAccess to write tile ID into the config reg.
 */
int program_IDs(int x_tiles, int y_tiles, int *active_tiles, int **core_count)
{
    int cm3_count;
    for (int y=0; y<y_tiles; y=y+1) {
        switch(y) {
            case 0: 
                CHAIN_SELECT = 1; 
                CHAIN_SELECT_1 = 0;
                break;
            case 1: 
                CHAIN_SELECT = 0; 
                CHAIN_SELECT_1 = 1; 
                break;
            default:
                CHAIN_SELECT = 0;
                CHAIN_SELECT_1 = 0;
                pc.printf("[programIDs] bad CHAIN_SELECT: curr_tile_y = %d\r\n", y);
                break;
        }
        //cm3_count = core_count * active_tiles[y];
        cm3_count = 0;
        int curr_active_tiles = active_tiles[y];
        for (int x=0; x<curr_active_tiles; x++) {
            cm3_count += core_count[x][y];
        }
        for (int x=0; x<x_tiles; x=x+1) {
        //for (int x=x_tiles-1; x>=0; x=x-1) {
            pc.printf("[programIDs] write ID x:%d y:%d***********\r\n", x, y);
            unsigned int tile_id = 0x80000000 + (x << 16) + y;
            //unsigned int tile_id = 0x24770011 + (x << 16) + y;
            memAccess(false, true, x, 0, 0x40000040, tile_id, 1, false, y, active_tiles, x_tiles, cm3_count, core_count);
            /*while (reg_write(false, true, x, 0, 0x40000040, tile_id, y, active_tiles, x_tiles, cm3_count, core_count) != 0); {
                tckTicks(150);
            }*/
            //memAccess(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int RW, bool halt, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int core_count)
            tckTicks(150);
        }
    }
    pc.printf("[program_IDs] finished****************\r\n");
    return 0;
}

/*
 * Function Name: program_PLLs
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][]
 * Description: loop through all the tiles and program tile ID
 * Comments: The outer loop goes through all the chain, 
 *           the inner loop goes throught all the tiles in one chain.
 *           Then call memAccess to write tile ID into the config reg.
 */
int program_PLLs(int x_tiles, int y_tiles, int *active_tiles, int **core_count)
{
    int cm3_count;
    for (int y=0; y<y_tiles; y=y+1) {
        /*switch(y) {
            case 0: 
                CHAIN_SELECT = 1; 
                CHAIN_SELECT_1 = 0;
                break;
            case 1: 
                CHAIN_SELECT = 0; 
                CHAIN_SELECT_1 = 1; 
                break;
            default:
                CHAIN_SELECT = 0;
                CHAIN_SELECT_1 = 0;
                pc.printf("[PLLConfig] bad CHAIN_SELECT: curr_tile_y = %d\r\n", y);
                break;
        }*/
        CHAIN_SELECT = 1;
        //cm3_count = core_count * active_tiles[y];
        cm3_count = 0;
        int curr_active_tiles = active_tiles[y];
        for (int x=0; x<curr_active_tiles; x++) {
            cm3_count += core_count[x][y];
        }
        for (int x=0; x<x_tiles; x=x+1) {
        //for (int x=x_tiles-1; x>=0; x=x-1) {
            pc.printf("[PLL Config Reg] write PLL Config x:%d y:%d***********\r\n", x, y);
            unsigned int pll_config_reg = 0x102E9;
            memAccess(false, true, x, 0, 0x40000028, pll_config_reg, 1, false, y, active_tiles, x_tiles, cm3_count, core_count);
            wait_us(2);
            pll_config_reg = 0x102EB;
            memAccess(false, true, x, 0, 0x40000028, pll_config_reg, 1, false, y, active_tiles, x_tiles, cm3_count, core_count);
            
            pc.printf("[PLL Config] Checking if PLL Locked at address 0x400000A4\r\n");
            memAccess(false, true, x, 0, 0x400000A4, pll_config_reg, 0, false, y, active_tiles, x_tiles, cm3_count, core_count);
            /*while (reg_write(false, true, x, 0, 0x40000040, tile_id, y, active_tiles, x_tiles, cm3_count, core_count) != 0); {
                tckTicks(150);
            }*/
            //memAccess(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int RW, bool halt, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int core_count)
            tckTicks(150);
        }
    }
    pc.printf("[program_PLLs] finished****************\r\n");
    return 0;
}




/*
 * Function Name: elf_loader_norun
 * Arguments: broadcast, whole_tile, block_tile, tile_core, curr_tile_y,
 *            active_tiles[], x_tiles, cm3_count, core_count[][]
 * Description: This function does similar JTAG object setup as memAccess,
 *              then wipe memory and call loadELF
 * Comments: Very similar structure to memAccess, except after setting up
 *           the JTAG object, do a sequence of memory operations that are
 *           essential to program loading (loadELF)
 */
int elf_loader_norun(bool broadcast, bool whole_tile, int block_tile, int tile_core, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int **core_count)
{
    //DigitalOut FINISH_FLAG (LED3);
    //_USB_CONSOLE.baud(9600);
    
    //////////////////////////////// Perhaps change this to 80 characters width in the future for standard terminal output
    _USB_CONSOLE.printf("****************************************************************\r\n");
    _USB_CONSOLE.printf("************** Starting Waferscale_Prototype_ELF_Loader **************\r\n");
    _USB_CONSOLE.printf("********** UCLA NanoCAD Lab, www.nanocad.ee.ucla.edu *****************\r\n");
    _USB_CONSOLE.printf("********** UIUC Passat Reseach Group *********************************\r\n");
    _USB_CONSOLE.printf("****************************************************************\r\n\r\n");
    int num_cores;
    int same_prog_tmp;
    
    if (broadcast) {
        SAMEPROG = 1;
        same_prog_tmp = 1;
        num_cores = active_tiles[curr_tile_y];
        pc.printf("Broadcast to whole chain, curr_tile_y = %d\r\n", curr_tile_y);
    } else if (whole_tile) {
        SAMEPROG = 1;
        same_prog_tmp = 1;
        num_cores = active_tiles[curr_tile_y];
        pc.printf("Broadcast to a tile, x:%d y:%d\r\n", block_tile, curr_tile_y);
    } else {
        SAMEPROG = 0;
        same_prog_tmp = 0;
        num_cores = cm3_count;
        pc.printf("To a single core: x:%d y:%d core:%d\r\n", block_tile, curr_tile_y, tile_core);
    }

    int core_op[num_cores];
    int core_op_loc = 0;

    if (broadcast) {
        for (int i=0; i<num_cores; i++) {
            core_op[i] = 1;
        }
    } else if (whole_tile) {
        for (int i=0; i<num_cores; i++) {
            if (i==block_tile) core_op[i] = 1;
            else core_op[i] = 0;
        }
    } else {
        for (int x=0; x<block_tile; x++) {
            core_op_loc += core_count[x][curr_tile_y];
        }
        core_op_loc += tile_core;
        for (int i=0; i<num_cores; i++) {
            //if (i==(block_tile*core_count + tile_core)) {
            if (i == core_op_loc) {
                core_op[i] = 1;
            } else {
                core_op[i] = 0;
            }
        }
    }

    ////////////////////////////////
    /* Reset the chip */
    _USB_CONSOLE.printf("** Resetting test chip...\r\n");
    
    CORERESETn = 0;
    tckTicks(4096);
    CORERESETn = 1;
    tckTicks(4096);
    
    

    JTAG* _JTAG = new JTAG;
    _JTAG->setCurrTileID(curr_tile_y);
    _JTAG->setNumCores(num_cores);
    _JTAG->setNumTiles(active_tiles[curr_tile_y]);

    for (int k=0; k<num_cores; k=k+1) {
        if (core_op[k]) _JTAG->setCore(k);
    }

    _JTAG->getConfig();
    pc.printf("[JTAG config] SAMEPROG = %d\r\n", same_prog_tmp);

    pc.printf("[elfLoading]** Verify Chip ID...\r\n");
    int idcode = _JTAG->readID();
    if(idcode != 0x4ba00477) {//0x4ba00477
        pc.printf("ERROR: IDCode %X, exiting program.\r\n", idcode);
        wait(2);
        //power_down();
        delete _JTAG;
        return -1;
    }
    pc.printf("IDCode %X\r\n", idcode);
    
    _JTAG->reset();
    _JTAG->leaveState();
    //_JTAG->PowerupDAP();
    pc.printf("Powering up DAP\r\n");
    while (_JTAG->PowerupDAP() != 0) {
        _JTAG->reset();
        _JTAG->leaveState();
    }
   //IA commenting out
    if (_JTAG->haltCore() != 0) {
        pc.printf("ERROR: Could not halt core, exiting program.\r\n");
        delete _JTAG;
        return -1;
    }
    pc.printf("core halted.\r\n");

    /* Poll until newline is pressed so that filesystem is open for copying of file. */
    /*char c;
    printf("Press spacebar in your terminal once ELF has been loaded into the mbed's flash.\r\n");
    while(1) {
        c = pc.getc();      
        if (c == 0x20) {
            break;
        }
    }
    pc.printf("\r\n");*/

   
    // the following loop is just for read testing
    /*for (int k=0; k<num_cores; k=k+1) {
        _JTAG->setCore(k);
    }
    _JTAG->getConfig();*/
    
    /* Set IMEM offset to 0x0 */
    if (_JTAG->zeroImemOffset() != 0) {
        pc.printf("ERROR: Could not set Instruction Memory to 0x00 offset, exiting program. \r\n");
        wait(2);
        power_down();
        return -1;
    }
    // the following loop is just for read testing
    /*for (int k=0; k<num_cores; k=k+1) {
        _JTAG->setCore(k);
    }
    _JTAG->getConfig();*/

    
    /* Wipe Memory */
    //IA commenting out wiping memory
    /*_USB_CONSOLE.printf("Wiping IMEM...\r\n");
    _JTAG->wipeMemRange(IMEM_MIN, IMEM_MIN + 0x00002000 - 0x00000004);
    _USB_CONSOLE.printf("Wiping DMEM...\r\n");
    _JTAG->wipeMemRange(DMEM_MIN, DMEM_MIN + 0x00004000 - 0x00000004);
    //_USB_CONSOLE.printf("Wiping sys mem...\r\n");
    //_JTAG->wipeMemRange(0x60000000, 0x8FFFFFFF - 0x00000004);//0x9FFFFFFF
    _USB_CONSOLE.printf("Done wiping memory.\r\n\r\n");*/
   /**** IA****/
    //pc.printf("Entering PowerupDAP\r\n");
    //_JTAG->PowerupDAP();//??

    //if (_JTAG->haltCore() != 0) {
    //    pc.printf("ERROR: Could not halt core, exiting program.\r\n");
    //    delete _JTAG;
    //    return -1;
    //}
    //pc.printf("core halted.\r\n");

    //////////////////////
    /* setup elf path and call loadELF */
    char path[128];
    if (broadcast) {
        snprintf(path, 128, "/local/H.elf");
    } else if (whole_tile) {
        snprintf(path, 128, "/local/H.elf");
    } else {
        //snprintf(path, 128, "/local/hello_%d%d.elf", core_op_loc, curr_tile_y);
        snprintf(path, 128, "/local/H.elf"); //IA
    }
    pc.printf("Loading file %s\r\n", path);
    if (_JTAG->loadElf(path) != 0) {
        pc.printf("ERROR: Could not load ELF file\r\n");
    }
    if (_JTAG->haltCore() != 0) {
        pc.printf("ERROR: Could not halt core, exiting program.\r\n");
        delete _JTAG;
        return -1;
    }
    pc.printf("post loadELF, core halted.\r\n");

    pc.printf("CORERESETn = 0\r\n");
    //CORERESETn = 0;
    // load next chain so that this chain won't run
    tckTicks(4096);
    
    delete _JTAG;
    delete [] core_op;
    return 0;
}

/*
 * Function Name: loopElfLoader
 * Arguments: x_tiles, y_tiles, active_tiles[], core_count[][]
 * Description: loop through all the chains and load programs to all cores
 * Comments: The outer loop goes through all the chain, 
 *           the inner loop goes throught all the tiles and cores in one chain.
 *           The API for calling elf_loader_norun is very similar to memAccess.
 *           Right now it only supports one mode on loading, but can be easily
 *           modified if needed.
 */
int loopElfLoader(int x_tiles, int y_tiles, int *active_tiles, int **core_count)
{
    int cm3_count;
    
    for (int y=0; y<y_tiles; y=y+1) {
        switch(y) {
            case 0: 
                CHAIN_SELECT = 1; 
                CHAIN_SELECT_1 = 0;
                break;
            case 1: 
                CHAIN_SELECT = 0; 
                CHAIN_SELECT_1 = 1; 
                break;
            default:
                CHAIN_SELECT = 0;
                CHAIN_SELECT_1 = 0;
                pc.printf("[loopElfLoader] bad CHAIN_SELECT: curr_tile_y = %d\r\n", y);
                break;
        }
        //cm3_count = core_count * active_tiles[y];
        cm3_count = 0;
        int curr_active_tiles = active_tiles[y];
        for (int x=0; x<curr_active_tiles; x++) {
            cm3_count += core_count[x][y];
        }
        for (int x=0; x<x_tiles; x=x+1) {
            //for (int k=0; k<core_count[x][y]; k++){
                pc.printf("[loopElfLoader] loading core %d @ tile(%d,%d)***********\r\n", 0, x, y);
                //elf_loader_norun(false, false, x, k, y, active_tiles, x_tiles, cm3_count, core_count);
                elf_loader_norun(false, true, x, 0, y, active_tiles, x_tiles, cm3_count, core_count); //IA
            //}
            //elf_loader_norun(false, true, x, 0, y, active_tiles, x_tiles, cm3_count, core_count);
            /*while (reg_write(false, true, x, 0, 0x40000040, tile_id, y, active_tiles, x_tiles, cm3_count, core_count) != 0); {
                tckTicks(150);
            }*/
            //memAccess(bool broadcast, bool whole_tile, int block_tile, int tile_core, unsigned int addr, unsigned int value, int RW, bool halt, int curr_tile_y, int *active_tiles, int x_tiles, int cm3_count, int core_count)
            tckTicks(150);
        }
    }
    pc.printf("[loopElfLoader] finished****************\r\n");
    
    char c;
    printf("Press spacebar in your terminal once ELF has been loaded into the mbed's flash.\r\n");
    while(1) {
        c = pc.getc();      
        if (c == 0x20) {
            break;
        }
    }
    pc.printf("\r\n");
    
    return 0;
}

/*
int validateElfHeader(Elf32_Ehdr &elfheader)
{
    //assumption is that endianness is the same?
    
    if ((elfheader.e_ident[EI_MAG0] != ELFMAG0) || (elfheader.e_ident[EI_MAG1] != ELFMAG1)  || (elfheader.e_ident[EI_MAG2] != ELFMAG2) || (elfheader.e_ident[EI_MAG3] != ELFMAG3)) {
        pc.printf("Magic number not found. This may not be an ELF program.\r\n");
        return -1;
    }
    
    if (elfheader.e_ident[EI_CLASS] != ELFCLASS32) {
        pc.printf("This is not a 32-bit ELF file. Cannot process. \r\n");
        return -1;
    }
    
    if (elfheader.e_ident[EI_DATA] == ELFDATA2LSB) {
        pc.printf("This is a little endian ELF file. This mbed program will only run if this ELF file is little endian, but makes no guarantees that endianness is correct. \r\n");
    }
    else if (elfheader.e_ident[EI_DATA] == ELFDATA2MSB) {
        pc.printf("This is a big endian ELF file. Cannot process. \r\n");
        return -1;
    }
    
    if (elfheader.e_machine != EM_ARM) { 
        pc.printf("This is not an ARM machine. Cannot process. \r\n");
        return -1;
    }
    
    return 0;   
}*/

/*
 * Function Name: main
 * Arguments: none/void
 * Description: The main function consists of the power-on routine, 
 *              and mid/post program execution controls/accesses
 * Comments: The power-on rountine includes the following in order
 *           1) Control signal initialization and initial power-on reset
 *           2) Getting tile configuration, which is connected to this 
 *              software, from the user. The inputs required are: 
 *              number of tiles in x, number of tiles in y, and
 *              number of cores in each tile
 *           3) tile unlooping, program tile IDs, program loader, 
 *              clocking configuration (PLL and clock forwarding), 
 *              and other configuration if needed
 *           4) Clocking resets, COREreset for all tiles to start 
 *              the programs
 *           5) mid/post program execution operations if needed
 */
int main() {
    pc.printf("[mBED] main() started ****\r\n");

    /*
    POR    = 0;
    SEL    = 0; //1: BG_REF | 0: Diode
    EN     = 1;
    TUNE   = 0;
    LDO_EN = 0; //Active Low
    
    while(1) {wait(1);}
    */

    TILE_TEST = 0;
    PORESETn = 1;
    CORERESETn = 1;
    TCK_EN = 0;
    CLK_RSTn = 1;
    TDI = 0;
    TCK = 0;
    TMS = 0;
    SAMEPROG = 0;

    CHAIN_SELECT = 1;
    SAMEPROG = 1;
    TILE_TEST = 1;

    PORESETn = 0;
    tckTicks(1024);//ensure synchronous reset
    PORESETn = 1;
    tckTicks(1024);//ensure synchronous reset
    
    CORERESETn = 0;
    tckTicks(1024);//ensure synchronous reset
    CORERESETn = 1;
    tckTicks(1024);//ensure synchronous reset



    pc.printf("Control signals initalized, getting config...\r\n");

    pc.printf("Type in number of tile in x\r\n");
    int x_tiles = 1; //SP getNum();
    pc.printf("Type in number of tile in y\r\n");
    int y_tiles = 1; //SP getNum();

    /*pc.printf("Type in number of cores in each tile\r\n");
    int core_count = getNum();*/
    int **core_count;
    core_count = new int *[x_tiles];
    for (int x=0; x<x_tiles; x++) {
        core_count[x] = new int[y_tiles];
    }
    
    for (int y=0; y<y_tiles; y++) {
        for (int x=0; x<x_tiles; x++) {
            //pc.printf("Type in the number of cores in tile(%d,%d)\r\n", x, y);
            //core_count[x][y] = getNum();
            pc.printf("Type in the number of cores MSB in tile(%d,%d)\r\n", x, y);
            int msb = 1;  //IA getNum();
            pc.printf("Type in the number of cores LSB in tile(%d,%d)\r\n", x, y);
            int lsb = 4; //IA getNum();
            core_count[x][y] = msb*10+lsb;
            pc.printf("Number of cores: %d\r\n", core_count[x][y]);
        }
    }

    /*
    /////JTAG TEST CODE [SP]
    JTAG* _JTAG = new JTAG;
    _JTAG->reset();
    _JTAG->setCurrTileID(0);
    _JTAG->setNumCores(1);
    _JTAG->setNumTiles(1);
    _JTAG->core_op[0] = 1;

    for (int k=0; k<1; k=k+1) {
        _JTAG->setCore(k);
    }

    _JTAG->getConfig();
    _JTAG->reset();
    _JTAG->leaveState();
    _JTAG->PowerupDAP();
    _JTAG->readID();
    //_JTAG->bypass_state();
    
    
    _JTAG->reset(); //SP
    _JTAG->leaveState();
    
    _JTAG->setState('i');
    
    TDI = 1;
    //_JTAG->clockTicks(128);
    for (int ck=0; ck<8; ck++)
    {
        int tdo_test = TDO;
        int tdi_test = TDI;
        _JTAG->clockTicks(1);
        pc.printf("TDI: %d TDO: %d \r\n", tdi_test, tdo_test);
        }
    
    //unsigned int id = _JTAG->shiftBits(JTAG_BYPASS_2, 600);
    //pc.printf("ID: %d", id);
    _JTAG->leaveState();
    
    _JTAG->setState('d');
    _JTAG->clockLow();
    
    for(int i=0; i<16; i++)
    {
        int tmp_tdo = TDO;
        int tmp_tdi = TDI;
        pc.printf("Value of TDI: %d TDO: %d \t", tmp_tdi, tmp_tdo);
        pc.printf("Iteration: %d\r\n", i);
        
        tckTicks(1);
        
        if ( (i % 4) == 0) {
                _JTAG->DataHigh();
        } else {
                _JTAG->DataLow();
        }
    }
    */

    //while(1) {wait(10);}

    //int cm3_max = core_count * x_tiles;

    int active_tiles[y_tiles];
    for (int i=0; i<y_tiles; i++) {
        active_tiles[i] = 1;
    }

    TILE_TEST = 1;

    CORERESETn = 0;

    tckTicks(4096);//ensure synchronous reset
    
    CORERESETn = 0;

    tckTicks(4096);//ensure synchronous reset

    DigitalOut FINISH_FLAG (LED3);
    _USB_CONSOLE.baud(9600);

    /* elfLoader sanity check */
    /*JTAG* _JTAG = new JTAG;
    _JTAG->setCurrTileID(0);
    _JTAG->setNumCores(1);
    _JTAG->setNumTiles(1);

    for (int k=0; k<1; k=k+1) {
        _JTAG->setCore(k);
    }*/
    ////////OPEN THE PROGRAM FILE////////////
    char path[64];
    snprintf(path, 64, "/local/H.elf");
    
    FILE *fp_0 = fopen(path, "rb"); //open as read binary mode
    if (fp_0 == NULL) {
        pc.printf("Could not open %s . Exiting. \r\n", path);
        return -1;
    }
    /////////ELF HEADER PROCESSING/////////////
    
    //load ELF header
    pc.printf("[JL] Begin loading the ELF header. \r\n");
    if (fseek(fp_0, 0, SEEK_SET) != 0) {
        pc.printf("Could not move to start of ELF header. Exiting. \r\n");
    }
    Elf32_Ehdr elfheader_0;
    if (fread((void *)&elfheader_0, sizeof(elfheader_0), 1, (FILE *)fp_0) != 1) { //number of elements successfully read is returned
        pc.printf("Could not read the ELF header. Exiting. \r\n");
    }
    pc.printf("[JL] fread succeeded. \r\n");
    /*if (validateElfHeader(elfheader) != 0) {
        pc.printf("ELF Header had an invalid field; please validate the ELF file. Exiting. \r\n");
        return -1;
    }
    pc.printf("ELF header is valid! Going on to load all the program headers. \r\n\r\n");

    //////////PROGRAM HEADER PROCESSING //////////
    
    //find program headers
    uint32_t phdr_offset = elfheader.e_phoff;   //elf offset that program header begins
    uint32_t phdr_num = elfheader.e_phnum;      //how many program headers there are, loop through these
    uint32_t phdr_sz = elfheader.e_phentsize;   //size of each program header entry

    //loop through all program headers, and if program header is PT_LOAD then write it into memory
    Elf32_Phdr progheader;
    pc.printf("number of program headers: %d\r\n", phdr_num);
    for (int i = 0; i < phdr_num; i++) {
        //move to next program header
        if (fseek(fp, i * phdr_sz + phdr_offset, SEEK_SET) != 0) {
            pc.printf("Could not move to program header number %d. Exiting. \r\n", i);
            return -1;
        }
        
        if (fread((char *)&progheader, sizeof(progheader), 1, (FILE *)fp) != 1) { //reads that particular program header
            pc.printf("Could not read in a program header block at offset %x. Exiting. \r\n", i * phdr_sz + phdr_offset);
            return -1;
        }
        
        else{
            pc.printf("Size of program header: %x \n", sizeof(progheader));
        }
        
        if (progheader.p_type == PT_LOAD) { //load segment corresponding to program header
            pc.printf("Encountered loadable segment number %d. \r\n", i);
            //loadSegment(progheader, fp);
        }
        else { //don't load segment corresponding to program header
            pc.printf("Encountered non-loadable segment number %d. Moving on to next segment.\r\n", i);
        }
        
    }
    pc.printf("Successfully loaded all loadable segments into memory. \r\n");

    delete _JTAG;*/
    fclose(fp_0);
    
    char c;
    printf("Press spacebar to continue\r\n");
    while(1) {
        
        c = pc.getc();      
        if (c == 0x20) {
            break;
        }
    }
    pc.printf("\r\n");
    

    //loopRead(1, y_tiles, active_tiles, core_count, 0x4000003C);
    
    //while(1) tckTicks(1000);

    
    unloopAll(x_tiles, y_tiles, active_tiles, core_count);

    for (int i=0; i<y_tiles; i++) {
        pc.printf("active_tiles[%d] = %d\r\n", i, active_tiles[i]);
    }
    
    printf("Press spacebar to continue\r\n");
    while(1) {
        c = pc.getc();      
        if (c == 0x20) {
            break;
        }
    }
    pc.printf("\r\n");
    
    //loopRead(x_tiles, y_tiles, active_tiles, core_count, 0x4000003C);

    tckTicks(4096);//ensure synchronous reset

    program_IDs(x_tiles, y_tiles, active_tiles, core_count);
    
    pc.printf("********Done with program ID routine********* \r\n");
    
    unsigned int addr = 0x40000004;
    unsigned int clk_fwd_config= 0x1FF0000F;
    pc.printf("[Pre Execution] Setting Clock Forwarding Config to Addr: %x \r\n", addr);
    memAccess(false, true, 0, 0, addr, clk_fwd_config, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);

    addr = 0x40000034;
    clk_fwd_config= 0x00000001;
    pc.printf("[Pre Execution] Setting Clock Forwarding Config to Addr: %x \r\n", addr);
    memAccess(false, true, 0, 0, addr, clk_fwd_config, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    
    
    addr = 0x4000003C;
    pc.printf("[Pre Execution] Checking JTAG Forwarding Config at Reg addr: %x \r\n", addr);
    memAccess(false, true, 0, 0, addr, 0x0, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    //memAccess(false, true, 0, 0, addr, 0x00000001, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    //memAccess(false, true, 0, 0, addr, 0x00000001, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    memAccess(false, true, 0, 0, addr, 0x00000000, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);

    program_PLLs(x_tiles, y_tiles, active_tiles, core_count);   

    wait_us(100);
    TILE_TEST = 1;
    wait_us(1000);
    CLK_RSTn = 1;
    wait_us(1000);
    CLK_RSTn = 0;
    wait_us(1000);
    CLK_RSTn = 1;
    wait_us(100);

    memAccess(false, true, 0, 0, addr, clk_fwd_config, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    
    TILE_TEST = 1;
    tckTicks(1000);
    
    memAccess(false, true, 0, 0, addr, clk_fwd_config, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    

    
    printf("Press spacebar to continue\r\n");
    while(1) {
        c = pc.getc();      
        if (c == 0x20) {
            break;
        }
    }
    pc.printf("\r\n");

    //tckTicks(4096);
    //loopRead(x_tiles, y_tiles, active_tiles, core_count, 0x40000040);//0x40000040
    //tckTicks(4096);
    //loopRead(x_tiles, y_tiles, active_tiles, core_count, 0x40000040);

    //loopMemWrite(x_tiles, y_tiles, active_tiles, core_count, 0x00000004, 0x600DBEEF);

    tckTicks(4096);

    TILE_TEST = 1; //IA
    CORERESETn = 0; //Added Core reset to negate the effect of changing it to 1 earlier IA
    tckTicks(4096);

    loopElfLoader(x_tiles, y_tiles, active_tiles, core_count);
    
    memAccess(false, true, 0, 0, 0x40000000, 0x9, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    
    addr = 0x40000000;
    pc.printf("[Pre Execution] Reading Addr: %x ", addr);
    memAccess(false, true, 0, 0, addr, 0x9, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    
    // [SP] : This is the register to stub network interfaces
    addr = 0x40000030;
    pc.printf("[Pre Execution] Writing Addr: %x ", addr);
    memAccess(false, true, 0, 0, addr, 0x0000, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    
    /*
    for (int i = 0; i < 4; i++) {
        addr = 0x20000000 + i*4;
        unsigned int data = i*2;
        pc.printf("[Pre Execution] Writing Addr: %x with Data: %x ", addr, data);
         memAccess(false, true, 0, 0, addr, data, 1, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
        }
    */
    
    CORERESETn = 0;
    if (TILE_TEST){
        tckTicks(10);
    }
    else{
        wait_us(0.1);
    }
    TILE_TEST = 1; //IA Reverting tile test to 0 just before execution
    
    
    CORERESETn = 1; //Change to 1 to let the core run
    if (TILE_TEST){
        tckTicks(1520000);
    }
    else{
       float wait_time_s = 0.0001;
       pc.printf("Starting wait time of %f seconds \r\n", wait_time_s);
       //wait(0.85);
       wait(wait_time_s);
    }
    
  
   
/*    for (int j = 0; j < 1; j++) {
        addr = 0xe000 + j*4;
        pc.printf("[Post Execution] Reading Addr: %X", addr);
        memAccess(false, true, 0, 0, addr, j*2, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
    }
*/    
    
    unsigned int base_addr = 0x20020000;
    unsigned int error_count = 0;
    unsigned int error_addr = 0;
    for (int offset = 0; offset<64; offset++){
        addr = base_addr + (offset*4);
        pc.printf("[Post Execution] Reading Addr: %x ", addr);
        int value = memAccess(false, true, 0, 0, addr, offset*2+3, 0, false, 0, active_tiles, x_tiles, core_count[0][0], core_count);
        if (value != offset*2+3) {error_count++; error_addr = addr;}
    }
    pc.printf("[Post Execution] Memory read error count %d; error addr: %x ", error_count, error_addr);
    
    printf("Press spacebar to continue\r\n");
    while(1) {
        c = pc.getc();      
        if (c == 0x20) {
            break;
        }
    }
    pc.printf("\r\n");
    
    while(1){tckTicks(1000000000);}

    /*wait_us(100);
    TILE_TEST = 0;
    wait_us(1000);
    CLK_RSTn = 1;
    wait_us(1000);
    CLK_RSTn = 0;
    wait_us(1000);
    CLK_RSTn = 1;*/
    wait(10);
    
    TCK_EN = 0;
    wait_us(100);
    TILE_TEST = 1;
    CORERESETn = 0;
    tckTicks(4096);//ensure synchronous reset
    CORERESETn = 1;
    tckTicks(4096);//ensure synchronous reset
    TILE_TEST = 1;

    pc.printf("[main] program starts...\r\n");

    wait(1);

    loopRead(x_tiles, y_tiles, active_tiles, core_count, 0x40000000);

}
