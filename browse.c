#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
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
int first_row;
int cursor_dy = 0; /* delta y (how many rows cursor has moved down) */

/** directory interacting */
size_t highlighted_line = 0;

/*********************************************************************/

/** terminal **/

/* calculate number of rows from cursor to bottom of screen */
int screen_rows_left() {
	int cursor_rows, cursor_cols;
	if (get_cursor_position(ttyfd, &cursor_rows, &cursor_cols) == -1)
		err(1, "get_cursor_position");
	return screen_rows - cursor_rows;
}

void set_window_info() {
	if (get_window_size(&screen_rows, &screen_cols) == -1)
		err(1, "get_window_size");

	int row, col;
	if (get_cursor_position(ttyfd, &row, &col) == -1)
		err(1, "get_cursor_position");
	first_row = row - cursor_dy;
}

/* clear lines under first row */
void clear_lines() {
	move_cursor_to(ttyfd, first_row, 1);
	write(ttyfd, "\x1b[J", 3);
}

/* move cursor to highlighted entry */
void reset_cursor() {
	int row = first_row + (int)highlighted_line;
	move_cursor_to(ttyfd, row, 1);
}

/*** directory displaying and interacting ***/

void process_key_press() {
	char c = read_key();

	switch (c) {
	case CTRL('c'):
		exit(0);
		break;
	}
}

void interact() {
	for (;;) {
		process_key_press();
	}
}

void display(struct dirent *entries[], size_t from, size_t to) {
	if (from > to) {
		fprintf(stderr, "invalid range %zu-%zu", from, to);
		exit(1);
	}

	struct dirent **subset = &entries[from];
	size_t count = to - from;

	for (size_t i = 0; i < count; i++) {
		write(ttyfd, subset[i]->d_name, strlen(subset[i]->d_name));
		if (i + 1 < count) {
			write(ttyfd, "\r\n", 2);
			cursor_dy++;
		}
	}
	reset_cursor();

	interact();
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
	/* collect and sort entries */
	int count;
	struct dirent **entries;
	if ((count = scandir(dirname, &entries, show_entry_p, compare_entries)) == -1)
		err(1, "scandir");

	/* display and interact with entries */
	display(entries, 0, count - 1);
}

void leave(size_t count, struct dirent *entries[]) {
	/* free entries */
	for (size_t i = 0; i < count; i++) {
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
	enable_raw_mode();
	set_window_info();

	/* detect window size change */
	struct sigaction act = {0};
	act.sa_handler = set_window_info;
	sigaction(SIGWINCH, &act, NULL);

	atexit(clear_lines);

	/* start browsing */
	browse(".");

	return 0; /* assume that the kernel frees ttyfd */

usage:
	fprintf(stderr, "usage: %s [-a]\n", argv[0]);
	return 1;
}
