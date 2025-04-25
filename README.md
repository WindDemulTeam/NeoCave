# Its my PET-project of Arcade Machine Emulator - Cave SH-3 Based System  

## Overview  

This is a pet project aimed at emulating an arcade hardware platform based on the **Hitachi SH-3 CPU**, similar to systems used in games like *Ibara* and other Cave arcade titles. The emulator replicates the basic functionality of this architecture, including CPU execution, sound processing, and video rendering.  

### Key Hardware Specifications  
- **Main CPU**: Hitachi SH-3 (32-bit RISC) @ **133 MHz**  
- **Sound Chip**: Yamaha YMZ770C-F (4-channel ADPCM + DSP effects)  
- **FPGA**: Altera Cyclone EP1C12F324C8 (likely handles video processing & SDRAM control)  
- **Memory**:  
  - **NAND Flash**: K9F1G08U0A (128MB)  
  - **Flash ROMs**:  
    - 1x MX29LV160BBTC-90 (2MB)  
    - 2x MX29LV320ABTC-90 (4MB each, with space for 2 more unused)  
  - **SDRAM**:  
    - 1x MT48LC2M32B2-6 (64MB)  
    - 2x MT46V16M16 (32MB each)  
- **Audio Amplifier**: LA4708  

*(Based on hardware info from [System16](https://www.system16.com/hardware.php?id=868))*  

## Features  

### Currently Implemented  
- **SH-3 CPU core**  
- **Memory subsystem** (Flash, SDRAM, and NAND Flash emulation)  
- **YMZ770C-F sound chip**   
- **Basic FPGA-assisted video rendering** 

## Getting Started  

### Prerequisites  
- C++20 compiler (GCC, Clang, or MSVC)
- CMake 3.12+  
- SDL3 (for rendering/audio) 

### Building 
```bash  
mkdir build && cd build  
cmake .. -DCMAKE_BUILD_TYPE=Release  
make -j4  
