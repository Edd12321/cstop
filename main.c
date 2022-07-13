/** main.c
 *
 * @version v1.0
 * @author Edd12321 <sanduedi309@gmail.com>
 */

#pragma GCC optimize("Ofast")
#pragma GCC target("avx,avx2,fma")

#include <sys/ioctl.h>
#include <signal.h>
#include "proc.h"
#include "keys.h"

/***** MACRO BEGIN *****/
#define ever (;;)
#define ll long long
#define ui unsigned int
#ifndef STDOUT_FILENO
  #define STDOUT_FILENO 1
#endif

#define $CURSOR_HIDE() {     \
   fputs("\e[?25l", stdout); \
}

#define $CURSOR_SHOW() {     \
   fputs("\e[?25h", stdout); \
}
/***** MACRO END *****/

/** Restores cursor on SIGINT
 *
 * @param  none
 * @return void
 */
void
handle_sig(int sig)
{
	$CURSOR_SHOW();
	exit(0);
}

/** Init screen-size variables
 *
 * @param  none
 * @return void
 */
ll rows,columns;
void
init_vars(void)
{
	struct winsize term;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &term);

	rows    = term.ws_row - 6;
	columns = term.ws_col;
}

/** Clears the screen
 *
 * @param  none
 * @return void
 */

ui tasks;
ui tasks_rn;
ui tasks_sl;
ui tasks_st;
ui tasks_zb;

int
main(void)
{
	/* everything happens in the files below */
	$CURSOR_HIDE();
	signal(SIGINT, handle_sig);

	for ever {
		clrscr();
		init_vars();

		stats(rows, columns); //proc.c
		table(rows, columns); //proc.c

		guide();              //keys.c
		input();              //keys.c
	}
	return 0;
}
