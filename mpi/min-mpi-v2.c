#include <stdio.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int max_array_size = 10;

int findMin(int *arr, int size)
{
    int min = INT_MAX;
    int i;
    for (i = 0; i < size; i++)
    {
        if (arr[i] < min)
        {
            min = arr[i];
        }
    }
    return min;
}

void printArray(int *arr, int size)
{
    printf("Printing array.\n");
    int i = 0;
    for (i = 0; i < size; i++)
    {
        printf("%d %d \n", i, arr[i]);
    }
}

int **readFileToArray(int **numbersRef, char ***argv, int *work)
{
    FILE *stream;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    *work = 0;
    *numbersRef = (int *)malloc(max_array_size * sizeof(int));

    stream = fopen((*argv)[1], "r");
    if (stream == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, stream)) != -1)
    {
        if (*work >= max_array_size)
        {
            max_array_size = max_array_size * 2;
            *numbersRef = (int *)realloc(*numbersRef, sizeof(int) * max_array_size);
        }
        (*numbersRef)[*work] = atoi(line);
        *work = *work + 1;
    }
    free(line);
    fclose(stream);
    return numbersRef;
}

int main(int argc, char **argv)
{
    MPI_Status status;
    int numProcs, rank, i, j, index, countElementsReceived, totalWork, countPerWorker, min;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int *numbers;     // the number array.
    int *workerArray; // worker array.
    //printf("NUM PROCS: %d\n", numProcs);

    //double start = MPI_Wtime();
    if (numProcs > 1)
    {
        if (rank == 0)
        {
            readFileToArray(&numbers, &argv, &totalWork);

            countPerWorker = totalWork / numProcs;
            //printf("Total work: %d\n", totalWork);
            //printf("Worker count: %d\n", numProcs);

            for (i = 1; i < numProcs - 1; i++) // send to all except myself.
            {
                index = i * countPerWorker;
                //printf("Sending from index I GUESS: %d\n", index);
                MPI_Send(&countPerWorker, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send(&numbers[index], countPerWorker, MPI_INT, i, 0, MPI_COMM_WORLD);
                //MPI_Send(&totalWork, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }

            //sending last elements left...
            index = i * countPerWorker;
            int elements_left = totalWork - index;
            //int lastPackage = elements_left + index;
            //printf("Sending upto index with remaining %d elements: %d\n", elements_left, lastPackage);
            //printf("Sending elements left: %d\n", elements_left);
            MPI_Send(&elements_left, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&numbers[index], elements_left, MPI_INT, i, 0, MPI_COMM_WORLD);

            // master works...
            min = findMin(numbers, countPerWorker);
            // collecting mins from workers...
            int tmp;
            for (i = 1; i < numProcs; i++)
            {
                MPI_Recv(&tmp, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                int sender = status.MPI_SOURCE;
                //printf("sender id: %d\n", sender);
                if (tmp < min)
                {
                    min = tmp;
                }
            }
            //printf("Min is %d\n", min);
            free(numbers);
        }
        else
        {
            // receive total number of elements to allocate.
            MPI_Recv(&countElementsReceived, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            int *tempWorkerArray = (int *)malloc(countElementsReceived * sizeof(int));

            // receive the array to be processed.
            MPI_Recv(&tempWorkerArray[0], countElementsReceived, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

            int minLocal = findMin(tempWorkerArray, countElementsReceived);

            free(tempWorkerArray);
            // send local min to master.
            //printf("LOCAL MIN IS: %d\n", minLocal);
            MPI_Send(&minLocal, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    else if (numProcs == 1)
    {
        readFileToArray(&numbers, &argv, &totalWork);
        min = findMin(numbers, totalWork);
    }
    else
    {
        printf("%s", "DO NOT RUN WITH 0 PROCESSORS!");
    }

    MPI_Barrier(MPI_COMM_WORLD);
    //everybody waits until here...
    MPI_Bcast(&min, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // MPI_Barrier(MPI_COMM_WORLD);
    // double end = MPI_Wtime();

    // printf("Time Elapsed: %f\n", (end - start) * 1000);

    printf("Broadcasted min is: %d\n", min);

    MPI_Finalize();
    return 0;
}
