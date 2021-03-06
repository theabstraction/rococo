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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

namespace Rococo
{
	using namespace Rococo::Sex;

	void DeclareCppEnum(FileAppender& appender, const EnumContext& ec, cr_sex senumDef, const ParseContext& pc);

	int AppendNamespace(FileAppender& appender, cstr fqStructName)
	{
		NamespaceSplitter splitter(fqStructName);
		cstr nsRoot, nsSubspace;
		if (splitter.SplitHead(nsRoot, nsSubspace))
		{
			appender.Append(("namespace %s { "), nsRoot);
			return 1 + AppendNamespace(appender, nsSubspace);
		}

		appender.Append('\n');
		return 0;
	}

	void CloseNamespace(FileAppender& appender, int nsDepth)
	{
		for (int i = 0; i < nsDepth; ++i)
		{
			appender.Append('}');
		}
	}

	void AppendStructShortName(FileAppender& appender, cstr fqStructName)
	{
		NamespaceSplitter splitter(fqStructName);
		cstr ns, shortName;
		appender.Append(("%s"), splitter.SplitTail(ns, shortName) ? shortName : fqStructName);
	}

	void AppendMethodDeclaration(FileAppender& appender, cr_sex method, cstr root, const ParseContext& pc)
	{
		cr_sex smethodName = method.GetElement(0);
		cstr methodName = smethodName.String()->Buffer;

		int outputIndex = GetOutputPosition(method);
		if (outputIndex >= method.NumberOfElements())
		{
			appender.Append(("\tvirtual void %s("), methodName);
		}
		else
		{
			cr_sex s = method.GetElement(outputIndex);

			int typeIndex = 0;
			int valueIndex = 1;

			cr_sex stype = s.GetElement(typeIndex);
			cr_sex sname = s.GetElement(valueIndex);

			cstr outputType = StringFrom(stype);
			cstr outputName = StringFrom(sname);

			appender.Append(("\tvirtual "));

			auto i = pc.primitives.find(outputType);
			if (i != pc.primitives.end())
			{
				AppendCppType(appender, s, outputType, pc);
				appender.Append(("/* %s */ %s("), outputName, methodName);
			}
			else
			{
				auto j = pc.interfaces.find(outputType);
				if (j == pc.interfaces.end())
				{
					Throw(stype, ("Expecting primitive return type or interface"));
				}

				AppendCppType(appender, s, outputType, pc);
				appender.Append(("* /* %s */ %s("), outputName, methodName);
			}
		}

		// Write inputs

		int inputCount = 1;

		int i;
		for (i = 1; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			if (IsAtomic(s))
			{
				cstr arg = s.String()->Buffer;
				if (AreEqual(arg, ("->")))
				{
					i++;
					break;
				}
			}

			int typeIndex = 0;
			int valueIndex = 1;

			if (s.NumberOfElements() == 3)
			{
				typeIndex++;
				valueIndex++;

				if (!IsAtomic(s[0]) || !AreEqual(s[0].String(), ("const")))
				{
					Throw(s[0], ("Expecting 'const' as first argument in 3 element input expression"));
				}
			}

			cr_sex stype = s.GetElement(typeIndex);
			cstr type = StringFrom(s, typeIndex);
			if (!AreEqual(type, ("#")))
			{
				cr_sex sname = s.GetElement(valueIndex);

				ValidateSexyType(stype);
				ValidateSexyVariable(sname);

				cstr inputtype = stype.String()->Buffer;
				cstr name = sname.String()->Buffer;

				if (inputCount > 1) appender.Append((", "));
				inputCount++;

				if (s.NumberOfElements() == 3 || AreEqual(inputtype, ("IString")))
				{
					appender.Append(("const "));
				}

				AppendCppType(appender, s, inputtype, pc);

				if (pc.structs.find(inputtype) != pc.structs.end())
				{
					appender.Append('&');
				}

				appender.Append((" %s"), name);
			}
		}

		i++; // The first output is what is returned

		int firstOutput = i;

		for (; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			cstr outputType = StringFrom(s, 0);
			cstr name = StringFrom(s, 1);

			if (i > firstOutput)
			{
				if (i > 1) appender.Append((", "));
			}
			else appender.Append((", /* -> */ "));

			AppendCppType(appender, s, outputType, pc);

			appender.Append(("& %s"), name);
		}

		appender.Append((") = 0;"));
	}

	void WriteInterfaceDeclaration(FileAppender& writer, cstr qualifiedName, int depth)
	{
		NamespaceSplitter splitter(qualifiedName);
		cstr head, body;
		if (splitter.SplitHead(head, body))
		{
			writer.Append(("namespace %s { "), head);
			WriteInterfaceDeclaration(writer, body, depth + 1);
			writer.Append(("}"));
		}
		else
		{
			writer.Append(("\n\tstruct %s;\n"), qualifiedName);
		}
	}

	void GenerateDeclarations(const ParseContext& pc)
	{
		std::unordered_set<stdstring> enumFiles;
		for (auto& i : pc.enums)
		{
			enumFiles.insert(i.ec.appendSexyFile);
		}

		for (auto& i : pc.enums)
		{
			FileAppender sexyFileAppender(i.ec.appendSexyFile);
			DeclareSexyEnum(sexyFileAppender, i.ec, *i.sdef, pc);
		}

		pc.namespaces.clear();

		enumFiles.clear();
		for (auto& i : pc.enums)
		{
			enumFiles.insert(i.ec.appendCppHeaderFile);
		}

		for (auto& i : enumFiles)
		{
			FileAppender cppFileAppender(i.c_str());
			for (auto& i : pc.enums)
			{
				DeclareCppEnum(cppFileAppender, i.ec, *i.sdef, pc);
			}
		}

		enumFiles.clear();
		for (auto& i : pc.interfaces)
		{
			if (*i.second->ic.appendCppHeaderFile)
			{
				auto& ic = i.second->ic;
				enumFiles.insert(ic.appendCppHeaderFile);
			}
		}

		for (auto& i : enumFiles)
		{
			FileAppender cppFileAppender(i.c_str());
			for (auto& i : pc.interfaces)
			{
				if (*i.second->ic.appendCppHeaderFile)
				{
					WriteInterfaceDeclaration(cppFileAppender, i.second->ic.asCppInterface.SexyName(), 0);
					cppFileAppender.Append(("\n\n"));
				}
			}
		}
	}

	void DeclareCppEnum(FileAppender& appender, const EnumContext& ec, cr_sex senumDef, const ParseContext& pc)
	{
		int nsDepth = AppendNamespace(appender, ec.asCppEnum.SexyName());
		if (nsDepth > 0)
		{
			appender.Append(("\t"));
		}

		appender.Append(("enum "));
		AppendStructShortName(appender, ec.asCppEnum.SexyName());
		appender.Append((": "));
		AppendStructShortName(appender, ec.underlyingType.SexyName());
		appender.Append(nsDepth > 0 ? ("\n\t{\n") : ("\n{\n"));

		for (auto& i : ec.values)
		{
			auto& name = i.first;
			appender.Append(nsDepth > 0 ? ("\t\t") : ("\t"));
			AppendStructShortName(appender, ec.asCppEnum.SexyName());
			appender.Append(("_%s = %I64d, \t// 0x%I64x\n"), name.c_str(), i.second, i.second);
		}

		appender.Append(nsDepth > 0 ? ("\t};") : ("};"));

		appender.Append('\n');

		appender.Append(("\tbool TryParse(const Rococo::fstring& s, "));
		AppendStructShortName(appender, ec.asCppEnum.SexyName());
		appender.Append(("& value);\n"));

		appender.Append(("\tbool TryShortParse(const Rococo::fstring& s, "));
		AppendStructShortName(appender, ec.asCppEnum.SexyName());
		appender.Append(("& value);\n"));

		appender.Append(("\tfstring ToShortString("));
		AppendStructShortName(appender, ec.asCppEnum.SexyName());
		appender.Append((" value); "));

		if (nsDepth > 0)
		{
			appender.Append('\n');
			CloseNamespace(appender, nsDepth);
		}

		appender.Append(("\n\n"));
	}

	void DeclareCppInterface(FileAppender& appender, const InterfaceContext& ic, cr_sex interfaceDef, const ISExpression* methods, const ParseContext& pc)
	{
		int nsDepth = AppendNamespace(appender, ic.asCppInterface.SexyName());
		if (nsDepth > 0)
		{
			appender.Append(("\t"));
		}

		appender.Append(("ROCOCOAPI "));
		AppendStructShortName(appender, ic.asCppInterface.SexyName());
		if (ic.cppBase)
		{
			NamespaceSplitter splitter(ic.cppBase);
			cstr head, tail;
			if (splitter.SplitHead(head, tail))
			{
				char cppCompressedNSName[256];
				char cppNSName[256];
				GetFQCppStructName(cppCompressedNSName, cppNSName, 256, ic.cppBase);
				appender.Append(": %s", cppNSName);
			}
			else
			{
				appender.Append(": %s", ic.cppBase);
			}
		}
		appender.Append(nsDepth > 0 ? ("\n\t{\n") : ("\n{\n"));

		NamespaceSplitter splitter(ic.asCppInterface.SexyName());
		cstr root, subspace;
		if (!splitter.SplitHead(root, subspace)) root = ("");

		if (methods != NULL)
		{
			for (int i = 1; i < methods->NumberOfElements(); ++i)
			{
				cr_sex method = methods->GetElement(i);
				appender.Append('\t');
				AppendMethodDeclaration(appender, method, root, pc);
				appender.Append('\n');
			}
		}

		appender.Append(nsDepth > 0 ? ("\t};") : ("};"));

		if (nsDepth > 0)
		{
			appender.Append('\n');
			CloseNamespace(appender, nsDepth);
		}

		NamespaceSplitter nsSplitter(ic.asCppInterface.SexyName());
		cstr ns, shortName;
		if (!splitter.SplitTail(ns, shortName))
		{
			sexstringstream<256> s;
			s.sb << ("Cpp interface ") << ic.asCppInterface.SexyName() << ("needs a namespace prefix.");
			Throw(interfaceDef, "%s", (cstr)s);
		}

		char cppCompressedNSName[256];
		char cppNSName[256];
		GetFQCppStructName(cppCompressedNSName, cppNSName, 256, ns);

		appender.Append(("\n\n"));

		if (ic.nceContext.SexyName()[0] != 0)
		{
			int depth = AppendNamespace(appender, ic.asCppInterface.SexyName());
			appender.Append(("\tvoid AddNativeCalls_%s(Rococo::Script::IPublicScriptSystem& ss, %s* nceContext);\n"), ic.asCppInterface.CompressedName(), ic.nceContext.FQName());
			while (depth > 0)
			{
				depth--;
				appender.Append(("}"));
			}
		}

		appender.Append(("\n\n"));
	}

	typedef std::unordered_map<stdstring, int> TAttributeMap;
	bool HasAttributeThrows(const TAttributeMap& map)
	{
		return map.find(("throws")) != map.end();
	}

	void AddNativeInputs(TAttributeMap& attributes, FileAppender& appender, cr_sex methodArgs, int inputStart, int inputEnd, const ParseContext& pc)
	{
		for (int i = inputEnd; i >= inputStart; --i)
		{
			cr_sex s = methodArgs.GetElement(i);

			if (s.NumberOfElements() != 2 && s.NumberOfElements() != 3)
			{
				Throw(s, ("Expected input argument pair (<type> <value>) or (const <type> value)"));
			}

			int typeIndex = 0;
			int valueIndex = 1;

			if (s.NumberOfElements() == 3)
			{
				typeIndex++;
				valueIndex++;

				if (!IsAtomic(s[0]) || !AreEqual(s[0].String(), ("const")))
				{
					Throw(s[0], ("Expecting 'const' as first argument in 3 element input expression"));
				}
			}

			cr_sex stype = s.GetElement(typeIndex);
			cr_sex svalue = s.GetElement(valueIndex);

			cstr sxhtype = StringFrom(stype);
			cstr fieldName = StringFrom(svalue);

			if (AreEqual(sxhtype, ("#")))
			{
				attributes.insert(std::make_pair(fieldName, 1));
			}
			else
			{
				bool isString = false;
				if (AreEqual(sxhtype, ("IString")) || AreEqual(sxhtype, ("Sys.Text.IString")))
				{
					isString = true;
				}

				bool isStringBuilder = false;
				if (AreEqual(sxhtype, ("IStringBuilder")) || AreEqual(sxhtype, ("Sys.Text.IStringBuilder")))
				{
					isStringBuilder = true;
				}

				if (!isString && !isStringBuilder)
				{
					appender.Append(("\t\t"));
					AppendCppType(appender, stype, sxhtype, pc);
				}

				if (!isString && pc.structs.find(sxhtype) != pc.structs.end() && !isStringBuilder)
				{
					appender.Append('*');
				}

				if (!isString && !isStringBuilder) appender.Append((" %s;\n"), fieldName);

				if (isString)
				{
					appender.Append(("\t\t_offset += sizeof(IString*);\n"));
				}
				else if (isStringBuilder)
				{
					appender.Append(("\t\t_offset += sizeof(VirtualTable**);\n"));
				}
				else
				{
					appender.Append(("\t\t_offset += sizeof(%s);\n"), fieldName);
				}

				if (isString) appender.Append(("\t\tIString* _%s;\n"), fieldName);
				if (isStringBuilder) appender.Append(("\t\tVirtualTable** %s;\n"), fieldName);

				appender.Append(isString ? ("\t\tReadInput(_%s, _sf, -_offset);\n") : ("\t\tReadInput(%s, _sf, -_offset);\n"), fieldName);

				if (isString)
				{
					appender.Append(("\t\t"));
					AppendCppType(appender, stype, sxhtype, pc);
					appender.Append((" %s {"), fieldName);
					appender.Append((" _%s->buffer, _%s->length };\n\n"), fieldName, fieldName);
				}
				else if (isStringBuilder)
				{
					appender.Append(("\t\tRococo::Helpers::StringPopulator _%sPopulator(_nce, %s);"), fieldName, fieldName);
				}

				appender.Append(("\n"));
			}
		}
	}

	void AddNativeImplementation(FileAppender& appender, const InterfaceContext& ic, cr_sex method, const ParseContext& pc)
	{
		cstr methodName = StringFrom(method.GetElement(0));

		appender.Append(("\tvoid Native%s%s"), ic.asCppInterface.CompressedName(), methodName);

		appender.Append(("(NativeCallEnvironment& _nce)\n"));
		appender.Append(("\t{\n"));

		appender.Append(("\t\tRococo::uint8* _sf = _nce.cpu.SF();\n"));
		appender.Append(("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n"));

		bool hasInitializedStringStruct = false;

		TAttributeMap attributes;

		// Write inputs
		int outputStart = GetOutputPosition(method);
		AddNativeInputs(attributes, appender, method, 1, outputStart - 2, pc);

		if (ic.isSingleton)
		{
			appender.Append(("\t\t%s* _pObject = reinterpret_cast<%s*>(_nce.context);\n"), ic.asCppInterface.FQName(), ic.asCppInterface.FQName());
		}
		else
		{
			// No NCE singleton context, so take the instance value that was initialized in the constructor 	
			appender.Append(("\t\t%s* _pObject;\n"), ic.asCppInterface.FQName());
			appender.Append(("\t\t_offset += sizeof(_pObject);\n\n"));
			appender.Append(("\t\tReadInput(_pObject, _sf, -_offset);\n"));
		}

		bool outputIsInterface = false;
		std::vector<stdstring> outputPrefix;
		for (int i = outputStart; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);

			cr_sex stype = s.GetElement(0);
			cr_sex svalue = s.GetElement(1);

			cstr type = StringFrom(stype);

			char cppName[256];
			char compressedName[256];

			TTypeMap::const_iterator k = pc.primitives.find(type);
			if (k == pc.primitives.end())
			{
				auto z = pc.interfaces.find(type);
				if (z == pc.interfaces.end())
				{
					Throw(stype, ("Could not find type amongst the primitives or interfaces"));
				}
				else
				{
					SafeFormat(cppName, sizeof(cppName), ("%s* %s = "), z->second->ic.asCppInterface.FQName(), StringFrom(svalue));
					outputPrefix.push_back(cppName);
					outputIsInterface = true;
				}
			}
			else
			{
				GetFQCppStructName(compressedName, cppName, 256, k->second.cppType.c_str());

				char moreHacks[256];
				SafeFormat(moreHacks, sizeof(moreHacks), ("%s %s = "), cppName, StringFrom(svalue));
				outputPrefix.push_back(moreHacks);
			}
		}

		if (HasAttributeThrows(attributes))
		{
			appender.Append(("try\n\t\t{\n\t\t\t"));
		}

		int index = 0;
		if (outputStart < method.NumberOfElements())
		{
			cr_sex s = method.GetElement(outputStart);

			int typeIndex = 0;

			if (s.NumberOfElements() == 3)
			{
				typeIndex++;
			}

			cr_sex stype = s.GetElement(typeIndex);
			cr_sex svalue = s.GetElement(typeIndex + 1);

			appender.Append(("\t\t%s"), outputPrefix[0].c_str());
		}
		else
		{
			appender.Append(("\t\t"));
		}

		appender.Append(("_pObject->%s("), methodName);

		int inputCount = 1;

		// Append the input arguments to the method invocation
		for (int i = 1; i < outputStart - 1; ++i)
		{
			cr_sex s = method.GetElement(i);

			int typeIndex = 0;

			if (s.NumberOfElements() == 3)
			{
				typeIndex++;
			}

			cr_sex stype = s.GetElement(typeIndex);
			cr_sex svalue = s.GetElement(typeIndex + 1);
			cstr type = StringFrom(stype);

			if (!AreEqual(type, ("#")))
			{
				if (inputCount > 1) appender.Append((", "));
				inputCount++;

				if (AreEqual(type, ("IStringBuilder")) || AreEqual(type, ("Sys.Type.IStringBuilder")))
				{
					appender.Append(("_%sPopulator"), StringFrom(svalue));
				}
				else
				{
					if (!AreEqual(type, ("IString")) && !AreEqual(type, ("Sys.Type.IString")) && pc.structs.find(type) != pc.structs.end())
					{
						appender.Append('*');
					}

					appender.Append(("%s"), StringFrom(svalue));
				}
			}
			else
			{
				// Skip input arguments that are attributes rather than type-name pairs.
			}
		}

		// Write outputs
		for (int i = outputStart + 1; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);

			cr_sex stype = s.GetElement(0);
			cr_sex svalue = s.GetElement(1);

			if (i > 1) appender.Append((", "));
			appender.Append(("/* out */ %s"), StringFrom(svalue));
		}

		appender.Append((");\n"));

		if (HasAttributeThrows(attributes))
		{
			appender.Append(("\t\t}\n\t\tcatch(%s& _ex) { _HandleScriptException(_nce, _ex); }\n"), pc.cppException);
		}

		for (int i = outputStart; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);

			cr_sex stype = s.GetElement(0);
			cr_sex svalue = s.GetElement(1);

			appender.Append(("\t\t_offset += sizeof(%s);\n"), outputIsInterface ? ("CReflectedClass*") : StringFrom(svalue));

			if (outputIsInterface)
			{
				auto z = pc.interfaces.find(stype.String()->Buffer);

				NamespaceSplitter splitter(z->second->ic.asSexyInterface);

				cstr body, tail;
				splitter.SplitTail(body, tail);

				appender.Append(("\t\tauto& _%sStruct = Rococo::Helpers::GetDefaultProxy((\"%s\"),(\"%s\"), (\"Proxy%s\"), _nce.ss);\n"), StringFrom(svalue), body, tail, tail);
				appender.Append(("\t\tCReflectedClass* _sxy%s = _nce.ss.Represent(_%sStruct, %s);\n"), StringFrom(svalue), StringFrom(svalue), StringFrom(svalue));
				appender.Append(("\t\tWriteOutput(&_sxy%s->header.pVTables[0], _sf, -_offset);\n"), StringFrom(svalue));
			}
			else
			{
				appender.Append(("\t\tWriteOutput(%s, _sf, -_offset);\n"), StringFrom(svalue));
			}
		}

		appender.Append(("\t}\n"));
	}

	void AppendFactoryArguments(FileAppender& appender, cr_sex method, int inputStart, int inputEnd, const ParseContext& pc, bool includeTypes)
	{
		cstr argPrefix = ("_");

		// Append the input arguments to the method invocation
		for (int i = inputStart; i <= inputEnd; ++i)
		{
			cr_sex arg = method.GetElement(i);

			cr_sex stype = arg.GetElement(0);
			cr_sex svalue = arg.GetElement(1);
			cstr type = StringFrom(stype);

			if (!AreEqual(type, ("#")))
			{
				appender.Append((", "));

				if (AreEqual(type, ("IString")) || AreEqual(type, ("Sys.Type.IString")))
				{
					if (includeTypes)
					{
						TTypeMap::const_iterator j = pc.structs.find(("IString"));
						if (j == pc.primitives.end())
						{
							Throw(stype, ("Missing IString definition in the config. Suggest adding an entry of the form: (struct IString Sys.Type.IString fstring)"));
						}

						char compressedStructName[256];
						char cppStructName[256];
						GetFQCppStructName(compressedStructName, cppStructName, 256, j->second.cppType.c_str());

						appender.Append(("const %s& %s%s"), cppStructName, argPrefix, StringFrom(svalue));
					}
					else
					{
						appender.Append(("%s"), StringFrom(svalue));
					}
				}
				else
				{
					TTypeMap::const_iterator j = pc.structs.find(type);
					if (j != pc.structs.end())
					{
						if (includeTypes)
						{
							cstr cppType = j->second.cppType.c_str();
							appender.Append(("const %s& %s%s"), cppType, argPrefix, StringFrom(svalue));
						}
						else
						{
							appender.Append(("*%s"), StringFrom(svalue));
						}
					}
					else
					{
						if (includeTypes)
						{
							j = pc.primitives.find(type);
							if (j == pc.primitives.end()) Throw(stype, ("Error evaluating type-name to a primitive or a derivative type"));

							char compressedStructName[256];
							char cppStructName[256];
							GetFQCppStructName(compressedStructName, cppStructName, 256, j->second.cppType.c_str());
							appender.Append(("%s %s"), cppStructName, argPrefix);
						}

						appender.Append(("%s"), StringFrom(svalue));
					}
				}
			}
			else
			{
				// Skip input arguments that are attributes rather than type-name pairs.
			}
		}
	}

	void ImplementNativeFunctions(FileAppender& appender, const EnumContext& ec, const ParseContext& pc)
	{
		int nsDepth = AppendNamespace(appender, ec.asCppEnum.SexyName());
		if (nsDepth > 0)
		{
			appender.Append(("\t"));
		}

		NamespaceSplitter splitter(ec.asCppEnum.SexyName());
		cstr ns, tail;
		splitter.SplitTail(ns, tail);

#ifdef char_IS_WIDE
		char stringIndicator = L'L';
#else
		char stringIndicator = ' ';
#endif

		appender.Append(("bool TryParse(const Rococo::fstring& s, %s& value)\n"), tail);
		appender.Append(("\t{\n"));

		bool first = true;
		for (auto& i : ec.values)
		{
			appender.Append(("\t\t%s (s == %c\"%s_%s\"_fstring)\n"), first ? ("if") : ("else if"), stringIndicator, tail, i.first.c_str());
			appender.Append(("\t\t{\n"));
			appender.Append(("\t\t\tvalue = %s_%s;\n"), tail, i.first.c_str());
			appender.Append(("\t\t}\n"));

			first = false;
		}

		appender.Append(("\t\telse\n"));
		appender.Append(("\t\t{\n"));
		appender.Append(("\t\t\treturn false;\n"));
		appender.Append(("\t\t}\n\n"));

		appender.Append(("\t\treturn true;\n"));
		appender.Append(("\t}\n\n"));

		appender.Append(("\tbool TryShortParse(const Rococo::fstring& s, %s& value)\n"), tail);
		appender.Append(("\t{\n"));

		first = true;
		for (auto& i : ec.values)
		{
			appender.Append(("\t\t%s (s == %c\"%s\"_fstring)\n"), first ? ("if") : ("else if"), stringIndicator, i.first.c_str());
			appender.Append(("\t\t{\n"));
			appender.Append(("\t\t\tvalue = %s_%s;\n"), tail, i.first.c_str());
			appender.Append(("\t\t}\n"));

			first = false;
		}

		appender.Append(("\t\telse\n"));
		appender.Append(("\t\t{\n"));
		appender.Append(("\t\t\treturn false;\n"));
		appender.Append(("\t\t}\n\n"));

		appender.Append(("\t\treturn true;\n"));
		appender.Append(("\t}\n"));

		appender.Append(("\tfstring ToShortString(%s value)\n"), tail);
		appender.Append(("\t{\n"));

		appender.Append(("\t\tswitch(value)\n"));
		appender.Append(("\t\t{\n"));

		for (auto& i : ec.values)
		{
			appender.Append(("\t\t\tcase %s_%s:\n"), tail, i.first.c_str());
			appender.Append(("\t\t\t\treturn \"%s\"_fstring;\n"), i.first.c_str());
		}

		appender.Append(("\t\t\tdefault:\n"));
		appender.Append(("\t\t\t\treturn {\"\",0};\n"));

		appender.Append(("\t\t}\n"));
		appender.Append(("\t}\n"));

		while (nsDepth > 0)
		{
			nsDepth--;
			appender.Append(("}"));
		}

		appender.Append(("// %s\n\n"), ec.asCppEnum.SexyName());
	}

	void ImplementNativeFunctions(FileAppender& appender, const InterfaceContext& ic, const ISExpression* methods[], const ParseContext& pc)
	{
		appender.Append(("// BennyHill generated Sexy native functions for %s \n"), ic.asCppInterface.FQName());
		appender.Append(("namespace\n{\n\tusing namespace Rococo;\n\tusing namespace Rococo::Sex;\n\tusing namespace Rococo::Script;\n\tusing namespace Rococo::Compiler;\n\n"));

		for(size_t t = 0; methods[t] != nullptr; ++t)
		{
			for (int i = 1; i < methods[t]->NumberOfElements(); ++i)
			{
				cr_sex method = methods[t]->GetElement(i);
				AddNativeImplementation(appender, ic, method, pc);
			}
		}

		appender.Append('\n');

		if (!ic.isSingleton)
		{
			for (TExpressions::const_iterator i = ic.factories.begin(); i != ic.factories.end(); ++i)
			{
				cr_sex sfactoryDef = **i;
				if (sfactoryDef.NumberOfElements() < 2)
				{
					Throw(sfactoryDef, ("expecting (factory <fully-qualified-sexy-function-name> <(arg-pair1)...(arg-pairN)>)"));
				}

				cstr factoryName = StringFrom(sfactoryDef, 1);
				ValidateFQSexyFunction(sfactoryDef, factoryName);

				CppType factoryType;
				factoryType.Set(factoryName);

				appender.Append(("\tvoid NativeGetHandleFor%s(NativeCallEnvironment& _nce)\n"), factoryType.CompressedName());
				appender.Append(("\t{\n"));
				appender.Append(("\t\tRococo::uint8* _sf = _nce.cpu.SF();\n"));
				appender.Append(("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n"));

				TAttributeMap attributes;
				AddNativeInputs(attributes, appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc);
				appender.Append(("\t\t%s* nceContext = reinterpret_cast<%s*>(_nce.context);\n"), ic.nceContext.FQName(), ic.nceContext.FQName());
				appender.Append(("\t\t// Uses: %s* FactoryConstruct%s(%s* _context"), ic.asCppInterface.FQName(), factoryType.CompressedName(), ic.nceContext.FQName());
				AppendFactoryArguments(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc, true);
				appender.Append((");\n"));

				appender.Append(("\t\t%s* pObject = FactoryConstruct%s(nceContext"), ic.asCppInterface.FQName(), factoryType.CompressedName());
				AppendFactoryArguments(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc, false);
				appender.Append((");\n"));

				appender.Append(("\t\t_offset += sizeof(IString*);\n"));
				appender.Append(("\t\tWriteOutput(pObject, _sf, -_offset);\n"));
				appender.Append(("\t}\n"));
			}
		}

		appender.Append(("}\n\n"));

		NamespaceSplitter nsSplitter(ic.asCppInterface.SexyName());
		cstr ns, shortName;
		nsSplitter.SplitTail(ns, shortName);

		CppType nsType;
		nsType.Set(ns);

		int depth = AppendNamespace(appender, ic.asCppInterface.SexyName());

		appender.Append(("\tvoid AddNativeCalls_%s(Rococo::Script::IPublicScriptSystem& ss, %s* _nceContext)\n"), ic.asCppInterface.CompressedName(), ic.nceContext.FQName());
		appender.Append(("\t{\n"));

		if (methods != NULL)
		{
			appender.Append(("\t\tconst INamespace& ns = ss.AddNativeNamespace((\""));

			NamespaceSplitter splitter(ic.asSexyInterface);

			cstr ns, shortName;
			splitter.SplitTail(ns, shortName);

			appender.Append(("%s.Native\"));\n"), ns);

			int factoryIndex = 0;

			if (!ic.isSingleton)
			{
				for (TExpressions::const_iterator i = ic.factories.begin(); i != ic.factories.end(); ++i, ++factoryIndex)
				{
					cr_sex sfactoryDef = **i;
					cstr factoryName = StringFrom(sfactoryDef, 1);
					ValidateFQSexyFunction(sfactoryDef, factoryName);

					CppType factoryType;
					factoryType.Set(factoryName);

					appender.Append(("\t\tss.AddNativeCall(ns, NativeGetHandleFor%s, _nceContext, (\"GetHandleFor%s%d "), factoryType.CompressedName(), shortName, factoryIndex);

					for (int i = 2; i < sfactoryDef.NumberOfElements(); i++)
					{
						cr_sex arg = sfactoryDef.GetElement(i);
						AppendInputPair(appender, arg, pc);
					}

					appender.Append((" -> (Pointer hObject)\"));\n"));
				}
			}

			for (size_t k = 0; methods[k] != nullptr; ++k)
			{
				for (int t = 1; t < methods[k]->NumberOfElements(); ++t)
				{
					cr_sex method = methods[k]->GetElement(t);
					cstr methodName = StringFrom(method.GetElement(0));
					appender.Append(("\t\tss.AddNativeCall(ns, Native%s%s, %s, (\""), ic.asCppInterface.CompressedName(), methodName, ic.isSingleton ? ("_nceContext") : ("nullptr"));
					appender.Append(("%s%s "), shortName, methodName);
					if (!ic.isSingleton)
					{
						appender.Append(("(Pointer hObject)"));
					}

					int outputPos = GetOutputPosition(method);
					for (int i = 1; i < outputPos - 1; i++)
					{
						cr_sex arg = method.GetElement(i);
						AppendInputPair(appender, arg, pc);
					}

					appender.Append((" -> "));
					for (int i = outputPos; i < method.NumberOfElements(); i++)
					{
						cr_sex arg = method.GetElement(i);
						AppendOutputPair(appender, arg, pc);
					}

					appender.Append(("\"));\n"));
				}
			}
		}
		appender.Append(("\t}\n"));

		while (depth > 0)
		{
			depth--;
			appender.Append(("}"));
		}

		appender.Append(("\n"));
	}
}