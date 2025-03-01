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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/


namespace Rococo::Sex
{
	void Throw(cr_sex e, const fstring& f)
	{
		Throw(e, "%s", f.buffer);
	}

	void ThrowTypeMismatch(cr_sex s, const IStructure& a, const IStructure& b, cstr extra)
	{
		Throw(s, "Type mismatch: %s != %s. %s", GetFriendlyName(a), GetFriendlyName(b), extra);
	}

	void AssertAtomicMatch(cr_sex s, cstr value)
	{
		if (!IsAtomic(s) || !AreEqual(s.String(), value))
		{
			Throw(s, "Expecting '%s'  at this position", value);
		}
	}

	void AssertCompoundOrNull(cr_sex e)
	{
		if (e.Type() != EXPRESSION_TYPE_COMPOUND && e.Type() != EXPRESSION_TYPE_NULL)
		{
			Throw(e, "Expecting compound or null expression, but found a %s expression", ToString(e.Type()));
		}
	}

	void ThrowTokenAlreadyDefined(cr_sex s, cstr name, cstr repository, cstr type)
	{
		Throw(s, "%s '%s' is already defined in %s", type, name, repository);
	}

	INamespaceBuilder& AssertGetSubspace(IProgramObject& object, cr_sex s, cstr name)
	{
		INamespaceBuilder* ns = object.GetRootNamespace().FindSubspace(name);
		if (ns == NULL)
		{
			Throw(s, "Unrecognized namespace");
		}
		return *ns;
	}

	void AssertSplitTail(NamespaceSplitter& splitter, cr_sex s, OUT cstr& body, OUT cstr& tail)
	{
		if (!splitter.SplitTail(OUT body, OUT tail))
		{
			Throw(s, "Expecting fully qualified namespace name");
		}
	}

	void AssertMacroShortName(cr_sex s, cstr shortName)
	{
		if (!IsAlphabetical(shortName[0]))
		{
			Throw(s, "Expecting macro name to begin with a letter {a-z} or {A-Z} at position[0]");
		}

		int len = StringLength(shortName);

		for (int i = 1; i < len; ++i)
		{
			char c = shortName[i];
			if (!IsAlphaNumeric(c))
			{
				Throw(s, "Expecting identifier character to be an alphanumeric {A-Z or a-z or 0-9} at position[%d]", i);
			}
		}
	}

	void AssertPascalCaseNameValid(cstr text, int length, int maxLength, cstr desc)
	{
		if (length == 0)
		{
			Rococo::Throw(0, "Expecting non-blank string identifier");
		}

		if (length >= maxLength)
		{
			Rococo::Throw(0, "Expecting %s, but the string was too long. Exceeded %d characters", desc, maxLength);
		}

		enum STATE
		{
			STATE_EXPECTING_NEW_ITEM,
			STATE_WITHIN_ITEM,
		} state = STATE_EXPECTING_NEW_ITEM;

		for (int i = 0; i < length; i++)
		{
			char c = text[i];

			switch (state)
			{
			case STATE_EXPECTING_NEW_ITEM:
				if (!IsCapital(c))
				{
					Rococo::Throw(0, "Expecting %s to begin with capital letter {A-Z} at position[%d]", desc, i);
				}
				state = STATE_WITHIN_ITEM;
				break;
			case STATE_WITHIN_ITEM:
				if (c == '.')
				{
					state = STATE_EXPECTING_NEW_ITEM;
				}
				else if (!IsAlphaNumeric(c))
				{
					Rococo::Throw(0, "Expecting alphanumeric {A-Z or a-z or 0-9} in %s at position[%d]", desc, i);
				}
				break;
			}
		}

		if (state == STATE_EXPECTING_NEW_ITEM)
		{
			Rococo::Throw(0, "Expecting %s to terminate on an alphanumeric {A-Z or a-z or 0-9} at position[%d]", desc, length);
		}
	}

	void AssertQualifiedIdentifier(cstr text, int length, cstr desc)
	{
		if (length == 0)
		{
			Rococo::Throw(0, "Expecting non-blank string identifier");
		}

		if (length >= NAMESPACE_MAX_LENGTH)
		{
			Rococo::Throw(0, "Expecting %s, but the string was too long. Exceeded %d characters", desc, NAMESPACE_MAX_LENGTH);
		}

		int startIndex = 0;

		if (text[0] == '$')
		{
			if (length == 1)
			{
				// '$'
				return;
			}
			else if (length > 1 && text[1] != '.')
			{
				Rococo::Throw(0, "Default namespace character detected, but expected it to be followed by a dot");
			}

			startIndex = 2;
		}

		enum STATE
		{
			STATE_EXPECTING_NEW_ITEM,
			STATE_WITHIN_ITEM,
		} state = STATE_EXPECTING_NEW_ITEM;

		for (int i = startIndex; i < length; i++)
		{
			char c = text[i];

			switch (state)
			{
			case STATE_EXPECTING_NEW_ITEM:
				if (!IsCapital(c))
				{
					Rococo::Throw(0, "Expecting %s to begin with capital letter {A-Z} at position[%d]", desc, i);
				}
				state = STATE_WITHIN_ITEM;
				break;
			case STATE_WITHIN_ITEM:
				if (c == '.')
				{
					state = STATE_EXPECTING_NEW_ITEM;
				}
				else if (!IsAlphaNumeric(c))
				{
					Rococo::Throw(0, "Expecting alphanumeric {A-Z or a-z or 0-9} in %s at position[%d]", desc, i);
				}
				break;
			}
		}

		if (state == STATE_EXPECTING_NEW_ITEM)
		{
			Rococo::Throw(0, "Expecting typename to terminate on an alphanumeric {A-Z or a-z or 0-9} at position[%d]", desc, length);
		}
	}

	void AssertQualifiedIdentifier(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring text = e.String();

		try
		{
			AssertQualifiedIdentifier(text->Buffer, text->Length, "typename");
		}
		catch (IException& ex)
		{
			Throw(e, "%s", ex.Message());
		}
	}

	void AssertLocalVariableOrMember(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring text = e.String();

		if (text->Length == 0)
		{
			Throw(e, ("Expecting non-blank string identifier"));
		}

		if (text->Length >= NAMESPACE_MAX_LENGTH)
		{
			Throw(e, "Identifier was too long. Exceeded %d characters", NAMESPACE_MAX_LENGTH);
		}

		enum STATE
		{
			STATE_EXPECTING_NEW_ITEM,
			STATE_WITHIN_ITEM,
		} state = STATE_EXPECTING_NEW_ITEM;

		for (int i = 0; i < text->Length; i++)
		{
			char c = text->Buffer[i];

			switch (state)
			{
			case STATE_EXPECTING_NEW_ITEM:
				if (!IsLowerCase(c))
				{
					Throw(e, "Expecting local variable or member name '%s' to begin with lower case letter {A-Z} at position[%d]", text->Buffer, i);
				}
				state = STATE_WITHIN_ITEM;
				break;
			case STATE_WITHIN_ITEM:
				if (c == '.')
				{
					state = STATE_EXPECTING_NEW_ITEM;
				}
				else if (!IsAlphaNumeric(c))
				{
					Throw(e, "Expecting alphanumeric {A-Z or a-z or 0-9} at position[%d]", i);
				}
				break;
			}
		}

		if (state == STATE_EXPECTING_NEW_ITEM)
		{
			Throw(e, "Expecting identifier to terminate on an alphanumeric {A-Z or a-z or 0-9} at position[%d]");
		}
	}

	void AssertValidNamespaceDef(cr_sex e)
	{
		AssertQualifiedIdentifier(e);

		const sexstring text = e.String();

		if (text->Length >= NAMESPACE_MAX_LENGTH)
		{
			Throw(e, "Namespace name was greater than the maximum length of %d characters", NAMESPACE_MAX_LENGTH);
		}
	}

	[[noreturn]] void ThrowNamespaceConflict(cr_sex s, const INamespace& n1, const INamespace& n2, cstr type, cstr token)
	{
		Throw(s, "%s %s could belong to either %s or %s", type, token, n1.FullName()->Buffer, n2.FullName()->Buffer);
	}

	[[noreturn]] void ThrowTokenNotFound(cr_sex s, cstr item, cstr repository, cstr type)
	{
		Throw(s, "%s '%s' not found in %s", item, type, repository);
	}

	INamespace& AssertGetNamespace(IProgramObject& object, cr_sex s, cstr fullName)
	{
		INamespace* ns = object.GetRootNamespace().FindSubspace(fullName);
		if (ns == NULL)
		{
			Throw(s, "The namespace %s  was unrecognized.", fullName);
		}
		return *ns;
	}

	void AssertValidFunctionName(cr_sex e)
	{
		AssertQualifiedIdentifier(e);

		const sexstring text = e.String();

		enum { FUNCTION_NAME_MAX_TOTAL_LENGTH = 63 };

		for (const char* c = text->Buffer; *c != 0; ++c)
		{
			if (*c == '.')
			{
				ptrdiff_t finalLen = ((ptrdiff_t)text->Length) - (c - text->Buffer) - 1;
				if (finalLen > FUNCTION_NAME_MAX_TOTAL_LENGTH)
				{
					Throw(e, "Method name was greater than the maximum length of %d characters", FUNCTION_NAME_MAX_TOTAL_LENGTH);
				}

				return;
			}
		}

		if (text->Length > FUNCTION_NAME_MAX_TOTAL_LENGTH)
		{
			Throw(e, "Function name was greater than the maximum length of %d  characters", FUNCTION_NAME_MAX_TOTAL_LENGTH);
		}
	}

	void AssertLocalIdentifier(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring s = e.String();

		if (!IsLowerCase(s->Buffer[0]))
		{
			Throw(e, "Expecting identifier to begin with a lower case letter {a-z} at position[0]");
		}

		for (int i = 1; i < s->Length; ++i)
		{
			char c = s->Buffer[i];

			if (!IsAlphaNumeric(c))
			{
				Throw(e, "Expecting identifier character to be an alphanumeric {A-Z or a-z or 0-9} at position[%d]", i);
			}
		}
	}

	void AssertTypeIdentifier(cr_sex src, cstr name)
	{
		if (!IsCapital(name[0]))
		{
			Throw(src, "Expecting typename to begin with an upper case letter {A-Z} at position[0]");
		}

		int len = StringLength(name);
		for (int i = 1; i < len; ++i)
		{
			char c = name[i];
			if (!IsAlphaNumeric(c))
			{
				Throw(src, "Expecting typename character to be an alphanumeric{A - Z or a - z or 0 - 9} at position[%d]", i);
			}
		}
	}

	void AssertTypeIdentifier(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring s = e.String();
		AssertTypeIdentifier(e, s->Buffer);

		if (!IsCapital(s->Buffer[0]))
		{
			Throw(e, "Expecting typename to begin with an upper case letter {A-Z} at position[0]");
		}

		for (int i = 1; i < s->Length; ++i)
		{
			char c = s->Buffer[i];
			if (!IsAlphaNumeric(c))
			{
				Throw(e, "Expecting typename character to be an alphanumeric {A-Z or a-z or 0-9} at position[%d]", i);
			}
		}
	}

	void AssertValidStructureName(cr_sex e)
	{
		AssertTypeIdentifier(e);

		sexstring text = e.String();

		enum { STRUCT_NAME_MAX_TOTAL_LENGTH = 31 };
		if (text->Length > STRUCT_NAME_MAX_TOTAL_LENGTH)
		{
			Throw(e, "Structure name was greater than the maximum length of %d characters", STRUCT_NAME_MAX_TOTAL_LENGTH);
		}
	}

	void AssertValidInterfaceName(cr_sex src, cstr name)
	{
		AssertTypeIdentifier(src, name);

		enum { INTERFACE_NAME_MAX_TOTAL_LENGTH = 31 };

		if (StringLength(name) > INTERFACE_NAME_MAX_TOTAL_LENGTH)
		{
			Throw(src, "Interface name was greater than the maximum length of %d characters", INTERFACE_NAME_MAX_TOTAL_LENGTH);
		}
	}

	void AssertValidArchetypeName(cr_sex src, cstr name)
	{
		AssertTypeIdentifier(src, name);

		enum { ARCHETYPE_NAME_MAX_TOTAL_LENGTH = 31 };

		if (StringLength(name) > ARCHETYPE_NAME_MAX_TOTAL_LENGTH)
		{
			Throw(src, "Archetype name was greater than the maximum length of %d characters", ARCHETYPE_NAME_MAX_TOTAL_LENGTH);
		}
	}

	void AssertKeyword(cr_sex e, int arg, cstr name)
	{
		cr_sex item = GetAtomicArg(e, arg);
		if (!AreEqual(item.String(), name))
		{
			Throw(e, "Expecting %s at position %d", name, arg);
		}
	}

	enum { ARGCOUNT_UNSPECIFIED = -1 };
	void AssertCompound(cr_sex e, cstr headName, int32 minElements, int32 maxElements)
	{
		AssertCompound(e);
		AssertNotTooFewElements(e, minElements);
		AssertNotTooManyElements(e, maxElements);

		if (minElements >= 1 && headName != NULL)
		{
			const ISExpression& firstElement = e.GetElement(0);
			AssertAtomic(firstElement);

			if (!AreEqual(firstElement.String(), headName))
			{
				Throw(e, "Expecting expression (%s ...)  but found a (%s ...)", headName, firstElement.c_str());
			}
		}
	}

	sexstring GetDefiniteAtomicArg(cr_sex s, cstr headName, int argNumber)
	{
		AssertCompound(s, headName, argNumber + 1, ARGCOUNT_UNSPECIFIED);
		const ISExpression& child = s.GetElement(argNumber);
		AssertAtomic(child);
		return child.String();
	}

	void StreamArg(StringBuilder& streamer, cstr name, const IStructure& type)
	{
		streamer << (" (") << GetFriendlyName(type) << (" ") << name << (")");
	}

	void StreamFullMethod(StringBuilder& streamer, const IArchetype& callee)
	{
		streamer << callee.Name() << (" ");

		int nInputs = callee.NumberOfInputs();
		if (callee.IsVirtualMethod()) nInputs--;

		for (int i = 0; i < nInputs; i++)
		{
			cstr name = callee.GetArgName(i + callee.NumberOfOutputs());
			const IStructure& st = callee.GetArgument(i + callee.NumberOfOutputs());
			StreamArg(streamer, name, st);
		}

		streamer << ("->");

		for (int i = 0; i < callee.NumberOfOutputs(); i++)
		{
			cstr name = callee.GetArgName(i);
			const IStructure& st = callee.GetArgument(i);
			StreamArg(streamer, name, st);
		}
	}

	void ValidateNumberOfInputArgs(cr_sex s, const IArchetype& callee, int numberOfSuppliedInputArgs)
	{
		if (callee.NumberOfInputs() == numberOfSuppliedInputArgs) return;
		if (IsGetAccessor(callee) && numberOfSuppliedInputArgs == 0) return;

		char fullMethod[256];
		StackStringBuilder ssb(fullMethod, sizeof fullMethod);
		StreamFullMethod(ssb, callee);

		Throw(s, "Function call '%s' was supplied with too %s inputs", fullMethod, callee.NumberOfInputs() < numberOfSuppliedInputArgs ? "many" : "few");
	}
}