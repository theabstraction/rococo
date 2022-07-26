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

namespace Rococo
{
   namespace Script
   {
      IStructureBuilder* MatchStructureDirect(cr_sex typeExpr, IModuleBuilder& module)
      {
         if (!IsAtomic(typeExpr))
            return NULL;
         cstr type = typeExpr.String()->Buffer;

         return module.FindStructure(type);
      }

      IStructure* MatchMemberViaStructure(cr_sex originExpression, cstr root, IStructureBuilder& rootStructure)
      {
         NamespaceSplitter splitter(root);

         cstr head, body;
         if (!splitter.SplitHead(OUT head, OUT body))
         {
            return FindMember(rootStructure, root);
         }

         IStructureBuilder* s = FindMember(rootStructure, head);
         if (s == NULL) return NULL;

         return MatchMemberViaStructure(originExpression, body, *s);
      }

      IStructure* MatchMemberViaStructure(cr_sex originExpression, cstr root, INamespaceBuilder& ns)
      {
         NamespaceSplitter splitter(root);

         cstr head, body;
         if (!splitter.SplitHead(OUT head, OUT body))
         {
            return ns.FindStructure(root);
         }

         IStructureBuilder* s = ns.FindStructure(head);
         if (s == NULL) return NULL;

         return MatchMemberViaStructure(originExpression, root, *s);
      }

      IStructure* MatchStructureViaAlias(cr_sex originExpression, cstr root, INamespaceBuilder& ns)
      {
         NamespaceSplitter splitter(root);

         cstr head, body;
         if (!splitter.SplitHead(OUT head, OUT body))
         {
            return ns.FindStructure(root);
         }

         if (IsCapital(head[0]))
         {
            // Starts with capital letter, so it may be a sub-namespace
            INamespaceBuilder* subNS = ns.FindSubspace(head);
            if (subNS == NULL)
            {
               return NULL;
            }
            else
            {
               return MatchStructureViaAlias(originExpression, body, *subNS);
            }
         }
         else
         {
            // Starts with lower cast letter, so may be a structure variable
            return MatchMemberViaStructure(originExpression, body, ns);
         }
      }

      IFunctionBuilder& MustMatchFunction(IModuleBuilder& module, cr_sex s, cstr name)
      {
         if (!IsCapital(name[0]))
         {
            Throw(s, ("Functions begin with a capital letter"));
         }

         NamespaceSplitter splitter(name);

         cstr body, tail;
         if (splitter.SplitTail(OUT body, OUT tail))
         {
            INamespaceBuilder* ns = Compiler::MatchNamespace(module, body);
            if (ns == NULL)
            {
               Throw(s, ("Could not find namespace prefixing function"));
            }

            IFunctionBuilder* f = ns->FindFunction(tail);
            if (f == NULL)
            {
               ThrowTokenNotFound(s, name, ns->FullName()->Buffer, ("namespace"));
            }

            return *f;
         }
         else
         {
            IFunctionBuilder* f = module.FindFunction(name);
            if (f != NULL) return *f;

            IFunctionBuilder* finalFunction = NULL;
            INamespaceBuilder* finalNS = NULL;
            for (int i = 0; i < module.PrefixCount(); ++i)
            {
               INamespaceBuilder& prefix = module.GetPrefix(i);
               f = prefix.FindFunction(name);

               if (f != NULL)
               {
                  if (finalFunction != NULL)
                     ThrowNamespaceConflict(s, prefix, *finalNS, ("function"), name);
                  else
                  {
                     finalFunction = f;
                     finalNS = &prefix;
                  }
               }
            }

            if (finalFunction == NULL)
            {
               ThrowTokenNotFound(s, name, module.Name(), ("function"));
            }

            return *finalFunction;
         }
      }

      IStructureBuilder* MatchStructure(cr_sex typeExpr, IModuleBuilder& module)
      {
         if (!IsAtomic(typeExpr))
            return NULL;

         cstr type = typeExpr.String()->Buffer;

         if (!IsCapital(type[0]))
         {
            if (AreEqual(type, ("array")))
            {
               // Special intrinsic
               return module.Object().GetModule(0).FindStructure(("_Array"));
            }
            return NULL;
         }

         NamespaceSplitter splitter(type);

         cstr body, tail;
         if (splitter.SplitTail(OUT body, OUT tail))
         {
            INamespaceBuilder* ns = Compiler::MatchNamespace(module, body);
            if (ns == NULL) ThrowTokenNotFound(typeExpr, body, ("program"), ("namespace"));

            IStructureBuilder* s = ns->FindStructure(tail);
            if (s != NULL)
            {
               return s;
            }
         }
         else
         {
            IStructureBuilder* s = module.Object().IntrinsicModule().FindStructure(type);
            if (s == NULL)
            {
               for (int i = 0; i < module.PrefixCount(); ++i)
               {
                  INamespaceBuilder& prefix = module.GetPrefix(i);
                  s = prefix.FindStructure(type);
                  if (s != NULL)
                  {
                     return s;
                  }

                  IInterfaceBuilder* interf = prefix.FindInterface(type);
                  if (interf != NULL)
                  {
                     return &interf->NullObjectType();
                  }
               }

               s = MatchStructureDirect(typeExpr, module);
               return s;
            }
            else
            {
               return s;
            }
         }

         return NULL;
      }

      IFunctionBuilder* MatchFunction(cr_sex nameExpr, IModuleBuilder& module)
      {
         if (!IsAtomic(nameExpr))
            return NULL;

         cstr name = nameExpr.String()->Buffer;

         if (!IsCapital(name[0]))
         {
            return NULL;
         }

         NamespaceSplitter splitter(name);

         cstr body, tail;
         if (splitter.SplitTail(OUT body, OUT tail))
         {
            INamespaceBuilder* ns = Compiler::MatchNamespace(module, body);
            if (ns == NULL)
            {
               Throw(nameExpr, ("Could not find namespace prefixing function"));
            }

            return ns->FindFunction(tail);
         }
         else
         {
            IFunctionBuilder* f = module.FindFunction(name);
            if (f != NULL) return f;

            IFunctionBuilder* finalFunction = NULL;
            INamespaceBuilder* finalNS = NULL;
            for (int i = 0; i < module.PrefixCount(); ++i)
            {
               INamespaceBuilder& prefix = module.GetPrefix(i);
               f = prefix.FindFunction(name);

               if (f != NULL)
               {
                  if (finalFunction != NULL)
                     ThrowNamespaceConflict(nameExpr, prefix, *finalNS, ("function"), name);
                  else
                  {
                     finalFunction = f;
                     finalNS = &prefix;
                  }
               }
            }

            return finalFunction;
         }

         return NULL;
      }

      IInterfaceBuilder* GetInterfaceFQN(cr_sex baseExpr, CScript& script)
      {
         AssertAtomic(baseExpr);

         cstr fqn = baseExpr.String()->Buffer;

         NamespaceSplitter splitter(fqn);

         cstr nsName, shortName;
         if (!splitter.SplitTail(OUT nsName, OUT shortName))
         {
            Throw(baseExpr, "Expecting fully qualified namespace name");
         }

         INamespaceBuilder* ns;

         if (Eq(nsName, "$"))
         {
             ns = static_cast<INamespaceBuilder*>(const_cast<INamespace*>(script.ProgramModule().DefaultNamespace()));
         }
         else
         {
             ns = GetProgramObject(script).GetRootNamespace().FindSubspace(nsName);
         }

         if (ns == NULL) ThrowTokenNotFound(baseExpr, nsName, "program", "namespace");

         return ns->FindInterface(shortName);
      }

      IInterfaceBuilder& GetInterfaceInModule(cr_sex interfExpr, IModuleBuilder& module)
      {
         NamespaceSplitter splitter(interfExpr.String()->Buffer);

         INamespaceBuilder* NS = NULL;

         cstr ns, shortName;
         if (splitter.SplitTail(OUT ns, OUT shortName))
         {
            NS = module.Object().GetRootNamespace().FindSubspace(ns);
            if (NS == NULL) ThrowTokenNotFound(interfExpr, ns, ("program"), ("namespace"));

            IInterfaceBuilder* interf = NS->FindInterface(shortName);
            if (interf == NULL)  ThrowTokenNotFound(interfExpr, shortName, ns, ("interface"));

            return *interf;
         }
         else
         {
            IInterfaceBuilder* interf = NULL;

            for (int i = 0; i < module.PrefixCount(); i++)
            {
               INamespaceBuilder& prefix = module.GetPrefix(i);
               interf = prefix.FindInterface(shortName);
               if (interf != NULL)
               {
                  if (NS != NULL) ThrowNamespaceConflict(interfExpr, *NS, prefix, ("interface"), shortName);
                  NS = &prefix;
               }
            }

            if (NS == NULL) ThrowTokenNotFound(interfExpr, shortName, module.Name(), ("interface"));
            return *interf;
         }
      }

      IInterfaceBuilder* MatchInterface(cr_sex typeExpr, IModuleBuilder& module)
      {
         if (!IsAtomic(typeExpr))
            return NULL;

         cstr type = typeExpr.String()->Buffer;

         if (!IsCapital(type[0]) && *type != '$')
         {
            return NULL;
         }

         NamespaceSplitter splitter(type);

         cstr body, tail;
         if (splitter.SplitTail(OUT body, OUT tail))
         {
            INamespaceBuilder* ns = Compiler::MatchNamespace(module, body);
            if (ns == nullptr)
            {
                if (Eq(body, "$"))
                {
                    ns = static_cast<INamespaceBuilder*>(const_cast<INamespace*>(module.DefaultNamespace()));
                }
            }

            if (ns == NULL) ThrowTokenNotFound(typeExpr, body, ("program"), ("namespace"));

            IInterfaceBuilder* interf = ns->FindInterface(tail);
            if (interf != NULL)
            {
               return interf;
            }

            return NULL;
         }
         else
         {
            INamespaceBuilder* sysType = module.Object().GetRootNamespace().FindSubspace(("Sys.Type"));
            IInterfaceBuilder* interf = sysType->FindInterface(type);
            INamespaceBuilder* iterfNS = sysType;

            for (int i = 0; i < module.PrefixCount(); ++i)
            {
               INamespaceBuilder& prefix = module.GetPrefix(i);
               IInterfaceBuilder* ithInterf = prefix.FindInterface(type);
               if (ithInterf != NULL)
               {
                  if (interf != NULL && interf != ithInterf)
                  {
                     Rococo::Sex::ThrowNamespaceConflict(typeExpr, *iterfNS, prefix, ("interface"), type);
                  }

                  interf = ithInterf;
                  iterfNS = &prefix;
               }
            }

            return interf;
         }
      }

      IArchetype* MatchArchetype(cr_sex typeExpr, IModuleBuilder& module)
      {
         if (!IsAtomic(typeExpr))
            return NULL;

         cstr type = typeExpr.String()->Buffer;

         if (!IsCapital(type[0]))
         {
            return NULL;
         }

         NamespaceSplitter splitter(type);

         cstr body, tail;
         if (splitter.SplitTail(OUT body, OUT tail))
         {
            INamespaceBuilder* ns = Compiler::MatchNamespace(module, body);
            if (ns == NULL)
            {
               ThrowTokenNotFound(typeExpr, body, ("program"), ("namespace"));
            }

            IArchetype* a = ns->FindArchetype(tail);
            if (a != NULL)
            {
               return NULL;
            }
         }
         else
         {
            for (int i = 0; i < module.PrefixCount(); ++i)
            {
               INamespaceBuilder& prefix = module.GetPrefix(i);
               IArchetype* a = prefix.FindArchetype(type);
               if (a != NULL)
               {
                  return a;
               }
            }
         }

         return NULL;
      }

      IStructure* GetStructByQualifiedPath(ICodeBuilder& builder, cstr structName)
      {
         NamespaceSplitter splitter(structName);
         cstr nspath, publicName;

         if (splitter.SplitTail(OUT nspath, OUT publicName))
         {
            INamespaceBuilder* ns = builder.Module().Object().GetRootNamespace().FindSubspace(nspath);
            if (ns != NULL)
            {
               IStructure* st = ns->FindStructure(publicName);
               return st;
            }
         }

         return NULL;
      }

      IInterface& GetRequiredInterface(cr_sex s, ICodeBuilder& builder, cstr fullyQualifiedName)
      {
         IInterfaceBuilder* i = GetInterface(builder.Module().Object(), fullyQualifiedName);
         if (i == NULL)
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Cannot find ") << fullyQualifiedName;
            Throw(s, streamer);
         }

         return *i;
      }

      void GetThisRefDef(OUT MemberDef& def, ICodeBuilder& builder, cr_sex s)
      {
         if (!builder.TryGetVariableByName(OUT def, ("this")))
         {
            Throw(s, ("Expecting 'this' to be defined in the context"));
         }

         if (def.Usage == ARGUMENTUSAGE_BYVALUE)
         {
            Throw(s, ("Expecting 'this' to be by reference"));
         }
      }

      const IStructure& GetClass(cr_sex classExpr, CScript& script)
      {
         const IStructure* classType = MatchStructure(classExpr, GetModule(script));
         if (classType == NULL)	Throw(classExpr, ("Expecting a type"));
         if (!classType->Prototype().IsClass)	Throw(classExpr, ("Expecting a class"));
         return *classType;
      }

      const IStructure& GetThisInterfaceRefDef(OUT MemberDef& def, ICodeBuilder& builder, cr_sex s)
      {
         GetThisRefDef(OUT def, builder, s);
         const IStructure& interfaceType = *def.ResolvedType;
         if (!interfaceType.Prototype().IsClass)	Throw(s, ("The variable is not of interface type"));
         if (!AreEqual(interfaceType.Name(), ("_Null"), 4)) Throw(s, ("The variable is not of interface type"));
         if (interfaceType.InterfaceCount() != 1) Throw(s, ("Internal algorithmic error.Expecting  one and only one interface with the null class"));
         return interfaceType;
      }

      int GetInterfaceOffset(int index)
      {
         return sizeof(size_t) * (index + 1) + sizeof(size_t);
      }

      void GetVariableByName(ICodeBuilder& builder, OUT MemberDef& def, cstr name, cr_sex s)
      {
         if (!builder.TryGetVariableByName(def, name))
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Error, cannot find entry ") << name;
            Throw(s, streamer);
         }
      }
   }//Script
}//Sexy