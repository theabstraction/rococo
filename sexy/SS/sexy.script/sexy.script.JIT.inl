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
      ID_API_CALLBACK JITCallbackId(IScriptSystem& ss);
      const SEXCHAR* const SKIPSTRING = SEXTEXT("#!skip");

      void SetPCToFunctionStart(IScriptSystem& ss, const IFunction& f)
      {
         CodeSection section;
         f.Code().GetCodeSection(OUT section);

         size_t programStart = f.Object().ProgramMemory().GetFunctionAddress(section.Id);

         VM::CPU& cpu = ss.ProgramObject().VirtualMachine().Cpu();
         cpu.SetPC(cpu.ProgramStart + programStart);
      }

      enum JIT_TYPE
      {
         JIT_TYPE_FACTORY = 0xFAC100EE,
         JIT_TYPE_FUNCTION = 0xF00BAAAA,
         JIT_TYPE_MACRO = 0x1000ACA0
      };

      void CompileFunction(IFunctionBuilder& f, CScript& script, cr_sex s, IScriptSystem& ss)
      {
         csexstr name = f.Name();
         f.Builder().DeleteSymbols();
         CompileFunctionFromExpression(f, IN s, script);
         SetPCToFunctionStart(ss, f);
      }

      void CompileFactory(IFactoryBuilder& factory, CScript& script, cr_sex s, IScriptSystem& ss)
      {
         csexstr name = factory.Name();

         int bodyIndex = GetIndexOf(1, s, SEXTEXT(":"));
         if (bodyIndex < 0) Throw(s, SEXTEXT("Could not find body indicator ':' in factory definition"));
         if (bodyIndex >= s.NumberOfElements()) Throw(s, SEXTEXT("Body indicator ':' was at the end of the expression. Expecting body to follow it"));

         factory.Constructor().Builder().DeleteSymbols();
         CompileFactoryBody(factory, s, bodyIndex + 1, script);
         SetPCToFunctionStart(ss, factory.Constructor());
      }

      void _cdecl OnJITRoutineNeedsCompiling_Protected(VariantValue* registers, IScriptSystem& ss)
      {
         void* pObj = registers[252].vPtrValue;
         ISExpression* s = (ISExpression*)registers[253].vPtrValue;
         CScript* script = (CScript*)registers[254].vPtrValue;
         int32 type = registers[255].int32Value;

         switch (type)
         {
         case JIT_TYPE_FUNCTION:
            CompileFunction(*(IFunctionBuilder*)pObj, *script, *s, ss);
            break;
         case JIT_TYPE_FACTORY:
            CompileFactory(*(IFactoryBuilder*)pObj, *script, *s, ss);
            break;
         case JIT_TYPE_MACRO:
         {
            IMacroBuilder& macro = *(IMacroBuilder*)pObj;
            macro.Implementation().Builder().DeleteSymbols();
            CompileMacroFromExpression(macro, *script, *s);
            SetPCToFunctionStart(ss, macro.Implementation());
         }
         break;
         default:
            ss.ProgramObject().Log().Write(SEXTEXT("OnJITRoutineNeedsCompiling called with bad type"));
            ss.ProgramObject().VirtualMachine().Throw();
            return;
         }
      }

      void _cdecl OnJITRoutineNeedsCompiling(VariantValue* registers, void* context)
      {
         IScriptSystem& ss = *(IScriptSystem*)context;
         ISExpression* s = (ISExpression*)registers[253].vPtrValue;

         try
         {
            OnJITRoutineNeedsCompiling_Protected(registers, ss);
         }
         catch (ParseException& ex)
         {
            ss.ProgramObject().Log().OnJITCompileException(ex);
            ss.ProgramObject().VirtualMachine().Throw();
         }
         catch (Sexy::IException& iex)
         {
            ParseException pex(Vec2i{ 0,0 }, Vec2i{ 0,0 }, s->Tree().Source().Name(), iex.Message(), SEXTEXT(""), s);
            ss.ProgramObject().Log().OnJITCompileException(pex);
            ss.ProgramObject().VirtualMachine().Throw();
         }
      }

      void CompileJITStubBytecode(ICodeBuilder& builder, void* ptr, cr_sex s, CScript& script, JIT_TYPE type, IScriptSystem& ss)
      {
         VM::IAssembler& assembler = builder.Assembler();

         builder.Begin();

         VariantValue Ptr, exprPtr, scrPtr;
         Ptr.vPtrValue = ptr;

         builder.AddSymbol(SKIPSTRING);
         assembler.Append_SetRegisterImmediate(252, Ptr, BITCOUNT_POINTER);

         exprPtr.vPtrValue = const_cast<ISExpression*>(&s);
         builder.AddSymbol(SKIPSTRING);
         assembler.Append_SetRegisterImmediate(253, exprPtr, BITCOUNT_POINTER);

         scrPtr.vPtrValue = &script;
         builder.AddSymbol(SKIPSTRING);
         assembler.Append_SetRegisterImmediate(254, scrPtr, BITCOUNT_POINTER);

         VariantValue typeVal;
         typeVal.int32Value = type;
         builder.AddSymbol(SKIPSTRING);
         assembler.Append_SetRegisterImmediate(255, typeVal, BITCOUNT_32);

         builder.AddSymbol(SKIPSTRING);
         assembler.Append_Invoke(JITCallbackId(ss));

         assembler.Append_Exit(VM::REGISTER_D4);

         builder.End();
         builder.Assembler().Clear();
      }

      void CompileJITStub(IFunctionBuilder& f, cr_sex fdef, CScript& script, IScriptSystem& ss)
      {
         csexstr name = f.Name();
         ICodeBuilder& builder = f.Builder();

         CompileJITStubBytecode(builder, &f, fdef, script, JIT_TYPE_FUNCTION, ss);
      }

      void CompileJITStub(IFactoryBuilder* f, cr_sex fdef, CScript& script, IScriptSystem& ss)
      {
         csexstr name = f->Name();
         ICodeBuilder& builder = f->Constructor().Builder();

         CompileJITStubBytecode(builder, (void*)f, fdef, script, JIT_TYPE_FACTORY, ss);
      }

      void CompileJITStub(IMacroBuilder* m, CScript& script, IScriptSystem& ss)
      {
         csexstr name = m->Name();

         ICodeBuilder& builder = m->Implementation().Builder();

         CompileJITStubBytecode(builder, (void*)m, *((ISExpression*)m->Expression()), script, JIT_TYPE_MACRO, ss);
      }
   }//Script
}//Sexy