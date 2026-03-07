#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h> 
#include <pthread.h>
#include <string.h>

#define LOG     0x01
#define log(...) {  \
    do {    \
        if ((LOG)) { \
            printf("\n[%s:%s:%d] ==>> ", __FILE__, __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
        } \
    } while (0); \
} // Logging


void run_cmd(char *cmd[])
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(cmd[0], cmd);
        perror("execvp failed");
        exit(1);
    }
    else
    {
        wait(NULL);
    }
}

int main(int argc, char const *argv[]) {

    if (argc != 3) {
        log("Usage: ./wifi-connect SSID PASS");
        exit(EXIT_FAILURE);
    }

    const char *ssid = argv[1];
    const char *pass = argv[2];

    char cmd_buf[256];

    snprintf(cmd_buf, sizeof(cmd_buf),
             "wpa_passphrase \"%s\" \"%s\" > /etc/wpa_supplicant.conf",
             ssid, pass);

    char *gen_conf[] = {"sh", "-c", cmd_buf, NULL};

    char *iface_up[] = {"ip", "link", "set", "wlan0", "up", NULL};

    char *wpa_run[] = {
        "wpa_supplicant",
        "-B",
        "-i", "wlan0",
        "-c", "/etc/wpa_supplicant.conf",
        NULL};

    char *dhcp_run[] = {"dhcpcd", "wlan0", NULL};

    printf("Starting WiFi connection...\n");

    run_cmd(iface_up);
    run_cmd(gen_conf);
    run_cmd(wpa_run);
    //run_cmd(dhcp_run);

    printf("WiFi connection process finished\n");

    return 0;
}

