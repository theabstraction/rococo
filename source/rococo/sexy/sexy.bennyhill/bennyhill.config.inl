#pragma once

/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#include <rococo.strings.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Strings;

	void ParseCppRoot(cr_sex sconfigItem, ParseContext& pc)
	{
		if (*pc.cppRootDirectory != 0) Throw(sconfigItem, ("cpp.root has already been specified"));
		if (sconfigItem.NumberOfElements() != 2) Throw(sconfigItem, ("Expecting (cpp.root <cpp-root-directory>). If the first character of <cpp-root-directory> is $, then $ is substituted with the project root"));
		cstr cppPath = StringFrom(sconfigItem.GetElement(1));

		if (*cppPath == '$')
		{
			SafeFormat(pc.cppRootDirectory, _MAX_PATH, ("%s%s"), pc.projectRoot, cppPath + 1);
		}
		else
		{
			SafeFormat(pc.cppRootDirectory, _MAX_PATH, ("%s"), cppPath);
		}
	}

	void ParseCppException(cr_sex sconfigItem, ParseContext& pc)
	{
		if (*pc.cppException != 0) Throw(sconfigItem, ("cpp.exception has already been specified"));
		if (sconfigItem.NumberOfElements() != 2) Throw(sconfigItem, ("Expecting (cpp.exception <cpp-exception name>)."));

		cstr cppException = StringFrom(sconfigItem.GetElement(1));
		SafeFormat(pc.cppException, 128, ("%s"), cppException);
	}

	void ParseTypeFile(cr_sex sconfigItem, ParseContext& pc)
	{
		if (*pc.cppRootDirectory == 0) Throw(sconfigItem, ("cpp.root must be specified before cpp.types"));
		if (*pc.cppTypesFilename != 0) Throw(sconfigItem, ("cpp.types has already been specified"));
		if (*pc.sexyTypesFilename != 0) Throw(sconfigItem, ("sexy.types has already been specified"));
		if (sconfigItem.NumberOfElements() != 3) Throw(sconfigItem, ("Expecting (cpp.types <sexy-types> <cpp-types>). Either argument can be prefixed with $project$ to map to the project root, or $cpp$ to map to the C++ root"));
		cstr cppTypesPath = StringFrom(sconfigItem.GetElement(2));

		fstring contentPrefix = "$content$"_fstring;
		fstring projectPrefix = "$project$"_fstring;
		fstring cppPrefix = "$cpp$"_fstring;

		if (AreEqual(cppTypesPath, projectPrefix.buffer, projectPrefix.length))
		{
			SafeFormat(pc.cppTypesFilename, _MAX_PATH, "%s%s", pc.projectRoot, cppTypesPath + projectPrefix.length);
		}
		else if (AreEqual(cppTypesPath, cppPrefix.buffer, cppPrefix.length))
		{
			SafeFormat(pc.cppTypesFilename, _MAX_PATH, "%s%s", pc.cppRootDirectory, cppTypesPath + cppPrefix.length);
		}
		else if (AreEqual(cppTypesPath, contentPrefix.buffer, contentPrefix.length))
		{
			SafeFormat(pc.cppTypesFilename, _MAX_PATH, "%s%s", pc.cppRootDirectory, cppTypesPath + contentPrefix.length);
		}
		else
		{
			SafeFormat(pc.cppTypesFilename, _MAX_PATH, "%s", cppTypesPath);
		}

		cstr sexyTypesPath = StringFrom(sconfigItem.GetElement(1));

		if (AreEqual(sexyTypesPath, projectPrefix.buffer, projectPrefix.length))
		{
			SafeFormat(pc.sexyTypesFilename, _MAX_PATH, "%s%s", pc.projectRoot, sexyTypesPath + projectPrefix.length);
			IO::ToSysPath(pc.sexyTypesFilename);
		}
		else if (AreEqual(sexyTypesPath, contentPrefix.buffer, contentPrefix.length))
		{
			SafeFormat(pc.sexyTypesFilename, _MAX_PATH, "%s%s", pc.contentRoot, sexyTypesPath + contentPrefix.length);
			IO::ToSysPath(pc.sexyTypesFilename);
		}
		else
		{
			SafeFormat(pc.sexyTypesFilename, _MAX_PATH, "%s", sexyTypesPath);
			IO::ToSysPath(pc.sexyTypesFilename);
		}
	}

	void ParsePrimitive(cr_sex sprimitiveDef, ParseContext& pc)
	{
		if (sprimitiveDef.NumberOfElements() != 4) Throw(sprimitiveDef, ("Expecting 4 elements: (primitive <sxh-type> <sxy-type> <cpp-type>)"));

		cstr sxhType = StringFrom(sprimitiveDef, 1);
		cstr sxyType = StringFrom(sprimitiveDef, 2);
		cstr cppType = StringFrom(sprimitiveDef, 3);

		if (AreEqual(cppType, ("bool")))
		{
			Throw(sprimitiveDef, ("Primitive type bool in the config file is unnacceptable, as Sexy scripting primitives must be 32-bit or 64-bit. Type bool is 8-bit on many C++ compilers."));
		}

		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);

		auto i = pc.primitives.find(sxhType);
		if (i != pc.primitives.end()) Throw(sprimitiveDef, ("sxh-type has already been defined"));

		TypeDef def;
		def.cppType = cppType;
		def.sexyType = sxyType;

		pc.primitives[sxhType] = def;
	}

	void ParseInterfaceDecl(cr_sex sprimitiveDef, ParseContext& pc)
	{
		if (sprimitiveDef.NumberOfElements() != 4) Throw(sprimitiveDef, "Expecting 4 elements: (interface <sxh-type> <sxy-type> <cpp-type>)");
		
		cstr sxhType = StringFrom(sprimitiveDef, 1);
		cstr sxyType = StringFrom(sprimitiveDef, 2);
		cstr cppType = StringFrom(sprimitiveDef, 3);

		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);

		auto i = pc.interfaces.find(sxhType);
		if (i != pc.interfaces.end()) Throw(sprimitiveDef, ("sxh-type has already been defined"));

		InterfaceDef* def = new InterfaceDef();
		SafeFormat(def->ic.asSexyInterface, "%s", sxyType);
		def->ic.asCppInterface.Set(cppType);
		def->sdef = &sprimitiveDef;
		pc.interfaces.insert(std::make_pair(stdstring(sxhType), def));

	}

	void ParseStruct(cr_sex sprimitiveDef, ParseContext& pc)
	{
		if (sprimitiveDef.NumberOfElements() != 4) Throw(sprimitiveDef, ("Expecting 4 elements: (struct <sxh-type> <sxy-type> <cpp-type>)"));

		cstr sxhType = StringFrom(sprimitiveDef, 1);
		cstr sxyType = StringFrom(sprimitiveDef, 2);
		cstr cppType = StringFrom(sprimitiveDef, 3);

		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);

		TTypeMap::iterator i = pc.structs.find(sxhType);
		if (i != pc.structs.end()) Throw(sprimitiveDef, ("sxh-type has already been defined"));

		i = pc.primitives.find(sxhType);
		if (i != pc.primitives.end()) Throw(sprimitiveDef, ("sxh-type has already been defined as a primitive"));

		TypeDef def;
		def.cppType = cppType;
		def.sexyType = sxyType;

		pc.structs[sxhType] = def;
	}

	int AppendCppNamespaceFromType(FileAppender& appender, cstr cppType, char cppTypeShortName[256])
	{
		NamespaceSplitter splitter(cppType);

		cstr nsRoot, tail;
		if (splitter.SplitHead(nsRoot, tail))
		{
			appender.Append(("namespace %s { "), nsRoot);
			return AppendCppNamespaceFromType(appender, tail, cppTypeShortName) + 1;
		}
		else
		{
			SafeFormat(cppTypeShortName, 256, ("%s"), cppType);
			return 0;
		}
	}

	void AppendStructDefToCppTypeFile(FileAppender& cppTypeAppender, cstr cppType, cr_sex sprimitiveDef, const ParseContext& pc)
	{
		char cppTypeShortName[256];
		int nsDepth = AppendCppNamespaceFromType(cppTypeAppender, cppType, cppTypeShortName);
		if (nsDepth > 0) cppTypeAppender.Append('\n');

		cppTypeAppender.Append(("\tstruct %s\n\t{\n"), cppTypeShortName);

		for (int i = 4; i < sprimitiveDef.NumberOfElements(); ++i)
		{
			cr_sex field = sprimitiveDef.GetElement(i);
			if (field.NumberOfElements() != 2) Throw(field, ("Expecting two elements in a field definition (<field-type> <field-name>)"));

			cstr fieldType = StringFrom(field, 0);
			cstr fieldName = StringFrom(field, 1);

			ValidateSexyVariable(field, fieldName);

			cppTypeAppender.Append(("\t\t"));
			AppendCppType(EQualifier::None, cppTypeAppender, field, fieldType, pc);
			cppTypeAppender.Append((" %s;\n"), fieldName);
		}

		cppTypeAppender.Append(("\t};\n"));

		if (nsDepth > 0)
		{
			for (int i = 0; i < nsDepth; i++)
			{
				cppTypeAppender.Append('}');
			}

			cppTypeAppender.Append('\n');
		}
	}

	void AppendStructDefToSexyTypeFile(FileAppender& sexyTypeAppender, cstr sexyType, cr_sex sprimitiveDef, const ParseContext& pc)
	{
		NamespaceSplitter splitter(sexyType);
		cstr ns, shortName;
		if (!splitter.SplitTail(ns, shortName))
		{
			Throw(sprimitiveDef, ("Expecting fully qualified sexy struct name"));
		}

		sexyTypeAppender.Append(("(struct %s\n"), shortName);

		for (int i = 4; i < sprimitiveDef.NumberOfElements(); ++i)
		{
			cr_sex field = sprimitiveDef.GetElement(i);
			if (field.NumberOfElements() != 2) Throw(field, ("Expecting two elements in a field definition (<field-type> <field-name>)"));

			cstr fieldType = StringFrom(field, 0);
			cstr fieldName = StringFrom(field, 1);

			auto j = pc.primitives.find(fieldType);
			if (j == pc.primitives.end())
			{
				j = pc.structs.find(fieldType);
				if (j == pc.structs.end())
				{
					Throw(field.GetElement(0), ("Cannot find corresponding sexy type"));
				}
			}

			ValidateSexyVariable(field, fieldName);
			sexyTypeAppender.Append(("\t(%s %s)\n"), j->second.sexyType.c_str(), fieldName);
		}

		sexyTypeAppender.Append((")\n(alias %s %s.%s)\n\n"), shortName, ns, shortName);
	}

	void ParseStructDef(cr_sex sprimitiveDef, ParseContext& pc, FileAppender& cppTypeAppender, FileAppender& sexyTypeAppender)
	{
		if (*pc.cppRootDirectory == 0) Throw(sprimitiveDef, ("cpp.root must be specified before a defstruct"));
		if (*pc.cppTypesFilename == 0) Throw(sprimitiveDef, ("cpp.types must be specified before a defstruct"));

		if (sprimitiveDef.NumberOfElements() < 5) Throw(sprimitiveDef, "Expecting 5 or more elements: (defstruct <sxh-type> <sxy-type> <cpp-type> (field1)...(fieldN))");

		if (!IsAtomic(sprimitiveDef[1]) || !IsAtomic(sprimitiveDef[2]) || !IsAtomic(sprimitiveDef[3]))
		{
			Throw(sprimitiveDef, "Expected 3 atomics in positions 1, 2 and 3 of expression.\n i.e (defstruct <sxh - type> <sxy - type> <cpp - type> (field1)...(fieldN))");
		}

		cstr sxhType = StringFrom(sprimitiveDef, 1);
		cstr sxyType = StringFrom(sprimitiveDef, 2);
		cstr cppType = StringFrom(sprimitiveDef, 3);

		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);

		AppendStructDefToCppTypeFile(cppTypeAppender, cppType, sprimitiveDef, pc);
		AppendStructDefToSexyTypeFile(sexyTypeAppender, sxyType, sprimitiveDef, pc);

		auto i = pc.structs.find(sxhType);
		if (i != pc.structs.end()) Throw(sprimitiveDef, ("sxh-type has already been defined"));

		i = pc.primitives.find(sxhType);
		if (i != pc.primitives.end()) Throw(sprimitiveDef, ("sxh-type has already been defined as a primitive"));

		TypeDef def;
		def.cppType = cppType;
		def.sexyType = sxyType;

		pc.structs[sxhType] = def;
	}

	void ParseConfig(cr_sex configDef, ParseContext& pc)
	{
		int nStructDefs = 0;

		for (int i = 0; i < configDef.NumberOfElements(); ++i)
		{
			cr_sex sconfigItem = configDef.GetElement(i);
			if (!IsCompound(sconfigItem)) Throw(sconfigItem, ("Expecting compound item in config definition"));
			cr_sex sconfigCommand = sconfigItem.GetElement(0);
			cstr configCommand = StringFrom(sconfigCommand);

			if (AreEqual(configCommand, ("cpp.root")))
			{
				ParseCppRoot(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, ("cpp.exception")))
			{
				ParseCppException(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, ("type.files")))
			{
				ParseTypeFile(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, ("primitive")))
			{
				ParsePrimitive(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, ("struct")))
			{
				ParseStruct(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, ("interface")))
			{
				ParseInterfaceDecl(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, ("defstruct")))
			{
				nStructDefs++;
			}
			else
			{
				Throw(sconfigItem, ("Expecting one of { cpp.root, type.files, primitive, struct, defstruct }"));
			}
		}

		if (nStructDefs > 0)
		{
			if (*pc.cppTypesFilename == 0) Throw(configDef, ("cpp.types was not defined"));
			if (*pc.sexyTypesFilename == 0) Throw(configDef, ("sexy.types was not defined"));
			if (*pc.cppException == 0) Throw(configDef, ("cpp.exception was not defined"));

			FileAppender cppTypeAppender(pc.cppTypesFilename);
			FileAppender sexyTypeAppender(pc.sexyTypesFilename);

			cppTypeAppender.Append("#pragma once\n\n");

			SYSTEMTIME t;
			GetSystemTime(&t);

			char signature[256];
			SafeFormat(signature, "// Generated by BennyHill on %u/%u/%u %u:%u:%u UTC\n\n", (uint32) t.wDay, (uint32) t.wMonth, (uint32) t.wYear, (uint32) t.wHour, (uint32) t.wMinute, (uint32) t.wSecond);

			cppTypeAppender.Append(signature);

			for (int i = 0; i < configDef.NumberOfElements(); ++i)
			{
				cr_sex sconfigItem = configDef.GetElement(i);
				cstr configCommand = StringFrom(sconfigItem, 0);

				if (AreEqual(configCommand, ("defstruct")))
				{
					ParseStructDef(sconfigItem, pc, cppTypeAppender, sexyTypeAppender);
				}
			}
		}

		if (*pc.cppRootDirectory == 0) Throw(configDef, ("cpp.root was not specified"));
	}

	void LoadConfigXC(cr_sex configSpec, ParseContext& pc)
	{
		Auto<ISParser> parser = CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());
		Auto<ISourceCode> configSrc;
		Auto<ISParserTree> tree;

		try
		{
			WideFilePath wXCPath;
			Assign(wXCPath, pc.xcPath);
			configSrc = parser->LoadSource(wXCPath, Vec2i{ 1,1 });
			tree = parser->CreateTree(configSrc());

			ParseConfig(tree().Root(), pc);

			IO::ToSysPath(pc.sexyTypesFilename);
			IO::ToSysPath(pc.cppTypesFilename);
			IO::ToSysPath(pc.cppRootDirectory);
			IO::ToSysPath(pc.projectRoot);
		}
		catch (ParseException& ex)
		{
			WriteToStandardOutput("%s: %s. Specimen: %s", pc.xcPath, ex.Message(), ex.Specimen());
			Throw(ex);
		}
	}
}