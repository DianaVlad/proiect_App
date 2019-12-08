#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include <math.h>

#define RGB 1
#define GS 2

typedef unsigned char pixelGS;

typedef struct {
    unsigned char R;
    unsigned char G;
    unsigned char B;
}pixelRGB;

typedef struct {
    int ct;
    int height;
    int width;
    int maxval;
    pixelRGB* picC;
    pixelGS* picGS;
}image;

int num_threads;
int CHUNK = 10;
image input, output;
float scale;

void allocPicture(image *img) {
    if (img->ct == RGB) {
        img->picC = malloc(sizeof(pixelRGB *) * img->height * img->width);
        if (img->picC == NULL) {
            exit(1);
        }
    } else if (img->ct == GS) {
        img->picGS = malloc(sizeof(pixelGS *) * img->height * img->width);
        if (img->picGS == NULL) {
            exit(1);
        }
    }
}

void readFromImage(image *input, char *fileName) {
    FILE *file = fopen(fileName, "rb");
    char typeC[3];
    fscanf(file, "%s\n%d %d\n%d\n",
        typeC, &(input->width), &(input->height), &(input->maxval));
    if (typeC[1] == '6') {
        input->ct = RGB;
    } else if (typeC[1] == '5') {
        input->ct = GS;
    } else {
        exit(1);
    }

    allocPicture(input);
    if (input->ct == RGB) { // RGB
        fread(input->picC, 1, sizeof(pixelRGB) * input->width * input->height, file);
    } else {                // GRAYSCALE
        fread(input->picGS, 1, sizeof(pixelGS) * input->width * input->height, file);
    }
    fclose(file);
}

void writeToImage(image *img, char *fileName) {
    FILE *file = fopen(fileName, "wb");
    if (img->ct == RGB) {
        fprintf(file, "P6\n%d %d\n%d\n",
            img->width, img->height, img->maxval);
        fwrite(img->picC, 1, sizeof(pixelRGB) * img->width * img->height, file);
    } else {
        fprintf(file, "P5\n%d %d\n%d\n",
            img->width, img->height, img->maxval);
        fwrite(img->picGS, 1, sizeof(pixelGS) * img->width * img->height, file);
    }
    fclose(file);
}

void riibUp(image *input, image *output) {
    int i, j;

    long x, y = 0;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    long x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    int xr, yr;

    int indexInputBase, indexInput, indexOutput = 0;

    pixelRGB TL, TR, BL, BR;

    omp_set_num_threads(num_threads);

    #pragma omp parallel for \
    shared(input, output,  x_ratio, y_ratio, num_threads) \
    private(i, j, xr, yr, x, y, indexInput, indexOutput, indexInputBase, TL, TR, BL, BR, x_diff, y_diff, one_min_y_diff, one_min_x_diff) \
    schedule(dynamic, CHUNK)
    for (i = 0; i < output->height; ++i) {
        yr = (int) (y >> 16);
        y_diff = y - ((long)yr << 16);
        one_min_y_diff = 65536 - y_diff;
        x = 0;
        indexInputBase = yr * input->width;
        for (j = 0; j < output->width; ++j) {
            xr = (int) (x >> 16);
            x_diff = x - ((long)xr << 16);
            one_min_x_diff = 65536 - x_diff;

            indexInput = indexInputBase + xr;

            TL = input->picC[indexInput];
            TR = input->picC[indexInput + 1];
            BL = input->picC[indexInput + input->width];
            BR = input->picC[indexInput + input->width + 1];

            output->picC[indexOutput].R = (unsigned char)((
                one_min_x_diff * one_min_y_diff * BL.R + x_diff * one_min_y_diff * BR.R
                + y_diff * one_min_x_diff * TL.R + x_diff * y_diff * TR.R
                ) >> 32);

            output->picC[indexOutput].G = (unsigned char)((
                one_min_x_diff * one_min_y_diff * BL.G + x_diff * one_min_y_diff * BR.G
                + y_diff * one_min_x_diff * TL.G + x_diff * y_diff * TR.G
                ) >> 32);

            output->picC[indexOutput].B = (unsigned char)((
                one_min_x_diff * one_min_y_diff * BL.B + x_diff * one_min_y_diff * BR.B
                + y_diff * one_min_x_diff * TL.B + x_diff * y_diff * TR.B
                ) >> 32);
            x += x_ratio;
            //printf("%d\n", indexOutput);
            ++indexOutput;
        }
        y += y_ratio;
    }
    
}

void riibDown() {
    int i, j;

    long x, y;
    int y_ratio = (((long)input.height - 1) << 16) / output.height;
    y = y_ratio >> 1;
    int x_ratio = (((long)input.width - 1) << 16) / output.width;
    int xr, yr;

    int indexInput, indexInputBase, indexOutput = 0;
    pixelRGB TL, TR, BL, BR;
    omp_set_num_threads(num_threads);
   
    #pragma omp parallel for \
    shared(input, output,  x_ratio, y_ratio) \
    private(i, j, xr, yr, x, y, indexInput, indexOutput, indexInputBase, TL, TR, BL, BR)
    for (i = 0; i < output.height; i++) {
        x = x_ratio >> 1;
        yr = y >> 16;
        indexInputBase = yr * input.width;
        for (j = 0; j < output.width; j++) {
            xr = x >> 16;

            indexInput = indexInputBase + xr;

            TL = input.picC[indexInput];
            TR = input.picC[indexInput + 1];
            BL = input.picC[indexInput + input.width];
            BR = input.picC[indexInput + input.width + 1];

            output.picC[indexOutput].R = (unsigned char)(((int)TL.R
                + (int)TR.R
                + (int)BL.R
                + (int)BR.R) >> 2);

            output.picC[indexOutput].G = (unsigned char)(((int)TL.G
                + (int)TR.G
                + (int)BL.G
                + (int)BR.G) >> 2);

            output.picC[indexOutput].B = (unsigned char)(((int)TL.B
                + (int)TR.B
                + (int)BL.B
                + (int)BR.B) >> 2);
            printf("%d\n", indexOutput);
            x += x_ratio;
            ++indexOutput;
        }
        y += y_ratio;
    }
    
}


int main(int argc, char *argv[]) {
    float time = 0;
    clock_t start;
    clock_t end;

    start = clock();
    
    readFromImage(&input, argv[1]);
    output.ct = input.ct;

    scale = atof(argv[3]);
    num_threads = atoi(argv[4]);


    float newWidth = (float)input.width * scale;
    float newHeight = (float)input.height * scale;
    output.width = (int)newWidth;
    output.height = (int)newHeight;
    output.maxval = input.maxval;
    printf("%d %d\n",output.height, output.width);
    CHUNK = output.height/ num_threads;
    printf("%d\n", CHUNK);
    if (scale > 1) {
        allocPicture(&output);
        riibUp(&input, &output);
    } else if (scale < 1 && scale > 0) {
        allocPicture(&output);
        riibDown();
    }

    writeToImage(&output, argv[2]);

    if (input.ct == RGB) {
        free(input.picC);
        free(output.picC);
    } else {
        free(input.picGS);
        free(output.picGS);
    }

    end = clock();
    time = (float)(end - start);
    printf("%f seconds\n", time/CLOCKS_PER_SEC);
    return 0;
}