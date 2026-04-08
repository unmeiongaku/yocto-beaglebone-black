#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

// ===== PATH =====
#define BASE "/sys/bus/iio/devices/iio:device0/"

#define ACC_X BASE "in_accel_x_raw"
#define ACC_Y BASE "in_accel_y_raw"
#define ACC_Z BASE "in_accel_z_raw"

#define GYR_X BASE "in_anglvel_x_raw"
#define GYR_Y BASE "in_anglvel_y_raw"
#define GYR_Z BASE "in_anglvel_z_raw"

#define MAG_X BASE "in_magn_x_raw"
#define MAG_Y BASE "in_magn_y_raw"
#define MAG_Z BASE "in_magn_z_raw"

#define YAW   BASE "in_rot_yaw_raw"
#define PITCH BASE "in_rot_pitch_raw"
#define ROLL  BASE "in_rot_roll_raw"

#define TEMP  BASE "in_temp_input"
#define CALIB BASE "bno055_calibration_status"

/* ================= MENU ================= */
void menu()
{
    printf("\033[2J\033[H"); // clear screen

    printf("====== BNO055 TEST TOOL ======\n");
    printf("[SENSOR]\n");
    printf(" 1. Acceleration\n");
    printf(" 2. Gyroscope\n");
    printf(" 3. Magnetometer\n");
    printf(" 4. Quaternion\n");
    printf(" 5. Euler\n");
    printf(" 6. Linear Accel\n");
    printf(" 7. Gravity\n");
    printf(" 8. Temperature\n");

    printf("\n[SYSTEM]\n");
    printf(" 9.  Read ALL (stream)\n");
    printf("10. Calibration Offset\n");
    printf("11. Calibration Status\n");

    printf("\n[CONTROL]\n");
    printf("12. Get Mode\n");
    printf("13. Set Mode\n");
    printf("14. Chip ID\n");

    printf("\n 0. Exit\n");
    printf("Select: ");
}

// ===== READ SYSFS =====
int read_val(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buf[32];
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return atoi(buf);
}


// ===== NON-BLOCKING KEY =====
void set_nonblocking(int enable)
{
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);

    if (enable) {
        ttystate.c_lflag &= ~ICANON;
        ttystate.c_lflag &= ~ECHO;
    } else {
        ttystate.c_lflag |= ICANON;
        ttystate.c_lflag |= ECHO;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (enable)
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    else
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

// ===== STREAM ALL =====
void stream_all()
{
    printf("Press 'q' to quit...\n");
    set_nonblocking(1);

    while (1) {
        int ax = read_val(ACC_X);
        int ay = read_val(ACC_Y);
        int az = read_val(ACC_Z);

        int gx = read_val(GYR_X);
        int gy = read_val(GYR_Y);
        int gz = read_val(GYR_Z);

        int mx = read_val(MAG_X);
        int my = read_val(MAG_Y);
        int mz = read_val(MAG_Z);

        int yaw = read_val(YAW);
        int pitch = read_val(PITCH);
        int roll = read_val(ROLL);

        int temp = read_val(TEMP);
        int calib = read_val(CALIB);

        printf("\033[2K\r");
        printf("ACC[%4d %4d %4d] | ", ax, ay, az);
        printf("GYR[%4d %4d %4d] | ", gx, gy, gz);
        printf("MAG[%4d %4d %4d] | ", mx, my, mz);
        printf("YPR[%4d %4d %4d] | ", yaw, pitch, roll);
        printf("T:%2d | CAL:%d", temp, calib);

        fflush(stdout);

        char c = getchar();
        if (c == 'q') break;

        usleep(100000); // 100ms
    }

    set_nonblocking(0);
    printf("\nExit stream.\n");
}

// ===== READ ONCE =====
void read_acc()
{
    printf("ACC: %d %d %d\n",
           read_val(ACC_X),
           read_val(ACC_Y),
           read_val(ACC_Z));
}

int main()
{   
    int choice;
    while (1) {
        menu();
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            read_acc();
            break;

        case 2:
            stream_all();
            break;

        case 0:
            printf("Bye!\n");
            return 0;

        default:
            printf("Invalid!\n");
        }
    }
    return 0;
}