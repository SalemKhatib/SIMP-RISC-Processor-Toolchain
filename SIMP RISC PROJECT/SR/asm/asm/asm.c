#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>


#define MAX_LINES   4096           /* maximum words in MEM          */
#define MAX_LABEL   50             /* maximum single‑label length    */

static char  label_name[MAX_LINES][MAX_LABEL];  /* collected label identifiers */
static int   label_addr[MAX_LINES];             /* their resolved addresses    */
static char  unresolved[MAX_LINES][MAX_LABEL];  /* labels seen in immediates   */
static unsigned int mem[MAX_LINES] = { 0 };       /* simulated memory            */

static int pc = 0; /* index while emitting machine words */
static int label_count = 0; /* number of labels collected         */
static int unresolved_count = 0; /* number of big‑imm labels inserted  */
static int max_addr = 0; /* highest address written so far     */

static bool            load_source(const char* path);
static void            resolve_labels(void);
static void            dump_mem(const char* path);
static unsigned int    str2uint(const char* s);
static int             opcode_id(const char* str);
static int             reg_id(const char* str);
static void            strip_comment(char* line);
static char* ltrim(char* s);


int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source.asm> <memin.txt>\n", argv[0]);
        return 1;
    }

    if (!load_source(argv[1]))
        return 1;              /* error already printed */

    resolve_labels();          /* patch label addresses  */
    dump_mem(argv[2]);         /* write MEM[] to file    */
    puts("Done! Output saved to memin.txt");
    return 0;
}


static bool load_source(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp) {
        perror("open source");
        return false;
    }

    char line[512];
    while (fgets(line, sizeof line, fp)) {
        strip_comment(line);

        /* tokenise by space / tab / comma / newline */
        char* toks[10];
        int   ntok = 0;
        char* tok = strtok(line, " ,\t\n");
        while (tok && ntok < 10) {
            toks[ntok++] = ltrim(tok);
            tok = strtok(NULL, " ,\t\n");
        }
        if (ntok == 0) continue;          /* blank or comment‑only line */

        
        if (toks[0][strlen(toks[0]) - 1] == ':') {
            if (!isalpha((unsigned char)toks[0][0])) {
                fprintf(stderr, "Error: invalid label name '%s'\n", toks[0]);
                fclose(fp);
                return false;
            }
            toks[0][strlen(toks[0]) - 1] = '\0';          /* strip ':' */
            strcpy(label_name[label_count], toks[0]);
            label_addr[label_count] = pc;
            label_count++;

            /* shift the rest tokens left so parsing continues */
            for (int i = 0; i < ntok - 1; ++i) toks[i] = toks[i + 1];
            if (--ntok == 0) continue;    /* label‑only line */
        }

        
        if (ntok == 3 && strcmp(toks[0], ".word") == 0) {
            int addr = (int)str2uint(toks[1]);
            unsigned int value = str2uint(toks[2]);
            mem[addr] = value;
            if (addr > max_addr) max_addr = addr;
            continue;
        }

        
        if (ntok == 5) {
            unsigned int opcode = opcode_id(toks[0]);
            mem[pc] |= opcode << 24;

            for (int i = 1; i <= 3; ++i) {
                int r = reg_id(toks[i]);
                mem[pc] |= (unsigned)r << (24 - i * 4);
            }

            bool bigimm = false;
            /* immediate is a label name  → postpone */
            if (isalpha((unsigned char)toks[4][0])) {
                bigimm = true;
                mem[pc] |= (unsigned)bigimm << 8; /* set bigimm flag */
                mem[pc + 1] = 0;                   /* placeholder word */
                strcpy(unresolved[unresolved_count++], toks[4]);
                pc += 2;
            }
            else {
                unsigned int imm = str2uint(toks[4]);
                if (imm > 127 && imm < 0xFFFFFF80u) {
                    bigimm = true;                 /* imm32 path */
                    mem[pc] |= (unsigned)bigimm << 8;
                    mem[pc + 1] = imm;             /* next word full imm32 */
                    pc += 2;
                }
                else {                             /* fits in 8‑bit */
                    mem[pc] |= imm & 0xFFu;
                    pc += 1;
                }
            }
            if (pc - 1 > max_addr) max_addr = pc - 1;
            continue;
        }

    }
    fclose(fp);
    return true;
}


static void resolve_labels(void)
{
    int idx = 0;
    int unresolved_idx = 0;

    while (idx < pc) {
        bool bigimm = (mem[idx] >> 8) & 1u;
        bool is_placeholder = (mem[idx + 1] == 0);

        if (bigimm && is_placeholder) {
            for (int i = 0; i < label_count; ++i) {
                if (strcmp(label_name[i], unresolved[unresolved_idx]) == 0) {
                    mem[idx + 1] = (unsigned)label_addr[i];
                    ++unresolved_idx;
                    break;
                }
            }
            idx += 2;
        }
        else {
            ++idx;
        }
    }
}


static void dump_mem(const char* path)
{
    FILE* fp = fopen(path, "w");
    if (!fp) {
        perror("open memin");
        return;
    }
    for (int i = 0; i <= max_addr; ++i)
        fprintf(fp, "%08X\n", mem[i]);
    fclose(fp);
}


static unsigned int str2uint(const char* s)
{
    if (!s) return 0;
    if (strlen(s) > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return (unsigned)strtoul(s, NULL, 16);
    return (unsigned)strtoul(s, NULL, 10);
}

typedef struct { const char* name; int id; } Pair;

static int opcode_id(const char* str)
{
    static const Pair tab[] = {
        {"add", 0}, {"sub", 1}, {"mul", 2}, {"and", 3}, {"or", 4},  {"xor", 5},
        {"sll", 6}, {"sra", 7}, {"srl", 8}, {"beq", 9}, {"bne", 10}, {"blt", 11},
        {"bgt", 12}, {"ble", 13}, {"bge", 14}, {"jal", 15}, {"lw", 16}, {"sw", 17},
        {"reti", 18}, {"in", 19}, {"out", 20}, {"halt", 21}
    };
    for (size_t i = 0; i < sizeof tab / sizeof tab[0]; ++i)
        if (strcmp(str, tab[i].name) == 0) return tab[i].id;
    return -1;
}

static int reg_id(const char* str)
{
    static const Pair tab[] = {
        {"$zero", 0}, {"$imm", 1}, {"$v0", 2}, {"$a0", 3}, {"$a1", 4}, {"$a2", 5},
        {"$a3", 6},   {"$t0", 7},  {"$t1", 8}, {"$t2", 9}, {"$s0", 10}, {"$s1", 11},
        {"$s2", 12},  {"$gp", 13}, {"$sp", 14}, {"$ra", 15}
    };
    for (size_t i = 0; i < sizeof tab / sizeof tab[0]; ++i)
        if (strcmp(str, tab[i].name) == 0) return tab[i].id;
    return -1;
}

static void strip_comment(char* line)
{
    if (!line) return;
    for (size_t i = 0; line[i]; ++i)
        if (line[i] == '#') { line[i] = '\0'; break; }
}

static char* ltrim(char* s)
{
    while (*s && isspace((unsigned char)*s)) ++s;
    if (*s == '\0') return s;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) --end;
    end[1] = '\0';
    return s;
}
