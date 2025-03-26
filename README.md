# waferscale_code
This repository contains the starter code for testing the waferscale processor.

# Mbed Setup
1) Go to https://studio.keil.arm.com/ and create an account
2) After creating an account create a new project and select `empty Mbed OS project` for Mbed OS 6
3) Copy the files from `mbed_code` and delete the `mbed-os` folder that was created with the project
4) In the build target section, select `mbed LPC1768`
5) Run the build and upload the bin file onto the Mbed board

# ELF Code Compilation
1) Download ECLIPSE Studio
2) Create a new C/C++ project, pick C++ Managed Build
3) Create an empty project and when prompted to select toolchain, pick `GNU Tools for ARM Embedded Processors (arm-none-eabi-gcc)` (need to find download link)
4) Copy and paste all folders in demo code (`Ring_Comm`) into project directory
5) Click build and run the program
6) Upload the code as `H.elf` onto the mbed board
