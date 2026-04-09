/*
 * Минимальный fetch в духе простых скриптов: цвета, ascii слева, строки инфо справа.
 * Без popen, malloc и кэша — только fopen, sysinfo, uname, opendir.
 */

#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#define NORMAL "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"
#define WHITE "\x1B[37m"
#define GRAY "\x1B[90m"
#define NAVY "\x1B[2m"

#define SEPARATOR "  "
#define COLOR_CHAR "● "
#define COLOR_KEY MAGENTA
#define COLOR_MAIN WHITE

/* Рыбка слева. Без ANSI внутри строк, чтобы работало ровное выравнивание. */
static const char *ascii[] = {
	"     .--.     ",
	"    |o_o |    ",
	"    |:_/ |    ",
	"   //   \\ \\   ",
	"  (|     | )  ",
	" /'\\_   _/`\\ ",
	" \\___)=(___/ ",
	"              ",
};

static int line;

static void print_colored(const char *text, const char *fmt)
{
	printf("%s%s", fmt, text);
}

static void newline(void)
{
	printf("\n");
	if ((size_t)line < sizeof ascii / sizeof ascii[0]) {
		printf("%s%-14s%s%s", COLOR_MAIN, ascii[line], NORMAL, SEPARATOR);
		line++;
	}
}

static void print_info(const char *key, const char *value)
{
	printf("%s%-7s%s%s", COLOR_KEY, key, NORMAL, SEPARATOR);
	print_colored(value, COLOR_MAIN);
	newline();
}

static void chomp(char *s)
{
	s[strcspn(s, "\n")] = '\0';
}

static void unquote(char *s)
{
	size_t n = strlen(s);
	if (n >= 2 && s[0] == '"' && s[n - 1] == '"') {
		memmove(s, s + 1, n - 2);
		s[n - 2] = '\0';
	}
}

static void read_os_line(const char *prefix, char *out, size_t cap)
{
	FILE *f = fopen("/etc/os-release", "r");
	out[0] = '\0';
	if (!f)
		return;

	char buf[256];
	size_t plen = strlen(prefix);
	while (fgets(buf, sizeof buf, f)) {
		chomp(buf);
		if (strncmp(buf, prefix, plen) != 0)
			continue;
		snprintf(out, cap, "%s", buf + plen);
		unquote(out);
		break;
	}
	fclose(f);
}

static void read_kernel(char *out, size_t cap)
{
	struct utsname u;
	uname(&u);
	snprintf(out, cap, "%s", u.release);
}

static void read_uptime(char *out, size_t cap)
{
	struct sysinfo si;
	sysinfo(&si);
	unsigned long t = si.uptime;
	unsigned h = (unsigned)(t / 3600);
	unsigned m = (unsigned)((t % 3600) / 60);
	unsigned s = (unsigned)(t % 60);
	snprintf(out, cap, "%uh %um %us", h, m, s);
}

static int count_pacman(void)
{
	DIR *d = opendir("/var/lib/pacman/local");
	if (!d)
		return -1;
	int n = 0;
	struct dirent *e;
	while ((e = readdir(d)) != NULL) {
		if (e->d_name[0] != '.')
			n++;
	}
	closedir(d);
	return n;
}

static int count_dpkg(void)
{
	FILE *f = fopen("/var/lib/dpkg/status", "r");
	if (!f)
		return -1;
	char buf[256];
	int n = 0;
	while (fgets(buf, sizeof buf, f)) {
		if (strncmp(buf, "Package:", 8) == 0)
			n++;
	}
	fclose(f);
	return n;
}

static void read_packages(char *out, size_t cap)
{
	int n = count_pacman();
	if (n >= 0) {
		snprintf(out, cap, "%d pkgs (pacman)", n);
		return;
	}
	n = count_dpkg();
	if (n >= 0) {
		snprintf(out, cap, "%d pkgs (dpkg)", n);
		return;
	}
	snprintf(out, cap, "unknown");
}

static void read_init(char *out, size_t cap)
{
	FILE *f = fopen("/proc/1/comm", "r");
	if (!f) {
		snprintf(out, cap, "unknown");
		return;
	}
	fgets(out, (int)cap, f);
	fclose(f);
	chomp(out);
}

static void print_colors(void)
{
	print_colored("colors", COLOR_KEY);
	printf(SEPARATOR);
	const char *cols[] = { RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, NAVY };
	for (size_t i = 0; i < sizeof cols / sizeof cols[0]; i++)
		print_colored(COLOR_CHAR, cols[i]);
	printf("\n");
}

int main(void)
{
	char name[128], version[64], kernel[128], uptime[64], pkgs[96], init[32];
	char id[64];

	read_os_line("NAME=", name, sizeof name);
	read_os_line("VERSION_ID=", version, sizeof version);
	read_os_line("ID=", id, sizeof id);
	if (name[0] == '\0')
		read_os_line("PRETTY_NAME=", name, sizeof name);
	if (version[0] == '\0')
		read_os_line("VERSION=", version, sizeof version);
	if (version[0] == '\0' && (strcmp(id, "arch") == 0 || strcmp(id, "void") == 0))
		snprintf(version, sizeof version, "rolling");

	read_kernel(kernel, sizeof kernel);
	read_uptime(uptime, sizeof uptime);
	read_packages(pkgs, sizeof pkgs);
	read_init(init, sizeof init);

	line = 0;

	newline();
	struct passwd *pw = getpwuid(getuid());
	print_colored(pw ? pw->pw_name : "?", YELLOW);
	print_colored("@", RED);
	char host[256];
	gethostname(host, sizeof host);
	print_colored(host, BLUE);
	newline();

	print_info("os", name[0] ? name : "Linux");
	print_info("ver", version[0] ? version : "—");
	print_info("krnl", kernel);
	print_info("up", uptime);
	print_info("pkg", pkgs);
	print_info("init", init);

	print_colors();

	printf("\n%s", NORMAL);
	return 0;
}
