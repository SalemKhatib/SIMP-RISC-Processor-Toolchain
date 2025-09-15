# SIMP-RISC-Processor-Toolchain
A complete software toolchain, including a C-based assembler and cycle-accurate simulator, for a custom 32-bit RISC processor.
# C-Based Assembler and Simulator for a 32-bit RISC Processor

This project is a complete software toolchain for "SIMP," a custom 32-bit RISC processor with a MIPS-like instruction set, developed as part of the Computer Structure course at Tel Aviv University. The toolchain includes a two-pass assembler and a cycle-accurate simulator, both written entirely in C.

## Key Features

### 1. Two-Pass Assembler
- **C-based implementation** that translates symbolic assembly language into 32-bit machine code.
- **Symbol Table Management:** Properly resolves forward-referenced labels by performing two passes over the source code.
- **Instruction Encoding:** Accurately encodes all 22 of the processor's opcodes and their arguments into the specified binary format.
- **Support for Directives:** Includes support for `.word` directives to directly initialize memory locations.

### 2. Cycle-Accurate Simulator
- **Full ISA Implementation:** Simulates the entire fetch-decode-execute pipeline for the custom RISC ISA.
- **Complex I/O Subsystem:** Models a variety of memory-mapped hardware peripherals:
    - 256x256 monochrome monitor with a frame buffer.
    - 128-sector hard disk with simulated DMA for block transfers.
    - Programmable timer.
    - 32 general-purpose LEDs.
    - 8-digit 7-segment display.
- **Interrupt Handling:** Implements a full interrupt mechanism with three distinct IRQ sources (timer, disk, external line), an interrupt handler address register, and context saving/restoring via `reti`.
- **Detailed Tracing:** Generates comprehensive log files, including a cycle-by-cycle trace of register values (`trace.txt`), a log of all hardware register I/O (`hwregtrace.txt`), and final memory/disk states.

## How to Build and Run

*Note: The source code and test files are located within the `SIMP RISC PROJECT` directory.*

### Compiling
The assembler and simulator can be compiled using GCC from the root of the repository:
```bash
# Note: Adjust paths if your .c files are in different subfolders
gcc -o assembler "SIMP RISC PROJECT/SR/asm/assembler.c"
gcc -o simulator "SIMP RISC PROJECT/SR/sim/simulator.c"
