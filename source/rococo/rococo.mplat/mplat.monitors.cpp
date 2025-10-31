#include <rococo.types.h>
#include <rococo.io.h>
#include <rococo.os.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <rococo.target.h>
#include <Windows.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.sexml.h>
#include <rococo.functional.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

namespace Rococo::IO
{
	typedef Rococo::IO::IShaderMonitor* (*FN_RococoGraphics_CreateShaderMonitor)(Rococo::IO::IShaderMonitorEvents& eventHandler, cstr targetDirectory);

	IO::IShaderMonitor* TryCreateShaderMonitor(cstr section, Rococo::IO::IShaderMonitorEvents& eventHandler) noexcept
	{
		IO::IShaderMonitor* result = nullptr;

		if (!section || *section == 0)
		{
			section = "monitor.shader";
		}

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
						builder.AddEscapedStringToList(R"(\work\rococo\source\rococo\dx11.renderer\shaders\gbuffer)");
						builder.CloseListAttribute();
						builder.CloseDirective();
					}
				);
			}

			OS::LoadUserSEXML(nullptr, section,
				[&eventHandler, &result](const ISEXMLDirectiveList& topLevelDirectives)
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
							AutoFree<IO::IShaderMonitor> shaderMonitor = createShaderMonitor(eventHandler, targetDirectory);

							shaderMonitor->SetMonitorDirectory(monitorDirectory);

							for (size_t i = 0; i < includes.NumberOfElements(); i++)
							{
								cstr includePath = includes[i];
								shaderMonitor->AddIncludePath(includePath);
							}

							result = shaderMonitor.Detach();
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

			SafeFormat(message, "Exception creating shader monitor %s: %s. Code 0x%8.8X", sexPath.buf, ex.Message(), ex.ErrorCode());
			printf("%s\n", message);
			OutputDebugStringA(message);
		}

		return result;
	}
}