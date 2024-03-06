#include <rococo.compiler.options.h>
#define ROCOCO_SEXML_API ROCOCO_API_EXPORT
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
			Throw(0, "%s: blank [section]", __FUNCTION__);
		}

		organization = (organization && *organization) ? organization : s_defaultOrganization;
		WideFilePath wDir;
		IO::GetUserPath(wDir.buf, WideFilePath::CAPACITY, organization);

		if (!IO::IsDirectory(wDir))
		{
			return false;
		}

		WideFilePath wFullName;
		Format(wFullName, L"%ls\\%hs.sexml", wDir.buf, section);

		return IO::IsFileExistant(wFullName);
	}

	ROCOCO_SEXML_API void GetUserSEXMLFullPath(U8FilePath& path, cstr organization, cstr section)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __FUNCTION__);
		}

		organization = (organization && *organization) ? organization : s_defaultOrganization;

		WideFilePath wDir;
		IO::GetUserPath(wDir.buf, WideFilePath::CAPACITY, organization);

		WideFilePath wFullName;
		Format(wFullName, L"%ls\\%hs.sexml", wDir.buf, section);

		Format(path, "%ls", wFullName.buf);
	}

	ROCOCO_SEXML_API void LoadUserSEXML(cstr organization, cstr section, Function<void(const ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __FUNCTION__);
		}

		organization = (organization && *organization) ? organization : s_defaultOrganization;

		WideFilePath wDir;
		IO::GetUserPath(wDir.buf, WideFilePath::CAPACITY, organization);

		if (!IO::IsDirectory(wDir))
		{
			Throw(0, "%s(%s, %s,...):\n\tNo such directory: %ls\n", __FUNCTION__, organization, section, wDir.buf);
		}

		WideFilePath wFullName;
		Format(wFullName, L"%ls\\%hs.sexml", wDir.buf, section);

		struct ANON: IStringPopulator
		{
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
			Throw(0, "%s: Error parsing %s: %s. (line %d pos %d)", __FUNCTION__, u8Path.buf, ex.Message(), ex.Start().y, ex.Start().x);
		}
		catch (...)
		{
			Throw(0, "%s: Error parsing %s: Unhandled exception", __FUNCTION__, u8Path.buf);
		}

		AutoFree<ISEXMLRootSupervisor> root = CreateSEXMLParser(*allocator, tree->Root());

		onLoad.Invoke(*root);
	}

	ROCOCO_SEXML_API void SaveUserSEXML(cstr organization, cstr section, Function<void(ISEXMLBuilder& builder)> onBuild)
	{
		if (section == nullptr || *section == 0)
		{
			Throw(0, "%s: blank [section]", __FUNCTION__);
		}
		
		organization = (organization && *organization) ? organization : s_defaultOrganization;

		WideFilePath wOrganizationDir;
		IO::GetUserPath(wOrganizationDir, organization);

		if (!IO::IsDirectory(wOrganizationDir))
		{
			IO::CreateDirectoryFolder(wOrganizationDir);
		}

		WideFilePath wDir;
		Format(wDir, L"%hs\\%hs.sexml", organization, section);

		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(64_kilobytes);
		AutoFree<ISEXMLBuilder> pBuilder = CreateSEXMLBuilder(dsb->Builder(), false);

		try
		{
			onBuild.Invoke(*pBuilder);
			pBuilder->ValidateClosed();
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s(%s, %s, ...): %s", __FUNCTION__, organization, section, ex.Message());
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

	ROCOCO_SEXML_API void LoadSXMLBySysPath(const wchar_t* filename, Function<void(const ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		struct ANON : IStringPopulator
		{
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
			Throw(0, "%s: Error parsing %s: %s. (line %d pos %d)", __FUNCTION__, u8Path.buf, ex.Message(), ex.Start().y, ex.Start().x);
		}
		catch (...)
		{
			Throw(0, "%s: Error parsing %s: Unhandled exception", __FUNCTION__, u8Path.buf);
		}

		AutoFree<ISEXMLRootSupervisor> root = CreateSEXMLParser(*allocator, tree->Root());

		onLoad.Invoke(*root);
	}

	ROCOCO_SEXML_API void LoadSXMLBySysPath(cstr filename, Function<void(const ISEXMLDirectiveList& topLevelDirectives)> onLoad)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		LoadSXMLBySysPath(wPath, onLoad);
	}
}