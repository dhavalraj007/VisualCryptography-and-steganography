#include<iostream>
#include<vector>
#include<random>
#include<algorithm>
#include<string>
#define STB_IMAGE_IMPLEMENTATION
#include"stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"stb/stb_image_write.h"

#include"glad/glad.h"
#define SDL_MAIN_HANDLED
#include"sdl2/SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

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
struct Image {
	char* name="";
	unsigned char* data=nullptr;
	int w, h, n;
	GLuint textureID;
};
static PixelRGBA blackRGBA{ 0,0,0,255 };
static PixelRGBA whiteRGBA{ 255,255,255,10 };

int to1d(int x, int y, int xlim) { return xlim * y + x; }


bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}

bool LoadTextureFromData(Image& img)
{
	if (img.data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.w, img.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);

	img.textureID= image_texture;
	return true;
}
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

std::vector<bool> BWGeneration(Image& img)
{
	unsigned char* data = stbi_load(img.name, &img.w, &img.h, &img.n, 0);
	PixelRGB* inData = reinterpret_cast<PixelRGB*>(data);
	if (!data)
	{
		std::cout << "error loading image!";
		return std::vector<bool>(0);
	}
	//create BW data for source image
	PixelRGBA* bwData = new PixelRGBA[img.w * img.h];
	std::vector<bool> origIsBlack(img.w * img.h, false);
	for (int i = 0; i < img.w * img.h; i++)
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
	img.data = reinterpret_cast<unsigned char*>(bwData);
	stbi_write_png(("BW" + std::string(img.name)).c_str(), img.w, img.h, 4, reinterpret_cast<unsigned char*>(bwData), img.w * 4);
	std::cout << ("BW" + std::string(img.name)) << " written." << std::endl;
	return origIsBlack;
}

void doVC(char* sourceFileName,Image& sourceImg,Image& outImg1,Image& outImg2)
{
	sourceImg.name = sourceFileName;
	auto origIsBlack = BWGeneration(sourceImg);
	
	PixelRGBA* img1data = new PixelRGBA[sourceImg.w * sourceImg.h * 4];
	PixelRGBA* img2data = new PixelRGBA[sourceImg.w * sourceImg.h * 4];
	auto to1d = [](int x, int y, int xlim) {return xlim * y + x; };
	for (int _x = 0; _x < sourceImg.w; _x++)
	{
		for (int _y = 0; _y < sourceImg.h; _y++)
		{
			//for each pixel in orig image

			int x_img = _x * 2;
			int y_img = _y * 2;
			if (origIsBlack[to1d(_x, _y, sourceImg.w)])	//if original pixel is black
			{
				if (flipCoin())
				{
					setSubPixel(img1data, x_img, y_img, sourceImg.w * 2, 0, 1, 1, 0);
					setSubPixel(img2data, x_img, y_img, sourceImg.w * 2, 1, 0, 0, 1);
				}
				else
				{
					setSubPixel(img1data, x_img, y_img, sourceImg.w * 2, 1, 0, 0, 1);
					setSubPixel(img2data, x_img, y_img, sourceImg.w * 2, 0, 1, 1, 0);
				}
			}
			else		//if original pixel is white
			{
				if (flipCoin())
				{
					setSubPixel(img1data, x_img, y_img, sourceImg.w * 2, 0, 1, 1, 0);
					setSubPixel(img2data, x_img, y_img, sourceImg.w * 2, 0, 1, 1, 0);
				}
				else
				{
					setSubPixel(img1data, x_img, y_img, sourceImg.w * 2, 1, 0, 0, 1);
					setSubPixel(img2data, x_img, y_img, sourceImg.w * 2, 1, 0, 0, 1);
				}
			}
		}
	}

	outImg1.data = reinterpret_cast<unsigned char*>(img1data);
	outImg1.w = sourceImg.w * 2;
	outImg1.h = sourceImg.h * 2;
	outImg1.n = 4;
	outImg2.data = reinterpret_cast<unsigned char*>(img2data);
	outImg2.w = sourceImg.w * 2;
	outImg2.h = sourceImg.h * 2;
	outImg2.n = 4;

	stbi_write_png("outputImage1.png", outImg1.w,outImg1.h,outImg1.n, reinterpret_cast<unsigned char*>(img1data), outImg1.w * 4);
	stbi_write_png("outputImage2.png", outImg2.w, outImg2.h, outImg2.n, reinterpret_cast<unsigned char*>(img2data), outImg2.w * 4);
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
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}


	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 860, 450, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	Image sourceImg, outImg1, outImg2;
	doVC("Naruto.jpg", sourceImg, outImg1, outImg2);
	LoadTextureFromData(sourceImg);
	LoadTextureFromData(outImg1);
	LoadTextureFromData(outImg2);
	//LoadTextureFromFile("outputImage1.png", &my_image_texture, &my_image_width, &my_image_height);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		//if (show_demo_window)
			//ImGui::ShowDemoWindow(&show_demo_window);

		ImGui::PushStyleColor(ImGuiCol_WindowBg, { 1.0f, 1.0f, 1.0f, 0.5f });
		ImGui::Begin("imgs");
		ImVec2 avail_size = ImGui::GetContentRegionAvail();
		ImGui::Text("source Image");
		ImGui::Image((void*)(intptr_t)sourceImg.data, avail_size);
		ImGui::Text("out Image 1");
		ImGui::Image((void*)(intptr_t)outImg1.data, avail_size);
		ImGui::Text("out Image 2");
		ImGui::Image((void*)(intptr_t)outImg2.data, avail_size);
		ImGui::End();
		ImGui::PopStyleColor();
		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
			SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
		}

		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}