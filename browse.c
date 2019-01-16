#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
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

/** ui **/
struct screen {
	int rows;
	int cols;
} Screen;

struct ui {
	size_t total_rows;
	size_t starting_row;
	size_t cursor_offset; /* how many rows is cursor from top row? */

	char *top_text;
	char *bottom_text;

	struct dirent **entries;
	size_t entries_count;
	size_t entries_from;
};
const struct ui default_ui = { 20, 0, 0, "", "", NULL, 0, 0 };
struct ui UI;

/*********************************************************************/

/*** terminal ***/

/* calculate number of rows from cursor to bottom of screen */
void set_window_info() {
	if (get_window_size(&Screen.rows, &Screen.cols) == -1)
		err(1, "get_window_size");

	int row, col;
	if (get_cursor_position(ttyfd, &row, &col) == -1)
		err(1, "get_cursor_position");
	UI.starting_row = row - UI.cursor_offset;
}

/* clear rows under first row */
void clear_rows() {
	move_cursor_to(ttyfd, UI.starting_row, 1);
	write(ttyfd, "\x1b[J", 3);
}

/* make space to fit ui */
void make_space() {
	int row, col;
	if (get_cursor_position(ttyfd, &row, &col) == -1)
		err(1, "get_cursor_position");

	for (size_t i = 0; i < UI.total_rows; i++) {
		write(ttyfd, "\r\n", 2);
	}

	move_cursor_to(ttyfd, row + 2, 1);
}

/*** interaction ***/

int highlight_next() {
	return 0;
}
int highlight_previous() {
	return 0;
}

void process_key_press() {
	char c = read_key();

	switch (c) {
	case 'q':
	case CTRL('c'):
	case CTRL('d'):
		exit(0);
		break;
	case 'j':
		highlight_next();
		break;
	case 'k':
		highlight_previous();
		break;
	}
}

/*** ui ***/

/* move cursor to highlighted entry */
void reset_cursor() {
	int row = UI.starting_row + UI.cursor_offset;
	move_cursor_to(ttyfd, row, 1);
}

void interact() {
	for (;;) {
		process_key_press();
	}
}

void redraw() {
	move_cursor_to(ttyfd, UI.starting_row, 1);

	/* draw top row */
	write(ttyfd, UI.top_text, strlen(UI.top_text));
	write(ttyfd, "\r\n", 2);

	/* draw entries */
	size_t max_entries = UI.total_rows - 2;
	if (max_entries > UI.entries_count) max_entries = UI.entries_count;
	for (size_t i = UI.entries_from; i < max_entries; i++) {
		write(ttyfd, UI.entries[i]->d_name, strlen(UI.entries[i]->d_name));
		if (i + 1 < max_entries) {
			write(ttyfd, "\r\n", 2);
		}
	}

	move_cursor_to(ttyfd, UI.starting_row, 1);
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

	UI.top_text = (char *)dirname;
	UI.entries = entries;
	UI.entries_count = count;
	UI.entries_from = 0;
	redraw();
	interact();
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

	UI = default_ui;

	if ((ttyfd = open("/dev/tty", O_RDWR)) == -1) err(1, "open");

	/* terminal handling */
	enable_raw_mode();
	set_window_info();
	make_space();

	atexit(clear_rows);

	/* detect window size change */
	struct sigaction act = {0};
	act.sa_handler = set_window_info;
	sigaction(SIGWINCH, &act, NULL);

	/* start browsing */
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) err(1, "getcwd");
	browse(cwd);

	return 0; /* assume that the kernel frees ttyfd */

usage:
	fprintf(stderr, "usage: %s [-a]\n", argv[0]);
	return 1;
}
