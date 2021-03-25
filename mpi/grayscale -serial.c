#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>

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
        //printf("Pixels freed");
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

void convertToGrayScale(Matrix *matrix)
{
    int i;
    int j;
    int size = matrix->dimension * matrix->dimension;

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

void writeMatrixToFile(Matrix *matrix)
{
    int size = matrix->dimension * matrix->dimension;
    FILE *fp;
    int i;

    fp = fopen("greyscale -serial-output.txt", "w");

    /* write 10 lines of text into the file stream*/
    for (i = 0; i < size; i++)
    {
        fprintf(fp, "%d\n", matrix->pixels[i]->red);
    }

    /* close the file*/
    fclose(fp);
}

int main(int argc, char **argv)
{
    // clock_t start, end;
    // double cpu_time_used;
    // start = clock();
    Matrix *matrix = readFileTo2DMatrix(argv);
    
    convertToGrayScale(matrix);
    //printMatrix(matrix);
    writeMatrixToFile(matrix);
    printf("Written to file successfully\n");

    // end = clock();
    // cpu_time_used = (double)(((double)(end - start)) / CLOCKS_PER_SEC) * 1000;
    // printf("Time elapsed: %f\n", cpu_time_used);
    freeMatrix(matrix);

    return 0;
}