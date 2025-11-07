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

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(controlFlowData);
                    UNUSED(object);
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

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(object);
                    builder.PushControlFlowPoint(*controlFlowData);
                    CompileExpressionSequence(ce, 1, s.NumberOfElements() - 3, s);
                    builder.PopControlFlowPoint();
                }
            } bodySection(s, ce);

            ce.Builder.AppendDoWhile(bodySection, loopCriterion, CONDITION_IF_NOT_EQUAL);
        }

        // Returns -1 if no matches
        int FindIndexOfMatchingAtomic(int startIndex, cr_sex s, cstr token)
        {
            for (int i = startIndex; i < s.NumberOfElements(); ++i)
            {
                auto& si = s[i];
                if (IsAtomic(si) && Eq(si.c_str(), token))
                {
                   return i;
                }
            }

            return -1;
        }

        void CompileForLoop(CCompileEnvironment& ce, cr_sex s)
        {
            // (for (initialization-pre-loop-expressions)(binary-predicate)(finally expression) 
            //    (body)
            // )

            AssertCompound(s);
            AssertNotTooFewElements(s, 4);

            cr_sex sInitialization = s[1];

            CompileExpression(ce, sInitialization);

            cr_sex sBinaryPredicate = s[2];

            cr_sex sFinally = s[3];

            struct ConditionSection : public ICompileSection
            {
                ConditionSection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(object);
                    UNUSED(controlFlowData);
                    bool negate = false;
                    if (!TryCompileBooleanExpression(ce, s, true, negate))
                    {
                        Throw(s, "Expecting boolean valued expression as the condition in the (for ...) statement");
                    }

                    if (negate) builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
                    builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);
                }
            } loopCriterion(sBinaryPredicate, ce);

            struct BodySection : public ICompileSection
            {
                BodySection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;
    
                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(object);
                    if (s.NumberOfElements() > 4)
                    {
                        builder.PushControlFlowPoint(*controlFlowData);
                        CompileExpressionSequence(ce, 4, s.NumberOfElements() - 1, s);
                        builder.PopControlFlowPoint();
                    }
                }
            } bodySection(s, ce);

            struct FinalSection : public ICompileSection
            {
                FinalSection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(builder);
                    UNUSED(object);
                    UNUSED(controlFlowData);
                    if (s.Type() != EXPRESSION_TYPE_NULL)
                    {
                        CompileExpression(ce, s);
                    }
                }
            } finalSection(sFinally, ce);

            ce.Builder.AppendWhileDo(loopCriterion, CONDITION_IF_NOT_EQUAL, bodySection, finalSection);
        }

        void CompileWhileLoop(CCompileEnvironment& ce, cr_sex s)
        {
            // (while (binary-predicate) (action1) ... (actionN) 
            // finally (end-section)
            // )

            AssertCompound(s);
            AssertNotTooFewElements(s, 2);

            int finallyPos = FindIndexOfMatchingAtomic(2, s, "finally");
            if (finallyPos == s.NumberOfElements() - 1)
            {
                Throw(s, "The [finally] statement was not followed by any directives");
            }

            finallyPos = finallyPos < 0 ? s.NumberOfElements() : finallyPos;

            struct ConditionSection : public ICompileSection
            {
                ConditionSection(cr_sex _s, CCompileEnvironment& _ce) : s(_s), ce(_ce) {}

                cr_sex s;
                CCompileEnvironment& ce;

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(object);
                    UNUSED(controlFlowData);
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
                BodySection(cr_sex _s, CCompileEnvironment& _ce, int32 _finalPos) : s(_s), ce(_ce), finallyPos(_finalPos) {}

                cr_sex s;
                CCompileEnvironment& ce;
                int finallyPos;

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(object);
                    if (finallyPos > 2)
                    {
                        builder.PushControlFlowPoint(*controlFlowData);
                        CompileExpressionSequence(ce, 2, finallyPos-1, s);
                        builder.PopControlFlowPoint();
                    }
                }
            } bodySection(s, ce, finallyPos);

            struct FinalSection : public ICompileSection
            {
                FinalSection(cr_sex _s, CCompileEnvironment& _ce, int _finallyPos) : s(_s), ce(_ce), finallyPos(_finallyPos) {}

                cr_sex s;
                CCompileEnvironment& ce;
                int finallyPos;

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
                {
                    UNUSED(builder);
                    UNUSED(object);
                    UNUSED(controlFlowData);
                    if (finallyPos < s.NumberOfElements() - 1)
                    {
                        CompileExpressionSequence(ce, finallyPos + 1, s.NumberOfElements() - 1, s);
                    }
                }
            } finalSection(s, ce, finallyPos);

            ce.Builder.AppendWhileDo(loopCriterion, CONDITION_IF_NOT_EQUAL, bodySection,finalSection);
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
                Throw(s, "'continue' is only valid inside a loop construct");
            }

            AppendDeconstructTailVariables(ce, s, false, ce.Builder.SectionArgCount(cfd.SectionIndex));

            int32 toContinue = ((int32)cfd.ContinuePosition) - ((int32)ce.Builder.Assembler().WritePosition());
            ce.Builder.Assembler().Append_Branch(toContinue);
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

                void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData /* NULL, as If...Else does not support break and continue */) override
                {
                    UNUSED(builder);
                    UNUSED(object);
                    UNUSED(controlFlowData);
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

        bool IsLabel(cr_sex s, cstr labelText)
        {
            if (s.NumberOfElements() == 2)
            {
                cr_sex sDirective = s[0];
                cr_sex sArg = s[1];

                if (IsAtomic(sDirective) && IsAtomic(sArg))
                {
                    if (Eq(sDirective.c_str(), "label"))
                    {
                        cstr labelArg = sArg.c_str();
                        if (Eq(labelArg, labelText))
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        inline int32 Diff(cr_sex s, size_t posA, size_t posB)
        {
            int64 a = (int64)posA;
            int64 b = (int64)posB;
            int64 delta = a - b;
            if (delta < (int64)INT_MIN || delta > (int64) INT_MAX)
            {
                Throw(s, "Unexpected diff length. Do not use gigabyte length functions.");
            }
            return (int32)delta;
        }

        void CompileGoto(CCompileEnvironment& ce, cr_sex s)
        {
            if (s.NumberOfElements() != 2 || !IsAtomic(s[1]))
            {
                Throw(s, "Expecting two atomic elements (goto <label>). Where <label> is a camel-case string defined in either a sibling or ancestor of the expression containing the goto statement.");
            }

            cr_sex sLabel = s[1];

            cstr labelName = sLabel.c_str();

            AssertLocalIdentifier(sLabel);

            cr_sex sParent = *s.Parent();
            UNUSED(sParent);

            size_t labelPosition = ce.Builder.GetLabelPosition(labelName);

            try
            {
                if (labelPosition == (size_t) -1LL)
                {
                    size_t gotoPosition = ce.Builder.Assembler().WritePosition();
                    ce.Builder.PreventMoreVariablesUntil(labelName);
                    ce.Builder.Assembler().Append_Branch(0); // This will get overwritten with label delta
                    ce.Builder.MarkGoto(gotoPosition, labelName);
                }
                else
                {
                    size_t gotoCleanupPosition = ce.Builder.Assembler().WritePosition();
                    ce.Builder.AddDestructors(labelPosition, gotoCleanupPosition);
                    size_t gotoBranchPosition = ce.Builder.Assembler().WritePosition();
                    int32 delta = Diff(s, labelPosition, gotoBranchPosition);
                    ce.Builder.Assembler().Append_Branch(delta);
                }
            }
            catch (ParseException&)
            {
                throw;
            }
            catch (IException& ex)
            {
                Throw(s, "%s", ex.Message());
            }
        }

        void CompileLabel(CCompileEnvironment& ce, cr_sex s)
        {
            if (s.NumberOfElements() != 2 || !IsAtomic(s[1]))
            {
                Throw(s, "Expecting two atomic elements (label <labelName>). Where <labelName> is a camel-case string");
            }

            cr_sex sLabel = s[1];

            cstr labelName = sLabel.c_str();

            AssertLocalIdentifier(sLabel);

            try
            {
                AddSymbol(ce.Builder, "(label %s)", labelName);
                ce.Builder.MarkLabel(labelName);
            }
            catch (IException& ex)
            {
                Throw(sLabel, "(label %s) failed to compile: %s", labelName,  ex.Message());
            }
        }
    }//Script
}//Sexy