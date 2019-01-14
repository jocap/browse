#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <sys/types.h> /* DIR */
#include <dirent.h> /* opendir */
#include <fcntl.h> /* open */

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
size_t screen_rows = 20;
size_t screen_rows_free = 4;

/** program **/
char *program_name;

/*** helper functions ***/

void die_usage() {
	fprintf(stderr, "usage: %s [-a]\n", program_name);
	exit(1);
}

/*** directory displaying ***/

void display(size_t count, struct dirent *entries[]) {
	for (size_t i = 0; i < count; i++) {
		printf("%s\n", entries[i]->d_name);
	}
}

/*** directory reading ***/

void count_dir(DIR *stream, size_t *all, size_t *visible) {
	*all = 0;
	*visible = 0;

	long location;
	if ((location = telldir(stream)) == -1) err(1, "telldir");
	rewinddir(stream);

	struct dirent *entry;
	while ((entry = readdir(stream)) != NULL) {
		if (strncmp(entry->d_name, ".", 1) == 0) {
			(*all)++;
		} else {
			(*all)++;
			(*visible)++;
		}
	}

	seekdir(stream, location); /* reset */
}

char *browse(char *dirname) {
	/* count entries */
	DIR *stream;
	size_t all;
	size_t visible;
	size_t count;

	if ((stream = opendir(dirname)) == NULL) err(1, "opendir");

	count_dir(stream, &all, &visible);

	if (option_all) count = all;
	else count = visible;

	/* collect entries */
	struct dirent *entries[count];
	struct dirent *entry;
	size_t i = 0;

	while ((entry = readdir(stream)) != NULL) {
		if (!option_all && strncmp(entry->d_name, ".", 1) == 0) continue;
		entries[i] = entry;
		i++;
	}

	/* display entries */
	display(count, entries);

	/* return selection */
	size_t size = strlen(entries[0]->d_name) + 1; /* placeholder */
	char *selection = malloc(size);
	strlcpy(selection, entries[0]->d_name, size);

	if (closedir(stream) == -1) err(1, "closedir");

	return selection;
}

/*** command-line interface ***/

int main(int argc, char *argv[]) {
	program_name = argv[0];
	if ((ttyfd = open("/dev/tty", O_RDWR)) == -1) err(1, "open");

	if (argc > 2) die_usage();
	if (argc == 2) {
		if (strcmp(argv[1], "-a") == 0) option_all = true;
		else die_usage();
	}

	char *selection = browse(".");
	printf("%s\n", selection);
	return 0;
	/* assume that the kernel frees ttyfd */
}
