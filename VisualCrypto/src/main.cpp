#include<iostream>
#include<vector>
#include<random>
#include<algorithm>
#include<string>
#define STB_IMAGE_IMPLEMENTATION
#include"stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"stb/stb_image_write.h"
static std::random_device random_device;
static std::mt19937 random_engine{ random_device() };
static std::bernoulli_distribution coin_distribution{ 0.5 };
bool flipCoin()
{
	return coin_distribution(random_engine);
}
int getRandomFrom(int l, int h)
{
	std::uniform_int_distribution<int> dist(l, h);
	return dist(random_engine);
}
std::vector<int> getSample(const std::vector<int>& input, const std::vector<int>& execpt, int n)
{
	std::vector<int> in;
	std::set_difference(input.begin(), input.end(), execpt.begin(), execpt.end(), std::inserter(in, in.begin()));
	std::vector<int> sample;
	std::sample(in.begin(), in.end(), std::back_inserter(sample),
		n, random_engine);
	return sample;
}

struct PixelRGB {
	uint8_t r, g, b;
};
struct PixelRGBA
{
	uint8_t r, g, b, a;
};
static PixelRGBA blackRGBA{ 0,0,0,255 };
static PixelRGBA whiteRGBA{ 255,255,255,10 };
int to1d(int x, int y, int xlim) { return xlim * y + x; }

void setSubPixel(PixelRGBA* imgData, int x, int y, int xlim, bool topLeft, bool topRight, bool bottomLeft, bool bottomRight)
{
	imgData[to1d(x, y, xlim)] = topLeft ? whiteRGBA : blackRGBA;		//top left
	imgData[to1d(x + 1, y, xlim)] = topRight ? whiteRGBA : blackRGBA;		//top right
	imgData[to1d(x, y + 1, xlim)] = bottomLeft ? whiteRGBA : blackRGBA;		//bottom left
	imgData[to1d(x + 1, y + 1, xlim)] = bottomRight ? whiteRGBA : blackRGBA;		//bottom right
}
void setSubPixel(PixelRGBA* imgData, int x, int y, int xlim, const std::vector<bool>& bws)
{
	setSubPixel(imgData, x, y, xlim, bws[0], bws[1], bws[2], bws[3]);
}
void setSubPixel(PixelRGBA* imgData, int x, int y, int xlim, const std::vector<int>& whiteIndices)
{
	std::vector<bool> bws(4, 0);
	for (auto wi : whiteIndices)
		bws[wi] = 1;
	setSubPixel(imgData, x, y, xlim, bws);
}

std::vector<bool> BWGeneration(const std::string& name, int& inX, int& inY, int& inN)
{
	unsigned char* data = stbi_load(name.c_str(), &inX, &inY, &inN, 0);
	PixelRGB* inData = reinterpret_cast<PixelRGB*>(data);
	if (!data)
	{
		std::cout << "error loading image!";
		return std::vector<bool>(0);
	}

	//create BW data for source image
	PixelRGBA* bwData = new PixelRGBA[inX * inY];
	std::vector<bool> origIsBlack(inX * inY, false);
	for (int i = 0; i < inX * inY; i++)
	{
		PixelRGB inPx = *(inData + i);
		PixelRGBA& outPx = *(bwData + i);
		uint8_t grey = int(((inPx.r / 255.f) * 0.2126 + (inPx.g / 255.f) * 0.7152 + (inPx.b / 255.f) * 0.0722) * 255);
		//std::cout << int(grey)<<std::endl;
		origIsBlack[i] = grey < (127);
		if (origIsBlack[i])
			outPx = blackRGBA;
		else
			outPx = whiteRGBA;
	}
	stbi_write_png(("BW" + name).c_str(), inX, inY, 4, reinterpret_cast<unsigned char*>(bwData), inX * 4);
	std::cout << ("BW" + name) << "written." << std::endl;
	return origIsBlack;
}

int doVC()
{
	int inX, inY, inN;
	auto origIsBlack = BWGeneration("Naruto.jpg", inX, inY, inN);

	PixelRGBA* img1data = new PixelRGBA[inX * inY * 4];
	PixelRGBA* img2data = new PixelRGBA[inX * inY * 4];
	auto to1d = [](int x, int y, int xlim) {return xlim * y + x; };
	for (int _x = 0; _x < inX; _x++)
	{
		for (int _y = 0; _y < inY; _y++)
		{
			//for each pixel in orig image

			int x_img = _x * 2;
			int y_img = _y * 2;
			if (origIsBlack[to1d(_x, _y, inX)])	//if original pixel is black
			{
				if (flipCoin())
				{
					setSubPixel(img1data, x_img, y_img, inX * 2, 0, 1, 1, 0);
					setSubPixel(img2data, x_img, y_img, inX * 2, 1, 0, 0, 1);
				}
				else
				{
					setSubPixel(img1data, x_img, y_img, inX * 2, 1, 0, 0, 1);
					setSubPixel(img2data, x_img, y_img, inX * 2, 0, 1, 1, 0);
				}
			}
			else		//if original pixel is white
			{
				if (flipCoin())
				{
					setSubPixel(img1data, x_img, y_img, inX * 2, 0, 1, 1, 0);
					setSubPixel(img2data, x_img, y_img, inX * 2, 0, 1, 1, 0);
				}
				else
				{
					setSubPixel(img1data, x_img, y_img, inX * 2, 1, 0, 0, 1);
					setSubPixel(img2data, x_img, y_img, inX * 2, 1, 0, 0, 1);
				}
			}
		}
	}

	stbi_write_png("outputImage1.png", inX * 2, inY * 2, 4, reinterpret_cast<unsigned char*>(img1data), (inX * 2) * 4);
	stbi_write_png("outputImage2.png", inX * 2, inY * 2, 4, reinterpret_cast<unsigned char*>(img2data), (inX * 2) * 4);
	return 0;
}
int doVS()
{
	//read source image to be encrypted
	int inX, inY, inN;
	auto origIsBlack = BWGeneration("Naruto.jpg", inX, inY, inN);

	//read source image 1 and source image 2
	int src1InX, src1InY, src1InN;
	auto src1IsBlack = BWGeneration("src1.jpg", src1InX, src1InY, src1InN);
	int src2InX, src2InY, src2InN;
	auto src2IsBlack = BWGeneration("src2.jpg", src2InX, src2InY, src2InN);

	PixelRGBA* img1data = new PixelRGBA[inX * inY * 4];
	PixelRGBA* img2data = new PixelRGBA[inX * inY * 4];

	for (int _x = 0; _x < inX; _x++)
	{
		for (int _y = 0; _y < inY; _y++)
		{
			//for each pixel in orig image

			int x_img = _x * 2;
			int y_img = _y * 2;
			if (origIsBlack[to1d(_x, _y, inX)])	//if original pixel is black
			{
				if (src1IsBlack[to1d(_x, _y, inX)] && src2IsBlack[to1d(_x, _y, inX)])	//B1B2
				{
					auto selectedWhite = getSample({ 0,1,2,3 }, {}, 1);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite);
					auto selectedWhite2 = getSample({ 0,1,2,3 }, selectedWhite, 1);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite2);
				}
				else if (src1IsBlack[to1d(_x, _y, inX)] && !src2IsBlack[to1d(_x, _y, inX)])	//B1W2
				{
					auto selectedWhite = getSample({ 0,1,2,3 }, {}, 1);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite);
					auto selectedWhite2 = getSample({ 0,1,2,3 }, selectedWhite, 2);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite2);
				}
				else if (!src1IsBlack[to1d(_x, _y, inX)] && src2IsBlack[to1d(_x, _y, inX)])	//W1B2
				{
					auto selectedWhite = getSample({ 0,1,2,3 }, {}, 1);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite);
					auto selectedWhite2 = getSample({ 0,1,2,3 }, selectedWhite, 2);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite2);
				}
				else		//W1W2
				{
					auto selectedWhite = getSample({ 0,1,2,3 }, {}, 2);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite);
					auto selectedWhite2 = getSample({ 0,1,2,3 }, selectedWhite, 2);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite2);
				}
			}
			else		//if original pixel is white
			{
				if (src1IsBlack[to1d(_x, _y, inX)] && src2IsBlack[to1d(_x, _y, inX)])	//B1B2
				{
					auto selectedWhite = getSample({ 0,1,2,3 }, {}, 1);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite);
				}
				else if (src1IsBlack[to1d(_x, _y, inX)] && !src2IsBlack[to1d(_x, _y, inX)])	//B1W2
				{
					int selectedWhite = getRandomFrom(0, 3);
					setSubPixel(img1data, x_img, y_img, inX * 2, std::vector<int>{selectedWhite});
					auto selectedWhite2 = getSample({ 0,1,2,3 }, { selectedWhite }, 1);
					selectedWhite2.push_back(selectedWhite);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite2);
				}
				else if (!src1IsBlack[to1d(_x, _y, inX)] && src2IsBlack[to1d(_x, _y, inX)])	//W1B2
				{
					int selectedWhite = getRandomFrom(0, 3);
					setSubPixel(img2data, x_img, y_img, inX * 2, std::vector<int>{selectedWhite});
					auto selectedWhite2 = getSample({ 0,1,2,3 }, { selectedWhite }, 1);
					selectedWhite2.push_back(selectedWhite);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite2);
				}
				else		//W1W2
				{
					auto selectedWhite = getSample({ 0,1,2,3 }, {}, 2);
					setSubPixel(img1data, x_img, y_img, inX * 2, selectedWhite);
					auto selectedWhite2 = getSample({ 0,1,2,3 }, selectedWhite, 1);
					selectedWhite2.push_back(selectedWhite[size_t(flipCoin())]);
					setSubPixel(img2data, x_img, y_img, inX * 2, selectedWhite2);
				}
			}
		}
	}

	stbi_write_png("outputImage1.png", inX * 2, inY * 2, 4, reinterpret_cast<unsigned char*>(img1data), (inX * 2) * 4);
	stbi_write_png("outputImage2.png", inX * 2, inY * 2, 4, reinterpret_cast<unsigned char*>(img2data), (inX * 2) * 4);
	return 0;
}
int main()
{
	return doVS();
}