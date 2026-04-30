#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>

// ============================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================

void print_pid(void) {
    printf("PID процесса: %d\n", getpid());
}

void wait_seconds(int seconds, const char *message) {
    printf("\n Ждем %d секунд: %s\n", seconds, message);
    for (int i = seconds; i > 0; i--) {
        printf("  %d...\n", i);
        sleep(1);
    }
}

void print_maps_header(const char *stage) {
    printf("\n┌─────────────────────────────────────────────────────────────┐\n");
    printf("│ СТАДИЯ: %-45s │\n", stage);
    printf("└─────────────────────────────────────────────────────────────┘\n");
    printf("Команда для просмотра: cat /proc/%d/maps\n", getpid());
}

// 3.3 РЕКУРСИВНАЯ ФУНКЦИЯ ДЛЯ ЗАПОЛНЕНИЯ СТЕКА
void recursive_stack(int depth, int max_depth) {
    char buffer[4096*100]; 
    memset(buffer, 'A' + (depth % 26), sizeof(buffer));
    
    if (depth < max_depth) {
        printf("  Рекурсия: глубина %d, адрес буфера = %p\n", 
               depth, (void*)buffer);
        recursive_stack(depth + 1, max_depth);
    } else {
        printf("  Достигнута максимальная глубина %d\n", max_depth);
        printf("  Адрес последнего буфера: %p\n", (void*)buffer);
    }
}

void demonstrate_stack_growth(void) {
    print_maps_header("3.3 РЕКУРСИВНЫЙ РОСТ СТЕКА");
    recursive_stack(0, 10);
}


// 3.5-3.6 ВЫДЕЛЕНИЕ ПАМЯТИ НА КУЧЕ В ЦИКЛЕ
void demonstrate_heap_growth(void) {
    print_maps_header("3.5-3.6 ВЫДЕЛЕНИЕ ПАМЯТИ НА КУЧЕ (HEAP)");
  
    void *pointers[10];
    int sizes[] = {1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    
    for (int i = 0; i < 10; i++) {
        pointers[i] = malloc(sizes[i]);
        if (pointers[i]) {
            printf("  Итерация %d: malloc(%d) -> %p\n", 
                   i+1, sizes[i], pointers[i]);
        } else {
            printf("  Итерация %d: malloc(%d) - ОШИБКА!\n", i+1, sizes[i]);
        }
        
        // Записываем данные, чтобы страницы реально выделились
        if (pointers[i]) {
            memset(pointers[i], i, sizes[i]);
        }
        
        wait_seconds(2, "Смотрим на рост heap в /proc/pid/maps");
    }
    
    printf("\n  heap растет вверх (адреса увеличиваются):\n");
    for (int i = 0; i < 10 && pointers[i]; i++) {
        printf("    %p <- блок %d\n", pointers[i], i+1);
    }
    
    // 3.7 ОСВОБОЖДАЕМ ПАМЯТЬ
    print_maps_header("3.7 ОСВОБОЖДЕНИЕ ПАМЯТИ");
    printf("Освобождаем память в обратном порядке...\n");
    
    for (int i = 9; i >= 0; i--) {
        if (pointers[i]) {
            free(pointers[i]);
            printf("  free(%p)\n", pointers[i]);
        }
    }
    printf("Память возвращена в кучу, но адресное пространство может не уменьшиться\n");
}

// ============================================
// 3.8 MMAP (добавление региона)
// ============================================

void demonstrate_mmap_anonymous(void) {
    print_maps_header("3.8 MMAP ANONYMOUS (10 страниц)");
    
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t region_size = 10 * page_size;  // 10 страниц
    printf("Размер страницы: %zu байт\n", page_size);
    printf("Размер региона: %zu байт (10 страниц)\n", region_size);
    
    // MAP_ANONYMOUS - память не связана с файлом
    // MAP_PRIVATE - изменения приватны
    void *region = mmap(NULL, region_size, 
                        PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE,
                        -1, 0);
    
    if (region == MAP_FAILED) {
        perror("mmap");
        return;
    }
    
    printf("Создан регион: %p - %p (размер %zu)\n", 
           region, (char*)region + region_size, region_size);
    
    wait_seconds(5, "Наблюдаем за новым регионом в /proc/pid/maps");
    
    // ========================================
    // 3.10 ИЗМЕНЕНИЕ ПРАВ ДОСТУПА
    // ========================================
    
    print_maps_header("3.10 ИЗМЕНЕНИЕ ПРАВ ДОСТУПА");
    
    // 3.10.1 ЗАПРЕТ НА ЧТЕНИЕ
    printf("\n--- 3.10.1 Запрещаем чтение ---\n");
    printf("Пытаемся прочитать данные...\n");
    
    if (mprotect(region, region_size, PROT_WRITE) == -1) {
        perror("mprotect");
    } else {
        printf("Установлены права: только запись (PROT_WRITE)\n");
        printf("Пытаемся прочитать: ");
        fflush(stdout);
        
        // Это вызовет SEGFAULT!
        // volatile char c = *(volatile char*)region;
        printf("(закомментировано, чтобы не падать)\n");
        printf("  ⚠️  При чтении возник бы Segmentation Fault\n");
    }
    
    // 3.10.2 ЗАПРЕТ НА ЗАПИСЬ
    printf("\n--- 3.10.2 Запрещаем запись ---\n");
    printf("Устанавливаем права: только чтение (PROT_READ)\n");
    
    if (mprotect(region, region_size, PROT_READ) == -1) {
        perror("mprotect");
    } else {
        printf("Пытаемся записать: ");
        fflush(stdout);
        // Это вызовет SEGFAULT!
        // *(char*)region = 'X';
        printf("(закомментировано, чтобы не падать)\n");
        printf("  ⚠️  При записи возник бы Segmentation Fault\n");
    }
    
    wait_seconds(3, "Наблюдаем за изменением прав в /proc/pid/maps");
    
    // ========================================
    // 3.10.4 ОТСОЕДИНЕНИЕ СТРАНИЦ 4-6
    // ========================================
    
    print_maps_header("3.10.4 ОТСОЕДИНЕНИЕ СТРАНИЦ 4-6");
    
    // Страницы с 4 по 6 (нумерация с 0)
    // Адрес 4-й страницы = region + 4 * page_size
    // Размер = 3 страницы (4,5,6)
    void *unmap_start = (char*)region + 4 * page_size;
    size_t unmap_size = 3 * page_size;
    
    printf("Отсоединяем страницы 4, 5, 6:\n");
    printf("  Начало: %p\n", unmap_start);
    printf("  Размер: %zu байт (3 страницы)\n", unmap_size);
    
    if (munmap(unmap_start, unmap_size) == -1) {
        perror("munmap");
    } else {
        printf("Страницы успешно отсоединены\n");
        printf("В /proc/pid/maps появится разрыв в адресном пространстве\n");
    }
    
    wait_seconds(5, "Наблюдаем за разрывом в адресном пространстве");
    
    // Освобождаем оставшиеся страницы
    // Первые 4 страницы (0-3)
    void *first_part = region;
    size_t first_size = 4 * page_size;
    
    // Последние страницы (7-9) — теперь они сместились, так как мы удалили 4-6
    void *second_part = (char*)region + 7 * page_size;
    size_t second_size = 3 * page_size;
    
    printf("\nОчищаем оставшиеся регионы...\n");
    munmap(first_part, first_size);
    munmap(second_part, second_size);
    printf("  munmap(%p, %zu)\n", first_part, first_size);
    printf("  munmap(%p, %zu)\n", second_part, second_size);
}


int main(void) {
 
    // 3.1-3.2: Выводим PID и ждем 10 секунд для мониторинга
    printf("\n=== 3.1-3.2 ПОДГОТОВКА К МОНИТОРИНГУ ===\n");
    print_pid();
    wait_seconds(17, "Настройте мониторинг в другом терминале");
    
    // 3.3-3.4: Рекурсивный рост стека
    demonstrate_stack_growth();
    wait_seconds(5, "Наблюдаем за изменением стека");
    
    // 3.5-3.7: Выделение памяти на куче в цикле
   // demonstrate_heap_growth();
    wait_seconds(5, "Наблюдаем за изменением heap");
    
    // 3.8-3.10: mmap и работа с регионами
   // demonstrate_mmap_anonymous();
    

    return 0;
}