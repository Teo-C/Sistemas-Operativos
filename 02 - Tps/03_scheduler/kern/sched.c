#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

extern int total_tickets;

void sched_halt(void);


struct Env* next_runnable_env(int index) {
	struct Env* current; 

	// Desde curent_env hasta final
	for (int i = index+1; i < NENV + index; i++) {
		current = &envs[i];
		if (current && current->env_status == ENV_RUNNABLE) return (current);
	}

	// Desde 0 hasta current_env
	for (int j = 0; j < index + NENV; j++) {
		current = &envs[j];
		if (current && current->env_status == ENV_RUNNABLE) return (current);
	}

	return NULL;
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here
	// Wihtout scheduler, keep runing the last environment while it exists
	#ifdef RR_SCHDLG
	int env_index = 0;
	if (curenv) {
		env_index = ENVX(curenv->env_id);
	}

	idle = next_runnable_env(env_index);

	if (idle) {
		env_run(idle);
	}
	else if (curenv && curenv->env_status == ENV_RUNNING) {
		env_run(curenv);
	} 
	#endif 

	#ifdef LT_SCHDLG
	int winner_ticket = random(total_tickets); // random entre [0, total_tickets]
    int counter = 0;
    int runnable_envs = 0;
    int rounds = 0;
    while(1) {
        rounds++;
        for(int i = 0; i < NENV; i++) {
            if (envs[i].env_status == ENV_RUNNABLE) runnable_envs++;
            counter += envs[i].priority;
            if(counter >= winner_ticket) {
                if (envs[i].env_status == ENV_RUNNABLE) env_run(&envs[i]); 
            }
        }
        if (rounds && !runnable_envs) {
            if (curenv && curenv->env_status == ENV_RUNNING) env_run(curenv);
            else break;
        }
    }

	#endif
	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on performance.
	// Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
