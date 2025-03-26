#include "jtag.h"
#include "mbed.h"
#include "pinout.h"
#include "mmap.h"
#include "lcd.h"
#include "elf.h"
#include "stdio.h"

using namespace std;

//-----------------------------------------
// Memory

unsigned int JTAG::memRead(unsigned int baseaddr, unsigned int readdata[], int size, bool check, bool print)
{
    unsigned int mismatch = 0;

    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);

    unsigned int addr = baseaddr;

    int i = 0;
    while(i*1024 < size) {
        writeAPACC(addr, AP_TAR, false);

        readAPACC(AP_DRW, false, false);
        unsigned int j;
        for(j=1024*i+1; j<1024*(i+1) && j<size; j++) {
            unsigned int word = readAPACC(AP_DRW, false, false);
            if(check) {
                if(readdata[j-1] != word) {
                    mismatch++;
                    if(print) {
                        pc.printf("Mismatch line %x, was %x, expected %x\r\n", j-1, word, readdata[j-1]);
                    }
                }
            }
            readdata[j-1] = word;
        }
        unsigned int word = rdBuff(false);
        if(check) {
            if(readdata[j-1] != word) {
                mismatch++;
                if(print) {
                    pc.printf("Mismatch line %x, was %x, expected %x\r\n", j-1, word, readdata[j-1]);
                }
            }
        }
        readdata[j-1] = word;

        addr = addr + 1024*4;
        i++;
    }
    return mismatch;
}

void JTAG::memWrite(unsigned int baseaddr, unsigned int writedata[], int size, bool zero)
{
    if(zero){
       pc.printf("Called with %x, %x\r\n",  baseaddr, size);
    }
    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);

    unsigned int addr = baseaddr;

    int i = 0;
    while(i*1024 < size) {
        writeAPACC(addr, AP_TAR, false);

        for(int j=1024*i; j<1024*(i+1) && j<size; j++) {
            if(zero) {
                writeAPACC(0,            AP_DRW, false);
            } else {
                writeAPACC(writedata[j], AP_DRW, false);
            }
        }

        addr = addr + 1024*4;
        i++;
    }
}

unsigned int JTAG::readMemory(unsigned int address)
{
    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);
    //pc.printf("Memory Address %d \n", address);
    writeAPACC(address, AP_TAR);
    return readAPACC(AP_DRW);
}

void JTAG::writeMemory(unsigned int address, unsigned int value)
{
    writeBanksel(0);
    //pc.printf("[writeMemory] writeBanksel done\r\n");
    writeAPACC(0x23000052, AP_CSW);
    //pc.printf("[writeMemory] writeAPACC AP_CSW done\r\n");
    writeAPACC(address, AP_TAR);
    //pc.printf("[writeMemory] writeAPACC addr done\r\n");
    writeAPACC(value, AP_DRW);
    //pc.printf("[writeMemory] writeAPACC value done\r\n");
    //rdBuff(1);
}

int JTAG::loadProgram()
{
    PowerupDAP();

    /*if (haltCore() != 0) {
        pc.printf("Could not halt core during program load.\r\n");
    }*/

   // dual_printf("Reading Program HEX");
   // pc.printf("loading program\r\n");
    
    FILE *fp = fopen("/local/program.hex", "r");
    //FILE *fp = fopen("/local/led_blink.hex", "r");
    if (fp == NULL) {
        pc.printf("Error in open /local/program.hex\r\n");
        return -1;
    }
    pc.printf("Program open\r\n");

    // Similar to MemWrite here
    pc.printf("Begin loading program into IMEM\r\n");
    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);
    unsigned int addr = 0x10000000;
    //unsigned int addr = IMEM_MIN;
    //unsigned int count = 0;

    while(!feof(fp) && addr <= IMEM_MAX) {
        writeAPACC(addr, AP_TAR, false);
        for(int j=0; j<1024 && !feof(fp) && addr+j*4<=IMEM_MAX; j++) {
            unsigned int d;
            fscanf(fp, "%X", &d);
            writeAPACC(d, AP_DRW, false);
            printf("loadProgram: addr = %x, data = %x\r\n", addr+j*4, d);
        }
        addr = addr + 1024*4;
        /*count++;
        if (count % 100 == 0) {
            pc.printf("loading mem, count = %d", count);
        }*/
    }

   // dual_printf("Check prog in Imem");
    pc.printf("Done loading IMEM, start checking...\r\n");
    unsigned int mismatch = 0;

    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);
    addr = 0x10000000;
    //addr = IMEM_MIN;

    while(!feof(fp)) {
        writeAPACC(addr, AP_TAR, false);

        readAPACC(AP_DRW, false, false);
        unsigned int j;
        for(j=1; j<1024 && !feof(fp); j++) {
            unsigned int word = readAPACC(AP_DRW, false, false);
            unsigned int d;
            fscanf(fp, "%X", &d);
            if(d != word) {
                mismatch++;
                pc.printf("Mismatch line %x, was %x, expected %x\r\n", j-1, word, d);
            }
        }
        if(!feof(fp)){
            unsigned int word = rdBuff(false);
            unsigned int d;
            fscanf(fp, "%X", &d);
            if(d != word) {
                mismatch++;
                pc.printf("Mismatch line %x, was %x, expected %x\r\n", j-1, word, d);
            }
        }
        addr = addr + 1024*4;
    }
    if(mismatch) {
        dual_printf("Mem Load Failed");
        return -1;
    }

    fclose(fp);

    //dual_printf("Map Imem to addr 0");
    writeMemory(set_imem, 1);
    return 0;
}


int JTAG::haltCore()
{
    for (int i = 0; i < num_cores; i++) {
        if (core_op[i] != 1) {
            pc.printf("core %d doesn't need operation\r\n", i);
        }
    }
    unsigned int address;
    unsigned int value;
    address = DHCSR_ADDR;
    value = DHCSR_DBGKEY | DHCSR_C_HALT | DHCSR_C_DEBUGEN;
    pc.printf("Value written %d\r\n", value);
    writeMemory(address, value);
    value = readMemory(address);
    pc.printf("Value read %d\r\n", value);

    if (! ((value & DHCSR_C_HALT) && (value & DHCSR_C_DEBUGEN)) ) {
        pc.printf("cannot halt the core, check DHCSR...\r\n");
        
        //IA
        char c;
        printf("Press spacebar in your terminal once ELF has been loaded into the mbed's flash.\r\n");
        while(1) {
            c = pc.getc();      
            if (c == 0x20) {
                break;
            }
        }
        pc.printf("\r\n");
        
        return -1;
    }
    
    return 0;   
}

int JTAG::zeroImemOffset()
{
    //set Imem offset to address 0
    //writeMemory(set_imem, 1);//set_imem //IA removed this
    return 0;
}

//-----------------------------------------
// Single core functionality in a multicore setting
int JTAG::haltCore_single(int core_id)
{
    saveCore();
    unsetAllCore();
    setCore(core_id);
    int retval;
    
    unsigned int address;
    unsigned int value;
    address = DHCSR_ADDR;
    value = DHCSR_DBGKEY | DHCSR_C_HALT | DHCSR_C_DEBUGEN;
    writeMemory(address, value);
    value = readMemory(address);

    if (! ((value & DHCSR_C_HALT) && (value & DHCSR_C_DEBUGEN)) ) {
        pc.printf("cannot halt the core, check DHCSR...\r\n");
        retval =  -1;
    } else {
        retval = 0;
    }
    
    restoreCore();
    
    return retval;
    
}

unsigned int JTAG::readMem_single(int core_id, unsigned int address)
{
    saveCore();
    unsetAllCore();
    setCore(core_id);
    
    unsigned int retval = readMemory(address);
    
    restoreCore();
    return retval;
}

void JTAG::writeMem_single(int core_id, unsigned int address, unsigned int value)
{
    saveCore();
    unsetAllCore();
    setCore(core_id);
    
    writeMemory(address, value);
    
    restoreCore();
    return;
}


//-----------------------------------------
// ELF Loading Functionality
//

void JTAG::dumpMemToFile(FILE * &fp, unsigned int startaddr, unsigned int lastaddr) //start address to last address, inclusive
{
    unsigned int value, address;
    for (address = startaddr; address <= lastaddr; address += 0x00000004) { //see if we can read one byte at a time?
        value = readMemory(address);
        fprintf(fp, "Address: %x , Value: %x\r\n", address, value); //dump that particular address out
    }
}


void JTAG::wipeMemRange(unsigned int startaddr, unsigned int lastaddr) //start address to last address, inclusive
{
    unsigned int value, address;
    //unsigned int rdata;
    value = 0x00000000;
    for (address = startaddr; address <= lastaddr; address += 0x00000004) {
        writeMemory(address, value);
        //pc.printf("Wrote to memory address %u\r\n", address);
        /*rdata = readMemory(address);
        if (rdata != value) {
            pc.printf("wipeMemRange: address %X didn't wiped, data = %X\r\n", address, rdata);
        }
        if (address == 0x20000100) {
            pc.printf("addr 0x20000100 contains data %X\r\n", rdata);
        }*/
    }
}


void JTAG::writeBufToMem(unsigned int baseaddr, unsigned int lastaddr, unsigned int writedata[], int size)//start address to last address, inclusive
{
    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);

    unsigned int value, address;
    int index;
    
    //write word-by-word
    index = 0;
    address = baseaddr;
    while ((index < size) && (address <= lastaddr)) {
        value = writedata[index];
        writeMemory(address, value);
        index += 1;
        address += 0x00000004;
        if (byte_count_flag) {
            unsigned int prev_bytes = byte_written;
            byte_written += 4;
            if (byte_written < prev_bytes) {
                pc.printf("[WARNING]byte_written overflow!\r\n");
            }
        }
    }
    if (index == size) {
        //pc.printf("Stopped loading this data buffer because all words were loaded into memory. Congratulations!\r\n");
    }
    if (address > lastaddr) {
        pc.printf("Stopped loading this data buffer because we exceeded the last address you specified. Uh-oh!\r\n");
    }
}


int JTAG::validateElfHeader(Elf32_Ehdr &elfheader)
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
}

int JTAG::loadSegment(Elf32_Phdr &progheader, FILE * &fp)
{
    //first, hustle some info out of progheader, then call memWrite(?)
    if (progheader.p_filesz > progheader.p_memsz) {
        pc.printf("This is an invalid program header. The file size is larger than the memory size. Exiting. \r\n");
        return -1;
    }
    else {
        pc.printf("The program header has a valid memory size of %u bytes and file size of %u bytes. \r\n", progheader.p_memsz, progheader.p_filesz);
    }
    
    if (fseek(fp, progheader.p_offset, SEEK_SET) != 0) {
        pc.printf("Could not move to desired location in file where program segment begins. Exiting. \r\n");
        return -1;
    }
    else {
        pc.printf("Moved to where this loadable segment begins.\r\n");
    }
    
    //copy a total of progheader.p_filesz bytes into the mbed's memory as a dynamic array. Then call writeBufToMem on it. Dope!
    
    //create new temporary buffer and read into it. Note -- functionality needs to be changed since it is possible to run out of memory on the mbed
        
    int writedata_sz = ceil(progheader.p_filesz/(double)sizeof(unsigned int)); //round up to the nearest multiple of 32-bit
    pc.printf("For a file size of %d bytes, a write array of %d elements was made.\r\n", progheader.p_filesz, writedata_sz);
    //unsigned int *writedata = new unsigned int[writedata_sz];
    int writedata_sz_new = writedata_sz;
    if (writedata_sz > 20)
    {
        pc.printf("[JL] writedata_sz > 20\r\n");
        int num = ceil((double)writedata_sz/20.0);
        pc.printf("For a file size of %d bytes, number of %d bytes of array required is %d. \r\n", progheader.p_filesz, (20*sizeof(unsigned int)), num);
        for(int i=0; i<(num-1); i++)
        {   
            writedata_sz_new = writedata_sz_new - 20;
            //unsigned int *writedata = new unsigned int[writedata_sz];
            unsigned int *writedata = new unsigned int[20];
            for (int j = 0; j < 20; j++) {
                writedata[j] = 0x00000000;
            }
            //pc.printf("For a file size of %d bytes, a write array of %d elements was made.\r\n", progheader.p_filesz, writedata_sz);
            long offset = progheader.p_offset + sizeof(unsigned int)*i*20;
            
            if (fseek(fp, offset, SEEK_SET) == 0)
            {
                pc.printf("Seek successful 1st: num = %d, i = %d\r\n", num, i);
                unsigned int file_size = 20*sizeof(unsigned int);
                if (fread((void *)writedata, file_size, 1, (FILE *)fp) != 1) {
                    pc.printf("Could not read bytes for this segment from the ELF. \r\n");   
                }
            }
            
            //load segment
            pc.printf("Begin loading current segment. \r\n");
            writeBanksel(0);
            writeAPACC(0x23000052, AP_CSW);
            unsigned int startaddr = progheader.p_paddr + (sizeof(unsigned int)*i*20);//IMEM_MIN + 
            pc.printf("loadSegment: startaddr = %x\r\n", startaddr);
            writeBufToMem(startaddr, 0xFFFFFFFF /*change this*/ , writedata, 20);//0xFFFFFFFF IMEM_MAX
            delete [] writedata;
        }
        
        unsigned int *writedata = new unsigned int[writedata_sz_new];
        //unsigned int *writedata = new unsigned int[20];
        for (int i = 0; i < writedata_sz_new; i++) {
            writedata[i] = 0x00000000;
        }
        //pc.printf("For a file size of %d bytes, a write array of %d elements was made.\r\n", progheader.p_filesz, writedata_sz);
        
        long offset = progheader.p_offset + sizeof(unsigned int)*(num-1)*20;
            if (fseek((FILE *)fp,offset,SEEK_SET) == 0)
            {
                pc.printf("Seek successful 2nd\r\n");
                unsigned int file_size = writedata_sz_new*sizeof(unsigned int);
                if (fread((void *)writedata, file_size, 1, (FILE *)fp) != 1) {
                    pc.printf("Could not read bytes for this segment from the ELF. \r\n");   
                }
            }
        
        //load segment
        pc.printf("Begin loading current segment. \r\n");
        writeBanksel(0);
        writeAPACC(0x23000052, AP_CSW);
        unsigned int startaddr = progheader.p_paddr + (sizeof(unsigned int)*(num-1)*20);//IMEM_MIN + 
        writeBufToMem(startaddr, 0xFFFFFFFF /*change this*/ , writedata, writedata_sz_new);//IMEM_MAX
        delete [] writedata;
        
    }
    else
    {
        pc.printf("[JL] writedata_sz <= 20\r\n");
        unsigned int *writedata = new unsigned int[writedata_sz];
        //unsigned int *writedata = new unsigned int[20];
        for (int i = 0; i < writedata_sz; i++) {
            writedata[i] = 0x00000000;
        }
        pc.printf("For a file size of %d bytes, a write array of %d elements was made.\r\n", progheader.p_filesz, writedata_sz);
        
        if (fread((void *)writedata, progheader.p_filesz, 1, (FILE *)fp) != 1) {
            pc.printf("Could not read bytes for this segment from the ELF. \r\n");   
        }
        
        //load segment
        pc.printf("Begin loading current segment. \r\n");
        writeBanksel(0);
        writeAPACC(0x23000052, AP_CSW);
        
        writeBufToMem(progheader.p_paddr, 0xFFFFFFFF /*change this */, writedata, writedata_sz);// IMEM_MIN + , IMEM_MAX
        delete [] writedata;
    }
    
    pc.printf("Verify correct write functionality here!\r\n\r\n");
    
    /*pc.printf("Done loading IMEM, start checking...\r\n");
    FILE *fp_hex = fopen("/local/program.hex", "r");
    unsigned int mismatch = 0;

    writeBanksel(0);
    writeAPACC(0x23000052, AP_CSW);
    addr = 0x10000000;
    //unsigned int addr = IMEM_MIN;

    while(!feof(fp_hex)) {
        writeAPACC(addr, AP_TAR, false);

        readAPACC(AP_DRW, false, false);
        unsigned int j;
        for(j=1; j<1024 && !feof(fp_hex); j++) {
            unsigned int word = readAPACC(AP_DRW, false, false);
            unsigned int d;
            fscanf(fp_hex, "%X", &d);
            if(d != word) {
                mismatch++;
                pc.printf("Mismatch line %x, was %x, expected %x\r\n", j-1, word, d);
            }
        }
        if(!feof(fp_hex)){
            unsigned int word = rdBuff(false);
            unsigned int d;
            fscanf(fp_hex, "%X", &d);
            if(d != word) {
                mismatch++;
                pc.printf("Mismatch line %x, was %x, expected %x\r\n", j-1, word, d);
            }
        }
        addr = addr + 1024*4;
    }
    if(mismatch) {
        dual_printf("Mem Load Failed");
        return -1;
    }*/
    
    
   // delete [] writedata;
    
    return 0;
}


int JTAG::loadElf(char* path)
{
    
    ////////OPEN THE PROGRAM FILE////////////
    
    //FILE *fp = fopen("/local/Hello.elf", "rb"); //open as read binary mode
    //FILE *fp = fopen("/local/hello.elf", "rb"); //open as read binary mode
    FILE *fp = fopen(path, "rb"); //open as read binary mode
    if (fp == NULL) {
        pc.printf("Could not open %s . Exiting. \r\n", path);
        return -1;
    }
    
    /////////START COUNTING BYTES PER CLOCK/////////////
    byte_count_flag = 1;
    clk_count_flag = 1;
    pc.printf("Start counting Bytes written and JTAG clk.\r\n");
    
    /////////ELF HEADER PROCESSING/////////////
    
    //load ELF header
    pc.printf("Begin loading the ELF header. \r\n");
    if (fseek(fp, 0, SEEK_SET) != 0) {
        pc.printf("Could not move to start of ELF header. Exiting. \r\n");
    }
    Elf32_Ehdr elfheader;
    if (fread((void *)&elfheader, sizeof(elfheader), 1, (FILE *)fp) != 1) { //number of elements successfully read is returned
        pc.printf("Could not read the ELF header. Exiting. \r\n");
    }
    
    //check for validity of fields
    if (validateElfHeader(elfheader) != 0) {
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
            pc.printf("[JL] Before loadSegment: Bytes written: %d bytes, JTAG clks taken: %d cycles\r\n", rptBytes(), rptClks());
            loadSegment(progheader, fp);
            pc.printf("[JL] After loadSegment: Bytes written: %d bytes, JTAG clks taken: %d cycles\r\n", rptBytes(), rptClks());
        }
        else { //don't load segment corresponding to program header
            pc.printf("Encountered non-loadable segment number %d. Moving on to next segment.\r\n", i);
        }
        
    }
    pc.printf("Successfully loaded all loadable segments into memory. \r\n");

    fclose(fp);
    
    /////////DISABLE COUNTING BYTES PER CLOCK/////////////
    byte_count_flag = 0;
    clk_count_flag = 0;
    
    pc.printf("[JL] Bytes written: %d bytes, JTAG clks taken: %d cycles\r\n", rptBytes(), rptClks());
    resetBytes();
    resetClks();
    pc.printf("Bytes written count and JTAG clk count resetted.\r\n");
    
    return 0;
}



// ------------------------------------------------
// DP/AP Config

unsigned int JTAG::rdBuff(bool set_ir=true)
{
    if(set_ir) {
        setIR(JTAG_DPACC);
    }
    //shiftData(0, DP_RDBUFF, READ);
    return shiftData(0, DP_RDBUFF, READ);
}

unsigned int JTAG::readDPACC(unsigned char addr, bool set_ir, bool rdthis)
{
    //pc.printf("readDPACC addr = %x, set_ir = %d, rdthis = %d\r\n", addr, set_ir, rdthis);
    if(set_ir) {
        setIR(JTAG_DPACC);
    }
    unsigned int retdata = shiftData(0, addr, READ);
    //retdata = shiftData(0, addr, READ);
    if(rdthis) {
        return rdBuff();
    } else {
        return retdata;
    }
}
unsigned int JTAG::readAPACC(unsigned char addr, bool set_ir, bool rdthis)
{
    //pc.printf("readAPACC addr = %x, set_ir = %d, rdthis = %d\r\n", addr, set_ir, rdthis);
    if(set_ir) {
        setIR(JTAG_APACC);
    }
    unsigned int retdata = shiftData(0, addr, READ);
    //retdata = shiftData(0, addr, READ);
    if(rdthis) {
        return rdBuff();
    } else {
        return retdata;
    }
}

void JTAG::writeAPACC(unsigned int data, unsigned char addr, bool set_ir)
{
    if(set_ir) {
        setIR(JTAG_APACC);
    }
    //shiftData(data, addr, WRITE);
    shiftData(data, addr, WRITE);
}

void JTAG::writeDPACC(unsigned int data, unsigned char addr, bool set_ir)
{
    //pc.printf("writeDPACC data = %x, addr = %x, set_ir = %d\r\n", data, addr, set_ir);
    if(set_ir) {
        setIR(JTAG_DPACC);
        //pc.printf("JTAG_DPACC IR set\r\n");
    }
    shiftData(data, addr, WRITE);
    //unsigned int retdata = shiftData(data, addr, WRITE);
    //pc.printf("writeDPACC retdata = %x\r\n", retdata);
}

void JTAG::writeBanksel(unsigned int banksel, bool set_ir)
{
    if(set_ir) {
        setIR(JTAG_DPACC);
    }
    //shiftData(banksel << 4, DP_SELECT, WRITE);
    shiftData(banksel << 4, DP_SELECT, WRITE);
}

void JTAG::DAP_enable(void)
{
    setState('r');
    leaveState();
    // write CTRL to enable DAP

    writeDPACC(0x50000000, DP_CTRLSTAT);
    writeDPACC(0x00000000, AP_SELECT);
    writeAPACC(0x23000042, AP_CSW);
}


int JTAG::PowerupDAP()
{
    writeDPACC(0x12345678, DP_SELECT);
    int rd = readDPACC(DP_SELECT);
    if(rd != 0x12000070) {
        pc.printf("[PowerupDAP()] DP SELECT %x, expecting 0x12000070\r\n", rd);
        //exit(1);
        return -1;
    }
    writeDPACC(0xedcba987, DP_SELECT);
    rd = readDPACC(DP_SELECT);
    if(rd != 0xed000080) {
        pc.printf("[PowerupDAP()] DP SELECT %x, expecting 0xed000080\r\n", rd);
        //exit(1);
        return -1;
    }
    writeDPACC(0x10000000, DP_CTRLSTAT);
    rd = readDPACC(DP_CTRLSTAT);
    if(rd != 0x30000000) {
        pc.printf("[PowerupDAP()] DP CTRL %x, expecting 30000000\r\n", rd);
        //exit(1);
        return -1;
    }
    writeDPACC(0x40000000, DP_CTRLSTAT);
    rd = readDPACC(DP_CTRLSTAT);
    if(rd != 0xc0000000) {
        pc.printf("[PowerupDAP()] DP CTRL %x, expecting c0000000\r\n", rd);
        //exit(1);
        return -1;
    }
    writeDPACC(0x50000000, DP_CTRLSTAT);
    rd = readDPACC(DP_CTRLSTAT);
    if(rd != 0xf0000000) {
        pc.printf("[PowerupDAP()] DP CTRL %x, expecting f0000000\r\n", rd);
        //exit(1);
        return -1;
    }
    //pc.printf("[PowerupDAP()] writeBanksel(0xf);\r\n");
    writeBanksel(0xf);
    rd = readAPACC(AP_IDR);
    if(rd != 0x24770011) {
        pc.printf("[PowerupDAP()] AP IDR %x, expecting 24770011\r\n", rd);
        //exit(1);
        //return -1;
    }
    while (readAPACC(AP_IDR) != 0x24770011) {
        writeBanksel(0xf);
    }
    // pc.printf("DAP Activated\r\n");
    return 0;
   //dual_printf("DAP Activated");
}

// --------------------------------
// State Manipulation

void JTAG::setIR(unsigned char A)
{
    setState('i');
    /*char one = shiftBits(A, 4);
    //one = shiftBits(A, 4);
    if(one != 1) {
        //dual_printf("ERROR: JTAG IR, core %d\r\n", num_core-1);
        pc.printf("ERROR: JTAG IR, core %d\r\n", num_cores-1);
        pc.printf("Got %x instead of 1\r\n", one);
    }
    unsigned int n = num_cores - 1;
    while(n > 0) {
        one = shiftBits(A, 4);
        if(one != 1) {
            pc.printf("ERROR: JTAG IR, core %d\r\n", n-1);
            pc.printf("Got %x instead of 1\r\n", one);
        }
        n--;
    }*/
    
    
    int n = num_cores - 1;
    //for (n=0; n<num_cores; n=n+1) {
    for (n=num_cores - 1; n>=0; n=n-1) {
        char one = 0;
        if (core_op[n] == 1) {
            one = shiftBits(A, 4);
            // pc.printf("core %d IR set to %x\r\n", n, A);
        } else {
            one = shiftBits(JTAG_BYPASS, 4);
            //one = shiftBits(A, 4);
            // pc.printf("core %d IR set to BYPASS\r\n", n);
            
        }
        if (one != 1) {
            pc.printf("ERROR: JTAG IR, core %d, got %x instead of 1\r\n", n, one);
        }
    }
    leaveState();
}

//moves to specified state from IDLE (reset from anywhere)
void JTAG::setState(unsigned char c)
{
    switch (c) {
        case 'n':
            break;
        case 'r':
            reset();
            break;
        case 'd':
            TMSHigh();
            clockTicks(1);
            TMSLow();
            clockTicks(2);
            state = 'd';
            break;
        case 'i':
            TMSHigh();
            clockTicks(2);
            TMSLow();
            clockTicks(2);
            state = 'i';
            break;
        default:
            break;
    }
}

//leave from current state to idle state
void JTAG::leaveState(void)
{
    switch (state) {
        case 'n':
            break;
        case 'r':
            TMSLow();
            clockTicks(1);
            state = 'n';
            break;
        case 'i':
            TMSHigh();
            clockTicks(2);
            TMSLow();
            clockTicks(1);
            state = 'n';
            break;
        case 'd':
            TMSHigh();
            clockTicks(2);
            TMSLow();
            clockTicks(1);
            state = 'n';
            break;
        default:
            break;
    }
    pre_shift = 0;
}

void JTAG::reset(void)
{
    TMSHigh();
    clockTicks(10);
    TMSLow();
    state = 'r';
    return;
}

unsigned int JTAG::readID(void)
{
    setIR(JTAG_IDCODE);
    
    setState('d');
    // unsigned int id = shiftBits(0, 32);
    // pc.printf("ID %d: %x\r\n", num_cores-1, id);
    // /*pc.printf("ID 1: %X\r\n", id);
    // id = shiftBits(0, 32);
    // pc.printf("ID 0: %X\r\n", id);*/
    // unsigned int n = num_cores - 1;
    // while(n > 0) {
    //     id = shiftBits(0, 32);
    //     pc.printf("ID %d: %x\r\n", n-1, id);
    //     n--;
    // }
    unsigned int id;
    int n = num_cores-1;
    //for (n=0; n<num_cores; n=n+1) {
    for (n=num_cores-1; n>=0; n=n-1) {
        if (core_op[n] == 1) {
            id = shiftBits(0, 32);
            // pc.printf("[readID] core %d: ID=%x\r\n", n, id);
            if (id != 0x4ba00477) {
                pc.printf("core %d: ID=%x\r\n", n, id);
            }
        } else {
            shiftBits(0, 1);
            // pc.printf("[readID] core %d bypassed\r\n", n);
        }
        
    }
    
    leaveState();

    return id;
}

// --------------------------------------------
// Data Shifting

unsigned int JTAG::shiftBits(unsigned int data, int n)
{
    //pc.printf("[shiftBits] data:%x, size:%d\r\n", data, n);
    unsigned int c=0;
    int tdo_reg = 0;
    /*int tmp_tdo_0;
    switch(curr_tile_y) {
        case 0:
            tmp_tdo_0 = TDO;
            break;
        case 1:
            tmp_tdo_0 = TDO_1;
            break;
        case 2:
            tmp_tdo_0 = TDO_2;
            break;
        default: 
            pc.printf("Bad TDO: curr_tile_y = %d\r\n", curr_tile_y);
            break;
    }
    if (tmp_tdo_0) {
        c += 0x1;
    }*/
    //pc.printf("[shiftBits] i=0, c=%x\r\n", c);
    clockLow();
    //tdo_reg = active_tile[curr_tile_y];
    //tdo_reg = num_cores;
    tdo_reg = (num_tiles == 1) ? 0 : (num_tiles-1);
    //tdo_reg = num_tiles - 1; //SP
    //if (pre_shift == 0) {
    //pc.printf("Num_Tiles = %d , pre_shift = %d , n = %d \r\n", num_tiles, pre_shift, n);
    if (pre_shift == 0 && tdo_reg != 0)  {
        clockTicks(tdo_reg);
        pre_shift = 1;
        pc.printf("[shiftBits] pre shifted %d\r\n", tdo_reg);
    }
    
    int i;
    for (i=0; i<n; i++) {
        int tmp_tdo;
        //pc.printf("Current TIle: %d\r\n",  curr_tile_y);
        switch(curr_tile_y) {
        case 0:
            tmp_tdo = TDO;
            //IA
            /*if (i==0)
            tmp_tdo = 1;
            else
            tmp_tdo = 0;*/ //IA
            break;
        case 1:
            tmp_tdo = TDO_1;
            break;
        case 2:
            tmp_tdo = TDO_2;
            break;
        default: 
            pc.printf("Bad TDO index: curr_tile_y = %d\r\n", curr_tile_y);
            break;
        }
        /*if (tmp_tdo) {
            c += 0x1;
        }*/
        //if (TDO) {
        //if (tmp_tdo && i!=0) {
        if (tmp_tdo) {
            c += (0x1 << i);
        }
        //pc.printf("[shiftBits] i=%d, c=%X, data=%X\r\n", i, c, data);

        clockTicks(1);

        if ( (data & 1)== 0 ) {
            DataLow();
        } else {
            DataHigh();
        }
        data=data>>1;
    }

    /*int tmp_tdo_f;
    switch(curr_tile_y) {
        case 0:
            tmp_tdo_f = TDO;
            break;
        case 1:
            tmp_tdo_f = TDO_1;
            break;
        case 2:
            tmp_tdo_f = TDO_2;
            break;
        default: 
            pc.printf("Bad TDO: curr_tile_y = %d\r\n", curr_tile_y);
            break;
    }
    if (tmp_tdo_f) {
        c = (c >> 1) + (0x1 << i);
    }*/

    return c;
}

unsigned int JTAG::shiftData(unsigned int data, char addr, bool rw)
{
    //pc.printf("shiftData: data = %X, addr = %X, rw = %d\r\n", data, addr, rw);
    unsigned int retdata[num_cores];
    for (int r=0; r<num_cores; r=r+1) {
        retdata[r] = 0;
    }
    //unsigned int retdata;

    if (num_cores > 1) setState('d');

    if (core_op[num_cores-1]) {
        bool gotwait = true;
        while(gotwait) {
            gotwait = false;

            if (num_cores > 1) {
                //setState('d'); done above
            } else {
                setState('d');
            }
            // First 3 bits are either OK/FAULT 010, or WAIT 001
            int okstat = shiftBits(rw, 1);
            okstat |= (shiftBits(addr >> 2, 2) << 1);
            /*pc.printf("shiftData: shifted 3 bit\r\n");
            pc.printf("okstat = %d\r\n", okstat);*/

            if(okstat == 1) {
                wait_indicator = !wait_indicator;
                //pc.printf("[JL] shiftData, core %d: got wait at addr = %x, data = %x\r\n", num_cores-1, addr, data);
                leaveState();
                gotwait = true;
            } else if(okstat == 2) {
                // Got OK/FAULT
            } else {
                pc.printf("invalid OK Stat %x, core %d\r\n", okstat, num_cores-1);
                /*leaveState();
                exit(1);*/
            }
        }
        retdata[num_cores-1] = shiftBits(data, 32);
    } else {
        //shiftBits(0, 32);
        //pc.printf("[shiftData] core %d bypassed\r\n", num_cores-1);
        shiftBits(0, 1);
    }
    //unsigned int retdata = shiftBits(0, 32);
    
    /*int okstat = shiftBits(rw, 1);
    okstat |= (shiftBits(addr >> 2, 2) << 1);
    if(okstat == 1) {
        wait_indicator = !wait_indicator;
        pc.printf("[JL] Second shiftData: got wait\r\n");
    } else if (okstat == 2) {
        //pc.printf("[JL] Second shiftData: OK/FAULT\r\n");
    } else {
        pc.printf("[JL] Second shiftData: invalid okstat\r\n");
    }*/
    //retdata = shiftBits(data, 32);
    //shiftBits(0, 32);
    
    int n = num_cores - 1;
    //while(n > 0) {
    for (n = num_cores-1; n>0; n=n-1) {
        if (core_op[n-1]) {
            int okstat = shiftBits(rw, 1);
            okstat |= (shiftBits(addr >> 2, 2) << 1);
            if(okstat == 1) {
                wait_indicator = !wait_indicator;
                //pc.printf("[JL] shiftData, core %d: got wait at addr = %x, data = %x\r\n", n-1, addr, data);
            } else if (okstat == 2) {
                //pc.printf("[JL] shiftData, core %d: OK/FAULT\r\n", n-1);
            } else {
                pc.printf("[JL] shiftData, core %d: invalid okstat\r\n", n-1);
            }
            retdata[n-1] = shiftBits(data, 32);
        } else {
            //shiftBits(0, 32);
            //pc.printf("[shiftData] core %d bypassed\r\n", n);
            shiftBits(0, 1);
        }
        //retdata = shiftBits(data, 32);
        //n--;
    }
    
    leaveState();
    for (int k = 0; k < num_cores; k++) {
        //pc.printf("[shiftData] retdata[%d] = %x\r\n", k, retdata[k]);
    }

    /*for (int k = 0; k < num_cores; k++) {
        //n = num_core - 1 - k;
        if (core_op[k]) {
            return retdata[k];
        }
    }*/
    //for (int k = num_cores-1; k >= 0; k--) {
    for (int k = 0; k < num_cores; k++) {
        //n = num_core - 1 - k;
        if (core_op[k]) {
            return retdata[k];
        }
    }
    //return retdata;
    return 0;
}


// ----------------------------------
// Toggle Functions

void JTAG::DataLow(void)
{
    wait_us(delay);
    TDI = 0;
    //tdo_forced = 0; //IA
}
void JTAG::DataHigh(void)
{
    wait_us(delay);
    TDI = 1;
    //tdo_forced = 1; //IA
}

void JTAG::clockLow(void)
{
    wait_us(delay);
    TCK = 0;
    if (clk_count_flag) clk_level = 0;
}

void JTAG::clockHigh(void)
{
    wait_us(delay);
    TCK = 1;
    if (clk_count_flag == 1 && clk_level == 0) {
        unsigned int prev_clk_count = jtag_clk_count;
        jtag_clk_count += 1;
        clk_level = 0;
        if (jtag_clk_count < prev_clk_count) {
            pc.printf("[WARNING]jtag_clk_count overflow!\r\n");
        }
    }
}

void JTAG::clockTicks(unsigned char c)
{
    int i;
    clockLow();
    for (i=0; i<c; i++) {
        clockLow();
        clockHigh();
    }
    clockLow();
}

void JTAG::TMSHigh(void)
{
    wait_us(delay);
    TMS = 1;
}

void JTAG::TMSLow(void)
{
    wait_us(delay);
    TMS = 0;
}

// --------------------------------
// Initializing and Config

JTAG::JTAG()
{
    //TDO.mode(PullUp);
    delay = 0;
    TMS = 0;
    TCK = 0;
    TDI = 0;
    //num_core = NUM_CORE;
    num_cores = 0;
    core_op = NULL;
    core_op_bk = NULL;
    /*for (int i = 0; i < num_core; i++) {
        core_op[i] = 0;
        core_op_bk[i] = 0;
    }*/
    /*core_count = 0;
    cm3_max = 0;
    
    cm3_count = 0;*/
    //active_tile = NULL;
    pre_shift = 0;
    
    byte_written = 0;
    jtag_clk_count = 0;
    clk_count_flag = 0;
    clk_level = 0;
    //tdo_forced = 1; //IA
    reset();
    leaveState();
    return;
}

JTAG::~JTAG()
{
    if (core_op != NULL) {
        delete [] core_op;
    }
    if (core_op_bk != NULL) {
        delete [] core_op_bk;
    }

    /*if (active_tile != NULL) {
        delete [] active_tile;
    }*/
}

void JTAG::setCurrTileID(int y)
{
    //curr_tile_x = x;
    curr_tile_y = y;
    //pc.printf("[JTAG] curr_tile_x = %d, curr_tile_y = %d\r\n", curr_tile_x, curr_tile_y);
    // pc.printf("[JTAG] curr_tile_y = %d\r\n", curr_tile_y);
}

void JTAG::setNumCores(int n)
{
    num_cores = n;
    
    core_op = new int[num_cores];
    core_op_bk = new int[num_cores];
    for (int i = 0; i < num_cores; i++) {
        core_op[i] = 0;
        core_op_bk[i] = 0;
    }
}

void JTAG::setNumTiles(int n)
{
    num_tiles = n;
}

/*void JTAG::setCM3MAX(int core_count, int x_tiles)
{
    cm3_max = core_count * x_tiles;
    core_op = new int[cm3_max];
    core_op_bk = new int[cm3_max];
    for (int i = 0; i < cm3_max; i++) {
        core_op[i] = 0;
        core_op_bk[i] = 0;
    }
}*/

void JTAG::setCore(int n)
{
    core_op[n] = 1;
}

void JTAG::setAllCore(void)
{
    for (int k = 0; k < num_cores; k++) {
        core_op[k] = 1;
    }
}

void JTAG::unsetCore(int n)
{
    core_op[n] = 0;
}

void JTAG::unsetAllCore(void)
{
    for (int k = 0; k < num_cores; k++) {
        core_op[k] = 0;
    }
}

void JTAG::saveCore(void)
{
    for (int k = 0; k < num_cores; k++) {
        core_op_bk[k] = core_op[k];
    }
}

void JTAG::restoreCore(void)
{
    for (int k = 0; k < num_cores; k++) {
        core_op[k] = core_op_bk[k];
    }
}

int JTAG::getCore(int n)
{
    return core_op[n];
}

void JTAG::getConfig(void)
{
    pc.printf("[JTAG config] curr_tile_y = %d, num_cores = %d, num_tiles = %d\r\n", curr_tile_y, num_cores, num_tiles);
    for (int i=0; i<num_cores; i=i+1) {
        pc.printf("[JTAG config] core_op[%d] = %d\r\n", i, core_op[i]);
    }
    //pc.printf("[JTAG config] pre_shift = %d\r\n", pre_shift);
}

unsigned int JTAG::rptBytes(void)
{
    return byte_written;
}

unsigned int JTAG::rptClks(void)
{
    return jtag_clk_count;
}

void JTAG::resetBytes(void)
{
    byte_written = 0;
}

void JTAG::resetClks(void)
{
    jtag_clk_count = 0;
}

void JTAG::setJTAGspeed(int speed)
{
    delay = 1000/speed;
    return;
}

//IA
void JTAG::bypass_state(int flag)
{
    reset();
   
    leaveState();
    
    setState('i');
    unsigned int id = shiftBits(JTAG_BYPASS_2, 32);
    leaveState();
    setState('d');
    clockLow();
    
    /*
    for(int i=0; i<64; i++)
    {
        char c_in;
        int tmp_tdo = TDO;
        int tmp_tdi = TDI;
        pc.printf("Value of TDI: %d TDO: %d \t", tmp_tdi, tmp_tdo);
        pc.printf("Iteration: %d\r\n", i);
        //printf("Current state inside bypass mode: %c\r\n", state);
        //printf("Press spacebar after checking value of TDO.\r\n");
        while(1) {
            c_in = pc.getc();      
            if (c_in == 0x20) {
                break;
            }
        }
        
        clockTicks(1);

        if (flag){
            if ( (i % 2) == 0) {
                DataHigh();
            } else {
                DataLow();
            }
        }
        else {
            if ( (i % 4) == 0) {
                DataHigh();
            } else {
                DataLow();
            }
            }
    }*/
    
    
}