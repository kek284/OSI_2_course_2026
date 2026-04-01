#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Константы для режимов доступа */
#define DIR_MODE 0755
#define FILE_MODE 0644

/* Константы для размеров буферов */
#define READ_BUF_SIZE 1024
#define SYMLINK_BUF_SIZE 1024

/* Константы для возвращаемых значений системных вызовов */
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERROR -1

/* Константы для проверки fd */
#define INVALID_FD -1

/* Константы для проверки указателей */
#define NULL_PTR ((void*)0)

/* Константы для прав доступа (восьмеричные маски) */
#define S_IRWXU_MT 0700
#define S_IRUSR_MT 0400
#define S_IWUSR_MT 0200
#define S_IXUSR_MT 0100
#define S_IRWXG_MT 0070
#define S_IRGRP_MT 0040
#define S_IWGRP_MT 0020
#define S_IXGRP_MT 0010
#define S_IRWXO_MT 0007
#define S_IROTH_MT 0004
#define S_IWOTH_MT 0002
#define S_IXOTH_MT 0001

/* Константы для максимальных значений */
#define MAX_MODE 07777
#define MODE_BASE 8

/* Константы для сообщений об ошибках */
#define ERR_USAGE "Ошибка: неверное количество аргументов\n"
#define ERR_CMD "Ошибка: неизвестная команда\n"
#define ERR_MODE_FORMAT "Ошибка: неверный формат прав доступа. Используйте восьмеричное число (например, 0644)\n"
#define ERR_MODE_RANGE "Ошибка: права доступа не могут превышать %o\n"

/* Константы для флагов open() */
#define OPEN_FLAGS_CREATE (O_CREAT | O_WRONLY | O_EXCL)
#define OPEN_FLAGS_READ O_RDONLY

/* Константы для стандартных потоков */
#define STDIN_FILENO_ST 0
#define STDOUT_FILENO_ST 1
#define STDERR_FILENO_ST 2

void make_dir(char *path) {
    int result = mkdir(path, DIR_MODE);
    if (result == SYSCALL_ERROR) {
        perror("mkdir");
    }
}

void list_dir(char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL_PTR) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL_PTR) {
        /* Пропускаем текущий и родительский каталоги */
        if (strcmp(entry->d_name, ".") != SYSCALL_SUCCESS && 
            strcmp(entry->d_name, "..") != SYSCALL_SUCCESS) {
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
}

void remove_dir(char *path) {
    int result = rmdir(path);
    if (result == SYSCALL_ERROR) {
        perror("rmdir");
    }
}

void create_file(char *path) {
    int fd = open(path, OPEN_FLAGS_CREATE, FILE_MODE);
    if (fd == INVALID_FD) {
        perror("open");
        return;
    }
    close(fd);
}

void cat_file(char *path) {
    int fd = open(path, OPEN_FLAGS_READ);
    if (fd == INVALID_FD) {
        perror("open");
        return;
    }

    char buf[READ_BUF_SIZE];
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > SYSCALL_SUCCESS) {
        ssize_t written = write(STDOUT_FILENO_ST, buf, n);
        if (written == SYSCALL_ERROR) {
            perror("write");
            break;
        }
    }

    if (n == SYSCALL_ERROR) {
        perror("read");
    }

    close(fd);
}

void remove_file(char *path) {
    int result = unlink(path);
    if (result == SYSCALL_ERROR) {
        perror("unlink");
    }
}

void make_symlink(char *target, char *linkname) {
    int result = symlink(target, linkname);
    if (result == SYSCALL_ERROR) {
        perror("symlink");
    }
}

void read_symlink(char *path) {
    char buf[SYMLINK_BUF_SIZE];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == SYSCALL_ERROR) {
        perror("readlink");
        return;
    }
    buf[len] = '\0';
    printf("%s\n", buf);
}

void cat_symlink_target(char *path) {
    /* cat_file выводит содержимое файла, на который указывает ссылка */
    cat_file(path);
}

void remove_symlink(char *path) {
    int result = unlink(path);
    if (result == SYSCALL_ERROR) {
        perror("unlink");
    }
}

void make_hardlink(char *target, char *linkname) {
    int result = link(target, linkname);
    if (result == SYSCALL_ERROR) {
        perror("link");
    }
}

void remove_hardlink(char *path) {
    int result = unlink(path);
    if (result == SYSCALL_ERROR) {
        perror("unlink");
    }
}

void stat_file(char *path) {
    struct stat st;
    int result = stat(path, &st);
    if (result == SYSCALL_ERROR) {
        perror("stat");
        return;
    }

    printf("Права: %o\n", st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
    printf("Число ссылок: %lu\n", (unsigned long)st.st_nlink);
    
    /* Дополнительная информация о типе файла */
    if (S_ISREG(st.st_mode))
        printf("Тип: обычный файл\n");
    else if (S_ISDIR(st.st_mode))
        printf("Тип: каталог\n");
    else if (S_ISLNK(st.st_mode))
        printf("Тип: символьная ссылка\n");
}

void chmod_file(char *path, char *mode_str) {
    char *endptr;
    mode_t mode = strtol(mode_str, &endptr, MODE_BASE);
    
    /* Проверка корректности ввода */
    if (*endptr != '\0' || mode_str[0] == '\0') {
        fprintf(stderr, ERR_MODE_FORMAT);
        return;
    }
    
    /* Проверка диапазона */
    if (mode > MAX_MODE) {
        fprintf(stderr, ERR_MODE_RANGE, MAX_MODE);
        return;
    }

    int result = chmod(path, mode);
    if (result == SYSCALL_ERROR) {
        perror("chmod");
    }
}

int main(int argc, char *argv[]) {
    char *cmd = basename(argv[0]);

    /* Проверка количества аргументов для каждой команды */
    if (strcmp(cmd, "mkdir_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        make_dir(argv[1]);
    }
    else if (strcmp(cmd, "ls_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        list_dir(argv[1]);
    }
    else if (strcmp(cmd, "rmdir_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        remove_dir(argv[1]);
    }
    else if (strcmp(cmd, "touch_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        create_file(argv[1]);
    }
    else if (strcmp(cmd, "cat_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        cat_file(argv[1]);
    }
    else if (strcmp(cmd, "rm_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        remove_file(argv[1]);
    }
    else if (strcmp(cmd, "lns_cmd") == SYSCALL_SUCCESS) {
        if (argc != 3) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        make_symlink(argv[1], argv[2]);
    }
    else if (strcmp(cmd, "readlink_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        read_symlink(argv[1]);
    }
    else if (strcmp(cmd, "catlink_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        cat_symlink_target(argv[1]);
    }
    else if (strcmp(cmd, "rmlinks_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        remove_symlink(argv[1]);
    }
    else if (strcmp(cmd, "ln_cmd") == SYSCALL_SUCCESS) {
        if (argc != 3) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        make_hardlink(argv[1], argv[2]);
    }
    else if (strcmp(cmd, "rmhard_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        remove_hardlink(argv[1]);
    }
    else if (strcmp(cmd, "stat_cmd") == SYSCALL_SUCCESS) {
        if (argc != 2) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        stat_file(argv[1]);
    }
    else if (strcmp(cmd, "chmod_cmd") == SYSCALL_SUCCESS) {
        if (argc != 3) {
            fprintf(stderr, ERR_USAGE);
            return SYSCALL_ERROR;
        }
        chmod_file(argv[1], argv[2]);
    }
    else {
        fprintf(stderr, ERR_CMD);
        return SYSCALL_ERROR;
    }

    return SYSCALL_SUCCESS;
}