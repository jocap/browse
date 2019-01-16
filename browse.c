#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

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

/*********************************************************************/

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

void leave(size_t count, struct dirent *entries[]) {
	/* free entries */
	for (size_t i = 0; i < count; i++) {
		free(entries[i]);
	}
	free(entries);
}

void browse(const char *dirname) {
	/* collect and sort entries */
	int count;
	struct dirent **entries;
	if ((count = scandir(dirname, &entries, show_entry_p, compare_entries)) == -1)
		err(1, "scandir");

	/* draw entries */

	/* eventually */
	leave(count, entries);
}

/*** command-line interface ***/

int main(int argc, char *argv[]) {
	if (argc > 2) goto usage;
	if (argc == 2) {
		if (strcmp(argv[1], "-a") == 0) option_all = true;
		else goto usage;
	}

	/* start browsing */
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) err(1, "getcwd");
	browse(cwd);

	return 0; /* assume that the kernel frees ttyfd */

usage:
	fprintf(stderr, "usage: %s [-a]\n", argv[0]);
	return 1;
}
