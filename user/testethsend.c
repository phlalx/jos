#include <inc/lib.h>

#define N 50 
#define MAX_LEN 100 

char buffer[N][MAX_LEN];

void
umain(int argc, char **argv)
{
	int i = 0;

	for (i = 0; i < N; i++) {
		snprintf(buffer[i], MAX_LEN, "hello %d\n", i);
	}

	for (i = 0; i < N; i++) {
		sys_send_packet(buffer[i], strlen(buffer[i]));
	}
}