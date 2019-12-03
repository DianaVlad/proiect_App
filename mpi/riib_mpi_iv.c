#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
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

void riibUp(image *input, image *output) {
    if (output->height == 0 || output->width == 0) return;
    int i, j;

    long x, y = 0;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    long x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    int xr, yr;

    int indexInputBase, indexInput, indexOutput = 0;

    if (output->ct == GS) {
        pixelGS TL, TR, BL, BR;
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
                ++indexOutput;
            }
            y += y_ratio;
        }
    }
}

void riibDown(image *input, image *output) {
    if (output->height == 0 || output->width == 0) return;
    int i, j;

    long x, y;
    int y_ratio = (((long)input->height - 1) << 16) / output->height;
    y = y_ratio >> 1;
    int x_ratio = (((long)input->width - 1) << 16) / output->width;
    int xr, yr;

    int indexInput, indexInputBase, indexOutput = 0;

    if (output->ct == GS) {
        for (i = 0; i < output->height; ++i) {
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
        for (i = 0; i < output->height; ++i) {
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

/*
 * argv[1] - input
 * argv[2] - output
 * argv[3] - scale
 */

int main(int argc, char *argv[]) {
    int rank;
    int threadCount;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &threadCount);

    image input, output;
    image partialInput, partialOutput;

    char *lineR;
    char *lineG;
    char *lineB;

    int data[5];

    int startsI[threadCount];
    int endsI[threadCount];

    int startsO[threadCount];
    int endsO[threadCount];

    int partialInputLen;

    float scale = atof(argv[3]);

    float xRatio, yRatio;

    int i, j;

    int startI, endI, startO, endO;

    if (scale == 1) {
        if (rank == 0) {
            readFromImage(&input, argv[1]);
            writeToImage(&input, argv[2]);
        }
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        readFromImage(&input, argv[1]);
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
                riibUp(&input, &output);
            } else {
                riibDown(&input, &output);
            }
        } else {
            /*
             * compute output width & height
             * create type and limits and broadcast them
             * alloc output image
             * compute x_ratio & y_ratio
             * compute startsO[threads] & endsO[threads]
             * compute startsI[threads] & endsI[threads] from partial outputs limits
             * for each thread send lines startsI[thread] through endsI[thread]
             * create a partialInput and partialOutput for rank 0
             */

            data[0] = input.ct;
            data[1] = input.height;
            data[2] = input.width;
            data[3] = output.height;
            data[4] = output.width;

            MPI_Bcast(&data, 5, MPI_INT, 0, MPI_COMM_WORLD);

            xRatio = (float)(input.height - 1) / output.height;
            yRatio = (float)(input.width - 1) / output.width;

            int maxLineLength = 0;

            for (i = 0; i < threadCount; ++i) {
                startsO[i] = i * ceil((double)data[3] / threadCount);
                endsO[i] = min((i + 1) * ceil((double)data[3] / threadCount),
                             (double)data[3]);

                startsI[i] = xRatio * startsO[i];
                endsI[i] = xRatio * endsO[i];

                if (maxLineLength < endsI[i] - startsI[i]) {
                    maxLineLength = endsI[i] - startsI[i];
                }
                if (maxLineLength < endsO[i] - startsO[i]) {
                    maxLineLength = endsO[i] - startsO[i];
                }
            }

            if (input.width > output.width) {
                maxLineLength *= input.width;
            } else {
                maxLineLength *= output.width;
            }


            double mem = input.width * input.height + output.width * output.height;
            mem += mem * threadCount;
            if (input.ct == RGB) {
                mem += maxLineLength * 3;
            } else {
                mem += maxLineLength;
            }

            if (input.ct == RGB) {
                lineR = malloc(sizeof(char) * maxLineLength);
                lineG = malloc(sizeof(char) * maxLineLength);
                lineB = malloc(sizeof(char) * maxLineLength);
                for (i = 1; i < threadCount; ++i) {
                    if (endsO[i] == startsO[i]) continue;
                    int indexPartial = 0;
                    int index = startsI[i] * input.width;
                    int max = endsI[i] * input.width;
                    for (j = index; j < max; ++j) {
                        lineR[indexPartial] = (char)input.picC[j].R;
                        lineG[indexPartial] = (char)input.picC[j].G;
                        lineB[indexPartial] = (char)input.picC[j].B;
                        ++indexPartial;
                    }
                    MPI_Send(lineR, indexPartial, MPI_CHAR, i, 0,
                        MPI_COMM_WORLD);
                    MPI_Send(lineG, indexPartial, MPI_CHAR, i, 0,
                        MPI_COMM_WORLD);
                    MPI_Send(lineB, indexPartial, MPI_CHAR, i, 0,
                        MPI_COMM_WORLD);
                }

                partialInput.ct = input.ct;
                partialOutput.ct = input.ct;
                partialInput.width = input.width;
                partialOutput.width = output.width;
                partialInput.height = endsI[0];
                partialOutput.height = endsO[0];

                allocPicture(&partialInput);
                allocPicture(&partialOutput);

                int max = partialInput.height * partialInput.width;

                for (i = 0; i < max; ++i) {
                    partialInput.picC[i].R = input.picC[i].R;
                    partialInput.picC[i].G = input.picC[i].G;
                    partialInput.picC[i].B = input.picC[i].B;
                }

            } else {
                lineG = malloc(sizeof(char) * maxLineLength);
                for (i = 1; i < threadCount; ++i) {
                    if (endsO[i] == startsO[i]) continue;
                    int indexPartial = 0;
                    int index = startsI[i] * input.width;
                    int max = endsI[i] * input.width;
                    for (j = index; j < max; ++j) {
                        lineG[indexPartial] = (char)input.picGS[j];
                        ++indexPartial;
                    }
                    MPI_Send(lineG, indexPartial, MPI_CHAR, i, 0,
                        MPI_COMM_WORLD);
                }

                partialInput.ct = input.ct;
                partialOutput.ct = input.ct;
                partialInput.width = input.width;
                partialOutput.width = output.width;
                partialInput.height = endsI[0];
                partialOutput.height = endsO[0];

                allocPicture(&partialInput);
                allocPicture(&partialOutput);

                int max = partialInput.height * partialInput.width;

                for (i = 0; i < max; ++i) {
                    partialInput.picGS[i] = input.picGS[i];
                }
            }
        }
    } else {
        /*
         * broadcast data input & output
         * compute xRatio and yRatio
         * compute limits for partial input & partial output
         * allocate space for partial input & partial output
         * receive lines from rank 0
         * compute riib
         * send partial output back to rank 0
         */
        MPI_Bcast(&data, 5, MPI_INT, 0, MPI_COMM_WORLD);
    
        xRatio = (float)(data[1] - 1) / data[3];
        yRatio = (float)(data[2] - 1) / data[4];

        startO = rank * ceil((double)data[3] / threadCount);
        endO = min((rank + 1) * ceil((double)data[3] / threadCount),
                     (double)data[3]);

        startI = xRatio * startO;
        endI = xRatio * endO;

        partialInput.ct = data[0];
        partialOutput.ct = data[0];
        partialInput.width = data[2];
        partialOutput.width = data[4];
        partialInput.height = endI - startI;
        partialOutput.height = endO - startO;

        allocPicture(&partialInput);
        allocPicture(&partialOutput);

        int lineLength = partialInput.height * partialInput.width;
        if (lineLength < partialOutput.height * partialOutput.width) {
            lineLength = partialOutput.height * partialOutput.width;
        }

        int recv_length = partialInput.height * partialInput.width;

        if (partialOutput.height == 0 || partialOutput.width == 0) {
            ;
        } else if (partialInput.ct == RGB) {
            lineR = malloc(sizeof(char) * lineLength);
            lineG = malloc(sizeof(char) * lineLength);
            lineB = malloc(sizeof(char) * lineLength);
            MPI_Recv(lineR, recv_length, MPI_CHAR, 0, MPI_ANY_TAG,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(lineG, recv_length, MPI_CHAR, 0, MPI_ANY_TAG,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(lineB, recv_length, MPI_CHAR, 0, MPI_ANY_TAG,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (i = 0; i < recv_length; ++i) {
                partialInput.picC[i].R = (unsigned char)lineR[i];
                partialInput.picC[i].G = (unsigned char)lineG[i];
                partialInput.picC[i].B = (unsigned char)lineB[i];
            }
        } else if (partialInput.ct == GS) {
            lineG = malloc(sizeof(char) * lineLength);
            MPI_Recv(lineG, recv_length, MPI_CHAR, 0, MPI_ANY_TAG,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (i = 0; i < recv_length; ++i) {
                partialInput.picGS[i] = (unsigned char)lineG[i];
            }
        }
    }

    /*
     * scale down/up the partialInput and store it in partialOutput 
     */
    if (threadCount != 1) {
        if (scale > 1) {
            riibUp(&partialInput, &partialOutput);
        } else {
            riibDown(&partialInput, &partialOutput);
        }
    }

    if (threadCount == 1) {
        writeToImage(&output, argv[2]);

        destroyPicture(&input);
        destroyPicture(&output);
    } else if (rank == 0) {
        /*
         * store partialOutput in output
         * receive from each thread a partialOutput
         * store each partialOutput in output image
         * generate output image
         */

        if (output.ct == RGB) {
            int currentIndex = 0;
            int lineLength = (endsO[0] - startsO[0]) * output.width;

            for (i = 0; i < lineLength; ++i) {
                output.picC[i].R = partialOutput.picC[i].R;
                output.picC[i].G = partialOutput.picC[i].G;
                output.picC[i].B = partialOutput.picC[i].B;
            }
            currentIndex += lineLength;

            for (i = 1; i < threadCount; ++i) {
                lineLength = (endsO[i] - startsO[i]) * output.width;
                if (lineLength == 0) continue;
                MPI_Recv(lineR, lineLength, MPI_CHAR, i, MPI_ANY_TAG,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(lineG, lineLength, MPI_CHAR, i, MPI_ANY_TAG,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(lineB, lineLength, MPI_CHAR, i, MPI_ANY_TAG,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int indexPartial = 0;
                pixelRGB *save = output.picC + currentIndex;

                for (j = 0; j < lineLength; ++j) {
                    save[j].R = (unsigned char)lineR[j];
                    save[j].G = (unsigned char)lineG[j];
                    save[j].B = (unsigned char)lineB[j];
                }
                currentIndex += lineLength;
            }
        } else if (output.ct == GS) {
            int currentIndex = 0;
            int lineLength = (endsO[0] - startsO[0]) * output.width;

            for (i = 0; i < lineLength; ++i) {
                output.picGS[i] = partialOutput.picGS[i];
            }
            currentIndex += lineLength;

            for (i = 1; i < threadCount; ++i) {
                lineLength = (endsO[i] - startsO[i]) * output.width;
                if (lineLength == 0) continue;
                MPI_Recv(lineG, lineLength, MPI_CHAR, i, MPI_ANY_TAG,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                pixelGS *save = output.picGS + currentIndex;
                for (j = 0; j < lineLength; ++j) {
                    save[j] = (unsigned char)lineG[j];
                }
                currentIndex += lineLength;
            }
        }

        writeToImage(&output, argv[2]);

        destroyPicture(&input);
        destroyPicture(&output);
    } else {
        int lineLength = partialOutput.width * partialOutput.height;

        if (partialOutput.height == 0 || partialOutput.width == 0) {
            ;
        } else if (partialOutput.ct == RGB) {
            for (i = 0; i < lineLength; ++i) {
                lineR[i] = partialOutput.picC[i].R;
                lineG[i] = partialOutput.picC[i].G;
                lineB[i] = partialOutput.picC[i].B;
            }
            MPI_Send(lineR, lineLength, MPI_CHAR, 0, 0,
                MPI_COMM_WORLD);
            MPI_Send(lineG, lineLength, MPI_CHAR, 0, 0,
                MPI_COMM_WORLD);
            MPI_Send(lineB, lineLength, MPI_CHAR, 0, 0,
                MPI_COMM_WORLD);
        } else {

            for (i = 0; i < lineLength; ++i) {
                lineG[i] = partialOutput.picGS[i];
            }
            MPI_Send(lineG, lineLength, MPI_CHAR, 0, 0,
                MPI_COMM_WORLD);
        }
    }

    destroyPicture(&partialInput);
    destroyPicture(&partialOutput);

    if (partialInput.ct == RGB) {
        free(lineR);
        free(lineG);
        free(lineB);
    } else {
        free(lineG);
    }

    MPI_Finalize();
    return 0;
}