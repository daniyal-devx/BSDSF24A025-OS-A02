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

(answer added in Feature-2)

**Q2. The st_mode field in struct stat is an integer that contains both the file
type and the permission bits. Explain how you can use bitwise operators (like &)
and predefined macros (like S_IFDIR or S_IRUSR) to extract this information.**

(answer added in Feature-2)

---

## Feature-3: ls-v1.2.0 - Column Display (Down Then Across)

**Q1. Explain the general logic for printing items in a "down then across"
columnar format. Why is a simple single loop through the list of filenames
insufficient for this task?**

(answer added in Feature-3)

**Q2. What is the purpose of the ioctl system call in this context? What would
be the limitations of your program if you only used a fixed-width fallback
(e.g., 80 columns) instead of detecting the terminal size?**

(answer added in Feature-3)

---

## Feature-4: ls-v1.3.0 - Horizontal Column Display (-x Option)

**Q1. Compare the implementation complexity of the "down then across" (vertical)
printing logic versus the "across" (horizontal) printing logic. Which one
requires more pre-calculation and why?**

(answer added in Feature-4)

**Q2. Describe the strategy you used in your code to manage the different display
modes (-l, -x, and default). How did your program decide which function to call
for printing?**

(answer added in Feature-4)

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
