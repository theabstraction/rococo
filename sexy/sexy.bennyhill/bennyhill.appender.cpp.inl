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

namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;

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
				Throw(stype, SEXTEXT("Expecting primitive return type"));
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

				if (s.NumberOfElements() == 3)
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

	void DeclareCppInterface(FileAppender& appender, const InterfaceContext& ic, cr_sex interfaceDef, const ISExpression* methods, const ParseContext& pc)
	{
		int nsDepth = AppendNamespace(appender, ic.asCppInterface.SexyName());
		if (nsDepth > 0)
		{
			appender.Append(SEXTEXT("\t"));
		}

		appender.Append(SEXTEXT("struct "));
		AppendStructShortName(appender, ic.asCppInterface.SexyName());
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
			sexstringstream s;
			s << SEXTEXT("Cpp interface ") << ic.asCppInterface.SexyName() << SEXTEXT("needs a namespace prefix.") << std::ends;
			Throw(interfaceDef, s.str().c_str());
		}

		SEXCHAR cppCompressedNSName[256];
		SEXCHAR cppNSName[256];
		GetFQCppStructName(cppCompressedNSName, cppNSName, 256, ns);

		appender.Append(SEXTEXT("\n\nnamespace %s\n{\n\tvoid AddNativeCalls_%s(Sexy::Script::IPublicScriptSystem& ss, %s* nceContext);\n}\n\n"), cppNSName, ic.asCppInterface.CompressedName(), ic.nceContext.FQName());
	}

	typedef std::unordered_map<stdstring,int> TAttributeMap;
	bool HasAttributeThrows(const TAttributeMap& map)
	{
		return map.find(SEXTEXT("throws")) != map.end();
	}

	void AddNativeInputs(TAttributeMap& attributes, FileAppender& appender, cr_sex methodArgs, int inputStart, int inputEnd, const ParseContext& pc)
	{
		bool hasInitializedStringStruct = false;

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
					if (!hasInitializedStringStruct)
					{
						hasInitializedStringStruct = true;
						appender.Append(SEXTEXT("\n\t\t#pragma pack(push,1)\n"));
						appender.Append(SEXTEXT("\t\tstruct _IString { Sexy::Compiler::VirtualTable* vTable; Sexy::int32 length; Sexy::csexstr buffer; };\n"));
						appender.Append(SEXTEXT("\t\t#pragma pack(pop)\n"));
					}
				}
			
				if (!isString) 
				{
					appender.Append(SEXTEXT("\t\t"));
					AppendCppType(appender, stype, sxhtype, pc);
				}

				if (!isString && pc.structs.find(sxhtype) != pc.structs.end())
				{
					appender.Append('*');
				}

				if (!isString) appender.Append(SEXTEXT(" %s;\n"), fieldName);			
			
				if (isString)
				{
					appender.Append(SEXTEXT("\t\t_offset += sizeof(void*);\n\n"), fieldName);
				}
				else
				{
					appender.Append(SEXTEXT("\t\t_offset += sizeof(%s);\n\n"), fieldName);
				}

				if (isString) appender.Append(SEXTEXT("\t\t_IString* _%s;\n"), fieldName);

				appender.Append(isString ? SEXTEXT("\t\tReadInput(_%s, _sf, -_offset);\n") : SEXTEXT("\t\tReadInput(%s, _sf, -_offset);\n"), fieldName);

				if (isString)
				{
					appender.Append(SEXTEXT("\t\t"));
					AppendCppType(appender, stype, sxhtype, pc);
					appender.Append(SEXTEXT(" %s;\n"), fieldName);	
					appender.Append(SEXTEXT("\t\t%s.buffer = _%s->buffer;\n\t\t%s.length = _%s->length;\n\n"), fieldName, fieldName, fieldName, fieldName);
				}
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
	
		for (int i = outputStart; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			
			cr_sex stype = s.GetElement(0);
			cr_sex svalue = s.GetElement(1);

			csexstr type = StringFrom(stype);

			TTypeMap::const_iterator k = pc.primitives.find(type);
			if (k == pc.primitives.end()) Throw(stype, SEXTEXT("Could not find type amongst the primitives"));

			SEXCHAR cppName[256];
			SEXCHAR compressedName[256];
			GetFQCppStructName(compressedName, cppName, 256, k->second.cppType.c_str());

			appender.Append(SEXTEXT("\t\t%s %s;\n"), cppName, StringFrom(svalue));
		}

		if (HasAttributeThrows(attributes))
		{
			appender.Append(SEXTEXT("try\n\t\t{\n\t\t\t"));
		}

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

			appender.Append(SEXTEXT("\t\t%s = "), StringFrom(svalue));
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
			
				if (!AreEqual(type, SEXTEXT("IString")) && !AreEqual(type, SEXTEXT("Sys.Type.IString")) && pc.structs.find(type) != pc.structs.end())
				{
					appender.Append('*');
				}

				appender.Append(SEXTEXT("%s"), StringFrom(svalue));
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

			appender.Append(SEXTEXT("\t\t_offset += sizeof(%s);\n"), StringFrom(svalue));
			appender.Append(SEXTEXT("\t\tWriteOutput(%s, _sf, -_offset);\n"), StringFrom(svalue));
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
						TTypeMap::const_iterator j = pc.primitives.find(SEXTEXT("IString"));
						if (j == pc.primitives.end())
						{
							Throw(stype, SEXTEXT("Missing IString definition in the config. Suggest adding an entry of the form: (primitive IString Sys.Type.IString Sys.SexString)"));
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

	void ImplementNativeFunctions(FileAppender& appender, const InterfaceContext& ic, const ISExpression* methods, const ParseContext& pc)
	{
		appender.Append(SEXTEXT("// BennyHill generated Sexy native functions for %s \n"), ic.asCppInterface.FQName());
		appender.Append(SEXTEXT("namespace\n{\n\tusing namespace Sexy;\n\tusing namespace Sexy::Sex;\n\tusing namespace Sexy::Script;\n\tusing namespace Sexy::Compiler;\n"));

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
				appender.Append(SEXTEXT("\t\tSexy::uint8* sf = _nce.cpu.SF();\n"));
				appender.Append(SEXTEXT("\t\tptrdiff_t offset = 2 * sizeof(size_t);\n"));

				TAttributeMap attributes;
				AddNativeInputs(attributes, appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc);
				appender.Append(SEXTEXT("\t\t%s* nceContext = reinterpret_cast<%s*>(_nce.context);\n"), ic.nceContext.FQName(), ic.nceContext.FQName());
				appender.Append(SEXTEXT("\t\t// Uses: %s* FactoryConstruct%s(%s* _context"), ic.nceContext.FQName(), factoryType.CompressedName(), ic.nceContext.FQName());
				AppendFactoryArguments(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc, true);
				appender.Append(SEXTEXT(");\n"));

				appender.Append(SEXTEXT("\t\t%s* pObject = FactoryConstruct%s(nceContext"), ic.asCppInterface.FQName(), factoryType.CompressedName());
				AppendFactoryArguments(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc, false);
				appender.Append(SEXTEXT(");\n"));

				appender.Append(SEXTEXT("\t\toffset += sizeof(void*);\n"));
				appender.Append(SEXTEXT("\t\tWriteOutput(pObject, sf, -offset);\n"));
				appender.Append(SEXTEXT("\t}\n"));
			}
		}

		NamespaceSplitter nsSplitter(ic.asCppInterface.SexyName());
		csexstr ns, shortName;
		nsSplitter.SplitTail(ns, shortName);

		CppType nsType;
		nsType.Set(ns);

		appender.Append(SEXTEXT("}\nnamespace %s\n{\n\tvoid AddNativeCalls_%s(Sexy::Script::IPublicScriptSystem& ss, %s* _nceContext)\n"), nsType.FQName(), ic.asCppInterface.CompressedName(), ic.nceContext.FQName());
		appender.Append(SEXTEXT("\t{\n\n"));

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

		appender.Append(SEXTEXT("\n}\n"));
	}
}