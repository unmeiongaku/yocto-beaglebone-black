#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <poll.h>

#define DEVICE "/dev/m_cdev"
#define BUFFER_SIZE 256

#define M_IOCTL_CLEAR_BUFFER _IO('m', 0)
#define M_IOCTL_GET_SIZE     _IOR('m', 1, int)


int fd;

/* ================= WRITE ================= */

void test_write()
{
    char buf[BUFFER_SIZE];

    printf("Enter text: ");
    fgets(buf, BUFFER_SIZE, stdin);

    write(fd, buf, strlen(buf));

    printf("Write done\n");
}

/* ================= READ ================= */

void test_read()
{
    char buf[BUFFER_SIZE];

    int ret = read(fd, buf, BUFFER_SIZE-1);

    if (ret > 0)
    {
        buf[ret] = '\0';
        printf("Driver says: %s\n", buf);
    }
    else
    {
        printf("No data\n");
    }
}

/* ================= IOCTL ================= */

void test_ioctl_get_size()
{
    int size;

    ioctl(fd, M_IOCTL_GET_SIZE, &size);

    printf("Driver buffer size: %d\n", size);
}

void test_ioctl_clear()
{
    ioctl(fd, M_IOCTL_CLEAR_BUFFER);

    printf("Buffer cleared\n");
}

/* ================= POLL ================= */

void test_poll()
{
    struct pollfd pfd;

    pfd.fd = fd;
    pfd.events = POLLIN;

    printf("Waiting for data...\n");

    int ret = poll(&pfd, 1, 5000);

    if (ret > 0)
        printf("Driver has data!\n");
    else
        printf("Timeout\n");
}

/* ================= STRESS THREAD ================= */

void *stress_thread(void *arg)
{
    int id = *(int*)arg;

    char msg[64];

    for(int i = 0; i < 10; i++)
    {
        sprintf(msg, "thread %d message %d\n", id, i);

        write(fd, msg, strlen(msg));

        usleep(100000);
    }

    return NULL;
}

void test_stress()
{
    pthread_t t1, t2;

    int id1 = 1;
    int id2 = 2;

    pthread_create(&t1, NULL, stress_thread, &id1);
    pthread_create(&t2, NULL, stress_thread, &id2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Stress test done\n");
}

/* ================= MENU ================= */

void menu()
{
    printf("\n====== DRIVER TEST TOOL ======\n");
    printf("1. Write\n");
    printf("2. Read\n");
    printf("3. IOCTL Get Size\n");
    printf("4. IOCTL Clear Buffer\n");
    printf("5. Poll Test\n");
    printf("6. Stress Test\n");
    printf("0. Exit\n");
}

int main()
{
    fd = open(DEVICE, O_RDWR);

    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    int choice;

    while(1)
    {
        menu();

        printf("Select: ");
        scanf("%d", &choice);
        getchar();

        switch(choice)
        {
            case 1:
                test_write();
                break;

            case 2:
                test_read();
                break;

            case 3:
                test_ioctl_get_size();
                break;

            case 4:
                test_ioctl_clear();
                break;

            case 5:
                test_poll();
                break;

            case 6:
                test_stress();
                break;

            case 0:
                close(fd);
                return 0;

            default:
                printf("Invalid\n");
        }
    }

    return 0;
}
