#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 16
#define BTB_COUNT 16
#define MAXY_LENGHT 512
#define MEMORY_MAP_LENGTH 16384
long memory_map_val[MEMORY_MAP_LENGTH] = {0};
char* options;

CPU*
CPU_init(char* filename, char* option)
{
    CPU* cpu = malloc(sizeof(*cpu));
    if(!filename){
        printf("Error!! Missing filename");
        exit(0);
    }
    if (!cpu) {
        return NULL;
    }
    /* Create register files */
    cpu->regs= create_registers(REG_COUNT);
    cpu->btb= create_btb(BTB_COUNT);
    cpu->pt= create_pt(BTB_COUNT);
    cpu->filename = filename;
    cpu->fetch_latch.has_inst = 1;
    cpu->fetch_latch.halt_triggered = 0;
    cpu->register_read_latch.unfreeze = 0;
    cpu->analysis_latch.unfreeze = 0;
    cpu->decode_latch.unfreeze = 0;
    cpu->fetch_latch.unfreeze = 0;
    cpu->clock = 1;
    cpu->pc = 0;
    cpu->raw = 0;
    options = option;
    make_memory_map();

    return cpu;
}

/*
 * This function de-allocates CPU cpu.
 */
void
CPU_stop(CPU* cpu)
{
    free(cpu);
}

/*
 * This function read the file and add the data to string array 
 */
void
load_the_instructions(CPU *cpu){
    FILE *filePointer = fopen(cpu->filename, "r");
    int county = 0;
    if (filePointer == NULL)
    {
        printf("Error: could not open file %s", cpu->filename);
    }
    char buffer[MAXY_LENGHT];
    while (fgets(buffer, MAXY_LENGHT, filePointer)){
        strcpy(cpu->instructions[county],buffer);
        county++;
    }
    cpu->instructionLength = county ;
    fclose(filePointer);
}

/*
 * This function prints the content of the registers.
 */
void
print_registers(CPU *cpu){
    
    
    printf("================================\n\n");

    printf("=============== STATE OF ARCHITECTURAL REGISTER FILE ==========\n\n");

    printf("--------------------------------\n");
    for (int reg=0; reg<REG_COUNT; reg++) {
        printf("REG[%2d]   |   Value=%ld  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n\n");
}

void print_display(CPU *cpu){
    printf("================================\n");
    printf("Clock Cycle #: %d\n", cpu->clock);
    printf("--------------------------------\n");

   for (int reg=0; reg<REG_COUNT; reg++) {
       
        printf("REG[%2d]   |   Value=%ld  \n",reg,cpu->regs[reg].value);
        printf("--------------------------------\n");
    }
    printf("================================\n");
    printf("\n");

}

void print_btb(CPU *cpu){
    printf("\n");
    printf("============ BTB =================================\n");
    printf("\n");
    
    for (int i=0;i<BTB_COUNT; i++){
        printf("|	 BTB[%2d]	|	Tag=%d   |   Target=%d   |\n", i,cpu->btb[i].tag,cpu->btb[i].target);
    }
}

void print_pt(CPU *cpu){
    printf("\n");
    printf("============ Prediction Table  ==================\n");
    printf("\n");
    
    for (int i=0;i<BTB_COUNT; i++){
        printf("|	 PT[%2d]	|  Pattern=%d   |\n", i,cpu->pt[i].pattern);
    }
}

int load_the_memory(){
    char *filename = "memory_map.txt"; 
    FILE *filePointer = fopen(filename, "r");
    int county = 0; 
    long n;
    if (filePointer == NULL) {
        printf("Error: could not open file %s", filename);
    }
    while (fscanf(filePointer, " %ld", &n) == 1) {
        memory_map_val[county] = n;
        county++;
    }
    fclose(filePointer);
    return (-1);
}

void make_memory_map(){
    char c;
    FILE *fptr1, *fptr2;
    char read_file[] = "memory_map.txt";
    char write_file[] = "output_memory_map.txt";
    fptr1 = fopen(read_file, "r");
    if (fptr1 == NULL)
    {
        printf("Cannot open file %s \n", read_file);
        exit(0);
    }
    fptr2 = fopen(write_file, "w+");
    if (fptr2 == NULL)
    {
        printf("Cannot open file %s \n", write_file);
        exit(0);
    }
    c = fgetc(fptr1);
    while (c != EOF)
    {
        fputc(c, fptr2);
        c = fgetc(fptr1);
    }
    fclose(fptr1);
    fclose(fptr2);
}

int write_the_memory(long val,int num){
    num = num/4;
    memory_map_val[num] = val;

    char *filename = "output_memory_map.txt"; 
    FILE *filePointer = fopen(filename, "w+");
    
    int county = 0;
    int n;
    if (filePointer == NULL)
    {
        printf("Error: could not open file %s", filename);
    }

    for (int i=0; i<MEMORY_MAP_LENGTH; i++){
        fprintf(filePointer, "%ld ", memory_map_val[i]);
    }
    fclose(filePointer);
    return (-1);
}

/*
 *  CPU CPU simulation loop
 */
int
CPU_run(CPU* cpu)
{
    load_the_instructions(cpu);
    cpu->hazard = 0;
    simulate(cpu);
    print_registers(cpu);
    cpu->ipc = (double)cpu->instructionLength/(double)cpu->clock;
    printf("Stalled cycles due to data hazard: %d\n", cpu->hazard);
    printf("Total execution cycles: %d\n",cpu->clock);
    printf("Total instruction simulated: %d\n", cpu->instructionLength);
    printf("IPC: %6f\n",cpu->ipc);
    return 0;
}

Register*
create_registers(int size){
    Register* regs = malloc(sizeof(*regs) * size);
    if (!regs) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        regs[i].value = 0;
        regs[i].is_writing = 0;
    }
    return regs;
}

Btb*
create_btb(int size){
    Btb* btb = malloc(sizeof(*btb) * size);
    if (!btb) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        btb[i].tag = -1;
        btb[i].target = -1;
    }
    return btb;
}

Pt*
create_pt(int size){
    Pt* pt = malloc(sizeof(*pt) * size);
    if (!pt) {
        return NULL;
    }
    for (int i=0; i<size; i++){
        pt[i].pattern = 3;
    }
    return pt;
}

void
freed_registers(CPU* cpu,int size){
    for (int i=0; i<size; i++){
        cpu->regs[i].freed_this_cycle = 0;
    }
}

/*
*The Main Pipeline Implementation
*/ 
void simulate(CPU* cpu){
    if(strcmp(options,"pipeline") == 0){
        printf("================================");
        printf("\nClock Cycle #: %d\n",cpu->clock);
        printf("--------------------------------\n");
    }

    load_the_memory();
    int last_inst = 0;
    for(;;){
        if(writeback_unit(cpu)){
            // print_display(cpu);
            break;
        }
        memory2_unit(cpu);
        memory1_unit(cpu); 
        branch_unit(cpu); 
        divider_unit(cpu); 
        multiplier_unit(cpu);
        adder_unit(cpu); 
        register_read_unit(cpu); 
        analysis_unit(cpu); 
        decode_unit(cpu); 
        fetch_unit(cpu);
        clear_forwarding(cpu);
        // print_btb(cpu);
        // print_pt(cpu);
        // print_display(cpu);
        cpu->clock++;
        // printf("Ha JAT: %d",cpu->hazard);
        cpu->fetch_latch.has_inst = 1;  
        if(cpu->clock > 1000){
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("\n================================");
            printf("\nClock Cycle #: %d\n",cpu->clock);
            printf("--------------------------------\n");
        }
    }
    // print_display(cpu);
    if(strcmp(options,"pipeline") == 0){
    printf("\n================================\n");
    }
    //TODO Loop Or recursion?
    // Create Struct or F!!!
    // printf("Hahaha");
}

int writeback_unit(CPU* cpu){
    if(cpu->writeback_latch.has_inst == 1 && cpu->writeback_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("WB             : %s",cpu->instructions[cpu->writeback_latch.pc]);
        }
        printf("Buffer: %d\n",cpu->writeback_latch.buffer);
        if(strcmp(cpu->writeback_latch.opcode,"ret") == 0){
            cpu->writeback_latch.has_inst = 0;
            return(1);
        }
        else if(strcmp(cpu->writeback_latch.opcode,"st") == 0){
            return(0);
        }
        if(strcmp(cpu->writeback_latch.opcode,"bez") == 0 || strcmp(cpu->writeback_latch.opcode,"bgez") == 0 || strcmp(cpu->writeback_latch.opcode,"blez") == 0 | strcmp(cpu->writeback_latch.opcode,"bgtz") == 0 || strcmp(cpu->writeback_latch.opcode,"bltz") == 0){}
        else{
            cpu->regs[atoi(cpu->writeback_latch.rg1+1)].value = cpu->writeback_latch.buffer;
        }
        if(cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing > 0){
            // printf("Freeing: %s\n",cpu->writeback_latch.rg1);
            cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing--;
        }
        return(0);
    }
    else if(cpu->writeback_latch.halt_triggered==1){
        cpu->writeback_latch.halt_triggered = 0;
        return(0);
    }
}

void memory2_unit(CPU* cpu){
    if(cpu->memory2_latch.has_inst == 1 && cpu->memory2_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->memory2_latch.pc],"") == 0){
            cpu->writeback_latch = cpu->memory2_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("Mem2           : %s",cpu->instructions[cpu->memory2_latch.pc]);
        }
        if (strcmp(cpu->memory2_latch.opcode,"ld")==0){
            if (cpu->memory2_latch.or1[0] == 82){
                cpu->memory2_latch.buffer = memory_map_val[cpu->memory2_latch.rg2_val/4];
                // cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value = load_the_memory(cpu->memory2_latch.rg2_val);
            }
            else{
                cpu->memory2_latch.buffer = memory_map_val[atoi(cpu->memory2_latch.or1+1)/4];
                // cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value = load_the_memory(atoi(cpu->memory2_latch.or1+1));
            }
        }
        else if (strcmp(cpu->memory2_latch.opcode,"st")==0){
            if (cpu->memory2_latch.or1[0] == 82){
                cpu->memory2_latch.buffer = write_the_memory(cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value,cpu->memory2_latch.rg2_val);
            }
            else{
                cpu->memory2_latch.buffer = write_the_memory(cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value,atoi(cpu->memory2_latch.or1+1));
            }
        }
        cpu->writeback_latch = cpu->memory2_latch;
    }
    else if (cpu->memory2_latch.halt_triggered==1){
        cpu->writeback_latch = cpu->memory2_latch;
    }
    else{
        cpu->writeback_latch = cpu->memory2_latch;
    }
}

void memory1_unit(CPU* cpu){
    if(cpu->memory1_latch.has_inst == 1 && cpu->memory1_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->memory1_latch.pc],"") == 0){
            cpu->memory2_latch = cpu->memory1_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("Mem1           : %s",cpu->instructions[cpu->memory1_latch.pc]);
        }
        if(strcmp(cpu->memory1_latch.opcode,"add") == 0 || strcmp(cpu->memory1_latch.opcode,"sub") == 0 || strcmp(cpu->memory1_latch.opcode,"set") == 0 || strcmp(cpu->memory1_latch.opcode,"mul") == 0 || strcmp(cpu->memory1_latch.opcode,"div") == 0){
            strcpy(cpu->mem1_reg,cpu->memory1_latch.rg1);
            cpu->mem1_val = cpu->memory1_latch.buffer;
        }
        else{
            strcpy(cpu->mem1_reg,"NULL");
            cpu->mem1_val = -1;
        }
        cpu->memory2_latch = cpu->memory1_latch;
    }
    else if(cpu->memory1_latch.halt_triggered==1){
        cpu->memory2_latch = cpu->memory1_latch;
    }
    else{
        cpu->memory2_latch = cpu->memory1_latch;
    }
}

void branch_unit(CPU* cpu){
    if(cpu->branch_latch.has_inst == 1 && cpu->branch_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->branch_latch.pc],"") == 0){
            cpu->memory1_latch = cpu->branch_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("BR             : %s",cpu->instructions[cpu->branch_latch.pc]);
        }
        if(strcmp(cpu->branch_latch.opcode,"add") == 0 || strcmp(cpu->branch_latch.opcode,"sub") == 0 || strcmp(cpu->branch_latch.opcode,"set") == 0 || strcmp(cpu->branch_latch.opcode,"mul") == 0 || strcmp(cpu->branch_latch.opcode,"div") == 0){
            strcpy(cpu->br_reg,cpu->branch_latch.rg1);
            cpu->br_val = cpu->branch_latch.buffer;
        }
        else{
            strcpy(cpu->br_reg,"NULL");
            cpu->br_val = -1;
        }
        if(strcmp(cpu->branch_latch.opcode,"bez") == 0){
            if(cpu->regs[atoi(cpu->branch_latch.rg1+1)].value == 0){
                if(cpu->branch_latch.branch_taken == 1){
                    printf("Branch Already Taken");
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                }
                else{
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    return;
                }
            }
            else{
                if(cpu->branch_latch.branch_taken == 1){
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    printf("bez not taken but should be Taken");
                    return;
                }
                else{
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern > 0){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern--;
                    }
                    printf("bez branch not taken");
                }
            }
        }
        else if(strcmp(cpu->branch_latch.opcode,"bgez") == 0){
            if(cpu->regs[atoi(cpu->branch_latch.rg1+1)].value >= 0){
                if(cpu->branch_latch.branch_taken == 1){
                    printf("Branch Already Taken");
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                }
                else{
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    return;
                }
            }
            else{
                if(cpu->branch_latch.branch_taken == 1){
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    printf("bez not taken but should be Taken");
                    return;
                }
                else{
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern > 0){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern--;
                    }
                    printf("bez branch not taken");
                }
            }
        }
        else if(strcmp(cpu->branch_latch.opcode,"blez") == 0){
            if(cpu->regs[atoi(cpu->branch_latch.rg1+1)].value >= 0){
                if(cpu->branch_latch.branch_taken == 1){
                    printf("Branch Already Taken");
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                }
                else{
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    return;
                }
            }
            else{
                if(cpu->branch_latch.branch_taken == 1){
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    printf("bez not taken but should be Taken");
                    return;
                }
                else{
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern > 0){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern--;
                    }
                    printf("bez branch not taken");
                }
            }
        }
        else if(strcmp(cpu->branch_latch.opcode,"bgtz") == 0){
            if(cpu->regs[atoi(cpu->branch_latch.rg1+1)].value >= 0){
                if(cpu->branch_latch.branch_taken == 1){
                    printf("Branch Already Taken");
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                }
                else{
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    return;
                }
            }
            else{
                if(cpu->branch_latch.branch_taken == 1){
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    printf("bez not taken but should be Taken");
                    return;
                }
                else{
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern > 0){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern--;
                    }
                    printf("bez branch not taken");
                }
            }
        }
        else if(strcmp(cpu->branch_latch.opcode,"bltz") == 0){
            if(cpu->regs[atoi(cpu->branch_latch.rg1+1)].value >= 0){
                if(cpu->branch_latch.branch_taken == 1){
                    printf("Branch Already Taken");
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                }
                else{
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    return;
                }
            }
            else{
                if(cpu->branch_latch.branch_taken == 1){
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].tag = 1;
                    cpu->btb[(cpu->branch_latch.instAddr/4)%16].target = atoi(cpu->branch_latch.or1+1);
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern < 7){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern++;
                    }
                    cpu->pc = cpu->btb[(cpu->branch_latch.instAddr/4)%16].target/4;
                    cpu->fetch_latch.has_inst = 0;
                    cpu->decode_latch.has_inst = 0;
                    cpu->analysis_latch.has_inst = 0;
                    cpu->register_read_latch.has_inst = 0;
                    cpu->adder_latch.has_inst = 0;
                    cpu->multiplier_latch.has_inst = 0;
                    cpu->divider_latch.has_inst = 0;
                    cpu->memory1_latch = cpu->branch_latch;
                    cpu->branch_latch.has_inst = 0;
                    printf("bez not taken but should be Taken");
                    return;
                }
                else{
                    if(cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern > 0){
                        cpu->pt[(cpu->branch_latch.instAddr/4)%16].pattern--;
                    }
                    printf("bez branch not taken");
                }
            }
        }
        cpu->memory1_latch = cpu->branch_latch;
    }
    else if (cpu->branch_latch.halt_triggered==1){
        cpu->memory1_latch = cpu->branch_latch;
    }
    else{
        cpu->memory1_latch = cpu->branch_latch;
    }
}

void divider_unit(CPU* cpu){
    if(cpu->divider_latch.has_inst == 1 && cpu->divider_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->divider_latch.pc],"") == 0){
            cpu->branch_latch = cpu->divider_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("DIV            : %s",cpu->instructions[cpu->divider_latch.pc]);
        }
        if(strcmp(cpu->divider_latch.opcode,"div") == 0){
            //TODO Write Divide Logic
            if(cpu->divider_latch.or1[0] == 82 && cpu->divider_latch.or2[0] == 82){
                if(strcmp(cpu->divider_latch.or1,cpu->mem1_reg) == 0 && strcmp(cpu->divider_latch.or2,cpu->mem1_reg) == 0){
                    cpu->divider_latch.buffer = cpu->mem1_val / cpu->mem1_val;
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->mem1_reg) == 0){
                    cpu->divider_latch.buffer = cpu->mem1_val / cpu->divider_latch.rg3_val;
                }
                else if(strcmp(cpu->divider_latch.or2,cpu->mem1_reg) == 0){
                    cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / cpu->mem1_val;
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->br_reg) == 0 && strcmp(cpu->divider_latch.or2,cpu->br_reg) == 0){
                    cpu->divider_latch.buffer = cpu->br_val / cpu->br_val;
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->br_reg) == 0){
                    cpu->divider_latch.buffer = cpu->br_val / cpu->divider_latch.rg3_val;
                }
                else if (strcmp(cpu->divider_latch.or2,cpu->br_reg) == 0){
                    cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / cpu->br_val;
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->div_reg) == 0 && strcmp(cpu->divider_latch.or2,cpu->div_reg) == 0){
                    cpu->divider_latch.buffer = cpu->div_val / cpu->div_val;
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->div_reg) == 0){
                    cpu->divider_latch.buffer = cpu->div_val / cpu->divider_latch.rg3_val;
                }
                else if(strcmp(cpu->divider_latch.or2,cpu->div_reg) == 0){
                    cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / cpu->div_val;
                }
                else{
                    cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / cpu->divider_latch.rg3_val;
                }
            }
            else if (cpu->divider_latch.or1[0] == 82){
                if(strcmp(cpu->divider_latch.or1,cpu->mem1_reg) == 0){
                    cpu->divider_latch.buffer = cpu->mem1_val / atoi(cpu->divider_latch.or2+1);
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->br_reg) == 0){
                    cpu->divider_latch.buffer = cpu->br_val / atoi(cpu->divider_latch.or2+1);
                }
                else if(strcmp(cpu->divider_latch.or1,cpu->div_reg) == 0){
                    cpu->divider_latch.buffer = cpu->div_val / atoi(cpu->divider_latch.or2+1);
                }
                else{
                    cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / atoi(cpu->divider_latch.or2+1);
                }
            }
            else if (cpu->divider_latch.or2[0] == 82){
                if(strcmp(cpu->divider_latch.or2,cpu->mem1_reg) == 0){
                    cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / cpu->mem1_val;
                }
                else if(strcmp(cpu->divider_latch.or2,cpu->br_reg) == 0){
                    cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / cpu->br_val;
                }
                else if(strcmp(cpu->divider_latch.or2,cpu->div_reg) == 0){
                    cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / cpu->div_val;
                }
                else{
                    cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / cpu->divider_latch.rg3_val;
                }
            }
            else{
                cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / atoi(cpu->divider_latch.or2+1);
            }
        }
        if(strcmp(cpu->divider_latch.opcode,"add") == 0 || strcmp(cpu->divider_latch.opcode,"sub") == 0 || strcmp(cpu->divider_latch.opcode,"set") == 0 || strcmp(cpu->divider_latch.opcode,"mul") == 0 || strcmp(cpu->divider_latch.opcode,"div") == 0){
        // if(strcmp(cpu->divider_latch.opcode,"div") == 0){
            strcpy(cpu->div_reg,cpu->divider_latch.rg1);
            cpu->div_val = cpu->divider_latch.buffer;
        }
        else{
            strcpy(cpu->div_reg,"NULL");
            cpu->div_val = -1;
        }
        cpu->branch_latch = cpu->divider_latch;
    }
    else if(cpu->divider_latch.halt_triggered==1){
        cpu->branch_latch = cpu->divider_latch;
    }
}

void multiplier_unit(CPU* cpu){
    if(cpu->multiplier_latch.has_inst == 1 && cpu->multiplier_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->multiplier_latch.pc],"") == 0){
            cpu->divider_latch = cpu->multiplier_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("MUL            : %s",cpu->instructions[cpu->multiplier_latch.pc]);
        }
        if(strcmp(cpu->multiplier_latch.opcode,"mul") == 0){
            //TODO Write Multiplication Logic
            if(cpu->multiplier_latch.or1[0] == 82 && cpu->multiplier_latch.or2[0] == 82){
                if(strcmp(cpu->multiplier_latch.or1,cpu->div_reg) == 0 && strcmp(cpu->multiplier_latch.or2,cpu->div_reg) == 0){
                    cpu->multiplier_latch.buffer = cpu->div_val * cpu->div_val;
                }
                else if(strcmp(cpu->multiplier_latch.or1,cpu->div_reg) == 0){
                    cpu->multiplier_latch.buffer = cpu->div_val * cpu->multiplier_latch.rg3_val;
                }
                else if(strcmp(cpu->multiplier_latch.or2,cpu->div_reg) == 0){
                    cpu->multiplier_latch.buffer = cpu->multiplier_latch.rg2_val * cpu->div_val;
                }
                else{
                    cpu->multiplier_latch.buffer = cpu->multiplier_latch.rg2_val * cpu->multiplier_latch.rg3_val;
                } 
            }
            else if(cpu->multiplier_latch.or1[0] == 82){
                if(strcmp(cpu->multiplier_latch.or1,cpu->div_reg) == 0){
                    cpu->multiplier_latch.buffer = cpu->div_val * atoi(cpu->multiplier_latch.or2+1);
                }
                else{
                    cpu->multiplier_latch.buffer = cpu->multiplier_latch.rg2_val * atoi(cpu->multiplier_latch.or2+1);
                } 
            }
            else if(cpu->multiplier_latch.or2[0] == 82){
                if(strcmp(cpu->multiplier_latch.or2,cpu->div_reg) == 0){
                    cpu->multiplier_latch.buffer = atoi(cpu->multiplier_latch.or1+1) * cpu->div_val;
                }
                else{
                    cpu->multiplier_latch.buffer = atoi(cpu->multiplier_latch.or1+1) * cpu->multiplier_latch.rg3_val;
                }
            }
            else{
                cpu->multiplier_latch.buffer = atoi(cpu->multiplier_latch.or1+1) * atoi(cpu->multiplier_latch.or2+1);
            }
        }
        if(strcmp(cpu->multiplier_latch.opcode,"add") == 0 || strcmp(cpu->multiplier_latch.opcode,"sub") == 0 || strcmp(cpu->multiplier_latch.opcode,"set") == 0 || strcmp(cpu->multiplier_latch.opcode,"mul") == 0){
        // if(strcmp(cpu->multiplier_latch.opcode,"mul") == 0){
            strcpy(cpu->mul_reg,cpu->multiplier_latch.rg1);
            cpu->mul_val = cpu->multiplier_latch.buffer;
        }
        else{
            strcpy(cpu->mul_reg,"NULL");
            cpu->mul_val = -1;
        }
        printf("Buffer: %d\n",cpu->multiplier_latch.buffer);
        cpu->divider_latch = cpu->multiplier_latch;
    }
    else if (cpu->multiplier_latch.halt_triggered==1){
        cpu->divider_latch = cpu->multiplier_latch;
    }
}

void adder_unit(CPU* cpu){
    if(cpu->adder_latch.has_inst == 1 && cpu->adder_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->adder_latch.pc],"") == 0){
            cpu->multiplier_latch = cpu->adder_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("ADD            : %s",cpu->instructions[cpu->adder_latch.pc]);
        }
        if(strcmp(cpu->adder_latch.opcode,"add") == 0){
            //TODO Write Subtraction Logic
            if (cpu->adder_latch.or1[0] == 82 && cpu->adder_latch.or2[0] == 82){
                if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0 && strcmp(cpu->adder_latch.or2,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->add_val + cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->add_val + cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0 && strcmp(cpu->adder_latch.or2,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->mul_val + cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->mul_val + cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0 && strcmp(cpu->adder_latch.or2,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->div_val + cpu->div_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->div_val + cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + cpu->div_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->br_reg) == 0 && strcmp(cpu->adder_latch.or2,cpu->br_reg) == 0){
                    cpu->adder_latch.buffer = cpu->br_val + cpu->br_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->br_reg) == 0){
                    cpu->adder_latch.buffer = cpu->br_val + cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->br_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + cpu->br_val;
                }
                else{
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + cpu->adder_latch.rg3_val;
                }
            }
            else if (cpu->adder_latch.or1[0] == 82){
                if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->add_val + atoi(cpu->adder_latch.or2+1);
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->mul_val + atoi(cpu->adder_latch.or2+1);
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->div_val + atoi(cpu->adder_latch.or2+1);
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->br_reg) == 0){
                    cpu->adder_latch.buffer = cpu->br_val + atoi(cpu->adder_latch.or2+1);
                }
                else{
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + atoi(cpu->adder_latch.or2+1);
                }
            }
            else if (cpu->adder_latch.or2[0] == 82){
                if(strcmp(cpu->adder_latch.or2,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + cpu->div_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->br_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + cpu->br_val;
                }
                else{
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + cpu->adder_latch.rg3_val;
                }
            }
            else{
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + atoi(cpu->adder_latch.or2+1);
            }
        }
        else if(strcmp(cpu->adder_latch.opcode,"sub") == 0){
            //TODO Write Subtraction Logic
            if (cpu->adder_latch.or1[0] == 82 && cpu->adder_latch.or2[0] == 82){
                if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->add_val - cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->mul_val - cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->div_val - cpu->adder_latch.rg3_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - cpu->div_val;
                }
                else{
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - cpu->adder_latch.rg3_val;
                }
            }
            else if (cpu->adder_latch.or1[0] == 82){
                if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->add_val - atoi(cpu->adder_latch.or2+1);
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->mul_val - atoi(cpu->adder_latch.or2+1);
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->div_val - atoi(cpu->adder_latch.or2+1);
                }
                else{
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - atoi(cpu->adder_latch.or2+1);
                }
            }
            else if (cpu->adder_latch.or2[0] == 82){
                if(strcmp(cpu->adder_latch.or2,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or2,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - cpu->div_val;
                }
                else{
                    cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - cpu->adder_latch.rg3_val;
                }
            }
            else{
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - atoi(cpu->adder_latch.or2+1);
            }
        }
        else if(strcmp(cpu->adder_latch.opcode,"set") == 0){
            //TODO Write Set Logic
            if (cpu->adder_latch.or1[0] == 82){
                if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0){
                    cpu->adder_latch.buffer = cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0){
                    cpu->adder_latch.buffer = cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0){
                    cpu->adder_latch.buffer = cpu->div_val;
                }
                else{
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val;
                }  
            }
            else{
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1);
            }  
        }
        else if(strcmp(cpu->adder_latch.opcode,"ld") == 0){
            if (cpu->adder_latch.or1[0] == 82){
                if(strcmp(cpu->adder_latch.or1,cpu->add_reg) == 0){
                    cpu->adder_latch.rg2_val = cpu->add_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->mul_reg) == 0){
                    cpu->adder_latch.rg2_val = cpu->mul_val;
                }
                else if(strcmp(cpu->adder_latch.or1,cpu->div_reg) == 0){
                    cpu->adder_latch.rg2_val = cpu->div_val;
                }
                else{
                    cpu->adder_latch.rg2_val = cpu->adder_latch.rg2_val;
                }
            }
        }
        if(strcmp(cpu->adder_latch.opcode,"st") != 0){
            cpu->regs[atoi(cpu->adder_latch.rg1+1)].is_writing++;
        }
        if(strcmp(cpu->adder_latch.opcode,"add") == 0 || strcmp(cpu->adder_latch.opcode,"sub") == 0 || strcmp(cpu->adder_latch.opcode,"set") == 0){
            strcpy(cpu->add_reg,cpu->adder_latch.rg1);
            cpu->add_val = cpu->adder_latch.buffer;            
        }
        else{
            strcpy(cpu->add_reg,"NULL");
            cpu->add_val = -1;
        }
        cpu->multiplier_latch = cpu->adder_latch;
    }
    else if (cpu->adder_latch.halt_triggered==1){
        cpu->multiplier_latch = cpu->adder_latch;
    }
}

void  register_read_unit(CPU* cpu){
    int stallit1 = 0;
    int stallit2 = 0;
    if(cpu->register_read_latch.has_inst == 1 && cpu->register_read_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->register_read_latch.pc],"") == 0){
            cpu->adder_latch = cpu->register_read_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("RR             : %s",cpu->instructions[cpu->register_read_latch.pc]);
        }
        cpu->register_read_latch.buffer = -1;   //Initialize Buffer Value
        // Check if Register is been writen!!!!
        if(cpu->register_read_latch.unfreeze == 1){
            cpu->hazard++;
            // printf("Unfreezing Pipeline\n");
            cpu->analysis_latch.halt_triggered = 0;
            cpu->decode_latch.halt_triggered = 0;
            cpu->fetch_latch.halt_triggered = 0;
            cpu->register_read_latch.unfreeze = 0;
        }
        if (strcmp(cpu->register_read_latch.opcode,"st") == 0 && cpu->regs[atoi(cpu->register_read_latch.rg1+1)].is_writing > 0){
                if(strcmp(cpu->register_read_latch.rg1,cpu->add_reg) == 0 || strcmp(cpu->register_read_latch.rg1,cpu->mul_reg) == 0 || strcmp(cpu->register_read_latch.rg1,cpu->div_reg) == 0){
                    // Dont Stall
                }
                else{
                    // printf("(st)Blocked because of: %s\n",cpu->register_read_latch.rg1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    return;
                }
        }

        if (strcmp(cpu->register_read_latch.opcode,"ld") == 0 && cpu->register_read_latch.or1[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing > 0){
                // if(strcmp(cpu->register_read_latch.or1,cpu->add_reg) == 0 || strcmp(cpu->register_read_latch.or1,cpu->mul_reg) == 0 || strcmp(cpu->register_read_latch.or1,cpu->div_reg) == 0){
                //     // Dont Stall
                // }
                // else{
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    return;
                // }
        }
        
        if(strcmp(cpu->register_read_latch.opcode,"add") == 0 || strcmp(cpu->register_read_latch.opcode,"sub") == 0 || strcmp(cpu->register_read_latch.opcode,"set") == 0 || strcmp(cpu->register_read_latch.opcode,"mul") == 0  || strcmp(cpu->register_read_latch.opcode,"div") == 0){
            
            if (cpu->register_read_latch.or1[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing > 0){
                // Halt because of OR1
                // printf("Blocked because of: %s\n",cpu->register_read_latch.or1);
                stallit1 = 1;
            }
            if(cpu->register_read_latch.or2[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or2+1)].is_writing > 0){
                // Halt because of OR2
                // printf("Blocked because of: %s\n",cpu->register_read_latch.or2);
                stallit2 = 1;
            }
            if(strcmp(cpu->register_read_latch.or1,cpu->add_reg) == 0 || strcmp(cpu->register_read_latch.or1,cpu->mul_reg) == 0 ){
                stallit1 = 0;
            }
            if(strcmp(cpu->register_read_latch.or2,cpu->add_reg) == 0 || strcmp(cpu->register_read_latch.or2,cpu->mul_reg) == 0 ){
                stallit2 = 0;
            }
            printf("Add: %s\nMul: %s\nDiv: %s\n",cpu->add_reg,cpu->mul_reg,cpu->div_reg);
            printf("Stall1: %d\nStall2: %d\n",stallit1,stallit2);
            if(stallit1 == 1 || stallit2 == 1){
                cpu->adder_latch.halt_triggered = 1;
                cpu->register_read_latch.halt_triggered = 1;
                cpu->analysis_latch.halt_triggered = 1;
                cpu->decode_latch.halt_triggered = 1;
                cpu->fetch_latch.halt_triggered = 1;
                return;
            }
        }
        // Read the Register Values
        if (cpu->register_read_latch.or1[0] == 82){   // Check if operand is register?
            strcpy(cpu->register_read_latch.rg2,cpu->register_read_latch.or1);
            cpu->register_read_latch.rg2_val = cpu->regs[atoi(cpu->register_read_latch.or1+1)].value;
        }
        if (cpu->register_read_latch.or2[0] == 82){  // Check if operand is register?
            strcpy(cpu->register_read_latch.rg3,cpu->register_read_latch.or2);
            cpu->register_read_latch.rg3_val = cpu->regs[atoi(cpu->register_read_latch.or2+1)].value;
        }
        cpu->register_read_latch.rg1_val = cpu->regs[atoi(cpu->register_read_latch.rg1+1)].value;
        cpu->adder_latch = cpu->register_read_latch;
    }
    else if (cpu->register_read_latch.halt_triggered==1){
        int sstall1 = 0;
        int sstall2 = 0;
        int frw1 = 0;
        int frw2 = 0;
        // printf("RR Halted\n");
        cpu->hazard++;
        if(strcmp(options,"pipeline") == 0){
            printf("RR             : %s",cpu->instructions[cpu->register_read_latch.pc]);
        }
        if(strcmp(cpu->register_read_latch.opcode,"add") == 0 || strcmp(cpu->register_read_latch.opcode,"sub") == 0 || strcmp(cpu->register_read_latch.opcode,"set") == 0 || strcmp(cpu->register_read_latch.opcode,"mul") == 0 || strcmp(cpu->register_read_latch.opcode,"div") == 0){
            if (cpu->register_read_latch.or1[0] == 82){
                if(cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing < 1){
                    // Halt because of OR1
                    // printf("Unblock because of: %s\n",cpu->register_read_latch.or1);
                    sstall1 = 1;
                }
                else{
                    sstall1 = 0;
                }
            }
            else{
                sstall1 = 1;
            }
            if(cpu->register_read_latch.or2[0] == 82){
                if(cpu->regs[atoi(cpu->register_read_latch.or2+1)].is_writing < 1){
                    // Halt because of OR2   
                    sstall2 = 1;
                }
                else{
                    sstall2 = 0;
                }
            }
            else{
            sstall2 = 1; 
            }
            if(strcmp(cpu->register_read_latch.or1,cpu->add_reg) == 0 || strcmp(cpu->register_read_latch.or1,cpu->mul_reg) == 0 ){
                frw1 = 1;
            }
            if(strcmp(cpu->register_read_latch.or2,cpu->add_reg) == 0 || strcmp(cpu->register_read_latch.or2,cpu->mul_reg) == 0){
                frw2 = 1;
            }
            printf("Add: %s\nMul: %s\nDiv: %s\n",cpu->add_reg,cpu->mul_reg,cpu->div_reg);
            printf("Stall1: %d\nStall2: %d\n",sstall1,sstall2);
            if(frw1 == 1 || frw2 == 1){
                // cpu->hazard++;
                // printf("Unfreezing Pipeline\n");
                cpu->analysis_latch.halt_triggered = 0;
                cpu->decode_latch.halt_triggered = 0;
                cpu->fetch_latch.halt_triggered = 0;
                cpu->register_read_latch.unfreeze = 0;
                cpu->register_read_latch.halt_triggered = 0;
                cpu->adder_latch = cpu->register_read_latch;
                return;
            }
            if(sstall1 == 1 && sstall2 == 1){
                cpu->register_read_latch.unfreeze = 1;
                cpu->register_read_latch.halt_triggered = 0;
                return;
            }
        }
        if (strcmp(cpu->register_read_latch.opcode,"st") == 0 && cpu->regs[atoi(cpu->register_read_latch.rg1+1)].is_writing < 1){
                // printf("(st)Unblock because of: %s\n",cpu->register_read_latch.rg1);
                cpu->register_read_latch.unfreeze = 1;
                cpu->register_read_latch.halt_triggered = 0;
                return;
        }
        if (strcmp(cpu->register_read_latch.opcode,"ld") == 0 && cpu->register_read_latch.or1[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing < 1){
                cpu->register_read_latch.unfreeze = 1;
                cpu->register_read_latch.halt_triggered = 0;
                return;
        }
    }
}

void analysis_unit(CPU* cpu){
    if(cpu->analysis_latch.has_inst == 1 && cpu->analysis_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->analysis_latch.pc],"") == 0){
            cpu->register_read_latch = cpu->analysis_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("IA             : %s",cpu->instructions[cpu->analysis_latch.pc]);
        }
        cpu->register_read_latch=cpu->analysis_latch;
    }
    else if(cpu->analysis_latch.halt_triggered==1){
        if(strcmp(options,"pipeline") == 0){
            printf("IA             : %s",cpu->instructions[cpu->analysis_latch.pc]);
        }
    }
}

void decode_unit(CPU* cpu){
    if(cpu->decode_latch.has_inst == 1 && cpu->decode_latch.halt_triggered==0){
        if(strcmp(cpu->instructions[cpu->decode_latch.pc],"")  == 0){
            cpu->analysis_latch = cpu->decode_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("ID             : %s",cpu->instructions[cpu->decode_latch.pc]);
        }
        cpu->analysis_latch = cpu->decode_latch;
    }
    else if (cpu->decode_latch.halt_triggered==1){
        if(strcmp(options,"pipeline") == 0){
            printf("ID             : %s",cpu->instructions[cpu->decode_latch.pc]);
        }
    }
}

void fetch_unit(CPU* cpu){
    if(cpu->fetch_latch.has_inst == 1 && cpu->fetch_latch.halt_triggered == 0){

        cpu->fetch_latch.pc = cpu->pc;
        cpu->pc++;
        char str1[128];
        if(strcmp(cpu->instructions[cpu->fetch_latch.pc],"") == 0){
            cpu->decode_latch = cpu->fetch_latch;
            return;
        }
        if(strcmp(options,"pipeline") == 0){
            printf("IF             : %s",cpu->instructions[cpu->fetch_latch.pc]);
        }
        strcpy(str1,cpu->instructions[cpu->fetch_latch.pc]);

        //-----------------------------Dynamic Spliting---------------------------------------------
        char **token= NULL;
        char *p = str1;
        char *sepa=" ";
        size_t  arr_len = 0,q;
        for (;;)
        {
            p += strspn(p, sepa);
            if (!(q = strcspn(p, sepa)))
                    break;
            if (q)
            {
                    token = realloc(token, (arr_len+1) * sizeof(char *));
                    token[arr_len] = malloc(q+1);
                    strncpy(token[arr_len], p, q);
                    token[arr_len][q] = 0;
                    arr_len++;
                    p += q;
            }
        }
        token = realloc(token, (arr_len+1) * sizeof(char *));
        token[arr_len] = NULL;
        cpu->fetch_latch.instLen = arr_len;

        //-------------------------------------Dynamic Spliting------------------------------------------
        if(arr_len > 4 ){
            cpu->fetch_latch.instAddr = (atoi)(token[0]);
            strcpy(cpu->fetch_latch.opcode,token[1]);
            strcpy(cpu->fetch_latch.rg1,token[2]);
            strcpy(cpu->fetch_latch.or1,token[3]);
            strcpy(cpu->fetch_latch.or2,token[4]);
        }
        else if(arr_len == 4){
            cpu->fetch_latch.instAddr = (atoi)(token[0]);
            strcpy(cpu->fetch_latch.opcode,token[1]);
            strcpy(cpu->fetch_latch.rg1,token[2]);
            strcpy(cpu->fetch_latch.or1,token[3]); 
        }
        else if(arr_len == 3){
            cpu->fetch_latch.instAddr = (atoi)(token[0]);
            strcpy(cpu->fetch_latch.opcode,token[1]);
            strcpy(cpu->fetch_latch.rg1,token[2]); 
        }
        else if(arr_len == 2){
            cpu->fetch_latch.instAddr = (atoi)(token[0]);
            strcpy(cpu->fetch_latch.opcode,token[1]);
        }
        cpu->fetch_latch.opcode[strcspn(cpu->fetch_latch.opcode, "\r\t\n")] = 0;

        // implement BTB and prediction table
        if(strcmp(cpu->fetch_latch.opcode,"bez") == 0){
            if(cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern > 3){
                // cpu->fetch_latch.pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                cpu->pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                printf("bez branch Address %d & %d\n",cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern,cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target);
                cpu->fetch_latch.branch_taken = 1;
            }
            else{
                cpu->fetch_latch.branch_taken = 0;
            }
        }
        if(strcmp(cpu->fetch_latch.opcode,"bgez") == 0){
            if(cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern > 3){
                // cpu->fetch_latch.pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                cpu->pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                printf("bgez branch Address %d & %d\n",cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern,cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target);
                cpu->fetch_latch.branch_taken = 1;
            }
            else{
                cpu->fetch_latch.branch_taken = 0;
            }
        }
        else if(strcmp(cpu->fetch_latch.opcode,"blez") == 0){
            if(cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern > 3){
                // cpu->fetch_latch.pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                cpu->pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                printf("blez branch Address %d & %d\n",cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern,cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target);
                cpu->fetch_latch.branch_taken = 1;
            }
            else{
                cpu->fetch_latch.branch_taken = 0;
            }
        }
        else if(strcmp(cpu->fetch_latch.opcode,"bgtz") == 0){
            if(cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern > 3){
                // cpu->fetch_latch.pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                cpu->pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                printf("bgtz branch Address %d & %d\n",cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern,cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target);
                cpu->fetch_latch.branch_taken = 1;
            }
            else{
                cpu->fetch_latch.branch_taken = 0;
            }
        }
        else if(strcmp(cpu->fetch_latch.opcode,"bltz") == 0){
            if(cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern > 3){
                // cpu->fetch_latch.pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                cpu->pc = (cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target) / 4;
                printf("bltz branch Address %d & %d\n",cpu->pt[(cpu->fetch_latch.instAddr/4)%16].pattern,cpu->btb[(cpu->fetch_latch.instAddr/4)%16].target);
                cpu->fetch_latch.branch_taken = 1;
            }
            else{
                cpu->fetch_latch.branch_taken = 0;
            }
        }

        // BTB implementation ended
        
        cpu->decode_latch = cpu->fetch_latch;
    }
    else if (cpu->fetch_latch.halt_triggered == 1){
        if(strcmp(options,"pipeline") == 0){
            printf("IF             : %s",cpu->instructions[cpu->fetch_latch.pc+1]);
        }
    }
}

void clear_forwarding(CPU* cpu){
    strcpy(cpu->add_reg,"NULL");
    strcpy(cpu->mul_reg,"NULL");
    strcpy(cpu->div_reg,"NULL");
    strcpy(cpu->br_reg,"NULL");
    strcpy(cpu->mem1_reg,"NULL");
}