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

namespace Sexy
{
   namespace Script
   {
      const IFactory& GetFactoryInModuleByFQN(cr_sex factoryExpr, csexstr ns, csexstr shortName, IModule& module)
      {
         const INamespace* NS = module.Object().GetRootNamespace().FindSubspace(ns);
         if (NS == NULL)
         {
            sexstringstream streamer;
            streamer << SEXTEXT("Cannot find the namespace: ") << ns << std::ends;
            Throw(factoryExpr, streamer.str().c_str());
         }

         const IFactory* factory = NS->FindFactory(shortName);
         if (factory == NULL)
         {
            sexstringstream streamer;
            streamer << SEXTEXT("Cannot find the factory in the namespace: ") << shortName;
            Throw(factoryExpr, streamer);
         }

         return *factory;
      }

      IFactory& GetFactoryInModuleByPrefix(cr_sex factoryExpr, IModuleBuilder& module)
      {
         IFactory* uniqueFactory = NULL;
         INamespace* NS = NULL;

         for (int i = 0; i < module.PrefixCount(); i++)
         {
            INamespaceBuilder& prefix = module.GetPrefix(i);
            IFactory* factory = prefix.FindFactory(factoryExpr.String()->Buffer);
            if (factory != NULL)
            {
               if (NS != NULL)
               {
                  sexstringstream streamer;
                  streamer << SEXTEXT("Module uses more than one namespace that has a factory of the given name: ") << NS->FullName() << SEXTEXT(" and ") << prefix.FullName();
                  Throw(factoryExpr, streamer);
               }

               uniqueFactory = factory;
               NS = &prefix;
            }
         }

         if (uniqueFactory == NULL)
         {
            sexstringstream streamer;
            streamer << SEXTEXT("Cannot find the factory in any namespace used by the module");
            Throw(factoryExpr, streamer);
         }

         return *uniqueFactory;
      }

      const IFactory& GetFactoryInModule(cr_sex factoryExpr, IModuleBuilder& module)
      {
         sexstring factoryName = factoryExpr.String();
         NamespaceSplitter splitter(factoryName->Buffer);

         csexstr ns, shortName;
         if (splitter.SplitTail(OUT ns, OUT shortName))
         {
            return GetFactoryInModuleByFQN(factoryExpr, ns, shortName, module);
         }
         else
         {
            return GetFactoryInModuleByPrefix(factoryExpr, module);
         }
      }

      void CompileFactoryCall(CCompileEnvironment& ce, const IFactory& factory, csexstr id, csexstr refId, cr_sex args, const IInterface& interf)
      {
         const IFunction& factoryFunction = factory.Constructor();
         const IFunction* inlineConstructor = factory.InlineConstructor();
         const IStructure* inlineClass = factory.InlineClass();

         CodeSection section;
         if (inlineConstructor == NULL)
         {
            ce.Builder.Append_InitializeVirtualTable(id);
            factoryFunction.Code().GetCodeSection(OUT section);
         }
         else
         {
            ce.Builder.Append_InitializeVirtualTable(id, *inlineClass);
            inlineConstructor->Code().GetCodeSection(OUT section);
         }

         int explicitInputCount = ArgCount(factoryFunction) - 1;
         int mapIndex = GetIndexOf(1, args, SEXTEXT("->"));
         if (mapIndex > 0) Throw(args, SEXTEXT("Mapping token are not allowed in constructor calls, which have no output"));
         if (args.NumberOfElements() - 1 < explicitInputCount) Throw(args, SEXTEXT("Too few arguments to factory call"));
         if (args.NumberOfElements() - 1 > explicitInputCount) Throw(args, SEXTEXT("Too many arguments to factory call"));

         int inputStackAllocCount = PushInputs(ce, args, factoryFunction, true, 1);
         inputStackAllocCount += CompileInstancePointerArg(ce, id);

         ce.Builder.AddSymbol(factoryFunction.Name());
         ce.Builder.Assembler().Append_CallById(section.Id);

         ce.Builder.MarkExpression(args.Parent());

         RepairStack(ce, *args.Parent(), factoryFunction);

         if (inlineConstructor == NULL)
         {
            ce.Builder.AssignTempToVariable(0, refId); // Factory instanced constructor calls leave the address of the new interface into D4 which updates the reference.
         }
         else
         {
            int interfaceIndex = GetIndexOfInterface(*inlineClass, interf);
            if (interfaceIndex < 0)
            {
               sexstringstream streamer;
               streamer << inlineConstructor->Name() << SEXTEXT(" does not support the interface ") << interf.Name();
               Throw(args, streamer);
            }

            ce.Builder.AssignVariableRefToTemp(id, 0, sizeof(size_t) * (interfaceIndex + 1) + sizeof(int32));
            ce.Builder.AddSymbol(refId);
            ce.Builder.AssignTempToVariable(0, refId);
         }

         ce.Builder.AssignClosureParentSF();
      }

      bool TryCompileAsLateFactoryCall(CCompileEnvironment& ce, const MemberDef& targetDef, cr_sex directive)
      {
         cr_sex factoryCall = directive.GetElement(2);

         if (!IsCompound(factoryCall))
         {
            return false;
         }

         if (factoryCall.NumberOfElements() < 1)
         {
            return false;
         }

         cr_sex factoryNameExpr = factoryCall.GetElement(0);
         if (!IsAtomic(factoryNameExpr))
         {
            return false;
         }

         const IStructure& targetStruct = *targetDef.ResolvedType;
         if (!IsNullType(targetStruct))
         {
            return false;
         }

         csexstr targetName = directive.GetElement(0).String()->Buffer;

         TokenBuffer targetRefName;
         GetRefName(targetRefName, targetName);

         MemberDef refDef;
         if (!ce.Builder.TryGetVariableByName(OUT refDef, targetRefName))
         {
            return false;
         }

         const IFactory& factory = GetFactoryInModule(factoryNameExpr, GetModule(ce.Script));

         CompileFactoryCall(ce, factory, targetName, targetRefName, factoryCall, targetStruct.GetInterface(0));

         return true;
      }

      void CompileConstructFromFactory(CCompileEnvironment& ce, const IStructure& nullType, csexstr id, cr_sex args)
      {
         // This function turns (<IInterface> id (<Factory> <arg1>...<argN>)) into assembly

         TokenBuffer refName;
         GetRefName(refName, id);

         ce.Builder.AddSymbol(refName);
         AddVariable(ce, NameString::From(refName), ce.Object.Common().TypePointer());

         AddSymbol(ce.Builder, SEXTEXT("%s %s"), GetFriendlyName(nullType), id);

         AddVariable(ce, NameString::From(id), nullType);

         cr_sex factoryExpr = GetAtomicArg(args, 0);
         const IFactory& factory = GetFactoryInModule(factoryExpr, GetModule(ce.Script));

         CompileFactoryCall(ce, factory, id, refName, args, nullType.GetInterface(0));
      }
   }//Script
}//Sexy