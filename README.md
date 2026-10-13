# BSDSF24A025-OS-A02 - Operating Systems Programming Assignment 02

Re-engineering the Unix `ls` utility in C, feature by feature.

## Build

```sh
make            # builds bin/ls
make clean      # removes obj/ and bin/ artifacts
```

## Usage

```sh
./bin/ls              # columnar listing of the current directory
./bin/ls -l           # long listing format
./bin/ls -x           # horizontal (across) column display
./bin/ls -R           # recursive listing
```

## Versions

| Version | Feature                                    |
|---------|--------------------------------------------|
| v1.0.0  | Initial project setup and build            |
| v1.1.0  | Complete long listing format (`-l`)        |
| v1.2.0  | Column display, down then across           |
| v1.3.0  | Horizontal column display (`-x`)           |
| v1.4.0  | Alphabetical sort (`qsort`)                |
| v1.5.0  | Colorized output based on file type        |
| v1.6.0  | Recursive listing (`-R`)                   |

## Project structure

```
BSDSF24A025-OS-A02/
├── src/        # C source code (ls-v1.0.0.c, extended each feature)
├── bin/        # compiled program
├── obj/        # object files
├── man/        # man page (bonus task)
├── Makefile    # build script
├── README.md   # this file
└── REPORT.md   # analysis report (answers to report questions)
```
