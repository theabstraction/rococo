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

#include "sexy.script.stdafx.h"
#include "sexy.compiler.public.h"
#include "sexy.debug.types.h"
#include "..\STC\stccore\sexy.compiler.helpers.h"
#include "sexy.s-parser.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"

#include <string>

#include <rococo.api.h>

#ifdef _WIN32
# include <malloc.h>
# include <excpt.h>
# define TRY_PROTECTED __try
# define CATCH_PROTECTED __except (EXCEPTION_EXECUTE_HANDLER)
#else
# define TRY_PROTECTED try
# define CATCH_PROTECTED catch (SignalException& sigEx)

# define COMBINE1(X,Y) X##Y  // helper macro
# define COMBINE(X,Y) COMBINE1(X,Y)
# define PROTECT ThrowOnSignal COMBINE(ev_segv_,__LINE__)(SIGSEGV);ThrowOnSignal COMBINE(ev_bus_,__LINE__)(SIGBUS); try
# define CATCH  catch(SignalException& ex)

namespace
{
	typedef void(*FN_SignalHandler)(int);

	struct SignalException
	{
		int signalCode;
	};

	struct ThrowOnSignal
	{
		FN_SignalHandler previousHandler;
		int code;

		static void OnSignal(int code)
		{
			throw SignalException{ code };
		}

		ThrowOnSignal(int code)
		{
			this->previousHandler = signal(code, ThrowOnSignal::OnSignal);
			this->code = code;
		}

		~ThrowOnSignal()
		{
			signal(code, previousHandler);
		}
	};
}
#endif

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::Sex;
using namespace Rococo::VM;
using namespace Rococo::Strings;

namespace Rococo
{
   namespace Compiler
   {
	   SCRIPTEXPORT_API cstr GetTypeName(const IStructure& s)
      {
         cstr name = s.Name();
         if (AreEqual(name, ("_Null_"), 6))
         {
            return s.GetInterface(0).Name();
         }
         else
         {
            return name;
         }
      }
   }
   namespace Script
   {
      using namespace Rococo::Debugger;

      void FormatVariableDesc(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeVFormat(variable.Value, VariableDesc::VALUE_CAPACITY, format, args);
      }

      void FormatVariableDescLocation(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeVFormat(variable.Location, VariableDesc::LOCATION_CAPACITY, format, args);
      }

      void FormatVariableDescName(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeVFormat(variable.Name, VariableDesc::NAME_CAPACITY, format, args);
      }

      void FormatVariableDescType(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeVFormat(variable.Type, VariableDesc::TYPE_CAPACITY, format, args);
      }

      void AddSFToVarEnum(VariableDesc& variable, const Rococo::uint8* SF)
      {
         variable.Address = 0;
         FormatVariableDescLocation(variable, "CPU");
         FormatVariableDescName(variable, "SF");
         FormatVariableDescType(variable, "Register");
         FormatVariableDesc(variable, "0x%p", SF);
      }

      void AddOldSFToVarEnum(VariableDesc& variable, const Rococo::uint8* SF)
      {
         const Rococo::uint8** pOldSF = (const Rococo::uint8**)(SF - 2 * sizeof(size_t));
         variable.Address = -2 * (int) sizeof(size_t);
         FormatVariableDescLocation(variable, "CPU");
         FormatVariableDescName(variable, "Caller's SF");
         FormatVariableDescType(variable, "Register");

         if (*pOldSF != NULL)
            FormatVariableDesc(variable, "0x%p", *pOldSF);
         else
            FormatVariableDesc(variable, "Execeution Stub");
      }

      void AddReturnAddressToVarEnum(VariableDesc& variable, const Rococo::uint8* SF)
      {
         const Rococo::uint8** pRet = (const Rococo::uint8**)(SF - sizeof(size_t));
         variable.Address = -(int)sizeof(size_t);
         FormatVariableDescLocation(variable, "CPU");
         FormatVariableDescName(variable, "Return Address");
         FormatVariableDescType(variable, "Register");
         FormatVariableDesc(variable, "0x%p", *pRet);
      }

      struct AsciiName
      {
         char data[64];

         AsciiName(cstr name)
         {
            SafeFormat(data, 64, "%s", name);
         }
      };

      void ProtectedFormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, VARTYPE type, const void* pVariableData)
      {
         switch (type)
         {
         case VARTYPE_Bad:
            SafeFormat(buffer, bufferCapacity, "Bad type");
            break;
         case VARTYPE_Bool:
         {
            const Rococo::int32 value = *(const Rococo::int32*) pVariableData;
            if (value == 0 || value == 1) SafeFormat(buffer, bufferCapacity, (value == 1 ? "true" : "false"));
            else SafeFormat(buffer, bufferCapacity, "%d", value, value);
         }
         break;
		 case VARTYPE_Array:
		 case VARTYPE_List:
		 case VARTYPE_Map:
         case VARTYPE_Derivative:
            SafeFormat(buffer, bufferCapacity, "");
            break;
         case VARTYPE_Int32:
         {
            const Rococo::int32* pValue = (const Rococo::int32*) pVariableData;
            SafeFormat(buffer, bufferCapacity, "%d (0x%X)", *pValue, *pValue);
         }
         break;
         case VARTYPE_Int64:
         {
            const Rococo::int64* pValue = (const Rococo::int64*) pVariableData;
            SafeFormat(buffer, bufferCapacity, "0x%1llX", *pValue, *pValue);
         }
         break;
         case VARTYPE_Float32:
         {
            const float* pValue = (const float*)pVariableData;
            SafeFormat(buffer, bufferCapacity, "%g", *pValue);
         }
         break;
         case VARTYPE_Float64:
         {
            const double* pValue = (const double*)pVariableData;
			double value = *pValue;
            SafeFormat(buffer, bufferCapacity, "%lg", value);
         }
         break;
         case VARTYPE_Pointer:
         {
            void **ppData = (void**)pVariableData;
            const void* ptr = *ppData;
            Rococo::cstr symbol = ss.GetSymbol(ptr);
            if (symbol == NULL)
            {
               SafeFormat(buffer, bufferCapacity, "0x%p", ptr);
            }
            else
            {
               SafeFormat(buffer, bufferCapacity, "%s", symbol);
            }
			break;
         }
		 case VARTYPE_Closure:
		 {
			 struct ArcheTypeObject
			 {
				 ID_BYTECODE byteCodeId;
				 uint8* parentSF;
			 };
			 ArcheTypeObject* e = (ArcheTypeObject*)pVariableData;
			 SafeFormat(buffer, bufferCapacity, "ID %lld - PSF %llx", e->byteCodeId, (int64) e->parentSF);
		 }
         break;
         default:
            SafeFormat(buffer, bufferCapacity, "Unknown type");
         }
      }

      const Rococo::uint8* GetMemberPtr(const IStructure& s, const Rococo::uint8* sf, ptrdiff_t offset)
      {
		 if (s.InterfaceCount() > 0)
		 {
			 InterfacePointer pInstance = (InterfacePointer)(sf);
			 ObjectStub* stub = InterfaceToInstance(pInstance);
			 return ((const uint8*)stub) + offset;
		 }

		  return sf + offset;
      }

      const Rococo::Compiler::IStructure* GetConcreteType(const IStructure& s, const Rococo::uint8* instance, ptrdiff_t offset, ObjectStub*& header)
      {
         if (s.InterfaceCount() == 0) return NULL;
		 header = (ObjectStub*) GetMemberPtr(s, instance, offset);
         return (header != NULL && header->Desc != NULL) ? header->Desc->TypeInfo : NULL;
      }
   }
}

namespace Rococo
{ 
   namespace Script
   {
	   SCRIPTEXPORT_API void FormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, VARTYPE type, const void* pVariableData)
	   {
			TRY_PROTECTED
			{
				ProtectedFormatValue(ss, buffer, bufferCapacity, type, pVariableData);
			}
			CATCH_PROTECTED
			{
				CopyString(buffer, bufferCapacity, "Bad pointer");
			}
	   }

	   SCRIPTEXPORT_API void ForeachStackLevel(Rococo::Compiler::IPublicProgramObject& obj, ICallStackEnumerationCallback& cb)
	   {
		   IProgramMemory& mem = obj.ProgramMemory();
		   CPU& cpu = obj.VirtualMachine().Cpu();
		   const uint8* sf = cpu.SF();
		   const uint8* pc = cpu.PC();

		   const uint8* currentFrame = sf;
		   const uint8* currentLine = pc;

		   for (; currentLine != NULL; currentLine = GetReturnAddress(cpu, currentFrame), currentFrame = GetCallerSF(cpu, currentFrame))
		   {
			   ID_BYTECODE runningId = mem.GetFunctionContaingAddress(currentLine - cpu.ProgramStart);
			   if (runningId != 0)
			   {
				   const Rococo::Compiler::IFunction* f = Rococo::Script::GetFunctionFromBytecode(obj, runningId);
				   if (f != NULL)
				   {
					   AsciiName name(f->Name());
					   cb.OnStackLevel(currentFrame, name.data);
				   }
				   else
				   {
					   char desc[128];
					   SafeFormat(desc, 128, "---Unknown function. ID_BYTECODE %u---", (uint32) (size_t) runningId);
					   cb.OnStackLevel(currentFrame, desc);
				   }
			   }
			   else 
			   {
				   if (currentFrame == NULL)
				   {
					   cb.OnStackLevel(currentFrame, "---Execution Stub---");
				   }
				   else
				   {
					   cb.OnStackLevel(currentFrame, "---Could not resolve function---");
				   }
			   }
		   }
	   }

	   SCRIPTEXPORT_API const Rococo::Sex::ISExpression* GetSexSymbol(CPU& cpu, const uint8*, Rococo::Script::IPublicScriptSystem& ss)
	   {
		   size_t pcOffset = cpu.PC() - cpu.ProgramStart;

		   const Rococo::Compiler::IFunction* f = GetFunctionAtAddress(ss.PublicProgramObject(), pcOffset);

		   if (f == NULL) return NULL;

		   Rococo::Compiler::CodeSection section;
		   f->Code().GetCodeSection(section);

		   IPublicProgramObject& po = ss.PublicProgramObject();
		   IVirtualMachine& vm = po.VirtualMachine();

		   //size_t functionLength = po.ProgramMemory().GetFunctionLength(section.Id);

		   size_t functionStartAddress = po.ProgramMemory().GetFunctionAddress(section.Id);

		   //const uint8* fstart = vm.Cpu().ProgramStart + functionStartAddress;

		   size_t fnOffset = pcOffset - po.ProgramMemory().GetFunctionAddress(section.Id);

		   const Rococo::Sex::ISExpression* s = (const Rococo::Sex::ISExpression*) f->Code().GetSymbol(fnOffset).SourceExpression;
		   return s;
	   }

	   SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionFromBytecode(const Rococo::Compiler::IModule& module, Rococo::ID_BYTECODE id)
	   {
		   for(int j = 0; j < module.FunctionCount(); ++j)
		   {
			   const IFunction& f = module.GetFunction(j);
				
			   CodeSection section;
			   f.Code().GetCodeSection(section);
			   if (section.Id == id)
			   {
				   return &f;
			   }
		   }

		   return NULL;
	   }

	   SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionFromBytecode(Rococo::Compiler::IPublicProgramObject& obj, Rococo::ID_BYTECODE id)
	   {
		   for(int i = 0; i < obj.ModuleCount(); ++i)
		   {
			   const Rococo::Compiler::IFunction* f = GetFunctionFromBytecode(obj.GetModule(i), id);
			   if (f != NULL) return f;
		   }

		   return GetFunctionFromBytecode(obj.IntrinsicModule(), id);
	   }

	   SCRIPTEXPORT_API const Rococo::Compiler::IFunction* GetFunctionAtAddress(Rococo::Compiler::IPublicProgramObject& po, size_t pcOffset)
	   {
		   //IVirtualMachine& vm = po.VirtualMachine();
		   IProgramMemory& mem = po.ProgramMemory();

		   ID_BYTECODE runningId = mem.GetFunctionContaingAddress(pcOffset);
		   if (runningId != 0)
		   {
			   const IFunction* f = GetFunctionFromBytecode(po, runningId);
			   return f;
		   }

		   return NULL;
	   }

	   SCRIPTEXPORT_API const uint8* GetCallerSF(CPU& cpu, const uint8* sf)
	   {
		   if (sf >= cpu.StackStart + 2 * sizeof(size_t) && sf < cpu.StackEnd)
		   {
			   uint8* pValue = ((uint8*) sf) - 2 * sizeof(size_t) ;
			   void** ppValue = (void**) pValue;

			   return (*ppValue < sf) ? (const uint8*) (*ppValue) : NULL;
		   }
		   else
		   {
			   return NULL;
		   }
	   }

	   SCRIPTEXPORT_API const uint8* GetReturnAddress(CPU& cpu, const uint8* sf)
	   {
			TRY_PROTECTED
			{
				if (sf >= cpu.StackStart + 4 && sf < cpu.StackEnd)
				{
					uint8* pValue = ((Rococo::uint8*) sf) - sizeof(size_t) ;
					void** ppValue = (void**) pValue;
					const uint8* caller = (const uint8*) *ppValue;
					if (caller >= cpu.ProgramStart && caller < cpu.ProgramEnd)
					{
						return caller;
					}
				}
			}
			CATCH_PROTECTED
			{
			}

			return NULL;		
	   }

	   SCRIPTEXPORT_API const uint8* GetStackFrame(Rococo::VM::CPU& cpu, int32 callDepth)
	   {
		   const uint8* sf = cpu.SF();
		   for(int32 depth = callDepth; depth > 0; depth--)
		   {
			   sf = GetCallerSF(cpu, sf);
		   }
		   return sf;
	   }

	   SCRIPTEXPORT_API const uint8* GetPCAddress(Rococo::VM::CPU& cpu, int32 callDepth)
	   {
		   const uint8* pc = cpu.PC();
		   const uint8* sf = cpu.SF();
		   for(int32 depth = callDepth; depth > 0; depth--)
		   {
			   pc = GetReturnAddress(cpu, sf);
			   sf = GetCallerSF(cpu, sf);			
		   }
		   return pc;
	   }

	   SCRIPTEXPORT_API bool GetVariableByIndex(cstr& name, MemberDef& def, const IStructure*& pseudoType, const Rococo::uint8*& SF, IPublicScriptSystem& ss, size_t index, size_t callDepth)
	   {
		   const uint8* pc;
		   const IFunction* f;
		   size_t fnOffset;
		   size_t pcOffset;
		   if (!GetCallDescription(SF, pc, f, fnOffset, ss, callDepth, pcOffset)) return false;

		   if (index < 3) return false; // Not real variables
					
		   size_t count = 3;
	
		   const IStructure* lastPseudo = NULL;
		   cstr lastPseudoName = nullptr;

		   for(int i = 0; i < f->Code().GetLocalVariableSymbolCount(); ++i)
		   {
			   f->Code().GetLocalVariableSymbolByIndex(OUT def, OUT name, i);

			   if (fnOffset < def.pcStart || fnOffset > def.pcEnd)
			   {
				   continue;
			   }

			   if (def.location == Rococo::Compiler::VARLOCATION_NONE)
			   {
				   lastPseudo = def.ResolvedType;
				   lastPseudoName = name;
				   continue;
			   }	

			   pseudoType = NULL;

			   if (count == index)
			   {
				   //const void* pVariableData = SF + def.SFOffset;
				   if (def.Usage == Rococo::Compiler::ARGUMENTUSAGE_BYVALUE)
				   {
					   if (lastPseudo != NULL && lastPseudoName != NULL)
					   {
						   TokenBuffer expectedToken;
						   StringPrint(expectedToken, ("_ref_%s"), lastPseudoName);
						   if (AreEqual(expectedToken.Text, name))
						   {
							   pseudoType =  lastPseudo;
						   }
					   }
				   }

				   return true;
			   }

			   count++;			
		   }

		   return false;
	   }

	   SCRIPTEXPORT_API bool FindVariableByName(MemberDef& def, const IStructure*& pseudoType, const Rococo::uint8*& SF, IPublicScriptSystem& ss, cstr searchName, size_t callDepth)
	   {
		   size_t nVariables = GetCurrentVariableCount(ss, callDepth);
		   for(size_t i = 0; i < nVariables; ++i)
		   {
			   cstr name;
			   if (!GetVariableByIndex(name, def, pseudoType, SF, ss, i, callDepth)) continue;

			   if (AreEqual(name, searchName))
			   {
				   return true;
			   }
		   }

		   return false;
	   }

	   SCRIPTEXPORT_API size_t GetCurrentVariableCount(IPublicScriptSystem& ss, size_t callDepth)
	   {
		   const uint8* sf;
		   const uint8* pc;
		   const IFunction* f;
		   size_t fnOffset;
		   size_t pcOffset;
		   if (!GetCallDescription(sf, pc, f, fnOffset, ss, callDepth, pcOffset)) return 0;

		   size_t count = 0;
		   for(int i = 0; i < f->Code().GetLocalVariableSymbolCount(); ++i)
		   {
			   Rococo::Compiler::MemberDef def;
			   Rococo::cstr name;
			   f->Code().GetLocalVariableSymbolByIndex(OUT def, OUT name, i);

			   if (fnOffset < def.pcStart || fnOffset > def.pcEnd)
			   {
				   continue;
			   }

			//   if (AreEqual(name, ("_arg"), 4)) continue;

			   count++;
		   }

		   return count + 3; // + SF + return address + old SF
	   }

	   SCRIPTEXPORT_API void SkipJIT(Rococo::Compiler::IPublicProgramObject& po)
	   {
		   IVirtualMachine& vm = po.VirtualMachine();
		   IProgramMemory& mem = po.ProgramMemory();

		   while(true)
		   {
			   size_t pcOffset = vm.Cpu().PC() - vm.Cpu().ProgramStart;
			
			   const IFunction* f = GetFunctionAtAddress(po, pcOffset);
			   if (f != NULL)
			   {
				   CodeSection section;
				   f->Code().GetCodeSection(section);

				   size_t fnOffset = pcOffset - mem.GetFunctionAddress(section.Id);
				
				   cstr symbol = f->Code().GetSymbol(fnOffset).Text;
				   if (symbol != NULL && AreEqual(symbol, ("#!skip")))
				   {
					   vm.StepInto(true);
					   continue;
				   }
			   }

			   break;
		   }
	   }

	   SCRIPTEXPORT_API bool GetCallDescription(const uint8*& sf, const uint8*& pc, const IFunction*& f, size_t& fnOffset, IPublicScriptSystem& ss, size_t callDepth, size_t& pcOffset)
	   {
		   IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		   IPublicProgramObject& obj = ss.PublicProgramObject();
		   CPU& cpu = vm.Cpu();

		   sf = Rococo::Script::GetStackFrame(cpu, (int32) callDepth);
		   pc = Rococo::Script::GetPCAddress(cpu, (int32) callDepth);

		   if (pc == NULL) return false;

		   pcOffset = (size_t) (pc - cpu.ProgramStart);

		   f = GetFunctionAtAddress(obj, pcOffset);
		   if (f == NULL) return false;

		   CodeSection cs;
		   f->Code().GetCodeSection(OUT cs);

		   const Rococo::uint8* startOfFunction = obj.ProgramMemory().GetFunctionAddress(cs.Id) + obj.ProgramMemory().StartOfMemory();
		   ptrdiff_t pcOffsetRel = pc - startOfFunction;
		   if (pcOffsetRel < 0) return false;
		   fnOffset = (size_t) pcOffsetRel;
		   return true;
	   }

	   void FormatString(REF VariableDesc& desc, cstr prefix, cstr p)
	   {
		   if (p == nullptr)
		   {
			   SafeFormat(desc.Value, "%s: <null>", prefix);
		   }
		   else if (*p == 0)
		   {
			   SafeFormat(desc.Value, "%s: <blank-string>", prefix);
		   }
		   else if (strlen(p) < 64)
		   {
			   SafeFormat(desc.Value, "%s: \"%s\"", prefix, p);
		   }
		   else
		   {
			   SafeFormat(desc.Value, "%s: \"%.64s...\"", prefix, p);
		   }
	   }

	   bool TryFormatIString(REF VariableDesc& desc, IN const ObjectStub& object, IN const IInterface& refInterf)
	   {
		   for (const Rococo::Compiler::IInterface* interface = &refInterf; interface != nullptr; interface = interface->Base())
		   {
			   if (AreEqual(interface->NullObjectType().Name(), "_Null_Sys_Type_IString"))
			   {
				   auto& stringObjt = (const CStringConstant&)object;
				   cstr p = stringObjt.pointer;
				   FormatString(desc, "", p);
				   return true;
			   }
		   }

		   return false;
	   }

	   bool TryFormatExpressionProtected(REF VariableDesc& desc, IN const ObjectStub& object, IPublicScriptSystem& ss)
	   {
		   if (ss.GetExpressionType() != object.Desc->TypeInfo)
		   {
			   return false;
		   }

		   auto& exprObject = (ObjectStubWithHandle&)object;
		   const Sex::ISExpression* s = (const Sex::ISExpression*)exprObject.handle;

		   if (s == nullptr)
		   {
			   SafeFormat(desc.Value, "Null expression pointer");
			   return true;
		   }

		   if (s->Parent() == nullptr)
		   {
				SafeFormat(desc.Value, "root: %s", s->Tree().Source().Name());
			    return true;
		   }
		   
		   switch (s->Type())
		   {
		   case EXPRESSION_TYPE_ATOMIC:
			   FormatString(desc, "atomic: ", s->c_str());
			   break;
		   case EXPRESSION_TYPE_STRING_LITERAL:
			   FormatString(desc, "string: ", s->c_str());
			   break;
		   case EXPRESSION_TYPE_NULL:
			   SafeFormat(desc.Value, "null: line %d, pos %d to line %d pos %d", s->Start().y, s->Start().x, s->End().y, s->End().x);
			   break;
		   case EXPRESSION_TYPE_COMPOUND:
			   SafeFormat(desc.Value, "compound: %d elements. line %d, pos %d to line %d pos %d", s->NumberOfElements(), s->Start().y, s->Start().x, s->End().y, s->End().x);
			   break;
		   default:
			   return false;
		   }

		   return true;
	   }

	   bool TryFormatExpression(REF VariableDesc& desc, IN const ObjectStub& object, IPublicScriptSystem& ss)
	   {
			TRY_PROTECTED
			{
				return TryFormatExpressionProtected(desc, object, ss);
			}
			CATCH_PROTECTED
			{
				SafeFormat(desc.Value, "<bad expression>");
				return true;
			}
	   }

	   void ForVariable(int index, const IFunction& f, size_t fnOffset, const Rococo::Compiler::IStructure*& lastPseudo, cstr& lastPseudoName, const uint8* sf, IPublicScriptSystem& ss, Rococo::Debugger::IVariableEnumeratorCallback& variableEnum)
	   {
		   VariableDesc variable = { 0 };

			MemberDef def;
			cstr name;
			f.Code().GetLocalVariableSymbolByIndex(OUT def, OUT name, index);

			if (fnOffset < def.pcStart || fnOffset > def.pcEnd)
			{
				return;
			}

			if (AreEqual(name, ("_arg"), 4)) return;

			if (def.location == Rococo::Compiler::VARLOCATION_NONE)
			{
				lastPseudo = def.ResolvedType;
				lastPseudoName = name;
				return;
			}

			const uint8* pVariableData = sf + def.SFOffset;
			if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			{
				if (def.ResolvedType->InterfaceCount() > 0)
				{
					auto* pInterface = *(const uint8**)pVariableData;

					ObjectStub* object;
					TRY_PROTECTED
					{
						object = (ObjectStub*)(pInterface + (*(InterfacePointer)pInterface)->OffsetToInstance);
						pVariableData = (uint8*)object;
					}
					CATCH_PROTECTED
					{
						SafeFormat(variable.Value, variable.VALUE_CAPACITY, "(bad interface) %p", pVariableData);
						return;
					}

					const auto& refInterf = def.ResolvedType->GetInterface(0);

					*variable.Value = 0;

					if (!TryFormatIString(variable, *object, refInterf))
					{
						TryFormatExpression(variable, *object, ss);
					}

					if (*variable.Value == 0)
					{
						auto& concrete = *object->Desc->TypeInfo;
						SafeFormat(variable.Value, variable.VALUE_CAPACITY, "%s of %s", concrete.Name(), concrete.Module().Name());
					}
				}
				else
				{
					FormatValue(ss, variable.Value, variable.VALUE_CAPACITY, def.ResolvedType->VarType(), pVariableData);
				}

				variable.s = def.ResolvedType;
				variable.instance = pVariableData;
				variable.parentName = nullptr;

				if (lastPseudo != NULL && lastPseudoName != NULL)
				{
					TokenBuffer expectedToken;
					StringPrint(expectedToken, ("_ref_%s"), lastPseudoName);

					if (AreEqual(expectedToken.Text, name))
					{
						AsciiName desc(Compiler::GetTypeName(*lastPseudo));
						AsciiName last(lastPseudoName);
						FormatVariableDescType(variable, "%s*", desc.data);
						FormatVariableDescName(variable, "%s", last.data);
					}
					else
					{
						AsciiName desc(Compiler::GetTypeName(*def.ResolvedType));
						AsciiName asciiName(name);
						FormatVariableDescType(variable, "%s", desc.data);
						FormatVariableDescName(variable, "%s", asciiName.data);
					}
				}
				else
				{
					AsciiName desc(Compiler::GetTypeName(*def.ResolvedType));
					AsciiName asciiName(name);
					FormatVariableDescType(variable, "%s", desc.data);
					FormatVariableDescName(variable, "%s", asciiName.data);
				}
			}
			else
			{
				const void** ppData = (const void**)pVariableData;
	
				TRY_PROTECTED
				{
					if (IsIString(*def.ResolvedType))
					{
						InterfacePointer* ip = (InterfacePointer*)ppData;
						auto* s = (CStringConstant*) InterfaceToInstance(*ip);
						cstr p = s->pointer;
						size_t len = strlen(p);
						if (len < 64)
						{
							FormatVariableDesc(variable, "-> %1llX \"%s\"", (int64)*ppData, p);
						}
						else
						{
							FormatVariableDesc(variable, "-> %1llX \"%.64s...\"", (int64)*ppData, p);
						}
					}
					else
					{
						FormatVariableDesc(variable, "-> %1llX", (int64)*ppData);
					}
				}
				CATCH_PROTECTED
				{
					FormatVariableDesc(variable, "Bad pointer");
				}

				AsciiName desc(Compiler::GetTypeName(*def.ResolvedType));
				FormatVariableDescType(variable, "*%s", desc.data);

				AsciiName asciiName(name);
				FormatVariableDescName(variable, "%s", asciiName.data);

				variable.s = def.ResolvedType;

				switch (def.ResolvedType->VarType())
				{
				case VARTYPE_Array:
				case VARTYPE_List:
				case VARTYPE_Map:
					variable.instance = (const uint8*)pVariableData;
					break;
				default:
					variable.instance = * (const uint8**)pVariableData;
				}
			}

			switch (def.location)
			{
			case VARLOCATION_NONE:
				variable.Address = 0;
				variable.instance = 0;
				FormatVariableDescLocation(variable, "Pseudo");
				break;
			case VARLOCATION_INPUT:
				variable.Address = def.SFOffset;
				FormatVariableDescLocation(variable, "Input");
				break;
			case VARLOCATION_OUTPUT:
				FormatVariableDescLocation(variable, "Output");
				variable.Address = def.SFOffset;
				break;
			case VARLOCATION_TEMP:
				FormatVariableDescLocation(variable, "Temp");
				variable.Address = def.SFOffset;
				break;
			}

			TRY_PROTECTED
			{
				variableEnum.OnVariable(index + 3, variable, def);
			}
			CATCH_PROTECTED
			{
				FormatVariableDescLocation(variable, "Bad");
				variable.Address = 0xBADBADBADBADBAD0;
			}
	   }

	   SCRIPTEXPORT_API void ForeachVariable(Rococo::Script::IPublicScriptSystem& ss, Rococo::Debugger::IVariableEnumeratorCallback& variableEnum, size_t callDepth)
	   {
		   const uint8* sf;
		   const uint8* pc;
		   const IFunction* f;
		   size_t fnOffset;
		   size_t pcOffset;
		   if (!GetCallDescription(sf, pc, f, fnOffset, ss, callDepth, pcOffset)) return;

		   const Rococo::Compiler::IStructure* lastPseudo = nullptr;
		   cstr lastPseudoName = nullptr;

		   VariableDesc variable = { 0 };

		   MemberDef registerDef = { 0 };

		   AddSFToVarEnum(variable, sf);
		   variableEnum.OnVariable(0, variable, registerDef);
		   AddReturnAddressToVarEnum(variable, sf);
		   variableEnum.OnVariable(1, variable, registerDef);
		   AddOldSFToVarEnum(variable, sf);
		   variableEnum.OnVariable(2, variable, registerDef);

		   size_t count = 3;

			for (int i = 0; i < f->Code().GetLocalVariableSymbolCount(); ++i)
			{
				TRY_PROTECTED
				{
					ForVariable(i, *f, fnOffset, lastPseudo, lastPseudoName, sf, ss, variableEnum);
				}
				CATCH_PROTECTED
				{
				}
			}
	   }

	   SCRIPTEXPORT_API bool GetMembers(IPublicScriptSystem& ss, const IStructure& s, cstr parentName, const uint8* instance, ptrdiff_t offset, MemberEnumeratorCallback& enumCallback, int recurseDepth)
	   {
		   if (s.VarType() != VARTYPE_Derivative && !IsContainerType(s.VarType())) return true;

		   if (recurseDepth > 5)
		   {
			   return false;
		   }

		   TRY_PROTECTED
		   {

			   ObjectStub* concreteInstancePtr = NULL;
			//   InterfacePointer pInterface = *(InterfacePointer*)(instance);
			   const IStructure* concreteType = GetConcreteType(s, instance, offset, concreteInstancePtr);
			   const IStructure* specimen;

			   if (concreteType != NULL)
			   {
				   specimen = concreteType;
				   instance = (const uint8*)concreteInstancePtr;
			   }
			   else
			   {
				   specimen = &s;
				   instance += offset;
			   }

			   ptrdiff_t suboffset = 0;
			   for (int i = 0; i < specimen->MemberCount(); ++i)
			   {
				   const Rococo::Compiler::IMember& member = specimen->GetMember(i);

				   TokenBuffer childName;

				   if (parentName)
				   {
					   StringPrint(childName, ("%s.%s"), parentName, member.Name());
				   }
				   else
				   {
					   StringPrint(childName, ("%s"), member.Name());
				   }

				   if (member.IsInterfaceVariable())
				   {
					   // Interfaces that are part of structures are always references
					   const uint8** ppInstance = (const uint8**)(instance + suboffset);
					   enumCallback.OnMember(ss, childName, member, *ppInstance, (int) suboffset, recurseDepth + 1);
				   }
				   else if (member.UnderlyingType() && member.UnderlyingType()->VarType() == VARTYPE_Array)
				   {
					   const ArrayImage* a = *(const ArrayImage**)(instance + suboffset);
					   enumCallback.OnArrayMember(ss, childName, member, a, instance + suboffset, (int)suboffset, recurseDepth + 1);
				   }
				   else if (member.UnderlyingType() && member.UnderlyingType()->VarType() == VARTYPE_Map)
				   {
					   const MapImage* a = *(const MapImage**)(instance + suboffset);
					   enumCallback.OnMapMember(ss, childName, member, a, instance + suboffset, (int)suboffset, recurseDepth + 1);
				   }
				   else if (member.UnderlyingType() && member.UnderlyingType()->VarType() == VARTYPE_List)
				   {
					   const ListImage* a = *(const ListImage**)(instance + suboffset);
					   enumCallback.OnListMember(ss, childName, member, a, instance + suboffset, (int)suboffset, recurseDepth + 1);
				   }
				   else
				   {
					   enumCallback.OnMember(ss, childName, member, instance + suboffset, (int) suboffset, recurseDepth + 1);
				   }

				   const int sizeofMember = member.SizeOfMember();
				   suboffset += sizeofMember;
			   }

			   return true;
		   }
		   CATCH_PROTECTED
		   {
			   return false;
		   }
	   }

	   SCRIPTEXPORT_API const Rococo::uint8* GetInstance(const MemberDef& def, const IStructure* /* pseudoType */, const Rococo::uint8* SF)
	   {
		   cstr structName = def.ResolvedType->Name();
		   if (def.ResolvedType->InterfaceCount() != 0 || AreEqual(structName, "_MapNode") || AreEqual(structName, "_Node"))
		   {
			   // An output, so SF + offset must be deferenced
			   const uint8** pItem = (const uint8**)(SF + def.SFOffset);
			   return *pItem;
		   }
		   else if (def.Usage == ARGUMENTUSAGE_BYREFERENCE)
		   {
			   const uint8** pItem = (const uint8**)(SF + def.SFOffset);
			   return *pItem;
		   }
		   else
		   {
			   return SF + def.SFOffset;
		   }
	   }

	   SCRIPTEXPORT_API cstr GetShortName(const Rococo::Compiler::IStructure& s)
	   {
		   return IsNullType(s) ? s.GetInterface(0).Name() : s.Name();
	   }

	   SCRIPTEXPORT_API cstr GetInstanceTypeName(const MemberDef& def, const IStructure* pseudoType)
	   {
		   return GetShortName(pseudoType != NULL ? *pseudoType : *def.ResolvedType);
	   }

	   SCRIPTEXPORT_API cstr GetInstanceVarName(cstr name, const IStructure* pseudoType)
	   {
		   return pseudoType != NULL ? (name + StringLength(("_ref_"))) : name;
	   }

	   SCRIPTEXPORT_API const Rococo::Compiler::IStructure* FindStructure(IPublicScriptSystem& ss, cstr fullyQualifiedName)
	   {
		   NamespaceSplitter splitter(fullyQualifiedName);

		   cstr nsBody, stTail;
		   if (!splitter.SplitTail(nsBody, stTail))
		   {
			   LogError(ss.PublicProgramObject().Log(), ("Expecting fully qualified structure name, but was supplied '%s'"), fullyQualifiedName);
			   return NULL;
		   }

		   const INamespace& root = ss.PublicProgramObject().GetRootNamespace();
		   const INamespace* ns = root.FindSubspace(nsBody);

		   if (ns == NULL) return NULL;

		   return ns->FindStructure(stTail);
	   }

	   SCRIPTEXPORT_API void AddNativeCallSecurity(IPublicScriptSystem& ss, cstr nativeNamespace, cstr permittedPingPath)
	   {
		   auto& ns = ss.AddNativeNamespace(nativeNamespace);
		   ss.AddNativeCallSecurityForNS(ns, permittedPingPath);
	   }

	   const ISExpression* GetSourceExpression(IPublicProgramObject& po, const IFunction& f, size_t pcOffset)
	   {
		   CodeSection section;
		   f.Code().GetCodeSection(section);

		   size_t fnOffset = pcOffset - po.ProgramMemory().GetFunctionAddress(section.Id);

		   const ISExpression* s = (const ISExpression*) f.Code().GetSymbol(fnOffset).SourceExpression;
		   return s;
	   }
	} // Script
} // Rococo