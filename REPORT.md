# REPORT.md - Analysis Report

Student: Daniyal Usman
Roll No: BSDSF24A025
Assignment: Operating Systems Programming Assignment - 02 (Re-engineering `ls`)

This report contains the answers to the report questions of every feature,
from Feature-2 (v1.1.0) to Feature-7 (v1.6.0).

---

## Feature-2: ls-v1.1.0 - Complete Long Listing Format

**Q1. What is the crucial difference between the stat() and lstat() system calls?
In the context of the ls command, when is it more appropriate to use lstat()?**

The crucial difference is in how the two calls treat symbolic links. `stat()`
follows a symbolic link and returns the metadata of the file the link points
to, whereas `lstat()` does not follow the link and returns the metadata of the
link itself.

In the context of `ls`, `lstat()` is the appropriate call (and is what the
real `ls` uses by default). `ls` must describe the directory entry itself:
a symbolic link must be shown with the type character `l` and with a size
equal to the length of its target path, not the size of the target file. If
`ls` used `stat()`, every symlink would be indistinguishable from its target,
and a dangling link (whose target does not exist) would make `stat()` fail
with `ENOENT`, so the entry could not be listed at all. `stat()` is only the
right choice when the caller explicitly wants to resolve the link, such as
when the `-L` option is given to `ls`.

**Q2. The st_mode field in struct stat is an integer that contains both the file
type and the permission bits. Explain how you can use bitwise operators (like &)
and predefined macros (like S_IFDIR or S_IRUSR) to extract this information.**

`st_mode` packs the file type and the permission/special bits into one integer,
so bitwise operations are used to extract each piece:

- **File type**: the type bits are masked out with the bit mask `S_IFMT`
  (`st_mode & S_IFMT`) and the result is compared with the type constants
  `S_IFDIR`, `S_IFREG`, `S_IFLNK`, `S_IFCHR`, `S_IFBLK`, `S_IFIFO`,
  `S_IFSOCK`. In practice one uses the convenience macros `S_ISDIR(st_mode)`,
  `S_ISREG(st_mode)`, `S_ISLNK(st_mode)`, etc., which perform exactly that
  mask-and-compare internally. In my code `mode_to_string()` uses these macros
  to choose the first character of the permission string (`d`, `-`, `l`, ...).
- **Permission bits**: each bit is tested individually with the bitwise AND
  operator, e.g. `(st_mode & S_IRUSR)` is non-zero when the owner-read bit is
  set. The same pattern applies to `S_IWUSR`/`S_IXUSR` (owner),
  `S_IRGRP`/`S_IWGRP`/`S_IXGRP` (group) and `S_IROTH`/`S_IWOTH`/`S_IXOTH`
  (others). My code uses these tests to build the `rwx` triplets.
- **Special bits**: `S_ISUID`, `S_ISGID` and `S_ISVTX` are tested the same
  way; when set they replace the corresponding execute character with
  `s`/`S` (setuid/setgid) or `t`/`T` (sticky), again using `&`.

---

## Feature-3: ls-v1.2.0 - Column Display (Down Then Across)

**Q1. Explain the general logic for printing items in a "down then across"
columnar format. Why is a simple single loop through the list of filenames
insufficient for this task?**

The entries are first arranged in a conceptual grid of `ncols` columns
and `nrows` rows that is filled **column by column**: the first `nrows`
filenames go into column 0 (top to bottom), the next `nrows` into
column 1, and so on. Printing therefore iterates **row by row**: for
row `r` the program prints the items stored at indices `r`,
`r + nrows`, `r + 2*nrows`, ... i.e. the `r`-th item of every column,
padding each item to the common column width so the columns line up.

A single loop through the array is insufficient for two reasons:
(1) the array is in column-major order, so printing it straight through
would fill the first row completely before starting the second — that is
"across then down", the layout of `ls -x`, not of `ls`; and
(2) the layout parameters (longest filename, column width, number of
columns and rows) must all be known *before* the first line is
printed, so at least one gathering pass and the row/column index
arithmetic are unavoidable.

**Q2. What is the purpose of the ioctl system call in this context? What
would be the limitations of your program if you only used a fixed-width
fallback (e.g., 80 columns) instead of detecting the terminal size?**

`ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` asks the terminal driver for
the size of the window (rows and columns) attached to stdout. It lets
the program compute how many filename columns actually fit on screen,
so the layout adapts when the terminal is resized.

If only a fixed 80-column fallback were used, the output would be
wrong in both directions: on a narrower terminal the lines would
overflow the screen and the columns would be misaligned by the
terminal's own wrapping, while on a wider terminal space would be
wasted and fewer entries would be shown per line. The fallback is
still necessary for the case where stdout is not a terminal at all
(redirected to a file or a pipe), because `ioctl` fails there and no
size can be detected.

---

## Feature-4: ls-v1.3.0 - Horizontal Column Display (-x Option)

**Q1. Compare the implementation complexity of the "down then across" (vertical)
printing logic versus the "across" (horizontal) printing logic. Which one
requires more pre-calculation and why?**

The vertical ("down then across") logic needs far more pre-calculation. Before
a single character can be printed it must know the longest filename, the column
width, the number of columns, and — crucially — the number of rows, because the
number of rows is what defines the *stride* used to address the array
(`index = col * nrows + row`). The row count depends on the item count, the
column count and the terminal width, and it is needed even to decide which
items belong to the first printed line. Each printed line is then rebuilt from
several non-contiguous array positions.

The horizontal ("across") logic needs only the column width and the terminal
width, both of which are constant for the whole run. It simply walks the array
in order and keeps a running `pos` variable with the current horizontal
position, wrapping when `pos + colwidth > width`. No row count, no index
arithmetic and no second pass are needed — which is exactly why, in the real
`ls`, `-x` is the simpler mode.

**Q2. Describe the strategy you used in your code to manage the different display
modes (-l, -x, and default). How did your program decide which function to call
for printing?**

The state is kept in two independent `int` flags in `main()` — `long_flag` and
`horizontal_flag` — each set by its own `case` in the `getopt()` loop (`"lx"`).
These flags are then passed down to `do_ls()`, which gathers the directory
entries once and dispatches on them with a simple `if / else if / else` chain:
long listing if `long_flag`, otherwise horizontal printing if `horizontal_flag`,
otherwise the default down-then-across columns. This keeps the decision in a
single place (rather than mixing the checks into each print function), means the
entries are read from the directory exactly once regardless of the mode, and
allows options to be combined later simply by adding another flag to the chain.

---

## Feature-5: ls-v1.4.0 - Alphabetical Sort

**Q1. Why is it necessary to read all directory entries into memory before you
can sort them? What are the potential drawbacks of this approach for directories
containing millions of files?**

Sorting is a rearrangement of an entire collection: `qsort()` permutes the
array as a whole, and the columnar display needs the total number of entries
plus the longest name *before* printing anything. Since `readdir()` only
produces one entry per call, in whatever order the file system returns them,
the entries have to be accumulated in a dynamic array first; only then can the
collection be sorted as a unit and afterwards traversed in the sorted order.

The drawbacks of this "load everything into memory" approach are:
- **Memory pressure**: a directory with millions of files needs
  `count * sizeof(struct file_info)` plus one `strdup()` allocation per name,
  which can reach hundreds of megabytes and may trigger swapping or an
  allocation failure. This is why real `ls` does not do it this way.
- **Latency and useless work**: nothing can be printed until the *entire*
  directory has been read and sorted, so the user waits much longer, and the
  program does the allocation and sort work even when the output is piped to
  `head`.
- **Scaling and errors**: the reallocation and sorting costs grow linearly and
  n·log n respectively, and each `strdup()` is another allocation that can
  fail in the middle of a huge directory.
- **External sort**: the usual alternative for very large directories is a
  multi-pass external merge sort (sort chunks to temporary files, then merge),
  trading memory for extra I/O.

**Q2. Explain the purpose and signature of the comparison function required by
qsort(). How does it work, and why must it take const void * arguments?**

The comparison function tells `qsort()` how to order two elements of *our*
array. Its signature must be exactly:

```c
int compare(const void *a, const void *b);
```

`qsort()` has no type information — it only knows it is sorting an array of
`nmemb` elements of `size` bytes — so it passes each element as a generic
`void *` pointing directly at that element of the array. The comparison
function therefore casts the two pointers back to pointers to the array's
element type (`const struct file_info *`) and compares whatever member carries
the ordering. In my code it is:

```c
static int compare_names(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info *)a;
    const struct file_info *fb = (const struct file_info *)b;
    return strcmp(fa->name, fb->name);
}
```

It returns a negative value when `a` must come first, zero when the two are
equal, and a positive value when `b` must come first; here that ordering is
produced by `strcmp()`, which is why the comparison is alphabetical (and
case-sensitive, exactly like the real `ls` in the C locale). The arguments are
`const void *` because the comparator must only *read* the elements: `qsort()`
is allowed to pass pointers to `const` data, and declaring them `const` makes
it a compile error if the function tries to modify an element while sorting.

---

## Feature-6: ls-v1.5.0 - Colorized Output Based on File Type

**Q1. How do ANSI escape codes work to produce color in a standard Linux
terminal? Show the specific code sequence for printing text in green.**

A terminal emulator does not interpret colour names — it interprets bytes.
An ANSI escape code is a short control sequence, starting with the ESC
character (`\033`, ASCII 27), that the terminal driver consumes and acts on
instead of displaying. The general form is `ESC [ parameters m`, where the
final `m` means "manipulate display attributes" and the numeric parameters
select the attribute:

- `0` — reset all attributes (back to the default foreground);
- `1` — bold / high intensity;
- `30`–`37` — the eight standard foreground colours (30 black, 31 red,
  32 green, 33 yellow, 34 blue, 35 magenta, 36 cyan, 37 white);
- `40`–`47` — the matching background colours;
- `7` — reverse video (swap foreground and background).

So printing text in green is the three-part sequence "switch to green → text
→ switch back":

```c
printf("\033[0;32m");   /* 0 = reset, 32 = green foreground */
printf("Hello");
printf("\033[0m");      /* restore the default colour        */
```

which is exactly the pattern my `ANSI_GREEN` / `ANSI_RESET` macros and
`print_coloured_name()` helper implement. The reset is essential: without it
the colour would "bleed" into every following line. Note that the escape
sequence itself takes no space on screen, which is why my column padding is
computed from `strlen(name)` and not from the length of the printed string.

**Q2. To color an executable file, you need to check its permission bits. Explain
which bits in the st_mode field you need to check to determine if a file is
executable by the owner, group, or others.**

`st_mode` stores the three execute bits in its low permission bits, one per
class of user, and each is tested with a bitwise AND:

- `S_IXUSR` (0100 octal) — the **execute/search bit for the owner**;
- `S_IXGRP` (0010 octal) — the **execute bit for the group**;
- `S_IXOTH` (0001 octal) — the **execute bit for others**.

To decide *whether a file is executable at all* — which is the question `ls`
actually asks when colouring, since it must mark a file green if anybody can
run it — the three are OR-ed together in a single test:

```c
if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        /* executable → green */
```

Because `&` has to bind all the bits for the expression to be non-zero, this
one test is true when *any* of the three execute bits is set, i.e. when the
mode is `rwx`, `r-x`, `--x`, and so on for at least one class. Note that the
kernel never sets an execute bit without the matching read bit, so this is
safe. If a program needed to distinguish *who* may execute, it would test the
bits separately (`st_mode & S_IXUSR`, `st_mode & S_IXGRP`, `st_mode & S_IXOTH`)
and read each one's truth value individually. The same masking technique is
reused for the permission string (`S_IRUSR`, `S_IWUSR`, …) and for the
special bits `S_ISUID`, `S_ISGID` and `S_ISVTX`.

---

## Feature-7: ls-v1.6.0 - Recursive Listing (-R Option)

**Q1. In a recursive function, what is a "base case"? In the context of your
recursive ls, what is the base case that stops the recursion from continuing
forever?**

A **base case** is the condition in a recursive function under which it stops
calling itself and simply returns its result — the terminating branch of the
recursion. Every recursive function needs at least one, otherwise the calls
repeat forever and the program ends in a stack overflow (infinite recursion).

In my recursive `ls`, the base case is **a directory that contains no
subdirectory to descend into** (only regular files, or nothing at all). Such a
directory is read, sorted, printed, and then the recursion loop finds no
entry satisfying `S_ISDIR(...)`, so it makes no further recursive call and
returns to its caller. This "leaf directory" case is what terminates every
chain of calls.

Equally important is that the recursion can never revisit a directory it has
already listed, and `read_entries()` guarantees this by skipping every entry
whose name begins with a dot — which excludes the two self-referencing
directories `.` (this directory) and `..` (its parent). Without that filter,
`do_ls(".")` would call `do_ls("./.")`, then `do_ls("././.")`, and so on
without end. Combined with the base case above, every branch of the recursion
is finite, and the total number of calls is exactly the number of directories
in the tree.

**Q2. Explain why it is essential to construct a full path (e.g.,
"parent_dir/subdir") before making a recursive call. What would happen if you
simply called do_ls("subdir") from within the do_ls("parent_dir") function call?**

The `dir` argument is the *complete filesystem path* that the function will
hand to `opendir()`. A directory entry read from the parent gives only a bare
name (what `readdir()` returns — never a path), so before recursing the program
must join that name with the path it is currently listing
(`snprintf(path, ..., "%s/%s", dir, files[i].name)`). The child call then
receives `"./src"`, which `opendir()` resolves relative to the process's
current working directory — and that is correct only because the current
working directory never changes.

If `do_ls("subdir")` were called instead, the name would be resolved relative
to the current working directory, which is the directory the *user* launched
the program in, not the parent directory being listed. The consequences are:

- **Listing the wrong directory (or nothing):** while listing `/home/user/docs`,
  a call with `"sub"` would open `./sub` relative to the *original* working
  directory. The recursion would either descend into an entirely unrelated
  directory, or fail with "Cannot open directory" if no such directory exists
  there — so nested trees would simply be missing from the output.
- **Wrong metadata:** the `lstat()` calls build `"%s/%s"` paths too, so every
  size, permission and link count shown for the subdirectory entries would
  belong to the wrong file — or be missing entirely.
- **Broken recursion:** once inside the wrong directory, every further level
  would be resolved from the same wrong base, so the traversal loses its
  meaning completely and cannot reach the intended tree.

Building the full path at every level is therefore what makes the descent
correct and keeps each recursive call self-contained: `do_ls()` only ever needs
one argument — an absolute or working-directory-relative *path* — to describe
exactly which directory it must list.
