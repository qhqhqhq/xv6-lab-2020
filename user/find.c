#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char pwd[512];
char *p = pwd;

void find(char *dirname, char *filename) {
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(dirname, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", dirname);
        return;
    }
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", dirname);
        close(fd);
        return;
    }
    if (st.type != T_DIR) {
        fprintf(2, "find: %s is not directory\n", dirname);
        close(fd);
        return;
    }
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        if (!strcmp(".", de.name) || !strcmp("..", de.name) ) continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(pwd, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", pwd);
            continue;
        }
        switch (st.type)
        {
        case T_FILE:
            if (!strcmp(filename, de.name)) {
                fprintf(1, "%s\n", pwd);
            }
            break;
        case T_DIR:
            char *temp_p = p;
            p = p + strlen(de.name);
            *p++ = '/';
            *p = 0;
            find(pwd, filename);
            p = temp_p;
            break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "find: argc wrong\n");
        exit(0);
    }

    char *dirname = argv[1];
    char *filename = argv[2];

    strcpy(pwd, dirname);
    p = pwd + strlen(dirname);
    *p++ = '/';
    *p = 0;

    find(dirname, filename);

    exit(0);
}