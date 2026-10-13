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

(answer added in Feature-5)

**Q2. Explain the purpose and signature of the comparison function required by
qsort(). How does it work, and why must it take const void * arguments?**

(answer added in Feature-5)

---

## Feature-6: ls-v1.5.0 - Colorized Output Based on File Type

**Q1. How do ANSI escape codes work to produce color in a standard Linux
terminal? Show the specific code sequence for printing text in green.**

(answer added in Feature-6)

**Q2. To color an executable file, you need to check its permission bits. Explain
which bits in the st_mode field you need to check to determine if a file is
executable by the owner, group, or others.**

(answer added in Feature-6)

---

## Feature-7: ls-v1.6.0 - Recursive Listing (-R Option)

**Q1. In a recursive function, what is a "base case"? In the context of your
recursive ls, what is the base case that stops the recursion from continuing
forever?**

(answer added in Feature-7)

**Q2. Explain why it is essential to construct a full path (e.g.,
"parent_dir/subdir") before making a recursive call. What would happen if you
simply called do_ls("subdir") from within the do_ls("parent_dir") function call?**

(answer added in Feature-7)
