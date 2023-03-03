# Bare Metal Raspberry PI Pico
 A bare metal C programming environment for the Raspberry Pi Pico with minimal dependences.  This is "true" bare metal C with no SDK or other layes between the programmer and the hardware.  A USB CDC serial driver, header files providing standard port naming, and a copy-to-RAM stage 2 bootloader are provided.  The environment also includes a python-based tools to checksum the bootloader and create UF2 files to minimize dependences.

# Motivation
 A software-development kit (SDK) facilitates rapid development and improves portability but adds a layer of abstraction that hides many of the low-level hardware features, which is undesirable for teaching bare-metal embedded systems.  This environment was developed for use in an embedded systems course at Binghamton University.  
 
 # Status
 This environment has not yet undergone significant testing but will be updated as bugs are disovered.
 
 # Included Examples
 A sample C program to turn on the red LED on the Pico is provided in the src directory.  To make the UF2, run make led_on.uf2.
