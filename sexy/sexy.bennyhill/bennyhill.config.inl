/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

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

namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;

	void ParseCppRoot(cr_sex sconfigItem, ParseContext& pc)
	{
		if (*pc.cppRoot != 0) Throw(sconfigItem, SEXTEXT("cpp.root has already been specified"));
		if (sconfigItem.NumberOfElements() != 2) Throw(sconfigItem, SEXTEXT("Expecting (cpp.root <cpp-root-directory>). If the first character of <cpp-root-directory> is $, then $ is substituted with the project root"));
		csexstr cppPath = StringFrom(sconfigItem.GetElement(1));

		if (*cppPath == '$')
		{
			StringPrint(pc.cppRoot, _MAX_PATH, SEXTEXT("%s%s"), pc.projectRoot, cppPath + 1);
		}
		else
		{
			StringPrint(pc.cppRoot, _MAX_PATH, SEXTEXT("%s"), cppPath);
		}
	}

	void ParseCppException(cr_sex sconfigItem, ParseContext& pc)
	{
		if (*pc.cppException != 0) Throw(sconfigItem, SEXTEXT("cpp.exception has already been specified"));
		if (sconfigItem.NumberOfElements() != 2) Throw(sconfigItem, SEXTEXT("Expecting (cpp.exception <cpp-exception name>)."));

		csexstr cppException = StringFrom(sconfigItem.GetElement(1));
		StringPrint(pc.cppException, _MAX_PATH, SEXTEXT("%s"), cppException);
	}

	void ParseTypeFile(cr_sex sconfigItem, ParseContext& pc)
	{
		if (*pc.cppRoot == 0) Throw(sconfigItem, SEXTEXT("cpp.root must be specified before cpp.types"));
		if (*pc.cppTypes != 0) Throw(sconfigItem, SEXTEXT("cpp.types has already been specified"));
		if (*pc.sexyTypes != 0) Throw(sconfigItem, SEXTEXT("sexy.types has already been specified"));
		if (sconfigItem.NumberOfElements() != 3) Throw(sconfigItem, SEXTEXT("Expecting (cpp.types <sexy-types> <cpp-types>). Either argument can be prefixed with $project$ to map to the project root, or $cpp$ to map to the C++ root"));
		csexstr cppTypesPath = StringFrom(sconfigItem.GetElement(2));

		csexstr projectPrefix = SEXTEXT("$project$");
		csexstr cppPrefix = SEXTEXT("$cpp$");
		if (AreEqual(cppTypesPath, projectPrefix, StringLength(projectPrefix)))
		{
			StringPrint(pc.cppTypes, _MAX_PATH, SEXTEXT("%s%s"), pc.projectRoot, cppTypesPath + StringLength(projectPrefix));
		}
		else if (AreEqual(cppTypesPath, cppPrefix, StringLength(cppPrefix)))
		{
			StringPrint(pc.cppTypes, _MAX_PATH, SEXTEXT("%s%s"), pc.cppRoot, cppTypesPath + StringLength(cppPrefix));
		}
		else
		{
			StringPrint(pc.cppTypes, _MAX_PATH, SEXTEXT("%s"), cppTypesPath);
		}

		csexstr sexyTypesPath = StringFrom(sconfigItem.GetElement(1));

		if (AreEqual(sexyTypesPath, projectPrefix, StringLength(projectPrefix)))
		{
			StringPrint(pc.sexyTypes, _MAX_PATH, SEXTEXT("%s%s"), pc.projectRoot, sexyTypesPath + StringLength(projectPrefix));
		}
		else
		{
			StringPrint(pc.sexyTypes, _MAX_PATH, SEXTEXT("%s"), sexyTypesPath);
		}
	}

	void ParsePrimitive(cr_sex sprimitiveDef, ParseContext& pc)
	{
		if (sprimitiveDef.NumberOfElements() != 4) Throw(sprimitiveDef, SEXTEXT("Expecting 4 elements: (primitive <sxh-type> <sxy-type> <cpp-type>)"));

		csexstr sxhType = StringFrom(sprimitiveDef, 1);
		csexstr sxyType = StringFrom(sprimitiveDef, 2);
		csexstr cppType = StringFrom(sprimitiveDef, 3);

		if (AreEqual(cppType, SEXTEXT("bool")))
		{
			Throw(sprimitiveDef, SEXTEXT("Primitive type bool in the config file is unnacceptable, as Sexy scripting primitives must be 32-bit or 64-bit. Type bool is 8-bit on many C++ compilers."));
		}

		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);

		auto i = pc.primitives.find(sxhType);
		if (i != pc.primitives.end()) Throw(sprimitiveDef, SEXTEXT("sxh-type has already been defined"));

		TypeDef def;
		def.cppType = cppType;
		def.sexyType = sxyType;

		pc.primitives[sxhType] = def;
	}

	void ParseStruct(cr_sex sprimitiveDef, ParseContext& pc)
	{
		if (sprimitiveDef.NumberOfElements() != 4) Throw(sprimitiveDef, SEXTEXT("Expecting 4 elements: (struct <sxh-type> <sxy-type> <cpp-type>)"));

		csexstr sxhType = StringFrom(sprimitiveDef, 1);
		csexstr sxyType = StringFrom(sprimitiveDef, 2);
		csexstr cppType = StringFrom(sprimitiveDef, 3);

		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);

		TTypeMap::iterator i = pc.structs.find(sxhType);
		if (i != pc.structs.end()) Throw(sprimitiveDef, SEXTEXT("sxh-type has already been defined"));

		i = pc.primitives.find(sxhType);
		if (i != pc.primitives.end()) Throw(sprimitiveDef, SEXTEXT("sxh-type has already been defined as a primitive"));

		TypeDef def;
		def.cppType = cppType;
		def.sexyType = sxyType;

		pc.structs[sxhType] = def;
	}

	int AppendCppNamespaceFromType(FileAppender& appender, csexstr cppType, SEXCHAR cppTypeShortName[256])
	{
		NamespaceSplitter splitter(cppType);

		csexstr nsRoot, tail;
		if (splitter.SplitHead(nsRoot, tail))
		{
			appender.Append(SEXTEXT("namespace %s { "), nsRoot);
			return AppendCppNamespaceFromType(appender, tail, cppTypeShortName) + 1;
		}
		else
		{
			StringPrint(cppTypeShortName, 256, SEXTEXT("%s"), cppType);
			return 0;
		}
	}

	void ConvertAndAppendCppType(FileAppender& appender, csexstr cppType)
	{
		NamespaceSplitter splitter(cppType);

		csexstr nsRoot, tail;
		if (splitter.SplitHead(nsRoot, tail))
		{
			appender.Append(SEXTEXT("%s::"), nsRoot);
			return ConvertAndAppendCppType(appender, tail);
		}
		else
		{
			appender.Append(SEXTEXT("%s"), cppType);
		}
	}

	void AppendCppType(FileAppender& appender, cr_sex field, csexstr sxhfieldtype, const ParseContext& pc)
	{
		csexstr cpptypeDef = NULL;
		auto i = pc.primitives.find(sxhfieldtype);
		if (i != pc.primitives.end()) cpptypeDef = i->second.cppType.c_str();
		else 
		{
			auto j = pc.structs.find(sxhfieldtype);
			if (j != pc.structs.end()) cpptypeDef = j->second.cppType.c_str();
		}

		if (cpptypeDef == NULL) Throw(field, SEXTEXT("Cannot resolve the type. Neither a known struct or primitive"));

		ConvertAndAppendCppType(appender, cpptypeDef);
	}

	void AppendStructDefToCppTypeFile(FileAppender& cppTypeAppender, csexstr cppType, cr_sex sprimitiveDef, const ParseContext& pc)
	{
		SEXCHAR cppTypeShortName[256];
		int nsDepth = AppendCppNamespaceFromType(cppTypeAppender, cppType, cppTypeShortName);
		if (nsDepth > 0) cppTypeAppender.Append('\n');

		cppTypeAppender.Append(SEXTEXT("\tstruct %s\n\t{\n"), cppTypeShortName);

		for(int i = 4; i < sprimitiveDef.NumberOfElements(); ++i)
		{
			cr_sex field = sprimitiveDef.GetElement(i);
			if (field.NumberOfElements() != 2) Throw(field, SEXTEXT("Expecting two elements in a field definition (<field-type> <field-name>)"));

			csexstr fieldType = StringFrom(field, 0);
			csexstr fieldName = StringFrom(field, 1);
			
			ValidateSexyVariable(field, fieldName);

			cppTypeAppender.Append(SEXTEXT("\t\t"));
			AppendCppType(cppTypeAppender, field, fieldType, pc);
			cppTypeAppender.Append(SEXTEXT(" %s;\n"), fieldName);
		}

		cppTypeAppender.Append(SEXTEXT("\t};\n"));

		if (nsDepth > 0)
		{		
			for(int i = 0; i < nsDepth; i++)
			{
				cppTypeAppender.Append('}');
			}

			cppTypeAppender.Append('\n');
		}
	}

	void AppendStructDefToSexyTypeFile(FileAppender& sexyTypeAppender, csexstr sexyType, cr_sex sprimitiveDef, const ParseContext& pc)
	{
		NamespaceSplitter splitter(sexyType);
		csexstr ns, shortName;
		if (!splitter.SplitTail(ns, shortName))
		{
			Throw(sprimitiveDef, SEXTEXT("Expecting fully qualified sexy struct name"));
		}

		sexyTypeAppender.Append(SEXTEXT("(struct %s\n"), shortName);

		for(int i = 4; i < sprimitiveDef.NumberOfElements(); ++i)
		{
			cr_sex field = sprimitiveDef.GetElement(i);
			if (field.NumberOfElements() != 2) Throw(field, SEXTEXT("Expecting two elements in a field definition (<field-type> <field-name>)"));

			csexstr fieldType = StringFrom(field, 0);
			csexstr fieldName = StringFrom(field, 1);

			auto j = pc.primitives.find(fieldType);			
			if (j == pc.primitives.end())
			{
				j = pc.structs.find(fieldType);
				if (j == pc.structs.end())
				{
					Throw(field.GetElement(0), SEXTEXT("Cannot find corresponding sexy type")); 
				}
			}

			ValidateSexyVariable(field, fieldName);
			sexyTypeAppender.Append(SEXTEXT("\t(%s %s)\n"), j->second.sexyType.c_str(), fieldName);
		}

		sexyTypeAppender.Append(SEXTEXT(")\n(alias %s %s.%s)\n\n"), shortName, ns, shortName);
	}

	void ParseStructDef(cr_sex sprimitiveDef, ParseContext& pc, FileAppender& cppTypeAppender, FileAppender& sexyTypeAppender)
	{
		if (*pc.cppRoot == 0) Throw(sprimitiveDef, SEXTEXT("cpp.root must be specified before a defstruct"));
		if (*pc.cppTypes == 0) Throw(sprimitiveDef, SEXTEXT("cpp.types must be specified before a defstruct"));

		if (sprimitiveDef.NumberOfElements() < 5) Throw(sprimitiveDef, SEXTEXT("Expecting 5 or more elements: (defstruct <sxh-type> <sxy-type> <cpp-type> (field1)...(fieldN))"));

		csexstr sxhType = StringFrom(sprimitiveDef, 1);
		csexstr sxyType = StringFrom(sprimitiveDef, 2);
		csexstr cppType = StringFrom(sprimitiveDef, 3);
			
		ValidateCPPType(sprimitiveDef.GetElement(3), cppType);
		ValidateSexyType(sprimitiveDef.GetElement(2), sxyType);
		
		AppendStructDefToCppTypeFile(cppTypeAppender, cppType, sprimitiveDef, pc);
		AppendStructDefToSexyTypeFile(sexyTypeAppender, sxyType, sprimitiveDef, pc);

		auto i = pc.structs.find(sxhType);
		if (i != pc.structs.end()) Throw(sprimitiveDef, SEXTEXT("sxh-type has already been defined"));

		i = pc.primitives.find(sxhType);
		if (i != pc.primitives.end()) Throw(sprimitiveDef, SEXTEXT("sxh-type has already been defined as a primitive"));

		TypeDef def;
		def.cppType = cppType;
		def.sexyType = sxyType;

		pc.structs[sxhType] = def;
	}

	void ParseConfig(cr_sex configDef, ParseContext& pc)
	{
		int nStructDefs = 0;

		for(int i = 0; i < configDef.NumberOfElements(); ++i)
		{
			cr_sex sconfigItem = configDef.GetElement(i);
			if (!IsCompound(sconfigItem)) Throw(sconfigItem, SEXTEXT("Expecting compound item in config definition"));
			cr_sex sconfigCommand = sconfigItem.GetElement(0);
			csexstr configCommand = StringFrom(sconfigCommand);

			if (AreEqual(configCommand, SEXTEXT("cpp.root")))
			{
				ParseCppRoot(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, SEXTEXT("cpp.exception")))
			{
				ParseCppException(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, SEXTEXT("type.files")))
			{
				ParseTypeFile(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, SEXTEXT("primitive")))
			{
				ParsePrimitive(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, SEXTEXT("struct")))
			{
				ParseStruct(sconfigItem, pc);
			}
			else if (AreEqual(configCommand, SEXTEXT("defstruct")))
			{
				nStructDefs++;
			}
			else
			{
				Throw(configDef, SEXTEXT("Expecting one of { cpp.root, type.files, primitive, struct, defstruct }"));
			}
		}

		if (nStructDefs > 0)
		{
			if (*pc.cppTypes == 0) Throw(configDef, SEXTEXT("cpp.types was not defined")); 
			if (*pc.sexyTypes == 0) Throw(configDef, SEXTEXT("sexy.types was not defined"));
			if (*pc.cppException == 0) Throw(configDef, SEXTEXT("cpp.exception was not defined"));

			FileDeleteOnceOnly(pc.cppTypes);
			FileAppender cppTypeAppender(pc.cppTypes);

			FileDeleteOnceOnly(pc.sexyTypes);
			FileAppender sexyTypeAppender(pc.sexyTypes);

			for(int i = 0; i < configDef.NumberOfElements(); ++i)
			{
				cr_sex sconfigItem = configDef.GetElement(i);
				csexstr configCommand = StringFrom(sconfigItem, 0);

				if (AreEqual(configCommand, SEXTEXT("defstruct")))
				{
					ParseStructDef(sconfigItem, pc, cppTypeAppender, sexyTypeAppender);
				}
			}
		}

		if (*pc.cppRoot == 0) Throw(configDef, SEXTEXT("cpp.root was not specified"));
	}

	void ParseConfigSpec(cr_sex configSpec, ParseContext& pc)
	{
		if (configSpec.NumberOfElements() != 2) Throw(configSpec, SEXTEXT("Expecting two elements in a config spec (config <config_path>). The <config_path> can be prefixed with $, which if found, is substituted with the project root"));
	
		csexstr configPath = StringFrom(configSpec.GetElement(1));
		
		SEXCHAR fullconfigPath[_MAX_PATH];

		if (*configPath == '$')
		{
			StringPrint(fullconfigPath, _MAX_PATH, SEXTEXT("%s%s"), pc.projectRoot, configPath + 1);
		}
		else
		{
			StringPrint(fullconfigPath, _MAX_PATH, SEXTEXT("%s"), configPath);
		}

		CSParserProxy spp;

		try
		{
			Auto<ISourceCode> configSrc = spp().LoadSource(fullconfigPath, SourcePos(0,1));
			Auto<ISParserTree> tree = spp().CreateTree(configSrc());

			ParseConfig(tree().Root(), pc);
		}
		catch(ParseException& ex)
		{
			WriteToStandardOutput(SEXTEXT("%s: %s. Specimen: %s"), fullconfigPath, ex.Message(), ex.Specimen());
			OS::OSException oex;
			oex.exceptionNumber = -1;
			StringPrint(oex.message, 256, SEXTEXT("Error parsing config"));
			throw oex;
		}
	}
}