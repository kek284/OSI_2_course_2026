#include <unistd.h>
#include <string.h>  

int main() {
    const char *str = "hello world\n";     
    size_t len = strlen(str);       
    write(STDOUT_FILENO, str, len);          
    return 0;
}
