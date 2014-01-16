#ifndef RGB_PARSER_HH
#define RGB_PARSER_HH

#include <stdio.h>
#include <stdint.h>

// Pragma pack(push, 1) tells the compiler not to pad the header struct
#pragma pack(push, 1)
typedef struct
{
    int16_t magic;
    uint8_t storage;
    uint8_t bpc;
    uint16_t dimension;
    uint16_t sizeX;
    uint16_t sizeY;
    uint16_t sizeZ;
    int32_t pixmin;
    int32_t pixmax;
    uint8_t dummy1[4];
    uint8_t name[80];
    int32_t colormap;
    uint8_t dummy2[404];
} RGBHeader;
#pragma pack(pop)

typedef struct {
	int sizeX;
	int sizeY;

	unsigned char* channelR;
	unsigned char* channelG;
	unsigned char* channelB;

	unsigned char* RGB;

	RGBHeader* header;
} RGBImage;

// Reads an RGB image from a file
// Returns the image on success, NULL on failure
RGBImage* readRGBImage(const char* fileName);

RGBImage* writeRGBImage(RGBImage* img, const char* fileName);

// Frees an RGB image
void destroyRGBImage(RGBImage* image);

#endif