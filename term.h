#include <termios.h> /* struct termios */

void refresh_screen(); /* clear screen and move cursor to 1;1 */

int get_window_size(int *, int *);

int get_cursor_position(int, int *, int *);

int disable_raw_mode();

int enable_raw_mode();
