#include "aesdsocket.h"

#define SOCKET_PORT "9000"
#define INIT_BUFF_SIZE 1024
#define OUTPUT_FILE "/var/tmp/aesdsocketdata"

volatile bool signal_received = false;
int current_buffer_size = INIT_BUFF_SIZE;

int main (int argc, char ** argv) {
    
    // Register signals
    struct sigaction handlers_actions;
    memset(&handlers_actions, 0x00, sizeof(struct sigaction));
    handlers_actions.sa_handler = term_handler;
    reg_signal(SIGINT, &handlers_actions);
    reg_signal(SIGTERM, &handlers_actions);

    /* Setup socket */
    
    // Step 1: Define FD
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    // Step 2-1: Define address
    // Socket config via address options
    struct addrinfo socket_hints;
    memset(&socket_hints, 0x00, sizeof(struct addrinfo));
    socket_hints.ai_family = AF_UNSPEC; // Work with both protocols
    socket_hints.ai_flags = AI_PASSIVE; // Set the address automatically
    socket_hints.ai_socktype = SOCK_STREAM; // This is a stream socket

    // Define socket address
    struct addrinfo *socket_addr_info;
    int address_op_res = getaddrinfo(NULL, SOCKET_PORT, &socket_hints, &socket_addr_info);
    if (address_op_res != 0) {
        freeaddrinfo(socket_addr_info);
        close(socket_fd);
        log_message(LOG_ERR, "Unsuccessful socket address setting: %s", gai_strerror(address_op_res));
        exit(1);
    } else {
        log_message(LOG_INFO, "Adress successfully set.");
    }

    // Step 2-2: Bind socket
    // Enable address re-use
    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Bind
    int bind_op_res = bind(socket_fd, socket_addr_info->ai_addr, socket_addr_info->ai_addrlen);
    if (bind_op_res != 0) {
        freeaddrinfo(socket_addr_info);
        close(socket_fd);
        log_message(LOG_ERR, "Unsuccessful socket binding: %d (%s)", errno, strerror(errno));
        exit(1);
    } else {
        log_message(LOG_INFO, "Successfully binded.");
    }

    // Step 3: Enable listening
    int listen_op_res = listen(socket_fd, 1);
    if (listen_op_res != 0) {
        freeaddrinfo(socket_addr_info);
        close(socket_fd);
        log_message(LOG_ERR, "Unsuccessful socket listen enabling: %d (%s)", errno, strerror(errno));
        exit(1);
    } else {
        log_message(LOG_INFO, "Started listening ...");
    }
    remove(OUTPUT_FILE);
    // Check for background mode enabling
    if (argc > 1) {
        if (argc > 2) {
            log_message(LOG_ERR, "Expected 1 argument but got: %d", argc - 1);
            exit(1);
        } else if (strcmp(*(argv + 1), "-d") != 0) {
            log_message(LOG_ERR, "Unknown option: %s", *(argv + 1));
            exit(1);
        } else {
            log_message(LOG_INFO, "Sent to background process!");
            daemonize(); // Start as a daemon
        }
    }

    // Step 4: Wait for connection or termination
    
    // Reserve memory for client address
    int client_fd = -1;
    struct sockaddr client_address;
    char client_ip[16];
    unsigned char ip_d[4];
    memset(client_ip, 0x00, 16);
    socklen_t client_address_l = sizeof(struct sockaddr);

    // Reserve memory for incomming data
    char *data_buffer = malloc(current_buffer_size);
    if (data_buffer == NULL) {
        freeaddrinfo(socket_addr_info);
        close(socket_fd);
        log_message(LOG_ERR, "Couldn't allocate databuffer: %d (%s)", errno, strerror(errno));
        exit(1);
    }
    memset(data_buffer, 0x00, current_buffer_size);

    // Track received bytes
    size_t packet_bytes = 0;
    ssize_t bytes_received = 0;

    // Packet and connection trackers
    bool packet_received = false;
    bool connection_terminated = false;

    // Track bytes to send
    int read_size = 0;

    // Connect and wait for data
    while (!signal_received) {

        // Accept client connection
        client_fd = accept(socket_fd, &client_address, &client_address_l);
        if (client_fd < 0) {
            if (signal_received) break;
            log_message(LOG_ERR, "Unsuccessful accept client operation: %d (%s)", errno, strerror(errno));
            break;
        } else {
            memcpy(ip_d, client_address.sa_data + 2, 4);
            snprintf(client_ip, 16, "%d.%d.%d.%d", ip_d[0], ip_d[1], ip_d[2], ip_d[3]);
            log_message(LOG_INFO, "Accepted connection from %s\n", client_ip);
        }

        connection_terminated = false;
        while (!connection_terminated) {
            packet_received = false;
            do {
                // Receive data
                bytes_received = recv(client_fd, data_buffer + packet_bytes, current_buffer_size - packet_bytes, 0);
                packet_bytes += bytes_received;

                // Check if buffer is full, if so, increment space
                if (packet_bytes == current_buffer_size) {
                    current_buffer_size *= 2;
                    char *fallback_buffer = realloc(data_buffer, current_buffer_size);
                    if (fallback_buffer == NULL) {
                        free(data_buffer);
                        log_message(LOG_WARNING, "Couldn't increase buffer size: %s", strerror(errno));
                        break;
                    }
                    data_buffer = fallback_buffer;
                } else if (bytes_received <= 0) {
                    break;
                } else if (data_buffer[packet_bytes - 1] == '\n') {
                    packet_received = true;
                }
            } while (!packet_received);

            if (bytes_received < 0) {
                if (signal_received) break;
                log_message(LOG_ERR, "Unsuccessful data receieving operation: %d (%s)", errno, strerror(errno));
                break;
            } else if (bytes_received == 0) {
                connection_terminated = true;
            } else if (data_buffer != NULL && packet_bytes > 0) {

                if (append_to_file(data_buffer) != 0) {
                    log_message(LOG_ERR, "Error appending to file: %d (%s)", errno, strerror(errno));
                    exit(1);
                }

                memset(data_buffer, 0x00, current_buffer_size);
                if (read_from_file(data_buffer, &read_size) != 0) {
                    log_message(LOG_ERR, "Error reading from file: %d (%s)", errno, strerror(errno));
                    exit(1);
                }

                if (send(client_fd, data_buffer, read_size, 0) < 0) {
                    if (signal_received) break;
                    log_message(LOG_ERR, "Unsuccessful data sending operation: %d (%s)", errno, strerror(errno));
                    break;
                }
                shutdown(client_fd, SHUT_RDWR);
                connection_terminated = true;
            }
        }

        log_message(LOG_INFO, "Client disconnected.");
        packet_bytes = 0;
        memset(data_buffer, 0x00, current_buffer_size);

        if (client_fd != -1) {
            close(client_fd);
            client_fd = -1;
        }
    }

    free(data_buffer);
    log_message(LOG_INFO, "Starting cleanup!");
    freeaddrinfo(socket_addr_info);
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    if (client_fd != -1) {
        close(client_fd);
        client_fd = -1;
    }
    remove(OUTPUT_FILE);
    log_message(LOG_INFO, "Finished execution.");
    return 0;
}

void reg_signal(int signal_num, struct sigaction * action) {
    if (sigaction(signal_num, action, NULL) != 0) {
        log_message(LOG_ERR, "Error %d (%s) while register process of %s", errno, strerror(errno), strsignal(signal_num));
        exit(1);
    }
}

static void term_handler(int signal_num) {
    signal_received = true;
    log_message(LOG_INFO, "Caught signal, exiting");
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

int append_to_file(char *data) {
    FILE * fd = fopen(OUTPUT_FILE, "a");
    
    if (fd == NULL) {
        return errno;
    }

    size_t textLen = strlen(data);
    size_t writtenBytes = fwrite(data, 1, strlen(data), fd);

    if (writtenBytes < textLen) {
        return errno;
    }

    if (fclose(fd) != 0) {
        return errno;
    }

    return 0;
}

int read_from_file(char *send_buffer, int *read_size) {
    FILE *fd = fopen(OUTPUT_FILE, "r");

    if (fd != NULL) {
        *read_size = fread(send_buffer, 1, current_buffer_size, fd);

        if (fclose(fd) != 0) {
            return errno;
        }

    } else if (ferror(fd)) {
        return errno;
    }

    return 0;
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
    va_list args_logs, args_io;
    va_start(args_logs, message);
    va_copy(args_io, args_logs);
    vsyslog(log_level, message, args_logs);

    if (!daemoned) {
        printf("[%s] ", log_level_str(log_level));
        vprintf(message, args_io);
        printf("\n");
    }

    va_end(args_logs);
    va_end(args_io);
}