/*
    next-hack's bmp2rawVideo.
    Copyright 2017 Nicola Wrachien and Franco Gobbo (next-hack.com)
    This file is part of next-hack's bmp2rawVideo.

    next-hack's bmp2rawVideo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    next-hack's bmp2rawVideo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with next-hack's bmp2rawVideo.  If not, see <http://www.gnu.org/licenses/>.

    -----------------------------------------------------------------------------------

    Special note from the authors!

    This program is neither bug-free nor optimized. Use it wisely, as we did not bother to
    handle every possible case (incorrect file format, incorrect parameters, etc.).

    Furthermore this program is meant only for 160x128 displays. You must modify it according
    to yours. Beware that displays with larger resolutions will yield lower frame rates!

*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <limits.h>
#include "b2r_util.h"
//
#define VERSION "1.0"
//
#define MAXFILENAMELENGTH 1024
//
unsigned char *LoadBitmapFile(char *filename, BITMAPINFOHEADER *bitmapInfoHeader);
int createRawAudioVideo(char *baseBitmapFileName, char *waveFileName, char *outputFile, int numberOfFrames, int width, int height, int useAudio, int samplesPerBlock);
int getWaveFileInfo(WAVHEADER *header, FILE *ptr);
// main function.
int main(int argc, char **argv)
{
    int  numFrames = INT_MAX;
    int i;
    int useAudio = 0;
    char *inputFileName = NULL, *outputFileName = NULL , *waveFileName = NULL;
    int samplesPerBlock = 256;
    if (argc < 3)
    {
        printf("bmp2rawVideo V%s, an open-source tool for creating videos for Arduino.\n\n",VERSION);
        printf("To get the most updated version, visit next-hack.com!\n\n");
        printf("Usage: bmp2rawVideo -i inputFile [-w waveFile] -o outputFile [-n nnnnn] [-s sss]\n");
        printf("Where:\n");
        printf("Arguments within square brackets [] are optional.\n");
        printf("-i  inputFile specifies input file. It must be of the form <baseName>%%0<n>d.bmp, where n is the number of the digits. TEST%%04d = the frames have the base name 'TEST' and 4 digits\n");
        printf("Each frame is a 160x128 24 bit BMP.\n");
        printf("-w waveFile specifies a 1-channel 16-bit linear PCM encoded wav file, with 20000 Hz sampling rate!\n");
        printf("-o outputFile specifies output file.\n");
        printf("-n specifies the number frames to be generated (if not specified, the program continues until the first missing frame)\n");
        printf("-s Number of audio samples per block 1.256\n");
    }
    for (i = 1; i<argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
        {
            i++;
            inputFileName = argv[i];
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            i++;
            outputFileName = argv[i];
        }
        else if (strcmp(argv[i], "-w") == 0)
        {
            i++;
            waveFileName = argv[i];
            useAudio = 1;
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            i++;
            sscanf(argv[i],"%d",&numFrames);
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            i++;
            sscanf(argv[i],"%d",&samplesPerBlock);
            if (samplesPerBlock > 256 || samplesPerBlock < 1)
                samplesPerBlock = 256;
        }

    }
    if (inputFileName == NULL)
    {
        printf("Error: input file not specified");
        return 1;
    }
    if (outputFileName == NULL)
    {
        printf("Error: output file not specified");
        return 1;
    }
    createRawAudioVideo(inputFileName,waveFileName, outputFileName, numFrames, 160, 128, useAudio, samplesPerBlock);
    printf("End of process\n");
    return 0;
}
unsigned int LoadBitmapFileInBuffer(char *filename,BITMAPINFOHEADER *bitmapInfoHeader,int sizeOfBuffer, unsigned char *bitmapImage)
{
    FILE *filePtr;                      // file pointer
    BITMAPFILEHEADER bitmapFileHeader;  // bitmap file header
    int loaded=0;
    //open filename in read binary mode
    filePtr = fopen(filename,"rb");
    if (filePtr == NULL)
        return 1;
    // read the bitmap file header
    fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER),1,filePtr);
    // verify bitmap id
    if (bitmapFileHeader.bfType !=0x4D42)
    {
        printf("Error: wrong file format!\n");
        fclose(filePtr);
        return 1;
    }
    //read the bitmap info header
    fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER),1,filePtr);
    //move file point to the begging of bitmap data
    fseek(filePtr, bitmapFileHeader.bOffBits, SEEK_SET);
    // we need to set this. We found that in some cases it is not properly set!
    bitmapInfoHeader->biSizeImage = bitmapInfoHeader->biWidth * bitmapInfoHeader->biHeight * 3;
    //read in the bitmap image data
    loaded = fread(bitmapImage,1,sizeOfBuffer,filePtr);
    //make sure the bitmap image data was read
    if (loaded != bitmapInfoHeader->biSizeImage)
    {
        printf("Error: cannot read file, or wrong pixel format or wrong resolution!\n");
        fclose(filePtr);
        return 1;
    }
    //close file and return 0, as no error occurred!
    fclose(filePtr);
    return 0;
}
int littleEndian32ToInt(unsigned char *buffer)
{
  return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
}
int littleEndian16ToInt(unsigned char *buffer)
{
  return buffer[0] | (buffer[1] << 8);
}
int checkWaveFileInfo(WAVHEADER *header, FILE *ptr, int sampleRate)
{
    unsigned char buffer[4];
    // read header parts
    fread(header->riff, sizeof(header->riff), 1, ptr);
    fread(buffer, 4, 1, ptr);
    // convert little endian to big endian 4 byte int
    header->overall_size  = littleEndian32ToInt(buffer);

    fread(header->wave, sizeof(header->wave), 1, ptr);
    if (memcmp(header->wave, "WAVE", sizeof(header->wave)))
    {
        printf ("Error: not a wav file!\n");
        return 1;
    }
    fread(header->fmt_chunk_marker, sizeof(header->fmt_chunk_marker), 1, ptr);
    //
    fread(buffer, 4, 1, ptr);
    // convert little endian to int
    header->length_of_fmt = littleEndian32ToInt(buffer);
    //
    fread(buffer, 2, 1, ptr);
    //
    header->format_type = littleEndian16ToInt(buffer);
    if (header->format_type != 1)
    {
        printf ("Error: wrong format type!\n");
        return 1;
    }
    fread(buffer, 2, 1, ptr);
    header->channels = littleEndian16ToInt(buffer);
    if (header->channels != 1)
    {
        printf ("Error: wrong number of channels!\n");
        return 1;
    }
    fread(buffer, 4, 1, ptr);
    header->sample_rate =  littleEndian32ToInt(buffer);
    if (header->sample_rate != sampleRate)
    {
        printf("Error: wrong sample rate! (must be %d Hz!)",sampleRate);
        return 1;
    }
    fread(buffer, 4, 1, ptr);
    header->byterate  =  littleEndian32ToInt(buffer);
    if (header->byterate != sampleRate*2)
    {
        printf("Error: wrong byte rate! (must be %d Bytes/s!)",2*sampleRate);
        return 1;
    }
    fread(buffer, 2, 1, ptr);
    header->block_align = littleEndian16ToInt(buffer);
    fread(buffer, 2, 1, ptr);
    header->bits_per_sample = littleEndian16ToInt(buffer);
    if (header->bits_per_sample != 16)
    {
        printf("Error: wrong bit per sample! (must be 16!)");
        return 1;
    }
    fread(header->data_chunk_header, sizeof(header->data_chunk_header), 1, ptr);
    fread(buffer, 4, 1, ptr);
    header->data_size = littleEndian32ToInt(buffer);
    return 0;
}
// Read the next samples!
int readNextSamples(FILE *src, unsigned char *buffer, int nSamples)
{
    int i;
    memset(&buffer[2 * nSamples ],0,512 - 2 * nSamples);         // add mute for missing samples
    fread(buffer, 2, nSamples, src);      // read the requested amount of samples.
    for (i = 0; i<  256; i++)
    {
        buffer[2 * i + 1]= 128 + buffer[ 2 * i + 1];
    }
    return 0;
}
// unified video only or video+audio conversion.
int createRawAudioVideo(char *baseBitmapFileName, char *waveFileName, char *outputFile, int numberOfFrames, int width, int height, int useAudio, int samplesPerBlock)
{
    FILE *destFilePtr;                      // file pointer of the final file
    BITMAPINFOHEADER bitmapInfoHeader;
    WAVHEADER wH;                           // header of the wav file
    FILE *wFile;                            // wav file handler
    unsigned char *bitmapData,*destData;    // pointers to the read bitmap and the destination frame + audio buffer
    int j;                                  // next byte to write in the destData buffer.
    int value;                              // 16-bit pixel in format RRRRRGGGGGBBBBB
    int r,g,b;                              // 8-bit red, green and blue
    int q;                                  // number of quarter of frame. The audio data is interleaved each quarter of frame.
    int err = 0;                            // error variable (if 1 the main loop exits)
    int n_img;
    int res;
    int pos;                                // where is the line on the bitmap data buffer.
    int x,y;                                // current pixel to be processed.
    char imageFileName[MAXFILENAMELENGTH];
    // The file is as follows
    // 512 bytes with samples (actually we use samplesPerBlock*2 hytes)
    // 4 x (sample + quarter of image)
    destData = malloc(width * height * 2 + 4096);   // each frame consists of: 512 bytes audio + a quarter of frame + 512 bytes audio + a quarter of frame.
    if (destData == NULL)
    {
        printf("Error: cannot create a buffer as large as %d bytes\n",width * height * 2 + 4096);
        return 1;
    }
    // we create another buffer to read the image. This might be faster than reading line by line.
    bitmapData = malloc (width * height * 3);
    if (bitmapData == NULL)
    {
        printf("Error: cannot create a buffer as large as %d bytes\n",width * height * 3);
        free(destData);
        return 1;
    }
    if (useAudio)
    {
        wFile = fopen(waveFileName,"rb");
        if (!wFile)
        {
            free(destData);
            free (bitmapData);
            printf("Error: cannot read audio file!\n");
            return 2;
        }
        // TODO: Check if wave file is ok! For now we assume our file at 20kHz.
        err = checkWaveFileInfo(&wH,wFile, 20000);
    }
    //
    destFilePtr = fopen(outputFile,"wb");
    if (!destFilePtr)
    {
        if (useAudio)
            fclose(wFile);
        free(destData);
        free (bitmapData);
        printf("Error: cannot create destination file!\n");
        return 2;
    }





    if (useAudio)
    {
        readNextSamples(wFile,destData,samplesPerBlock);
        if (fwrite(destData,1,512,destFilePtr) != 512)
        {
            printf("Error: cannot save  to destination file\n");
            err = 1;
        }

    }
    for (n_img = 1; n_img <= numberOfFrames && !err; n_img++)
    {
        // Load current frame
        snprintf(imageFileName,sizeof(imageFileName),baseBitmapFileName,n_img);
        res = LoadBitmapFileInBuffer(imageFileName,&bitmapInfoHeader,3*width*height,bitmapData);
        printf("Converting %s\n",imageFileName);
        if (res == 0)
        {
            if (bitmapInfoHeader.biWidth != width || bitmapInfoHeader.biHeight != height || bitmapInfoHeader.biBitCount != 24)
            {
                printf("Error: image %s has wrong width (%d, required %d), or height (%d, required %d) or bits per pixel (%d, required 24)\n",
                       imageFileName,bitmapInfoHeader.biWidth, width, bitmapInfoHeader.biHeight,height,bitmapInfoHeader.biBitCount);
                err = 1;
            }
            // new frame: reset the position of the next byte to be written.
            j = 0;
            // process each quarter of frame.
            for (q = 0; q < 4; q++)
            {
                // firstly, insert the audio
                if (useAudio)
                {
                    readNextSamples(wFile,&destData[j],samplesPerBlock);
                    j+= 512;
                }
                // then put a quarter of frame.
                for (y = q * (bitmapInfoHeader.biHeight >> 2) ; y < (q + 1) * (bitmapInfoHeader.biHeight >> 2) ; y++)
                {
                    // calculate the position of the current line
                    pos = 3 * (bitmapInfoHeader.biHeight - 1 - y ) * bitmapInfoHeader.biWidth;
                    for (x = 0; x < bitmapInfoHeader.biWidth; x++)
                    {
                        // The bitmap is BGR and upside down!
                        b = bitmapData[pos] >> 3;
                        g = bitmapData[pos + 1] >> 2;
                        r = bitmapData[pos + 2] >> 3;
                        value = r << 11 | g << 5 | b;
                        destData[j++] = value >> 8;
                        destData[j++] = value & 0xFF;
                        pos += 3;
                    }
                }
            }
            if (fwrite(destData,1,j,destFilePtr) != j)
            {
                printf("Error: cannot write to output file.\n");
                err = 1;
            }
        }
        else
        {
            printf("Image %s cannot be read. This might simply mean that all the frames were processed!\n",imageFileName);
            err = 1;
        }
    }
    free(bitmapData);
    fclose(destFilePtr);
    if (useAudio)
        fclose(wFile);
    free(destData);
    return err;
}
