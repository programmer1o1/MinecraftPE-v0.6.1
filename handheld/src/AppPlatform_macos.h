#ifndef APPPLATFORM_MACOS_H__
#define APPPLATFORM_MACOS_H__

#include "AppPlatform.h"
#include "platform/log.h"
#include <fstream>
#include <sstream>
#include <png.h>

static void png_macos_readFile(png_structp pngPtr, png_bytep data, png_size_t length) {
    ((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

class AppPlatform_macos : public AppPlatform
{
public:
    AppPlatform_macos() {}

    // macOS is always "fast" enough for the game
    bool isSuperFast() { return true; }

    bool supportsTouchscreen() { return false; }

    int getScreenWidth()  { return 854; }
    int getScreenHeight() { return 480; }

    float getPixelsPerMillimeter() { return 4.0f; }

    // Always licensed on desktop
    int checkLicense() { return 0; }
    bool hasBuyButtonWhenInvalidLicense() { return false; }

    TextureData loadTexture(const std::string& filename_, bool textureFolder)
    {
        TextureData out;

        std::string filename = textureFolder
            ? "data/images/" + filename_
            : filename_;

        std::ifstream source(filename.c_str(), std::ios::binary);
        if (!source) {
            LOGI("Couldn't find file: %s\n", filename.c_str());
            return out;
        }

        png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!pngPtr)
            return out;

        png_infop infoPtr = png_create_info_struct(pngPtr);
        if (!infoPtr) {
            png_destroy_read_struct(&pngPtr, NULL, NULL);
            return out;
        }

        png_set_read_fn(pngPtr, (void*)&source, png_macos_readFile);
        png_read_info(pngPtr, infoPtr);

        out.w = png_get_image_width(pngPtr, infoPtr);
        out.h = png_get_image_height(pngPtr, infoPtr);

        png_bytep* rowPtrs = new png_bytep[out.h];
        out.data = new unsigned char[4 * out.w * out.h];
        out.memoryHandledExternally = false;

        int stride = 4 * out.w;
        for (int i = 0; i < out.h; ++i)
            rowPtrs[i] = (png_bytep)&out.data[i * stride];

        png_read_image(pngPtr, rowPtrs);
        png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
        delete[] rowPtrs;
        source.close();

        return out;
    }

    BinaryBlob readAssetFile(const std::string& filename)
    {
        std::ifstream file(("data/" + filename).c_str(), std::ios::binary | std::ios::ate);
        if (!file)
            return BinaryBlob();

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        BinaryBlob blob;
        blob.size = (int)size;
        blob.data = new unsigned char[size];
        if (!file.read((char*)blob.data, size)) {
            delete[] blob.data;
            return BinaryBlob();
        }
        return blob;
    }

    std::string getDateString(int s)
    {
        std::stringstream ss;
        ss << s << " s (UTC)";
        return ss.str();
    }
};

#endif /* APPPLATFORM_MACOS_H__ */
