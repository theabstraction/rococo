#include <rococo.types.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.sexml.h>
#include <rococo.functional.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

namespace Rococo::IO
{
	typedef Rococo::IO::IShaderMonitor* (*FN_RococoGraphics_CreateShaderMonitor)(Rococo::Strings::IStringPopulator& logger, cstr targetDirectory);

	IO::IShaderMonitor* TryCreateShaderMonitor(Strings::IStringPopulator& logger) noexcept
	{
		IO::IShaderMonitor* result = nullptr;

		cstr section = "monitor.shader";

		try
		{
			if (!OS::IsUserSEXMLExistant(nullptr, section))
			{
				OS::SaveUserSEXML(nullptr, section, [](ISEXMLBuilder& builder)
					{
						builder.AddDirective("Directories");
						builder.AddStringLiteral("monitor-path", R"(\work\rococo\source\rococo\dx11.renderer\shaders\)");
						builder.AddStringLiteral("target-path", R"(\work\rococo\content\shaders\compiled\)");
						builder.OpenListAttribute("include-paths");
						builder.AddEscapedStringToList(R"(\work\rococo\source\rococo\dx11.renderer\shaders\)");
						builder.CloseListAttribute();
						builder.CloseDirective();
					}
				);
			}

			OS::LoadUserSEXML(nullptr, section,
				[&logger, &result](const ISEXMLDirectiveList& topLevelDirectives)
				{
					size_t startIndex = 0;
					auto& dDirectories = GetDirective(topLevelDirectives, "Directories", IN OUT startIndex);

					cstr monitorDirectory = AsString(dDirectories["monitor-path"]).c_str();
					cstr targetDirectory = AsString(dDirectories["target-path"]).c_str();
					auto& includes = AsStringList(dDirectories["include-paths"]);
					
					HMODULE hShaderMonitorLib = LoadLibraryA("dx11.hlsl.monitor.dll");
					if (hShaderMonitorLib)
					{
						auto createShaderMonitor = (FN_RococoGraphics_CreateShaderMonitor)GetProcAddress(hShaderMonitorLib, "RococoGraphics_CreateShaderMonitor");
						if (createShaderMonitor)
						{
							AutoFree<IO::IShaderMonitor> shaderMonitor = createShaderMonitor(logger, targetDirectory);

							shaderMonitor->SetMonitorDirectory(monitorDirectory);

							for (size_t i = 0; i < includes.NumberOfElements(); i++)
							{
								cstr includePath = includes[i];
								shaderMonitor->AddIncludePath(includePath);
							}

							result = shaderMonitor.Release();
							return;
						}
					}
				}
			);
		}
		catch (IException& ex)
		{
			char message[4096];

			U8FilePath sexPath;
			OS::GetUserSEXMLFullPath(sexPath, nullptr, section);

			SafeFormat(message, "%s: %s. Code 0x%8.8X", sexPath.buf, ex.Message(), ex.ErrorCode());
			OutputDebugStringA(message);
		}

		return result;
	}
}