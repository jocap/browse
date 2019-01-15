#include <termios.h> /* struct termios */

void refresh_screen(); /* clear screen and move cursor to 1;1 */

void move_cursor_to(int, int, int); /* fd, row, col */

int get_window_size(int *, int *); /* rows, cols */

int get_cursor_position(int, int *, int *); /* fd, row, col */

int disable_raw_mode();

int enable_raw_mode();

char read_key();
