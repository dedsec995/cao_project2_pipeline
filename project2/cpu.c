#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cpu.h"

#define REG_COUNT 16
#define MAXY_LENGHT 512
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

int
load_the_memory(int num){
    num = num/4;
    char *filename = "memory_map.txt"; 
    FILE *filePointer = fopen(filename, "r");
    int county = 0;
    int n;
    if (filePointer == NULL)
    {
        printf("Error: could not open file %s", filename);
    }
    while (fscanf(filePointer, " %d", &n) == 1) {
        if((num) == county){
            fclose(filePointer);
            return n;
        }
        county++;
    }
    fclose(filePointer);
    return (-1);
}

void make_memory_map(){
    char c;
    FILE *fptr1, *fptr2;
    char read_file[] = "memory_map.txt";
    char write_file[] = "memory_map2.txt";
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

int
write_the_memory(int val,int num){
    num = num/4;
    char *filename = "memory_map2.txt"; 
    FILE *filePointer = fopen(filename, "r+");
    int county = 0;
    int n;
    if (filePointer == NULL)
    {
        printf("Error: could not open file %s", filename);
    }
    while (fscanf(filePointer, " %d", &n) == 1) {
        if((num) == county){
            fprintf(filePointer,"%d",val);
            fclose(filePointer);
            return 1;
        }
        county++;
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
    }
    return regs;
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
        freed_registers(cpu,REG_COUNT);
        printf("Hazard: %d",cpu->hazard);
        print_display(cpu);
        cpu->clock++;
        if(strcmp(options,"pipeline") == 0){
            printf("\n================================");
            printf("\nClock Cycle #: %d\n",cpu->clock);
            printf("--------------------------------\n");
        }
        if(cpu->clock > 1000){
            break;
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
        if(cpu->raw == 1){
            cpu->raw = 0;
            cpu->register_read_latch.halt_triggered = 0;
            cpu->analysis_latch.halt_triggered = 0;
            cpu->decode_latch.halt_triggered = 0;
            cpu->fetch_latch.halt_triggered = 0;
            cpu->register_read_latch.unfreeze = 0;
            cpu->analysis_latch.unfreeze = 0;
            cpu->decode_latch.unfreeze = 0;
            cpu->fetch_latch.unfreeze = 0;
        }
        if(strcmp(cpu->writeback_latch.opcode,"ret") == 0){
            cpu->writeback_latch.has_inst = 0;
            return(1);
        }
        else if(strcmp(cpu->writeback_latch.opcode,"st") == 0){
            return(0);
        }
        else if (strcmp(cpu->writeback_latch.opcode,"st") == 0){
            cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing = 0;
            cpu->regs[atoi(cpu->writeback_latch.rg1+1)].freed_this_cycle = 1;
            printf("Unblocking: %s\n", cpu->writeback_latch.rg1);
            return(0);
        }
        cpu->regs[atoi(cpu->writeback_latch.rg1+1)].value = cpu->writeback_latch.buffer;
        if (cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing == 1 && cpu->register_read_latch.halt_triggered == 1){
            if(strcmp(cpu->writeback_latch.rg1,cpu->register_read_latch.or1) == 0 || strcmp(cpu->writeback_latch.rg1,cpu->register_read_latch.or2) == 0){
                cpu->writeback_latch.halt_triggered = 0;
                cpu->register_read_latch.unfreeze = 1;
                cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing = 0;
                printf("Unblocking by freeing reg: %s\n", cpu->writeback_latch.rg1);
                cpu->hazard++;
                return(0);
            }
            else if (strcmp(cpu->register_read_latch.opcode,"st") == 0){
                if(strcmp(cpu->writeback_latch.rg1,cpu->register_read_latch.rg1) == 0){
                    cpu->writeback_latch.halt_triggered = 0;
                    cpu->register_read_latch.unfreeze = 1;
                    cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing = 0;
                    printf("Unblocking by freeing reg: %s\n", cpu->writeback_latch.rg1);
                    cpu->hazard++;
                    return(0);
                }
            }
            else{
                cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing = 0;
                cpu->regs[atoi(cpu->writeback_latch.rg1+1)].freed_this_cycle = 1;
                printf("Unblocking: %s\n", cpu->writeback_latch.rg1);
                return(0);    
            }
        }
        if (cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing == 1){
            cpu->regs[atoi(cpu->writeback_latch.rg1+1)].is_writing = 0;
            cpu->regs[atoi(cpu->writeback_latch.rg1+1)].freed_this_cycle = 1;
            printf("Unblocking: %s\n", cpu->writeback_latch.rg1);
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
        if(strcmp(options,"pipeline") == 0){
            printf("Mem2           : %s",cpu->instructions[cpu->memory2_latch.pc]);
        }
        if(strcmp(cpu->memory2_latch.opcode,"ret") == 0){
            cpu->writeback_latch = cpu->memory2_latch;
            cpu->memory2_latch.has_inst = 0;
            return;
        }
        else if (strcmp(cpu->memory2_latch.opcode,"ld")==0){
            // printf("Ld executed\n");
            if (cpu->memory2_latch.or1[0] == 82){
                cpu->memory2_latch.buffer = load_the_memory(cpu->memory2_latch.rg2_val);
                // cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value = load_the_memory(cpu->memory2_latch.rg2_val);
            }
            else{
                cpu->memory2_latch.buffer = load_the_memory(atoi(cpu->memory2_latch.or1+1));
                // cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value = load_the_memory(atoi(cpu->memory2_latch.or1+1));
            }
            // printf("ld buffer: %d\n",cpu->memory2_latch.buffer);
        }
        else if (strcmp(cpu->memory2_latch.opcode,"st")==0){
            // printf("Ld executed\n");
            if (cpu->memory2_latch.or1[0] == 82){
                cpu->memory2_latch.buffer = write_the_memory(cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value,cpu->memory2_latch.rg2_val);
            }
            else{
                cpu->memory2_latch.buffer = write_the_memory(cpu->regs[atoi(cpu->memory2_latch.rg1+1)].value,atoi(cpu->memory2_latch.or1+1));
            }
            // printf("ld buffer: %d\n",cpu->memory2_latch.buffer);
        }
        if(strcmp(cpu->memory2_latch.opcode,"st") != 0 ){
            if(cpu->regs[atoi(cpu->memory2_latch.rg1+1)].is_writing != 1){
                cpu->regs[atoi(cpu->memory2_latch.rg1+1)].is_writing = 1;
                printf("Blocking : %s\n",cpu->memory2_latch.rg1);
            }
        }
        cpu->writeback_latch = cpu->memory2_latch;
    }
    else if (cpu->memory2_latch.halt_triggered==1){
        cpu->writeback_latch = cpu->memory2_latch;
        // cpu->writeback_latch.halt_triggered = 0;
    }
}

void memory1_unit(CPU* cpu){
    if(cpu->memory1_latch.has_inst == 1 && cpu->memory1_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("Mem1           : %s",cpu->instructions[cpu->memory1_latch.pc]);
        }
        if(strcmp(cpu->memory1_latch.opcode,"ret") == 0){
            cpu->memory2_latch = cpu->memory1_latch;
            cpu->memory1_latch.has_inst = 0;
            return;
        }
        if(strcmp(cpu->memory1_latch.opcode,"st") != 0 ){
            if(cpu->regs[atoi(cpu->memory1_latch.rg1+1)].is_writing != 1){
                cpu->regs[atoi(cpu->memory1_latch.rg1+1)].is_writing = 1;
                printf("Blocking : %s\n",cpu->memory1_latch.rg1);
            }
        }
        cpu->memory2_latch = cpu->memory1_latch;
    }
    else if(cpu->memory1_latch.halt_triggered==1){
        cpu->memory2_latch = cpu->memory1_latch;
    }
}

void branch_unit(CPU* cpu){
    if(cpu->branch_latch.has_inst == 1 && cpu->branch_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("BR             : %s",cpu->instructions[cpu->branch_latch.pc]);
        }
        if(strcmp(cpu->branch_latch.opcode,"ret") == 0){
            cpu->memory1_latch = cpu->branch_latch;
            cpu->branch_latch.has_inst = 0;
            return;
        }
        if(strcmp(cpu->branch_latch.opcode,"st") != 0 ){
            if(cpu->regs[atoi(cpu->branch_latch.rg1+1)].is_writing != 1){
                cpu->regs[atoi(cpu->branch_latch.rg1+1)].is_writing = 1;
                printf("Blocking : %s\n",cpu->branch_latch.rg1);
            }
        }
        cpu->memory1_latch = cpu->branch_latch;
    }
    else if (cpu->branch_latch.halt_triggered==1){
        cpu->memory1_latch = cpu->branch_latch;
    }
}

void divider_unit(CPU* cpu){
    if(cpu->divider_latch.has_inst == 1 && cpu->divider_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("DIV            : %s",cpu->instructions[cpu->divider_latch.pc]);
        }
        if(strcmp(cpu->divider_latch.opcode,"div") == 0){
            if(cpu->regs[atoi(cpu->divider_latch.rg1+1)].is_writing ==1 && cpu->register_read_latch.halt_triggered == 1){
                cpu->regs[atoi(cpu->divider_latch.rg1+1)].is_writing = 0;
                cpu->register_read_latch.halt_triggered = 0;
                cpu->analysis_latch.halt_triggered = 0;
                cpu->decode_latch.halt_triggered = 0;
                cpu->fetch_latch.halt_triggered = 0;
                cpu->register_read_latch.unfreeze = 0;
                cpu->analysis_latch.unfreeze = 0;
                cpu->decode_latch.unfreeze = 0;
                cpu->fetch_latch.unfreeze = 0;
                cpu->hazard++;
            }
            else if (cpu->regs[atoi(cpu->divider_latch.rg1+1)].is_writing ==1){
                cpu->regs[atoi(cpu->divider_latch.rg1+1)].is_writing = 0;
            }
            else{}
        }
        else if(strcmp(cpu->divider_latch.opcode,"st") == 0 ){
        }
        else{
            if(cpu->regs[atoi(cpu->divider_latch.rg1+1)].is_writing !=1){
                cpu->regs[atoi(cpu->divider_latch.rg1+1)].is_writing = 1;
                printf("Blocking : %s\n",cpu->divider_latch.rg1);
            }
        }
        if(strcmp(cpu->divider_latch.opcode,"ret") == 0){
            cpu->branch_latch = cpu->divider_latch;
            cpu->divider_latch.has_inst = 0;
            return;
        }
        else if(strcmp(cpu->divider_latch.opcode,"div") == 0){
            //TODO Write Divide Logic
            if(cpu->divider_latch.or1[0] == 82 && cpu->divider_latch.or2[0] == 82){
                cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / cpu->divider_latch.rg3_val;
            }
            if (cpu->divider_latch.or1[0] == 82){
                cpu->divider_latch.buffer = cpu->divider_latch.rg2_val / atoi(cpu->divider_latch.or2+1);
            }
            else if (cpu->divider_latch.or2[0] == 82){
                cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / cpu->divider_latch.rg3_val;
            }
            else{
                cpu->divider_latch.buffer = atoi(cpu->divider_latch.or1+1) / atoi(cpu->divider_latch.or2+1);
            }
        }
        cpu->branch_latch = cpu->divider_latch;
    }
    else if(cpu->divider_latch.halt_triggered==1){
        cpu->branch_latch = cpu->divider_latch;
    }
}

void multiplier_unit(CPU* cpu){
    if(cpu->multiplier_latch.has_inst == 1 && cpu->multiplier_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("MUL            : %s",cpu->instructions[cpu->multiplier_latch.pc]);
        }
        if(strcmp(cpu->multiplier_latch.opcode,"mul") == 0 ){
            if(cpu->regs[atoi(cpu->multiplier_latch.rg1+1)].is_writing ==1 && cpu->register_read_latch.halt_triggered == 1){
                cpu->regs[atoi(cpu->multiplier_latch.rg1+1)].is_writing = 0;
                cpu->register_read_latch.halt_triggered = 0;
                cpu->analysis_latch.halt_triggered = 0;
                cpu->decode_latch.halt_triggered = 0;
                cpu->fetch_latch.halt_triggered = 0;
                cpu->register_read_latch.unfreeze = 0;
                cpu->analysis_latch.unfreeze = 0;
                cpu->decode_latch.unfreeze = 0;
                cpu->fetch_latch.unfreeze = 0;
                cpu->hazard++;
            }
            else if(cpu->regs[atoi(cpu->multiplier_latch.rg1+1)].is_writing ==1){
                cpu->regs[atoi(cpu->multiplier_latch.rg1+1)].is_writing = 0;
            }
            else{}
        }
        else if(strcmp(cpu->multiplier_latch.opcode,"st") == 0 ){
        }
        else if((cpu->multiplier_latch.buffer !=-1) && (strcmp(cpu->register_read_latch.opcode,"ld") != 0 && strcmp(cpu->register_read_latch.opcode,"div") != 0)){}
        else{
            if(cpu->regs[atoi(cpu->multiplier_latch.rg1+1)].is_writing !=1){
                cpu->regs[atoi(cpu->multiplier_latch.rg1+1)].is_writing = 1;
                printf("Blocking : %s\n",cpu->multiplier_latch.rg1);
            }
        }
        if(strcmp(cpu->multiplier_latch.opcode,"ret") == 0){
            cpu->divider_latch = cpu->multiplier_latch;
            cpu->multiplier_latch.has_inst = 0;
            return;
        }
        else if(strcmp(cpu->multiplier_latch.opcode,"mul") == 0){
            //TODO Write Multiplication Logic
            if(cpu->multiplier_latch.or1[0]){
                cpu->multiplier_latch.buffer = cpu->multiplier_latch.rg2_val * atoi(cpu->multiplier_latch.or2+1); 
            }
            else if(cpu->multiplier_latch.or2[0]){
                cpu->multiplier_latch.buffer = atoi(cpu->multiplier_latch.or1+1) * cpu->multiplier_latch.rg3_val;
            }
            else{
                cpu->multiplier_latch.buffer = atoi(cpu->multiplier_latch.or1+1) * atoi(cpu->multiplier_latch.or2+1);
            }
            strcpy(cpu->mul_reg,cpu->multiplier_latch.rg1);
            cpu->mul_val = cpu->multiplier_latch.buffer;
            cpu->divider_latch = cpu->multiplier_latch;
            return;
        }
        strcpy(cpu->mul_reg,"NULL");
        cpu->mul_val = -1;
        cpu->divider_latch = cpu->multiplier_latch;
    }
    else if (cpu->multiplier_latch.halt_triggered==1){
        cpu->divider_latch = cpu->multiplier_latch;
    }
}

void adder_unit(CPU* cpu){
    if(cpu->adder_latch.has_inst == 1 && cpu->adder_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("ADD            : %s",cpu->instructions[cpu->adder_latch.pc]);
        }
        if(strcmp(cpu->adder_latch.opcode,"set") == 0 || strcmp(cpu->adder_latch.opcode,"add") == 0 || strcmp(cpu->adder_latch.opcode,"sub") == 0 ){
        }
        else if(strcmp(cpu->adder_latch.opcode,"st") == 0 ){
        }
        else{
            cpu->regs[atoi(cpu->adder_latch.rg1+1)].is_writing = 1;
            printf("Blocking : %s\n",cpu->adder_latch.rg1);
        }
        if(strcmp(cpu->adder_latch.opcode,"ret") == 0){
            cpu->multiplier_latch = cpu->adder_latch;
            cpu->adder_latch.has_inst = 0;
            return;
        }
        else if(strcmp(cpu->adder_latch.opcode,"add") == 0){
            //TODO Write Subtraction Logic
            if (cpu->adder_latch.or1[0] == 82 && cpu->adder_latch.or2[0] == 82){
                cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + cpu->adder_latch.rg3_val;
            }
            else if (cpu->adder_latch.or1[0] == 82){
                    cpu->adder_latch.buffer = cpu->adder_latch.rg2_val + atoi(cpu->adder_latch.or2+1);
            }
            else if (cpu->adder_latch.or2[0] == 82){
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + cpu->adder_latch.rg3_val;
            }
            else{
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) + atoi(cpu->adder_latch.or2+1);
            }
            strcpy(cpu->add_reg,cpu->adder_latch.rg1);
            cpu->add_val = cpu->adder_latch.buffer;
            cpu->multiplier_latch = cpu->adder_latch;
            return;
        }
        else if(strcmp(cpu->adder_latch.opcode,"sub") == 0){
            //TODO Write Subtraction Logic
            if (cpu->adder_latch.or1[0] == 82 && cpu->adder_latch.or2[0] == 82){
                cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - cpu->adder_latch.rg3_val;
            }
            else if (cpu->adder_latch.or1[0] == 82){
                cpu->adder_latch.buffer = cpu->adder_latch.rg2_val - atoi(cpu->adder_latch.or2+1);
            }
            else if (cpu->adder_latch.or2[0] == 82){
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - cpu->adder_latch.rg3_val;
            }
            else{
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1) - atoi(cpu->adder_latch.or2+1);
            }
            strcpy(cpu->add_reg,cpu->adder_latch.rg1);
            cpu->add_val = cpu->adder_latch.buffer;
            cpu->multiplier_latch = cpu->adder_latch;
            return;
        }
        else if(strcmp(cpu->adder_latch.opcode,"set") == 0){
            //TODO Write Set Logic
            if (cpu->adder_latch.or1[0] == 82){
                cpu->adder_latch.buffer = cpu->adder_latch.rg2_val;  
            }
            else{
                cpu->adder_latch.buffer = atoi(cpu->adder_latch.or1+1);
            }  
            strcpy(cpu->add_reg,cpu->adder_latch.rg1);
            cpu->add_val = cpu->adder_latch.buffer;
            cpu->multiplier_latch = cpu->adder_latch;
            return;
        }
        else if(strcmp(cpu->adder_latch.opcode,"ld") == 0){
            // cpu->regs[atoi(cpu->adder_latch.rg1+1)].is_writing = 1;
            // printf("ABlocking : %s\n",cpu->adder_latch.rg1);
        }
        strcpy(cpu->add_reg,"NULL");
        cpu->add_val = -1;
        cpu->multiplier_latch = cpu->adder_latch;
    }
    else if (cpu->adder_latch.halt_triggered==1){
        cpu->multiplier_latch = cpu->adder_latch;
    }
}

void  register_read_unit(CPU* cpu){
    if(cpu->register_read_latch.has_inst == 1 && cpu->register_read_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("RR             : %s",cpu->instructions[cpu->register_read_latch.pc]);
        }
        cpu->register_read_latch.buffer = -1;
        if(strcmp(cpu->register_read_latch.opcode,"ret") == 0){
            cpu->adder_latch = cpu->register_read_latch;
            cpu->register_read_latch.has_inst = 0;
            return;
        }
        if (cpu->register_read_latch.or1[0] == 82){
            strcpy(cpu->register_read_latch.rg2,cpu->register_read_latch.or1);
            cpu->register_read_latch.rg2_val = cpu->regs[atoi(cpu->register_read_latch.or1+1)].value;
        }
        if (cpu->register_read_latch.or2[0] == 82){
            strcpy(cpu->register_read_latch.rg3,cpu->register_read_latch.or2);
            cpu->register_read_latch.rg3_val = cpu->regs[atoi(cpu->register_read_latch.or2+1)].value;
        }
        cpu->register_read_latch.rg1_val = cpu->regs[atoi(cpu->register_read_latch.rg1+1)].value;
        // if(strcmp(cpu->adder_latch.opcode,"add") == 0 || strcmp(cpu->adder_latch.opcode,"sub") == 0 || strcmp(cpu->adder_latch.opcode,"set") == 0){
        //     cpu->adder_latch = cpu->register_read_latch;
        //     return;
        // }

        // Checking if Registers are Been Written
        if (strcmp(cpu->register_read_latch.opcode,"st") == 0){
            if(cpu->regs[atoi(cpu->register_read_latch.rg1+1)].is_writing == 1){
                    printf("(st)Blocked because of: %s\n",cpu->register_read_latch.rg1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    return;
            }
        }

        if(cpu->register_read_latch.or1[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing == 1){
                    printf("OR1: %d\n", cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing);
                    printf("Blocked because of: %s\n",cpu->register_read_latch.or1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                return;
        }
        if (cpu->register_read_latch.or2[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or2+1)].is_writing == 1){
                    printf("OR2: %d\n", cpu->regs[atoi(cpu->register_read_latch.or2+1)].is_writing);
                    printf("Blocked because of writing: %s\n",cpu->register_read_latch.or2);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                return;
        }
        else if(strcmp(cpu->register_read_latch.opcode,"ld") == 0){
            if (cpu->register_read_latch.or1[0] == 82){
                cpu->register_read_latch.rg2_val = cpu->regs[atoi(cpu->register_read_latch.or1+1)].value;
            }
            else{
                cpu->register_read_latch.rg1_val = cpu->regs[atoi(cpu->register_read_latch.rg1+1)].value;
            }
            cpu->register_read_latch.rg1_val = cpu->regs[atoi(cpu->register_read_latch.rg1+1)].value;
            cpu->adder_latch = cpu->register_read_latch;
            //TODO Read memory map
            return;
        }
        else if (strcmp(cpu->register_read_latch.opcode,"set") == 0){
            cpu->adder_latch = cpu->register_read_latch;
            return;
        }

        //Checking if registers are freed this cycle
        if (strcmp(cpu->register_read_latch.opcode,"st") == 0){
            if(cpu->regs[atoi(cpu->register_read_latch.rg1+1)].freed_this_cycle == 1){
                    printf("(st)Blocked because of: %s\n",cpu->register_read_latch.rg1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    cpu->raw = 1;
                    return;
            }
        }
        if(cpu->register_read_latch.or1[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or1+1)].freed_this_cycle == 1){
                    printf("OR1: %d\n", cpu->regs[atoi(cpu->register_read_latch.or1+1)].freed_this_cycle);
                    printf("Blocked because of: %s\n",cpu->register_read_latch.or1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    cpu->raw = 1;
                return;
        }
        if (cpu->register_read_latch.or2[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.rg1+1)].freed_this_cycle == 1){
                    printf("OR2: %d\n", cpu->regs[atoi(cpu->register_read_latch.or2+1)].freed_this_cycle);
                    printf("Blocked because of freed from cycle: %s\n",cpu->register_read_latch.or2);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    cpu->raw = 1;
                return;
        }


        cpu->register_read_latch.rg1_val = cpu->regs[atoi(cpu->register_read_latch.rg1+1)].value;
        cpu->adder_latch = cpu->register_read_latch;
    }
    else if(cpu->register_read_latch.unfreeze == 1){
        cpu->hazard++;
        if (strcmp(cpu->register_read_latch.opcode,"st") == 0){
            if(cpu->regs[atoi(cpu->register_read_latch.rg1+1)].is_writing == 1){
                    printf("(st)Blocked because of: %s\n",cpu->register_read_latch.rg1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    cpu->register_read_latch.unfreeze =0;
                    return;
            }
        }
        if(cpu->register_read_latch.or1[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing == 1){
                    printf("OR1: %d\n", cpu->regs[atoi(cpu->register_read_latch.or1+1)].is_writing);
                    printf("Blocked because of: %s\n",cpu->register_read_latch.or1);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    cpu->register_read_latch.unfreeze =0;
                return;
        }
        if (cpu->register_read_latch.or2[0] == 82 && cpu->regs[atoi(cpu->register_read_latch.or2+1)].is_writing == 1){
                    printf("OR2: %d\n", cpu->regs[atoi(cpu->register_read_latch.or2+1)].is_writing);
                    printf("Blocked because of is writing in freeze: %s\n",cpu->register_read_latch.or2);
                    cpu->adder_latch.halt_triggered = 1;
                    cpu->register_read_latch.halt_triggered = 1;
                    cpu->analysis_latch.halt_triggered = 1;
                    cpu->decode_latch.halt_triggered = 1;
                    cpu->fetch_latch.halt_triggered = 1;
                    cpu->register_read_latch.unfreeze =0;
                return;
        }
        cpu->register_read_latch.unfreeze =0;
        cpu->register_read_latch.halt_triggered = 0;
        cpu->analysis_latch.unfreeze = 1;
        cpu->decode_latch.unfreeze = 1;
        cpu->fetch_latch.unfreeze = 1;
        if(strcmp(options,"pipeline") == 0){
            printf("RR             : %s",cpu->instructions[cpu->register_read_latch.pc]);
        }
    }
    else if (cpu->register_read_latch.halt_triggered==1){
        cpu->hazard++;
        if(strcmp(options,"pipeline") == 0){
            printf("RR             : %s",cpu->instructions[cpu->register_read_latch.pc]);
        }
    }
}

void analysis_unit(CPU* cpu){
    if(cpu->analysis_latch.has_inst == 1 && cpu->analysis_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("IA             : %s",cpu->instructions[cpu->analysis_latch.pc]);
        }
        if(strcmp(cpu->analysis_latch.opcode,"ret") == 0){
            cpu->register_read_latch=cpu->analysis_latch;
            cpu->analysis_latch.has_inst = 0;
            return;
        }
        cpu->register_read_latch=cpu->analysis_latch;
    }
    else if(cpu->analysis_latch.unfreeze == 1){
        cpu->analysis_latch.unfreeze =0;
        cpu->analysis_latch.halt_triggered = 0;
        if(strcmp(options,"pipeline") == 0){
            printf("IA             : %s",cpu->instructions[cpu->analysis_latch.pc]);
        }
    }
    else if(cpu->analysis_latch.halt_triggered==1){
        if(strcmp(options,"pipeline") == 0){
            printf("IA             : %s",cpu->instructions[cpu->analysis_latch.pc]);
        }
        // cpu->register_read_latch=cpu->analysis_latch;
    }
}

void decode_unit(CPU* cpu){
    if(cpu->decode_latch.has_inst == 1 && cpu->decode_latch.halt_triggered==0){
        if(strcmp(options,"pipeline") == 0){
            printf("ID             : %s",cpu->instructions[cpu->decode_latch.pc]);
        }
        if(strcmp(cpu->decode_latch.opcode,"ret") == 0){
            cpu->analysis_latch = cpu->decode_latch;
            cpu->decode_latch.has_inst = 0;
            return;
        }
        cpu->analysis_latch = cpu->decode_latch;
    }
    else if(cpu->decode_latch.unfreeze == 1){
        cpu->decode_latch.unfreeze =0;
        cpu->decode_latch.halt_triggered = 0;
        if(strcmp(options,"pipeline") == 0){
            printf("ID             : %s",cpu->instructions[cpu->decode_latch.pc]);
        }
    }
    else if (cpu->decode_latch.halt_triggered==1){
        if(strcmp(options,"pipeline") == 0){
            printf("ID             : %s",cpu->instructions[cpu->decode_latch.pc]);
        }
        // cpu->analysis_latch = cpu->decode_latch;
    }
}

void fetch_unit(CPU* cpu){
    if(cpu->fetch_latch.has_inst == 1 && cpu->fetch_latch.halt_triggered == 0){
        cpu->fetch_latch.pc = cpu->pc;
        cpu->pc++;
        char str1[128];
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
        if(strcmp(cpu->fetch_latch.opcode,"ret") == 0){
            cpu->decode_latch = cpu->fetch_latch;
            cpu->fetch_latch.has_inst = 0;
            return;
        }
        cpu->decode_latch = cpu->fetch_latch;
    }
    else if(cpu->fetch_latch.unfreeze == 1){
        cpu->fetch_latch.unfreeze =0;
        cpu->fetch_latch.halt_triggered = 0;
        if(strcmp(options,"pipeline") == 0){
            printf("IF             : %s",cpu->instructions[cpu->fetch_latch.pc]);
            // printf("IF             : Unfreeze");

        }
    }
    else if (cpu->fetch_latch.halt_triggered == 1){
        if(strcmp(options,"pipeline") == 0){
            printf("IF             : %s",cpu->instructions[cpu->fetch_latch.pc+1]);
            // printf("IF             : Stall");

        }
        // cpu->decode_latch = cpu->fetch_latch;
    }
}

