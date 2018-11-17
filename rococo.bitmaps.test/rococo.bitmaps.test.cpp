#include <rococo.api.h>

#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <vector>

#include <rococo.window.h>
#include <rococo.io.h>
#include <rococo.strings.h>

#include <rococo.textures.h>
#include <rococo.imaging.h>

#include <rococo.libs.inl>

using namespace Rococo;
using namespace Rococo::Windows;

class MainWindow : StandardWindowHandler, public IListViewEvents
{
   ModalDialogHandler modalHandler;
   IDialogSupervisor* dialogWindow;
   IListViewSupervisor* imageView;

   virtual void OnDrawItem(DRAWITEMSTRUCT& dis)
   {

   }

   virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis)
   {

   }

   virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
   {
      if (type != RESIZE_TYPE_MINIMIZED)
      {
         RECT rect;
         GetClientRect(*dialogWindow, &rect);

         MoveWindow(*imageView, 0, 0, rect.right, rect.bottom, TRUE);
      }
   }

   MainWindow() : dialogWindow(nullptr)
   {
   }

   ~MainWindow()
   {
      Rococo::Free(dialogWindow);
   }

   virtual void OnClose(HWND hWnd)
   {
      modalHandler.TerminateDialog(IDCANCEL);
   }

   virtual void OnMenuCommand(HWND hWnd, DWORD id)
   {
   }

   void PostConstruct()
   {
      WindowConfig config;
      SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOW, nullptr, "Bitmap Test Dialog", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX, 0);
      dialogWindow = Windows::CreateDialogWindow(config, this);
      imageView = Windows::AddListView(*dialogWindow, GuiRect{ 0, 0, 1, 1 }, "Textures", *this,  LVS_ICON | LVS_ALIGNLEFT, 0, 0);

      RECT rect;
      GetClientRect(*dialogWindow, &rect);

      MoveWindow(*imageView, 0, 0, rect.right, rect.bottom, TRUE);
   }

public:
   static MainWindow* Create()
   {
      auto m = new MainWindow();
      m->PostConstruct();
      return m;
   }

   DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */, HIMAGELIST hImages)
   {
      ListView_SetImageList(imageView->ListViewHandle(), hImages, LVSIL_NORMAL);

      int nImages = ImageList_GetImageCount(hImages);

      char text[128];

      for (int i = 0; i < nImages; ++i)
      {
         LVITEMA item = { 0 };
         item.iItem = i;
         item.mask = LVIF_IMAGE | LVIF_TEXT;
         item.iImage = i;
            
         SafeFormat(text, sizeof(text), "Texture #%d", i);
         item.pszText = text;
         SendMessage(imageView->ListViewHandle(), LVM_INSERTITEMA, 0, (LPARAM)&item);
      }

      return dialogWindow->BlockModal(modalHandler.ModalControl(), owner, this);
   }

   void Free()
   {
      delete this;
   }
};

class ResourceLoader : public Rococo::Textures::IResourceLoader
{
private:
public:
   virtual void Load(cstr name, IEventCallback<Textures::CompressedTextureBuffer>& onLoad)
   {  
      auto* ext = GetFileExtension(name);

      Textures::COMPRESSED_TYPE type;
      if (ext == nullptr)
      {
         Throw(0, "Could not load image file: %s. No extension", name);
      }
      
      if (Eq(ext, ".tiff") || Eq(ext, ".tif"))
      {
         type = Textures::COMPRESSED_TYPE_TIF;
      }
      else if (Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
      {
         type = Textures::COMPRESSED_TYPE_JPG;
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

      onLoad.OnEvent(Textures::CompressedTextureBuffer{ &data[0], fileLength, type });
   }
};

class TextureArray_ImageList: public Textures::ITextureArray
{
public:
   struct ImageTexture
   {
      HBITMAP hBitmap;
	  RGBAb* pixels;
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
      HBITMAP hBitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pBits, nullptr, 0 );
      if (hBitmap == nullptr)
      {
         Throw(GetLastError(), "Could not create DIB section");
      }

      imageTextures.push_back({ hBitmap, (RGBAb*) pBits });
   }

   virtual void WriteSubImage(size_t index, const RGBAb* subImagePixels, const GuiRect& targetLocation)
   {
      int nRows = targetLocation.bottom - targetLocation.top;

      RGBAb* targetline = imageTextures[index].pixels + width * targetLocation.top;
      const RGBAb* sourceline = subImagePixels;

      size_t xOffset = targetLocation.left;
      size_t lineSize = sizeof(RGBAb) * (targetLocation.right - targetLocation.left);

      for (int j = 0; j < nRows; ++j)
      {
         memcpy_s(targetline + xOffset, lineSize, sourceline, lineSize);
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
};

void Main()
{  
   AutoFree<IAllocatorSupervisor> imageAllocator(Memory::CreateBlockAllocator(128, 0));
   Imaging::SetTiffAllocator(imageAllocator);
   Imaging::SetJpegAllocator(imageAllocator);

   char directory[MAX_PATH];
   if (!IO::ChooseDirectory(directory, MAX_PATH))
   {
      return;
   }

   ResourceLoader loader;
   TextureArray_ImageList tarray;
   AutoFree<MainWindow> mainWindow(MainWindow::Create());
   AutoFree<Rococo::Textures::ITextureArrayBuilderSupervisor> textureArrayBuilder = Textures::CreateTextureArrayBuilder(loader, tarray);

   struct ANON : IEventCallback<cstr>
   {
      Textures::ITextureArrayBuilder* builder;
      cstr root;

      virtual void OnEvent(cstr pathname)
      {
         auto* ext = GetFileExtension(pathname);
         if (Eq(ext, ".tiff") || Eq(ext, ".tif") || Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
         {
            char filename[MAX_PATH];
            SafeFormat(filename, sizeof(filename), "%s\\%s", root, pathname);
            builder->AddBitmap(filename);
         }      
      }

   } onFile;

   onFile.root = directory;
   onFile.builder = textureArrayBuilder;

   IO::ForEachFileInDirectory(directory, onFile);

   textureArrayBuilder->BuildTextures(1);

   HIMAGELIST hImages = tarray.CreateImageList();
   mainWindow->DoModal(nullptr, hImages);
   ImageList_Destroy(hImages);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   try
   {
      Rococo::Windows::InitRococoWindows(hInstance, nullptr, nullptr, nullptr, nullptr);
      Main();
   }
   catch (IException& ex)
   {
      Rococo::OS::ShowErrorBox(NullParent(), ex, "Bitmap Test Dialog - Exception");
   }

   return 0;
}