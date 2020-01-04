#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define RGB 1
#define GS 2

#define DO_MESSAGES

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

typedef struct {
    int threadId;
    int threadCount;
    float scale;
    image *input;
    image *output;
}args;

void allocPicture(image *img) {
    if (img == NULL) {
        return;
    }
    int i;
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

void destroyPicture(image *img) {
    if (img == NULL) {
        return;
    }
    if (img->ct == RGB && img->picC != NULL) {
        free(img->picC);
    } else if (img->ct == GS && img->picGS != NULL) {
        free(img->picGS);
    }
}

void readFromImage(image *input, char *fileName) {
    int i, j;
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

double min(double a, double b) {
    if (a > b) {
        return b;        
    }
    return a;
}

void riibUp(image *input, image *output, int outputLineStart, int outputLineEnd) {
    if (output->height == 0 || output->width == 0) return;
    int i, j;

    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    long x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    int xr, yr;

    int indexInputBase, indexInput, indexOutput = outputLineStart * output->width;
    long x, y = y_ratio * outputLineStart;

    if (output->ct == GS) {
        pixelGS TL, TR, BL, BR;
        for (i = outputLineStart; i < outputLineEnd; ++i) {
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

                TL = input->picGS[indexInput];
                TR = input->picGS[indexInput + 1];
                BL = input->picGS[indexInput + input->width];
                BR = input->picGS[indexInput + input->width + 1];

                output->picGS[indexOutput] = (pixelGS)((
                    one_min_x_diff * one_min_y_diff * (long)BL
                    + x_diff * one_min_y_diff * (long)BR
                    + y_diff * one_min_x_diff * (long)TL
                    + x_diff * y_diff * (long)TR) >> 32);

                x += x_ratio;
                ++indexOutput;
            }
            y += y_ratio;
        }
    } else if (output->ct == RGB) {
        pixelRGB TL, TR, BL, BR;
        for (i = outputLineStart; i < outputLineEnd; ++i) {
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
                ++indexOutput;
            }
            y += y_ratio;
        }
    }
}

void riibDown(image *input, image *output, int outputLineStart, int outputLineEnd) {
    if (output->height == 0 || output->width == 0) return;
    int i, j;

    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    long x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    int xr, yr;

    int indexInputBase, indexInput, indexOutput = outputLineStart * output->width;
    long x, y = y_ratio * outputLineStart;

    if (output->ct == GS) {
        for (i = outputLineStart; i < outputLineEnd; ++i) {
            x = x_ratio >> 1;
            yr = y >> 16;
            indexInputBase = yr * input->width;
            for (j = 0; j < output->width; ++j) {
                xr = x >> 16;

                indexInput = indexInputBase + xr;

                output->picGS[indexOutput] = (pixelGS)(((int)input->picGS[indexInput]
                    + (int)input->picGS[indexInput + input->width]
                    + (int)input->picGS[indexInput + 1]
                    + (int)input->picGS[indexInput + input->width + 1]) >> 2);

                x += x_ratio;
                ++indexOutput;
            }
            y += y_ratio;
        }
    } else if (output->ct == RGB) {
        pixelRGB TL, TR, BL, BR;
        for (i = outputLineStart; i < outputLineEnd; ++i) {
            x = x_ratio >> 1;
            yr = y >> 16;
            indexInputBase = yr * input->width;
            for (j = 0; j < output->width; ++j) {
                xr = x >> 16;

                indexInput = indexInputBase + xr;

                TL = input->picC[indexInput];
                TR = input->picC[indexInput + 1];
                BL = input->picC[indexInput + input->width];
                BR = input->picC[indexInput + input->width + 1];

                output->picC[indexOutput].R = (unsigned char)(((int)TL.R
                    + (int)TR.R
                    + (int)BL.R
                    + (int)BR.R) >> 2);

                output->picC[indexOutput].G = (unsigned char)(((int)TL.G
                    + (int)TR.G
                    + (int)BL.G
                    + (int)BR.G) >> 2);

                output->picC[indexOutput].B = (unsigned char)(((int)TL.B
                    + (int)TR.B
                    + (int)BL.B
                    + (int)BR.B) >> 2);

                x += x_ratio;
                ++indexOutput;
            }
            y += y_ratio;
        }
    }
}

void *threadFunction(void *arguments) {
    args *argStruct = (args *)arguments;
    int outputLineStart;
    int outputLineEnd;

    outputLineStart = argStruct->threadId
                    * ceil((double)argStruct->output->height
                           / argStruct->threadCount);
    outputLineEnd = min((argStruct->threadId + 1) 
                       * ceil((double)argStruct->output->height
                             / argStruct->threadCount),
                        (double)argStruct->output->height);

    if (argStruct->scale > 1) {
        riibUp(argStruct->input, argStruct->output, outputLineStart, outputLineEnd);
    } else if (argStruct->scale < 1 && argStruct->scale > 0) {
        riibDown(argStruct->input, argStruct->output, outputLineStart, outputLineEnd);
    }
}

/*
 * argv[1] - input
 * argv[2] - output
 * argv[3] - scale
 * argv[4] - thread count
 */

int main(int argc, char *argv[]) {
    int threadCount = atoi(argv[4]);
    image input, output;
    float scale = atof(argv[3]);
    int i, j;

    readFromImage(&input, argv[1]);

    if (scale == 1) {
        writeToImage(&input, argv[2]);
        return 0;
    }

    float newWidth = scale * input.width;
    float newHeight = scale * input.height;

    output.width = (int)newWidth;
    output.height = (int)newHeight;
    output.maxval = input.maxval;
    output.ct = input.ct;

    allocPicture(&output);
    if (threadCount == 1) {
        // run serial
        if (scale > 1) {
            riibUp(&input, &output, 0, output.height);
        } else {
            riibDown(&input, &output, 0, output.height);
        }
    } else {
        pthread_t threads[threadCount];
        args argums[threadCount];
        for (i = 0; i < threadCount; ++i) {
            argums[i].threadId = i;
            argums[i].threadCount = threadCount;
            argums[i].input = &input;
            argums[i].output = &output;
            argums[i].scale = scale;
        }

        for (i = 0; i < threadCount; ++i) {
            pthread_create(&(threads[i]), NULL, threadFunction, &(argums[i]));
        }

        for (i = 0; i < threadCount; ++i) {
            pthread_join(threads[i], NULL);
        }
    }

    writeToImage(&output, argv[2]);

    destroyPicture(&input);
    destroyPicture(&output);

    return 0;
}