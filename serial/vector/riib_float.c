#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int x, y;
    float y_ratio = ((float)(input->height - 1)) / output->height;
    float x_ratio = ((float)(input->width - 1)) / output->width;
    float x_diff, y_diff, ya, yb;
    float xr, yr;

    int indexInput, indexOutput = 0;

    if (output->ct == GS) {
        pixelGS TL, TR, BL, BR;
        for (i = 0; i < output->height; ++i) {
            for (j = 0; j < output->width; ++j) {
                xr = x_ratio * j;
                yr = y_ratio * i;

                x = (int)xr;
                y = (int)yr;
                x_diff = xr - x;
                y_diff = yr - y;

                indexInput = y * input->width + x;

                output->picGS[indexOutput] = (pixelGS)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)input->picGS[indexInput + input->width]
                    + x_diff * (1.0f - y_diff) * (float)input->picGS[indexInput + input->width + 1]
                    + y_diff * (1.0f - x_diff) * (float)input->picGS[indexInput]
                    + x_diff * y_diff * (float)input->picGS[indexInput + 1]);
                ++indexOutput;
            }
        }
    } else if (output->ct == RGB) {
        pixelRGB TL, TR, BL, BR;
        for (i = 0; i < output->height; ++i) {
            for (j = 0; j < output->width; ++j) {
                xr = x_ratio * j;
                yr = y_ratio * i;

                x = (int)xr;
                y = (int)yr;
                x_diff = xr - x;
                y_diff = yr - y;

                indexInput = y * input->width + x;

                TL = input->picC[indexInput];
                TR = input->picC[indexInput + 1];
                BL = input->picC[indexInput + input->width];
                BR = input->picC[indexInput + input->width + 1];

                output->picC[indexOutput].R = (unsigned char)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.R + x_diff * (1.0f - y_diff) * (float)BR.R
                    + y_diff * (1.0f - x_diff) * (float)TL.R + x_diff * y_diff * (float)TR.R
                    );

                output->picC[indexOutput].G = (unsigned char)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.G + x_diff * (1.0f - y_diff) * (float)BR.G
                    + y_diff * (1.0f - x_diff) * (float)TL.G + x_diff * y_diff * (float)TR.G
                    );

                output->picC[indexOutput].B = (unsigned char)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.B + x_diff * (1.0f - y_diff) * (float)BR.B
                    + y_diff * (1.0f - x_diff) * (float)TL.B + x_diff * y_diff * (float)TR.B
                    );
                ++indexOutput;
            }
        }
    }
}

void riibDown(image *input, image *output) {
    int i, j;

    float y_ratio = ((float)(input->height - 1)) / output->height;
    float x_ratio = ((float)(input->width - 1)) / output->width;
    float x, y = y_ratio / 2;
    int xr, yr;
    int indexInputBase = 0, indexOutput = 0, indexInput;

    if (output->ct == GS) {
        for (i = 0; i < output->height; ++i) {
            x = x_ratio / 2;
            yr = (int)y;
            indexInputBase = yr * input->width;
            for (j = 0; j < output->width; ++j) {
                xr = (int)x;

                indexInput = indexInputBase + x;

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
        for (i = 0; i < output->height; ++i) {
            x = x_ratio / 2;
            yr = (int)y;
            indexInputBase = yr * input->width;
            for (j = 0; j < output->width; ++j) {
                xr = (int)x;

                indexInput = indexInputBase + x;

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

int main(int argc, char *argv[]) {
    image input;
    image output;
    readFromImage(&input, argv[1]);
    output.ct = input.ct;

    float scale = atof(argv[3]);
    float newWidth = (float)input.width * scale;
    float newHeight = (float)input.height * scale;
    output.width = (int)newWidth;
    output.height = (int)newHeight;
    output.maxval = input.maxval;
    if (scale > 1) {
        allocPicture(&output);
        riibUp(&input, &output);
    } else if (scale < 1 && scale > 0) {
        allocPicture(&output);
        riibDown(&input, &output);
    }

    writeToImage(&output, argv[2]);

    if (input.ct == RGB) {
        free(input.picC);
        free(output.picC);
    } else {
        free(input.picGS);
        free(output.picGS);
    }
    return 0;
}