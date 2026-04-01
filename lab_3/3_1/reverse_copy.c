#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH_LEN 4096
#define BUFFER_SIZE 8192

#define DIR_PERMISSIONS 0755

#define READ_MODE "rb"
#define WRITE_MODE "wb"

#define DOT "."
#define DOT_DOT ".."

#define PATH_SEPARATOR '/'

#define SUCCESS 0
#define ERROR   -1

// =========================
// Реверс строки
// =========================
void reverse_string(const char *source_string, char *destination_string) {
    size_t length = strlen(source_string);

    for (size_t i = 0; i < length; i++) {
        destination_string[i] = source_string[length - 1 - i];
    }

    destination_string[length] = '\0';
}

const char* get_basename(const char *file_path) {
    const char *last_slash = strrchr(file_path, PATH_SEPARATOR);
    return (last_slash) ? last_slash + 1 : file_path;
}

int safe_snprintf(char *destination_buffer, size_t buffer_size, const char *format_string,
                  const char *first_argument, const char *second_argument) {
    int written_chars = snprintf(destination_buffer, buffer_size, format_string, 
                                  first_argument, second_argument);

    if (written_chars < 0 || (size_t)written_chars >= buffer_size) {
        return ERROR;
    }

    return SUCCESS;
}

size_t safe_strlcat(char *destination, const char *source, size_t buffer_size) {
    size_t dest_len = strlen(destination);
    size_t src_len = strlen(source);
    size_t total_len = dest_len + src_len;
    size_t copy_len;
    
    if (buffer_size <= dest_len) {
        return buffer_size + src_len;
    }
    
    copy_len = (src_len < buffer_size - dest_len) ? src_len : (buffer_size - dest_len - 1);
    
    if (copy_len > 0) {
        memcpy(destination + dest_len, source, copy_len);
    }
    
    destination[dest_len + copy_len] = '\0';
    
    return total_len;
}

int reverse_file_content(const char *source_file_path, const char *destination_file_path) {
    FILE *source_file = fopen(source_file_path, READ_MODE);
    if (!source_file) {
        perror("fopen src");
        return ERROR;
    }

    FILE *destination_file = fopen(destination_file_path, WRITE_MODE);
    if (!destination_file) {
        perror("fopen dst");
        fclose(source_file);
        return ERROR;
    }

    if (fseek(source_file, 0, SEEK_END) != SUCCESS) {
        perror("fseek");
        fclose(source_file);
        fclose(destination_file);
        return ERROR;
    }

    long file_size = ftell(source_file);
    if (file_size < 0) {
        perror("ftell");
        fclose(source_file);
        fclose(destination_file);
        return ERROR;
    }

    char buffer[BUFFER_SIZE];
    long remaining_bytes = file_size;

    while (remaining_bytes > 0) {
        long chunk_size = (remaining_bytes > BUFFER_SIZE) ? BUFFER_SIZE : remaining_bytes;
        long read_position = remaining_bytes - chunk_size;

        if (fseek(source_file, read_position, SEEK_SET) != SUCCESS) {
            perror("fseek");
            fclose(source_file);
            fclose(destination_file);
            return ERROR;
        }

        size_t bytes_read = fread(buffer, 1, (size_t)chunk_size, source_file);
        if (bytes_read != (size_t)chunk_size) {
            perror("fread");
            fclose(source_file);
            fclose(destination_file);
            return ERROR;
        }

        for (size_t i = 0; i < bytes_read / 2; i++) {
            char temp_char = buffer[i];
            buffer[i] = buffer[bytes_read - 1 - i];
            buffer[bytes_read - 1 - i] = temp_char;
        }

        if (fwrite(buffer, 1, bytes_read, destination_file) != bytes_read) {
            perror("fwrite");
            fclose(source_file);
            fclose(destination_file);
            return ERROR;
        }

        remaining_bytes -= (long)bytes_read;
    }

    fclose(source_file);
    fclose(destination_file);

    return SUCCESS;
}

int process_directory(const char *source_directory, const char *destination_directory) {
    DIR *directory_stream = opendir(source_directory);
    if (!directory_stream) {
        perror("opendir");
        return ERROR;
    }

    if (mkdir(destination_directory, DIR_PERMISSIONS) != SUCCESS && errno != EEXIST) {
        perror("mkdir");
        closedir(directory_stream);
        return ERROR;
    }

    struct dirent *directory_entry;

    while ((directory_entry = readdir(directory_stream)) != NULL) {

        if (strcmp(directory_entry->d_name, DOT) == SUCCESS ||
            strcmp(directory_entry->d_name, DOT_DOT) == SUCCESS) {
            continue;
        }

        char source_full_path[MAX_PATH_LEN];
        
        if (safe_snprintf(source_full_path, sizeof(source_full_path), "%s/%s",
                          source_directory, directory_entry->d_name) != SUCCESS) {
            fprintf(stderr, "Path too long (src)\n");
            continue;
        }

        struct stat file_info;
        if (lstat(source_full_path, &file_info) != SUCCESS) {
            perror("lstat");
            continue;
        }

        char reversed_name[MAX_PATH_LEN];
        reverse_string(directory_entry->d_name, reversed_name);

        char destination_full_path[MAX_PATH_LEN];
        if (safe_snprintf(destination_full_path, sizeof(destination_full_path), "%s/%s",
                          destination_directory, reversed_name) != SUCCESS) {
            fprintf(stderr, "Path too long (dst)\n");
            continue;
        }

        if (S_ISREG(file_info.st_mode)) {

            printf("FILE %s -> %s\n", source_full_path, destination_full_path);

            if (reverse_file_content(source_full_path, destination_full_path) != SUCCESS) {
                fprintf(stderr, "Error copying file: %s\n", source_full_path);
                continue;
            }

            if (chmod(destination_full_path, file_info.st_mode) != SUCCESS) {
                perror("chmod");
            }

        } else if (S_ISDIR(file_info.st_mode)) {

            printf("DIR  %s -> %s\n", source_full_path, destination_full_path);

            if (process_directory(source_full_path, destination_full_path) != SUCCESS) {
                fprintf(stderr, "Error processing directory: %s\n", source_full_path);
            }
        }
    }

    closedir(directory_stream);
    return SUCCESS;
}

int main(int argc, char *argv[]) {

    const char *source_path;
    char destination_path[MAX_PATH_LEN];
    
    if (argc == 2) {
        // Один аргумент: создаем в текущей директории с перевернутым именем
        source_path = argv[1];
        
        const char *base_name = get_basename(source_path);
        char reversed_base_name[MAX_PATH_LEN];
        reverse_string(base_name, reversed_base_name);
        
        if (getcwd(destination_path, sizeof(destination_path)) == NULL) {
            perror("getcwd");
            return EXIT_FAILURE;
        }
        
        size_t len = strlen(destination_path);
        if (destination_path[len - 1] != PATH_SEPARATOR) {
            strcat(destination_path, "/");
        }
        strcat(destination_path, reversed_base_name);
        
    } else if (argc == 3) {
        source_path = argv[1];
        const char *dest_parent = argv[2];
        
        // Получаем перевернутое имя исходного каталога
        const char *base_name = get_basename(source_path);
        char reversed_base_name[MAX_PATH_LEN];
        reverse_string(base_name, reversed_base_name);
        
        // Формируем полный путь: dest_parent + "/" + reversed_base_name
        strncpy(destination_path, dest_parent, sizeof(destination_path) - 1);
        destination_path[sizeof(destination_path) - 1] = '\0';
        
        // Добавляем разделитель если нужно
        size_t len = strlen(destination_path);
        if (len > 0 && destination_path[len - 1] != PATH_SEPARATOR) {
            if (len + 1 < sizeof(destination_path)) {
                strcat(destination_path, "/");
            } else {
                fprintf(stderr, "Path too long\n");
                return EXIT_FAILURE;
            }
        }
        
        // Добавляем перевернутое имя
        if (strlen(destination_path) + strlen(reversed_base_name) + 1 < sizeof(destination_path)) {
            strcat(destination_path, reversed_base_name);
        } else {
            fprintf(stderr, "Path too long\n");
            return EXIT_FAILURE;
        }
        
    } else {
        fprintf(stderr, "Usage: %s <source_dir> [destination_parent]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Проверяем существование исходного каталога
    struct stat st;
    if (stat(source_path, &st) != 0) {
        fprintf(stderr, "Error: Cannot access '%s': %s\n", source_path, strerror(errno));
        return EXIT_FAILURE;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: '%s' is not a directory\n", source_path);
        return EXIT_FAILURE;
    }
    
    printf("Source: %s\n", source_path);
    printf("Destination: %s\n", destination_path);
    printf("---\n");
    
    if (process_directory(source_path, destination_path) != SUCCESS) {
        return EXIT_FAILURE;
    }
    
    printf("Done\n");
    return EXIT_SUCCESS;
}