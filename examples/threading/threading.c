#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

struct thread_data *current_thread_data = NULL;
struct thread_data *previous_thread_data = NULL;

void* threadfunc(void* thread_param)
{
    // Added steps with previous data cleaning if necessary
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    int mc = pthread_mutex_lock(thread_func_args->mutex);
    if (mc != 0) {
        current_thread_data->error_code = mc;
        thread_func_args->thread_complete_success = false;
    } else {
        if (current_thread_data->prev_thread != NULL) {
            if (current_thread_data->prev_thread->thread_complete_success) {
                current_thread_data->prev_thread = NULL;
            }
        }
        usleep(thread_func_args->wait_to_release_ms * 1000);
        mc = pthread_mutex_unlock(thread_func_args->mutex);
        if (mc != 0) {
            current_thread_data->error_code = mc;
            thread_func_args->thread_complete_success = false;
        } else {
            thread_func_args->thread_complete_success = true;
        }
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    // Added required steps, allocation always happens but if there's any previous data is saved

    if (current_thread_data != NULL) {
        if (!current_thread_data->thread_complete_success) {
            previous_thread_data = current_thread_data;
        } else {
            current_thread_data = NULL;
        }
    }

    current_thread_data = init_thread_data(thread, mutex, wait_to_obtain_ms, wait_to_release_ms);
    if (previous_thread_data != NULL) {
        current_thread_data->prev_thread = previous_thread_data;
    }

    int rc = pthread_create(thread, NULL, threadfunc, current_thread_data);
    if (rc != 0) {
        return false;
    } else {
        return true;
    }
}

struct thread_data* init_thread_data(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms) {
    struct thread_data* new_thread_data = malloc(sizeof(struct thread_data));
    if (new_thread_data != NULL) {
        new_thread_data->prev_thread = NULL;
        new_thread_data->current_thread = thread;
        new_thread_data->mutex = mutex;
        new_thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
        new_thread_data->wait_to_release_ms = wait_to_release_ms;
        new_thread_data->error_code = 0;
        new_thread_data->thread_complete_success = false;
    }
    return new_thread_data;
}