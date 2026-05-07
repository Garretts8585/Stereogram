
#include <fstream>

// Project Includes
#include "SGIImage.h"

// Color and padding bits defines
#define BYTES_PER_COLOR 3
#define PADDING_BYTE_MULTIPLE 4
#define COLOR_VALUE_MAX 255
#define COLOR_VALUE_MIN 0

// BMP Header defines
#define FILE_HEADER_FORMAT_START 0
#define FILE_HEADER_FILE_SIZE_START 2
#define FILE_HEADER_HEADER_SIZE_START 10
#define INFO_HEADER_WIDTH_START 4
#define INFO_HEADER_HEIGHT_START 8
#define INFO_HEADER_COLOR_COUNT_START 14
#define BMP_FILE_HEADER_SIZE 14
#define BMP_INFO_HEADER_SIZE 40
#define BMP_HEADER_SIZE (BMP_INFO_HEADER_SIZE + BMP_FILE_HEADER_SIZE)

SGIImage::SGIImage() {
	Width = 0;
	Height = 0;
}

void SGIImage::Clear() {
	Width = 0;
	Height = 0;
	ImageBytes.clear();
}

bool SGIImage::IsEmpty() const {
	return Width <= 0 || Height <= 0;
}

SGIImage::SGIImage(const std::string& ImageFileToLoad) {
	LoadFromFile_CStyle(ImageFileToLoad);
}

const std::vector<unsigned char>& SGIImage::operator[](int y) const {
	return ImageBytes[y];
}

void SGIImage::LoadFromFile_CStyle(const std::string& FileName) {
	Clear();

	FILE* ImageFile = nullptr;
	fopen_s(&ImageFile, FileName.c_str(), "rb");
	if (ImageFile == nullptr) {
		return;
	}

	// Load the header
	unsigned char Header[BMP_HEADER_SIZE];
	fread(Header, sizeof(unsigned char), BMP_HEADER_SIZE, ImageFile);

	// Extract dimensions
	Width = *reinterpret_cast<int*>(&Header[BMP_FILE_HEADER_SIZE+ INFO_HEADER_WIDTH_START]);
	Height = *reinterpret_cast<int*>(&Header[BMP_FILE_HEADER_SIZE + INFO_HEADER_HEIGHT_START]);

	const int PaddingSize = (PADDING_BYTE_MULTIPLE - (Width % PADDING_BYTE_MULTIPLE)) % PADDING_BYTE_MULTIPLE;
	const int StrideSize = (Width * BYTES_PER_COLOR) + PaddingSize;

	int ImageSize = Width * Height * BYTES_PER_COLOR;
	if (ImageSize <= 0) {
		return;
	}

	// Load the image
	unsigned char* ImageBuffer = new unsigned char[ImageSize];
	fread(ImageBuffer, sizeof(unsigned char), ImageSize, ImageFile);

	// convert to 2d array
	for (int y = 0; y < Height; ++y) {
		ImageBytes.push_back(std::vector<unsigned char>());
		for (int x = 0; x < Width; ++x) {
			const int iBuffer = (x * BYTES_PER_COLOR) + (y * StrideSize);

			for (int i = 0; i < BYTES_PER_COLOR; ++i) {
				ImageBytes[y].push_back(static_cast<unsigned char>(ImageBuffer[iBuffer + i]));
			}
		}
	}

	delete [] ImageBuffer;
	fclose(ImageFile);
}

void SGIImage::SaveToFile_CStyle(const std::string& FileName) const {
	FILE* ImageFile = nullptr;
	fopen_s(&ImageFile, FileName.c_str(), "wb");

	if (!ImageFile) {
		return;
	}

	const int PaddingSize = (PADDING_BYTE_MULTIPLE - (Width % PADDING_BYTE_MULTIPLE)) % PADDING_BYTE_MULTIPLE;
	const int StrideSize = (Width * BYTES_PER_COLOR) + PaddingSize;
	const int FileSize = BMP_HEADER_SIZE + (StrideSize * Height);

	// Init file header
	unsigned char FileHeader[BMP_FILE_HEADER_SIZE];
	memset(FileHeader, 0, BMP_FILE_HEADER_SIZE);

	FileHeader[FILE_HEADER_FORMAT_START] = (unsigned char)'B'; // Bit map format code
	FileHeader[FILE_HEADER_FORMAT_START + 1] = (unsigned char)'M';
	FileHeader[FILE_HEADER_FILE_SIZE_START] = (unsigned char)(FileSize); // 32 bit Image size in 8 bit chucks
	FileHeader[FILE_HEADER_FILE_SIZE_START + 1] = (unsigned char)(FileSize >> 8);
	FileHeader[FILE_HEADER_FILE_SIZE_START + 2] = (unsigned char)(FileSize >> 16);
	FileHeader[FILE_HEADER_FILE_SIZE_START + 3] = (unsigned char)(FileSize >> 24);
	FileHeader[FILE_HEADER_HEADER_SIZE_START] = (unsigned char)(BMP_HEADER_SIZE); // header size

	// Init info header
	unsigned char InfoHeader[BMP_INFO_HEADER_SIZE];
	memset(InfoHeader, 0, BMP_INFO_HEADER_SIZE);

	InfoHeader[0] = (unsigned char)(BMP_INFO_HEADER_SIZE);
	
	InfoHeader[INFO_HEADER_WIDTH_START] = (unsigned char)(Width); // 32 bit Width in 8 bit chunks
	InfoHeader[INFO_HEADER_WIDTH_START + 1] = (unsigned char)(Width >> 8);
	InfoHeader[INFO_HEADER_WIDTH_START + 2] = (unsigned char)(Width >> 16);
	InfoHeader[INFO_HEADER_WIDTH_START + 3] = (unsigned char)(Width >> 24);
	
	InfoHeader[INFO_HEADER_HEIGHT_START] = (unsigned char)(Height); // 32 bit Height in 8 bit chunks
	InfoHeader[INFO_HEADER_HEIGHT_START + 1] = (unsigned char)(Height >> 8);
	InfoHeader[INFO_HEADER_HEIGHT_START + 2] = (unsigned char)(Height >> 16);
	InfoHeader[INFO_HEADER_HEIGHT_START + 3] = (unsigned char)(Height >> 24);
	
	// Covert color bytes to bits
	InfoHeader[INFO_HEADER_COLOR_COUNT_START] = (unsigned char)(BYTES_PER_COLOR*8);

	// Write the header
	fwrite(FileHeader, 1, BMP_FILE_HEADER_SIZE, ImageFile);
	fwrite(InfoHeader, 1, BMP_INFO_HEADER_SIZE, ImageFile);

	// Initialize padding
	unsigned char padding[BYTES_PER_COLOR];
	memset(padding, 0, BYTES_PER_COLOR);

	// Write image row by row
	for (int y = 0; y < Height; ++y) {
		fwrite(&ImageBytes[y][0], BYTES_PER_COLOR, Width, ImageFile);
		fwrite(padding, 1, PaddingSize, ImageFile);
	}

	fclose(ImageFile);
}

void SGIImage::GenerateRandomDotStereogramImage(const std::string& DepthImageFileName, int RepeatingPatternWidth, int MaxDepthShift) {
	Clear();

	// Load the depth image file
	SGIImage DepthImage(DepthImageFileName);
	if (DepthImage.IsEmpty()) {
		return;
	}

	Width = DepthImage.Width;
	Height = DepthImage.Height;

	// Generate the horizontally repeating random dot pattern
	SGIImage DotPattern;
	DotPattern.GenerateRandomDotImage(RepeatingPatternWidth, Height);

	// Create the stereogram
	for (int y = 0; y < Height; ++y) {
		ImageBytes.push_back(std::vector<unsigned char>());

		for (int x = 0; x < Width; ++x) {
			// Apparent depth desired from 0 to 1 for this pixel
			float DepthValue = static_cast<float>(DepthImage[y][x * BYTES_PER_COLOR]) / static_cast<float>(COLOR_VALUE_MAX);

			// Number of pixels to shift to give the depth effect
			int ShiftValue = static_cast<int>(floor(DepthValue * static_cast<float>(MaxDepthShift)));
			int TileY = y;

			// Add pixel to the image
			AddStereogramShiftedPixel(DotPattern, TileY, x, y, ShiftValue);
		}
	}
}

void SGIImage::GenerateTiledStereogramImage(const std::string& DepthImageFileName, const std::string& TileImageFileName, int MaxDepthShift) {
	Clear();

	// Load the depth image file as an image
	SGIImage DepthImage(DepthImageFileName);
	if (DepthImage.IsEmpty()) {
		return;
	}

	// Load the tile image
	SGIImage TileImage(TileImageFileName);
	if (TileImage.IsEmpty()) {
		return;
	}

	Width = DepthImage.Width;
	Height = DepthImage.Height;

	// Create the stereogram
	for (int y = 0; y < Height; ++y) {
		ImageBytes.push_back(std::vector<unsigned char>());

		for (int x = 0; x < Width; ++x) {
			// Apparent depth desired from 0 to 1 for this pixel
			const float DepthValue = static_cast<float>(DepthImage[y][x * BYTES_PER_COLOR]) / static_cast<float>(COLOR_VALUE_MAX);

			// Number of pixels to shift to give the depth effect
			const int ShiftValue = static_cast<int>(floor(DepthValue * static_cast<float>(MaxDepthShift)));
			const int TileY = y % TileImage.Height;

			// Add pixel to the image
			AddStereogramShiftedPixel(TileImage, TileY, x, y, ShiftValue);
		}
	}
}

void SGIImage::GenerateRandomDotImage(int InWidth, int InHeight) {
	Clear();

	Width = InWidth;
	Height = InHeight;

	for (int y = 0; y < Height; ++y) {
		ImageBytes.push_back(std::vector<unsigned char>());

		for (int x = 0; x < Width; ++x) {
			const int ColorValue = ((rand() % 2) == 0) ? COLOR_VALUE_MIN : COLOR_VALUE_MAX;

			for (int i = 0; i < BYTES_PER_COLOR; ++i)
				ImageBytes[y].push_back(ColorValue);
		}
	}
}

void SGIImage::AddStereogramShiftedPixel(const SGIImage& RepeatingPattern, int PatternY, int x, int y, int ShiftedAmount) {
	int ImageX = x * BYTES_PER_COLOR;
	
	// Copy the repeating pattern, if the pattern was shifted we need to copy the shift in future iterations
	const int CopyImageColumnToUse = (x - RepeatingPattern.Width) * BYTES_PER_COLOR;
	if (CopyImageColumnToUse >= 0) {
		// Copy our pixel from the previous spot in the repeating pattern in the image
		for (int i = 0; i < BYTES_PER_COLOR; ++i) {
			ImageBytes[y].push_back(ImageBytes[y][CopyImageColumnToUse + i]);
		}
	}
	else
	{
		// First iteration, just use the pixel from the repeating image
		const int RepeatingColumnToUse = (x % RepeatingPattern.Width) * BYTES_PER_COLOR;

		for (int i = 0; i < BYTES_PER_COLOR; ++i) {
			ImageBytes[y].push_back(RepeatingPattern[PatternY][RepeatingColumnToUse + i]);
		}
	}

	// Copy depth values back in the image to create the stereographic effect
	if (ShiftedAmount > 0) {
		const int ShiftedX = (x - ShiftedAmount) * BYTES_PER_COLOR;
		if (ShiftedX > 0) {
			for (int i = 0; i < BYTES_PER_COLOR; ++i) {
				ImageBytes[y][ShiftedX + i] = ImageBytes[y][ImageX + i];
			}
		}
	}
}
