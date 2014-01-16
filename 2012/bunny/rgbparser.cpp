#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rgbparser.hh"

// Reverses the byte order in 16 bits
int16_t reverseBytes(int16_t value) {
	return (int16_t)((value & 0xFFU) << 8 | (value & 0xFF00U) >> 8);
}

// Reverses the byte order in 16 bits
uint16_t reverseBytes(uint16_t value) {
	return (uint16_t)((value & 0xFFU) << 8 | (value & 0xFF00U) >> 8);
}

// Reverses the byte order in 32 bits
int32_t reverseBytes(int32_t value) {
	return (int32_t)((value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24);
}

RGBImage* readRGBImage(const char* fileName) {
	assert(fileName);

	FILE* file = fopen(fileName, "rb");

    if (file == NULL) {
        printf("Failed to open texture file at: %s\n", fileName);
        return NULL;
    }

	RGBImage* image;
	RGBHeader* header;

	char buffer[512];
	int ret;

	// Read the header 512 first bytes
	ret = fread(buffer, sizeof(char), 512, file);

	if (ret != 512) {
		fprintf(stderr, "Error while reading the RGB file header, only read %d/512 bytes\n", ret);
		fclose(file);
		return NULL;
	}

	header = (RGBHeader*)buffer;

	// Sanity check for header fields
	
	// Magic reveals the the file is actually a RGB file and intact
	if (reverseBytes(header->magic) != 474) {
		fprintf(stderr, "Error in RGB texture file header's magic field\n");
		fclose(file);
		return NULL;
	}
	// Value 1 indicates RLE, which would be bad for us, 0 means no RLE
	if (header->storage != 0) {
		fprintf(stderr, "Error in RGB texture file header's storage field\n");
		fclose(file);
		return NULL;
	}
	// Bytes per pixel component, must be one
	if (header->bpc != 1) {
		fprintf(stderr, "Error in RGB texture file header's BPC field\n");
		fclose(file);
		return NULL;
	}
	// We want RGB components, nothing more nothing less
	if (reverseBytes(header->dimension) != 3) {
		fprintf(stderr, "Error in RGB texture file header's dimension field\n");
		fclose(file);
		return NULL;
	}
	// We want RGB components, nothing more nothing less
	if (reverseBytes(header->sizeZ) != 3) {
		fprintf(stderr, "Error in RGB texture file header's sizeZ field\n");
		fclose(file);
		return NULL;
	}
	if (header->colormap != 0) {
		fprintf(stderr, "Error in RGB texture file header's colormap field\n");
		fclose(file);
		return NULL;
	}

	// Header was ok, allocate memory for the image structure
	image = (RGBImage*)malloc(sizeof(RGBImage));

	image->sizeX = reverseBytes(header->sizeX);
	image->sizeY = reverseBytes(header->sizeY);

	// Allocate memory and copy header
	image->header = (RGBHeader*)malloc(sizeof(RGBHeader));
	assert(image->header);
	*(image->header) = *header;

	// Calculate image size
	int size = image->sizeX * image->sizeY;

	// Allocate memory for R,G,B channels and the universal RGB channel
	image->channelR = (unsigned char*)calloc(size, sizeof(unsigned char));
	image->channelG = (unsigned char*)calloc(size, sizeof(unsigned char));
	image->channelB = (unsigned char*)calloc(size, sizeof(unsigned char));
	image->RGB = (unsigned char*)calloc(size * 3, sizeof(unsigned char));

	assert(image->channelR && image->channelG && image->channelB && image->RGB);

	// Read in the R,G,B channels
	ret = fread(image->channelR, sizeof(unsigned char), size, file);
	if (ret != size) {
		fprintf(stderr, "Error while reading the RGB file's red channel, only read %d/%d bytes\n", ret, size);
		fclose(file);
		return NULL;
	}

	ret = fread(image->channelG, sizeof(unsigned char), size, file);
	if (ret != size) {
		fprintf(stderr, "Error while reading the RGB file's green channel, only read %d/%d bytes\n", ret, size);
		fclose(file);
		return NULL;
	}

	ret = fread(image->channelB, sizeof(unsigned char), size, file);
	if (ret != size) {
		fprintf(stderr, "Error while reading the RGB file's blue channel, only read %d/%d bytes\n", ret, size);
		fclose(file);
		return NULL;
	}

	fclose(file);

	for (int i = 0; i < size; i++) {
		image->RGB[i*3] = image->channelR[i];
		image->RGB[i*3 + 1] = image->channelG[i];
		image->RGB[i*3 + 2] = image->channelB[i];
	}

	return image;
}

RGBImage* writeRGBImage(RGBImage* img, const char* fileName)
{
	assert(fileName);

	FILE* file = fopen(fileName, "wb");

    if (file == NULL) {
        printf("Failed to open texture file for writing at: %s\n", fileName);
        return NULL;
    }

    int size = img->sizeX * img->sizeY;

    // Write the header
    fwrite(img->header, sizeof(char), 512, file);

    // Write the channels
    fwrite(img->channelR, sizeof(unsigned char), size, file);
    fwrite(img->channelG, sizeof(unsigned char), size, file);
    fwrite(img->channelB, sizeof(unsigned char), size, file);

	fclose(file);

	return img;
}

void destroyRGBImage(RGBImage* image) {
	assert(image);

	free(image->header);
	free(image->channelR);
	free(image->channelG);
	free(image->channelB);
	
	free(image->RGB);

	free(image);
}