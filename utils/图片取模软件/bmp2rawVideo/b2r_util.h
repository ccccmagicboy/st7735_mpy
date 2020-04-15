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

*/
/*
    This file contains definitions of useful structures.
*/
#ifndef B2R_UTIL_H_INCLUDED
#define B2R_UTIL_H_INCLUDED

#pragma pack(push, 1)           // This is required to avoid padding.
typedef struct  tagBITMAPFILEHEADER
{
    uint16_t bfType;  //specifies the file type
    uint32_t bfSize;  //specifies the size in bytes of the bitmap file
    uint16_t bfReserved1;  //reserved; must be 0
    uint16_t bfReserved2;  //reserved; must be 0
    uint32_t bOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
} BITMAPFILEHEADER;

typedef struct __attribute__((packed)) tagBITMAPINFOHEADER
{
    uint32_t biSize;  //specifies the number of bytes required by the struct
    int32_t biWidth;  //specifies width in pixels
    int32_t biHeight;  //species height in pixels
    uint16_t biPlanes; //specifies the number of color planes, must be 1
    uint16_t biBitCount; //specifies the number of bit per pixel
    uint32_t biCompression;//spcifies the type of compression
    uint32_t biSizeImage;  //size of image in bytes
    int32_t biXPelsPerMeter;  //number of pixels per meter in x axis
    int32_t biYPelsPerMeter;  //number of pixels per meter in y axis
    uint32_t biClrUsed;  //number of colors used by th ebitmap
    uint32_t biClrImportant;  //number of colors that are important
}BITMAPINFOHEADER;

// WAVE file header format
typedef struct  {
	uint8_t riff[4];						// RIFF string
	uint32_t overall_size	;				// overall size of file in bytes
	uint8_t  wave[4];						// WAVE string
	uint8_t  fmt_chunk_marker[4];			// fmt string with trailing null char
	uint32_t length_of_fmt;					// length of the format data
	uint32_t format_type;					// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint32_t channels;						// no.of channels
	uint32_t sample_rate;					// sampling rate (blocks per second)
	uint32_t byterate;						// SampleRate * NumChannels * BitsPerSample/8
	uint32_t block_align;					// NumChannels * BitsPerSample/8
	uint32_t bits_per_sample;				// bits per sample, 8- 8bits, 16- 16 bits etc
	uint8_t  data_chunk_header [4];		// DATA string or FLLR string
	uint32_t data_size;						// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} WAVHEADER;
#pragma pack(pop)

#endif // B2R_UTIL_H_INCLUDED
