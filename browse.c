#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h> /* DIR */
#include <dirent.h> /* opendir */
#include <fcntl.h> /* open */
#include <unistd.h> /* write */

#include "term.h"

/**********************************************************************\
|                                                                      |
|  browse(1) is a terminal-based directory browser and selector.       |
|                                                                      |
|  The source code is divided into two separate parts:                 |
|                                                                      |
|  1. directory reading                                                |
|  2. directory displaying                                             |
|                                                                      |
|  The directory display is written *not* to the standard output,      |
|  but to the tty's file descriptor.                                   |
|                                                                      |
|  Only the final selection is to be printed on the standard output.   |
|                                                                      |
\**********************************************************************/

/** command-line options **/
bool option_all = false;

/** terminal **/
int ttyfd;
int screen_rows;
int screen_cols;
size_t screen_rows_free;

/*** directory displaying ***/

void display(size_t count, struct dirent *entries[]) {
	for (size_t i = 0; i < count; i++) {
		write(ttyfd, entries[i]->d_name, strlen(entries[i]->d_name));
		write(ttyfd, "\r\n", 2);
	}
}

/*** directory reading ***/

int show_entry_p(const struct dirent *entry) {
	if (option_all) return true;
	else {
		if (strncmp(".", entry->d_name, 1) == 0) return false;
		else return true;
	}
}

int compare_entries(const struct dirent **d1, const struct dirent **d2) {
	/* sort directories first */
	if ((*d1)->d_type == DT_DIR && (*d2)->d_type != DT_DIR) return -1;
	if ((*d1)->d_type != DT_DIR && (*d2)->d_type == DT_DIR) return 1;

	/* sort alphabetically */
	return alphasort(d1, d2);
}

void browse(const char *dirname) {
	/* collect entries */
	int count;
	struct dirent **entries;
	if ((count = scandir(dirname, &entries, show_entry_p, compare_entries)) == -1)
		err(1, "scandir");

	/* display entries */
	display(count, entries);

	/* free entries */
	for (int i = 0; i < count; i++) {
		free(entries[i]);
	}
	free(entries);
}

/*** command-line interface ***/

int main(int argc, char *argv[]) {
	if (argc > 2) goto usage;
	if (argc == 2) {
		if (strcmp(argv[1], "-a") == 0) option_all = true;
		else goto usage;
	}

	if ((ttyfd = open("/dev/tty", O_RDWR)) == -1) err(1, "open");

	/* terminal handling */
	if (get_window_size(&screen_rows, &screen_cols) == -1) err(1, "get_window_size");
	enable_raw_mode();

	/* start browsing */
	browse(".");

	return 0; /* assume that the kernel frees ttyfd */

usage:
	fprintf(stderr, "usage: %s [-a]\n", argv[0]);
	return 1;
}
