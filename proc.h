#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "global.h"

/***** MACRO BEGIN *****/
#define ever (;;)
#define ll long long
#define ull unsigned long long
#define MAGIC  256
#define MAGIC2 4096
#define uc unsigned char

#define $MAX_XPOS 11

#define $GREEN_FG  "\e[0;92m"
#define $BLUE_FG   "\e[0;94m"
#define $RED_FG    "\e[0;91m"
#define $GRAY_FG   "\e[0;97m"
#define $BOLD_FG   "\e[1;37m"
#define $YELLOW_FG "\e[1;33m"
#define $HEADER    "\e[0;30;42m"

#define $RESET          "\e[0m"

#define $SIGN(X) {               \
   if (elems[X][0] == '-')       \
      printf($RED_FG);           \
   else if (elems[X][0] == '0')  \
      printf($GRAY_FG);          \
   else                          \
      printf("%s", $GREEN_FG);   \
}
/***** MACRO END *****/

/** Checks if folder is numeric (PID) or not
 *
 * @param {uc} *str
 * @return void
 */
static bool
is_pid(uc *str)
{
	for (int i = 0; str[i]; ++i)
		if ((str[i] <= '0' || str[i] >= '9') && str[i] != '\0')
			return 0;
	return 1;
}

/** Shows system statistics, like htop
 *
 * @param {ll}rows,{ll}columns
 * @return void
 */
extern void
stats(ll rows, ll columns)
{
    FILE *fin;

    /***** UPTIME & LOADAVG BEGIN *****/
	/**************/
	/**  uptime  **/
	/**************/
    fin = fopen("/proc/uptime", "r");
    float up1, up2;
    ull up1_h, up1_m, up1_s,
        up2_h, up2_m, up2_s;

    fscanf(fin, "%f %f", &up1, &up2);
	fclose(fin);
	/* calculate H:m:s from seconds */
    up1_h = up1 / (60*60); up2_h = up2 / (60*60);
    up1  -= up1_h*(60*60); up2  -= up2_h*(60*60);

    up1_m = up1 / 60; up2_m = up2 / 60;
    up1  -= up1_m*60; up2  -= up2_m*60;

    up1_s = up1; up2_s = up2;

	/***************/
	/**  loadavg  **/
	/***************/
	fin = fopen("/proc/loadavg", "r");
	char *loadavg = NULL;
	size_t len;
	getline(&loadavg, &len, fin);
	fclose(fin);

    printf("cstop - UpTime: real %s%02d:%02d:%02d%s cpu %s%02d:%02d:%02d%s loadavg %s%s%s",
            $BOLD_FG, up1_h, up1_m, up1_s, $RESET,
            $BOLD_FG, up2_h, up2_m, up2_s, $RESET,
			$BOLD_FG, loadavg, $RESET);
    /***** UPTIME  & LOADAVG END *****/

    /***** TASKS BEGIN *****/

    printf("Tasks: %s%d%s total, %s%d%s running, %s%d%s sleeping, %s%d%s stopped, %s%d%s zombie\n",
           $BOLD_FG, tasks,    $RESET,
		   $BOLD_FG, tasks_rn, $RESET,
		   $BOLD_FG, tasks_sl, $RESET,
		   $BOLD_FG, tasks_st, $RESET,
		   $BOLD_FG, tasks_zb, $RESET);
    /***** TASKS END *****/

    /***** %CPU BEGIN *****/
	fin = fopen("/proc/stat", "r");

	char *names[10] = {"us", "sy", "ni", "id", "wa", "hi", "si", "st", "gu", "gn"};
	ui varbs[10];
	float sum = 0;
	
	printf("%cCpu(s): ", '%');

	fscanf(fin, "cpu: %u", &varbs[0]);
	for (int i = 1; i < 10; ++i) {
		fscanf(fin, "%u", &varbs[i]);
		sum += varbs[i];
	}
	/* restore the variable back in bounds */
	if (varbs[0] > 100)
		varbs[0] = 0;
	fclose(fin);
	for (int i = 0; i < 10; ++i)
		printf("%s%.2f%s %s  ", $BOLD_FG, (varbs[i]/sum)*100.0, $RESET, names[i]);

    /***** %CPU END *****/
    putchar('\n');
}

/** Display multiples of 1024 as KiB, MiB, GiB, ...
 *
 * @param {ull}num
 * @return void
 */
 void
 dp_units(ull num)
 {
		short d = -1; char ch;
		/* shorten as KiB, MiB, GiB, TiB, PiB, OiB, ZiB & YiB. */
		if (num < 1024) {
			ch = 'B';
		} else {
			while (num > 1024) {
				num >>= 10;
				++d;
			}
			switch(d) {
			 case 0: ch = 'K'; break;
			 case 1: ch = 'M'; break;
			 case 2: ch = 'G'; break;
			 case 3: ch = 'T'; break;
			 case 4: ch = 'P'; break;
			 case 5: ch = 'O'; break;
			 case 6: ch = 'Z'; break;
			 case 7: ch = 'Y'; break;
			}
		}
		printf("%s%4llu%c%s ", $BLUE_FG, num, ch, $RESET);
 }

ui c_task;

/** Shows all running processes
 *
 * @param {ll}rows,{ll}columns
 * @return void
 */
extern void
table(ll rows, ll columns)
{
	DIR *d = opendir("/proc");
	struct dirent *f;
	int k = 0;

	tasks = tasks_rn = tasks_sl = tasks_st = tasks_zb = 0;
	/* subtract this from the column count to get the final headerbar width */
	short space_left = 2;

	if (d) {
		/** We display the table header here **/
		printf("%s  ", $HEADER);
		if (x_pos == 0) { printf(    "   PID"); space_left += 6; }
		if (x_pos <= 1) { printf( "     USER"); space_left += 9; }
		if (x_pos <= 2) { printf(     "  PRI"); space_left += 5; }
		if (x_pos <= 3) { printf(     "   NI"); space_left += 5; }
		if (x_pos <= 4) { printf(   "   VIRT"); space_left += 7; }
		if (x_pos <= 5) { printf(    "   RES"); space_left += 6; }
		if (x_pos <= 6) { printf(    "   SHR"); space_left += 6; }
		if (x_pos <= 7) { printf(       "  S"); space_left += 3; }
		if (x_pos <= 8) { printf("%6cCPU",'%'); space_left += 5; }
		if (x_pos <= 9) { printf("%4cMEM",'%'); space_left += 4; }
		if (x_pos <=10) { printf( "    +TIME"); space_left += 9; }
          /* nothing */ { printf(  " Command"); space_left +=15; }

		for (int i = 0; i < columns - space_left; ++i)
			putchar(' ');
		printf("%s\n", $RESET);

		ui row_check = 0;
		while ((f = readdir(d)) != NULL) {
			if (is_pid(f->d_name)) {
                /***************************************************************************/
                /** INITIALIZE elems[25] TO BE USED IN PRINTING/CALCULATING PROCESS COUNT **/
                /***************************************************************************/
                uc pps[MAGIC];
                sprintf(pps, "/proc/%s/stat", f->d_name);
                FILE *fp = fopen(pps, "r");

                /* init for getdelim() with space separators */
                ssize_t len = 0;
                int j = 0;
                char *elems[52] = {NULL};

                /* don't tokenize paren contents */
                elems[0] = malloc(9);
                fscanf(fp, "%s (", elems[0]);
                /* begin reading to pointer */
                elems[1] = malloc(4097);
                uc tmp = '\0';
                for ever {
                    tmp = fgetc(fp);
                    if (tmp == ')')
                        break;
                    else
                        elems[1][j++] = tmp;
                }
                /* read space */
                fgetc(fp);

                for (j = 2; j < 52; ++j)
                    getdelim(&elems[j], &len, ' ', fp);
                fclose(fp);

                /* found a proc! */
                ++tasks;
                switch(elems[2][0]) {
                case 'R':
                    ++tasks_rn;
                    break;
                case 'S': //FALLTHROUGH
                case 'I':
                    ++tasks_sl;
                    break;
                case 'T':
                    ++tasks_st;
                    break;
                case 'Z':
                    ++tasks_zb;
                    break;
                }

				if (k >= y_pos && k < y_pos + rows) {
					/* check how many of the rows contain processes */
					++row_check;
					if (k == y_pos) {
						c_task = atoll(f->d_name);
						printf("%s>>%s", $YELLOW_FG, $RESET);
					} else {
                        printf("  ");
					}
					/*******************************************************/
					/** READ /proc/<pid>/stat CONTENTS AND PRINT THEM OUT **/
					/*******************************************************/
					if (x_pos == 0) {
						/*****************************/
						/**        PRINT PID        **/
						/*****************************/
						printf("%s%6s%s ", $BOLD_FG, f->d_name, $RESET);
					}
					if (x_pos <= 1) {
						/*****************************/
						/**       PRINT USER        **/
						/*****************************/
						struct stat strat;
						stat(pps, &strat);
						struct passwd *pwd = getpwuid(strat.st_uid);

						printf("%s%8s%s ", $GRAY_FG, pwd->pw_name, $RESET);
					}
					if (x_pos <= 2) {
						/*****************************/
						/**      PRINT PRIORITY     **/
						/*****************************/
						$SIGN(17);
						printf("%5s%s ", elems[17], $RESET);
					}
					if (x_pos <= 3) {
						/*****************************/
						/**       PRINT NICE        **/
						/*****************************/
						$SIGN(18);
						printf("%4s%s ", elems[18], $RESET);
					}
					if (x_pos <= 4) {
						/*****************************/
						/**PRINT VIRTUAL MEMORY SIZE**/
						/*****************************/
						ull num = atoll(elems[22]);
						dp_units(num);
					}
					if (x_pos <= 5) {
						/*****************************/
						/** PRINT RESIDENT SET SIZE **/
						/*****************************/
						ull num = atoll(elems[23]);
						dp_units(num);
					}
					if (x_pos <= 6) {
						/*****************************/
						/**PRINT SHR MEM CONSUMPTION**/
						/*****************************/
						/* get full path */
						uc fn[MAGIC];
						sprintf(fn, "/proc/%s/statm", f->d_name);

						FILE *fin = fopen(fn, "r");
						ull tmp;    /* dummy variable */
						ull shared; /* what we're actually gonna use */

						/* get 3rd content of /proc/<pid>/statm */
						fscanf(fin, "%llu %llu %llu", &tmp, &tmp, &shared);
						fclose(fin);

						/* print */
						dp_units(shared);
					}
					if (x_pos <= 7) {
						/*****************************/
						/**       PRINT STATUS      **/
						/*****************************/
						if (elems[2][0] <= 'A' || elems[2][0] >= 'Z')
							printf(" ?  ");
						else
							printf(" %s ", elems[2]);
					}
					if (x_pos <= 8) {
						/*****************************/
						/**        PRINT %CPU       **/
						/*****************************/
						/* sys uptime (in seconds) */
						float uptime;

						FILE *fin = fopen("/proc/uptime", "r");
						fscanf(fin, "%f", &uptime); fclose(fin);

						/* formula elements */
						ull utime = atoll(elems[13]),
						    stime = atoll(elems[14]),
					       cutime = atoll(elems[15]),
						   cstime = atoll(elems[16]),
						starttime = atoll(elems[21]);

						float sum = 0, telapsed = 0;

						/* clock ticks per second (hertz) */
						ull Hz = sysconf(_SC_CLK_TCK);

						/* now, we calculate the CPU usage */
						sum = utime+stime+cutime+cstime;
						telapsed = uptime-(starttime/Hz);
						printf("%7.2f", 100*(sum/Hz)/telapsed);
					}
					if (x_pos <= 9) {
						/*****************************/
						/**        PRINT %MEM       **/
						/*****************************/
						/* get the total memory so we can calculate the percentage */
                        FILE *fin;

						ull memtotal = 0L;
						ull size     = 0L;

						fin = fopen("/proc/meminfo", "r");
						/* read the first value into a buffer */
						fscanf(fin, "MemTotal: %ul kB\n", &memtotal);
						fclose(fin);

                        /* vsize is stored in elems[23] */
                        size = atoll(elems[23]);
						/* convert KiB to B */
						memtotal <<= 1;

                        printf("%7.2f", (float)size/(float)memtotal*100);
					}
					if (x_pos <= 10) {
						/*****************************/
						/**       PRINT TIME+       **/
						/*****************************/
						/* get full path*/
						uc          fn[MAGIC];
						struct stat strat;
						time_t      ptime;
						struct tm   ftime;

						sprintf(fn, "/proc/%s", f->d_name);
						stat(fn, &strat);

						time(&ptime);
						ptime -= strat.st_mtime;
						ftime = *localtime(&ptime);

						printf(" %02d:%02d:%02d ", ftime.tm_hour, ftime.tm_min, ftime.tm_sec);
					}
                    /**(nothing)**/ {
						/*****************************/
						/**       PRINT COMMS       **/
						/*****************************/
						/* init vars */
						uc fn[MAGIC], buf[MAGIC2]={""};

						int max_llength, modifier;
						if (x_pos < $MAX_XPOS) {
                            max_llength = columns-space_left+6;
                            modifier = 0;
                        } else {
                            max_llength = columns-2;
                            modifier = x_pos;
                        }

						sprintf(fn, "/proc/%s/cmdline", f->d_name);
						/* check if cmdline exists. If not, print comm.*/
						FILE *fin = fopen(fn, "r");
						int ch = fgetc(fin);
						if (ch == EOF)
                            sprintf(fn, "/proc/%s/comm\0", f->d_name);
                        else
                            ungetc(ch, fin);
                        fclose(fin);

						int fd   = open(fn, O_RDONLY);
						int size = read(fd, buf, MAGIC2);
                        /* remove trailing newline from the buffer, if it exists */
                        int lc = strlen(buf)-1;
                        if (buf[lc] == '\n') {
                            buf[lc] = '\0';
                            /* as /proc/<pid>/cmdline doesn't have trailing newlines at all, it
                               is safe to assume that the file in question is /proc/<pid>/comm.

                               As cstop marks these two files with different colors, this is what
                               we're going to do here. */
                            printf($GREEN_FG);
                        }

						uc *ptr = buf;
						uc *end = buf+size;

						for ( ;ptr < end; ) {
                            for (int i = 0;;++i) {
                                /* only print enough to fit to the screen */
                                if (i < max_llength) {
                                    /* if we reached out of bounds, print an empty space */
                                    if (ptr[i+modifier])
                                        putchar(ptr[i+modifier]);
                                    else
                                        putchar(' ');
                                } else {
                                    /* don't let the line cut off the screen */
                                    goto __table_func_next;
                                }
                            }
                            /* next command arg */
                            while (*ptr++);
						}
__table_func_next:
                        /* in case the command was marked as /proc/<pid>/comm */
                        printf($RESET);
                        close(fd);
                    }
					putchar('\n');
					/*** FREE POINTERS ***/
					free(elems[0]);
					free(elems[1]);
				}
				++k;
			}
		}
		closedir(d);
		/* fill up remaining console space */
		for (int i = row_check; i < rows; ++i)
			putchar('\n');
	}
}
