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
	void AppendProxyName(FileAppender& appender, csexstr fqInterfaceName, cr_sex s)
	{
		NamespaceSplitter splitter(fqInterfaceName);

		csexstr ns, shortname;
		if(!splitter.SplitTail(ns, shortname))
		{
			sexstringstream<1024> err;
			err.sb << SEXTEXT("Could not split interface name: ") << fqInterfaceName;
			Throw(s, *err.sb);
		}

		appender.Append(SEXTEXT("Proxy%s"), shortname);
	}

	void AppendInputPair(FileAppender& appender, cr_sex s, const ParseContext& pc)
	{
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

		csexstr type = StringFrom(s, typeIndex);
		if (type[0] == '#' && type[1] == 0)
		{
		}
		else
		{
			ValidateSexyType(stype);
			ValidateSexyVariable(svalue);

			csexstr sxhtype = stype.String()->Buffer;
			auto i = pc.primitives.find(sxhtype);
			if (i == pc.primitives.end())
			{
				i = pc.structs.find(sxhtype);
				if (i == pc.structs.end())
				{
					Throw(stype, SEXTEXT("Could not resolve sexy type"));
				}
			}

			appender.Append(SEXTEXT("(%s %s)"), i->second.sexyType.c_str(), svalue.String()->Buffer);
		}
	}

	void AppendInputValue(FileAppender& appender, cr_sex s, const ParseContext& pc)
	{
		if (s.NumberOfElements() != 2)
		{
			Throw(s, SEXTEXT("Expected input argument pair (<type> <value>)"));
		}

		cr_sex stype = s.GetElement(0);
		cr_sex svalue = s.GetElement(1);

		csexstr type = StringFrom(s, 0);
		if (type[0] == '#' && type[1] == 0)
		{
			// attribute arguments are not appended
		}
		else
		{
			ValidateSexyVariable(svalue);
			appender.Append(SEXTEXT("%s"), svalue.String()->Buffer);
		}
	}

	void AppendOutputPair(FileAppender& appender, cr_sex s, const ParseContext& pc)
	{
		if (s.NumberOfElements() != 2)
		{
			Throw(s, SEXTEXT("Expected output argument pair (<type> <value>)"));
		}

		cr_sex stype = s.GetElement(0);
		cr_sex svalue = s.GetElement(1);

		ValidateSexyType(stype);
		ValidateSexyVariable(svalue);

		csexstr sxhtype = stype.String()->Buffer;
		auto i = pc.primitives.find(sxhtype);
		if (i == pc.primitives.end())
		{
         i = pc.structs.find(sxhtype);
         if (i != pc.structs.end())
         {
            Throw(stype, SEXTEXT("The output type is a struct. Only primitives and generated interfaces are legal output for sexy functions."));
         }

         auto j = pc.interfaces.find(sxhtype);
         if (j == pc.interfaces.end())
         {
            Throw(stype, SEXTEXT("Could not resolve sexy type. Double-check the namespaces and the type name"));
         }			
         else
         {
            appender.Append(SEXTEXT("(%s %s)"), j->second->ic.asSexyInterface, svalue.String()->Buffer);
         }
		}
      else
      {
         appender.Append(SEXTEXT("(%s %s)"), i->second.sexyType.c_str(), svalue.String()->Buffer);
      }
	}

	int GetOutputPosition(cr_sex method)
	{
		for(int i = 1; i < method.NumberOfElements(); ++i)
		{
			cr_sex s = method.GetElement(i);
			if (IsAtomic(s))
			{
				csexstr arg = s.String()->Buffer;
				if (AreEqual(arg, SEXTEXT("->")))
				{
					return i + 1;
				}
			}
		}

		return method.NumberOfElements() + 1;
	}

	void AppendInputsAndOutputs(FileAppender& appender, cr_sex method, const ParseContext& pc)
	{
		int outputPos = GetOutputPosition(method);
		
		for(int i = 1; i < outputPos - 1; ++i)
		{
			AppendInputPair(appender, method.GetElement(i), pc);
		}

		appender.Append(SEXTEXT(" -> "));

		for (int i = outputPos; i < method.NumberOfElements(); ++i)
		{
			AppendOutputPair(appender,  method.GetElement(i), pc);
		}
	}

	void AppendSexyMethod(FileAppender& appender, cr_sex method, const ParseContext& pc)
	{
		appender.Append(SEXTEXT("\t("));

		cr_sex smethodName = method.GetElement(0);
		csexstr methodName = smethodName.String()->Buffer;
		appender.Append(SEXTEXT("%s "), methodName);
	
		AppendInputsAndOutputs(appender, method, pc);

		appender.Append(SEXTEXT(")\n"));
	}

	void AppendFQNativeName(FileAppender& appender, cr_sex method, csexstr interfaceName)
	{
		NamespaceSplitter splitter(interfaceName);

		csexstr ns, shortName;
		splitter.SplitTail(ns, shortName);

		csexstr methodName = method.GetElement(0).String()->Buffer;
		appender.Append(SEXTEXT("%s.Native.%s%s"), ns, shortName, methodName);
	}

	void AppendVarNameFromPair(FileAppender& appender, cr_sex arg)
	{
		int nameIndex = 1;

		if (arg.NumberOfElements() == 3)
		{
			nameIndex++;
		}

		csexstr argName = StringFrom(arg, nameIndex);
		appender.Append(SEXTEXT("%s"), argName);
	}

	void AppendNativeInvoke(FileAppender& appender, const InterfaceContext& ic, cr_sex method, const ParseContext& pc)
	{
		appender.Append('(');
		AppendFQNativeName(appender, method, ic.asSexyInterface);

		if (!ic.isSingleton) appender.Append(SEXTEXT(" this.hObject "));
		else  appender.Append(SEXTEXT(" "));

		int outputPos = GetOutputPosition(method);

		int inputCount = 1;
		
		for(int i = 1; i < outputPos - 1; ++i)
		{
			cr_sex arg = method.GetElement(i);
			if (AreEqual(StringFrom(arg,0), SEXTEXT("#")))
			{
			}
			else
			{
				if (inputCount > 1) appender.Append(' ');
				inputCount++;
				AppendVarNameFromPair(appender, method.GetElement(i));
			}
		}

		if (outputPos < method.NumberOfElements())
		{
			appender.Append(SEXTEXT(" -> "));

			for (int i = outputPos; i < method.NumberOfElements(); ++i)
			{
				if (i > outputPos) appender.Append(' ');
				AppendVarNameFromPair(appender,  method.GetElement(i));
			}
		}

		appender.Append(')');
	}

	void AppendSexyMethodProxy(FileAppender& appender, const InterfaceContext& ic, cr_sex method, const ParseContext& pc)
	{
		cr_sex smethodName = method.GetElement(0);
		csexstr methodName = smethodName.String()->Buffer;

		appender.Append(SEXTEXT("\t(method "));
		AppendProxyName(appender, ic.asSexyInterface, method);
		appender.Append(SEXTEXT(".%s "), methodName);
	
		AppendInputsAndOutputs(appender, method, pc);

		appender.Append(SEXTEXT(" : "));
		AppendNativeInvoke(appender, ic, method, pc);
		appender.Append(SEXTEXT(")\n"));
	}

	void DeclareSexyEnum(FileAppender& appender, const EnumContext& ec, cr_sex senumDef, const ParseContext& pc)
	{
		NamespaceSplitter splitter(ec.asSexyEnum);

		csexstr body, tail;
		if (!splitter.SplitTail(body, tail))
		{
			Throw(senumDef, SEXTEXT("Could not split namespace of enum definition"));
		}

		for (auto &i : ec.values)
		{
			appender.Append(SEXTEXT("(macro %s.%s%s in out (out.AddAtomic \"0x%I64X\"))\n"), body, tail, i.first.c_str(), i.second);
		}

		appender.Append(SEXTEXT("\n"));
	}


	void DeclareSexyInterface(FileAppender& appender, const InterfaceContext& ic, const ISExpression* methods, const ParseContext& pc)
	{
		appender.Append(SEXTEXT("(interface %s"), ic.asSexyInterface);

		if (methods != NULL)
		{
			appender.Append('\n');
			// The first element in the expression is the 'method' keyword, which we have validated elsewhere
			for(int i = 1; i < methods->NumberOfElements(); ++i)
			{
				cr_sex method = methods->GetElement(i);
				if (!IsCompound(method)) Throw(method, SEXTEXT("Expecting compound expression for the method"));

				cr_sex smethodName = method.GetElement(0);
				if (!IsAtomic(smethodName)) Throw(smethodName, SEXTEXT("Expecting method name"));
				csexstr methodName = smethodName.String()->Buffer;
				ValidateSexyType(smethodName, methodName);

				AppendSexyMethod(appender, method, pc);
			}
		}

		appender.Append(SEXTEXT(")\n\n"));
	}

	void AppendFactoryInputs(FileAppender& appender, cr_sex sfactoryDef, int inputStart, int inputEnd, const ParseContext& pc)
	{
		for(int i = inputStart; i <= inputEnd; ++i)
		{
			cr_sex input = sfactoryDef.GetElement(i);
			AppendInputPair(appender, input, pc);
		}
	}

	void AppendFactoryInputValues(FileAppender& appender, cr_sex sfactoryDef, int inputStart, int inputEnd, const ParseContext& pc)
	{
		for(int i = inputStart; i <= inputEnd; ++i)
		{
			cr_sex input = sfactoryDef.GetElement(i);
			AppendInputValue(appender, input, pc);
		}
	}

	void ImplementSexyInterface(FileAppender& appender, const InterfaceContext& ic, const ISExpression* methods, cr_sex interfaceDef, const ParseContext& pc)
	{
		appender.Append(SEXTEXT("(class "));
		AppendProxyName(appender, ic.asSexyInterface, interfaceDef);

		if (ic.isSingleton)
		{
			appender.Append(SEXTEXT(" (implements %s))\n"), ic.asSexyInterface);
		}
		else
		{
			appender.Append(SEXTEXT(" (implements %s) (Pointer hObject))\n"), ic.asSexyInterface);
		}
		
		NamespaceSplitter splitter(ic.asSexyInterface);

		csexstr ns, shortName;
		splitter.SplitTail(ns, shortName);

		if (!ic.factories.empty())
		{
			appender.Append(SEXTEXT("\t(method "));
			AppendProxyName(appender, ic.asSexyInterface, interfaceDef);

			if (ic.isSingleton)
			{
				appender.Append(SEXTEXT(".Construct : )\n"));
			}
			else
			{
				appender.Append(SEXTEXT(".Construct (Pointer hObject): (this.hObject = hObject))\n"), ns, shortName);
			}
		}

		if (ic.hasDestructor)
		{
			if (ic.isSingleton)
			{
				Throw(interfaceDef, SEXTEXT("Singletons must not define a destructor"));
			}

			appender.Append(SEXTEXT("\t("));
			AppendProxyName(appender, ic.asSexyInterface, interfaceDef);
			appender.Append(SEXTEXT(".Destruct -> : "));
			
			appender.Append('(');
			
			appender.Append(SEXTEXT("%s.Native.%sDestruct"), ns, shortName);

			appender.Append(SEXTEXT(" this.hObject))\n"));
		}

		if (methods != NULL)
		{
			// The first element in the expression is the 'method' keyword, which we have validated elsewhere
			for(int i = 1; i < methods->NumberOfElements(); ++i)
			{
				cr_sex method = methods->GetElement(i);
				if (!IsCompound(method)) Throw(method, SEXTEXT("Expecting compound expression for the method"));

				cr_sex smethodName = method.GetElement(0);
				if (!IsAtomic(smethodName)) Throw(smethodName, SEXTEXT("Expecting method name"));
				csexstr methodName = smethodName.String()->Buffer;
				ValidateSexyType(smethodName, methodName);

				AppendSexyMethodProxy(appender, ic, method, pc);
			}
		}

		appender.Append('\n');

		if (ic.factories.size() != 1 && ic.isSingleton)
		{
			Throw(interfaceDef , SEXTEXT("Singleton objects should only require one factory to be defined."));
		}

		int factoryIndex = 0;
		for(TExpressions::const_iterator i = ic.factories.begin(); i != ic.factories.end(); ++i, ++factoryIndex)
		{
			cr_sex sfactoryDef = **i;
			
			csexstr factoryName = StringFrom(sfactoryDef, 1);
			ValidateFQSexyFunction(sfactoryDef, factoryName);

			NamespaceSplitter splitter(factoryName);
			csexstr head, tail;
			if (!splitter.SplitHead(head, tail))
			{
				Throw(sfactoryDef, SEXTEXT("The factory must be fully qualified. i.e Sys.Animals.GetTiger and not GetTiger"));
			}

			appender.Append(SEXTEXT("(factory %s %s "), factoryName, ic.asSexyInterface);

			if (ic.isSingleton && sfactoryDef.NumberOfElements() != 2)
			{
				Throw(sfactoryDef, SEXTEXT("The factories of singleton objects take no arguments"));
			}

			AppendFactoryInputs(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements()-1, pc);

			appender.Append(SEXTEXT(" :\n"));

			if (!ic.isSingleton)
			{
				appender.Append(SEXTEXT("\t(Pointer pObject = (%s.Native.GetHandleFor%s%d "), ns, shortName, factoryIndex);
				AppendFactoryInputValues(appender, sfactoryDef, 2, sfactoryDef.NumberOfElements() - 1, pc);
				appender.Append(SEXTEXT("))\n"));
				appender.Append(SEXTEXT("\t(construct "));
				AppendProxyName(appender, ic.asSexyInterface, interfaceDef);
				appender.Append(SEXTEXT(" pObject)\n)\n\n"));
			}
			else
			{
				appender.Append(SEXTEXT("\t(construct "));
				AppendProxyName(appender, ic.asSexyInterface, interfaceDef);
				appender.Append(SEXTEXT(")\n)\n\n"));
			}
		}
	}
}