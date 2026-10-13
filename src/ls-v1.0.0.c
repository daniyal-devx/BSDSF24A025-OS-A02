/*
 * ls-v1.0.0.c
 *
 * A simple re-implementation of the Unix `ls` utility.
 * Version 1.0.0 - starter version.
 *
 * Behavior:
 *   - Lists the entries of each directory given on the command line.
 *   - With no arguments, lists the current directory (".").
 *   - Hidden entries (those whose names begin with '.') are skipped,
 *     just like the base ls behavior (the -a option is not implemented yet).
 *
 * This file is the base program that will be extended, feature by feature,
 * through versions 1.1.0 ... 1.6.0 of the assignment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

/* Print the (non-hidden) entries of the directory `dirname`, one per line. */
static void do_ls(const char *dirname)
{
    DIR *dir_ptr = opendir(dirname);

    if (dir_ptr == NULL) {
        fprintf(stderr, "ls: cannot access '%s': No such file or directory\n",
                dirname);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir_ptr)) != NULL) {
        /* Skip hidden files: names starting with a dot (., .., .hidden, ...) */
        if (entry->d_name[0] == '.')
            continue;

        printf("%s\n", entry->d_name);
    }

    closedir(dir_ptr);
}

int main(int argc, char *argv[])
{
    /* No arguments: list the current directory. */
    if (argc == 1) {
        do_ls(".");
        return 0;
    }

    /* Otherwise: list each directory argument in turn. */
    for (int i = 1; i < argc; i++)
        do_ls(argv[i]);

    return 0;
}
