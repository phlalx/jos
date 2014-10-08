// Fork a binary tree of processes and display their structure.

#include <inc/lib.h>


void
umain(int argc, char **argv)
{
	if (fork() == 0) {
        cprintf("I'm the son\n");
		exit();
    } else {
        cprintf("I'm the father\n");
        exit();
    }

}

