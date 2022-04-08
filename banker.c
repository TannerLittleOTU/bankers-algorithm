#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Initialize constants for amount of customers and resources
// These may be any values >= 0
#define CUSTOMERS 5
#define RESOURCES 3

// The available amount of each resource
int available[RESOURCES];

// The maximum demand of each customer
int maximum[CUSTOMERS][RESOURCES];

// The amount currently allocated to each customer
int allocation[CUSTOMERS][RESOURCES];

// The remaining need of each customer
int need[CUSTOMERS][RESOURCES];

// Instantiate arrays
int available_tmp[RESOURCES];
int allocation_tmp[CUSTOMERS][RESOURCES];
int need_tmp[CUSTOMERS][RESOURCES];
int completed[CUSTOMERS] = {0};
int safeSequence[CUSTOMERS] = {0};

int request_resources(int customers, int request[]);
int release_resources(int customers);
int bankers_algorithm(int customers, const int request[]);

void *make_requests(void *arg);
void printMatrix();

pthread_mutex_t mutex;

int main(int argc, char const *argv[]) {
    // Ensure correct number of resources are entered
    if (argc != RESOURCES + 1) {
        fprintf(stderr, "Usage: banker");
        for (int i = 0; i < RESOURCES; i++) fprintf(stderr, " <resource %d>", i + 1);
        fprintf(stderr, "\n");

        exit(0);
    }

    // Ensure valid resource counts are entered
    for (int i = 1; i < argc; i++) {
        if (atoi(argv[i]) < 0) {
            fprintf(stderr, "Invalid resource count: %s\n", argv[i]);
            exit(0);
        }
    }

    // Initialize matrix
    for (int i = 0; i < RESOURCES; i++) {
        available[i] = atoi(argv[i + 1]);

        for (int j = 0; j < CUSTOMERS; j++) {
            maximum[j][i] = rand() % (available[i] + 1);
            need[j][i] = maximum[j][i];
            allocation[j][i] = 0;
        }
    }

    // Output initial matrix
    printMatrix();

    // Lock mutex
    pthread_mutex_init(&mutex, NULL);

    // Initialize threads
    pthread_t p[CUSTOMERS];

    for (int i = 0; i < CUSTOMERS; i++) {
        int *arg = malloc(sizeof(*arg));
        *arg = i;

        pthread_create(&(p[i]), NULL, make_requests, arg);
    }

    // Wait for all child threads to terminate
    for (int i = 0; i < CUSTOMERS; i++) {
        pthread_join((p[i]), NULL);
    }

    return 0;
}

void *make_requests(void *arg) {
    int customer = * ((int *) arg);

    while (!completed[customer]) {
        int sumRequest = 0;
        int request[RESOURCES] = {0};

        for (int i = 0; i < RESOURCES; i++) {
            request[i] = rand() % (need[customer][i] + 1);
            sumRequest = sumRequest + request[i];
        }

        if (sumRequest != 0)
            while (request_resources(customer, request) == -1);
    }

    return 0;
}

// Returns 0 if successful, -1 if unsuccessful
int request_resources(int customers, int request[]) {
    // Lock mutex
    pthread_mutex_lock(&mutex);

    printf("\nP%d has requested resources [A B C]: ", customers + 1);
    for (int i = 0; i < RESOURCES; i++) {
        printf("%d ", request[i]);
    }

    printf("\n");

    // Loop through resources
    for (int i = 0; i < RESOURCES; i++) {
        // Requested resources are more than available resources
        if (request[i] > available[i]) {
            printf("P%d is waiting for resources.\n", customers + 1);

            // Unlock Mutex
            pthread_mutex_unlock(&mutex);

            // Return unsuccessful
            return -1;
        }
    }

    // Check for safe state using bankers algorithm
    int status = bankers_algorithm(customers, request);

    if (status == 0) {
        printf("Safe sequence found: ");

        for (int i = 0; i < CUSTOMERS; i++) {
            printf("P%d ", safeSequence[i] + 1);
        }

        printf("\n");

        int complete = 1;

        // Updated resources
        for (int j = 0; j < RESOURCES; j++) {
            allocation[customers][j] = allocation[customers][j] + request[j];
            available[j] = available[j] - request[j];
            need[customers][j] = need[customers][j] - request[j];

            if (need[customers][j] != 0) {
                complete = 0;
            }
        }

        // Customer has released all resources
        if (complete) {
            completed[customers] = 1;
            release_resources(customers);
        }

        printMatrix();
    } else {
        printf("Safe sequence could not be found.\n");
        status = -1;
    }

    // Unlock mutex
    pthread_mutex_unlock(&mutex);

    // Return status
    return status;
}

int release_resources(int customers) {
    printf("P%d has released all resources.\n", customers + 1);

    // Update resources
    for (int j = 0; j < RESOURCES; j++) {
        available[j] = available[j] + allocation[customers][j];
        allocation[customers][j] = 0;
    }

    return 0;
}

// Determines safe sequence
int bankers_algorithm(int customers, const int request[]) {
    int complete[CUSTOMERS] = {0};

    // Copy resources to temporary arrays
    for (int i = 0; i < RESOURCES; i++) {
        available_tmp[i] = available[i];
        for (int j = 0; j < CUSTOMERS; j++) {
            allocation_tmp[j][i] = allocation[j][i];
            need_tmp[j][i] = need[j][i];
        }
    }

    // Update temporary resources
    for (int i = 0; i < RESOURCES; i++) {
        available_tmp[i] = available_tmp[i] - request[i];
        allocation_tmp[customers][i] = allocation_tmp[customers][i] + request[i];
        need_tmp[customers][i] = need_tmp[customers][i] - request[i];
    }

    // Determine  safe sequence
    int count = 0;

    while (1) {
        int customer = -1;

        for (int i = 0; i < CUSTOMERS; i++) {
            int valid = 1;
            for (int j = 0; j < RESOURCES; j++) {
                if (need_tmp[i][j] > available_tmp[j] || complete[i] == 1) {
                    valid = 0;
                    break;
                }
            }

            if (valid) {
                customer = i;
                break;
            }
        }

        if (customer != -1) {
            safeSequence[count] = customer;
            count++;
            complete[customer] = 1;
            for (int k = 0; k < RESOURCES; k++) {
                available_tmp[k] = available_tmp[k] + allocation_tmp[customer][k];
            }
        } else {
            for (int i = 0; i < CUSTOMERS; i++) {
                if (complete[i] == 0) {
                    return -1;
                }
            }

            return 0;
        }
    }
}

void printMatrix() {
    // Print header
    printf("\tAllocated\tMax\t\tAvailable\n");
    printf("\tA  B  C\t\tA  B  C\t\tA  B  C\n");

    // Loop through customers
    for (int i = 0; i < CUSTOMERS; i++) {
        // Output customers
        printf("P%d\t", i + 1);

        // Output  allocated resources for each customer
        for (int j = 0; j < RESOURCES; j++) {
            printf("%-*d ", (allocation[i][j] > 9 ? 1 : 2), allocation[i][j]);
        }

        printf("\t");

        // Output needed resources for each customer
        for (int j = 0; j < RESOURCES; j++) {
            printf("%-*d ", (need[i][j] > 9 ? 1 : 2), need[i][j]);
        }

        printf("\t");

        // Output available resources only on first line
        if (i == 0) {
            for (int k = 0; k < RESOURCES; k++) {
                printf("%-*d ", (available[k] > 9 ? 1 : 2), available[k]);
            }
        }

        printf("\n");
    }

    printf("\t\t\n");
}
