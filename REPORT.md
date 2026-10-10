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

