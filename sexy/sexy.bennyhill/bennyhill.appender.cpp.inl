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

   void DeclareCppEnum(FileAppender& appender, const EnumContext& ec, cr_sex senumDef, const ParseContext& pc);

	int AppendNamespace(FileAppender& appender, csexstr fqStructName)
	{
		NamespaceSplitter splitter(fqStructName);
		csexstr nsRoot, nsSubspace;
		if (splitter.SplitHead(nsRoot, nsSubspace)) 
		{
			appender.Append(SEXTEXT("namespace %s { "), nsRoot);
			return 1 + AppendNamespace(appender, nsSubspace);
		}

		appender.Append('\n');
		return 0;
	}

	void CloseNamespace(FileAppender& appender, int nsDepth)
	{
		for(int i = 0; i < nsDepth; ++i)
		{
			appender.Append('}');
		}
	}

	void AppendStructShortName(FileAppender& appender, csexstr fqStructName)
	{
		NamespaceSplitter splitter(fqStructName);
		csexstr ns, shortName;
		appender.Append(SEXTEXT("%s"), splitter.SplitTail(ns, shortName) ? shortName : fqStructName);
	}

	void AppendMethodDeclaration(FileAppender& appender, cr_sex method, csexstr root, const ParseContext& pc)
	{
		cr_sex smethodName = method.GetElement(0);
		csexstr methodName = smethodName.String()->Buffer;

		int outputIndex = GetOutputPosition(method);
		if (outputIndex >= method.NumberOfElements())
		{
			appender.Append(SEXTEXT("\tvirtual void %s("), methodName);
		}
		else
		{
			cr_sex s = method.GetElement(outputIndex);

			int typeIndex = 0;
			int valueIndex = 1;

			cr_sex stype = s.GetElement(typeIndex);
			cr_sex sname = s.GetElement(valueIndex);

			csexstr outputType = StringFrom(stype);
			csexstr outputName = StringFrom(sname);

			appender.Append(SEXTEXT("\tvirtual "));

			auto i = pc.primitives.find(outputType);
			if (i != pc.primitives.end())
			{
				AppendCppType(appender, s, outputType, pc);
				appender.Append(SEXTEXT("/* %s */ %s("), outputName, methodName);
			}
			else
			{
            auto j = pc.interfaces.find(outputType);
            if (j == pc.interfaces.end())
            {
               Throw(stype, SEXTEXT("Expecting primitive return type or interface"));
            }

            AppendCppType(appender, s, outputType, pc);
            appender.Append(SEXTEXT("* /* %s */ %s("), outputName, methodName);
			}
		}

		// Write inputs

		int inputCount = 1;

		int i;
		for(i = 1; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			if (IsAtomic(s))
			{
				csexstr arg = s.String()->Buffer;
				if (AreEqual(arg, SEXTEXT("->")))
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

				if (!IsAtomic(s[0]) || !AreEqual(s[0].String(), SEXTEXT("const")))
				{
					Throw(s[0], SEXTEXT("Expecting 'const' as first argument in 3 element input expression"));
				}
			}
			
			cr_sex stype = s.GetElement(typeIndex);
			csexstr type = StringFrom(s, typeIndex);
			if (!AreEqual(type, SEXTEXT("#")))
			{
				cr_sex sname = s.GetElement(valueIndex);

				ValidateSexyType(stype);
				ValidateSexyVariable(sname);

				csexstr inputtype = stype.String()->Buffer;
				csexstr name = sname.String()->Buffer;

				if (inputCount > 1) appender.Append(SEXTEXT(", "));
				inputCount++;

				if (s.NumberOfElements() == 3 || AreEqual(inputtype, SEXTEXT("IString")))
            {
					appender.Append(SEXTEXT("const "));
				}

				AppendCppType(appender, s, inputtype, pc);

				if (pc.structs.find(inputtype) != pc.structs.end())
				{
					appender.Append('&');
				}

				appender.Append(SEXTEXT(" %s"), name);
			}
		}

		i++; // The first output is what is returned

		int firstOutput = i;

		for (; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			csexstr outputType = StringFrom(s, 0);
			csexstr name = StringFrom(s, 1);

			if (i > firstOutput) 
			{
				if (i > 1) appender.Append(SEXTEXT(", "));
			}
			else appender.Append(SEXTEXT(", /* -> */ "));

			AppendCppType(appender, s, outputType, pc);

			appender.Append(SEXTEXT("& %s"), name);
		}

		appender.Append(SEXTEXT(") = 0;"));
	}

   void WriteInterfaceDeclaration(FileAppender& writer, csexstr qualifiedName, int depth)
   {
      NamespaceSplitter splitter(qualifiedName);
      csexstr head, body;
      if (splitter.SplitHead(head, body))
      {
         writer.Append(SEXTEXT("namespace %s { "), head);
         WriteInterfaceDeclaration(writer, body, depth + 1);
         writer.Append(SEXTEXT("}"));
      }
      else
      {
         writer.Append(SEXTEXT("\n\tstruct %s;\n"), qualifiedName);
      }
   }

   void GenerateDeclarations(const ParseContext& pc)
   {
      std::unordered_set<stdstring> enumFiles;
      for (auto& i : pc.enums)
      {
         enumFiles.insert(i.ec.appendSexyFile);
      }

      for (auto& i : enumFiles)
      {
         FileAppender sexyFileAppender(i.c_str());
         for (auto& i : pc.enums)
         {
            DeclareSexyEnum(sexyFileAppender, i.ec, *i.sdef, pc);
         }
      }

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
         auto& ic = i.second->ic;
         enumFiles.insert(ic.appendCppHeaderFile);
      }

      for (auto& i : enumFiles)
      {
         FileAppender cppFileAppender(i.c_str());
         for (auto& i : pc.interfaces)
         {
            WriteInterfaceDeclaration(cppFileAppender, i.second->ic.asCppInterface.SexyName(), 0);
            cppFileAppender.Append(SEXTEXT("\n\n"));
         }
      }
   }

	void DeclareCppEnum(FileAppender& appender, const EnumContext& ec, cr_sex senumDef, const ParseContext& pc)
	{
		int nsDepth = AppendNamespace(appender, ec.asCppEnum.SexyName());
		if (nsDepth > 0)
		{
			appender.Append(SEXTEXT("\t"));
		}

		appender.Append(SEXTEXT("enum "));
		AppendStructShortName(appender, ec.asCppEnum.SexyName());
		appender.Append(SEXTEXT(": "));
		AppendStructShortName(appender, ec.underlyingType.SexyName());
		appender.Append(nsDepth > 0 ? SEXTEXT("\n\t{\n") : SEXTEXT("\n{\n"));

		for (auto& i : ec.values)
		{
			auto& name = i.first;
			appender.Append(nsDepth > 0 ? SEXTEXT("\t\t") : SEXTEXT("\t"));
			AppendStructShortName(appender, ec.asCppEnum.SexyName());
			appender.Append(SEXTEXT("_%s = %I64d, \t// 0x%I64x\n"), name.c_str(), i.second, i.second);
		}

		appender.Append(nsDepth > 0 ? SEXTEXT("\t};") : SEXTEXT("};"));

		appender.Append('\n');

		appender.Append(SEXTEXT("\tbool TryParse(const Rococo::fstring& s, "));
		AppendStructShortName(appender, ec.asCppEnum.SexyName()); 
		appender.Append(SEXTEXT("& value);\n"));

		appender.Append(SEXTEXT("\tbool TryShortParse(const Rococo::fstring& s, "));
		AppendStructShortName(appender, ec.asCppEnum.SexyName());
		appender.Append(SEXTEXT("& value); "));

		if (nsDepth > 0)
		{
			appender.Append('\n');
			CloseNamespace(appender, nsDepth);
		}

		appender.Append(SEXTEXT("\n\n"));
	}

	void DeclareCppInterface(FileAppender& appender, const InterfaceContext& ic, cr_sex interfaceDef, const ISExpression* methods, const ParseContext& pc)
	{
		int nsDepth = AppendNamespace(appender, ic.asCppInterface.SexyName());
		if (nsDepth > 0)
		{
			appender.Append(SEXTEXT("\t"));
		}

		appender.Append(SEXTEXT("ROCOCOAPI "));
		AppendStructShortName(appender, ic.asCppInterface.SexyName());
      appender.Append(ic.inheritanceString);
		appender.Append(nsDepth > 0 ? SEXTEXT("\n\t{\n") : SEXTEXT("\n{\n"));

		NamespaceSplitter splitter(ic.asCppInterface.SexyName());
		csexstr root, subspace;
		if (!splitter.SplitHead(root, subspace)) root = SEXTEXT("");

		if (methods != NULL)
		{
			for(int i = 1; i < methods->NumberOfElements(); ++i)
			{
				cr_sex method = methods->GetElement(i);
				appender.Append('\t');
				AppendMethodDeclaration(appender, method, root, pc);
				appender.Append('\n');
			}
		}

		appender.Append(nsDepth > 0 ? SEXTEXT("\t};") : SEXTEXT("};"));

		if (nsDepth > 0)
		{
			appender.Append('\n');
			CloseNamespace(appender, nsDepth);
		}

		NamespaceSplitter nsSplitter(ic.asCppInterface.SexyName());
		csexstr ns, shortName;
		if (!splitter.SplitTail(ns, shortName))
		{
			sexstringstream<256> s;
			s.sb << SEXTEXT("Cpp interface ") << ic.asCppInterface.SexyName() << SEXTEXT("needs a namespace prefix.");
			Throw(interfaceDef, "%s", (cstr) s);
		}

		SEXCHAR cppCompressedNSName[256];
		SEXCHAR cppNSName[256];
		GetFQCppStructName(cppCompressedNSName, cppNSName, 256, ns);

      appender.Append(SEXTEXT("\n\n"));
      int depth = AppendNamespace(appender, ic.asCppInterface.SexyName());
      appender.Append(SEXTEXT("\tvoid AddNativeCalls_%s(Sexy::Script::IPublicScriptSystem& ss, %s* nceContext);\n"), ic.asCppInterface.CompressedName(), ic.nceContext.FQName());
      while (depth > 0)
      {
         depth--;
         appender.Append(SEXTEXT("}"));   
      }
      
      appender.Append(SEXTEXT("\n\n"));
	}

	typedef std::unordered_map<stdstring,int> TAttributeMap;
	bool HasAttributeThrows(const TAttributeMap& map)
	{
		return map.find(SEXTEXT("throws")) != map.end();
	}

	void AddNativeInputs(TAttributeMap& attributes, FileAppender& appender, cr_sex methodArgs, int inputStart, int inputEnd, const ParseContext& pc)
	{
		for(int i = inputEnd; i >= inputStart; --i)
		{
			cr_sex s = methodArgs.GetElement(i);		

			if (s.NumberOfElements() != 2 && s.NumberOfElements() != 3)
			{
				Throw(s, SEXTEXT("Expected input argument pair (<type> <value>) or (const <type> value)"));
			}

			int typeIndex = 0;
			int valueIndex = 1;

			if (s.NumberOfElements() == 3)
			{
				typeIndex++;
				valueIndex++;

				if (!IsAtomic(s[0]) || !AreEqual(s[0].String(), SEXTEXT("const")))
				{
					Throw(s[0], SEXTEXT("Expecting 'const' as first argument in 3 element input expression"));
				}
			}

			cr_sex stype = s.GetElement(typeIndex);
			cr_sex svalue = s.GetElement(valueIndex);

			csexstr sxhtype = StringFrom(stype);
			csexstr fieldName = StringFrom(svalue);

			if (AreEqual(sxhtype, SEXTEXT("#")))
			{
				attributes.insert(std::make_pair(fieldName, 1));
			}
			else
			{
				bool isString = false;
				if (AreEqual(sxhtype, SEXTEXT("IString")) || AreEqual(sxhtype, SEXTEXT("Sys.Text.IString")))
				{
					isString = true;
				}

            bool isStringBuilder = false;
            if (AreEqual(sxhtype, SEXTEXT("IStringBuilder")) || AreEqual(sxhtype, SEXTEXT("Sys.Text.IStringBuilder")))
            {
               isStringBuilder = true;
            }
			
				if (!isString && !isStringBuilder)
				{
					appender.Append(SEXTEXT("\t\t"));
               AppendCppType(appender, stype, sxhtype, pc);
				}

				if (!isString && pc.structs.find(sxhtype) != pc.structs.end() && !isStringBuilder)
				{
					appender.Append('*');
				}

				if (!isString && !isStringBuilder) appender.Append(SEXTEXT(" %s;\n"), fieldName);			
			
				if (isString)
				{
					appender.Append(SEXTEXT("\t\t_offset += sizeof(IString*);\n"));
				}
            else if (isStringBuilder)
            {
               appender.Append(SEXTEXT("\t\t_offset += sizeof(VirtualTable*);\n"));
            }
				else
				{
					appender.Append(SEXTEXT("\t\t_offset += sizeof(%s);\n"), fieldName);
				}

				if (isString) appender.Append(SEXTEXT("\t\tIString* _%s;\n"), fieldName);
            if (isStringBuilder) appender.Append(SEXTEXT("\t\tVirtualTable* %s;\n"), fieldName);

				appender.Append(isString ? SEXTEXT("\t\tReadInput(_%s, _sf, -_offset);\n") : SEXTEXT("\t\tReadInput(%s, _sf, -_offset);\n"), fieldName);

				if (isString)
				{
					appender.Append(SEXTEXT("\t\t"));
					AppendCppType(appender, stype, sxhtype, pc);
					appender.Append(SEXTEXT(" %s {"), fieldName);	
					appender.Append(SEXTEXT(" _%s->buffer, _%s->length };\n\n"), fieldName, fieldName);
				}
            else if (isStringBuilder)
            {
               appender.Append(SEXTEXT("\t\tSexy::Helpers::StringPopulator _%sPopulator(_nce, %s);"), fieldName, fieldName);
            }

				appender.Append(SEXTEXT("\n"));
			}
		}
	}
	
	void AddNativeImplementation(FileAppender& appender, const InterfaceContext& ic, cr_sex method, const ParseContext& pc)
	{
		csexstr methodName = StringFrom(method.GetElement(0));

		appender.Append(SEXTEXT("\tvoid Native%s%s"), ic.asCppInterface.CompressedName(), methodName);
	
		appender.Append(SEXTEXT("(NativeCallEnvironment& _nce)\n"));
		appender.Append(SEXTEXT("\t{\n"));

		appender.Append(SEXTEXT("\t\tSexy::uint8* _sf = _nce.cpu.SF();\n"));
		appender.Append(SEXTEXT("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n"));
	
		bool hasInitializedStringStruct = false;

		TAttributeMap attributes;

		// Write inputs
		int outputStart = GetOutputPosition(method);
		AddNativeInputs(attributes, appender, method, 1, outputStart-2, pc);

		if (ic.isSingleton)
		{
			appender.Append(SEXTEXT("\t\t%s* _pObject = reinterpret_cast<%s*>(_nce.context);\n"), ic.asCppInterface.FQName(), ic.asCppInterface.FQName());
		}
		else
		{
			// No NCE singleton context, so take the instance value that was initialized in the constructor 	
			appender.Append(SEXTEXT("\t\t%s* _pObject;\n"), ic.asCppInterface.FQName());
			appender.Append(SEXTEXT("\t\t_offset += sizeof(_pObject);\n\n"));
			appender.Append(SEXTEXT("\t\tReadInput(_pObject, _sf, -_offset);\n"));
		}
	
      bool outputIsInterface = false;
      std::vector<stdstring> outputPrefix;
		for (int i = outputStart; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			
			cr_sex stype = s.GetElement(0);
			cr_sex svalue = s.GetElement(1);

			csexstr type = StringFrom(stype);
         
         SEXCHAR cppName[256];
			SEXCHAR compressedName[256];

			TTypeMap::const_iterator k = pc.primitives.find(type);
         if (k == pc.primitives.end())
         {
            auto z = pc.interfaces.find(type);
            if (z == pc.interfaces.end())
            {
               Throw(stype, SEXTEXT("Could not find type amongst the primitives or interfaces"));
            }
            else
            {
               SafeFormat(cppName, _TRUNCATE, SEXTEXT("%s* %s = "), z->second->ic.asCppInterface.FQName(), StringFrom(svalue));
               outputPrefix.push_back(cppName);
               outputIsInterface = true;
            }
         }
         else
         {
            GetFQCppStructName(compressedName, cppName, 256, k->second.cppType.c_str());
           
            SEXCHAR moreHacks[256];
            SafeFormat(moreHacks, _TRUNCATE, SEXTEXT("%s %s = "), cppName, StringFrom(svalue));
            outputPrefix.push_back(moreHacks);
         }
		}

		if (HasAttributeThrows(attributes))
		{
			appender.Append(SEXTEXT("try\n\t\t{\n\t\t\t"));
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

			appender.Append(SEXTEXT("\t\t%s"), outputPrefix[0].c_str());
		}
		else
		{
			appender.Append(SEXTEXT("\t\t"));
		}

		appender.Append(SEXTEXT("_pObject->%s("), methodName);

		int inputCount = 1;

		// Append the input arguments to the method invocation
		for(int i = 1; i < outputStart - 1; ++i)
		{
			cr_sex s = method.GetElement(i);

			int typeIndex = 0;

			if (s.NumberOfElements() == 3)
			{
				typeIndex++;
			}
			
			cr_sex stype = s.GetElement(typeIndex);
			cr_sex svalue = s.GetElement(typeIndex + 1);
			csexstr type = StringFrom(stype);

			if (!AreEqual(type, SEXTEXT("#")))
			{
				if (inputCount > 1) appender.Append(SEXTEXT(", "));
				inputCount++;

            if (AreEqual(type, SEXTEXT("IStringBuilder")) || AreEqual(type, SEXTEXT("Sys.Type.IStringBuilder")))
            {
               appender.Append(SEXTEXT("_%sPopulator"), StringFrom(svalue));
            }
            else
            {
               if (!AreEqual(type, SEXTEXT("IString")) && !AreEqual(type, SEXTEXT("Sys.Type.IString")) && pc.structs.find(type) != pc.structs.end())
               {
                  appender.Append('*');
               }

               appender.Append(SEXTEXT("%s"), StringFrom(svalue));
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

			if (i > 1) appender.Append(SEXTEXT(", "));
			appender.Append(SEXTEXT("/* out */ %s"), StringFrom(svalue));
		}

		appender.Append(SEXTEXT(");\n"));

		if (HasAttributeThrows(attributes))
		{
			appender.Append(SEXTEXT("\t\t}\n\t\tcatch(%s& _ex) { _HandleScriptException(_nce, _ex); }\n"), pc.cppException);
		}

		for (int i = outputStart; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			
			cr_sex stype = s.GetElement(0);
			cr_sex svalue = s.GetElement(1);

			appender.Append(SEXTEXT("\t\t_offset += sizeof(%s);\n"), outputIsInterface ? SEXTEXT("CReflectedClass*") : StringFrom(svalue));

         if (outputIsInterface)
         {
            auto z = pc.interfaces.find(stype.String()->Buffer);

            NamespaceSplitter splitter(z->second->ic.asSexyInterface);

            csexstr body, tail;
            splitter.SplitTail(body, tail);

            appender.Append(SEXTEXT("\t\tauto& _%sStruct = Sexy::Helpers::GetDefaultProxy(SEXTEXT(\"%s\"),SEXTEXT(\"%s\"), SEXTEXT(\"Proxy%s\"), _nce.ss);\n"), StringFrom(svalue), body, tail, tail);
            appender.Append(SEXTEXT("\t\tCReflectedClass* _sxy%s = _nce.ss.Represent(_%sStruct, %s);\n"), StringFrom(svalue), StringFrom(svalue), StringFrom(svalue));
            appender.Append(SEXTEXT("\t\tWriteOutput(&_sxy%s->header._vTables[0], _sf, -_offset);\n"), StringFrom(svalue));
         }
         else
         {
            appender.Append(SEXTEXT("\t\tWriteOutput(%s, _sf, -_offset);\n"), StringFrom(svalue));
         }
		}

		appender.Append(SEXTEXT("\t}\n"));
	}

	void AppendFactoryArguments(FileAppender& appender, cr_sex method, int inputStart, int inputEnd, const ParseContext& pc, bool includeTypes)
	{
		csexstr argPrefix = SEXTEXT("_");

		// Append the input arguments to the method invocation
		for(int i = inputEnd; i >= inputStart; --i)
		{
			cr_sex arg = method.GetElement(i);
			
			cr_sex stype = arg.GetElement(0);
			cr_sex svalue = arg.GetElement(1);
			csexstr type = StringFrom(stype);

			if (!AreEqual(type, SEXTEXT("#")))
			{
				appender.Append(SEXTEXT(", "));

				if (AreEqual(type, SEXTEXT("IString")) || AreEqual(type, SEXTEXT("Sys.Type.IString"))) 
				{
					if (includeTypes)
					{
						TTypeMap::const_iterator j = pc.structs.find(SEXTEXT("IString"));
						if (j == pc.primitives.end())
						{
							Throw(stype, SEXTEXT("Missing IString definition in the config. Suggest adding an entry of the form: (struct IString Sys.Type.IString fstring)"));
						}

						SEXCHAR compressedStructName[256];
						SEXCHAR cppStructName[256];
						GetFQCppStructName(compressedStructName, cppStructName, 256, j->second.cppType.c_str());

						appender.Append(SEXTEXT("const %s& %s%s"), cppStructName, argPrefix, StringFrom(svalue));
					}
					else
					{
						appender.Append(SEXTEXT("%s"), StringFrom(svalue));
					}
				}
				else
				{
					TTypeMap::const_iterator j = pc.structs.find(type);
					if (j != pc.structs.end())
					{
						if (includeTypes)
						{
							csexstr cppType = j->second.cppType.c_str();
							appender.Append(SEXTEXT("const %s& %s%s"), cppType, argPrefix, StringFrom(svalue));
						}
						else
						{
							appender.Append(SEXTEXT("*%s"), StringFrom(svalue));
						}
					}
					else
					{
						if (includeTypes)
						{
							j = pc.primitives.find(type);
							if  (j == pc.primitives.end()) Throw(stype, SEXTEXT("Error evaluating type-name to a primitive or a derivative type"));

							SEXCHAR compressedStructName[256];
							SEXCHAR cppStructName[256];
							GetFQCppStructName(compressedStructName, cppStructName, 256, j->second.cppType.c_str());
							appender.Append(SEXTEXT("%s %s"), cppStructName, argPrefix);
						}
						
						appender.Append(SEXTEXT("%s"), StringFrom(svalue));
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
         appender.Append(SEXTEXT("\t"));
      }

      NamespaceSplitter splitter(ec.asCppEnum.SexyName());
      csexstr ns, tail;
      splitter.SplitTail(ns, tail);
	
#ifdef SEXCHAR_IS_WIDE
		SEXCHAR stringIndicator = L'L';
#else
		SEXCHAR stringIndicator = ' ';
#endif

		appender.Append(SEXTEXT("bool TryParse(const Rococo::fstring& s, %s& value)\n"), tail);
		appender.Append(SEXTEXT("\t{\n"));

		bool first = true;
		for (auto& i : ec.values)
		{
			appender.Append(SEXTEXT("\t\t%s (s == %c\"%s_%s\"_fstring)\n"), first ? SEXTEXT("if") : SEXTEXT("else if"), stringIndicator, tail, i.first.c_str());
			appender.Append(SEXTEXT("\t\t{\n"));
			appender.Append(SEXTEXT("\t\t\tvalue = %s_%s;\n"), tail, i.first.c_str());
			appender.Append(SEXTEXT("\t\t}\n"));

			first = false;
		}

		appender.Append(SEXTEXT("\t\telse\n"));
		appender.Append(SEXTEXT("\t\t{\n"));
		appender.Append(SEXTEXT("\t\t\treturn false;\n"));
		appender.Append(SEXTEXT("\t\t}\n\n"));

		appender.Append(SEXTEXT("\t\treturn true;\n"));
		appender.Append(SEXTEXT("\t}\n\n"));

		appender.Append(SEXTEXT("\tbool TryShortParse(const Rococo::fstring& s, %s& value)\n"), tail);
		appender.Append(SEXTEXT("\t{\n"));

		first = true;
		for (auto& i : ec.values)
		{
			appender.Append(SEXTEXT("\t\t%s (s == %c\"%s\"_fstring)\n"), first ? SEXTEXT("if") : SEXTEXT("else if"), stringIndicator, i.first.c_str());
			appender.Append(SEXTEXT("\t\t{\n"));
			appender.Append(SEXTEXT("\t\t\tvalue = %s_%s;\n"), tail, i.first.c_str());
			appender.Append(SEXTEXT("\t\t}\n"));

			first = false;
		}

		appender.Append(SEXTEXT("\t\telse\n"));
		appender.Append(SEXTEXT("\t\t{\n"));
		appender.Append(SEXTEXT("\t\t\treturn false;\n"));
		appender.Append(SEXTEXT("\t\t}\n\n"));

		appender.Append(SEXTEXT("\t\treturn true;\n"));
		appender.Append(SEXTEXT("\t}\n"));

      while (nsDepth > 0)
      {
         nsDepth--;
         appender.Append(SEXTEXT("}"));
      }

      appender.Append(SEXTEXT("// %s\n\n"), ec.asCppEnum.SexyName());
	}

	void ImplementNativeFunctions(FileAppender& appender, const InterfaceContext& ic, const ISExpression* methods, const ParseContext& pc)
	{
		appender.Append(SEXTEXT("// BennyHill generated Sexy native functions for %s \n"), ic.asCppInterface.FQName());
		appender.Append(SEXTEXT("namespace\n{\n\tusing namespace Sexy;\n\tusing namespace Sexy::Sex;\n\tusing namespace Sexy::Script;\n\tusing namespace Sexy::Compiler;\n\n"));

		if (methods != NULL)
		{
			for(int i = 1; i < methods->NumberOfElements(); ++i)
			{
				cr_sex method = methods->GetElement(i);
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
					Throw(sfactoryDef, SEXTEXT("expecting (factory <fully-qualified-sexy-function-name> <(arg-pair1)...(arg-pairN)>)"));
				}

				csexstr factoryName = StringFrom(sfactoryDef, 1);
				ValidateFQSexyFunction(sfactoryDef, factoryName);

				CppType factoryType;
				factoryType.Set(factoryName);

				appender.Append(SEXTEXT("\tvoid NativeGetHandleFor%s(NativeCallEnvironment& _nce)\n"), factoryType.CompressedName());
				appender.Append(SEXTEXT("\t{\n"));
				appender.Append(SEXTEXT("\t\tSexy::uint8* _sf = _nce.cpu.SF();\n"));
				appender.Append(SEXTEXT("\t\tptrdiff_t _offset = 2 * sizeof(size_t);\n"));

				TAttributeMap attributes;
				AddNativeInputs(attributes, appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc);
				appender.Append(SEXTEXT("\t\t%s* nceContext = reinterpret_cast<%s*>(_nce.context);\n"), ic.nceContext.FQName(), ic.nceContext.FQName());
				appender.Append(SEXTEXT("\t\t// Uses: %s* FactoryConstruct%s(%s* _context"), ic.asCppInterface.FQName(), factoryType.CompressedName(), ic.nceContext.FQName());
				AppendFactoryArguments(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc, true);
				appender.Append(SEXTEXT(");\n"));

				appender.Append(SEXTEXT("\t\t%s* pObject = FactoryConstruct%s(nceContext"), ic.asCppInterface.FQName(), factoryType.CompressedName());
				AppendFactoryArguments(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc, false);
				appender.Append(SEXTEXT(");\n"));

				appender.Append(SEXTEXT("\t\t_offset += sizeof(IString*);\n"));
				appender.Append(SEXTEXT("\t\tWriteOutput(pObject, _sf, -_offset);\n"));
				appender.Append(SEXTEXT("\t}\n"));
			}
		}

      appender.Append(SEXTEXT("}\n\n"));

		NamespaceSplitter nsSplitter(ic.asCppInterface.SexyName());
		csexstr ns, shortName;
		nsSplitter.SplitTail(ns, shortName);

		CppType nsType;
		nsType.Set(ns);

      int depth = AppendNamespace(appender, ic.asCppInterface.SexyName());

		appender.Append(SEXTEXT("\tvoid AddNativeCalls_%s(Sexy::Script::IPublicScriptSystem& ss, %s* _nceContext)\n"), ic.asCppInterface.CompressedName(), ic.nceContext.FQName());
		appender.Append(SEXTEXT("\t{\n"));

		if (methods != NULL)
		{
			appender.Append(SEXTEXT("\t\tconst INamespace& ns = ss.AddNativeNamespace(SEXTEXT(\""));

			NamespaceSplitter splitter(ic.asSexyInterface);

			csexstr ns, shortName;
			splitter.SplitTail(ns, shortName);

			appender.Append(SEXTEXT("%s.Native\"));\n"), ns);

			int factoryIndex = 0;

			if (!ic.isSingleton)
			{
				for (TExpressions::const_iterator i = ic.factories.begin(); i != ic.factories.end(); ++i, ++factoryIndex)
				{
					cr_sex sfactoryDef = **i;
					csexstr factoryName = StringFrom(sfactoryDef, 1);
					ValidateFQSexyFunction(sfactoryDef, factoryName);

					CppType factoryType;
					factoryType.Set(factoryName);

					appender.Append(SEXTEXT("\t\tss.AddNativeCall(ns, NativeGetHandleFor%s, _nceContext, SEXTEXT(\"GetHandleFor%s%d "), factoryType.CompressedName(), shortName, factoryIndex);

					for (int i = 2; i < sfactoryDef.NumberOfElements(); i++)
					{
						cr_sex arg = sfactoryDef.GetElement(i);
						AppendInputPair(appender, arg, pc);
					}

					appender.Append(SEXTEXT(" -> (Pointer hObject)\"));\n"));
				}
			}

			for(int i = 1; i < methods->NumberOfElements(); ++i)
			{
				cr_sex method = methods->GetElement(i);
				csexstr methodName = StringFrom(method.GetElement(0));
				appender.Append(SEXTEXT("\t\tss.AddNativeCall(ns, Native%s%s, %s, SEXTEXT(\""), ic.asCppInterface.CompressedName(), methodName, ic.isSingleton ? SEXTEXT("_nceContext") : SEXTEXT("nullptr"));
				appender.Append(SEXTEXT("%s%s "), shortName, methodName);
				if (!ic.isSingleton)
				{
					appender.Append(SEXTEXT("(Pointer hObject)"));
				}

				int outputPos = GetOutputPosition(method);
				for(int i = 1; i < outputPos - 1; i++)
				{
					cr_sex arg = method.GetElement(i);
					AppendInputPair(appender, arg, pc);
				}

				appender.Append(SEXTEXT(" -> "));
				for(int i = outputPos; i < method.NumberOfElements(); i++)
				{
					cr_sex arg = method.GetElement(i);
					AppendOutputPair(appender, arg, pc);
				}

				appender.Append(SEXTEXT("\"));\n"));
			}
		}
		appender.Append(SEXTEXT("\t}\n"));

      while (depth > 0)
      {
         depth--;
         appender.Append(SEXTEXT("}"));
      }

		appender.Append(SEXTEXT("\n"));
	}
}