#include "aesdsocket.h"

int main (int argc, char ** argv) {
    
    // Register signals
    struct sigaction handlers_actions;
    memset(&handlers_actions, 0x00, sizeof(struct sigaction));
    handlers_actions.sa_handler = term_handler;
    reg_signal(SIGINT, &handlers_actions);
    reg_signal(SIGTERM, &handlers_actions);

    // Check for background mode
    if (argc > 1) {
        if (argc > 2) {
            log_message(LOG_ERR, "Expected 1 argument but got: %d", argc - 1);
            exit(1);
        } else if (strcmp(*(argv + 1), "-d") != 0) {
            log_message(LOG_ERR, "Unknown option: %s", *(argv + 1));
            exit(1);
        } else {
            daemonize(); // Start as a daemon
        }
    }

    

    pause();
}

void reg_signal(int signal_num, struct sigaction * action) {
    if (sigaction(signal_num, action, NULL) != 0) {
        log_message(LOG_ERR, "Error %d (%s) while register process of %s", errno, strerror(errno), strsignal(signal_num));
        exit(1);
    }
}

static void term_handler(int signal_num) {
    switch (signal_num) {
        case SIGINT:
            log_message(LOG_INFO, "Caught SIGINT ...");
            break;
        case SIGTERM:
            log_message(LOG_INFO, "Caught SIGTERM ...");
            break;
    }
}

void daemonize() {
    pid_t pid = fork();
    
    if (pid < 0) {
        log_message(LOG_ERR, "Couldn't fork process!");
        exit(1);
    }
    
    if (pid > 0) {
        log_message(LOG_INFO, "Started listening in background ...");
        exit(0); 
    }
    
    // Keep executing as daemon if able to register session
    if (setsid() < 0) {
        log_message(LOG_ERR, "Couldn't open session for listening!");
        exit(1);
    }
    
    chdir("/");
    
    // Send stdin/stdout/stderr to /dev/null
    int dev_null = open("/dev/null", O_RDWR);
    if (dev_null < 0) {
        log_message(LOG_ERR, "Unable to close IOs!");
        exit(1);
    }
    dup2(dev_null, STDIN_FILENO);
    dup2(dev_null, STDOUT_FILENO);
    dup2(dev_null, STDERR_FILENO);
    close(dev_null);
    daemoned = true;
}

const char* log_level_str(int level) {
    switch(level) {
        case LOG_ERR:     return "ERR";
        case LOG_WARNING: return "WARNING";
        case LOG_INFO:    return "INFO";
        default:          return "UNKNOWN";
    }
}

void log_message(int log_level, char * message, ...) {
    va_list args;
    va_start(args, message);
    
    if (daemoned) {
        vsyslog(log_level, message, args);
    } else {
        printf("[%s] ", log_level_str(log_level));
        vprintf(message, args);
        printf("\n");
    }

    va_end(args);
}