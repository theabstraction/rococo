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
        void CompileDoWhile(CCompileEnvironment& ce, cr_sex s)
        {
            // (do (action1) ... (actionN) while (binary-predicate) )

            AssertCompound(s);
            AssertNotTooFewElements(s, 3);

            cr_sex whileExpr = s.GetElement(s.NumberOfElements() - 2);
            AssertAtomic(whileExpr);

            if (!AreEqual(whileExpr.String(), ("while")))
            {
                Throw(whileExpr, ("Expecting 'while' in penultimate position of the do...while expression"));
            }

            struct ConditionSection : public ICompileSection
            {
                ConditionSection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData)
                {
                    cr_sex condition = s.GetElement(s.NumberOfElements() - 1);

                    bool negate = false;
                    if (!TryCompileBooleanExpression(ce, condition, true, negate))
                    {
                        Throw(condition, "Expecting boolean valued expression in the last position in the do...while statement");
                    }

                    if (negate) builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
                    builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);
                }
            } loopCriterion(s, ce);

            struct BodySection : public ICompileSection
            {
                BodySection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData)
                {
                    builder.PushControlFlowPoint(*controlFlowData);
                    CompileExpressionSequence(ce, 1, s.NumberOfElements() - 3, s);
                    builder.PopControlFlowPoint();
                }
            } bodySection(s, ce);

            ce.Builder.AppendDoWhile(bodySection, loopCriterion, CONDITION_IF_NOT_EQUAL);
        }

        void CompileWhileLoop(CCompileEnvironment& ce, cr_sex s)
        {
            // (while (binary-predicate) (action1) ... (actionN))

            AssertCompound(s);
            AssertNotTooFewElements(s, 2);

            struct ConditionSection : public ICompileSection
            {
                ConditionSection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData)
                {
                    cr_sex condition = s.GetElement(1);

                    bool negate = false;
                    if (!TryCompileBooleanExpression(ce, condition, true, negate))
                    {
                        Throw(condition, "Expecting boolean valued expression as the condition in the (while ...) statement");
                    }

                    if (negate) builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
                    builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);
                }
            } loopCriterion(s, ce);

            struct BodySection : public ICompileSection
            {
                BodySection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData)
                {
                    builder.PushControlFlowPoint(*controlFlowData);
                    CompileExpressionSequence(ce, 2, s.NumberOfElements() - 1, s);
                    builder.PopControlFlowPoint();
                }
            } bodySection(s, ce);

            ce.Builder.AppendWhileDo(loopCriterion, CONDITION_IF_NOT_EQUAL, bodySection);
        }

        void CompileBreak(CCompileEnvironment& ce, cr_sex s)
        {
            AssertNotTooManyElements(s, 1);

            ControlFlowData cfd;
            if (!ce.Builder.TryGetControlFlowPoint(OUT cfd))
            {
                Throw(s, "'break' is only valid inside a loop construct");
            }

            AppendDeconstructTailVariables(ce, s, false, ce.Builder.SectionArgCount(cfd.SectionIndex));
            int32 toBreak = ((int32)cfd.BreakPosition) - ((int32)ce.Builder.Assembler().WritePosition());
            ce.Builder.Assembler().Append_Branch(toBreak);
        }

        void CompileContinue(CCompileEnvironment& ce, cr_sex s)
        {
            AssertNotTooManyElements(s, 1);

            ControlFlowData cfd;
            if (!ce.Builder.TryGetControlFlowPoint(OUT cfd))
            {
                Throw(s, ("'continue' is only valid inside a loop construct"));
            }

            int32 toContinue = ((int32)cfd.ContinuePosition) - ((int32)ce.Builder.Assembler().WritePosition());
            ce.Builder.Assembler().Append_Branch(toContinue);
        }

        void CompileFinally(CCompileEnvironment& ce, cr_sex s)
        {
            ControlFlowData cfd;
            if (!ce.Builder.TryGetControlFlowPoint(OUT cfd))
            {
                Throw(s, ("'finally' is only valid inside a loop construct"));
            }

            int32 toFinally = ((int32)cfd.FinallyPosition) - ((int32)ce.Builder.Assembler().WritePosition());
            ce.Builder.Assembler().Append_Branch(toFinally);
        }

        void CompileIfThenElse(CCompileEnvironment& ce, cr_sex s)
        {
            // (   if binary-predicate (action1) ... (actionN) else (alternative1) ... (alternativeN)   )

            AssertCompound(s);
            AssertNotTooFewElements(s, 2);

            cr_sex condition = s.GetElement(1);

            bool negate = false;
            if (!TryCompileBooleanExpression(ce, condition, true, negate))
            {
                Throw(s, ("Expecting boolean expression"));
            }

            if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);

            ce.Builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);

            cr_sex keywordExpr = GetAtomicArg(s, 0);
            if (!AreEqual(keywordExpr.String(), ("if")))
            {
                Throw(s, ("Expecting If as first element in expression"));
            }

            int elsePos = -1;

            for (int i = 2; i < s.NumberOfElements(); ++i)
            {
                cr_sex arg = s.GetElement(i);
                if (IsAtomic(arg))
                {
                    sexstring token = arg.String();
                    if (AreEqual(token, ("else")))
                    {
                        if (elsePos == -1)
                        {
                            elsePos = i;
                        }
                        else
                        {
                            Throw(s, ("Duplicate 'else' keyword found in if...else expression"));
                        }
                    }
                    else
                    {
                        Throw(arg, ("Only 'else' is a legal atomic keyword in an (if ...) expression"));
                    }
                }
                else if (!IsCompound(arg))
                {
                    Throw(arg, ("Expecting atomic or compound expression in if...else expression"));
                }
            }

            struct Section : public ICompileSection
            {
                Section(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                int startPos;
                int endPos;
                CCompileEnvironment& ce;

                virtual void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData /* NULL, as If...Else does not support break and continue */)
                {
                    CompileExpressionSequence(ce, startPos, endPos, s);
                }
            } thenSection(s, ce), elseSection(s, ce);

            thenSection.startPos = 2;

            if (elsePos > thenSection.startPos)
            {
                thenSection.endPos = elsePos - 1;
                elseSection.startPos = elsePos + 1;
                elseSection.endPos = s.NumberOfElements() - 1;
            }
            else
            {
                thenSection.endPos = s.NumberOfElements() - 1;
                elseSection.startPos = -1;
                elseSection.endPos = -2;
            }

            ce.Builder.AppendConditional(CONDITION_IF_NOT_EQUAL, thenSection, elseSection);
        }

        void CompileForEachCore(CCompileEnvironment& ce, cr_sex s)
        {
            // (foreach <enumvar1> ... <enumvarN> # <collection> (action1) ....(actionN) )
            AssertNotTooFewElements(s, 4);

            int hashIndex = -1;

            for (int i = 1; i < s.NumberOfElements(); ++i)
            {
                cr_sex arg = GetAtomicArg(s, i);
                if (AreEqual(arg.String(), ("#")))
                {
                    hashIndex = i;
                    break;
                }
            }

            if (hashIndex < 2)
            {
                Throw(s, "(foreach <optional-enum-index-identifier> <element-ref-identifier> # <collection> (action1) ....(actionN) )");
            }

            cr_sex collection = s.GetElement(hashIndex + 1);

            if (IsCompound(collection))
            {
                // => array sub enumeration
                switch (collection.NumberOfElements())
                {
                case 2:
                    CompileLockRef(ce, s, hashIndex);
                    return;
                case 3:
                    CompileEnumerateArray(ce, s, hashIndex);
                    return;
                default:
                    Throw(collection, ("Expecting (<array-name> <lower-index> <upper-index>"));
                }
            }
            else if (IsAtomic(collection))
            {
                cstr srcName = collection.c_str();

                MemberDef def;
                if (ce.Builder.TryGetVariableByName(OUT def, srcName))
                {
                    if (*def.ResolvedType == ce.StructArray())
                    {
                        CompileEnumerateArray(ce, s, hashIndex);
                    }
                    else if (*def.ResolvedType == ce.StructList())
                    {
                        CompileEnumerateList(ce, s, hashIndex);
                    }
                    else if (*def.ResolvedType == ce.StructMap())
                    {
                        CompileEnumerateMap(ce, s, hashIndex);
                    }
                    else
                    {
                        Throw(collection, "Do not know how to enumerate type %s", GetFriendlyName(*def.ResolvedType));
                    }
                }
                else
                {
                    NamespaceSplitter splitter(srcName);
                    cstr head, body;
                    if (splitter.SplitHead(head, body))
                    {
                        if (ce.Builder.TryGetVariableByName(OUT def, head))
                        {
                            if (*def.ResolvedType == ce.StructArray())
                            {
                                CompileEnumerateArray(ce, s, hashIndex);
                                return;
                            }
                            else
                            {
                                Throw(collection, "Expecting array name: %s", head);
                            }
                        }
                        else
                        {
                            Throw(collection, "Expecting array name: %s", head);
                        }
                    }
                    else
                    {
                        Throw(collection, "Expecting collection variable name");
                    }
                }
            }
            else
            {
                Throw(collection, "Expecting compound or atomic collection expression");
            }
        }

        void CompileForEach(CCompileEnvironment& ce, cr_sex s)
        {
            ce.Builder.EnterSection();
            CompileForEachCore(ce, s);
            AppendDeconstruct(ce, s, true);
            ce.Builder.LeaveSection();
        }
    }//Script
}//Sexy