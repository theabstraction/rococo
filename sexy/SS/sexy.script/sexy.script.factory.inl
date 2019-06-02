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

namespace Rococo
{
   namespace Script
   {
	  int GetIndexOfInterface(const IStructure& concreteClass, const IInterface& interf);

      const IFactory& GetFactoryInModuleByFQN(cr_sex factoryExpr, cstr ns, cstr shortName, IModule& module)
      {
         const INamespace* NS = module.Object().GetRootNamespace().FindSubspace(ns);
         if (NS == NULL)
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot find the namespace: ") << ns;
            Throw(factoryExpr, *streamer.sb);
         }

         const IFactory* factory = NS->FindFactory(shortName);
         if (factory == NULL)
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot find the factory in the namespace: ") << shortName;
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
                  sexstringstream<1024> streamer;
                  streamer.sb << ("Module uses more than one namespace that has a factory of the given name: ") << NS->FullName()->Buffer << (" and ") << prefix.FullName()->Buffer;
                  Throw(factoryExpr, *streamer.sb);
               }

               uniqueFactory = factory;
               NS = &prefix;
            }
         }

         if (uniqueFactory == NULL)
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot find the factory in any namespace used by the module");
            Throw(factoryExpr, *streamer.sb);
         }

         return *uniqueFactory;
      }

      const IFactory& GetFactoryInModule(cr_sex factoryExpr, IModuleBuilder& module)
      {
         sexstring factoryName = factoryExpr.String();
         NamespaceSplitter splitter(factoryName->Buffer);

         cstr ns, shortName;
         if (splitter.SplitTail(OUT ns, OUT shortName))
         {
            return GetFactoryInModuleByFQN(factoryExpr, ns, shortName, module);
         }
         else
         {
            return GetFactoryInModuleByPrefix(factoryExpr, module);
         }
      }

      void CompileFactoryCall(CCompileEnvironment& ce, const IFactory& factory, cstr interfaceRefName, cr_sex args, const IInterface& interf)
      {
         const IFunction& factoryFunction = factory.Constructor();
         const IFunction* inlineConstructor = factory.InlineConstructor();
         const IStructure* inlineClass = factory.InlineClass();

		 if (inlineConstructor != NULL)
		 {
			 int interfaceIndex = GetIndexOfInterface(*inlineClass, interf);
			 if (interfaceIndex < 0)
			 {
				 sexstringstream<1024> streamer;
				 streamer.sb << inlineConstructor->Name() << (" does not support the interface ") << interf.Name();
				 Throw(args, streamer);
			 }
		 }

		 MemberDef def;
		 if (!ce.Builder.TryGetVariableByName(OUT def, interfaceRefName))
		 {
			 Throw(0, "Error, cannot find variable %s ", interfaceRefName);
		 }

         CodeSection section;
         factoryFunction.Code().GetCodeSection(OUT section);

         int explicitInputCount = ArgCount(factoryFunction);
         int mapIndex = GetIndexOf(1, args, ("->"));
         if (mapIndex > 0) Throw(args, ("Mapping token are not allowed in constructor calls, which have no output"));
         if (args.NumberOfElements() - 1 < explicitInputCount) Throw(args, ("Too few arguments to factory call"));
         if (args.NumberOfElements() - 1 > explicitInputCount) Throw(args, ("Too many arguments to factory call"));

         int inputStackAllocCount = PushInputs(ce, args, factoryFunction, false, 1);
        
         ce.Builder.AddSymbol(factoryFunction.Name());
         ce.Builder.Assembler().Append_CallById(section.Id); // pointer to interface should now be in D4

         ce.Builder.MarkExpression(args.Parent());

         RepairStack(ce, *args.Parent(), factoryFunction);

		 ce.Builder.AssignTempToVariable(0, interfaceRefName);

         ce.Builder.AssignClosureParentSFtoD6();
      }

      bool TryCompileAsLateFactoryCall(CCompileEnvironment& ce, const MemberDef& targetDef, cr_sex directive)
      {
		  cr_sex factoryCall = directive[2];

         if (!IsCompound(factoryCall))
         {
            return false;
         }

         if (factoryCall.NumberOfElements() < 1)
         {
            return false;
         }

         cr_sex factoryNameExpr = factoryCall[0];
         if (!IsAtomic(factoryNameExpr))
         {
            return false;
         }

         const IStructure& targetStruct = *targetDef.ResolvedType;
         if (!IsNullType(targetStruct))
         {
            return false;
         }

         cstr targetName = directive[0].String()->Buffer;
		 cstr factoryName = factoryNameExpr.String()->Buffer;

		 if (!IsCapital(factoryName[0]))
		 {
			 return false;
		 }

         const IFactory& factory = GetFactoryInModule(factoryNameExpr, GetModule(ce.Script));
		 
         CompileFactoryCall(ce, factory, targetName, factoryCall, targetStruct.GetInterface(0));

         return true;
      }

      void CompileConstructFromFactory(CCompileEnvironment& ce, const IStructure& nullType, cstr interfaceRefName, cr_sex args)
      {
         // This function turns (<IInterface> id (<Factory> <arg1>...<argN>)) into assembly

         AddSymbol(ce.Builder, ("%s %s"), GetFriendlyName(nullType), interfaceRefName);

		 AddInterfaceVariable(ce, NameString::From(interfaceRefName), nullType);

         cr_sex factoryExpr = GetAtomicArg(args, 0);
         const IFactory& factory = GetFactoryInModule(factoryExpr, GetModule(ce.Script));

         CompileFactoryCall(ce, factory, interfaceRefName, args, nullType.GetInterface(0));
      }
   }//Script
}//Sexy