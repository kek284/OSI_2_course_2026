#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define ERROR -1


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>


void print_pid(void) {
    printf("PID процесса: %d\n", getpid());
}

void wait_seconds(int seconds, const char *message) {
    printf("\nЖдем %d секунд: %s\n", seconds, message);
    for (int i = seconds; i > 0; i--) {
        printf("  %d...\n", i);
        sleep(1);
    }
}

void recursive_stack(int depth, int max_depth) {
    char buffer[4096*100];  
    
    memset(buffer, 'A' + (depth % 26), sizeof(buffer));
    
    printf("Глубина %d: адрес буфера = %p\n", depth, (void*)buffer);
    
    if (depth == 0 || depth == max_depth / 2 || depth == max_depth) {
        printf("  Смотрим /proc/self/maps на глубине %d:\n", depth);
        fflush(stdout);
        
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps | grep -E 'stack|heap'", getpid());
        system(cmd);
        printf("\n");
    }
    
    if (depth < max_depth) {
        sleep(2);  
        recursive_stack(depth + 1, max_depth);
    }
    
    if (depth > 0) {
        sleep(1);
    }
}

void demonstrate_stack_growth(void) {
    recursive_stack(0, 10);
}


void demonstrate_heap_growth(void) {   
    void *pointers[10];
    int sizes[] = {1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    //int sizes[] = {131072, 131072, 131072, 131072, 131072, 131072, 131072, 131072, 131072, 131072};
    for (int i = 0; i < 10; i++) {
        pointers[i] = malloc(sizes[i]);
        if (pointers[i]) {
            printf("  Итерация %d: malloc(%d) -> %p\n", i + 1, sizes[i], pointers[i]);
            memset(pointers[i], i, sizes[i]);

        } else {
            printf("  Итерация %d: malloc(%d) - ОШИБКА!\n", i+1, sizes[i]);
        }
        wait_seconds(2, "рост heap");
    }
    
    for (int i = 0; i < 10 && pointers[i]; i++) {
        printf("    %p <- блок %d\n", pointers[i], i + 1);
    }
       
    for (int i = 9; i >= 0; i--) {
        if (pointers[i]) {
            free(pointers[i]);
            printf("  free(%p)\n", pointers[i]);
        }
    }
}

void demonstrate_mmap_anonymous(void) {  
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t region_size = 50 * page_size; 
    printf("Размер страницы: %zu байт\n", page_size);
    printf("Размер региона: %zu байт \n", region_size);
    
    void *region = mmap(NULL, region_size, PROT_READ | PROT_WRITE,MAP_ANONYMOUS | MAP_PRIVATE,-1, 0);
    
    if (region == MAP_FAILED) {
        perror("mmap");
        return;
    }
    
    printf("Создан регион: %p - %p (размер %zu)\n", 
           region, (char*)region + region_size, region_size);
    
   // wait_seconds(10, "новый регион");
    getchar();


    // if (mprotect(region, region_size, PROT_WRITE) == ERROR) {
    //     perror("mprotect");
    // } else {
    //     printf("Установлены права: только запись (PROT_WRITE)\n");
    //     printf("Пытаемся прочитать: ");
    //     fflush(stdout);
        
    //     // Это вызовет SEGFAULT
    //    // volatile char c = *(volatile char*)region;
    // }
    
    // 3.10.2 ЗАПРЕТ НА ЗАПИСЬ
    // printf("\n--- 3.10.2 Запрещаем запись ---\n");
    // printf("Устанавливаем права: только чтение (PROT_READ)\n");
    
    // if (mprotect(region, region_size, PROT_READ) == ERROR) {
    //     perror("mprotect");
    // } else {
    //     printf("Пытаемся записать: ");
    //     fflush(stdout);
    //     wait_seconds(3, "изменение прав");

    //     // Это вызовет SEGFAULT!
    //     *(char*)region = 'X';
    // }
    
    // wait_seconds(3, "изменение прав");
    
        
//    // print_maps_header("3.10.4 ОТСОЕДИНЕНИЕ СТРАНИЦ 4-6");
   
    void *unmap_start = (char*)region + 4 * page_size;
    size_t unmap_size = 3 * page_size;
    
    printf("Отсоединяем страницы 4, 5, 6:\n");
    printf("  Начало: %p\n", unmap_start);
    printf("  Размер: %zu байт (3 страницы)\n", unmap_size);
    
    if (munmap(unmap_start, unmap_size) == ERROR) {
        perror("munmap");
    } 
    
    wait_seconds(5, "разрыв");
    
   
    void *first_part = region;
    size_t first_size = 4 * page_size;
    
    void *second_part = (char*)region + 7 * page_size;
    size_t second_size = 3 * page_size;
    getchar();

    printf("\nОчищаем оставшиеся регионы...\n");
    munmap(first_part, first_size);
    munmap(second_part, second_size);
    printf("  munmap(%p, %zu)\n", first_part, first_size);
    printf("  munmap(%p, %zu)\n", second_part, second_size);
    getchar();


}


int main(void) {
    print_pid();
    wait_seconds(18, "");
    
    //demonstrate_stack_growth();
    //wait_seconds(5, "изменение стека");
    
    //demonstrate_heap_growth();
    //wait_seconds(5, "изменение heap");
    
    demonstrate_mmap_anonymous();
      return 0;
}