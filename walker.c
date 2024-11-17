#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

typedef struct shared_val_tag {
    int loc;                        // user's current location
    int over;                       // if over = 0, walk is not over. if over=1, walk is over.
    pthread_mutex_t mtx;            // a mutex
    pthread_cond_t *cond;           // array of condition variables
} shared_val;

typedef struct thread_arg_tag {
    int n;
    int id;
    int value;  
    shared_val *sv;
} thread_arg_t;

void* thread_main(void* thread_arg) {
    thread_arg_t* arg = thread_arg;
    int n = arg->n;
    int id = arg->id;
    shared_val* sv = arg->sv;

    pthread_mutex_lock(&sv->mtx);
    
    while (1) {
        while (sv->loc != id && !sv->over) {
            pthread_cond_wait(&sv->cond[id], &sv->mtx);
        }
        
        if (sv->over) {
            pthread_mutex_unlock(&sv->mtx);
            pthread_exit(NULL);
        }

        int value = arg->value;  // Retrieve the current value
        if (value == 0) {
            sv->over = 1;
            printf("Ends at %d\n", id);
            for (int i = 0; i < n; i++) {
                pthread_cond_signal(&sv->cond[i]);
            }
            pthread_mutex_unlock(&sv->mtx);
            pthread_exit(NULL);
        }

        int steps = value;
        int next_loc = (id + steps) % n;
        arg->value--;  // Decrement the value at the current location
        sv->loc = next_loc;

        printf("At i=%d, user moves forward by %d space(s) to i=%d. New value = %d\n", 
                id, steps, next_loc, arg->value);
        
        pthread_cond_signal(&sv->cond[next_loc]);
        pthread_mutex_unlock(&sv->mtx);
        pthread_mutex_lock(&sv->mtx);
    }
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Usage: %s n\n", argv[0]);
        return 0;
    }

    int n = atoi(argv[1]);  // Number of locations and threads
    printf("n = %d\n", n);

    shared_val* val = malloc(sizeof(shared_val));
    val->loc = 0;   // Start at location 0
    val->over = 0;  // Walk is not over
    pthread_mutex_init(&val->mtx, NULL);
    val->cond = malloc(sizeof(pthread_cond_t) * n);

    for (int i = 0; i < n; i++) {
        pthread_cond_init(&val->cond[i], NULL);
    }

    pthread_t threads[n];
    thread_arg_t args[n];

    for (int i = 0; i < n; i++) {
        args[i].n = n;
        args[i].id = i;
        args[i].value = i + 1;  // Each location starts with value (i+1)
        args[i].sv = val;

        if (pthread_create(&threads[i], NULL, thread_main, &args[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Start the walk by signaling thread 0
    pthread_mutex_lock(&val->mtx);
    pthread_cond_signal(&val->cond[0]);
    pthread_mutex_unlock(&val->mtx);

    // Wait for all threads to finish
    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    for (int i = 0; i < n; i++) {
        pthread_cond_destroy(&val->cond[i]);
    }

    free(val->cond);
    pthread_mutex_destroy(&val->mtx);
    free(val);

    return 0;
}