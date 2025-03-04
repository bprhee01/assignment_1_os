/*
Ben Rhee
OS assignment 1
Prefix Sum parrallelized through hillis and steele algorithm
DO "./my-sum num_elems num_cores input_file output_file"
*/
using namespace std;
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>

// shared data 
struct SharedMemory {
    int n;                  // Number of elements
    int m;                  // Number of processes
    int *A;                 // Input array
    int *B;                 // Output array
    int max_runtime;        // Maximum iterations (log2(n))
    int layer;              // Current layer
    int barrier_count;      // Barrier counter for synchronization
    int barrier_phase;      // Barrier phase for synchronization
};


class Barrier {
private:
    int *current_process_count;    // Pointer to shared current_process_counter
    int *phase;    // Pointer to shared phase
    int total_processes;     // Total number of processes
public:
    Barrier(int *current_process_count_ptr, int *phase_ptr, int total_processes) {
        current_process_count = current_process_count_ptr;
        phase = phase_ptr;
        total_processes = total_processes;
        *current_process_count = 0;
        *phase = 0;
    }
    
    void arrive_and_wait(int pid) {
        //  if og process
        if (pid == 0) {
            while (*current_process_count < total_processes - 1) {
                usleep(10);
            }
            // Reset current_process_counter and toggle phase
            *current_process_count = 0;
            *phase = 1 - *phase;
        // worked processes
        } else {
            (*current_process_count)++;
            int current_phase = *phase;
            while (current_phase == *phase) {
                usleep(10); 
            }
        }
    }
};

//function prototypes
void process_input_file(const string &input_file, int *A, int n);
void write_output_file(const string &output_file, int *B, int n);
void prefix_sum(SharedMemory *shared_memory, int pid);

// main
int main(int argc, char *argv[]) {
    // parse command-line arguments
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

    // Initialize shared memory
    size_t size_segment = sizeof(SharedMemory) + (2 * n * sizeof(int)); 
    
    // Create shared memory segment
    int shmid = shmget(IPC_PRIVATE, size_segment, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    
    if (shmid == -1) {
        fprintf(stderr, "Error: segment creation failed.\n");
        exit(1);
    }

    // Attach segment
    SharedMemory *shared_memory = (SharedMemory*)shmat(shmid, nullptr, 0);

    if (shared_memory == (void*)-1) {
        fprintf(stderr, "Error: segment attachment failed.\n");
        shmctl(shmid, IPC_RMID, nullptr);
        exit(1);
    }

    // Setup memory address pointers
    shared_memory->A = (int*)((char*)shared_memory + sizeof(SharedMemory));
    shared_memory->B = shared_memory->A + n;

    // Initialize shared_memory variables
    shared_memory->n = n;
    shared_memory->m = m;
    shared_memory->max_runtime = static_cast<int>(ceil(log2(n)));
    shared_memory->layer = 0;
    shared_memory->barrier_count = 0;
    shared_memory->barrier_phase = 0;

    // Read input file
    process_input_file(input_file, shared_memory->A, n);

    // Initialize B as A for now
    for (int i = 0; i < n; i++) {
        shared_memory->B[i] = shared_memory->A[i];
    }
    
    // Create barrier with shared memory
    Barrier barrier(&shared_memory->barrier_count, &shared_memory->barrier_phase, m);
    
    // Create worker processes
    vector<pid_t> child_processes;
    for (int i = 0; i < m; i++) {
        pid_t pid = fork();

        // If in child process
        if (pid == 0) {
            prefix_sum(shared_memory, i); // i keeps tracks of the actual process num

            // Detach from segment
            shmdt(shared_memory);
            exit(0);
        }
        // Parent process
        else if (pid > 0) {
            child_processes.push_back(pid);
        }
        // Failure
        else {
            fprintf(stderr, "Error: process creation failed.\n");
            exit(1);
        }
    }

    // Wait for all processes to finish
    for (pid_t child : child_processes) {
        waitpid(child, nullptr, 0);
    }

    // Write output file
    write_output_file(output_file, shared_memory->B, n);
    
    // Clean memory
    shmdt(shared_memory); // Detach process from memory segment
    shmctl(shmid, IPC_RMID, nullptr); // Delete shared memory segment completely

    return 0;
}

void process_input_file(const string &input_file, int *A, int n) {
    ifstream input_file_stream(input_file);
    for (int i = 0; i < n; i++) {
        input_file_stream >> A[i];
    }
    input_file_stream.close();
}

void write_output_file(const string &output_file, int *B, int n) {
    ofstream output_file_steam(output_file);
    for (int i = 0; i < n; i++) {
        output_file_steam << B[i] << endl;
    }
    output_file_steam.close();
}

void prefix_sum(SharedMemory *shared_memory, int pid) {
    int n = shared_memory->n; // Total elements
    int m = shared_memory->m; // Total processes
    int max_runtime = shared_memory->max_runtime; // Max runtime
    int elems_per_process = n / m;
    int leftover_elems = n % m;

    int start_idx, end_idx;

    // Calculate start and end indices for this process
    if (pid < leftover_elems) {
        // First 'leftover_elems' processes handle (elems_per_process + 1) elements each
        start_idx = pid * (elems_per_process + 1);
        end_idx = start_idx + elems_per_process + 1;
    } else {
        // Remaining processes handle elems_per_process elements each
        start_idx = (leftover_elems * (elems_per_process + 1)) + 
                   ((pid - leftover_elems) * elems_per_process);
        end_idx = start_idx + elems_per_process;
    }

    Barrier barrier(&shared_memory->barrier_count, &shared_memory->barrier_phase, m);
    
    // initilize temporary array 
    vector<int> temp(n);

    // Hillis-Steele
    for (int layer = 0; layer < max_runtime; layer++) {
        // update layer if needed
        if (pid == 0) {
            shared_memory->layer = layer;
        }
        
        // Wait
        barrier.arrive_and_wait(pid);
        
        // Copy current output to temp array
        for (int i = 0; i < n; i++) {
            temp[i] = shared_memory->B[i];
        }
        
        // Calculate distance of complement for this layer
        int complement_distance = 1 << layer;
        
        // Calculate sum
        for (int i = start_idx; i < end_idx; i++) {
            // only sum complement if it exists
            if (i >= complement_distance) {
                shared_memory->B[i] = temp[i] + temp[i - complement_distance];
            }
        }
        
        barrier.arrive_and_wait(pid);
    }
}