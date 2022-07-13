#include <stdbool.h>

#pragma once

extern void
clrscr(void)
{
	/* evil ansi escape magic */
	printf("\x1b[2J\x1b[H");
}

#define ll long long
#define ui unsigned int

extern ui c_task;

extern int x_pos;
extern int y_pos;
extern ui tasks;
extern ui tasks_rn;
extern ui tasks_sl;
extern ui tasks_st;
extern ui tasks_zb;

extern ll rows;
extern ll columns;
