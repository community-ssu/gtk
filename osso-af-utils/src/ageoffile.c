#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
        struct stat buf;
        if (argc != 2) {
                printf("Usage: %s <file>\n", argv[0]);
                return 1;
        }
        if (lstat(argv[1], &buf) < 0) {
                perror("lstat");
                return 1;
        }
        printf("%u\n", buf.st_mtime);
        return 0;
}
