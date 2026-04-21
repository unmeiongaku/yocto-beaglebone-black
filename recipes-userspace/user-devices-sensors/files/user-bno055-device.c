#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// ===== PATH =====
#define BASE "/sys/bus/iio/devices/iio:device0/"

#define ACC_X BASE "in_accel_x_raw"
#define ACC_Y BASE "in_accel_y_raw"
#define ACC_Z BASE "in_accel_z_raw"

#define ACC_SCALE BASE "in_accel_scale"

#define GYR_X BASE "in_anglvel_x_raw"
#define GYR_Y BASE "in_anglvel_y_raw"
#define GYR_Z BASE "in_anglvel_z_raw"

#define GYR_SCALE BASE "in_anglvel_scale"

#define MAGN_X BASE "in_magn_x_raw"
#define MAGN_Y BASE "in_magn_y_raw"
#define MAGN_Z BASE "in_magn_z_raw"

#define MAGN_SCALE BASE "in_magn_scale"

#define YAW   BASE "in_rot_yaw_raw"
#define PITCH BASE "in_rot_pitch_raw"
#define ROLL  BASE "in_rot_roll_raw"

#define EUL_SCALE BASE "in_rot_scale"


#define TEMP  BASE "in_temp_input"
#define CALIB BASE "bno055_calibration_status"

/* ================= MENU ================= */
void menu()
{
    printf("\033[2J\033[H"); // clear screen

    printf("====== BNO055 TEST TOOL ======\n");
    printf("[SENSOR]\n");
    printf(" 1. Acceleration{raw}\n");
    printf(" 2. Acceleration{scale}\n");
    printf(" 3. Gyroscope{raw}\n");
    printf(" 4. Gyroscope{scale}\n");
    printf(" 5. Magnetometer{raw}\n");
    printf(" 6. Magnetometer{scale}\n");
    printf(" 7. Quaternion{raw}\n");
    printf(" 8. Quaternion{scale}\n");
    printf(" 9. Euler{raw}\n");
    printf(" 10. Euler{scale}\n");
    printf(" 11. Linear Accel{raw}\n");
    printf(" 12. Linear Accel{scale}\n");
    printf(" 13. Gravity{raw}\n");
    printf(" 14. Gravity{scale}\n");
    printf(" 15. Temperature\n");

    printf("\n[SYSTEM]\n");
    printf(" 16.  Read ALL (stream)\n");
    printf(" 17. Calibration Offset\n");
    printf(" 18. Calibration Status\n");

    printf("\n[CONTROL]\n");
    printf(" 19. Get Mode\n");
    printf(" 20. Set Mode\n");
    printf(" 21. Chip ID\n");

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

float read_val_float(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1.0f;

    char buf[64];
    fgets(buf, sizeof(buf), f);
    fclose(f);

    return strtof(buf, NULL);
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
    float accscale,gyrscale,magnscale,eulerscale;
    accscale = read_val_float(ACC_SCALE);
    gyrscale = read_val_float(GYR_SCALE);
    magnscale = read_val_float(MAGN_SCALE);
    eulerscale = read_val_float(EUL_SCALE);
    printf("Press 'q' to quit...\n");
    set_nonblocking(1);

    while (1) {
       
        float ax = (float)(read_val_float(ACC_X)*accscale);
        float ay = (float)(read_val_float(ACC_Y)*accscale);
        float az = (float)(read_val_float(ACC_Z)*accscale);

        float gx = (float)(read_val_float(GYR_X)*gyrscale);
        float gy = (float)(read_val_float(GYR_Y)*gyrscale);
        float gz = (float)(read_val_float(GYR_Z)*gyrscale);

        float mx = (float)(read_val_float(MAGN_X)*magnscale);
        float my = (float)(read_val_float(MAGN_Y)*magnscale);
        float mz = (float)(read_val_float(MAGN_Z)*magnscale);

        float yaw = (float)(read_val_float(YAW)*eulerscale);
        float pitch = (float)(read_val_float(PITCH)*eulerscale);
        float roll = (float)(read_val_float(ROLL)*eulerscale);

        int temp = (read_val(TEMP)/1000);
        int calib = read_val(CALIB);

        printf("\033[2K\r");
        printf("ACC[%6.2f %6.2f %6.2f] | ", ax, ay, az);
        printf("GYR[%6.2f %6.2f %6.2f] | ", gx, gy, gz);
        printf("MAG[%7.2f %7.2f %7.2f] | ", mx, my, mz);
        printf("YPR[%7.2f %7.2f %7.2f] | ", yaw, pitch, roll);
        printf("T:%2d | CAL:%d", temp, calib);

        fflush(stdout);

        char c = getchar();
        if (c == 'q') break;

        usleep(50000); // 100ms
    }

    set_nonblocking(0);
    //printf("\nExit stream.\n");
}

// ===== READ ONCE =====
void read_acc(int i)
{
    float xc,yc,zc,scale;
    int x,y,z;
    scale = read_val_float(ACC_SCALE);
    printf("Press 'q' to quit...\n");
    set_nonblocking(1);
    while(1){
        if(i == 1){
            xc = (float)(read_val(ACC_X)*scale);
            yc = (float)(read_val(ACC_Y)*scale);
            zc = (float)(read_val(ACC_Z)*scale);  
            printf("\r\033[KACC: X=%7.2f  Y=%7.2f  Z=%7.2f SCALE=%7.2f", xc, yc, zc,scale);
        }
        else if(i==0){
            x = read_val(ACC_X);
            y = read_val(ACC_Y);
            z = read_val(ACC_Z); 
            printf("\r\033[KACC: X=%d Y=%d Z=%d", x, y, z);
        }
        fflush(stdout);
        char c = getchar();
        if (c == 'q') break;
        usleep(100000); // 100ms
    }
    set_nonblocking(0);
}

int main()
{   
    int choice;
    while (1) {
        menu();
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            read_acc(0);
            break;

        case 2:
            read_acc(1);
            break;
        case 16:
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