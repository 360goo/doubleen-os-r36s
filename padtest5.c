/*
 * padtest.c - the first program of MyOS.
 *
 * It shows a welcome screen, then prints the name of every button you press.
 * Hold SELECT + START together to switch the handheld off.
 *
 * What it teaches:
 *   - In Linux, every input device (gamepad, keyboard) is a file in /dev/input.
 *   - Reading that file gives you "events": which button, pressed or released.
 *
 * Build:  gcc -static -O2 -o padtest padtest.c
 */
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_DEVICES 16

/* Give the most common gamepad button codes a readable name. */
static const char *button_name(int code)
{
    switch (code) {
    case BTN_SOUTH:      return "B (bottom)";
    case BTN_EAST:       return "A (right)";
    case BTN_NORTH:      return "X (top)";
    case BTN_WEST:       return "Y (left)";
    case BTN_TL:         return "L1";
    case BTN_TR:         return "R1";
    case BTN_TL2:        return "L2";
    case BTN_TR2:        return "R2";
    case BTN_SELECT:     return "SELECT";
    case BTN_START:      return "START";
    case BTN_MODE:       return "FN / HOME";
    case BTN_THUMBL:     return "L3";
    case BTN_THUMBR:     return "R3";
    case BTN_DPAD_UP:    return "D-pad UP";
    case BTN_DPAD_DOWN:  return "D-pad DOWN";
    case BTN_DPAD_LEFT:  return "D-pad LEFT";
    case BTN_DPAD_RIGHT: return "D-pad RIGHT";
    case KEY_VOLUMEUP:   return "VOLUME +";
    case KEY_VOLUMEDOWN: return "VOLUME -";
    case KEY_POWER:      return "POWER";
    default:             return NULL;
    }
}

int main(void)
{
    struct pollfd fds[MAX_DEVICES];
    char names[MAX_DEVICES][128];
    int count = 0;
    int select_down = 0, start_down = 0;

    /* 1. Open every input device in /dev/input (event0, event1, ...). */
 printf("\033c");
    printf("=====================================\n");
    printf("     Welcome to Doubleen Os 0.1\n");
    printf("=====================================\n\n");
    printf("Made by Heyan Ahlawat\n");
    DIR *dir = opendir("/dev/input");
    if (!dir) {
        perror("/dev/input");
        return 1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_DEVICES) {
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;
        char path[300];
        snprintf(path, sizeof path, "/dev/input/%s", entry->d_name);
        int fd = open(path, O_RDONLY);
        if (fd < 0)
            continue;
        ioctl(fd, EVIOCGNAME(sizeof names[count]), names[count]);
        fds[count].fd = fd;
        fds[count].events = POLLIN;
        count++;
    }
    closedir(dir);

    /* 2. Welcome screen. "\033c" clears the screen. */
    
    
    printf("Input devices found: %d\n", count);
    for (int i = 0; i < count; i++)
        printf("  - %s\n", names[i]);
    printf("\nPress any button.\n");
    printf("Hold SELECT + START to switch off.\n\n");
    fflush(stdout);

    /* 3. Main loop: wait until a device has an event, then read it. */
    for (;;) {
        if (poll(fds, count, -1) <= 0)
            continue;
        for (int i = 0; i < count; i++) {
            if (!(fds[i].revents & POLLIN))
                continue;
            struct input_event ev;
            if (read(fds[i].fd, &ev, sizeof ev) != sizeof ev)
                continue;

            if (ev.type == EV_KEY) {
                /* value: 1 = pressed, 0 = released, 2 = held (auto-repeat) */
                if (ev.value == 2)
                    continue;
                const char *name = button_name(ev.code);
                if (name)
                    printf("%-12s %s\n", name, ev.value ? "pressed" : "released");
                else
                    printf("button code %d %s\n", ev.code, ev.value ? "pressed" : "released");

                if (ev.code == BTN_SELECT) select_down = ev.value;
                if (ev.code == BTN_START)  start_down = ev.value;
                if (select_down && start_down) {
                    printf("\nSwitching off. Bye!\n");
                    fflush(stdout);
                    sync();
                    if (system("/bin/poweroff") != 0)
                        printf("poweroff failed\n");
                    return 0;
                }
            } else if (ev.type == EV_ABS && ev.value != 0) {
                /* Analog sticks and some D-pads send "absolute" positions. */
                if (ev.code == ABS_HAT0X)
                    printf("D-pad %s\n", ev.value < 0 ? "LEFT" : "RIGHT");
                else if (ev.code == ABS_HAT0Y)
                    printf("D-pad %s\n", ev.value < 0 ? "UP" : "DOWN");
            }
            fflush(stdout);
        }
    }
}
