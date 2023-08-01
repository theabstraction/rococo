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
      enum { MAX_TRANSFORM_DEPTH = 16 };

      void* GetInterface(const ObjectStub& header)
      {
         return (void*)&header.pVTables[0];
      }

      IMacroBuilder* DeclareMacro(cr_sex macroDef, IModuleBuilder& module)
      {
         AssertNotTooFewElements(macroDef, 5);

         cr_sex macroNameExpr = GetAtomicArg(macroDef, 1);
         cr_sex inNameExpr = GetAtomicArg(macroDef, 2);
         cr_sex outNameExpr = GetAtomicArg(macroDef, 3);

         AssertLocalIdentifier(inNameExpr);
         AssertLocalIdentifier(outNameExpr);

         NamespaceSplitter splitter(macroNameExpr.c_str());

         cstr nsRoot, shortName;
         AssertSplitTail(splitter, macroNameExpr, OUT nsRoot, OUT shortName);
         AssertMacroShortName(macroNameExpr, shortName);

         cstr staticShortName = macroNameExpr.c_str() + StringLength(nsRoot) + 1;

         INamespaceBuilder& ns = AssertGetSubspace(module.Object(), macroNameExpr, nsRoot);

         TokenBuffer fname;
         StringPrint(fname, ("#%s"), shortName);
         if (module.FindFunction(fname) != NULL)
         {
            ThrowTokenAlreadyDefined(macroNameExpr, shortName, module.Name(), ("macro"));
         }

         IFunctionBuilder &f = module.DeclareFunction(FunctionPrototype(fname, false), &macroDef);
         f.AddInput(NameString::From(inNameExpr.String()), TypeString::From(("Sys.Reflection.IExpression")), (void*)&inNameExpr);
         f.AddInput(NameString::From(outNameExpr.String()), TypeString::From(("Sys.Reflection.IExpressionBuilder")), (void*)&outNameExpr);

         if (!f.TryResolveArguments())
         {
            Throw(macroDef, "Error resolving arguments in macro: %s", fname);
         }

         IMacroBuilder* macro = ns.AddMacro(staticShortName, (void*)&macroDef, f);
         if (macro == NULL) ThrowTokenAlreadyDefined(macroNameExpr, shortName, ns.FullName()->Buffer, ("macro"));

         return macro;
      }

	  void Disassemble(VM::IDisassembler& disassembler, const IFunction& f, IPublicScriptSystem& ss);

      void CompileMacroFromExpression(IMacroBuilder& macro, CScript& script, cr_sex macroDef)
      {
         CCompileEnvironment ce(script, macro.Implementation().Builder());

         // (macro <macro-name> <in-name> <out-name> (xpr1) .... (xprN))
         IFunction& f = ce.Builder.Owner();

         cstr macroName = f.Name();

         ce.Builder.Begin();

         CompileExpressionSequence(ce, 4, macroDef.NumberOfElements() - 1, macroDef);

         ce.Builder.End();
         ce.Builder.Assembler().Clear();

#ifdef _DEBUG
		 if (Rococo::OS::IsDebugging())
		 {
			 AutoFree<VM::IDisassembler> disassembler ( ce.Object.VirtualMachine().Core().CreateDisassembler() );
			 Disassemble(*disassembler, f, ce.SS);
		 }
#endif
      }

      void CallMacro(IScriptSystem& ss, const IFunction& f, cr_sex s)
      {
         VM::IVirtualMachine& vm = ss.ProgramObject().VirtualMachine();
         const CClassExpression* input = ss.GetExpressionReflection(s);

		 ISExpressionBuilder* outputRoot = ss.CreateMacroTransform(s);

         CClassExpressionBuilder output;
         if (!ss.ConstructExpressionBuilder(output, outputRoot))
         {
            Throw(s, "%s: ConstructExpressionBuilder error", __FUNCTION__);
         }

         VM::CPU& cpu = vm.Cpu();
         if (cpu.SF() == nullptr)
         {
             // In the event that our macro is a top level macro, created before the program is initialized we need a functional virtual CPU to get the job done
             ss.ProgramObject().SetProgramAndEntryPoint(0);
             vm.InitCpu();
         }

         vm.Push(GetInterface(input->Header));
         vm.Push(GetInterface(output.Header));

         CodeSection macroSection;
         f.Code().GetCodeSection(OUT macroSection);

         EXECUTERESULT result = vm.ExecuteFunctionProtected(macroSection.Id);
         if (result != EXECUTERESULT_RETURNED)
         {
            Throw(s, "Error executing macro expansion");
         }

         vm.SetStatus(EXECUTERESULT_RUNNING);

         vm.PopPointer();
         vm.PopPointer();
      }
   }//Script
}//Sexy