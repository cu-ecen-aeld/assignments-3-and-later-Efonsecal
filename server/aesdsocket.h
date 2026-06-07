
#include <bsd/sys/queue.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

// Global variables
static bool daemoned = false;
static pthread_mutex_t data_file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Client data structure
struct client_data {
    int client_fd;
    struct addrinfo *socket_addr_info;
    int socket_fd;
    char *ip_address;
};

// Thread data structure
struct thread_data {
    pthread_t* thread;
    pthread_mutex_t *mutex;
    char *data_buffer;
    bool thread_complete_success;
    struct client_data *client;
};

// Linked list of thread data structures
typedef struct thread_data_node {
    struct thread_data *thread_data;
    SLIST_ENTRY(thread_data_node) entries;
} thread_data_node_t;

// Function prototypes
void reg_signal(int signal_num, struct sigaction * action);
static void term_handler(int signal_num);
void daemonize();
struct thread_data *new_client_thread_data(int socket_fd, struct addrinfo *socket_addr_info, int client_fd, char *ip_address);
void *process_client_data(void *arg);
void *append_timestamp();
int append_to_file(char *data);
int read_from_file(char *send_buffer, int *read_size, int buffer_size);
void log_message(int log_level, char * message, ...);
const char* log_level_str(int level);