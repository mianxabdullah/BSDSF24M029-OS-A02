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

# Feature-4 Report Questions (ls-v1.3.0)

## Q1: Compare the complexity of "down then across" vs "across" printing. Which needs more pre-calculation?

"Down then across" is more complex. Before printing anything it must know the total number of files, the number of columns that fit (`terminal_width / column_width`), and the number of rows (`ceil(count / columns)`). Printing then needs a nested loop with the index formula `names[row + col * rows]`.

"Across" needs almost no pre-calculation. It only needs the column width and the terminal width. It loops over the array once in order, prints each name padded to the column width, and keeps a running position counter. When the next name would exceed the terminal width, it prints a newline and resets the counter.

## Q2: How did you manage the different display modes? How does the program decide which function to call?

I used an `enum display_mode` with three values: `MODE_DEFAULT`, `MODE_LONG` and `MODE_HORIZONTAL`. The `getopt()` loop in `main` sets the variable `mode` when it sees `-l` or `-x`, and `mode` is passed to `do_ls()`. After `do_ls()` has read all filenames into an array, a `switch (mode)` calls the matching function: `print_long()` for `-l`, `print_horizontal()` for `-x`, and `print_columns()` (down then across) as the default. Adding a new mode only needs a new enum value, a new case, and a new print function.

# Feature-5 Report Questions (ls-v1.4.0)

## Q1: Why must all entries be read into memory before sorting? What are the drawbacks for directories with millions of files?

`readdir()` returns entries in whatever order the file system stores them, one at a time. A sort needs to compare every entry with the others, so the complete list must exist before the first item can be printed. A name that comes last in the directory may belong first alphabetically.

For a directory with millions of files, this has two drawbacks. Memory use grows with the number of entries (every name is copied with `strdup` and the pointer array keeps growing through `realloc`). Also, nothing is printed until the whole directory has been read and sorted, so the user waits a long time and sees no output. Sorting itself also costs O(n log n) time.

## Q2: What is the purpose and signature of the qsort() comparison function? Why const void * arguments?

`qsort()` is a generic function that sorts an array of any type, so it cannot know how to compare the elements. The comparison function tells it. The signature is `int compare(const void *a, const void *b)`. It must return a negative number if `a` should come before `b`, zero if they are equal, and a positive number if `a` should come after `b`.

`qsort()` passes pointers to the array elements. Since the elements here are `char *`, each argument is really a `char **`, so the function casts it (`*(char *const *)a`) to get the string and then calls `strcmp()`. The parameters are `const void *` because `qsort()` works with any element type, and `void *` is the generic pointer type in C. `const` promises that the comparison will not modify the elements being sorted.

# Feature-6 Report Questions (ls-v1.5.0)

## Q1: How do ANSI escape codes produce color? Show the sequence for green text.

ANSI escape codes are special character sequences that the terminal interprets as commands instead of printing them. A sequence starts with the escape character (`\033`, ASCII 27) followed by `[`, then one or more numbers separated by `;`, and ends with `m`, which means "set text style". The terminal applies the style to everything printed afterwards, until a reset code (`\033[0m`) restores the defaults.

Green text:
```c
printf("\033[0;32m%s\033[0m", "hello");
```
`\033[0;32m` switches on green (`32` is the green foreground color), `hello` is printed in green, and `\033[0m` resets the color. Because the codes take up bytes but no screen space, `printf("%-*s")` padding miscounts colored names, so in my code the padding is added separately after printing the name.

## Q2: Which bits of st_mode determine whether a file is executable by owner, group, or others?

Three permission bits are checked, each with bitwise AND:
- `st_mode & S_IXUSR` (0100): execute permission for the owner
- `st_mode & S_IXGRP` (0010): execute permission for the group
- `st_mode & S_IXOTH` (0001): execute permission for others

If any of them is non-zero, the file is executable by someone, and my code colors it green. A directory also has these bits set, so the directory check (`S_ISDIR`) must come first, otherwise directories would be colored as executables.

# Feature-7 Report Questions (ls-v1.6.0)

## Q1: What is a "base case" in a recursive function? What is the base case in your recursive ls?

The base case is the condition under which a recursive function stops calling itself and returns directly. Without it the recursion would never end.

In my `do_ls`, the base case is a directory that contains no subdirectories: the loop over its entries finds no `S_ISDIR` entry, so no further recursive call is made and the function returns. A directory that cannot be opened (`opendir()` returns NULL) is also a base case, since the function prints an error and returns. Hidden entries (including `.` and `..`) are skipped when reading, so the function never recurses into the current or parent directory. Using `lstat()` means symbolic links to directories are not followed, which prevents loops.

## Q2: Why must a full path be built before the recursive call? What if do_ls("subdir") was called from do_ls("parent_dir")?

`opendir()` and `lstat()` resolve relative names against the current working directory of the process, which does not change when `do_ls` descends into a directory. The entry name alone (`subdir`) only means something relative to its parent, so the program needs the full path (`parent_dir/subdir`).

If `do_ls("subdir")` was called directly, the program would look for `subdir` in the directory where it was started. It would either fail with "Cannot open directory" or, worse, list a different directory that happens to have the same name. Nested levels would fail in the same way. The full path is also what is printed in the `dir:` header, so the output shows exactly where each listing comes from.
