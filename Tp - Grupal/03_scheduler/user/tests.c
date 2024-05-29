// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{    
    int old_prio = sys_get_tkts(sys_getenvid());
    int new_prio = old_prio - 1;
    sys_set_tkts(sys_getenvid(), new_prio);
    cprintf("My current prio is %d, my new prio should be %d\n", old_prio, new_prio);

	if (fork() == 0) {
		cprintf("i am child %08x, my priority is %d\n", sys_getenvid(), sys_get_tkts(sys_getenvid()));
	} else {
        cprintf("i am parent %08x, my priority is %d\n", sys_getenvid(), sys_get_tkts(sys_getenvid()));
    }
}
