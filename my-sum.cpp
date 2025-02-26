#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <math.h>


//shared data
typedef struct {

} SharedMemory;


//reusable barrier

void reusableBarrier() {

}


//prefix sum 
void prefixSum() {

}


// main
int main(int argc, char *argv[]) {
    // Parse command-line arguments
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <n> <m> <input_file> <output_file>\n", argv[0]);
        exit(1);
    }
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    char *input_file = argv[3];
    char *output_file = argv[4];

    // Validate input
    if (n <= 0 || m <= 0) {
        fprintf(stderr, "Error: n and m must be greater than 0.\n");
        exit(1);
    }

    return 0;
}