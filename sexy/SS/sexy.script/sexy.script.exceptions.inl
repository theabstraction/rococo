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
      void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t startOfBody, size_t gotoCleanupPos, size_t handlerPos);

      void CompileTryCatchFinally(CCompileEnvironment& ce, cstr exName, cr_sex body, cr_sex handler, const ISExpression* cleanup)
      {
         ce.Builder.AddSymbol(("try"));
         ce.Builder.Assembler().Append_NoOperation();

         size_t startOfBody = ce.Builder.Assembler().WritePosition();
         ce.Script.PushTryCatchBlock(body);

         CompileExpressionSequence(ce, 0, body.NumberOfElements() - 1, body);
         ce.Script.PopTryCatchBlock();

         size_t gotoCleanupPos = ce.Builder.Assembler().WritePosition();

         CodeSection section;
         ce.Builder.GetCodeSection(OUT section);

         ce.Builder.Assembler().Append_Branch(0);

         ce.Builder.AddSymbol(("catch"));
         ce.Builder.Assembler().Append_NoOperation();

         size_t handlerPos = ce.Builder.Assembler().WritePosition();
         AddCatchHandler(ce.Script, section.Id, startOfBody, gotoCleanupPos, handlerPos);

		 AddInterfaceVariable(ce, NameString::From(exName), ce.Object.Common().SysTypeIException().NullObjectType());

         CompileExpressionSequence(ce, 0, handler.NumberOfElements() - 1, handler);

         ce.Builder.AddSymbol(("end-catch"));

         ce.Builder.PopLastVariables(1,true); // That pops the reference to the exception

         size_t cleanupPos = ce.Builder.Assembler().WritePosition();
         size_t buildToCleanupDelta = cleanupPos - gotoCleanupPos;
         if (cleanup != NULL)
         {
            ce.Builder.AddSymbol(("finally"));
            CompileExpressionSequence(ce, 0, cleanup->NumberOfElements() - 1, *cleanup);
         }

         ce.Builder.Assembler().SetWriteModeToOverwrite(gotoCleanupPos);
         ce.Builder.Assembler().Append_Branch((int)buildToCleanupDelta);
         ce.Builder.Assembler().SetWriteModeToAppend();
      }

      void CompileExceptionBlock(CCompileEnvironment& ce, cr_sex s)
      {
         // (try (body) catch exception-name (handler) finally (cleanup))
         if (s.NumberOfElements() != 5 && s.NumberOfElements() != 7)
         {
            sexstringstream<1024> streamer;
            streamer.sb << ("Expecting 5 or 7 elements in a (try (...) catch e (...) finally (...)) block. Found ") << s.NumberOfElements() << (" elements");
            Throw(s, *streamer.sb);
         }

         GetAtomicArg(s, 0);
         AssertKeyword(s, 0, ("try"));
         AssertKeyword(s, 2, ("catch"));

         cr_sex body = s.GetElement(1);
         AssertCompoundOrNull(body);

         cr_sex exNameExpr = GetAtomicArg(s, 3);
         AssertLocalIdentifier(exNameExpr);

         cr_sex handler = s.GetElement(4);
         AssertCompoundOrNull(handler);

         if (s.NumberOfElements() == 7) AssertKeyword(s, 5, ("finally"));

         const ISExpression* cleanup = NULL;

         if (s.NumberOfElements() == 7)
         {
            cleanup = &s.GetElement(6);
            AssertCompoundOrNull(*cleanup);
         }

         CompileTryCatchFinally(ce, exNameExpr.String()->Buffer, body, handler, cleanup);
      }

      bool HasInterface(const IInterface& interface, const IStructure& classspec)
      {
         for (int i = 0; i < classspec.InterfaceCount(); ++i)
         {
            if (&classspec.GetInterface(i) == &interface) return true;
         }

         return false;
      }

      void CompileThrow(CCompileEnvironment& ce, cr_sex s)
      {
         cr_sex ex = GetAtomicArg(s, 1);
         AssertLocalIdentifier(ex);

		 cstr exName = ex.String()->Buffer;

         MemberDef def;
         if (!ce.Builder.TryGetVariableByName(OUT def, exName))
         {
            Throw(ex, ("Expecting local exception identifier"));
         }

         const IInterface& interfExc = ce.Object.Common().SysTypeIException();
         if (!HasInterface(interfExc, *def.ResolvedType))
         {
            Throw(ex, ("The variable does not implement the Sys.Type.IException interface"));
         }

         IFunction* fnThrow = ce.Object.Common().SysType().FindFunction(("_throw"));
         if (fnThrow == NULL)
         {
            Throw(s, ("Cannot find intrinsic function Sys.Type.Exception._throw(Sys.Type.IException ex)"));
         }

         AddSymbol(ce.Builder, "%s", exName);

         ce.Builder.AssignVariableToTemp(exName, 0 /* D4 */); // Push a ref to the exception on the stack

         AddArgVariable(("exception"), ce, ce.Object.Common().TypePointer());
         ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);

         ce.Builder.AddSymbol(("throw"));
         AppendFunctionCallAssembly(ce, *fnThrow);

         MarkStackRollback(ce, s);

         ce.Builder.PopLastVariables(1,true); // Correct the stack - N.B, this allows continue strategy
      }
   }//Script
}//Sexy