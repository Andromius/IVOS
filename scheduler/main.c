// Based on https://c9x.me/articles/gthreads/code0.html
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "gthr.h"

// Dummy function to simulate some thread work
void f(void)
{
	static int x;
	int i = 0, id;

	id = ++x;
	while (true)
	{
		printf("F Thread id = %d, val = %d BEGINNING\n", id, ++i);
		gt_uninterruptible_nanosleep(0, 50000000);
		printf("F Thread id = %d, val = %d END\n", id, ++i);
		gt_uninterruptible_nanosleep(0, 50000000);
	}
}

// Dummy function to simulate some thread work
void g(void)
{
	static int x;
	int i = 0, id;

	id = ++x;
	while (true)
	{
		printf("G Thread id = %d, val = %d BEGINNING\n", id, ++i);
		gt_uninterruptible_nanosleep(0, 50000000);
		printf("G Thread id = %d, val = %d END\n", id, ++i);
		gt_uninterruptible_nanosleep(0, 50000000);
	}
}

int main(int argc, char **argv)
{
	if (argc == 1)
	{
		printf("Usage: %s [-rr|--round-robin] [-rrp|--round-robin-priority] [-lot|--lottery] [-h|--help]\n", argv[0]);
		printf("Press CTRL+C to display thread statistics\n");
		exit(0);
	}

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-rr") == 0 || strcmp(argv[i], "--round-robin") == 0)
		{
			set_scheduling_algorithm(round_robin);
		}
		else if (strcmp(argv[i], "-rrp") == 0 || strcmp(argv[i], "--round-robin-priority") == 0)
		{
			set_scheduling_algorithm(round_robin_prio);
		}
		else if (strcmp(argv[i], "-lot") == 0 || strcmp(argv[i], "--lottery") == 0)
		{
			printf("Lottery scheduling not implemented yet! [%s]\n", argv[i]);
			exit(1);
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			printf("Usage: %s [-rr|--round-robin] [-rrp|--round-robin-priority] [-lot|--lottery] [-h|--help]\n", argv[0]);
			printf("Press CTRL+C to display thread statistics\n");
			exit(0);
		}
		else
		{
			printf("Unknown argument: %s\nUse -h or --help for usage information\n", argv[i]);
			exit(1);
		}
	}

	gt_init();		  // initialize threads, see gthr.c
	gt_create(f, 3);  // set f() as first thread
	gt_create(f, 0);  // set f() as second thread
	gt_create(g, 10); // set g() as third thread
	gt_create(g, 6);  // set g() as fourth thread
	gt_return(1);	  // wait until all threads terminate
}
