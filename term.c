#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sys/ttydefaults.h> /* CTRL */
#include <termios.h> /* line discipline (raw mode) */
#include <unistd.h> /* read(2), write(2) */
#include <ctype.h> /* iscntrl(3) */
#include <sys/ioctl.h> /* ioctl, winsize */

/**********************************************************************\
|                                                                      |
|  term.c is based on code from kilo by antirez:                       |
|      github.com/antirez/kilo/                                        |
|                                                                      |
\**********************************************************************/

struct termios original_termios;

void refresh_screen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); /* clear screen */
	write(STDOUT_FILENO, "\x1b[H", 3); /* move cursor to 1;1 */
}

int get_window_size(int *rows, int *cols) {
	struct winsize ws;

	/* Terminal I/O Control Get WINdow SiZe */
	int response = ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
	if (response == -1 || ws.ws_col == 0) {
		/* TODO: fallback if ioctl(2) fails */
		return -1;
	} else {
		*rows = ws.ws_row;
		*cols = ws.ws_col;
		return 1;
	}
}

int get_cursor_position(int ttyfd, int *rows, int *cols) {
	/* ask n command for cursor position (6) */
	if (write(ttyfd, "\x1b[6n", 4) != 4) return -1;

	/* the response is in the format <esc>[24;80R */

	/* read response */
	char buf[32];
	size_t i = 0;

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break; /* read until R */
		i++;
	}

	/* parse response */
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

int disable_raw_mode() {
	/* restore original line discipline */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1)
		return -1;
	return 0;
}

void try_disable_raw_mode() {
	if (disable_raw_mode() == -1) err(1, "disable_raw_mode");
}

int enable_raw_mode() {
	/* store original line discipline */
	if (tcgetattr(STDIN_FILENO, &original_termios) == -1)
		return -1;

	atexit(try_disable_raw_mode);

	struct termios raw = original_termios;

	/* disable various features */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	/* disable blocking for read(2) */
	raw.c_cc[VMIN] = 0; /* min number of bytes for it to return */
	raw.c_cc[VTIME] = 1; /* max tenths of seconds to wait before that */

	/* apply new line discipline */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		return -1;

	return 0;
}

