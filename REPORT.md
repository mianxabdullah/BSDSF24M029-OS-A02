# Feature-2 Report Questions (ls-v1.1.0)

## Q1: What is the crucial difference between stat() and lstat()? When is lstat() more appropriate in ls?

`stat()` follows symbolic links and returns metadata of the file the link points to. 
`lstat()` does not follow them and returns metadata of the link itself (its type is `S_IFLNK` and its size is the length of the target path).

`ls -l` should use `lstat()`, because it must show a symlink as a link (type `l`, with the link's own permissions and size) and not as the file it points to. `stat()` would also fail on dangling links, where the target does not exist.

## Q2: How can bitwise operators and macros extract file type and permissions from st_mode?

`st_mode` packs the file type and the permission bits into one integer, so we use bitwise AND with masks to isolate the bits we need.

- **File type:** `mode & S_IFMT` isolates the type bits, and the result is compared with a constant, e.g. `(mode & S_IFMT) == S_IFDIR`. The macros `S_ISDIR()`, `S_ISREG()` and `S_ISLNK()` do exactly this.
- **Permissions:** each permission is one bit. `mode & S_IRUSR` is non-zero if the owner can read. The same applies to `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IWGRP`, `S_IXGRP`, `S_IROTH`, `S_IWOTH` and `S_IXOTH`. In my code each test sets the matching character of the permission string to `r`, `w`, `x` or `-`.
- **Special bits:** `S_ISUID`, `S_ISGID` and `S_ISVTX` (setuid, setgid, sticky) are tested the same way and replace the `x` slot with `s`/`S` or `t`/`T`. Lowercase means the execute bit is also set, and uppercase means it is not.


# Feature-3 Report Questions (ls-v1.2.0)

## Q1: Explain the logic for "down then across" printing. Why is a single loop insufficient?

First all filenames are read into an array, and the number of columns is calculated as `terminal_width / (max_filename_length + spacing)`. The number of rows is the ceiling of `total_files / columns`. Printing then goes row by row. For row `r` and column `c`, the item printed is `names[r + c * rows]`, so each column holds consecutive names going down, and the next column continues after it.

A single loop over the array prints names in sequential order (0, 1, 2, ...), which would fill a row left to right ("across"). Down-then-across needs the items of one row to be `rows` positions apart in the array, so we need a nested loop (rows outside, columns inside) and an index calculation. We also need the total count and the longest name before printing the first line, so the layout can be calculated.

## Q2: What is the purpose of ioctl here? What are the limitations of a fixed 80-column fallback?

`ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` asks the terminal driver for the current window size (`ws.ws_col` is the number of columns), so the program can fit as many columns as the terminal can actually show.

With a fixed width of 80, the output would not adapt. On a wide terminal, space would be wasted and the listing would be longer than needed. On a narrow terminal (less than 80 columns), lines would wrap in the middle of the layout and the columns would be misaligned. The program also could not react to the window being resized.
