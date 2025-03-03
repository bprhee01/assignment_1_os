/*
Ben Rhee
OS assignment 1
Prefix Sum parrallelized through hillis and steele algorithm
DO "./my-sum numElems numCores input_file output_file"
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


// Barrier class
class Barrier{
private:
    int current_process_count;
    int total_process_count;
public:
    Barrier(int m){
        total_process_count = m;
        current_process_count = 0;
    }
    void arrive_and_wait(int pid){

    }
};



// shared data structure
struct SharedMemory {
    int n;
    int m;
    int *A;
    int *B;
    Barrier *barrier;
};

//function prototypes
void process_input_file(const string &input_file, int *A, int n);
void write_output_file(const string &output_file, int *B, int n);
void prefix_sum(SharedMemory *shared_memory, int pid);

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

    //  init shared memory
    size_t size_segment = sizeof(SharedMemory) + (2 * n * sizeof(double)); // shared mem struct + size of the two arrays
    // create shared memory segment
    int shmid = shmget(IPC_PRIVATE, size_segment, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); // private for segments that know ID, size_segment, create segment and give read/write perms
    
    if (shmid == -1){
        fprintf(stderr, "Error: segment creation failed.\n");
        exit(1);
    }

    //attach segment
    SharedMemory *shared_memory = (SharedMemory*)shmat( shmid, nullptr, 0); //attaches schmat segment with id

    if (shared_memory == (void*)-1) {
        fprintf(stderr, "Error: segment attachment failed.\n");
        shmctl(shmid, IPC_RMID, nullptr);
        exit(1);
    }

    // setup memory address pointers
    shared_memory->A = (int*)((char*)shared_memory + sizeof(SharedMemory)); //skip of SharedMemory struct 
    shared_memory->B = shared_memory->A + n;

     //as of now the segment looks like [SharedData struct][Array A (n elements)][Array B (n elements)]

    //initiliaze shared_memory variables
    shared_memory->n = n;
    shared_memory->m = m;
    shared_memory->barrier = new Barrier(m);


    //read input file
    process_input_file(input_file,shared_memory->A,n);

    //initialize B as A for now
    for (int i = 0; i < n; i++) {
        shared_memory->B[i] = shared_memory->A[i];
    }
    
    //create worker processes
    vector<pid_t> child_processes;
    for (int i = 0; i < m; i ++){
        pid_t pid = fork();

        //if in child proccess
        if (pid == 0){
            prefix_sum(shared_memory, pid);

            //detach from segment
            shmdt(shared_memory);
            exit(0);
        }
        //parent process
        else if (pid > 0){
            child_processes.push_back(pid);
        }
        //failure
        else {
            fprintf(stderr, "Error: process creation failed.\n");
            exit(1);
        }
    }

    //wait for all proccesses to finish
    for (pid_t child : child_processes) {
        waitpid(child, nullptr, 0); //wait for child to finish
    }

    //write output file
    write_output_file(output_file, shared_memory->B, n);
    
    //clean memory
    delete shared_memory->barrier; //deallocate memory for barrier
    shmdt(shared_memory); //detach process from memory segment
    shmctl(shmid, IPC_RMID, nullptr); //delete shared memory segment completely

    return 0;
}

void process_input_file(const string &input_file, int *A, int n) {
    ifstream input_file_stream(input_file);
    for (int i = 0; i < n; i++){
        input_file_stream >> A[i];
    }
    input_file_stream.close();
}

void write_output_file(const string &output_file, int *B, int n) {
    ofstream output_file_steam(output_file);
    for (int i = 0; i < n; i++){
        output_file_steam << B[i] << endl;
    }
    output_file_steam.close();
}
void prefix_sum(SharedMemory *shared_memory, int pid){

}
