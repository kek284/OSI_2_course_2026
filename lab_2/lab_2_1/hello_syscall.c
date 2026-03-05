#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h> 


#define SYS_WRITE_ERROR -1

ssize_t sys_write(int fd, const void *buf, size_t count) {
    long ret = syscall(SYS_write, fd, buf, count);
    if (ret == SYS_WRITE_ERROR) {   
        return SYS_WRITE_ERROR;
    }
    return ret;
}

int main() {
    int fd = STDOUT_FILENO; 
    const char *message = "hello world\n";
    size_t len = strlen(message);

    sys_write(fd, message, len);

    return 0;
}