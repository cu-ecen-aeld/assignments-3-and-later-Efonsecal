#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

static bool daemoned = false;

void reg_signal(int signal_num, struct sigaction * action);
static void term_handler(int signal_num);
void daemonize();
int append_to_file(char *data);
int read_from_file(char *send_buffer, int *read_size);
void log_message(int log_level, char * message, ...);
const char* log_level_str(int level);