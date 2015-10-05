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


namespace Sexy { namespace Sex
{
	void Throw(ParseException& ex)
	{
		OS::BreakOnThrow();
		throw ex;
	}

	SCRIPTEXPORT_API void Throw(cr_sex e, csexstr message)
	{
		SEXCHAR specimen[64];
		GetSpecimen(specimen, e);
		ParseException ex(e.Start(), e.End(), e.Tree().Source().Name(), message, specimen, &e);
		Throw(ex);
	}	

	void Throw(cr_sex e, sexstringstream& streamer)
	{
		streamer << std::ends;
		Throw(e, streamer.str().c_str());
	}

	void ThrowTypeMismatch(cr_sex s, const IStructure& a, const IStructure& b, csexstr extra)
	{
		sexstringstream streamer;
		streamer << SEXTEXT("Type mismatch: ") << GetFriendlyName(a) << SEXTEXT(" != ") << GetFriendlyName(b) << SEXTEXT(". ") << extra;
		Throw(s, streamer);
	}

#ifdef SEXCHAR_IS_WIDE

	void Throw(const ISExpression& e, const char* message)
	{
		SEXCHAR specimen[64];
		GetSpecimen(specimen, e);
		wchar_t unicodeMessage[256];
		swprintf_s(unicodeMessage, 256, L"%S", message);
		ParseException ex(e.Start(), e.End(), e.Tree().Source().Name(), unicodeMessage, specimen, &e);
		OS::BreakOnThrow();
		throw ex;
	}

#endif

	csexstr ToString(EXPRESSION_TYPE type)
	{
		switch(type)
		{
		case EXPRESSION_TYPE_ATOMIC: return SEXTEXT("atomic");
		case EXPRESSION_TYPE_STRING_LITERAL: return SEXTEXT("string-literal");
		case EXPRESSION_TYPE_COMPOUND: return SEXTEXT("compound");
		case EXPRESSION_TYPE_NULL: return SEXTEXT("null");
		default: return SEXTEXT("unknown");
		}
	}

	void AssertExpressionType(cr_sex e, EXPRESSION_TYPE type)
	{
		if (e.Type() != type)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting ") << ToString(type) << " expression, but found " << ToString(e.Type()) << " expression";
			Throw(e, streamer);
		}
	}

	void AssertAtomicMatch(cr_sex s, csexstr value)
	{
		if (!IsAtomic(s) || !AreEqual(s.String(), SEXTEXT("=")))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting '") << value << SEXTEXT("' at this position");
			Throw(s, streamer);
		}
	}

	void AssertCompoundOrNull(cr_sex e)
	{
		if (e.Type() != EXPRESSION_TYPE_COMPOUND && e.Type() != EXPRESSION_TYPE_NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting compound or null expression, but found a ") << ToString(e.Type()) << " expression";
			Throw(e, streamer);
		}
	}

	void ThrowTokenAlreadyDefined(cr_sex s, csexstr name, csexstr repository, csexstr type)
	{
		sexstringstream streamer;
		streamer << type << SEXTEXT(" '") << name << SEXTEXT("' is already defined in ") << repository;
		Throw(s, streamer);
	}

	INamespaceBuilder& AssertGetSubspace(IProgramObject& object, cr_sex s, csexstr name)
	{
		INamespaceBuilder* ns = object.GetRootNamespace().FindSubspace(name);
		if (ns == NULL)
		{
			Throw(s, SEXTEXT("Unrecognized namespace"));
		}
		return *ns;
	}

	void AssertSplitTail(NamespaceSplitter& splitter, cr_sex s, OUT csexstr& body, OUT csexstr& tail)
	{
		if (!splitter.SplitTail(OUT body, OUT tail))
		{
			Throw(s, SEXTEXT("Expecting fully qualified namespace name"));
		}
	}

	void AssertMacroShortName(cr_sex s, csexstr shortName)
	{
		if (!IsLowerCase(shortName[0]))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting macro name to begin with a lower case letter {a-z} at position[0]");
			Throw(s, streamer);
		}

		int len = StringLength(shortName);

		for(int i = 1; i < len; ++i)
		{
			SEXCHAR c = shortName[i];
			if (!IsAlphaNumeric(c))
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Expecting identifier character to be an alphanumeric {A-Z or a-z or 0-9} at position[") << i << SEXTEXT("]");
				Throw(s, streamer);
			}
		}
	}

	SCRIPTEXPORT_API void AssertCompound(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_COMPOUND);	}
	SCRIPTEXPORT_API void AssertAtomic(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_ATOMIC);	}
	SCRIPTEXPORT_API void AssertStringLiteral(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_STRING_LITERAL);	}
	void AssertNull(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_NULL);	}

	void AssertQualifiedIdentifier(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring text = e.String();

		if (text->Length == 0)
		{
			Throw(e, SEXTEXT("Expecting non-blank string identifier"));
		}

		if (text->Length >= NAMESPACE_MAX_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting typename, but the string was too long. Exceeded ") << NAMESPACE_MAX_LENGTH << SEXTEXT(" characters");
			Throw(e, streamer);
		}

		enum STATE
		{
			STATE_EXPECTING_NEW_ITEM,
			STATE_WITHIN_ITEM,
		} state = STATE_EXPECTING_NEW_ITEM;

		for(int i = 0; i < text->Length; i++)
		{
			SEXCHAR c = text->Buffer[i];

			switch(state)
			{
			case STATE_EXPECTING_NEW_ITEM:
				if (!IsCapital(c))
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Expecting typename '") << text->Buffer << SEXTEXT("' to begin with capital letter {A-Z} at position[") << i << SEXTEXT("]");
					Throw(e, streamer);
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
					sexstringstream streamer;
					streamer << SEXTEXT("Expecting alphanumeric {A-Z or a-z or 0-9} at position[") << i << SEXTEXT("]");
					Throw(e, streamer);
				}
				break;
			}
		}

		if (STATE_EXPECTING_NEW_ITEM)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting typename to terminate on an alphanumeric {A-Z or a-z or 0-9} at position[") << text->Length << SEXTEXT("]");
			Throw(e, streamer);
		}
	}

	void AssertLocalVariableOrMember(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring text = e.String();

		if (text->Length == 0)
		{
			Throw(e, SEXTEXT("Expecting non-blank string identifier"));
		}

		if (text->Length >= NAMESPACE_MAX_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Identifier was too long. Exceeded ") << NAMESPACE_MAX_LENGTH << SEXTEXT(" characters");
			Throw(e, streamer);
		}

		enum STATE
		{
			STATE_EXPECTING_NEW_ITEM,
			STATE_WITHIN_ITEM,
		} state = STATE_EXPECTING_NEW_ITEM;

		for(int i = 0; i < text->Length; i++)
		{
			SEXCHAR c = text->Buffer[i];

			switch(state)
			{
			case STATE_EXPECTING_NEW_ITEM:
				if (!IsLowerCase(c))
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Expecting local variable or member nane '") << text->Buffer << SEXTEXT("' to begin with lower case letter {A-Z} at position[") << i << SEXTEXT("]");
					Throw(e, streamer);
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
					sexstringstream streamer;
					streamer << SEXTEXT("Expecting alphanumeric {A-Z or a-z or 0-9} at position[") << i << SEXTEXT("]");
					Throw(e, streamer);
				}
				break;
			}
		}

		if (STATE_EXPECTING_NEW_ITEM)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting identifier to terminate on an alphanumeric {A-Z or a-z or 0-9} at position[") << text->Length << SEXTEXT("]");
			Throw(e, streamer);
		}
	}

	void AssertValidNamespaceDef(cr_sex e)
	{
		AssertQualifiedIdentifier(e);

		const sexstring text = e.String();

		enum { NAMESPACE_MAX_TOTAL_LENGTH = 63 };

		if (text->Length > NAMESPACE_MAX_TOTAL_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Namespace name was greater than the maximum length of ") << NAMESPACE_MAX_TOTAL_LENGTH << SEXTEXT(" characters");
			Throw(e, streamer);
		}
	}

	void ThrowNamespaceConflict(cr_sex s, INamespace& n1, INamespace& n2, csexstr type, csexstr token)
	{
		sexstringstream streamer;
		streamer << type << SEXTEXT(" ") << token << SEXTEXT(" could belong to either ") << n1.FullName()->Buffer << (" or ") << n2.FullName()->Buffer;
		Throw(s, streamer);
	}

	void ThrowTokenNotFound(cr_sex s, csexstr item, csexstr repository, csexstr type)
	{
		sexstringstream streamer;
		streamer << type << SEXTEXT(" '") << item << SEXTEXT("' not found in ") << repository;
		Throw(s, streamer);
	}

	INamespace& AssertGetNamespace(IProgramObject& object, cr_sex s, csexstr fullName)
	{
		INamespace* ns = object.GetRootNamespace().FindSubspace(fullName);
		if (ns == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("The namespace ") << fullName << (" was unrecognized.");
			Throw(s, streamer);
		}
		return *ns;
	}			

	void AssertValidFunctionName(cr_sex e)
	{
		AssertQualifiedIdentifier(e);

		const sexstring text = e.String();

		enum {FUNCTION_NAME_MAX_TOTAL_LENGTH = 31 };

		for(const SEXCHAR* c = text->Buffer; *c != 0; ++c)
		{
			if (*c == '.')
			{
				ptrdiff_t finalLen = ((ptrdiff_t) text->Length) - (c - text->Buffer) - 1;
				if (finalLen > FUNCTION_NAME_MAX_TOTAL_LENGTH)
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Method name was greater than the maximum length of ") << FUNCTION_NAME_MAX_TOTAL_LENGTH << SEXTEXT(" characters");
					Throw(e, streamer);
				}

				return;
			}
		}

		if (text->Length > FUNCTION_NAME_MAX_TOTAL_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Function name was greater than the maximum length of ") << FUNCTION_NAME_MAX_TOTAL_LENGTH << SEXTEXT(" characters");
			Throw(e, streamer);
		}
	}

	void AssertLocalIdentifier(cr_sex e)
	{
		AssertAtomic(e);
		const sexstring s = e.String();

		if (!IsLowerCase(s->Buffer[0]))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting identifier to begin with a lower case letter {a-z} at position[0]");
			Throw(e, streamer);
		}

		for(int i = 1; i < s->Length; ++i)
		{
			SEXCHAR c = s->Buffer[i];

			if (!IsAlphaNumeric(c))
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Expecting identifier character to be an alphanumeric {A-Z or a-z or 0-9} at position[") << i << SEXTEXT("]");
				Throw(e, streamer);
			}
		}
	}

	void AssertTypeIdentifier(cr_sex src, csexstr name)
	{
		if (!IsCapital(name[0]))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting typename to begin with an upper case letter {A-Z} at position[0]");
			Throw(src, streamer);
		}

		int len = StringLength(name);
		for(int i = 1; i < len; ++i)
		{
			SEXCHAR c = name[i];
			if (!IsAlphaNumeric(c))
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Expecting typename character to be an alphanumeric {A-Z or a-z or 0-9} at position[") << i << SEXTEXT("]");
				Throw(src, streamer);
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
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting typename to begin with an upper case letter {A-Z} at position[0]");
			Throw(e, streamer);
		}

		for(int i = 1; i < s->Length; ++i)
		{
			SEXCHAR c = s->Buffer[i];
			if (!IsAlphaNumeric(c))
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Expecting typename character to be an alphanumeric {A-Z or a-z or 0-9} at position[") << i << SEXTEXT("]");
				Throw(e, streamer);
			}
		}
	}

	void AssertValidStructureName(cr_sex e)
	{
		AssertTypeIdentifier(e);

		sexstring text = e.String();

		enum {STRUCT_NAME_MAX_TOTAL_LENGTH = 31 };
		if (text->Length > STRUCT_NAME_MAX_TOTAL_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Structure name was greater than the maximum length of ") << STRUCT_NAME_MAX_TOTAL_LENGTH << SEXTEXT(" characters");
			Throw(e, streamer);
		}
	}

	void AssertValidInterfaceName(cr_sex src, csexstr name)
	{
		AssertTypeIdentifier(src, name);

		enum {INTERFACE_NAME_MAX_TOTAL_LENGTH = 31 };

		if (StringLength(name) > INTERFACE_NAME_MAX_TOTAL_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Interface name was greater than the maximum length of ") << INTERFACE_NAME_MAX_TOTAL_LENGTH << SEXTEXT(" characters");
			Throw(src, streamer);
		}
	}

	void AssertValidArchetypeName(cr_sex src, csexstr name)
	{
		AssertTypeIdentifier(src, name);

		enum {ARCHETYPE_NAME_MAX_TOTAL_LENGTH = 31 };

		if (StringLength(name) > ARCHETYPE_NAME_MAX_TOTAL_LENGTH)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Archetype name was greater than the maximum length of ") << ARCHETYPE_NAME_MAX_TOTAL_LENGTH << SEXTEXT(" characters");
			Throw(src, streamer);
		}
	}

	SCRIPTEXPORT_API void AssertNotTooManyElements(cr_sex e, int32 maxElements)
	{
		int32 elementCount = e.NumberOfElements();
		if (maxElements > 0 && elementCount > maxElements)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expression had more than the maximum of ") << maxElements << " element" << (maxElements > 1 ? SEXTEXT("s") : SEXTEXT(""));
			Throw(e, streamer);
		}
	}

	SCRIPTEXPORT_API void AssertNotTooFewElements(cr_sex e, int32 minElements)
	{
		int32 elementCount = e.NumberOfElements();
		if (minElements > 0 && elementCount < minElements)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expression had fewer than the minimum of ") << minElements << SEXTEXT(" element") << (minElements > 1 ? SEXTEXT("s") : SEXTEXT(""));
			Throw(e, streamer);
		}
	}

	SCRIPTEXPORT_API cr_sex GetAtomicArg(cr_sex e, int argIndex)
	{
		AssertCompound(e);
		AssertNotTooFewElements(e, argIndex+1);
		cr_sex arg =	e.GetElement(argIndex);
		AssertAtomic(arg);
		return arg;
	}

	void AssertKeyword(cr_sex e, int arg, csexstr name)
	{
		cr_sex item = GetAtomicArg(e, arg);
		if (!AreEqual(item.String(), name))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting ") << name << SEXTEXT(" at position ") << arg;
			Throw(e, streamer);
		}
	}

	enum { ARGCOUNT_UNSPECIFIED = -1};
	void AssertCompound(cr_sex e, csexstr headName, int32 minElements, int32 maxElements)
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
				sexstringstream streamer;
				streamer << SEXTEXT("Expecting expression (") << headName << SEXTEXT(" ...), but found a (") << firstElement.String()->Buffer << SEXTEXT(" ...)");
				Throw(e, streamer);
			}
		}
	}

	sexstring GetDefiniteAtomicArg(cr_sex s, csexstr headName, int argNumber)
	{
		AssertCompound(s, headName, argNumber+1, ARGCOUNT_UNSPECIFIED);
		const ISExpression& child = s.GetElement(argNumber);
		AssertAtomic(child);
		return child.String();
	}

	void StreamArg(sexstringstream& streamer, csexstr name, const IStructure& type)
	{
		streamer << SEXTEXT(" (") << GetFriendlyName(type) << SEXTEXT(" ") << name << SEXTEXT(")");
	}

	void StreamFullMethod(sexstringstream& streamer, const IArchetype& callee)
	{
		streamer << callee.Name() << SEXTEXT(" ");

		int nInputs = callee.NumberOfInputs();
		if (callee.IsVirtualMethod()) nInputs--;

		for(int i = 0; i < nInputs; i++)
		{
			csexstr name = callee.GetArgName(i+callee.NumberOfOutputs());
			const IStructure& st = callee.GetArgument(i+callee.NumberOfOutputs());
			StreamArg(streamer, name, st);
		}

		streamer << SEXTEXT("->");

		for(int i = 0; i < callee.NumberOfOutputs(); i++)
		{
			csexstr name = callee.GetArgName(i);
			const IStructure& st = callee.GetArgument(i);
			StreamArg(streamer, name, st);
		}
	}

	bool IsGetAccessor(const IArchetype& callee)
	{
		if (callee.NumberOfOutputs() != 1) return false;

		if (callee.IsVirtualMethod())
		{
			return callee.NumberOfInputs() == 1;
		}
		else
		{
			return callee.NumberOfInputs() == 0;
		}
	}

	void ValidateNumberOfInputArgs(cr_sex s, const IArchetype& callee, int numberOfSuppliedInputArgs)
	{
		if (callee.NumberOfInputs() == numberOfSuppliedInputArgs) return;
		if (IsGetAccessor(callee) && numberOfSuppliedInputArgs == 0) return;

		sexstringstream streamer;
		streamer << SEXTEXT("Function call '");

		StreamFullMethod(streamer, callee);
			
		streamer << SEXTEXT("' was supplied with ");

		if (callee.NumberOfInputs() < numberOfSuppliedInputArgs) 
		{
			streamer << SEXTEXT("too many inputs");
		}
		else
		{
			streamer << SEXTEXT("too few inputs");
		}
		
		streamer << std::ends;
		Throw(s, streamer.str().c_str());
	}
}}