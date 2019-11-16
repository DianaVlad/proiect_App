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
    pixelRGB** picC;
    pixelGS** picGS;
}image;

void allocPicture(image *img) {
    int i;
    if (img->ct == RGB) {
        img->picC = malloc(sizeof(pixelRGB *) * img->height);
        if (img->picC == NULL) {
            exit(1);
        }
        for (i = 0; i < img->height; i++) {
            img->picC[i] = malloc(sizeof(pixelRGB) * img->width);
            if (img->picC[i] == NULL) {
                exit(1);
            }
        }
    } else if (img->ct == GS) {
        img->picGS = malloc(sizeof(pixelGS *) * img->height);
        if (img->picGS == NULL) {
            exit(1);
        }
        for (i = 0; i < img->height; i++) {
            img->picGS[i] = malloc(sizeof(pixelGS) * img->width);
            if (img->picGS[i] == NULL) {
                exit(1);
            }
        }
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
        for (i = 0; i < input->height; i++) {
            fread(input->picC[i], 1, sizeof(pixelRGB) * input->width, file);
        }
    } else {                // GRAYSCALE
        for (i = 0; i < input->height; i++) {
            fread(input->picGS[i], 1, sizeof(pixelGS) * input->width, file);
        }
    }
    fclose(file);
}

void writeToImage(image *img, char *fileName) {
    FILE *file = fopen(fileName, "wb");
    int i;
    if (img->ct == RGB) {
        fprintf(file, "P6\n%d %d\n%d\n",
            img->width, img->height, img->maxval);
        for (i = 0; i < img->height; i++) {
            fwrite(img->picC[i], 1, sizeof(pixelRGB) * img->width, file);
        }
    } else {
        fprintf(file, "P5\n%d %d\n%d\n",
            img->width, img->height, img->maxval);
        for (i = 0; i < img->height; i++) {
            fwrite(img->picGS[i], 1, sizeof(pixelGS) * img->width, file);
        }
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

                TL = input->picGS[y    ][x    ];
                TR = input->picGS[y    ][x + 1];
                BL = input->picGS[y + 1][x    ];
                BR = input->picGS[y + 1][x + 1];

                output->picGS[i][j] = (pixelGS)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL
                    + x_diff * (1.0f - y_diff) * (float)BR
                    + y_diff * (1.0f - x_diff) * (float)TL
                    + x_diff * y_diff * (float)TR);
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

                TL = input->picC[y    ][x    ];
                TR = input->picC[y    ][x + 1];
                BL = input->picC[y + 1][x    ];
                BR = input->picC[y + 1][x + 1];

                output->picC[i][j].R = (unsigned char)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.R + x_diff * (1.0f - y_diff) * (float)BR.R
                    + y_diff * (1.0f - x_diff) * (float)TL.R + x_diff * y_diff * (float)TR.R
                    );

                output->picC[i][j].G = (unsigned char)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.G + x_diff * (1.0f - y_diff) * (float)BR.G
                    + y_diff * (1.0f - x_diff) * (float)TL.G + x_diff * y_diff * (float)TR.G
                    );

                output->picC[i][j].B = (unsigned char)(
                    (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.B + x_diff * (1.0f - y_diff) * (float)BR.B
                    + y_diff * (1.0f - x_diff) * (float)TL.B + x_diff * y_diff * (float)TR.B
                    );
            }
        }
    }
}

void riibDown(image *input, image *output) {
    int i, j;


    float y_ratio = ((float)(input->height - 1)) / output->height;
    float x_ratio = ((float)(input->width - 1)) / output->width;
    int xr, yr;
    float x, y;

    if (output->ct == GS) {
        for (i = 0; i < output->height; ++i) {
            x = x_ratio / 2;
            yr = (int)y;
            for (j = 0; j < output->width; ++j) {
                xr = (int)x;

                output->picGS[i][j] = (pixelGS)(((int)input->picGS[yr][xr]
                    + (int)input->picGS[yr + 1][xr]
                    + (int)input->picGS[yr][xr + 1]
                    + (int)input->picGS[yr + 1][xr + 1]) >> 2);

                x += x_ratio;
            }
            y += y_ratio;
        }
    } else if (output->ct == RGB) {
        for (i = 0; i < output->height; ++i) {
            x = x_ratio / 2;
            yr = (int)y;
            for (j = 0; j < output->width; ++j) {
                xr = (int)x;

                output->picC[i][j].R = (unsigned char)(((int)input->picC[yr][xr].R
                    + (int)input->picC[yr + 1][xr].R
                    + (int)input->picC[yr][xr + 1].R
                    + (int)input->picC[yr + 1][xr + 1].R) >> 2);

                output->picC[i][j].G = (unsigned char)(((int)input->picC[yr][xr].G
                    + (int)input->picC[yr + 1][xr].G
                    + (int)input->picC[yr][xr + 1].G
                    + (int)input->picC[yr + 1][xr + 1].G) >> 2);

                output->picC[i][j].B = (unsigned char)(((int)input->picC[yr][xr].B
                    + (int)input->picC[yr + 1][xr].B
                    + (int)input->picC[yr][xr + 1].B
                    + (int)input->picC[yr + 1][xr + 1].B) >> 2);

                x += x_ratio;
            }
            y += y_ratio;
        }
    }
}

int main(int argc, char *argv[]) {
    image input;
    image output;
    int i, j, k;
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
        for (i = 0; i < input.height; i++) {
            free(input.picC[i]);
        }
        free(input.picC);
        for (i = 0; i < output.height; i++) {
            free(output.picC[i]);
        }
        free(output.picC);
    } else {
        for (i = 0; i < input.height; i++) {
            free(input.picGS[i]);
        }
        free(input.picGS);
        for (i = 0; i < output.height; i++) {
            free(output.picGS[i]);
        }
        free(output.picGS);
    }
    return 0;
}