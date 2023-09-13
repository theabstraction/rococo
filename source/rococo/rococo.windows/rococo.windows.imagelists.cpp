#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.textures.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <vector>
#include <CommCtrl.h>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Graphics::Textures;
using namespace Rococo::Strings;

class ResourceLoader : public ICompressedResourceLoader
{
private:
public:
    void Load(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad) override
    {
        auto* ext = GetFileExtension(name);

        COMPRESSED_TYPE type;
        if (ext == nullptr)
        {
            Throw(0, "Could not load image file: %s. No extension", name);
        }

        if (Eq(ext, ".tiff") || Eq(ext, ".tif"))
        {
            type = COMPRESSED_TYPE_TIF;
        }
        else if (Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
        {
            type = COMPRESSED_TYPE_JPG;
        }
        else
        {
            Throw(0, "Could not load image file: %s. Unrecognized extension", name);
        }

        struct Anon
        {
            HANDLE hFile;

            ~Anon()
            {
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hFile);
                }
            }

            operator HANDLE() { return hFile; }
        };

        Anon hFile;

        hFile.hFile = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            Throw(GetLastError(), "Could not load image file: %s", name);
        }

        LARGE_INTEGER fileLengthQ;
        GetFileSizeEx(hFile, &fileLengthQ);

        if (fileLengthQ.QuadPart > 0x20000000LL)
        {
            Throw(0, "Filesize for %s exceeded limit", name);
        }

        DWORD fileLength = (DWORD)fileLengthQ.QuadPart;

        std::vector<uint8> data;
        data.resize(fileLength);

        DWORD read;
        if (!ReadFile(hFile, &data[0], fileLength, &read, nullptr) || read != fileLength)
        {
            Throw(0, "ReadFile failed for image file: %s", name);
        }

        CompressedTextureBuffer context{ &data[0], fileLength, type };
        onLoad.OnEvent(context);
    }
};

class TextureArray_ImageList : public IBitmapArray
{
public:
    struct ImageTexture
    {
        HBITMAP hBitmap;
        RGBQUAD* pixels;
    };

    int32 width{ 0 };

    std::vector<ImageTexture> imageTextures;

    size_t TextureCount() const
    {
        return imageTextures.size();
    }

    virtual void AddTexture()
    {
        BITMAPINFO info = { 0 };
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biHeight = -width;
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biCompression = BI_RGB;

        void* pBits;
        HBITMAP hBitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pBits, nullptr, 0);
        if (hBitmap == nullptr)
        {
            Throw(GetLastError(), "Could not create DIB section");
        }

        imageTextures.push_back({ hBitmap, (RGBQUAD*)pBits });
    }

    virtual void WriteSubImage(size_t index, const RGBAb* subImagePixels, const GuiRect& targetLocation)
    {
        int nRows = targetLocation.bottom - targetLocation.top;

        RGBQUAD* targetline = imageTextures[index].pixels + width * targetLocation.top;
        const RGBAb* sourceline = subImagePixels;

        size_t xOffset = targetLocation.left;

        size_t nCols = targetLocation.right - targetLocation.left;
        size_t lineSize = sizeof(RGBAb) * nCols;
        UNUSED(lineSize);

        for (int j = 0; j < nRows; ++j)
        {
            for (int i = 0; i < nCols; ++i)
            {
                auto col = sourceline[i];
                RGBQUAD& q = targetline[xOffset + i];
                q.rgbBlue = col.blue;
                q.rgbGreen = col.green;
                q.rgbRed = col.red;
                q.rgbReserved = 0;
            }

            targetline += width;
            sourceline += (targetLocation.right - targetLocation.left);
        }
    }

    ~TextureArray_ImageList()
    {
        ResetWidth(0);
    }

    virtual void ResetWidth(int32 width)
    {
        this->width = width;

        for (auto& i : imageTextures)
        {
            DeleteObject(i.hBitmap);
            i.hBitmap = nullptr;
        }
    }

    HIMAGELIST CreateImageList()
    {
        HIMAGELIST hImages = ImageList_Create(width, width, ILC_COLOR32, 0, 1);
        for (auto& i : imageTextures)
        {
            int index = ImageList_Add(hImages, i.hBitmap, nullptr);
            if (index < 0) Throw(GetLastError(), "ImageList_Add failed");
        }
        return hImages;
    }

    virtual int32 MaxWidth() const
    {
        return 1024;
    }

    void ResetWidth(int dx, int dy) override
    {
        UNUSED(dx);
        UNUSED(dy);
    }

    void WriteSubImage(size_t index, const uint8* grayPixels, Vec2i span) override
    {
        UNUSED(index);
        UNUSED(grayPixels);
        UNUSED(span);
    }
};

#include <rococo.imaging.h>
#include <rococo.api.h>

namespace Rococo::Windows
{
    ROCOCO_WINDOWS_API HIMAGELIST CreateImageList(cstr sysPath, int32 maxSupportedImages)
    {
        AutoFree<IAllocatorSupervisor> imageAllocator(Memory::CreateBlockAllocator(128, 0, "CreateImageList_ImageAllocator"));
        Imaging::SetTiffAllocator(imageAllocator);
        Imaging::SetJpegAllocator(imageAllocator);

        ResourceLoader loader;
        TextureArray_ImageList tarray;

        AutoFree<IBitmapArrayBuilderSupervisor> textureArrayBuilder = CreateBitmapArrayBuilder(loader, tarray);

        AutoFree<IPathCacheSupervisor> paths = CreatePathCache();
        paths->AddPathsFromDirectory(sysPath, false);
        paths->Sort();

        for (size_t i = 0; i < paths->NumberOfFiles() && i < maxSupportedImages; ++i)
        {
            cstr pathname = paths->GetFileName(i);

            auto* ext = GetFileExtension(pathname);
            if (Eq(ext, ".tiff") || Eq(ext, ".tif") || Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
            {
                textureArrayBuilder->AddBitmap(pathname);
            }
        }

        textureArrayBuilder->BuildBitmaps(1);

        HIMAGELIST hImages = tarray.CreateImageList();
        return hImages;
    }
}