#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/utsname.h>

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define CYAN "\033[38;5;51m"
#define PINK "\033[38;5;213m"
#define WHITE "\033[97m"

volatile sig_atomic_t running = 1;

void stop_animation(int sig) {
    (void)sig;
    running = 0;
}

void sleep_frame(void) {
    struct timespec t = {0, 80000000};
    nanosleep(&t, NULL);
}

void get_memory(char *buffer, size_t size) {
    FILE *f = fopen("/proc/meminfo", "r");

    if (!f) {
        snprintf(buffer, size, "Unavailable");
        return;
    }

    unsigned long total = 0, available = 0;
    char line[256];

    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "MemTotal: %lu kB", &total);

        if (sscanf(line, "MemAvailable: %lu kB", &available) == 1)
            continue;
    }

    fclose(f);

    unsigned long used = total > available ? total - available : 0;

    snprintf(buffer, size, "%lu MB / %lu MB",
             used / 1024, total / 1024);
}

void get_uptime(char *buffer, size_t size) {
    FILE *f = fopen("/proc/uptime", "r");

    if (!f) {
        snprintf(buffer, size, "Unknown");
        return;
    }

    double seconds = 0;
    fscanf(f, "%lf", &seconds);
    fclose(f);

    int hours = (int)seconds / 3600;
    int minutes = ((int)seconds % 3600) / 60;

    snprintf(buffer, size, "%dh %dm", hours, minutes);
}

void get_os(char *buffer, size_t size) {
    snprintf(buffer, size, "Linux");

    FILE *f = fopen("/etc/os-release", "r");
    if (!f) return;

    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *p = line + 12;
            p[strcspn(p, "\n")] = 0;

            if (*p == '"' || *p == '\'') {
                memmove(p, p + 1, strlen(p));
                p[strcspn(p, "\"'")] = 0;
            }

            snprintf(buffer, size, "%s", p);
            break;
        }
    }

    fclose(f);
}

int main(void) {
    signal(SIGINT, stop_animation);
    signal(SIGTERM, stop_animation);

    struct utsname sys;
    uname(&sys);

    char os[256];
    char memory[128];
    char uptime[128];

    const char *frames[] = {
        "          .--.          ",
        "         |o_o |         ",
        "         |:_/ |         ",
        "        //   \\ \\        ",
        "       (|     | )       ",
        "      /'\\_   _/`\\      ",
        "      \\___)=(___/      "
    };

    const char *spinner[] = {
        "⠋", "⠙", "⠹", "⠸",
        "⠼", "⠴", "⠦", "⠧",
        "⠇", "⠏"
    };

    int frame = 0;

    printf("\033[?25l");

    while (running) {
        get_os(os, sizeof(os));
        get_memory(memory, sizeof(memory));
        get_uptime(uptime, sizeof(uptime));

        printf("\033[2J\033[H");

        int bounce = (frame / 3) % 2;

        printf("\n");
        printf("  %s%sANIMATED NEOFETCH%s\n",
               BOLD, CYAN, RESET);

        printf("  %s%s%s  Live Linux System Monitor\n\n",
               PINK, spinner[frame % 10], RESET);

        for (int i = 0; i < 7; i++) {
            if (i == 0 || i == 6)
                printf("  ");

            else if (bounce)
                printf(" ");

            printf("%s%s%s\n", CYAN, frames[i], RESET);
        }

        printf("\n");
        printf("  %s────────────────────────────────────%s\n",
               PINK, RESET);

        printf("  %sOS       %s%s\n", CYAN, WHITE, os);
        printf("  %sKernel   %s%s %s\n",
               CYAN, WHITE, sys.sysname, sys.release);

        printf("  %sMachine  %s%s\n",
               CYAN, WHITE, sys.machine);

        printf("  %sUptime   %s%s\n",
               CYAN, WHITE, uptime);

        printf("  %sMemory   %s%s\n",
               CYAN, WHITE, memory);

        printf("  %s\n", RESET);

        int progress = frame % 31;

        printf("  %sSYSTEM ANIMATION %s\n",
               PINK, RESET);

        printf("  [");

        for (int i = 0; i < 30; i++) {
            if (i == progress)
                printf("%s>%s", CYAN, RESET);
            else
                printf("%s", i < progress ? "=" : ".");
        }

        printf("] %02d%%\n", progress * 100 / 30);

        printf("\n  %sFrame: %d | Press CTRL+C to exit%s\n",
               CYAN, frame, RESET);

        fflush(stdout);

        frame++;
        sleep_frame();
    }

    printf("\033[2J\033[H");
    printf("\033[?25h");
    printf("%sAnimated Neofetch stopped.%s\n", CYAN, RESET);

    return 0;
}
