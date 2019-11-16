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

void riibUp(image *input, image *output) {
    int i, j;
    //int x, y;
    //float y_ratio = ((float)(input->height - 1)) / output->height;
    //float x_ratio = ((float)(input->width - 1)) / output->width;
    //float x_diff, y_diff, ya, yb;
    //float xr, yr;

    long x, y = 0;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    long x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    int xr, yr;

    if (output->ct == GS) {
        pixelGS TL, TR, BL, BR;
        for (i = 0; i < output->height; ++i) {
            yr = (int) (y >> 16);
            y_diff = y - ((long)yr << 16);
            one_min_y_diff = 65536 - y_diff;
            x = 0;
            for (j = 0; j < output->width; ++j) {
                // xr = x_ratio * j;
                // yr = y_ratio * i;

                // x = (int)xr;
                // y = (int)yr;
                // x_diff = xr - x;
                // y_diff = yr - y;

                xr = (int) (x >> 16);
                x_diff = x - ((long)xr << 16);
                one_min_x_diff = 65536 - x_diff;

                TL = input->picGS[yr    ][xr    ];
                TR = input->picGS[yr    ][xr + 1];
                BL = input->picGS[yr + 1][xr    ];
                BR = input->picGS[yr + 1][xr + 1];

                // output->picGS[i][j] = (pixelGS)(
                //     (1.0f - x_diff) * (1.0f - y_diff) * (float)BL
                //     + x_diff * (1.0f - y_diff) * (float)BR
                //     + y_diff * (1.0f - x_diff) * (float)TL
                //     + x_diff * y_diff * (float)TR);
                output->picGS[i][j] = (pixelGS)((
                    one_min_x_diff * one_min_y_diff * (long)BL
                    + x_diff * one_min_y_diff * (long)BR
                    + y_diff * one_min_x_diff * (long)TL
                    + x_diff * y_diff * (long)TR) >> 32);

                x += x_ratio;
            }
            y += y_ratio;
        }
    } else if (output->ct == RGB) {
        pixelRGB TL, TR, BL, BR;
        for (i = 0; i < output->height; ++i) {
            yr = (int) (y >> 16);
            y_diff = y - ((long)yr << 16);
            one_min_y_diff = 65536 - y_diff;
            x = 0;
            for (j = 0; j < output->width; ++j) {
                // xr = x_ratio * j;
                // yr = y_ratio * i;

                // x = (int)xr;
                // y = (int)yr;
                // x_diff = xr - x;
                // y_diff = yr - y;

                xr = (int) (x >> 16);
                x_diff = x - ((long)xr << 16);
                one_min_x_diff = 65536 - x_diff;

                TL = input->picC[yr    ][xr    ];
                TR = input->picC[yr    ][xr + 1];
                BL = input->picC[yr + 1][xr    ];
                BR = input->picC[yr + 1][xr + 1];

                // output->picC[i][j].R = (unsigned char)(
                //     (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.R + x_diff * (1.0f - y_diff) * (float)BR.R
                //     + y_diff * (1.0f - x_diff) * (float)TL.R + x_diff * y_diff * (float)TR.R
                //     );

                // output->picC[i][j].G = (unsigned char)(
                //     (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.G + x_diff * (1.0f - y_diff) * (float)BR.G
                //     + y_diff * (1.0f - x_diff) * (float)TL.G + x_diff * y_diff * (float)TR.G
                //     );

                // output->picC[i][j].B = (unsigned char)(
                //     (1.0f - x_diff) * (1.0f - y_diff) * (float)BL.B + x_diff * (1.0f - y_diff) * (float)BR.B
                //     + y_diff * (1.0f - x_diff) * (float)TL.B + x_diff * y_diff * (float)TR.B
                //     );

                output->picC[i][j].R = (unsigned char)((
                    one_min_x_diff * one_min_y_diff * BL.R + x_diff * one_min_y_diff * BR.R
                    + y_diff * one_min_x_diff * TL.R + x_diff * y_diff * TR.R
                    ) >> 32);

                output->picC[i][j].G = (unsigned char)((
                    one_min_x_diff * one_min_y_diff * BL.G + x_diff * one_min_y_diff * BR.G
                    + y_diff * one_min_x_diff * TL.G + x_diff * y_diff * TR.G
                    ) >> 32);

                output->picC[i][j].B = (unsigned char)((
                    one_min_x_diff * one_min_y_diff * BL.B + x_diff * one_min_y_diff * BR.B
                    + y_diff * one_min_x_diff * TL.B + x_diff * y_diff * TR.B
                    ) >> 32);
                x += x_ratio;
            }
            y += y_ratio;
        }
    }
}

void riibDown(image *input, image *output) {
    int i, j;

    long x, y;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    y = y_ratio >> 1;
    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    int xr, yr;

    if (output->ct == GS) {
        for (i = 0; i < output->height; ++i) {
            x = x_ratio >> 1;
            yr = y >> 16;
            for (j = 0; j < output->width; ++j) {
                xr = x >> 16;

                if (yr >= input->height) {
                    printf(".");
                }
                if (xr >= input->width) {
                    printf("`");
                }

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
            x = x_ratio >> 1;
            yr = y >> 16;
            for (j = 0; j < output->width; ++j) {
                xr = x >> 16;

                if (yr >= input->height) {
                    printf(".");
                }
                if (xr >= input->width) {
                    printf("`");
                }

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


    FILE *file = fopen(argv[2], "wb");
    if (input.ct == RGB) {
        fprintf(file, "P6\n%d %d\n%d\n",
            output.width, output.height, output.maxval);
        for (i = 0; i < output.height; i++) {
            fwrite(output.picC[i], 1, sizeof(pixelRGB) * output.width, file);
        }
        for (i = 0; i < input.height; i++) {
            free(input.picC[i]);
        }
        free(input.picC);
        for (i = 0; i < output.height; i++) {
            free(output.picC[i]);
        }
        free(output.picC);
    } else {
        fprintf(file, "P5\n%d %d\n%d\n",
            output.width, output.height, output.maxval);
        for (i = 0; i < output.height; i++) {
            fwrite(output.picGS[i], 1, sizeof(pixelGS) * output.width, file);
        }
        for (i = 0; i < input.height; i++) {
            free(input.picGS[i]);
        }
        free(input.picGS);
        for (i = 0; i < output.height; i++) {
            free(output.picGS[i]);
        }
        free(output.picGS);
    }
    fclose(file);
    return 0;
}