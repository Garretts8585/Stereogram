#pragma once

#include <string>
#include <vector>

/* Class that represents an image that can be saved to BMP
 * with functions to create Stereograms
 */
class SGIImage {

public:
	/* Constructors */
	SGIImage();
	SGIImage(const std::string& ImageFileToLoad);

	/* Indexer into the image bytes */
	const std::vector<unsigned char>& operator[](int y) const;

	/* Loads a BMP image file data into our image data */
	void LoadFromFile_CStyle(const std::string& FileName);

	/* Saves our data to a BMP image file */
	void SaveToFile_CStyle(const std::string& FileName) const;

	/* 
	 * Creates a random dot stereogram image 
	 * 
	 * @param DepthImageFilName: Name of the depth file to open and use to shift pixels to create the 3D effect
	 * @param RepeatingPatternWidth: How many pixels until the random dot pattern should repeat
	 * @param MaxDepthShift: Number of pixels to shift a pixel when corresponding depth image is max
	 */
	void GenerateRandomDotStereogramImage(const std::string& DepthImageFilName, int RepeatingPatternWidth, int MaxDepthShift);

	/*
	* Creates a stereogram image using a repeating tile image
	*
	* @param DepthImageFilName: Name of the BMP depth file to open and use to shift pixels to create the 3D effect
	* @param TileImageFileName: Name of the BMP tile to repeat
	* @param MaxDepthShift: Number of pixels to shift a pixel when corresponding depth image is max
	*/
	void GenerateTiledStereogramImage(const std::string& DepthImageFileName, const std::string& TileImageFileName, int MaxDepthShift);

	/* Changes this image into random black and white pixels of given dimensions */
	void GenerateRandomDotImage(int Width, int Height);

	/* Clears image data */
	void Clear();

	/* Returns true if the image data is empty */
	bool IsEmpty() const;

protected:
	/* Helper function to add a pixel and add a shift version of it if appropriate */
	void AddStereogramShiftedPixel(const SGIImage& RepeatingPattern, int PatternY, int x, int y, int ShiftedAmount);

	// Image dimensions
	int Width;
	int Height;

	// Representation of colors for the image, colors are represented in 3 bytes like in a BMP file
	std::vector<std::vector<unsigned char>> ImageBytes;
};