#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;
    char buf[128];

    fd = open("/dev/m_cdev", O_RDWR);
    if (fd < 0)
    {
        perror("Cannot open device");
        return -1;
    }
    
    write(fd, "Hello Kernel", 12);

    int ret = read(fd, buf, sizeof(buf));
    if (ret > 0)
    {
        buf[ret] = '\0';
        printf("Data from driver: %s\n", buf);
    }

    close(fd);
    return 0;
}