#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

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
        printf("%d: %d \n", i, arr[i]);
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
    // clock_t start, end;
    // double cpu_time_used;
    // start = clock();
    int *numbers;
    int totalWork;
   
    readFileToArray(&numbers, &argv, &totalWork);
    //printArray(numbers, totalWork);
    int min = findMin(numbers, totalWork);
    // end = clock();
    // cpu_time_used = (double)(((double)(end - start)) / CLOCKS_PER_SEC) * 1000;
    // printf("Time elapsed: %f\n", cpu_time_used);

    printf("Min is %d\n", min);
    free(numbers);

    exit(EXIT_SUCCESS);
}