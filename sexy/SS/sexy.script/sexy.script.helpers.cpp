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

#include "sexy.script.stdafx.h"
#include "sexy.compiler.public.h"
#include "sexy.debug.types.h"
#include "Sexy.Script.h"
#include "Sexy.Compiler.h"
#include "sexy.compiler.helpers.h"
#include "sexy.s-parser.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"

#include <stdarg.h>
#include <string>

using namespace Sexy;
using namespace Sexy::Script;
using namespace Sexy::Compiler;
using namespace Sexy::Sex;
using namespace Sexy::VM;

namespace Sexy
{
   namespace Compiler
   {
      csexstr GetTypeName(const IStructure& s)
      {
         csexstr name = s.Name();
         if (AreEqual(name, SEXTEXT("_Null_"), 6))
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
      using namespace Sexy::Debugger;

      void FormatVariableDesc(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeFormat(variable.Value, VariableDesc::VALUE_CAPACITY, format, args);
      }

      void FormatVariableDescLocation(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeFormat(variable.Location, VariableDesc::LOCATION_CAPACITY, format, args);
      }

      void FormatVariableDescName(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeFormat(variable.Name, VariableDesc::NAME_CAPACITY, format, args);
      }

      void FormatVariableDescType(VariableDesc& variable, const char* format, ...)
      {
         va_list args;
         va_start(args, format);
         SafeFormat(variable.Type, VariableDesc::TYPE_CAPACITY, format, args);
      }

      void AddSFToVarEnum(VariableDesc& variable, const Sexy::uint8* SF)
      {
         variable.Address = 0;
         FormatVariableDescLocation(variable, "CPU");
         FormatVariableDescName(variable, "SF");
         FormatVariableDescType(variable, "Register");
         FormatVariableDesc(variable, "0x%p", SF);
      }

      void AddOldSFToVarEnum(VariableDesc& variable, const Sexy::uint8* SF)
      {
         const Sexy::uint8** pOldSF = (const Sexy::uint8**)(SF - 2 * sizeof(size_t));
         variable.Address = -2 * (int) sizeof(size_t);
         FormatVariableDescLocation(variable, "CPU");
         FormatVariableDescName(variable, "Caller's SF");
         FormatVariableDescType(variable, "Register");

         if (*pOldSF != NULL)
            FormatVariableDesc(variable, "0x%p", *pOldSF);
         else
            FormatVariableDesc(variable, "Execeution Stub");
      }

      void AddReturnAddressToVarEnum(VariableDesc& variable, const Sexy::uint8* SF)
      {
         const Sexy::uint8** pRet = (const Sexy::uint8**)(SF - sizeof(size_t));
         variable.Address = -(int)sizeof(size_t);
         FormatVariableDescLocation(variable, "CPU");
         FormatVariableDescName(variable, "Return Address");
         FormatVariableDescType(variable, "Register");
         FormatVariableDesc(variable, "0x%p", *pRet);
      }

      struct AsciiName
      {
         char data[64];

         AsciiName(csexstr name)
         {
#ifndef SEXCHAR_IS_WIDE
            sprintf_s(data, 64, "%s", name);
#else
            sprintf_s(data, 64, "%S", name);
#endif
         }
      };

      void ProtectedFormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, VARTYPE type, const void* pVariableData)
      {
         switch (type)
         {
         case VARTYPE_Bad:
            StringPrint(buffer, bufferCapacity, "Bad type");
            break;
         case VARTYPE_Bool:
         {
            const Sexy::int32 value = *(const Sexy::int32*) pVariableData;
            if (value == 0 || value == 1) StringPrint(buffer, bufferCapacity, (value == 1 ? "true" : "false"));
            else StringPrint(buffer, bufferCapacity, "%d", value, value);
         }
         break;
         case VARTYPE_Derivative:
            StringPrint(buffer, bufferCapacity, "");
            break;
         case VARTYPE_Int32:
         {
            const Sexy::int32* pValue = (const Sexy::int32*) pVariableData;
            StringPrint(buffer, bufferCapacity, "%d (0x%X)", *pValue, *pValue);
         }
         break;
         case VARTYPE_Int64:
         {
            const Sexy::int64* pValue = (const Sexy::int64*) pVariableData;
            StringPrint(buffer, bufferCapacity, "%ld (0x%lX)", *pValue, *pValue);
         }
         break;
         case VARTYPE_Float32:
         {
            const float* pValue = (const float*)pVariableData;
            StringPrint(buffer, bufferCapacity, "%g", *pValue);
         }
         break;
         case VARTYPE_Float64:
         {
            const double* pValue = (const double*)pVariableData;
            StringPrint(buffer, bufferCapacity, "%lg", *pValue);
         }
         break;
         case VARTYPE_Pointer:
         {
            void **ppData = (void**)pVariableData;
            const void* ptr = *ppData;
            Sexy::csexstr symbol = ss.GetSymbol(ptr);
            if (symbol == NULL)
            {
               StringPrint(buffer, bufferCapacity, "0x%p", ptr);
            }
            else
            {
               StringPrint(buffer, bufferCapacity, sizeof(SEXCHAR) == 1 ? "%s" : "%S", symbol);
            }
         }
         break;
         default:
            StringPrint(buffer, bufferCapacity, "Unknown type");
         }
      }

      struct VirtualTable
      {
         ptrdiff_t InterfaceToInstanceOffset;
      };

      struct Instance
      {
         const VirtualTable* VTableOrTypeDef;
      };

      const Sexy::uint8* GetMemberPtr(const IStructure& s, const Sexy::uint8* instance, ptrdiff_t offset)
      {
         ptrdiff_t instanceOffset = 0;
         if (s.InterfaceCount() > 0)
         {
            const Instance* pInstance = (const Instance*)(instance + offset);
            if (pInstance->VTableOrTypeDef != NULL)
            {
               instanceOffset = pInstance->VTableOrTypeDef->InterfaceToInstanceOffset;
            }
            else
            {
               return NULL;
            }
         }
         return (instance + offset + instanceOffset);
      }

      const Sexy::Compiler::IStructure* GetConcreteType(const IStructure& s, const Sexy::uint8* instance, ptrdiff_t offset, CClassHeader*& header)
      {
         if (s.InterfaceCount() == 0) return NULL;
         header = (CClassHeader*)GetMemberPtr(s, instance, offset);
         return (header != NULL && header->_typeInfo != NULL) ? header->_typeInfo->structDef : NULL;
      }
   }
}

namespace Sexy
{ 
   namespace Script
   {
	   SCRIPTEXPORT_API void FormatValue(IPublicScriptSystem& ss, char* buffer, size_t bufferCapacity, VARTYPE type, const void* pVariableData)
	   {
   #ifdef _WIN32
		   __try
   #endif
		   {
			   ProtectedFormatValue(ss, buffer, bufferCapacity, type, pVariableData);
		   }
   #ifdef _WIN32
		   __except(1)
		   {
			   strcpy_s(buffer, bufferCapacity, "Bad pointer");
		   }
   #endif
	   }

	   SCRIPTEXPORT_API void ForeachStackLevel(Sexy::Compiler::IPublicProgramObject& obj, ICallStackEnumerationCallback& cb)
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
				   const Sexy::Compiler::IFunction* f = Sexy::Script::GetFunctionFromBytecode(obj, runningId);
				   if (f != NULL)
				   {
					   AsciiName name(f->Name());
					   cb.OnStackLevel(currentFrame, name.data);
				   }
				   else
				   {
					   char desc[128];
					   sprintf_s(desc, 128, "---Unknown function. ID_BYTECODE %u---", (uint32) (size_t) runningId);
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

	   SCRIPTEXPORT_API void EnumerateRegisters(Sexy::VM::CPU& cpu, IRegisterEnumerationCallback& cb)
	   {
		   char value[128];

		   sprintf_s(value, 128, "0x%p", cpu.PC());
		   cb.OnRegister("PC", value);

		   sprintf_s(value, 128, "0x%p", cpu.SP());
		   cb.OnRegister("SP", value);

		   sprintf_s(value, 128, "0x%p", cpu.SF());
		   cb.OnRegister("SF", value);

		   sprintf_s(value, 128, "0x%X", cpu.SR());
		   cb.OnRegister("SR", value);

		   for(int i = 4; i < 256; ++i)
		   {
			   char name[16];
			   sprintf_s(name, 16, "D%u", i);
			   sprintf_s(value, 128, "%lld / 0x%llX", cpu.D[i].int64Value, cpu.D[i].int64Value);
			   cb.OnRegister(name, value);
		   }
	   }

	   SCRIPTEXPORT_API const Sexy::Sex::ISExpression* GetSexSymbol(CPU& cpu, const uint8* pcAddress, Sexy::Script::IPublicScriptSystem& ss)
	   {
		   size_t pcOffset = cpu.PC() - cpu.ProgramStart;

		   const Sexy::Compiler::IFunction* f = GetFunctionAtAddress(ss.PublicProgramObject(), pcOffset);

		   if (f == NULL) return NULL;

		   Sexy::Compiler::CodeSection section;
		   f->Code().GetCodeSection(section);

		   IPublicProgramObject& po = ss.PublicProgramObject();
		   IVirtualMachine& vm = po.VirtualMachine();

		   size_t functionLength = po.ProgramMemory().GetFunctionLength(section.Id);
		   size_t functionStartAddress = po.ProgramMemory().GetFunctionAddress(section.Id);

		   const uint8* fstart = vm.Cpu().ProgramStart + functionStartAddress;
		   size_t fnOffset = pcOffset - po.ProgramMemory().GetFunctionAddress(section.Id);

		   const Sexy::Sex::ISExpression* s = (const Sexy::Sex::ISExpression*) f->Code().GetSymbol(fnOffset).SourceExpression;
		   return s;
	   }

	   SCRIPTEXPORT_API const Sexy::Compiler::IFunction* GetFunctionFromBytecode(const Sexy::Compiler::IModule& module, Sexy::ID_BYTECODE id)
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

	   SCRIPTEXPORT_API const Sexy::Compiler::IFunction* GetFunctionFromBytecode(Sexy::Compiler::IPublicProgramObject& obj, Sexy::ID_BYTECODE id)
	   {
		   for(int i = 0; i < obj.ModuleCount(); ++i)
		   {
			   const Sexy::Compiler::IFunction* f = GetFunctionFromBytecode(obj.GetModule(i), id);
			   if (f != NULL) return f;
		   }

		   return GetFunctionFromBytecode(obj.IntrinsicModule(), id);
	   }

	   SCRIPTEXPORT_API const Sexy::Compiler::IFunction* GetFunctionAtAddress(Sexy::Compiler::IPublicProgramObject& po, size_t pcOffset)
	   {
		   IVirtualMachine& vm = po.VirtualMachine();
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
   #ifdef _WIN32
		   __try
   #endif
		   {

			   if (sf >= cpu.StackStart + 4 && sf < cpu.StackEnd)
			   {
				   uint8* pValue = ((Sexy::uint8*) sf) - sizeof(size_t) ;
				   void** ppValue = (void**) pValue;
				   const uint8* caller = (const uint8*) *ppValue;
				   if (caller >= cpu.ProgramStart && caller < cpu.ProgramEnd)
				   {
					   return caller;
				   }
			   }
		   }
   #ifdef _WIN32
		   __except(1)

		   {
		   }
   #endif

		   return NULL;		
	   }

	   SCRIPTEXPORT_API const uint8* GetStackFrame(Sexy::VM::CPU& cpu, int32 callDepth)
	   {
		   const uint8* sf = cpu.SF();
		   for(int32 depth = callDepth; depth > 0; depth--)
		   {
			   sf = GetCallerSF(cpu, sf);
		   }
		   return sf;
	   }

	   SCRIPTEXPORT_API const uint8* GetPCAddress(Sexy::VM::CPU& cpu, int32 callDepth)
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

	   SCRIPTEXPORT_API bool GetVariableByIndex(csexstr& name, MemberDef& def, const IStructure*& pseudoType, const Sexy::uint8*& SF, IPublicScriptSystem& ss, size_t index, size_t callDepth)
	   {
		   const uint8* pc;
		   const IFunction* f;
		   size_t fnOffset;

		   if (!GetCallDescription(SF, pc, f, fnOffset, ss, callDepth)) return false;

		   if (index < 3) return false; // Not real variables
					
		   size_t count = 3;
	
		   const IStructure* lastPseudo = NULL;
		   csexstr lastPseudoName;

		   for(int i = 0; i < f->Code().GetLocalVariableSymbolCount(); ++i)
		   {
			   f->Code().GetLocalVariableSymbolByIndex(OUT def, OUT name, i);

			   if (fnOffset < def.pcStart || fnOffset > def.pcEnd)
			   {
				   continue;
			   }

			   if (AreEqual(name, SEXTEXT("_arg"), 4)) continue;

			   if (def.location == Sexy::Compiler::VARLOCATION_NONE)
			   {
				   lastPseudo = def.ResolvedType;
				   lastPseudoName = name;
				   continue;
			   }	

			   pseudoType = NULL;

			   if (count == index)
			   {
				   const void* pVariableData = SF + def.SFOffset;
				   if (def.Usage == Sexy::Compiler::ARGUMENTUSAGE_BYVALUE)
				   {
					   if (lastPseudo != NULL && lastPseudoName != NULL)
					   {
						   TokenBuffer expectedToken;
						   StringPrint(expectedToken, SEXTEXT("_ref_%s"), lastPseudoName);
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

	   SCRIPTEXPORT_API bool FindVariableByName(MemberDef& def, const IStructure*& pseudoType, const Sexy::uint8*& SF, IPublicScriptSystem& ss, csexstr searchName, size_t callDepth)
	   {
		   size_t nVariables = GetCurrentVariableCount(ss, callDepth);
		   for(size_t i = 0; i < nVariables; ++i)
		   {
			   csexstr name;
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

		   if (!GetCallDescription(sf, pc, f, fnOffset, ss, callDepth)) return 0;

		   size_t count = 0;
		   for(int i = 0; i < f->Code().GetLocalVariableSymbolCount(); ++i)
		   {
			   Sexy::Compiler::MemberDef def;
			   Sexy::csexstr name;
			   f->Code().GetLocalVariableSymbolByIndex(OUT def, OUT name, i);

			   if (fnOffset < def.pcStart || fnOffset > def.pcEnd)
			   {
				   continue;
			   }

			   if (AreEqual(name, SEXTEXT("_arg"), 4)) continue;

			   count++;
		   }

		   return count + 3; // + SF + return address + old SF
	   }

	   SCRIPTEXPORT_API void SkipJIT(Sexy::Compiler::IPublicProgramObject& po)
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
				
				   csexstr symbol = f->Code().GetSymbol(fnOffset).Text;
				   if (symbol != NULL && AreEqual(symbol, SEXTEXT("#!skip")))
				   {
					   vm.StepInto(true);
					   continue;
				   }
			   }

			   break;
		   }
	   }

	   SCRIPTEXPORT_API bool GetCallDescription(const uint8*& sf, const uint8*& pc, const IFunction*& f, size_t& fnOffset, IPublicScriptSystem& ss, size_t callDepth)
	   {
		   IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();
		   IPublicProgramObject& obj = ss.PublicProgramObject();
		   CPU& cpu = vm.Cpu();

		   sf = Sexy::Script::GetStackFrame(cpu, (int32) callDepth);
		   pc = Sexy::Script::GetPCAddress(cpu, (int32) callDepth);

		   if (pc == NULL) return false;

		   f = GetFunctionAtAddress(obj, (size_t)(pc - cpu.ProgramStart));
		   if (f == NULL) return false;

		   CodeSection cs;
		   f->Code().GetCodeSection(OUT cs);

		   const Sexy::uint8* startOfFunction = obj.ProgramMemory().GetFunctionAddress(cs.Id) + obj.ProgramMemory().StartOfMemory();
		   ptrdiff_t pcOffsetRel = pc - startOfFunction;
		   if (pcOffsetRel < 0) return false;
		   fnOffset = (size_t) pcOffsetRel;
		   return true;
	   }

	   SCRIPTEXPORT_API void ForeachVariable(Sexy::Script::IPublicScriptSystem& ss, Sexy::Debugger::IVariableEnumeratorCallback& variableEnum, size_t callDepth)
	   {
		   const uint8* sf;
		   const uint8* pc;
		   const IFunction* f;
		   size_t fnOffset;

		   if (!GetCallDescription(sf, pc, f, fnOffset, ss, callDepth)) return;

		   const Sexy::Compiler::IStructure* lastPseudo = NULL;
		   csexstr lastPseudoName;

		   VariableDesc variable;
		
		   AddSFToVarEnum(variable, sf);
		   variableEnum.OnVariable(0, variable);
		   AddReturnAddressToVarEnum(variable, sf);
		   variableEnum.OnVariable(1, variable);
		   AddOldSFToVarEnum(variable, sf);
		   variableEnum.OnVariable(2, variable);
				
		   size_t count = 3;

		   for(int i = 0; i < f->Code().GetLocalVariableSymbolCount(); ++i)
		   {
			   MemberDef def;
			   csexstr name;
			   f->Code().GetLocalVariableSymbolByIndex(OUT def, OUT name, i);

			   if (fnOffset < def.pcStart || fnOffset > def.pcEnd)
			   {
				   continue;
			   }

			   if (AreEqual(name, SEXTEXT("_arg"), 4)) continue;

			   if (def.location == Sexy::Compiler::VARLOCATION_NONE)
			   {
				   lastPseudo = def.ResolvedType;
				   lastPseudoName = name;
				   continue;
			   }			

			   const void* pVariableData = sf + def.SFOffset;
			   if (def.Usage == ARGUMENTUSAGE_BYVALUE)
			   {
				   FormatValue(ss, variable.Value, variable.VALUE_CAPACITY, def.ResolvedType->VarType(), pVariableData);

				   if (lastPseudo != NULL && lastPseudoName != NULL)
				   {
					   TokenBuffer expectedToken;
					   StringPrint(expectedToken, SEXTEXT("_ref_%s"), lastPseudoName);

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
				   const void** ppData = (const void**) pVariableData;
   #ifdef _WIN32
				   __try
   #endif
				   {
					   FormatVariableDesc(variable, "0x%p (-> 0x%p)", pVariableData, *ppData);
				   }
   #ifdef _WIN32
				   __except(1)
				   {
					   FormatVariableDesc(variable, "Bad pointer");
				   }
   #endif
				
				   AsciiName desc(Compiler::GetTypeName(*def.ResolvedType));
				   FormatVariableDescType(variable, "*%s", desc.data);

				   AsciiName asciiName(name);
				   FormatVariableDescName(variable, "%s", asciiName.data);
			   }

			   switch(def.location)
			   {
			   case VARLOCATION_NONE:
				   variable.Address = 0;
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

			   variableEnum.OnVariable(count++, variable);
		   }
	   }

	   SCRIPTEXPORT_API bool GetMembers(IPublicScriptSystem& ss, const IStructure& s, csexstr parentName, const uint8* instance, ptrdiff_t offset, MemberEnumeratorCallback& enumCallback)
	   {
		   if (s.VarType() != VARTYPE_Derivative) return true;
   #ifdef _WIN32
		   __try
   #endif
		   {
  
			   CClassHeader* concreteInstancePtr = NULL;
			   const IStructure* concreteType = GetConcreteType(s, instance, offset, concreteInstancePtr);
			   const IStructure* specimen;

			   if (concreteType != NULL) 
			   {
				   specimen = concreteType;
				   instance = (const uint8*) concreteInstancePtr;
			   }
			   else
			   {
				   specimen = &s;
				   instance += offset;
			   }

			   ptrdiff_t suboffset = 0;
			   for(int i = 0; i < s.MemberCount(); ++i)
			   {
				   const Sexy::Compiler::IMember& member = specimen->GetMember(i);

				   TokenBuffer childName;
				   StringPrint(childName, SEXTEXT("%s.%s"), parentName, member.Name());

				   enumCallback.OnMember(ss, childName, member, instance + suboffset);

				   const int sizeofMember = member.SizeOfMember();
				   suboffset += sizeofMember;
			   }

			   return true;
		   }
   #ifdef _WIN32
		   __except(1)
		   {
			   return false;
		   }
   #endif
	   }

	   SCRIPTEXPORT_API const Sexy::uint8* GetInstance(const MemberDef& def, const IStructure* pseudoType, const Sexy::uint8* SF)
	   {
		   if (pseudoType != NULL)
		   {
			   const Sexy::uint8* instancePtr = *(const Sexy::uint8**) (SF + def.SFOffset);
			   return instancePtr;
		   }
		   else
		   {
			   if (def.Usage == Sexy::Compiler::ARGUMENTUSAGE_BYREFERENCE)
			   {
				   const Sexy::uint8** ppInstance = (const Sexy::uint8**) (SF + def.SFOffset);
				   return *ppInstance;
			   }
			   else
			   {
				   return SF + def.SFOffset;
			   }
		   }
	   }

	   SCRIPTEXPORT_API csexstr GetShortName(const Sexy::Compiler::IStructure& s)
	   {
		   return IsNullType(s) ? s.GetInterface(0).Name() : s.Name();
	   }

	   SCRIPTEXPORT_API csexstr GetInstanceTypeName(const MemberDef& def, const IStructure* pseudoType)
	   {
		   return GetShortName(pseudoType != NULL ? *pseudoType : *def.ResolvedType);
	   }

	   SCRIPTEXPORT_API csexstr GetInstanceVarName(csexstr name, const IStructure* pseudoType)
	   {
		   return pseudoType != NULL ? (name + StringLength(SEXTEXT("_ref_"))) : name;
	   }

	   SCRIPTEXPORT_API const Sexy::Compiler::IStructure* FindStructure(IPublicScriptSystem& ss, csexstr fullyQualifiedName)
	   {
		   NamespaceSplitter splitter(fullyQualifiedName);

		   csexstr nsBody, stTail;
		   if (!splitter.SplitTail(nsBody, stTail))
		   {
			   LogError(ss.PublicProgramObject().Log(), SEXTEXT("Expecting fully qualified structure name, but was supplied '%s'"), fullyQualifiedName);
			   return NULL;
		   }

		   const INamespace& root = ss.PublicProgramObject().GetRootNamespace();
		   const INamespace* ns = root.FindSubspace(nsBody);

		   if (ns == NULL) return NULL;

		   return ns->FindStructure(stTail);
	   }
   } // Script
} // Sexy