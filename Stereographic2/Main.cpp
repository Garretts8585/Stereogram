#include "SGIImage.h"

int main()
{
	SGIImage TestImage;

	TestImage.GenerateTiledStereogramImage("DepthImage.bmp", "Tiled.bmp", 25);
	TestImage.SaveToFile_CStyle("StereoImage.bmp");
}