#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <mpi.h>
#include <stddef.h>
#include <math.h>

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
            //printf("Red: %d, Green: %d, Blue: %d\n", red, green, blue);
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
    fp = fopen("greyscale -mpi-v2-output.txt", "w");

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
    fp = fopen("greyscale -mpi-v2-output.txt", "w");

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
    int numProcs, rank, i, j, indexRow, indexCol, perProc, dimension, gridDimension, sqProcs, totalPixels, procFactor, pixelsPerGrid, pixelsPerRow;
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
    Pixel *tempReceived;
    Pixel *finalGrey;
    Pixel *masterArray;
    int noPixelsSend;
    int noPixelsRecv;

    double start = MPI_Wtime();

    if (rank == 0)
    {
        matrix = readFileTo2DMatrix(argv);
        //copy the matrix to sendPixel array.
        dimension = matrix->dimension;
        sqProcs = (int)sqrt(numProcs);
        gridDimension = dimension / sqProcs;
        totalPixels = dimension * dimension;
        perProc = totalPixels / numProcs;
        pixelsPerGrid = gridDimension * gridDimension;
        pixelsPerRow = gridDimension;
        int row, col;
        sendPixelArray = malloc(sizeof(Pixel) * totalPixels);
        masterArray = malloc(sizeof(Pixel) * perProc);

        //sending number of processes to each processor.
        for (int proc = 1; proc < numProcs; proc++)
            MPI_Send(&perProc, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);

        for (i = 0; i < totalPixels; i++)
        {
            sendPixelArray[i].red = matrix->pixels[i]->red;
            sendPixelArray[i].green = matrix->pixels[i]->green;
            sendPixelArray[i].blue = matrix->pixels[i]->blue;
            //printf("%d\n", i);
        }

        int count = 0;
        //master sends data.
        for (int i = 0; i < dimension; i++)
        {
            row = i / gridDimension;
            for (int j = 0; j < dimension; j++)
            {
                col = j / gridDimension;

                if (row != 0 || col != 0)
                {
                    // //printf("Row: %d, Col: %d \n", row, col);
                    //printf("Processor: %d, index %d: \n", row*sqProcs+col, i * dimension + j);
                    MPI_Send(&sendPixelArray[i * dimension + j], 1, mpi_pixel_type, row * sqProcs + col, 0, MPI_COMM_WORLD);
                }
                else
                {
                    int red = sendPixelArray[i * dimension + j].red;
                    int green = sendPixelArray[i * dimension + j].green;
                    int blue = sendPixelArray[i * dimension + j].blue;

                    // sendPixelArray[i * dimension + j].red = 0.21 * red + 0.71 * green + 0.07 * blue;
                    // sendPixelArray[i * dimension + j].green = 0.21 * red + 0.71 * green + 0.07 * blue;
                    // sendPixelArray[i * dimension + j].blue = 0.21 * red + 0.71 * green + 0.07 * blue;
                    masterArray[count].red = 0.21 * red + 0.71 * green + 0.07 * blue;
                    masterArray[count].green = 0.21 * red + 0.71 * green + 0.07 * blue;
                    masterArray[count].blue = 0.21 * red + 0.71 * green + 0.07 * blue;
                    count++;
                }
            }
        }

        //Receive from slaves.
        finalGrey = malloc(totalPixels * sizeof(Pixel));
        tempReceived = malloc(perProc * sizeof(Pixel));

        for (int i = 0; i < numProcs; i++)
        {
            if (i == 0)
            {
                // do nothing. Master already computed.
                tempReceived = masterArray;
            }
            else
            {
                MPI_Recv(tempReceived, perProc, mpi_pixel_type, i, 0, MPI_COMM_WORLD, &status);
            }
            row = (int)(i / sqProcs);
            col = (int)(i % sqProcs);
            int index = 0;
            for (int k = row * gridDimension; k < (row + 1) * gridDimension; k++)
            {
                for (int j = col * gridDimension; j < (col + 1) * gridDimension; j++)
                {
                    finalGrey[k * dimension + j] = tempReceived[index];
                    index++;
                }
            }
        }

        //write to file.
        writePixelArrayToFile(finalGrey, totalPixels);
        free(finalGrey);
        free(tempReceived);
    }

    else
    {
        //slaves receive.
        MPI_Recv(&perProc, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        receivedPixelArray = malloc(sizeof(Pixel) * perProc);
        Pixel *pixel;
        for (int i = 0; i < perProc; i++)
        {
            MPI_Recv(&receivedPixelArray[i], 1, mpi_pixel_type, 0, 0, MPI_COMM_WORLD, &status);
            //printf("%d: Proc %d has received a pixel: %d\n", i, rank, receivedPixelArray[i].red);
            //convert pixel to grayscale.
            int red = receivedPixelArray[i].red;
            int green = receivedPixelArray[i].green;
            int blue = receivedPixelArray[i].blue;

            receivedPixelArray[i].red = 0.21 * red + 0.71 * green + 0.07 * blue;
            receivedPixelArray[i].green = 0.21 * red + 0.71 * green + 0.07 * blue;
            receivedPixelArray[i].blue = 0.21 * red + 0.71 * green + 0.07 * blue;
            //printf("%d: Proc %d has converted a pixel: %d\n", i, rank, receivedPixelArray[i].red);
        }
        // for( int i = 0; i < perProc; i++){
        //     printf("Proc %d: %d: Converted pixel reds: %d\n", rank, i, receivedPixelArray[i].red);
        // }

        //send back after processing.
        MPI_Send(receivedPixelArray, perProc, mpi_pixel_type, 0, 0, MPI_COMM_WORLD);
        printf("Proc %d has sent back pixels\n", rank);
        free(receivedPixelArray);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double end = MPI_Wtime();
    printf("Time Elapsed: %f\n", (end - start) * 1000);

    MPI_Type_free(&mpi_pixel_type);
    MPI_Finalize();
    return 0;
}