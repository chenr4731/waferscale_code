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
2) Follow the steps in the <a href="https://gnu-mcu-eclipse.github.io/tutorials/hello-arm/">Eclipse project tutorial</a>
5) Replace all the files using demo code (`Ring_Comm`)
6) Click build and run the program
7) Upload the code as `H.elf` onto the mbed board

# Running
After following the setup steps, connect to the mbed board and read the results using any serial reader.
