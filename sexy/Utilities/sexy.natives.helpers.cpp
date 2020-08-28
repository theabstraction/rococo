#include <sexy.types.h>
#include <sexy.stdstrings.h>
#include <sexy.strings.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <sexy.compiler.h>

namespace Rococo
{
   namespace Helpers
   {
      using namespace Rococo;
      using namespace Rococo::Sex;
      using namespace Rococo::Script;
      using namespace Rococo::Compiler;
     
	  StringPopulator::StringPopulator(NativeCallEnvironment& _nce, InterfacePointer pInterface)
	  {
		  auto* stub = InterfaceToInstance(pInterface);
		  auto* fsb = (FastStringBuilder*)stub;

		  if (fsb->stub.Desc->TypeInfo != _nce.ss.GetStringBuilderType())
		  {
			  _nce.ss.ThrowFromNativeCode(0, "Builder was not allocated using NewTokenBuilder, NewPathBuilder, NewParagraphBuilder or NewStringBuilder");
		  }
		  builder = fsb;
	  }
   
      void StringPopulator::Populate(cstr text)
      {
         StackStringBuilder sb(builder->buffer, builder->capacity, StringBuilder::BUILD_EXISTING);
         sb << text;
         builder->length = sb.Length();
      }

      const IStructure& GetDefaultProxy(cstr fqNS, cstr interfaceName, cstr proxyName, IPublicScriptSystem& ss)
      {
         auto* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(fqNS);
         if (ns == nullptr)
            Rococo::Throw(0, "GetDefaultProxy: Cannot find subspace %s", fqNS);

         auto* pInt = ns->FindInterface(interfaceName);
         if (pInt == nullptr)
            Rococo::Throw(0, "GetDefaultProxy: Cannot find interface %s in subspace", interfaceName);

         const Rococo::Compiler::IStructure& nullType = pInt->NullObjectType();
         auto& mod = nullType.Module();

         const IStructure* s = mod.FindStructure(proxyName);
         if (s == nullptr)
            Rococo::Throw(0, "GetDefaultProxy: Cannot find proxy %s in %s", proxyName, mod.Name());

         return *s;
      }
   } // Helpers
} // Sexy