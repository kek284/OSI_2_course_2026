#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int g_init = 2006;       
int g_uninit;          
const int g_const = 1000; 

void print_addr(const char *name, const char *section, void *addr) {
    printf("  %-30s [%-6s] %p\n", name, section, addr);
}

void print_warning(const char *msg) {
    printf("\n  !!! %s !!!\n", msg);
}

void print_separator(const char *title) {
    printf("\n=== %s ===\n", title);
}

void show_addrs(void) {
    int local = 67;
    static int static_var;
    const int local_const = 100;
    
    print_separator("АДРЕСА ПЕРЕМЕННЫХ");
    print_addr("локальная", "stack", &local);
    print_addr("статическая", "bss", &static_var);
    print_addr("локальная константа", "stack", (void*)&local_const);
    print_addr("глобальная init", "data", &g_init);
    print_addr("глобальная uninit", "bss", &g_uninit);
    print_addr("глобальная const", "rodata", (void*)&g_const);
}

void show_maps(void) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", getpid());
    
    print_separator("Информация");
    printf("  PID: %d\n", getpid());
    printf("  cat %s\n\n", path);
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    char line[200];
    for (int i = 0; i < 12 && fgets(line, sizeof(line), f); i++)
        printf("  %s", line);
    fclose(f);
    
    printf("\n  r-xp = .text    r--p = .rodata    rw-p = .data/.bss/heap/stack\n");
}

int* get_local_addr(void) {
    int x = 123;
    return &x;  // ОШИБКА
}

void show_dangling(void) {
    print_separator("ОПАСНЫЙ ВОЗВРАТ АДРЕСА");
    int *p = get_local_addr();
    printf("  Адрес: %p, значение: %d\n", (void*)p, *p);
    print_warning("Локальная переменная уже не существует");
}

void show_heap(void) {
    print_separator("РАБОТА С КУЧЕЙ");
    
    char *buffer1 = malloc(100);
    if (!buffer1) return;
    strcpy(buffer1, "Hello");
    printf("  buffer1 = %p -> '%s'\n", (void*)buffer1, buffer1);
    free(buffer1);
    printf("  after free(buffer1)\n");
    printf("  buffer1 = %p -> '%s'\n", (void*)buffer1, buffer1);
    char *buffer2 = malloc(100);
    if (!buffer2) return;
    strcpy(buffer2, "World");
    printf("  buffer2 = %p -> '%s'\n", (void*)buffer2, buffer2);
    
    if (buffer1 == buffer2) printf("  Адреса равны, память переиспользована\n");
    char *mid = buffer2 + 50;
    //free(mid);
    free(buffer2);
}

void show_env(void) {
    const char *name = "MY_VAR";
    print_separator("ПЕРЕМЕННЫЕ ОКРУЖЕНИЯ");
    
    char *val = getenv(name);
    if (!val) {
        printf("  Ошибка: export %s=value\n", name);
        return;
    }
    
    printf("  До:   %s = '%s'\n", name, val);
    setenv(name, "new_value", 1);
    printf("  После: %s = '%s'\n", name, getenv(name));
    print_warning("Изменения только для текущего процесса");

    char *new_value = getenv(name);
    printf(" new: %s = '%s'\n", name, new_value);

}

int main(void) {
  
    show_addrs();
    show_maps();
    //show_dangling();
    show_heap();
    show_env();

    printf("\n========== Нажмите Enter ==========\n");
    getchar();
    return 0;
}