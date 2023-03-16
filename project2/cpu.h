#ifndef _CPU_H_
#define _CPU_H_
#include <stdbool.h>
#include <assert.h>


typedef struct Register
{
    int value;          // contains register value

} Register;

/* Model of CPU */
typedef struct CPU
{
	/* Integer register file */
	Register *regs;

	
} CPU;

CPU*
CPU_init();

Register*
create_registers(int size);

int
CPU_run(CPU* cpu);

void
CPU_stop(CPU* cpu);

#endif
