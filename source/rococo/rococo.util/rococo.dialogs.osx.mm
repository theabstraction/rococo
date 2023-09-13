#include <rococo.api.h>

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

#include <rococo.strings.h>
#include <rococo.os.h>

namespace Rococo
{
	namespace OS
	{
		using namespace Rococo::Windows;

		void ShowErrorBox(IWindow& parent, IException& ex, cstr caption)
		{
		   char fullmessage[1024];
		   if (ex.ErrorCode() == 0)
		   {
				SafeFormat(fullmessage, "%s", ex.Message());
		   }
		   else
		   {
				char numError[256];
				OS::Format_C_Error(ex.ErrorCode(), numError, 256);
				SafeFormat(fullmessage, "%s:\n\t%s", numError, ex.Message());
		   }

		   NSAlert *alert =  [[[NSAlert alloc] init] autorelease];
		   [alert addButtonWithTitle : @"OK"];
		   [alert setMessageText : [NSString stringWithUTF8String:caption]];
		   [alert setInformativeText : [NSString stringWithUTF8String:fullmessage]];
		   [alert setAlertStyle : NSInformationalAlertStyle];
		   [alert runModal];
		}
	}
}