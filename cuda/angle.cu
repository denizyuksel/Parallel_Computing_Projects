
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <limits.h>


#define M_PI 3.1415926535897
#define VECTOR_COUNT 2

cudaError_t computeElementsHelper(int* a, int* b, int* lengthNoSqrt, int* dotProduct, int N, int blockSize);

__global__ void computeElementsKernel(int* lengthNoSqrt, int* product, int* a, int* b, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < N) {
        //printf("Doing something with thread %d\n", i);
        //printf("Element: %d %d\n", a[i], b[i]);

        //find the dot product.
        atomicAdd(product, a[i] * b[i]);

        //printf("Sumsquares one before: %d\n", lengthNoSqrt[0]);
        //printf("Sumsquares two before: %d\n", lengthNoSqrt[1]);

        atomicAdd(&(lengthNoSqrt[0]), a[i] * a[i]);
        atomicAdd(&(lengthNoSqrt[1]), b[i] * b[i]);


        //printf("Sumsquares one after: %d\n", lengthNoSqrt[0]);
        //printf("Sumsquares two after: %d\n", lengthNoSqrt[1]);

    }

}

__global__ void addKernel(int *c, const int *a, const int *b)
{
    int i = threadIdx.x;
    c[i] = a[i] + b[i];
}

int* genVector(int N) {
    int* vector = (int*)malloc(sizeof(int) * N);

    for (int i = 0; i < N; i++) {
        int randNum = rand() % 20 - 10;
        vector[i] = randNum;
    }
    return vector;
} 

int findDotProduct(int* a, int* b, int N) {
    int sum = 0;
    for (int i = 0; i < N; i++) {
        sum = sum + (a[i] * b[i]);
    }
    return sum;
}

void printArray(int* x, int size) {
    for (int i = 0; i < size; i++) {
        printf("arr[%d] = %d\n", i, x[i]);
    }
}

double findVectorLength(int* x, int N) {
    int sumSquares = 0;
    for (int i = 0; i < N; i++) {
        sumSquares = sumSquares + pow(x[i], 2);
    }
    //printf("SumSquares serial: %d\n", sumSquares);
    double distance = sqrt(sumSquares);
    return distance;
    
}

double convertToDegrees(double rad) {
   return rad * (180 / M_PI);
}

void printDeviceProperties() {
    printf("--------------------DEVICE PROPERTIES----------------------\n\n");

    int nDevices;

    cudaGetDeviceCount(&nDevices);
    for (int i = 0; i < nDevices; i++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        printf("Device Number: %d\n", i);
        printf("Device name: %s\n", prop.name);
        printf("Memory Clock Rate (KHz): %d\n",
            prop.memoryClockRate);
        printf("Memory Bus Width (bits): %d\n",
            prop.memoryBusWidth);
        printf("Peak Memory Bandwidth (GB/s): %f\n\n",
            2.0 * prop.memoryClockRate * (prop.memoryBusWidth / 8) / 1.0e6);
    }

}

double doTheSerialThing(int* vectorOne, int* vectorTwo, int SIZE) {
    //printf("-----------------SERIAL IMPLEMENTATION----------------------\n\n");
    double dotProduct = (double)findDotProduct(vectorOne, vectorTwo, SIZE);
    double vectorLengthOne = findVectorLength(vectorOne, SIZE);
    double vectorLengthTwo = findVectorLength(vectorTwo, SIZE);

    double cosTheta = dotProduct / (vectorLengthOne * vectorLengthTwo);
    double angleInRadians = acos(cosTheta);
    double angleInDegrees = convertToDegrees(angleInRadians);
    //printf("length one: %f\n", vectorLengthOne);
    //printf("length two: %f\n", vectorLengthTwo);

    //printf("Angle in radians: %f\n", angleInRadians);

    //printArray(vectorOne, SIZE);
    //printArray(vectorTwo, SIZE);

    //printf("DOT PRODUCT SERIAL: %f\n", dotProduct);
    return angleInDegrees;
}

int main(int argc, char** argv)
{
    //Before beginning, print device properties.
    //printDeviceProperties();

    srand(time(NULL));
    
    clock_t start, end;
    double cpu_time_used;

    int SIZE = atoi(argv[1]);
    int BLOCK_SIZE = atoi(argv[2]);
    int* vectorOne = NULL;
    int* vectorTwo = NULL;
    int lengthsNoSqrt[VECTOR_COUNT] = { 0 };
    int dotProduct[1] = { 0 };
    double angleSerial = 0;

    int numberBlocks = 0;

    if (SIZE % BLOCK_SIZE == 0)
        numberBlocks = SIZE / BLOCK_SIZE;
    else
        numberBlocks = (SIZE / BLOCK_SIZE) + 1;

    printf("Info\n------------------\n");
    printf("Number of elements: %d\n", SIZE);
    printf("Number of threads per block: %d\n", BLOCK_SIZE);
    printf("Number of blocks will be created: %d\n\n", numberBlocks);
   


    //arrays will be generated
    if (argc == 3) {
        printf("Time\n------------------\n");

        start = clock();              
        vectorOne = genVector(SIZE);
        vectorTwo = genVector(SIZE);
        end = clock();
        cpu_time_used = ((double)(end - start) * 1000) / CLOCKS_PER_SEC;

        printf("Time for the array generation : %f ms\n", cpu_time_used);
    }

    //arrays will be read from file.
    else if (argc == 4) {

        char const* const fileName = argv[3]; /* should check that argc > 1 */
        FILE* file = fopen(fileName, "r"); /* should check the result */
        char line[256];

        fgets(line, sizeof(line), file);
        int count = atoi(line);
        int* allArray = (int*)malloc(sizeof(int) * count * 2);
        vectorOne = (int*)malloc(sizeof(int) * count);
        vectorTwo = (int*)malloc(sizeof(int) * count);

        int i = 0;
        //printf("COUNT: %d\n", count);
        while (fgets(line, sizeof(line), file)) {
            /* note that fgets don't strip the terminating \n, checking its
               presence would allow to handle lines longer that sizeof(line) */
            int number = atoi(line);
            allArray[i] = number;
            i++;
        }
        /* may check feof here to make a difference between eof and io failure -- network
           timeout for instance */

        /*
        for (int i = 0; i < count; i++) {
            printf("allArray[%d] = %d\n", i, allArray[i]);
        }
        */
        
        for (int i = 0; i < count; i++) {
            vectorOne[i] = allArray[i];
        }
        for (int i = count; i < count * 2; i++) {
            vectorTwo[i - count] = allArray[i];
        }     
        fclose(file);
    }

    else {
        printf("GIVE APPROPRIATE NUMBER OF ARGUMENTS PLEASE!!!\n");
        return 0;
    }



    start = clock();
    angleSerial = doTheSerialThing(vectorOne, vectorTwo, SIZE);
    end = clock();
    cpu_time_used = ((double)(end - start) * 1000) / CLOCKS_PER_SEC;

    printf("Time for the CPU function: %f ms\n", cpu_time_used);

    //printf("---------------------PARALLEL IMPLEMENTATION-----------------\n\n");

    // Calculate angle with CUDA.
    cudaError_t cudaStatus = computeElementsHelper(vectorOne, vectorTwo, lengthsNoSqrt, dotProduct, SIZE, BLOCK_SIZE);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "computeElements failed!");
        return 1;
    }

    // find the angle here.
    double lenOne = sqrt( (double) lengthsNoSqrt[0]);
    double lenTwo = sqrt( (double) lengthsNoSqrt[1]);
    double cosTheta = (  ((double) (dotProduct[0])) / (lenOne * lenTwo));
    double angleInRadians = acos(cosTheta);
    double angle = convertToDegrees(angleInRadians);

    printf("\n");

    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }

    printf("Results\n-----------------\n");
    printf("CPU Result: %0.3f\n", angleSerial);
    printf("GPU Result: %0.3f\n", angle);

 
    return 0;
}


cudaError_t computeElementsHelper(int* a, int* b, int* lengthNoSqrt, int* dotProduct, int N, int blockSize)
{
    int* dev_a = 0;
    int* dev_b = 0;
    int* dev_lengthNoSqrt = 0;
    int* dev_product = 0;
    cudaError_t cudaStatus;

    clock_t start, end;
    double timeUsed;
    double totalGpuTime = 0;

    // Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?\n");
    }

    cudaStatus = cudaMalloc((void**)&dev_a, N * sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc dev a failed!\n");
    }

    cudaStatus = cudaMalloc((void**)&dev_b, N * sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc dev b failed!\n");
    }

    cudaStatus = cudaMalloc((void**)&dev_lengthNoSqrt, VECTOR_COUNT * sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc dev length failed!\n");
    }

    cudaStatus = cudaMalloc((void**)&dev_product, sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc dev product failed!\n");
    }

    // Copy input vectors from host memory to GPU buffers.

    start = clock();

    cudaStatus = cudaMemcpy(dev_a, a, N * sizeof(int), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!\n");
    }

    cudaStatus = cudaMemcpy(dev_b, b, N * sizeof(int), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy failed!\n");
    }

    end = clock();
    timeUsed = ((double)(end - start) * 1000) / CLOCKS_PER_SEC;
    totalGpuTime += timeUsed;

    printf("Time for the Host to Device transfer: %f ms\n", timeUsed);
    

    // Launch a kernel on the GPU with one thread for each element.
    int numberBlocks = 0;

    if (N % blockSize == 0)
        numberBlocks = N / blockSize;
    else
        numberBlocks = (N / blockSize) + 1;

    start = clock();

    computeElementsKernel <<< numberBlocks, blockSize >>> (dev_lengthNoSqrt, dev_product, dev_a, dev_b, N);

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "computeElementsKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
    }

    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
    }    

    end = clock();
    timeUsed = ((double)(end - start) * 1000) / CLOCKS_PER_SEC;
    totalGpuTime += timeUsed;

    printf("Time for the kernel execution: %f ms\n", timeUsed);

    start = clock();

    cudaStatus = cudaMemcpy(lengthNoSqrt, dev_lengthNoSqrt, VECTOR_COUNT * sizeof(int), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy for dev_lengths failed!\n");
    }

    cudaStatus = cudaMemcpy(dotProduct, dev_product, sizeof(int), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy for dotProduct failed!\n");
    }

    end = clock();
    timeUsed = ((double)(end - start) * 1000) / CLOCKS_PER_SEC;
    printf("Time for the Device to Host transfer: %f ms\n", timeUsed);
    totalGpuTime += timeUsed;

    printf("Total execution time for GPU: %f ms\n", totalGpuTime);

    cudaFree(dev_product);
    cudaFree(dev_lengthNoSqrt);
    cudaFree(dev_a);
    cudaFree(dev_b);

    return cudaStatus;
}