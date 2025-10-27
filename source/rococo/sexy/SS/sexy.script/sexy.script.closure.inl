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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM 'AS IS' WITHOUT
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
      void PostClosure(cr_sex s, IFunctionBuilder& closure, CScript& script);

      void CompileClosureDef(CCompileEnvironment& ce, cr_sex closureDef, const IArchetype& closureArchetype, bool mayUseParentSF)
      {
         // (closure ...input... -> ...output... : body)

         int mapIndex = GetIndexOf(1, closureDef, ("->"));
         if (mapIndex < 0)
         {
            Throw(closureDef, ("Expecting mapping token '->' in the closure definition. Ensure adequate whitespace between tokens."));
         }

         int bodyIndex = GetIndexOf(mapIndex + 1, closureDef, (":"));
         if (bodyIndex < 0)
         {
            Throw(closureDef, ("Expecting body indicator token ':' after the mapping token in the closure definition"));
         }

         int nOutputs = bodyIndex - (mapIndex + 1);
         int nInputs = mapIndex - 1;

         if (nInputs < closureArchetype.NumberOfInputs())
         {
            Throw(closureDef, ("Too few input arguments"));
         }
         else if (nInputs > closureArchetype.NumberOfInputs())
         {
            Throw(closureDef, ("Too many input arguments"));
         }

         if (nOutputs < closureArchetype.NumberOfOutputs())
         {
            Throw(closureDef, ("Too few output arguments"));
         }
         else if (nOutputs > closureArchetype.NumberOfOutputs())
         {
            Throw(closureDef, ("Too many output arguments"));
         }

         IFunctionBuilder& closure = ce.Builder.Module().DeclareClosure(ce.Builder.Owner(), mayUseParentSF, &closureDef);

         for (int i = mapIndex + 1; i < bodyIndex; ++i)
         {
            cr_sex outputExpr = closureDef.GetElement(i);
            AssertCompound(outputExpr);
            AssertNotTooFewElements(outputExpr, 2);
            AssertNotTooManyElements(outputExpr, 2);

            cr_sex outputType = GetAtomicArg(outputExpr, 0);
            cr_sex outputName = GetAtomicArg(outputExpr, 1);

            int outputIndex = i - mapIndex - 1;
            cstr name = outputName.c_str();
            cstr type = outputType.c_str();

            const IStructure& neededType = closureArchetype.GetArgument(outputIndex);
            if (!AreEqual(neededType.Name(), type))
            {
               Throw(outputType, "The output type did not match that of the archetype: %s", neededType.Name());
            }

            closure.AddOutput(NameString::From(name), TypeString::From(type), (void*)&outputExpr);
         }

         for (int i = 1; i < mapIndex; ++i)
         {
            cr_sex inputExpr = closureDef.GetElement(i);
            AssertCompound(inputExpr);
            AssertNotTooFewElements(inputExpr, 2);
            AssertNotTooManyElements(inputExpr, 2);

            cr_sex inputType = GetAtomicArg(inputExpr, 0);
            cr_sex imputName = GetAtomicArg(inputExpr, 1);

            int inputIndex = i - 1 + nOutputs;
            cstr name = imputName.c_str();
            cstr type = inputType.c_str();

            const IStructure& neededType = closureArchetype.GetArgument(inputIndex);
            if (!AreEqual(type, GetFriendlyName(neededType)))
            {
               Throw(inputType, "The input type did not match that of the archetype: %s", GetFriendlyName(neededType));
            }

            closure.AddInput(NameString::From(name), TypeString::From(type), (void*)&inputExpr);
         }

         if (!closure.TryResolveArguments())
         {
			 try
			 {
				 closure.ValidateArguments();
			 }
			 catch (IException& ex)
			 {
				 Throw(closureDef, "%s", ex.Message());
			 }

             Throw(closureDef, ("Could not resolve all of the function arguments"));
         }

         CodeSection section;
         closure.Builder().GetCodeSection(OUT section);

         VariantValue closureBytecodeId;
		 closureBytecodeId.int64Value = (int64) section.Id;
         ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, closureBytecodeId, BITCOUNT_64);

         PostClosure(closureDef, closure, ce.Script);
      }

      // Lambdas have form (: <input-names> => (body-directives) -> output)
      void CompileLambdaExpression(cr_sex lambdaDef, IFunctionBuilder& lambdaFunction, CScript& script)
      {
          // (: <input-names> => (body-expression-sequence) -> output)

          ICodeBuilder& builder = lambdaFunction.Builder();
          builder.Begin();

          CCompileEnvironment ce(script, builder);

          int bodyDirectiveIndex = lambdaFunction.NumberOfInputs() + 1;

          CompileExpressionSequence(ce, bodyDirectiveIndex+1, bodyDirectiveIndex+1, lambdaDef);

          try
          {
              builder.End();
          }
          catch (IException& e)
          {
              Throw(lambdaDef, e.Message());
          }

          builder.Assembler().Clear();
      }

      void CompileClosureBody(cr_sex closureDef, IFunctionBuilder& closure, CScript& script)
      {
         int bodyIndex = GetIndexOf(0, closureDef, ":");

         if (bodyIndex == 0)
         {
             // Lambda
             CompileLambdaExpression(closureDef, closure, script);
             return;
         }

         ICodeBuilder& builder = closure.Builder();
         builder.Begin();

         CCompileEnvironment ce(script, builder);

         CompileExpressionSequence(ce, bodyIndex + 1, closureDef.NumberOfElements() - 1, closureDef);

         try
         {
            builder.End();
         }
         catch (IException& e)
         {
            Throw(closureDef, e.Message());
         }

         builder.Assembler().Clear();
      }

      bool TryCompileClosureDef(CCompileEnvironment& ce, cr_sex s, const IArchetype& closureArchetype, bool mayUseParentSF)
      {
         AssertCompound(s);
         if (s.NumberOfElements() >= 1)
         {
            cr_sex head = s.GetElement(0);
            if (head.Type() == EXPRESSION_TYPE_ATOMIC)
            {
               if (AreEqual(head.String(), "closure"))
               {
                  CompileClosureDef(ce, s, closureArchetype, mayUseParentSF);
                  return true;
               }
            }
         }

         return false;
      }
   }//Script
}//Sexy