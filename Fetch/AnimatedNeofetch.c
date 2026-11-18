#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define ORANGE "\033[38;5;208m"
#define CYAN "\033[38;5;45m"
#define WHITE "\033[38;5;255m"
#define MUTED "\033[38;5;244m"

static int animate_output = 0;

static void pause_ms(long milliseconds) {
    if (!animate_output || milliseconds <= 0) return;

    struct timespec delay;
    delay.tv_sec = milliseconds / 1000;
    delay.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&delay, NULL);
}

static void print_animated(const char *text, long delay_ms) {
    for (const char *p = text; *p; ++p) {
        putchar(*p);
        fflush(stdout);
        pause_ms(delay_ms);
    }
}

static void trim(char *text) {
    size_t length = strlen(text);

    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }

    size_t start = 0;

    while (text[start] && isspace((unsigned char)text[start]))
        start++;

    if (start > 0)
        memmove(text, text + start, strlen(text + start) + 1);
}

static void read_os_name(char *output, size_t output_size) {
    snprintf(output, output_size, "Linux");

    FILE *file = fopen("/etc/os-release", "r");
    if (!file) return;

    char line[512];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *value = line + 12;
            trim(value);

            size_t length = strlen(value);

            if (length >= 2 &&
                ((value[0] == '"' && value[length - 1] == '"') ||
                 (value[0] == '\'' && value[length - 1] == '\''))) {
                value[length - 1] = '\0';
                value++;
            }

            snprintf(output, output_size, "%.*s",
                     (int)(output_size - 1), value);
            break;
        }
    }

    fclose(file);
}

static void read_cpu_name(char *output, size_t output_size) {
    snprintf(output, output_size, "Unknown CPU");

    FILE *file = fopen("/proc/cpuinfo", "r");
    if (!file) return;

    char line[512];

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "model name") ||
            strstr(line, "Hardware") ||
            strstr(line, "Processor")) {

            char *colon = strchr(line, ':');

            if (colon) {
                char *value = colon + 1;
                trim(value);

                if (*value)
                    snprintf(output, output_size, "%s", value);

                break;
            }
        }
    }

    fclose(file);
}

static void read_memory(char *output, size_t output_size) {
    unsigned long long total_kb = 0;
    unsigned long long available_kb = 0;

    FILE *file = fopen("/proc/meminfo", "r");

    if (!file) {
        snprintf(output, output_size, "Unavailable");
        return;
    }

    char line[256];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "MemTotal: %llu kB", &total_kb) == 1)
            continue;

        if (sscanf(line, "MemAvailable: %llu kB", &available_kb) == 1)
            continue;
    }

    fclose(file);

    if (total_kb == 0) {
        snprintf(output, output_size, "Unavailable");
        return;
    }

    unsigned long long used_mb =
        (total_kb - (available_kb > total_kb
        ? total_kb : available_kb)) / 1024ULL;

    unsigned long long total_mb = total_kb / 1024ULL;

    snprintf(output, output_size,
             "%llu MiB / %llu MiB", used_mb, total_mb);
}

static void read_uptime(char *output, size_t output_size) {
    FILE *file = fopen("/proc/uptime", "r");

    if (!file) {
        snprintf(output, output_size, "Unavailable");
        return;
    }

    double seconds = 0.0;

    if (fscanf(file, "%lf", &seconds) != 1)
        seconds = 0.0;

    fclose(file);

    unsigned long long total_minutes =
        (unsigned long long)(seconds / 60.0);

    unsigned long long days = total_minutes / 1440ULL;
    unsigned long long hours = (total_minutes % 1440ULL) / 60ULL;
    unsigned long long minutes = total_minutes % 60ULL;

    if (days > 0) {
        snprintf(output, output_size,
                 "%llu days, %llu hours, %llu mins",
                 days, hours, minutes);
    } else if (hours > 0) {
        snprintf(output, output_size,
                 "%llu hours, %llu mins", hours, minutes);
    } else {
        snprintf(output, output_size, "%llu mins", minutes);
    }
}

static const char *shell_name(void) {
    const char *shell = getenv("SHELL");

    if (!shell || !*shell)
        return "Unknown";

    const char *slash = strrchr(shell, '/');

    return slash ? slash + 1 : shell;
}

int main(void) {
    animate_output =
        isatty(STDOUT_FILENO) && getenv("NO_ANIMATION") == NULL;

    char username[128] = "user";

    const char *env_user = getenv("USER");

    if (env_user && *env_user) {
        snprintf(username, sizeof(username), "%s", env_user);
    } else {
        struct passwd *pw = getpwuid(getuid());

        if (pw && pw->pw_name)
            snprintf(username, sizeof(username), "%s", pw->pw_name);
    }

    char hostname[128] = "localhost";

    if (gethostname(hostname, sizeof(hostname) - 1) != 0)
        snprintf(hostname, sizeof(hostname), "localhost");

    hostname[sizeof(hostname) - 1] = '\0';

    struct utsname system_info;

    if (uname(&system_info) != 0) {
        memset(&system_info, 0, sizeof(system_info));

        snprintf(system_info.sysname,
                 sizeof(system_info.sysname), "Linux");

        snprintf(system_info.release,
                 sizeof(system_info.release), "unknown");

        snprintf(system_info.machine,
                 sizeof(system_info.machine), "unknown");
    }

    char os[256];
    char cpu[256];
    char memory[128];
    char uptime[128];
    char title[300];
    char details[8][512];

    read_os_name(os, sizeof(os));
    read_cpu_name(cpu, sizeof(cpu));
    read_memory(memory, sizeof(memory));
    read_uptime(uptime, sizeof(uptime));

    snprintf(title, sizeof(title), "%s@%s", username, hostname);

    snprintf(details[0], sizeof(details[0]), "OS        %s", os);
    snprintf(details[1], sizeof(details[1]), "Host      %s", hostname);
    snprintf(details[2], sizeof(details[2]),
             "Kernel    %s %s",
             system_info.sysname, system_info.release);

    snprintf(details[3], sizeof(details[3]), "CPU       %s", cpu);
    snprintf(details[4], sizeof(details[4]), "Uptime    %s", uptime);
    snprintf(details[5], sizeof(details[5]), "Memory    %s", memory);
    snprintf(details[6], sizeof(details[6]), "Shell     %s", shell_name());

    snprintf(details[7], sizeof(details[7]),
             "Terminal  %s",
             getenv("TERM") ? getenv("TERM") : "Unknown");

    const char *logo[] = {
        "          .--.        ",
        "         |o_o |       ",
        "         |:_/ |       ",
        "        //   \\ \\      ",
        "       (|     | )     ",
        "      /'\\_   _/`\\    ",
        "      \\___)=(___/    ",
        "                       "
    };

    puts("");

    for (int row = 0; row < 10; ++row) {
        const char *left =
            row < 8 ? logo[row] : "                       ";

        printf("%s%s%s  ", ORANGE, left, RESET);

        if (row == 0) {
            printf("%s%s%s%s", BOLD, CYAN, title, RESET);
        } else if (row == 1) {
            printf("%s--------------------------%s", MUTED, RESET);
        } else if (row >= 2 && row <= 9) {
            printf("%s", WHITE);
            print_animated(details[row - 2], 3);
            printf("%s", RESET);
        }

        putchar('\n');
        pause_ms(55);
    }

    printf("\n%s", MUTED);

    const char *swatches[] = {
        "  ", "  ", "  ", "  ",
        "  ", "  ", "  ", "  "
    };

    const char *swatch_colors[] = {
        "\033[40m", "\033[41m", "\033[42m", "\033[43m",
        "\033[44m", "\033[45m", "\033[46m", "\033[47m"
    };

    for (size_t i = 0;
         i < sizeof(swatches) / sizeof(swatches[0]); ++i) {
        printf("%s%s%s", swatch_colors[i], swatches[i], RESET);
    }

    printf("\n\n%s", RESET);

    return 0;
}
