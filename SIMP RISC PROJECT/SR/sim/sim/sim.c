#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MEM_SIZE 4096
#define NUM_REGS 16
#define IOREG_SIZE 23
#define DISK_SIZE 128 * 256
#define MONITOR_WIDTH 256
#define MONITOR_HEIGHT 256
const char* IORegisterNames[23] = {
    "irq0enable",   // 0
    "irq1enable",   // 1
    "irq2enable",   // 2
    "irq0status",   // 3
    "irq1status",   // 4
    "irq2status",   // 5
    "irqhandler",   // 6
    "irqreturn",    // 7
    "clks",         // 8
    "leds",         // 9
    "display7seg",  // 10
    "timerenable",  // 11
    "timercurrent", // 12
    "timermax",     // 13
    "diskcmd",      // 14
    "disksector",   // 15
    "diskbuffer",   // 16
    "diskstatus",   // 17
    "reserved18",   // 18
    "reserved19",   // 19
    "monitoraddr",  // 20
    "monitordata",  // 21
    "monitorcmd"    // 22
};

typedef enum {
    add = 0,
    sub = 1,
    mul = 2,
    and = 3,
    or = 4,
    xor = 5,
    sll = 6,
    sra = 7,
    srl = 8,
    beq = 9,
    bne = 10,
    blt = 11,
    bgt = 12,
    ble = 13,
    bge = 14,
    jal = 15,
    lw = 16,
    sw = 17,
    reti = 18,
    in = 19,
    out = 20,
    halt = 21
} Opcode;
// Simulator state - global arrays that are widely used
uint32_t memory[MEM_SIZE] = { 0 };
uint32_t registers[NUM_REGS] = { 0 };
uint32_t IORegister[IOREG_SIZE] = { 0 };
uint8_t monitor[MONITOR_WIDTH * MONITOR_HEIGHT] = { 0 };
uint32_t disk[DISK_SIZE] = { 0 };

uint32_t PC = 0;
bool halted = false;
bool irq_in_progress = false; // global value to handle if the irq in progress and should ignore more irqs
bool first_cycle_bigimm = false; // Is this cycle first cycle of bigimm opcode

// IRQ2 data
uint32_t* irq2_times; // irq2 size unknown
int irq2_count = 0;
int irq2_index = 0;

// File pointers for output
FILE* trace_fp, * hwregtrace_fp, * memout_fp, * regout_fp, * cycles_fp;
FILE* leds_fp, * display_fp, * diskout_fp, * monitor_txt_fp, * monitor_yuv_fp;

// Helper macros
#define IMM8_SIGN_EXTEND(val) ((val & 0x80) ? (val | 0xFFFFFF00) : val) // sign extension for 8 bit immediate

// Instruction fields - parsing according to opcode structure
#define OPCODE(i)   (((i) >> 24) & 0xFF) 
#define RD(i)       (((i) >> 20) & 0xF)
#define RS(i)       (((i) >> 16) & 0xF)
#define RT(i)       (((i) >> 12) & 0xF)
#define BIGIMM(i)   (((i) >> 8) & 0x1)
#define IMM8(i)     ((int8_t)((i) & 0xFF))

// Function declarations
void load_input_files(const char* argv[]);
void write_output_files(const char* argv[]);
void fetch_decode_execute();
void log_trace(uint32_t inst);
void log_hwreg(const char* type, const char* name, uint32_t data);
void update_timer();
void update_disk();
void check_and_trigger_irq();
int find_last_index_32(uint32_t arr[], int length);
void out_opcode(int rs, int rt, int rd);


void read_irq2in(const char* filename) { // reading irq2 values with dynamic allocated array, for flexibility in length
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        irq2_count = 0;
        return NULL;
    }

    size_t capacity = 16;
    irq2_count = 0;
    irq2_times = malloc(capacity * sizeof(uint32_t));
    if (!irq2_times) {
        perror("Failed to allocate memory");
        fclose(file);
        irq2_count = 0;
        return NULL;
    }

    while (!feof(file)) {
        if (irq2_count == capacity) { // check if need to increase array size
            capacity *= 2;
            unsigned int* temp = realloc(irq2_times, capacity * sizeof(unsigned int));
            if (!temp) {
                perror("Failed to reallocate memory");
                free(irq2_times);
                fclose(file);
                irq2_count = 0;
                return NULL;
            }
            irq2_times = temp;
        }
        if (fscanf(file, "%u", &irq2_times[irq2_count]) == 1) { // get each line into array
            irq2_count++;
        }
        else {
            // Skip non-numeric or malformed lines
            fscanf(file, "%*s");
        }
    }

    fclose(file);
}




void load_input_files(const char* argv[]) {
    // loading the content of the input files gotten as input
    // memin.txt
    FILE* memin = fopen(argv[1], "r");
    char line[100]; // the lines wont be longer than 100 characters (disk and memory content is 8 character)
    int i = 0;
    while (fgets(line, sizeof(line), memin) && i < MEM_SIZE) {
        sscanf(line, "%x", &memory[i]); // memin is in hex chars
        i++;
    }
    fclose(memin);

    // diskin.txt
    FILE* diskin = fopen(argv[2], "r");
    i = 0;
    while (fgets(line, sizeof(line), diskin) && i < DISK_SIZE) {
        sscanf(line, "%x", &disk[i]); // diskin is in hex chars
        i++;
    }
    fclose(diskin);

    // irq2in.txt
    read_irq2in(argv[3]);
}


int find_last_index_32(uint32_t arr[], int length) {
    // checks the last value of an array - arr, according to its given length, to check the last value different than 0
    int i = 0;
    int last_index = 0;
    uint32_t prev_data = 0;
    while (i < length) {
        if (arr[i] == 0 && prev_data != 0)
        {
            last_index = i;
        }
        prev_data = arr[i];
        i++;
    }
    return last_index;
}



void write_output_files(const char* argv[]) {
    // writing output files that present the state at the end of the program
    // memout.txt
    int i = 0;
    int last_index = 0;
    uint8_t prev_data = 0;
    memout_fp = fopen(argv[4], "w");
    last_index = find_last_index_32(memory, MEM_SIZE);
    for (i = 0; i < last_index; i++) {
        fprintf(memout_fp, "%08X\n", memory[i]);
    }
    fclose(memout_fp);

    // regout.txt
    regout_fp = fopen(argv[5], "w");
    for (i = 2; i < NUM_REGS; i++) {
        fprintf(regout_fp, "%08X\n", registers[i]);
    }
    fclose(regout_fp);

    //cycles.txt
    cycles_fp = fopen(argv[8], "w");
    fprintf(cycles_fp, "%08X\n", IORegister[8]);

    // diskout.txt
    last_index = find_last_index_32(disk, DISK_SIZE);
    for (i = 0; i < last_index; i++) {
        fprintf(diskout_fp, "%08X\n", disk[i]);
    }
    i = 0;
    last_index = 0;
    // monitor.txt and monitor.yuv, finding last calue because the type is 
    while (i < MONITOR_WIDTH * MONITOR_HEIGHT) {
        if (monitor[i] == 0 && prev_data != 0)
        {
            last_index = i;
        }
        prev_data = monitor[i];
        i++;
    }

    for (i = 0; i < last_index; i++) {
        fprintf(monitor_txt_fp, "%02X\n", monitor[i]);
    }
    for (i = 0; i < MONITOR_WIDTH * MONITOR_HEIGHT; i++) {

        fputc(monitor[i], monitor_yuv_fp);
    }

}

void out_opcode(int rs, int rt, int rd)
{
    uint32_t addr = (registers[rs] + registers[rt]) % IOREG_SIZE;
    if (addr == 9 && IORegister[addr] != registers[rd]) // leds write
        fprintf(leds_fp, "%08X %08X\n", IORegister[8], registers[rd]); // display7seg write
    if (addr == 10 && IORegister[addr] != registers[rd])
        fprintf(display_fp, "%08X %08X\n", IORegister[8], registers[rd]);
    IORegister[addr] = registers[rd];
    log_hwreg("WRITE", IORegisterNames[addr], registers[rd]);
    if (addr == 22 && registers[rd] == 1) { // monitor write
        IORegister[22] = 0;
        monitor[IORegister[20] % (MONITOR_WIDTH * MONITOR_HEIGHT)] = IORegister[21] & 0xFF;
    }
    if (addr == 14 && registers[rd] != 0) // disk access
        IORegister[17] = 1;
}


void fetch_decode_execute() {
    uint32_t inst = memory[PC];
    uint32_t opcode = OPCODE(inst);
    uint32_t rd = RD(inst);
    uint32_t rs = RS(inst);
    uint32_t rt = RT(inst);
    bool bigimm = BIGIMM(inst);
    uint32_t addr;
    registers[1] = IMM8_SIGN_EXTEND(IMM8(inst));
    if (bigimm) {
        registers[1] = (int32_t)memory[PC + 1];
    }
    if (!first_cycle_bigimm && bigimm) {
        first_cycle_bigimm = true;
        return;
    }
    if (first_cycle_bigimm)
        first_cycle_bigimm = false;
    log_trace(inst);

    switch (opcode) {
    case add:
        registers[rd] = registers[rs] + registers[rt];
        break;
    case sub:
        registers[rd] = registers[rs] - registers[rt];
        break;
    case mul:
        registers[rd] = registers[rs] * registers[rt];
        break;
    case and :
        registers[rd] = registers[rs] & registers[rt];
        break;
    case or :
        registers[rd] = registers[rs] | registers[rt];
        break;
    case xor :
        registers[rd] = registers[rs] ^ registers[rt];
        break;
    case sll:
        registers[rd] = registers[rs] << registers[rt];
        break;
    case sra:
        registers[rd] = ((int32_t)registers[rs]) >> registers[rt];
        break;
    case srl:
        registers[rd] = registers[rs] >> registers[rt];
        break;
    case jal:
        registers[rd] = PC + (bigimm ? 2 : 1);
        PC = registers[rs];
        break;
    case lw:
        registers[rd] = memory[(registers[rs] + registers[rt]) % MEM_SIZE];
        break;
    case in:
        addr = (registers[rs] + registers[rt]) % IOREG_SIZE;
        registers[rd] = IORegister[addr];
        log_hwreg("READ", IORegisterNames[addr], registers[rd]);
        break;
    case beq:
        if (registers[rs] == registers[rt])
            PC = registers[rd];
        else
            PC += (bigimm ? 2 : 1);
        break;
    case bne:
        if (registers[rs] != registers[rt])
            PC = registers[rd];
        else
            PC += (bigimm ? 2 : 1);
        break;
    case blt:
        if ((int32_t)registers[rs] < (int32_t)registers[rt]) // signed comparison
            PC = registers[rd];
        else
            PC += (bigimm ? 2 : 1);
        break;
    case bgt:
        if ((int32_t)registers[rs] > (int32_t)registers[rt])// signed comparison
            PC = registers[rd];
        else
            PC += (bigimm ? 2 : 1);
        break;
    case ble:
        if ((int32_t)registers[rs] <= (int32_t)registers[rt])// signed comparison
            PC = registers[rd];
        else
            PC += (bigimm ? 2 : 1);
        break;
    case bge:
        if ((int32_t)registers[rs] >= (int32_t)registers[rt])// signed comparison
            PC = registers[rd];
        else
            PC += (bigimm ? 2 : 1);
        break;
    case sw:
        memory[(registers[rs] + registers[rt]) % MEM_SIZE] = registers[rd];
        break;
    case reti:
        PC = IORegister[7];
        irq_in_progress = false;
        break;
    case out:
        out_opcode(rs, rt, rd);
        break;
    case halt:
        halted = true;
        break;
    }
    registers[0] = 0; // fix if someone overwrite registers[0]
    if (opcode != 15 && opcode != 18 && (opcode < 9 || opcode > 14)) { // for the branching the PC value is taken care in the opcode
        PC += bigimm ? 2 : 1;
    }
    return bigimm;
}

void log_trace(uint32_t inst) { // every close cycle  write to file the clock-cycle, PC and register values
    fprintf(trace_fp, "%08X %03X %08X", IORegister[8], PC, inst);
    for (int i = 0; i < NUM_REGS; i++) {
        fprintf(trace_fp, " %08X", registers[i]);
    }
    fprintf(trace_fp, "\n");
}

void log_hwreg(const char* type, const char* name, uint32_t data) { // every access to hw-register will be docemented
    fprintf(hwregtrace_fp, "%08X %s %s %08X\n", IORegister[8], type, name, data);
}

void update_timer() { // to handle timer, we update it every clock cycle if enabled
    if (IORegister[11] == 1) {
        IORegister[12]++;
        if (IORegister[12] == IORegister[13]) {
            IORegister[12] = 0;
            IORegister[3] = 1;
        }
    }
}

void update_disk() { // updating disk if the diskcmd was activated, than updating memory and disk state after 1024 cycles
    static int disk_cycle_count = 0;
    if (IORegister[14] != 0 && IORegister[17] == 1) {
        disk_cycle_count++;
        if (disk_cycle_count == 1024) {
            disk_cycle_count = 0;
            uint32_t sector = IORegister[15];
            uint32_t buffer_addr = IORegister[16];
            for (int i = 0; i < 128; i++) { // copying sector between memory and disk, according to diskcmd value
                if (IORegister[14] == 1)
                    memory[(buffer_addr + i) % MEM_SIZE] = disk[sector * 128 + i];
                else if (IORegister[14] == 2)
                    disk[sector * 128 + i] = memory[(buffer_addr + i) % MEM_SIZE];
            }
            IORegister[14] = 0;
            IORegister[17] = 0;
            IORegister[4] = 1;
        }
    }
}

void check_and_trigger_irq() { // checking that not in irq, and if irq is active (not in the middle of bigimm function), trigger irqhandler
    if (irq_in_progress) return;

    bool irq2_pending = (irq2_index < irq2_count && (irq2_times[irq2_index] == IORegister[8]));
    // check if the clock cycle corresponds to one of irq2 cycles and than procceds to the next index
    if (irq2_pending) {
        IORegister[5] = 1;
        irq2_index++;
    }

    bool irq = (IORegister[0] & IORegister[3]) | (IORegister[1] & IORegister[4]) | (IORegister[2] & IORegister[5]);

    if (irq && !first_cycle_bigimm) {
        irq_in_progress = true;
        IORegister[7] = PC;
        PC = IORegister[6];
    }
}

// Main
int main(int argc, char* argv[]) {
    if (argc != 14) {
        fprintf(stderr, "Usage: sim memin diskin irq2in memout regout trace hwregtrace cycles leds display7seg diskout monitor.txt monitor.yuv\n");
        return 1;
    }

    load_input_files(argv);

    // Open output files
    trace_fp = fopen(argv[6], "w");
    hwregtrace_fp = fopen(argv[7], "w");
    leds_fp = fopen(argv[9], "w");
    display_fp = fopen(argv[10], "w");
    diskout_fp = fopen(argv[11], "w");
    monitor_txt_fp = fopen(argv[12], "w");
    monitor_yuv_fp = fopen(argv[13], "wb");

    while (!halted) {
        fetch_decode_execute(); // execute command and check if bigimm is in use
        update_timer();
        update_disk();
        check_and_trigger_irq(); // update irq considering the usage of bigimm
        IORegister[8]++; // update the cycle count according to the bigimm usage of next opcode
    }

    // Finalize output
    write_output_files(argv);

    // Close output files
    fclose(trace_fp); fclose(hwregtrace_fp); fclose(cycles_fp);
    fclose(leds_fp); fclose(display_fp); fclose(diskout_fp);
    fclose(monitor_txt_fp); fclose(monitor_yuv_fp);

    return 0;
}

