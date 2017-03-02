namespace Sexy
{
   namespace Helpers
   {
      using namespace Sexy;
      using namespace Sexy::Sex;
      using namespace Sexy::Script;
      using namespace Sexy::Compiler;

      class StringPopulator : public IStringPopulator
      {
         CClassSysTypeStringBuilder* builder;
      public:
         StringPopulator(NativeCallEnvironment& _nce, VirtualTable* vTableBuilder)
         {
            char* _instance = ((char*)vTableBuilder) + vTableBuilder->OffsetToInstance;
            CClassDesc* _abstractClass = reinterpret_cast<CClassDesc*>(_instance);
            if (_abstractClass->structDef != _nce.ss.PublicProgramObject().GetSysType(SEXY_CLASS_ID_STRINGBUILDER))
            {
               _nce.ss.ThrowFromNativeCode(0, SEXTEXT("Builder was not a Sys.Type.StringBuilder"));
            }
            builder = reinterpret_cast<CClassSysTypeStringBuilder*>(_abstractClass);
         }
   
         virtual void Populate(csexstr text)
         {
            SafeCat(builder->buffer, builder->capacity, text, _TRUNCATE);
            builder->length = StringLength(builder->buffer);
         }
      };

      const IStructure& GetDefaultProxy(csexstr fqNS, csexstr interfaceName, csexstr proxyName, IPublicScriptSystem& ss)
      {
         auto* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(fqNS);
         if (ns == nullptr)
            Rococo::Throw(0, SEXTEXT("GetDefaultProxy: Cannot find subspace %s"), fqNS);

         auto* pInt = ns->FindInterface(interfaceName);
         if (pInt == nullptr)
            Rococo::Throw(0, SEXTEXT("GetDefaultProxy: Cannot find interface %s in subspace"), interfaceName);

         const Sexy::Compiler::IStructure& nullType = pInt->NullObjectType();
         auto& mod = nullType.Module();

         const IStructure* s = mod.FindStructure(proxyName);
         if (s == nullptr)
            Rococo::Throw(0, SEXTEXT("GetDefaultProxy: Cannot find proxy %s in %s"), proxyName, mod.Name());

         return *s;
      }
   } // Helpers
} // Sexy