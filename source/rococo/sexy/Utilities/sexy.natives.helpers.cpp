#define IS_SCRIPT_DLL

#include <sexy.script.h>
#include <sexy.stdstrings.h>
#include <sexy.strings.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.vm.cpu.h>
#include "..\STC\stccore\Sexy.Compiler.h"
#include <sexy.util.exports.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::Strings;

namespace Rococo
{
   namespace Helpers
   {    
      SEXYUTIL_API StringPopulator::StringPopulator(NativeCallEnvironment& _nce, InterfacePointer pInterface)
	  {
		  auto* stub = InterfaceToInstance(pInterface);
		  auto* fsb = (FastStringBuilder*)stub;

		  if (!fsb->stub.Desc->flags.IsSystem)
		  {
			  _nce.ss.ThrowFromNativeCodeF(0, "Builder %s was not a system string builder", GetFriendlyName(*fsb->stub.Desc->TypeInfo));
              return;
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