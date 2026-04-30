#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *envp[]) {
    printf("PID: %d\n", getpid());
printf("Засыпаю на 1 секунду...\n");
    sleep(1);
    
    printf("Делаю exec себя...\n");
    execl("/proc/self/exe", "program", NULL);
    
    printf("Hello world\n");
    return 0;
}