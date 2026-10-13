/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*
* Version 1.1.0: Complete Long Listing Format (-l)
*   -l  display a long listing of every entry (permissions, links,
*       owner, group, size, modification time), resembling "ls -l".
*
* Version 1.2.0: Column Display (down then across)
*   The default output is now printed in multiple columns, filled
*   from top to bottom and then across, like the real "ls".
*
* Version 1.3.0: Horizontal Column Display (-x)
*   -x  print the entries across the rows (left to right) instead of
*       down then across; the state is kept in the display-mode flags.
*
* Version 1.4.0: Alphabetical Sort
*   Every display mode now renders the entries in alphabetical order,
*   produced with qsort() and a strcmp()-based comparison function.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

/* One directory entry together with the metadata gathered with lstat(). */
struct file_info {
    char *name;
    struct stat st;
};

void do_ls(const char *dir, int long_flag, int horizontal_flag);

/* Convert the st_mode field into the familiar 10-character permission
 * string (e.g. "drwxr-xr-x"), including the setuid/setgid/sticky bits. */
static void mode_to_string(mode_t mode, char buf[11])
{
    if (S_ISDIR(mode))
        buf[0] = 'd';
    else if (S_ISLNK(mode))
        buf[0] = 'l';
    else if (S_ISCHR(mode))
        buf[0] = 'c';
    else if (S_ISBLK(mode))
        buf[0] = 'b';
    else if (S_ISFIFO(mode))
        buf[0] = 'p';
    else if (S_ISSOCK(mode))
        buf[0] = 's';
    else
        buf[0] = '-';

    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';
    buf[10] = '\0';

    /* The special bits replace the execute character (using the capital
     * variant when the corresponding execute bit is not set). */
    if (mode & S_ISUID)
        buf[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        buf[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        buf[9] = (mode & S_IXOTH) ? 't' : 'T';
}

/* Return the user name for uid, or the numeric id as a fallback string. */
static const char *user_name(uid_t uid, char buf[32])
{
    struct passwd *pw = getpwuid(uid);
    if (pw != NULL)
        return pw->pw_name;
    snprintf(buf, 32, "%u", (unsigned int)uid);
    return buf;
}

/* Return the group name for gid, or the numeric id as a fallback string. */
static const char *group_name(gid_t gid, char buf[32])
{
    struct group *gr = getgrgid(gid);
    if (gr != NULL)
        return gr->gr_name;
    snprintf(buf, 32, "%u", (unsigned int)gid);
    return buf;
}

/* Read every non-hidden entry of dir into a dynamically grown array.
 * Returns the array (which the caller must free) and stores the number
 * of entries in *count. Returns NULL if the directory cannot be opened. */
static struct file_info *read_entries(const char *dir, int *count)
{
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return NULL;
    }

    int capacity = 16;
    int n = 0;
    struct file_info *files = malloc(capacity * sizeof *files);
    if (files == NULL) {
        perror("malloc");
        closedir(dp);
        return NULL;
    }

    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')   /* skip hidden files */
            continue;

        if (n == capacity) {
            capacity *= 2;
            struct file_info *tmp = realloc(files, capacity * sizeof *files);
            if (tmp == NULL) {
                perror("realloc");
                break;
            }
            files = tmp;
        }

        files[n].name = strdup(entry->d_name);

        /* lstat() on the full path: it must work no matter which
         * directory we are listing, and it must not follow links. */
        char path[PATH_MAX];
        snprintf(path, sizeof path, "%s/%s", dir, entry->d_name);
        if (lstat(path, &files[n].st) != 0)
            memset(&files[n].st, 0, sizeof files[n].st);

        n++;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
    *count = n;
    return files;
}

/* Print the long listing (ls -l) of the gathered entries. */
static void print_long_listing(struct file_info *files, int count,
                               const char *dir)
{
    int width_links = 1, width_owner = 1, width_group = 1, width_size = 1;
    long total = 0;

    /* First pass: compute the "total" line and the width of every
     * column so that all lines line up exactly like the real ls -l. */
    for (int i = 0; i < count; i++) {
        const struct stat *st = &files[i].st;
        char buf[32];
        int len;

        total += st->st_blocks;   /* st_blocks is counted in 512-byte units */

        len = snprintf(NULL, 0, "%lu", (unsigned long)st->st_nlink);
        if (len > width_links)
            width_links = len;

        len = (int)strlen(user_name(st->st_uid, buf));
        if (len > width_owner)
            width_owner = len;

        len = (int)strlen(group_name(st->st_gid, buf));
        if (len > width_group)
            width_group = len;

        len = snprintf(NULL, 0, "%lld", (long long)st->st_size);
        if (len > width_size)
            width_size = len;
    }

    printf("total %ld\n", total / 2);

    /* Second pass: one line per entry. */
    for (int i = 0; i < count; i++) {
        const struct stat *st = &files[i].st;
        char mode[11], buf1[32], buf2[32];
        char *when = ctime(&st->st_mtime);  /* "Tue Oct 13 08:00:00 2026\n" */

        mode_to_string(st->st_mode, mode);

        printf("%s %*lld %-*s %-*s %*lld %.12s %s",
               mode,
               width_links, (long long)st->st_nlink,
               width_owner, user_name(st->st_uid, buf1),
               width_group, group_name(st->st_gid, buf2),
               width_size, (long long)st->st_size,
               when != NULL ? when + 4 : "?",   /* skip "Day ", keep 12 chars */
               files[i].name);

        /* Symbolic links: ls -l also shows the target of the
         * link, obtained with readlink(). */
        if (S_ISLNK(st->st_mode)) {
            char path[PATH_MAX];
            char target[PATH_MAX];
            ssize_t len;

            snprintf(path, sizeof path, "%s/%s", dir, files[i].name);
            len = readlink(path, target, sizeof target - 1);
            if (len != -1) {
                target[len] = '\0';
                printf(" -> %s", target);
            }
        }

        putchar('\n');
    }
}

/* Return the width of the terminal connected to stdout, falling
 * back to 80 columns when the size cannot be detected (for
 * example when the output is redirected to a file or a pipe). */
static int terminal_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    return 80;
}

/* Print the entries in columns, filling each column from top to
 * bottom before moving to the next column ("down then across"),
 * like the default output of the real ls. */
static void print_columns(struct file_info *files, int count)
{
    if (count == 0)
        return;

    /* The longest name determines the width of every column. */
    int maxlen = 0;
    for (int i = 0; i < count; i++) {
        int len = (int)strlen(files[i].name);
        if (len > maxlen)
            maxlen = len;
    }

    int colwidth = maxlen + 2;          /* two blank columns of padding */
    int ncols = terminal_width() / colwidth;
    if (ncols < 1)
        ncols = 1;
    if (ncols > count)
        ncols = count;
    int nrows = (count + ncols - 1) / ncols;

    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            int idx = col * nrows + row;    /* down then across */
            if (idx >= count)
                break;
            printf("%s", files[idx].name);
            /* Pad every item except the last one of its row. */
            if (idx + nrows < count)
                printf("%*s", colwidth - (int)strlen(files[idx].name), "");
        }
        printf("\n");
    }
}

/* Print the entries in columns, filling each row from left to right
 * and wrapping only when the line is full ("across"), like "ls -x". */
static void print_horizontal(struct file_info *files, int count)
{
    if (count == 0)
        return;

    int maxlen = 0;
    for (int i = 0; i < count; i++) {
        int len = (int)strlen(files[i].name);
        if (len > maxlen)
            maxlen = len;
    }

    int colwidth = maxlen + 2;      /* two blank columns of padding */
    int width = terminal_width();
    int pos = 0;                   /* current horizontal position */

    for (int i = 0; i < count; i++) {
        int namelen = (int)strlen(files[i].name);

        /* Would the next item not fit on this line? Start a new one. */
        if (pos > 0 && pos + colwidth > width) {
            printf("\n");
            pos = 0;
        }

        printf("%s", files[i].name);
        pos += namelen;

        /* Pad to the column width, but never past the terminal edge. */
        int pad = colwidth - namelen;
        if (pos + pad > width)
            pad = width - pos;
        if (pad > 0)
            printf("%*s", pad, "");
        pos += pad;
    }

    if (pos > 0)
        printf("\n");
}

/* Comparison function for qsort(): orders two entries by their name.
 * qsort() knows nothing about our data type, so it hands us two generic
 * pointers that must be cast back to elements of the array being sorted. */
static int compare_names(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info *)a;
    const struct file_info *fb = (const struct file_info *)b;
    return strcmp(fa->name, fb->name);
}

void do_ls(const char *dir, int long_flag, int horizontal_flag)
{
    int count = 0;
    struct file_info *files = read_entries(dir, &count);
    if (files == NULL)
        return;

    /* Sort once, right after reading and before any display function is
     * called, so that every display mode renders alphabetical order. */
    qsort(files, count, sizeof *files, compare_names);

    /* Display mode is decided once, from the two option flags. */
    if (long_flag)
        print_long_listing(files, count, dir);
    else if (horizontal_flag)
        print_horizontal(files, count);
    else
        print_columns(files, count);

    for (int i = 0; i < count; i++)
        free(files[i].name);
    free(files);
}

int main(int argc, char *argv[])
{
    int long_flag = 0;
    int horizontal_flag = 0;
    int opt;

    /* Parse the command-line options with getopt().
     * (getopt() takes a non-const argv, hence char *argv[] above.) */
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
        case 'l':
            long_flag = 1;
            break;
        case 'x':
            horizontal_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-lx] [directory ...]\n", argv[0]);
            return 1;
        }
    }

    if (optind == argc) {
        do_ls(".", long_flag, horizontal_flag);
    } else {
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], long_flag, horizontal_flag);
            puts("");
        }
    }
    return 0;
}
