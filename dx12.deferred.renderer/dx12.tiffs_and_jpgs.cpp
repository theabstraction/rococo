#include <rococo.api.h>
#include <rococo.imaging.h>
#include <rococo.dx12.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.io.h>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	class TIFF_Loader: public ITextureLoader
	{
	private:
		IInstallation& installation;
		AutoFree<IExpandingBuffer> scratch;
	public:
		TIFF_Loader(IInstallation& ref_installation):
			installation(ref_installation),
			scratch(CreateExpandingBuffer(4_megabytes))
		{

		}

		virtual ~TIFF_Loader()
		{

		}

		void Free() override
		{
			delete this;
		}

		bool TryLoad(cstr resourceName, ILoadEvent& onLoad)
		{
			struct CLOSURE : Imaging::IImageLoadEvents
			{
				ILoadEvent* loadEvent;

				void OnError(const char* message) override
				{
					Throw(0, "%s", message);
				}

				void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
				{
					loadEvent->OnRGBAb(data, span);
				}

				void OnAlphaImage(const Vec2i& span, const uint8* data) override
				{
					loadEvent->OnGreyscale(data, span);
				}
			} closure;
			closure.loadEvent = &onLoad;

			cstr ext = GetFileExtension(resourceName);
			if (!ext) return false;
			if (!EqI(ext, ".tiff") && !EqI(ext, ".tif")) return false;

			installation.LoadResource(resourceName, *scratch, 64_megabytes);

			Imaging::DecompressTiff(closure, scratch->GetData(), scratch->Length());

			return true;
		}
	};

	class JPEG_Loader : public ITextureLoader
	{
	private:
		IInstallation& installation;
		AutoFree<IExpandingBuffer> scratch;
	public:
		JPEG_Loader(IInstallation& ref_installation) :
			installation(ref_installation),
			scratch(CreateExpandingBuffer(4_megabytes))
		{

		}

		virtual ~JPEG_Loader()
		{

		}

		void Free() override
		{
			delete this;
		}

		bool TryLoad(cstr resourceName, ILoadEvent& onLoad)
		{
			struct CLOSURE : Imaging::IImageLoadEvents
			{
				ILoadEvent* loadEvent;

				void OnError(const char* message) override
				{
					Throw(0, "%s", message);
				}

				void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
				{
					loadEvent->OnRGBAb(data, span);
				}

				void OnAlphaImage(const Vec2i& span, const uint8* data) override
				{
					loadEvent->OnGreyscale(data, span);
				}
			} closure;
			closure.loadEvent = &onLoad;

			cstr ext = GetFileExtension(resourceName);
			if (!ext) return false;
			if (!EqI(ext, ".jpeg") && !EqI(ext, ".jpg")) return false;

			installation.LoadResource(resourceName, *scratch, 64_megabytes);

			Imaging::DecompressJPeg(closure, scratch->GetData(), scratch->Length());

			return true;
		}
	};
}

namespace Rococo::Graphics
{
	ITextureLoader* CreateTiffLoader(IInstallation& installation)
	{
		return new ANON::TIFF_Loader(installation);
	}

	ITextureLoader* CreateJPEGLoader(IInstallation& installation)
	{
		return new ANON::JPEG_Loader(installation);
	}
}