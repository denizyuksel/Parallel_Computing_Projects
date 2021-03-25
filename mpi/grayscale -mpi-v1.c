#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <mpi.h>
#include <stddef.h>

typedef struct Pixel
{
    int red;
    int green;
    int blue;
} Pixel;

typedef struct Matrix
{
    Pixel **pixels;
    int dimension;
} Matrix;

Pixel *getPixel(Matrix *matrixPtr, int i, int j)
{
    return matrixPtr->pixels[i * matrixPtr->dimension + j];
}

void printMatrix(Matrix *matrix)
{
    int i;
    int j;
    int size = matrix->dimension * matrix->dimension;

    /* USE 1D ARRAY REPRESENTATION
    for (i = 0; i < size; i++)
    {
        Pixel pixel = *(matrix->pixels[i]);
        int red = pixel.red;
        int green = pixel.green;
        int blue = pixel.blue;
        printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue);
    }
    */

    // USE 2D ARRAY REPRESENTATION
    for (i = 0; i < matrix->dimension; i++)
    {
        for (j = 0; j < matrix->dimension; j++)
        {
            Pixel *pixel = getPixel(matrix, i, j);
            int red = pixel->red;
            int green = pixel->green;
            int blue = pixel->blue;
            printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue);
        }
    }
}

void freeMatrix(Matrix *matrix)
{
    int i;
    int size = matrix->dimension * matrix->dimension;

    for (i = 0; i < size; i++)
    {
        free(matrix->pixels[i]);
    }
    free(matrix);
}

Matrix *readFileTo2DMatrix(char **argv)
{
    FILE *stream;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    stream = fopen(argv[1], "r");
    if (stream == NULL)
    {
        printf("%s", "INPUT VER LA");
        exit(EXIT_FAILURE);
    }
    Matrix *matrix = malloc(sizeof(Matrix));
    read = getline(&line, &len, stream);
    matrix->dimension = atoi(line); //gets the first line.
    matrix->pixels = malloc(matrix->dimension * matrix->dimension * sizeof(Pixel));

    int i = 0;
    while ((read = getline(&line, &len, stream)) != -1)
    {
        char *delim = ",";
        Pixel *p = malloc(sizeof(Pixel));
        p->red = atoi(strtok(line, delim));
        p->green = atoi(strtok(NULL, delim));
        p->blue = atoi(strtok(NULL, delim));
        matrix->pixels[i] = p;
        i++;
    }
    return matrix;
}

void convertMatrixToGrayScale(Matrix *matrix, int rowCount)
{
    int i;
    //int j;
    int size = rowCount * matrix->dimension;

    for (i = 0; i < size; i++)
    {
        Pixel *pixel = matrix->pixels[i];
        int red = pixel->red;
        int green = pixel->green;
        int blue = pixel->blue;
        pixel->red = 0.21 * red + 0.71 * green + 0.07 * blue;
        pixel->green = 0.21 * red + 0.71 * green + 0.07 * blue;
        pixel->blue = 0.21 * red + 0.71 * green + 0.07 * blue;
        //printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue);
    }
}

void convertPixelArrayToGrayScale(Pixel **pixelArr, int pixelCount, int index)
{
    int i;
    for (i = index; i < pixelCount; i++)
    {
        int red = (*pixelArr)[i].red;
        int green = (*pixelArr)[i].green;
        int blue = (*pixelArr)[i].blue;

        (*pixelArr)[i].red = 0.21 * red + 0.71 * green + 0.07 * blue;
        (*pixelArr)[i].green = 0.21 * red + 0.71 * green + 0.07 * blue;
        (*pixelArr)[i].blue = 0.21 * red + 0.71 * green + 0.07 * blue;
        //printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue);
    }
}

void writeMatrixToFile(Matrix *matrix, int startIndex, int rowsCount)
{
    //int size = matrix->dimension * matrix->dimension;
    FILE *fp;
    int i;
    int size = rowsCount * matrix->dimension;
    fp = fopen("greyscale -mpi-v1-output.txt", "w");

    for (i = startIndex; i < size; i++)
    {
        fprintf(fp, "%d\n", matrix->pixels[i]->red);
    }

    /* close the file*/
    fclose(fp);
}

void writePixelArrayToFile(Pixel *pixels, int pixelCount)
{
    //int size = matrix->dimension * matrix->dimension;
    FILE *fp;
    int i;
    fp = fopen("greyscale -mpi-v1-output.txt", "w");

    for (i = 0; i < pixelCount; i++)
    {
        fprintf(fp, "%d\n", pixels[i].red);
    }

    /* close the file*/
    fclose(fp);
}

int main(int argc, char **argv)
{
    MPI_Status status;
    int numProcs, rank, i, j, index, rowCountReceived, dimension, rowsPerWorker, dataAmount, totalReceived, totalRows, totalPixels, remaniningRows;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    int workArray[numProcs];

    // if (numProcs < 2)
    // {
    //     fprintf(stderr, "Requires at least two processes.\n");
    //     exit(-1);
    // }

    const int nitems = 3;
    int blocklengths[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_pixel_type;
    MPI_Aint offsets[3];

    offsets[0] = offsetof(Pixel, red);
    offsets[1] = offsetof(Pixel, green);
    offsets[2] = offsetof(Pixel, blue);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pixel_type);
    MPI_Type_commit(&mpi_pixel_type);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Matrix *matrix; // the matrix.
    Pixel *sendPixelArray;
    Pixel *receivedPixelArray;
    int noPixelsSend;
    int noPixelsRecv;

    //double start = MPI_Wtime();
    if (rank == 0)
    {
        matrix = readFileTo2DMatrix(argv);
        //copy the matrix to sendPixel array.
        dimension = matrix->dimension;
        totalRows = dimension;
        totalPixels = dimension * dimension;
        sendPixelArray = malloc(sizeof(Pixel) * totalPixels);
        for (i = 0; i < totalPixels; i++)
        {
            sendPixelArray[i].red = matrix->pixels[i]->red;
            sendPixelArray[i].green = matrix->pixels[i]->green;
            sendPixelArray[i].blue = matrix->pixels[i]->blue;
            //printf("%d\n", i);
        }

        //printf("Matrix dimension: %d\n", dimension);
        rowsPerWorker = totalPixels / numProcs;

        for (int i = 0; i < numProcs; i++)
        {
            workArray[i] = rowsPerWorker;
            // printf("Row for worker %d: %d\n", i, workArray[i]);
        }

        //distribute rows...
        remaniningRows = totalPixels % numProcs;
        while (remaniningRows > 0)
        {
            workArray[remaniningRows] += 1;
            remaniningRows--;
        }
        // for (int i = 0; i < numProcs; i++)
        // {
        //     workArray[i] = workArray[i] * dimension;
        // }

        // for (int i = 0; i < numProcs; i++)
        // {
        //     printf("Row for worker %d: %d\n", i, workArray[i]);
        // }

        index = workArray[0];
        for (i = 1; i < numProcs; i++) // send to all except myself.
        {
            //printf("Sending from index I GUESS: %d\n", index);
            MPI_Send(&workArray[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&sendPixelArray[index], workArray[i], mpi_pixel_type, i, 0, MPI_COMM_WORLD);
            //MPI_Send(&totalWork, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            //printf("Num rows send to proc %d: %d\n", i, workArray[i]);
            index = index + workArray[i];
        }

        // send to workers...

        //printf("Rank %d: pixel array sent\n", rank);

        index = 0;
        // master calculation...
        convertPixelArrayToGrayScale(&sendPixelArray, workArray[index], index);
        // for (int i = index; i < workArray[index]; i++)
        // {
        //     Pixel pixel = sendPixelArray[i];
        //     int red = pixel.red;
        //     int green = pixel.green;
        //     int blue = pixel.blue;
        //     //printf("ROW: %d Rank %d: Converted: red = %d green = %d blue = %d\n", i, rank, red, green, blue);
        //     //137409 rows
        // }

        //receive from workers...
        index = workArray[0];
        for (int i = 1; i < numProcs; i++)
        {
            MPI_Recv(&sendPixelArray[index], workArray[i], mpi_pixel_type, i, 0, MPI_COMM_WORLD, &status);
            convertPixelArrayToGrayScale(&sendPixelArray, workArray[i], index);
            index = index + workArray[i];
        }

        for (int i = 0; i < totalPixels; i++)
        {
            Pixel pixel = sendPixelArray[i];
            int red = pixel.red;
            int green = pixel.green;
            int blue = pixel.blue;
            //printf("ROW: %d Rank %d: Converted: red = %d green = %d blue = %d\n", i, rank, red, green, blue);
            //137409 rows
        }

        //write to file, finally.

        writePixelArrayToFile(sendPixelArray, totalPixels);

        //free send array.
        printf("Written to file successfully.\n");
        free(sendPixelArray);
    }
    else
    {
        MPI_Status status;
        const int src = 0;

        MPI_Recv(&noPixelsRecv, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        //printf("Pixel Received by proc %d: %d\n", rank, noPixelsRecv);

        receivedPixelArray = malloc(sizeof(Pixel) * noPixelsRecv);
        MPI_Recv(receivedPixelArray, noPixelsRecv, mpi_pixel_type, src, 0, MPI_COMM_WORLD, &status);
        for (int i = 0; i < noPixelsRecv; i++)
        {
            Pixel pixel = receivedPixelArray[i];
            int red = pixel.red;
            int green = pixel.green;
            int blue = pixel.blue;
            //printf("Rank %d: Received: red = %d green = %d blue = %d\n", rank, red, green, blue);
        }
        convertPixelArrayToGrayScale(&receivedPixelArray, noPixelsRecv, 0);
        MPI_Send(receivedPixelArray, noPixelsRecv, mpi_pixel_type, src, 0, MPI_COMM_WORLD);
        free(receivedPixelArray);
    }

    //MPI_Barrier(MPI_COMM_WORLD);
    
    // double end = MPI_Wtime();
    // printf("Time Elapsed: %f\n", (end - start) * 1000);


    MPI_Type_free(&mpi_pixel_type);
    MPI_Finalize();
    return 0;
}