/*
* Programming Assignment 02: lsv1.0.0
* This is the source file of version 1.0.0
* Read the write-up of the assignment to add the features to this base version
* Usage:
*       $ lsv1.0.0 
*       % lsv1.0.0  /home
*       $ lsv1.0.0  /home/kali/   /etc/
*/
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

extern int errno;
enum display_mode { MODE_DEFAULT, MODE_LONG, MODE_HORIZONTAL };
void do_ls(const char *dir, enum display_mode mode, int recursive);
void print_long(const char *dir, const char *name);
void mode_to_str(mode_t mode, char *str);
void print_columns(const char *dir, char **names, int count, int maxlen);
void print_horizontal(const char *dir, char **names, int count, int maxlen);
int compare_names(const void *a, const void *b);
void print_colored(const char *dir, const char *name);

int main(int argc, char *argv[])
{
    int opt;
    enum display_mode mode = MODE_DEFAULT;
    int recursive = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        case 'R':
            recursive = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l | -x] [-R] [dir...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode, recursive);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (recursive)
            {
                if (i > optind)
                    puts("");
            }
            else
            {
                printf("Directory listing of %s : \n", argv[i]);
            }
            do_ls(argv[i], mode, recursive);
            if (!recursive)
                puts("");
        }
    }
    return 0;
}


void do_ls(const char *dir, enum display_mode mode, int recursive)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    if (recursive)
        printf("%s:\n", dir);

    char **names = NULL;
    int count = 0, capacity = 0, maxlen = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            capacity = capacity ? capacity * 2 : 16;
            names = realloc(names, capacity * sizeof(char *));
            if (names == NULL)
            {
                perror("realloc");
                closedir(dp);
                return;
            }
        }

        names[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > maxlen)
            maxlen = len;
        count++;
    }

    if (errno != 0)
        perror("readdir failed");
    closedir(dp);

    qsort(names, count, sizeof(char *), compare_names);

    switch (mode)
    {
    case MODE_LONG:
        for (int i = 0; i < count; i++)
            print_long(dir, names[i]);
        break;
    case MODE_HORIZONTAL:
        print_horizontal(dir, names, count, maxlen);
        break;
    default:
        print_columns(dir, names, count, maxlen);
        break;
    }

    if (recursive)
    {
        for (int i = 0; i < count; i++)
        {
            char path[1024];
            struct stat st;

            snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
            if (lstat(path, &st) == -1)
                continue;
            if (S_ISDIR(st.st_mode))
            {
                printf("\n");
                do_ls(path, mode, recursive);
            }
        }
    }

    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

void print_long(const char *dir, const char *name)
{
    char path[1024];
    struct stat st;
    char perms[11];
    char owner[32], group[32];
    struct passwd *pw;
    struct group *gr;

    snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (lstat(path, &st) == -1)
    {
        perror("lstat");
        return;
    }

    mode_to_str(st.st_mode, perms);

    pw = getpwuid(st.st_uid);
    gr = getgrgid(st.st_gid);
    if (pw) snprintf(owner, sizeof(owner), "%s", pw->pw_name);
    else    snprintf(owner, sizeof(owner), "%u", st.st_uid);
    if (gr) snprintf(group, sizeof(group), "%s", gr->gr_name);
    else    snprintf(group, sizeof(group), "%u", st.st_gid);

    printf("%s %3lu %-8s %-8s %8ld %.12s ", perms,
           (unsigned long)st.st_nlink, owner, group,
           (long)st.st_size, ctime(&st.st_mtime) + 4);
    print_colored(dir, name);
    printf("\n");
}

void mode_to_str(mode_t mode, char *str)
{
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

void print_columns(const char *dir, char **names, int count, int maxlen)
{
    struct winsize ws;
    int width = 80; /* fallback */

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        width = ws.ws_col;

    int colw = maxlen + 2;
    int cols = width / colw;
    if (cols < 1)
        cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r + c * rows;
            if (idx < count)
            {
                print_colored(dir, names[idx]);
                printf("%*s", colw - (int)strlen(names[idx]), "");
            }
        }
        printf("\n");
    }
}

void print_horizontal(const char *dir, char **names, int count, int maxlen)
{
    struct winsize ws;
    int width = 80; /* fallback */

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        width = ws.ws_col;

    int colw = maxlen + 2;
    int pos = 0;

    for (int i = 0; i < count; i++)
    {
        if (pos > 0 && pos + colw > width)
        {
            printf("\n");
            pos = 0;
        }
        print_colored(dir, names[i]);
        printf("%*s", colw - (int)strlen(names[i]), "");
        pos += colw;
    }
    printf("\n");
}

int compare_names(const void *a, const void *b)
{
    const char *s1 = *(char *const *)a;
    const char *s2 = *(char *const *)b;
    return strcmp(s1, s2);
}

void print_colored(const char *dir, const char *name)
{
    char path[1024];
    struct stat st;
    const char *color = "";

    snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (lstat(path, &st) == 0)
    {
        mode_t m = st.st_mode;

        if (S_ISLNK(m))
            color = COLOR_PINK;
        else if (S_ISDIR(m))
            color = COLOR_BLUE;
        else if (S_ISCHR(m) || S_ISBLK(m) || S_ISSOCK(m) || S_ISFIFO(m))
            color = COLOR_REVERSE;
        else if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
            color = COLOR_RED;
        else if (m & (S_IXUSR | S_IXGRP | S_IXOTH))
            color = COLOR_GREEN;
    }

    printf("%s%s%s", color, name, COLOR_RESET);
}
