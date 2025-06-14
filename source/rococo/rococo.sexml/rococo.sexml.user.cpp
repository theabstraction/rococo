#include <rococo.compiler.options.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.strings.h>
#include <rococo.sexml.h>
#include <rococo.functional.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.api.h>

using namespace Rococo::Strings;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;

namespace Rococo::OS
{
	HString s_defaultOrganization = "19th-Century-Software";

	ROCOCO_SEXML_API bool IsUserSEXMLExistant(cstr organization, cstr section)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __ROCOCO_FUNCTION__);
		}

		organization = (organization && *organization) ? organization : s_defaultOrganization.c_str();
		WideFilePath wDir;
		IO::GetUserPath(wDir.buf, WideFilePath::CAPACITY, organization);

		if (!IO::IsDirectory(wDir))
		{
			return false;
		}

		WideFilePath wFullName;
		Rococo::Strings::Format(wFullName, _RW_TEXT("%ls\\%hs.sexml"), wDir.buf, section);

		return IO::IsFileExistant(wFullName);
	}

	ROCOCO_SEXML_API void GetUserSEXMLFullPath(U8FilePath& path, cstr organization, cstr section)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __ROCOCO_FUNCTION__);
		}

		organization = (organization && *organization) ? organization : s_defaultOrganization.c_str();

		WideFilePath wDir;
		IO::GetUserPath(wDir.buf, WideFilePath::CAPACITY, organization);

		WideFilePath wFullName;
		Rococo::Strings::Format(wFullName, _RW_TEXT("%ls\\%hs.sexml"), wDir.buf, section);

		Rococo::Strings::Format(path, "%ls", wFullName.buf);
	}

	ROCOCO_SEXML_API void LoadUserSEXML(cstr organization, cstr section, Function<void(const ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __ROCOCO_FUNCTION__);
		}

		organization = (organization && *organization) ? organization : s_defaultOrganization.c_str();

		WideFilePath wDir;
		IO::GetUserPath(wDir.buf, WideFilePath::CAPACITY, organization);

		if (!IO::IsDirectory(wDir))
		{
			Throw(0, "%s(%s, %s,...):\n\tNo such directory: %ls\n", __ROCOCO_FUNCTION__, organization, section, wDir.buf);
		}

		WideFilePath wFullName;
		Rococo::Strings::Format(wFullName, _RW_TEXT("%ls\\%hs.sexml"), wDir.buf, section);

		struct ANON: IStringPopulator
		{
			virtual ~ANON() {}

			HString result;
			void Populate(cstr text) override
			{
				result = text;
			}
		} onLoadFile;

		IO::LoadAsciiTextFile(onLoadFile, wFullName);

		AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(65_kilobytes, 0, "LoadUserDataAllocator");
		
		U8FilePath u8Path;
		Assign(u8Path, wFullName);
				
		Auto<ISParser> parser = CreateSexParser_2_0(*allocator);
		Auto<ISourceCode> src = parser->ProxySourceBuffer(onLoadFile.result.c_str(), (int) onLoadFile.result.length(), Vec2i{ 0,0 }, u8Path);
		Auto<ISParserTree> tree;

		try
		{
			tree = parser->CreateTree(*src);
		}
		catch (ParseException& ex)
		{
			Throw(0, "%s: Error parsing %s: %s. (line %d pos %d)", __ROCOCO_FUNCTION__, u8Path.buf, ex.Message(), ex.Start().y, ex.Start().x);
		}
		catch (...)
		{
			Throw(0, "%s: Error parsing %s: Unhandled exception", __ROCOCO_FUNCTION__, u8Path.buf);
		}

		AutoFree<ISEXMLRootSupervisor> root = CreateSEXMLParser(*allocator, tree->Root());

		onLoad.Invoke(*root);
	}

	ROCOCO_SEXML_API void SaveUserSEXML(cstr organization, cstr section, Function<void(ISEXMLBuilder& builder)> onBuild)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __ROCOCO_FUNCTION__);
		}
		
		organization = (organization && *organization) ? organization : s_defaultOrganization.c_str();

		WideFilePath wOrganizationDir;
		IO::GetUserPath(wOrganizationDir, organization);

		if (!IO::IsDirectory(wOrganizationDir))
		{
			IO::CreateDirectoryFolder(wOrganizationDir);
		}

		WideFilePath wDir;
		Rococo::Strings::Format(wDir, _RW_TEXT("%hs\\%hs.sexml"), organization, section);

		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(64_kilobytes);
		AutoFree<ISEXMLBuilder> pBuilder = CreateSEXMLBuilder(dsb->Builder(), false);

		try
		{
			onBuild.Invoke(*pBuilder);
			pBuilder->ValidateClosed();
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s(%s, %s, ...): %s", __ROCOCO_FUNCTION__, organization, section, ex.Message());
		}

		IO::SaveAsciiTextFile(IO::TargetDirectory_UserDocuments, wDir, *dsb->Builder());
	}

	ROCOCO_SEXML_API void SetDefaultOrganization(cstr defaultOrganization)
	{
		if (!defaultOrganization || *defaultOrganization == 0)
		{
			Throw(0, "%s: blank parameter", defaultOrganization);
		}

		for (cstr p = defaultOrganization; *p != 0; p++)
		{
			if (IsAlphaNumeric(*p))
			{
				continue;
			}

			switch (*p)
			{
			case ' ':
			case '-':
			case '_':
				break;
			default:
				Throw(0, "%s: bad character at position %llu", p - defaultOrganization);
			}
		}

		s_defaultOrganization = defaultOrganization;
	}

	ROCOCO_SEXML_API void LoadSXMLBySysPath(crwstr filename, Function<void(const ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		struct ANON : IStringPopulator
		{
			// Anal that VC++ compiler moans about missing virtual destructors even when instances are on the stack
			virtual ~ANON() {}

			HString result;
			void Populate(cstr text) override
			{
				result = text;
			}
		} onLoadFile;

		IO::LoadAsciiTextFile(onLoadFile, filename);

		AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(65_kilobytes, 0, "LoadUserDataAllocator");

		U8FilePath u8Path;
		Assign(u8Path, filename);

		Auto<ISParser> parser = CreateSexParser_2_0(*allocator);
		Auto<ISourceCode> src = parser->ProxySourceBuffer(onLoadFile.result.c_str(), (int)onLoadFile.result.length(), Vec2i{ 0,0 }, u8Path);
		Auto<ISParserTree> tree;

		try
		{
			tree = parser->CreateTree(*src);
		}
		catch (ParseException& ex)
		{
			Throw(0, "%s: Error parsing %s: %s. (line %d pos %d)", __ROCOCO_FUNCTION__, u8Path.buf, ex.Message(), ex.Start().y, ex.Start().x);
		}
		catch (...)
		{
			Throw(0, "%s: Error parsing %s: Unhandled exception", __ROCOCO_FUNCTION__, u8Path.buf);
		}

		AutoFree<ISEXMLRootSupervisor> root = CreateSEXMLParser(*allocator, tree->Root());

		onLoad.Invoke(*root);
	}

	ROCOCO_SEXML_API void ParseSXMLFromString(cstr name, cstr s, Function<void(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(65_kilobytes, 0, "LoadUserDataAllocator");

		Auto<ISParser> parser = CreateSexParser_2_0(*allocator);
		Auto<ISourceCode> src = parser->ProxySourceBuffer(s, StringLength(s), Vec2i{0,0}, name);
		Auto<ISParserTree> tree;

		try
		{
			tree = parser->CreateTree(*src);
		}
		catch (ParseException& ex)
		{
			Throw(0, "%s: Error parsing %s: %s. (line %d pos %d)", __ROCOCO_FUNCTION__, name, ex.Message(), ex.Start().y, ex.Start().x);
		}
		catch (...)
		{
			Throw(0, "%s: Error parsing %s: Unhandled exception", __ROCOCO_FUNCTION__, name);
		}

		AutoFree<ISEXMLRootSupervisor> root;

		try
		{
			root = CreateSEXMLParser(*allocator, tree->Root());
			onLoad.Invoke(*root);
		}
		catch (ParseException& ex)
		{
			Throw(0, "%s: Error parsing %s: %s. (line %d pos %d)", __ROCOCO_FUNCTION__, name, ex.Message(), ex.Start().y, ex.Start().x);
		}
		catch (IException& defaultEx)
		{
			Throw(defaultEx.ErrorCode(), "Error parsing %s: %s", name, defaultEx.Message());
		}
	}

	ROCOCO_SEXML_API void LoadSXMLBySysPath(cstr filename, Function<void(const ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		LoadSXMLBySysPath(wPath, onLoad);
	}

	ROCOCO_SEXML_API void SaveSXMLBySysPath(crwstr filename, Function<void(Rococo::Sex::SEXML::ISEXMLBuilder& builder)> onBuild)
	{
		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(64_kilobytes);
		AutoFree<ISEXMLBuilder> pBuilder = CreateSEXMLBuilder(dsb->Builder(), false);

		try
		{
			onBuild.Invoke(*pBuilder);
			pBuilder->ValidateClosed();
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s: %s", __ROCOCO_FUNCTION__, ex.Message());
		}

		IO::SaveAsciiTextFile(IO::TargetDirectory_Root, filename, *dsb->Builder());
	}

	ROCOCO_SEXML_API void SaveSXMLBySysPath(cstr filename, Function<void(Rococo::Sex::SEXML::ISEXMLBuilder& builder)> onBuild)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		SaveSXMLBySysPath(wPath, onBuild);
	}
}