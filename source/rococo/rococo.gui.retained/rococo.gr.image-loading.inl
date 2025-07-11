#include <rococo.gui.retained.ex.h>

#include <rococo.imaging.h>
#include <rococo.strings.h>
#include <rococo.io.h>
#include <memory.h>

namespace Rococo::Gui::Implementation
{
	template<class LAMBDA> void Load32bitRGBAbImage(cstr imagePingPath, IO::IInstallation& installation, LAMBDA onLoad)
	{
		using namespace Rococo;
		using namespace Rococo::Strings;

		struct ImageParser : Imaging::IImageLoadEvents
		{
			LAMBDA onLoad;
			HString err;

			void OnError(const char* message) override
			{
				err = message;
			}

			void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
			{
				onLoad(span, data);
			}

			void OnAlphaImage(const Vec2i& span, const uint8* data) override
			{
				UNUSED(span);
				UNUSED(data);
				err = "8bpp images not supported";
			}

			ImageParser(LAMBDA _onLoad) : onLoad(_onLoad)
			{

			}

			virtual ~ImageParser()
			{

			}
		} parser(onLoad);

		if (EndsWithI(imagePingPath, ".tiff") || EndsWithI(imagePingPath, ".tif"))
		{
			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(64_kilobytes);
			installation.LoadResource(imagePingPath, *buffer, 32_megabytes);
			Rococo::Imaging::DecompressTiff(parser, buffer->GetData(), buffer->Length());
		}
		else if (EndsWithI(imagePingPath, ".jpeg") || EndsWithI(imagePingPath, ".jpg"))
		{
			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(64_kilobytes);
			installation.LoadResource(imagePingPath, *buffer, 32_megabytes);
			Rococo::Imaging::DecompressJPeg(parser, buffer->GetData(), buffer->Length());
		}
		else
		{
			Throw(0, "Could not load image: %s. Only jpg and tiff files are recognized", imagePingPath);
		}

		if (parser.err.length() > 0)
		{
			Throw(0, "Could not parse image data for %s. Error: %s", imagePingPath, parser.err.c_str());
		}
	} // LoadImage
} // Rococo::Gui::Implementation