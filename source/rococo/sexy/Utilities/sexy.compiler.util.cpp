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

#include <sexy.types.h>
#include <sexy.strings.h>
#include <sexy.stdstrings.h>
#include <sexy.compiler.public.h>
#include "..\STC\stccore\Sexy.Compiler.h"
#include <sexy.debug.types.h>
#include <sexy.vm.cpu.h>

using namespace Rococo::Strings;

namespace Rococo::Compiler
{
	SEXYUTIL_API const Rococo::Sex::ISExpression& GetExpression(Rococo::Script::ISxyExpressionRef sexyIExpressionRef)
	{
		const auto& ip = reinterpret_cast<const Rococo::Compiler::InterfacePointer&>(sexyIExpressionRef);
		auto* stub = InterfaceToInstance(ip);

		struct ExpressionObject : Rococo::Compiler::ObjectStub
		{
			const Rococo::Sex::ISExpression* s;
		};

		if (!Eq(stub->Desc->TypeInfo->Module().Name(), "!scripts/native/Sys.Reflection.sxy"))
		{
			Throw(0, "%s: IExpression implementation must be the native C++ Expression defined for Sys.Reflection.sxy. The implementation was %s", __ROCOCO_FUNCTION__, stub->Desc->TypeInfo->Module().Name());
		}

		auto* sHolder = static_cast<ExpressionObject*>(stub);

		return *sHolder->s;
	}

	SEXYUTIL_API IStructureBuilder* FindMember(IStructureBuilder& s, cstr name)
	{
		for(int i = 0; i < s.MemberCount(); i++)
		{
			if (AreEqual(s.GetMember(i).Name(), name))
			{
				return s.GetMember(i).UnderlyingType();
			}
		}

		return NULL;
	}

	SEXYUTIL_API const IStructure* FindMember(const IStructure& s, cstr name)
	{
		for(int i = 0; i < s.MemberCount(); i++)
		{
			if (AreEqual(s.GetMember(i).Name(), name))
			{
				return s.GetMember(i).UnderlyingType();
			}
		}

		return NULL;
	}

	SEXYUTIL_API const IMember* FindMember(const IStructure& s, cstr name, OUT int& offset)
	{
		offset = 0;

		for(int i = 0; i < s.MemberCount(); i++)
		{
			auto& m = s.GetMember(i);
			cstr memberName = m.Name();
			if (AreEqual(memberName, name))
			{
				return &m;
			}

			offset += m.SizeOfMember();
		}

		return NULL;
	}

	bool GetMethodIndices(OUT int& interfaceIndex, OUT int& methodIndex, const IStructure& s, cstr interfaceName, cstr methodName)
	{
		interfaceIndex = -1;
		methodIndex = -1;

		for(int i = 0; i < s.InterfaceCount(); ++i)
		{
			const IInterface& interf = s.GetInterface(i);
			if (AreEqual(interf.Name(), interfaceName))
			{
				for(int j = 0; j < interf.MethodCount(); ++j)
				{
					if (AreEqual(interf.GetMethod(j).Name(), methodName))
					{
						interfaceIndex = i;
						methodIndex = j;
						return true;
					}
				}
			}
		}

		return false;
	}

	bool GetMethodIndices(OUT int& interfaceIndex, OUT int& methodIndex, const IStructure& s, cstr methodName)
	{
		interfaceIndex = -1;
		methodIndex = -1;

		for(int i = 0; i < s.InterfaceCount(); ++i)
		{
			const IInterface& interf = s.GetInterface(i);
				
			for(int j = 0; j < interf.MethodCount(); ++j)
			{
				if (AreEqual(interf.GetMethod(j).Name(), methodName))
				{
					interfaceIndex = i;
					methodIndex = j;
					return true;
				}
			}
		}

		return false;
	}

	SEXYUTIL_API cstr GetFriendlyName(const IStructure& s)
	{
		return IsNullType(s) ? s.GetInterface(0).Name() : s.Name();
	}

	SEXYUTIL_API IInterfaceBuilder* GetInterface(IProgramObject& object, cstr fullyQualifiedName)
	{
		NamespaceSplitter splitter(fullyQualifiedName);

		cstr body, name;
		if (splitter.SplitTail(OUT body, OUT name))
		{
			INamespaceBuilder* origin = object.GetRootNamespace().FindSubspace(body);
			if (origin != NULL)
			{
				return origin->FindInterface(name);
			}
		}

		return NULL;
	}

	SEXYUTIL_API IFunctionBuilder* FindByName(IFunctionEnumeratorBuilder& e, cstr publicName)
	{
		for(int i = 0; i < e.FunctionCount(); ++i)
		{
			IFunctionAliasBuilder& alias = e[i];
			if (AreEqual(alias.GetPublicName(), publicName))
			{
				return &alias.GetFunction();
			}
		}

		return NULL;
	}

	SEXYUTIL_API const IFunction* FindByName(const IFunctionEnumerator& e, cstr publicName)
	{
		for(int i = 0; i < e.FunctionCount(); ++i)
		{
			const IFunctionAlias& alias = e[i];
			if (AreEqual(alias.GetPublicName(), publicName))
			{
				return &alias.GetFunction();
			}
		}

		return NULL;
	}

	SEXYUTIL_API const IFunction* GetFunctionForBytecode(IPublicProgramObject& obj, ID_BYTECODE id)
	{
		for(int i = 0; i < obj.ModuleCount(); ++i)
		{
			const IModule& m = obj.GetModule(i);
			for(int j = 0; j < m.FunctionCount(); ++j)
			{
				const IFunction& f = m.GetFunction(j);
				
				CodeSection section;
				f.Code().GetCodeSection(section);
				if (section.Id == id)
				{
					return &f;
				}
			}
		}

		return NULL;
	}

	SEXYUTIL_API bool DoesClassImplementInterface(const IStructure& s, const IInterface& testInterf)
	{
		for(int i = 0; i < s.InterfaceCount(); ++i)
		{
			const IInterface& interf = s.GetInterface(i);
			for(const IInterface* z = &interf; z != NULL; z = z->Base())
			{
				if (&testInterf == z)
				{
					return true;
				}
			}
		}

		return false;
	}

	SEXYUTIL_API INamespaceBuilder* MatchNamespace(IModuleBuilder& module, cstr name)
	{
		INamespaceBuilder& root = module.Object().GetRootNamespace();

		INamespaceBuilder* ns = root.FindSubspace(name);
		if (ns != NULL) return ns;

		for(int i = 0; i < module.PrefixCount(); ++i)
		{
			INamespaceBuilder& prefix = module.GetPrefix(i);
			ns = prefix.FindSubspace(name);
			if (ns != NULL) return ns;
		}

		return NULL;
	}

	SEXYUTIL_API IStructureBuilder* MatchStructure(ILog& logger, cstr type, IModuleBuilder& module)
	{
		if (type[0] == '_')
		{
			// Special intrinsic
			IStructureBuilder* s = module.Object().GetModule(0).FindStructure(type);
			if (s != NULL)
			{
				return s;
			}
		}

		if (type[0] != '$' && !IsCapital(type[0]))
		{
			logger.Write(("Expecting type name to begin with capital letter"));
			return NULL;
		}

		NamespaceSplitter splitter(type);

		cstr body, tail;
		if (splitter.SplitTail(OUT body, OUT tail))
		{
			INamespaceBuilder* ns;

			if (Eq(body, "$"))
			{
				ns = static_cast<INamespaceBuilder*>(const_cast<INamespace*>(module.DefaultNamespace()));
			}
			else
			{
				ns = Compiler::MatchNamespace(module, body);
			}

			if (ns == NULL) 
			{
				if (module.Object().GetWarningLevel() == EWarningLevel::Always)
				{
					char buf[256];
					SafeFormat(buf, "Could not identify namespace %s", body);
					logger.Write(buf);
				}
				return NULL;
			}

			if (!IsCapital(tail[0]) && tail[0] != '_')
			{
				logger.Write("Expecting type name to begin with capital letter");
				return NULL;
			}

			IStructureBuilder* s = ns->FindStructure(tail);
			if (s != NULL)
			{
				return s;
			}
			else
			{
				if (module.Object().GetWarningLevel() == EWarningLevel::Always)
				{
					char buf[256];
					SafeFormat(buf, "Cannot find structure %s in namespace %s", tail, body);
					logger.Write(buf);
				}
			}

			return NULL;
		}
		else
		{
			IStructureBuilder* s = module.FindStructure(type);
			if (s != NULL)
			{
				return s;
			}

			s = module.Object().IntrinsicModule().FindStructure(type);
			if (s != NULL)
			{
				return s;
			}
			
			INamespaceBuilder* sysTypes = module.Object().GetRootNamespace().FindSubspace(("Sys.Type"));
			s = sysTypes->FindStructure(type);
			if (s != NULL)
			{
				return s;
			}

			IInterfaceBuilder* interf = sysTypes->FindInterface(type);
			if (interf != NULL) return &interf->NullObjectType();

			for(int i = 0; i < module.PrefixCount(); ++i)
			{
				INamespaceBuilder& prefix = module.GetPrefix(i);
				s = prefix.FindStructure(type);
				if (s != NULL)
				{
					return s;
				}

				interf = prefix.FindInterface(type);
				if (interf != NULL)
				{
					return &interf->NullObjectType();
				}
			}

			return NULL;				
		}			
	}

} // Rococo::Compiler

namespace Rococo::Script
{
	SEXYUTIL_API void EnumerateRegisters(Rococo::VM::CPU& cpu, Rococo::Debugger::IRegisterEnumerationCallback& cb)
	{
		char value[128];

		SafeFormat(value, 128, "0x%p", cpu.PC());
		cb.OnRegister("PC", value);

		SafeFormat(value, 128, "0x%p", cpu.SP());
		cb.OnRegister("SP", value);

		SafeFormat(value, 128, "0x%p", cpu.SF());
		cb.OnRegister("SF", value);

		SafeFormat(value, 128, "0x%X", cpu.SR());
		cb.OnRegister("SR", value);

		__try
		{
			for (int i = 4; i < VM::CPU::DATA_REGISTER_COUNT; ++i)
			{
				char name[16];
				SafeFormat(name, 16, "D%u", i);
				SafeFormat(value, 128, "%lld / 0x%1llX", cpu.D[i].int64Value, cpu.D[i].int64Value);
				cb.OnRegister(name, value);
			}
		}
		__except (1)
		{

		}
	}
}