#include <sys/resource.h>
#include <sys/select.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include "global.h"
#define b break

int x_pos;
int y_pos;

/***** MACRO BEGIN *****/
#define $GO_UP() {    \
   if (y_pos)         \
     --y_pos;         \
}b;

#define $GO_DOWN() {  \
   ++y_pos;           \
}b;

#define $GO_LEFT() {  \
   if (x_pos)         \
      --x_pos;        \
}b;

#define $GO_RIGHT() { \
   ++x_pos;           \
}b;

#define $KEY_BLOCK "\e[30;46m"
#define $RESET     "\e[0m"

/***** MACRO END *****/

/** Enable raw mode
 *
 * @param  none
 * @return void
 */
void
enable_raw(void)
{
	/* init-step */
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);

	/* disable canonic and echo mode */
	term.c_lflag &= ~(ICANON);
	term.c_lflag &= ~(ECHO);

	/* set attr */
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

/** Disable raw mode
 *
 * @param  none
 * @return void
 */
void
disable_raw(void)
{
	/* init-step */
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);

	/* re-enable defaults */
	term.c_lflag |= ICANON;
	term.c_lflag |= ECHO;

	/* set attr */
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

/** Checks if a key was hit
 *
 * @param none
 * @return {bool}
 */
bool
kbhit(void)
{
	struct timeval time = { 2L, 0L };
	fd_set val;

	FD_ZERO(&val);
	FD_SET(STDIN_FILENO, &val);

	return (select(1, &val, NULL, NULL, &time)>0);
}

/** Parses user input
 *
 * @param  none
 * @return void
 */
extern void
input(void)
{
	enable_raw();
	if (kbhit()) {
		char keypress = getchar();
#if defined(_CYGWIN)
		// format \224X
		if (keypress == 224) {
			switch(getchar()) {
			 case 'H': $GO_UP();
			 case 'P': $GO_DOWN();
			 case 'M': $GO_RIGHT();
			 case 'K': $GO_LEFT();
			}
		}
#else
		// format \033[X
		if (keypress == '\033') {
			if (getchar() == '[') {
				switch(getchar()) {
				 case 'A': $GO_UP();
				 case 'B': $GO_DOWN();
				 case 'C': $GO_RIGHT();
				 case 'D': $GO_LEFT();
				}
			}
		}
#endif //defined
		/** Not arrow keys **/
		else {
			int prio;
			switch(tolower(keypress)) {
			case 's':
				clrscr();
				printf("Which signal to send to process %u?\n\
====================================================================\n\
 1 - SIGHUP     2 - SIGINT   3 - SIGQUIT   4 - SIGILL    5 - SIGTRAP\n\
 6 - SIGABRT    7 - SIGBUS   8 - SIGFPE    9 - SIGKILL  10 - SIGUSR1\n\
11 - SIGSEGV   12 - SIGUSR2 13 - SIGPIPE  14 - SIGALRM  15 - SIGTERM\n\
16 - SIGSTKFLT 17 - SIGCHLD 18 - SIGCONT  19 - SIGSTOP  20 - SIGTSTP\n\
21 - SIGTTIN   22 - SIGTTOU 23 - SIGURG   24 - SIGXCPU  25 - SIGXFSZ\n\
26 - SIGVTALRM 27 - SIGPROF 28 - SIGWINCH 29 - SIGIO    30 - SIGPWR\n\
31 - SIGSYS    31 - SIGUNUSED(2)\n", c_task);
				ui sig;
				scanf("%u", &sig);
				kill(c_task, sig);
				break;
			case '-':
				prio = getpriority(PRIO_PROCESS, c_task)-1;
				setpriority(PRIO_PROCESS, c_task, prio);
				break;
			case '+':
				prio = getpriority(PRIO_PROCESS, c_task)+1;
				setpriority(PRIO_PROCESS, c_task, prio);
				break;
			}
			/* other input */
		}
	}
	/* reset stdin and flush */
	disable_raw();
	tcflush(STDIN_FILENO, TCIFLUSH);
}

/** List of keys at the bottom of the screen
 *
 * @param  none
 * @return void
 */
extern void
guide(void)
{
	//PROCESSES
	printf("Current proc: %u\n", c_task);

	//NAVIGATION
	printf(" ↑%sMove up%s",    $KEY_BLOCK, $RESET);
	printf(" ↓%sMove down%s",  $KEY_BLOCK, $RESET);
	printf(" →%sMove right%s", $KEY_BLOCK, $RESET);
	printf(" ←%sMove left%s",  $KEY_BLOCK, $RESET);

	//PROCESS EDIT
	printf(" S%sSend Signal%s",   $KEY_BLOCK, $RESET);
	printf(" +%sIncrease Nice%s", $KEY_BLOCK, $RESET);
	printf(" -%sDecrease Nice%s", $KEY_BLOCK, $RESET);

	//ETC
	printf(" Ctrl+C%sQuit%s", $KEY_BLOCK, $RESET);

	/* display the text from the stdout buffer */
	fflush(stdout);
}
