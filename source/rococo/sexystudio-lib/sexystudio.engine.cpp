// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.

// The SexyStudio completion engine: field, method, macro, enum and type completion, call tips, hints and goto definition.
// Moved out of SexyStudioIDE (sexystudio.cpp) unchanged, except that the two GUI calls it made are now IEngineHost callbacks.
// It works on AutoComplete::ISexyEditor and the workspace's current database only. No window, widget, IDE or Notepad++ type may appear here

#include "sexystudio.api.h"
#include "sexystudio.engine.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.auto-complete.h>
#include <rococo.sxytype-inference.h>
#include <rococo.types.inference.h>
#include <rococo.functional.h>
#include <rococo.api.qualifiers.h>
#include <ctype.h>
#include <string.h>
#include <vector>
#include <array>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Sex;
using namespace Rococo::Strings;
using namespace Rococo::AutoComplete;

namespace
{
	bool IsAComparison(cr_substring token)
	{
		constexpr std::array<const char*, 5> comparisonOperators
		{
			"=", "!=", "==", ">=", "<="
		};

		return std::any_of(comparisonOperators.begin(), comparisonOperators.end(), [token](cstr s) { return Eq(s, token); });
	}

	struct SexyCompletionEngine final : ISexyCompletionEngine, ICalltip
	{
		ISexyWorkspace& workspace;
		IEngineHost& host;

		SexyCompletionEngine(ISexyWorkspace& _workspace, IEngineHost& _host) : workspace(_workspace), host(_host)
		{
		}

		void Free() override
		{
			delete this;
		}

		// The current project's database, or the blank one. Never cached: the workspace may switch project or repopulate between calls
		ISexyDatabase& GetDatabase()
		{
			return workspace.GetDatabase();
		}

		int64 autoComplete_Replacement_StartPosition = 0;
		char callTipArgs[1024] = { 0 };

		std::vector<char> src_buffer;
		char src_line[1024];

		void SetCalltipForReplacement(cstr tip) override
		{
			SafeFormat(callTipArgs, "%s", tip);
		}


		void ReplaceCurrentSelectionWithCallTip(Rococo::AutoComplete::ISexyEditor& editor) override
		{
			int64 caretPos = editor.GetCaretPos();

			if (*callTipArgs != 0 && *callTipArgs != '!' && autoComplete_Replacement_StartPosition > 0 && autoComplete_Replacement_StartPosition < caretPos)
			{
				editor.ReplaceText(caretPos, caretPos, callTipArgs);
				callTipArgs[0] = 0;
			}
		}

		struct SpaceSeparatedStringItems : IEnumerator<cstr>
		{
			StringBuilder& sb;

			int count = 0;

			void operator()(cstr item) override
			{
				if (count > 0)
				{
					sb << " ";
				}

				count++;

				sb << item;
			}

			SpaceSeparatedStringItems(StringBuilder& _sb) : sb(_sb) {}
		};

		template<class ACTION> bool EnumerateFieldsOfClass(cr_substring className, cr_substring doc, ACTION& action)
		{
			cr_substring def = Rococo::Sex::Inference::GetClassDefinition(className, doc);
			if (def)
			{
				struct ANON: IFieldEnumerator
				{
					ACTION& action;

					void OnMemberVariable(cstr name, cstr type) override
					{
						if (Eq(type, "implements"_fstring))
						{

						}
						else
						{
							action(name);
						}
					}

					ANON(ACTION& _action) : action(_action) {}
				} buildList(action);

				Rococo::Sex::Inference::ForEachFieldOfClassDef(className, def, buildList);
				return true;
			}	

			return false;
		};

		struct RouteTextToAutoComplete: ISexyFieldEnumerator
		{
			IAutoCompleteBuilder& builder;
			ISexyDatabase& database;
			cr_substring prefix;
			cr_substring document;

			bool atLeastOneItem = false;

			int memberDepth = 0;

			Substring hint { Substring::Null() };

			RouteTextToAutoComplete(IAutoCompleteBuilder& _builder, cr_substring _prefix, ISexyDatabase& _database, cr_substring _document):
				builder(_builder), prefix(_prefix), database(_database), document(_document)
			{

			}

			~RouteTextToAutoComplete()
			{
				if (atLeastOneItem)
				{
					builder.ShowAndClearItems();
				}
			}

			void OnField(cstr fieldName, cr_substring memberSearch) override
			{
				atLeastOneItem = true;

				if (memberSearch && prefix)
				{
					char prefixString[128];
					prefix.CopyWithTruncate(prefixString, sizeof prefixString);
					size_t startSubstituteAt = memberSearch.start - prefix.start;
					size_t endSubstituteAt = startSubstituteAt + strlen(fieldName);
					if (endSubstituteAt < sizeof prefixString - 1)
					{
						memcpy(prefixString + startSubstituteAt, fieldName, strlen(fieldName) + 1);
						builder.AddItem(prefixString);
					}
				}
				else if (strstr(fieldName, "/@*"))
				{
					builder.AddItem(fieldName + 3);
				}
				else if (prefix)
				{
					char prefixString[128];
					prefix.CopyWithTruncate(prefixString, sizeof prefixString);

					char item[256];
					SafeFormat(item, "%s%s", prefixString, fieldName);
					builder.AddItem(item);
				}
				else
				{
					builder.AddItem(fieldName);
				}
			}

			void OnHintFound(cr_substring hintText) override
			{
				hint = hintText;
				builder.AddHint(hintText);
			}

			enum { MAX_MEMBER_DEPTH = 64 };

			void OnFieldType(cr_substring fieldType, cr_substring searchRoot) override
			{
				memberDepth++;
				if (!database.EnumerateVariableAndFieldList(searchRoot, fieldType, *this))
				{
					if (memberDepth > MAX_MEMBER_DEPTH)
					{
						if (OS::IsDebugging())
						{
							OS::TripDebugger();
						}
						memberDepth--;
						return;
					}
					Rococo::Sex::Inference::EnumerateLocalFields(*this, searchRoot, fieldType, document);
				}
				memberDepth--;
			}
		};

		void ShowCallTipAtCaretPos(ISexyEditor& editor, cr_substring tip)
		{
			size_t len = tip.Length() + 1;
			char* buf = (char*)alloca(len);
			tip.CopyWithTruncate(buf, len);
			editor.ShowCallTipAtCaretPos(buf);
		}

		void ShowAutocompleteDataForMacro(ISexyEditor& editor, cr_substring candidate, int64 displacementFromCaret, cr_substring doc)
		{
			int64 caretPos = editor.GetCaretPos();
			cstr openMacro = doc.start + caretPos - displacementFromCaret - 1;
			if (openMacro <= doc.start || *openMacro != '(')
			{
				// macro invocations have the form (#<name> ...)
				return;
			}

			Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);
			token.start++;
			if (token.start >= token.finish || !IsAlphaNumeric(*token.start))
			{
				// We need (#<a> with at least one character a before we start autocomplete. Generally Sexy enumeration macros will have some standard prefix
			}

			RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring::Null(), GetDatabase(), doc);
			GetDatabase().ForEachAutoCompleteMacroCandidate(token, routeTextToAutoComplete);
		}

		void ShowAutocompleteDataForVariable(ISexyEditor& editor, cr_substring candidate, int64 tokenDisplacementFromCaret)
		{
			static auto thisDot = "this."_fstring;

			int64 caretPos = editor.GetCaretPos();

			Substring doc = CachedDoc(editor);

			cstr docCaretPos = doc.start + caretPos;
			cstr start = docCaretPos - tokenDisplacementFromCaret;
			cstr end = start + Length(candidate);

			Substring candidateInDoc{ start, end };

			Substring variable = { candidateInDoc.start, candidateInDoc.finish };
	
			if (StartsWith(variable, thisDot))
			{
				variable.start += thisDot.length;
			}

			RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), candidateInDoc, GetDatabase(), doc);

			Rococo::Sex::Inference::TypeInference inference;
			bool isThis;
			inference = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(isThis, candidateInDoc, doc);
			if (inference.declarationType)
			{
				if (isThis)
				{
					auto addFieldToAutocomplete = [&editor](cstr fieldName)
					{
						char qualifiedFieldName[128];
						SafeFormat(qualifiedFieldName, "this.%s", fieldName);
						editor.AutoCompleteBuilder().AddItem(qualifiedFieldName);
					};

					if (EnumerateFieldsOfClass(inference.declarationType, doc, addFieldToAutocomplete))
					{
						editor.AutoCompleteBuilder().ShowAndClearItems();
					}
					else
					{
						Substring finalType = routeTextToAutoComplete.hint ? routeTextToAutoComplete.hint : inference.declarationType;
						ShowCallTipAtCaretPos(editor, finalType);
					}
				}
				else
				{
					if (inference.templateContainer)
					{
						GetDatabase().EnumerateTemplateMethods(variable, inference, routeTextToAutoComplete);
					}
					else if (GetDatabase().EnumerateVariableAndFieldList(variable, inference.declarationType, routeTextToAutoComplete))
					{

					}
					else
					{	
						Rococo::Sex::Inference::EnumerateLocalFields(routeTextToAutoComplete, candidateInDoc, inference.declarationType, doc);
						if (!routeTextToAutoComplete.atLeastOneItem)
						{
							auto finalType = routeTextToAutoComplete.hint ? routeTextToAutoComplete.hint : inference.declarationType;
							ShowCallTipAtCaretPos(editor, finalType);
						}
					}
				}
			}
		}

		void ShowAutocompleteDataForType(ISexyEditor& editor, cr_substring candidate, cr_substring doc)
		{
			Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);

			RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring::Null(), GetDatabase(), doc);
			GetDatabase().ForEachAutoCompleteCandidate(token, routeTextToAutoComplete);

			callTipArgs[0] = 0;

			if (!routeTextToAutoComplete.atLeastOneItem)
			{
				GetHintForCandidate(token, callTipArgs);
				if (callTipArgs[0] != 0)
				{
					editor.ShowCallTipAtCaretPos(callTipArgs);
				}
			}
		}

		void ShowFunctionArgumentsForType(ISexyEditor& editor, cr_substring candidate)
		{
			Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);

			callTipArgs[0] = 0;

			GetHintForCandidate(token, callTipArgs);
			if (callTipArgs[0] != 0)
			{
				editor.ShowCallTipAtCaretPos(callTipArgs);
			}
		}

		cstr FindFirstNonBlankPrecedingPoint(cr_substring doc, cstr p)
		{
			for (cstr s = p - 1; s > doc.start; s--)
			{
				if (!isblank(*s))
				{
					return s;
				}
			}

			return nullptr;
		}

		bool TryAddEnumsToAutocomplete(ISexyEditor& editor, cr_substring candidate, int64 tokenDisplacementFromCaret, cr_substring doc)
		{
			int64 caretPos = editor.GetCaretPos();

			cstr docCaretPos = doc.start + caretPos;
			cstr start = docCaretPos - tokenDisplacementFromCaret;
			cstr end = start + Length(candidate);

			Substring candidateInDoc{ start, end };

			cstr tokenEnd = FindFirstNonBlankPrecedingPoint(doc, start);
			if (!tokenEnd)
			{
				return false;
			}

			cstr precedingTokenPtr = Rococo::Sex::Inference::GetFirstNonTokenPointerFromRight(Substring{ doc.start, tokenEnd }, tokenEnd);
			if (!precedingTokenPtr)
			{
				return false;
			}

			Substring precedingToken{ precedingTokenPtr + 1, tokenEnd + 1 };

			if (!IsAComparison(precedingToken))
			{
				return false;
			}

			tokenEnd = FindFirstNonBlankPrecedingPoint(doc, precedingTokenPtr);
			if (!tokenEnd)
			{
				return false;
			}

			cstr identifierLeft = Rococo::Sex::Inference::GetFirstNonTokenPointerFromRight(Substring{ doc.start, tokenEnd }, tokenEnd);
			if (!identifierLeft)
			{
				return false;
			}

			Substring identifier{ identifierLeft + 1, tokenEnd + 1 };

			if (identifier.empty())
			{
				return false;
			}

			bool isThis = false;
			auto typeInference =  Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(OUT isThis, identifier, doc);
			Substring type = typeInference.declarationType;
			if (type.empty())
			{
				return false;
			}

			char typeBuffer[256];
			type.CopyWithTruncate(typeBuffer, sizeof typeBuffer);

			auto* e = GetDatabase().FindEnum(typeBuffer);
			if (!e || e->NumberOfEntries() == 0)
			{
				return false;
			}

			for (size_t i = 0; i < e->NumberOfEntries(); i++)
			{
				cstr name;
				int64 value;
				e->GetEntry(i, OUT name, OUT value);

				char entry[256];
				SafeFormat(entry, sizeof entry, ".%s", name);

				editor.AutoCompleteBuilder().AddItem(entry);
			}

			editor.AutoCompleteBuilder().ShowAndClearItems();

			return true;
		}

		// True if p, a pointer into the line, is inside a // comment or a string literal. Only the current line is examined, so a /* */ comment
		// that opened on an earlier line is not seen. Sexy escapes characters in strings with '&', e.g. &" is a quote inside a string
		static bool IsInCommentOrStringOnLine(cr_substring line, cstr p)
		{
			bool inString = false;

			for (cstr s = line.start; s < p; s++)
			{
				if (inString)
				{
					if (*s == '&' && s + 1 < p)
					{
						s++;
					}
					else if (*s == '"')
					{
						inString = false;
					}
				}
				else if (*s == '"')
				{
					inString = true;
				}
				else if (*s == '/' && s + 1 < p && s[1] == '/')
				{
					return true;
				}
			}

			return inString;
		}

		// Explicitly qualified enum constants: <EnumType>@<Value>, e.g. Day@Friday or Sys.Time.Day@Friday (see IsEnumConstantSyntax in sexy.script.functions.inl).
		// '@' appears nowhere else in Sexy code, so when the caret follows <EnumType>@<prefix>, where prefix may be empty, this owns the completion:
		// it lists the type's values that start with the prefix, and returns true whether or not it listed anything, so that the caller does not
		// fall back to type or variable completion of the prefix. The items are bare value names, and autoComplete_Replacement_StartPosition is set
		// to just after the '@', so ReplaceSelectedText swaps the prefix for the chosen name. Returns false if the text before the caret is not of that form
		bool TryAddQualifiedEnumsToAutocomplete(ISexyEditor& editor, cr_substring line, cstr activationPoint, const EditorCursor& cursor, cr_substring doc)
		{
			cstr valueStart = activationPoint + 1;
			while (valueStart > line.start && IsAlphaNumeric(valueStart[-1]))
			{
				valueStart--;
			}

			cstr at = valueStart - 1;
			if (at < line.start || *at != '@')
			{
				return false;
			}

			cstr typeStart = at;
			while (typeStart > line.start && (IsAlphaNumeric(typeStart[-1]) || typeStart[-1] == '.'))
			{
				typeStart--;
			}

			if (typeStart == at || !isupper(*typeStart))
			{
				// Enum types begin with a capital letter, so this is not an enum constant. It may be a variable, or an email address in a comment
				return true;
			}

			if (IsInCommentOrStringOnLine(line, typeStart))
			{
				return true;
			}

			char typeName[256];
			if (!Substring{ typeStart, at }.TryCopyWithoutTruncate(typeName, sizeof typeName))
			{
				return true;
			}

			char prefix[256];
			if (!Substring{ valueStart, activationPoint + 1 }.TryCopyWithoutTruncate(prefix, sizeof prefix))
			{
				return true;
			}

			PopulateImplicitNamespaces(doc);

			auto* e = GetDatabase().FindEnum(typeName);
			if (!e)
			{
				return true;
			}

			size_t nMatches = 0;
			bool exactMatch = false;

			for (size_t i = 0; i < e->NumberOfEntries(); i++)
			{
				cstr name;
				int64 value;
				e->GetEntry(i, OUT name, OUT value);

				if (StartsWith(name, prefix))
				{
					nMatches++;
					if (Eq(name, prefix))
					{
						exactMatch = true;
					}
				}
			}

			if (nMatches == 0 || (nMatches == 1 && exactMatch))
			{
				// Nothing to offer, or the value is already typed in full. The latter happens right after ReplaceSelectedText inserts a value, as it re-runs UpdateAutoComplete
				return true;
			}

			for (size_t i = 0; i < e->NumberOfEntries(); i++)
			{
				cstr name;
				int64 value;
				e->GetEntry(i, OUT name, OUT value);

				if (StartsWith(name, prefix))
				{
					editor.AutoCompleteBuilder().AddItem(name);
				}
			}

			autoComplete_Replacement_StartPosition = LinePointerToDocPosition(cursor, line, valueStart);

			editor.AutoCompleteBuilder().ShowAndClearItems();

			return true;
		}

		bool TryAddTokenOptionsToAutocomplete(ISexyEditor& editor, cr_substring candidate, int64 displacementFromCaret, cr_substring doc)
		{
			using namespace Rococo;

			if (!candidate)
			{
				return false;
			}
			else if (Rococo::Sex::Inference::IsSexyKeyword(candidate))
			{
				return false;
			}
			else if (*candidate.start == '#')
			{
				ShowAutocompleteDataForMacro(editor, candidate, displacementFromCaret, doc);
				return true;
			}
			else if (islower(*candidate.start))
			{
				ShowAutocompleteDataForVariable(editor, candidate, displacementFromCaret);
				return true;
			}
			else if (isupper(*candidate.start))
			{
				ShowAutocompleteDataForType(editor, candidate, doc);
				return true;
			}

			return false;
		}

		void ReplaceSelectedText(Rococo::AutoComplete::ISexyEditor& editor, cstr item)
		{
			int64 caretPos = editor.GetCaretPos();
			// The start may equal the caret when nothing has been typed after Day@, see TryAddQualifiedEnumsToAutocomplete. For other completions it is the token start, before the caret
			if (autoComplete_Replacement_StartPosition > 0 && autoComplete_Replacement_StartPosition <= caretPos)
			{
				editor.ReplaceText(autoComplete_Replacement_StartPosition, caretPos, item);
				UpdateAutoComplete(editor, nullptr);
			}
		}

		void SetHintToFunctionArguments(Rococo::AutoComplete::ISexyEditor&, const ISXYFunction& f, bool appendCloseParenthesis = true)
		{
			StackStringBuilder sb(callTipArgs, sizeof callTipArgs);
		
			for (int i = 0; i < f.InputCount(); ++i)
			{
				sb.AppendChar('(');

				EQualifier inputQualifier = f.InputQualifier(i);

				cstr qualifier;

				switch (inputQualifier)
				{
				default:
					qualifier = "";
					break;
				case EQualifier::Constant:
					qualifier = "const ";
					break;
				case EQualifier::Output:
					qualifier = "output ";
					break;
				case EQualifier::Ref:
					qualifier = "ref ";
					break;
				}
			
				sb << qualifier;

				sb << f.InputType(i);
				sb.AppendChar(' ');
				sb << f.InputName(i);
				sb.AppendChar(')');
			}

			sb << " -> ";

			for (int j = 0; j < f.OutputCount(); ++j)
			{
				sb.AppendChar('(');
				sb << f.OutputType(j);
				sb.AppendChar(' ');
				sb << f.OutputName(j);
				sb.AppendChar(')');
			}

			if (appendCloseParenthesis)
			{
				sb.AppendChar(')');
			}
		}

		const ISXYInterface* FindInterface(cr_substring type, const ISxyNamespace** ppNamespace = nullptr)
		{
			size_t len = type.Length() + 1;
			auto* buf = (char*)alloca(len);
			type.CopyWithTruncate(buf, len);
			return GetDatabase().FindInterface(buf, ppNamespace);
		}

		int interfaceDepth = 0;

		class InterfaceDepthCounter
		{
			SexyCompletionEngine& This;
		public:
			InterfaceDepthCounter(SexyCompletionEngine& ide): This(ide)
			{
				ide.interfaceDepth++;
			}
			~InterfaceDepthCounter()
			{
				This.interfaceDepth--;
			}
		};

		enum { MAX_INTERFACE_DEPTH = 64 };

		void FormatFactoryHint(cstr prefix, cstr suffix, char* buffer, size_t capacity, const ISXYFactory& factory, const ISxyNamespace* optionalNS)
		{
			StackStringBuilder sb(buffer, capacity);
			sb << prefix;

			if (optionalNS)
			{
				char fullname[256];
				GetFullNamespaceName(fullname, sizeof fullname, *optionalNS);
				sb << fullname;
				sb.AppendChar('.');
			}

			sb << factory.PublicName();

			for (int i = 0; i < factory.InputCount(); i++)
			{
				sb.AppendChar('(');
				sb << factory.InputType(i);
				sb.AppendChar(' ');
				sb << factory.InputName(i);
				sb.AppendChar(')');
			}
			sb << suffix;
		}

		bool TryShowCallTipForFactories(cr_substring type, cr_substring variableName, ISexyEditor& editor, ICalltip& calltip)
		{
			UNUSED(variableName);

			const ISxyNamespace* pNamespace = nullptr;
			auto* pInterface = FindInterface(type, &pNamespace);
			if (!pInterface)
			{
				return false;
			}

			char interfaceType[256];
			if (!type.TryCopyWithoutTruncate(interfaceType, sizeof interfaceType))
			{
				return false;
			}

			int candidates = 0;
			for (int i = 0; i < pNamespace->FactoryCount(); i++)
			{
				auto& f = pNamespace->GetFactory(i);

				char buf[256];
				f.GetDefinedInterface(buf, sizeof buf);

				if (EndsWith(buf, interfaceType))
				{
					candidates++;
				}
			}

			if (candidates == 1)
			{
				for (int i = 0; i < pNamespace->FactoryCount(); i++)
				{
					auto& f = pNamespace->GetFactory(i);

					char buf[256];
					f.GetDefinedInterface(buf, sizeof buf);

					if (EndsWith(buf, interfaceType))
					{
						char suggestion[256];
						FormatFactoryHint("(", "))", suggestion, sizeof  suggestion, f, pNamespace);
						editor.ShowCallTipAtCaretPos(suggestion);
						calltip.SetCalltipForReplacement(suggestion);
						break;
					}
				}
			}
			else if (candidates > 1)
			{
				char nsFullName[256];
				GetFullNamespaceName(nsFullName, sizeof nsFullName, *pNamespace);

				for (int i = 0; i < pNamespace->FactoryCount(); i++)
				{
					auto& f = pNamespace->GetFactory(i);

					char buf[256];
					f.GetDefinedInterface(buf, sizeof buf);

					if (EndsWith(buf, interfaceType))
					{
						char suggestion[256];
						StackStringBuilder sb(suggestion, sizeof suggestion);
						sb.AppendChar('(');		
						sb << nsFullName;
						sb.AppendChar('.');

						editor.AutoCompleteBuilder().AddItem(suggestion);
					}
				}
			}

			return true;
		}

		bool TryShowCallTipForMethods(cr_substring type, cr_substring methodName, ISexyEditor& editor)
		{
			InterfaceDepthCounter counter(*this);
	
			if (interfaceDepth > MAX_INTERFACE_DEPTH)
			{
				if (OS::IsDebugging())
				{
					OS::TripDebugger();
				}
				return false;
			}
	
			auto* pInterface = FindInterface(type);
			if (pInterface)
			{
				for (int i = 0; i < pInterface->MethodCount(); ++i)
				{
					auto& method = pInterface->GetMethod(i);
					auto sMethod = to_fstring(method.PublicName());
					if (Eq(sMethod, methodName))
					{
						SetHintToFunctionArguments(editor, method);
						editor.ShowCallTipAtCaretPos(callTipArgs);
						return true;
					}
				}

				auto* base = pInterface->Base();
				if (base)
				{
					bool result = TryShowCallTipForMethods(Substring::ToSubstring(base), methodName, editor);
					return result;
				}
			}

			return false;
		}

		bool TryGetFieldTypeOfType(char* fieldType, cstr type, cr_substring fieldName, cr_substring doc)
		{
			UNUSED(fieldType);
			UNUSED(type);
			UNUSED(fieldName);
			UNUSED(doc);
			if (OS::IsDebugging()) OS::TripDebugger();
			return false; // Not implemented
		}

		bool ValidateSubstringIsContainedWithin(cr_substring substring, cr_substring container)
		{
			if (substring.start < container.start || substring.start > container.finish)
			{
				if (OS::IsDebugging()) OS::TripDebugger();
				return false;
			}

			if (substring.finish < container.start || substring.finish > container.finish)
			{
				if (OS::IsDebugging()) OS::TripDebugger();
				return false;
			}
		}

		// Retrieves the searchTerm as it appears in the document
		Substring GetSearchTermInDoc(ISexyEditor& editor, cr_substring searchTerm, cr_substring doc)
		{
			cstr docCaretPos = doc.start + editor.GetCaretPos();
			int64 displacementFromCaret = searchTerm.Length();
			cstr start = docCaretPos - displacementFromCaret;
			cstr end = start + searchTerm.Length();
			return { start, end };
		}

		Substring GetTypeForMember(cr_substring type, cr_substring member, cr_substring doc)
		{
			struct: ISexyFieldEnumerator
			{
				Substring member{ Substring::Null() };
				Substring fieldType { Substring::Null() };

				void OnFieldType(cr_substring fieldType, cr_substring searchRoot) override
				{
					if (Eq(searchRoot, member))
					{
						this->fieldType = fieldType;
					}
				}

				void OnField(cstr fieldName, cr_substring memberSearch) override
				{
					UNUSED(fieldName);
					UNUSED(memberSearch);
				}

				void OnHintFound(cr_substring hint) override
				{
					UNUSED(hint);
				}

			} searchCallback;
			searchCallback.member = member;

			Rococo::Sex::Inference::EnumerateLocalFields(searchCallback, member, type, doc);

			return searchCallback.fieldType;
		}

		Substring GetSubType(cr_substring type, cstr subMember, cr_substring candidateInDoc, cr_substring doc)
		{		
			UNUSED(subMember);
			cstr nextDot = ForwardFind('.', candidateInDoc);
			if (!nextDot)
			{
				return Substring::Null();
			}

			Substring member = { candidateInDoc.start, nextDot };

			return GetTypeForMember(type, member, doc);
		}
	
		static bool IsWhitespace(char c)
		{
			switch (c)
			{
			case '\t':
			case ' ':
			case '\r':
			case '\n':
				return true;
			default:
				return false;
			}
		}

		bool TryFindAndShowCallTipForFactories(ISexyEditor& editor, cr_substring searchToken, cr_substring doc, ICalltip& calltip)
		{
			UNUSED(doc);

			if (!IsWhitespace(searchToken.finish[-1]))
			{
				return false;
			}

			// We are searching for the pattern '(<interface> <variable-name> '
			cstr lastOpenBrace = Rococo::Strings::ReverseFind('(', searchToken);
			if (lastOpenBrace == nullptr)
			{
				return false;
			}

			cstr lastCloseBrace = Rococo::Strings::ReverseFind(')', searchToken);
			if (lastCloseBrace != nullptr && lastCloseBrace > lastOpenBrace)
			{
				return false;
			}

			// We now have found '( .... '

			cstr interfaceName = nullptr;

			for (cstr p = lastOpenBrace + 1; p < searchToken.finish; p++)
			{
				if (IsWhitespace(*p))
				{
					continue;
				}

				if (!IsCapital(*p))
				{
					// Unknown character, so this cannot be a plain interface name
					return false;
				}

				interfaceName = p;
				break;
			}

			if (!interfaceName)
			{
				return false;
			}

			Substring interfaceNameAndArg = { interfaceName, searchToken.finish };

			cstr interfaceNameEnd = nullptr;
			for (cstr p = interfaceNameAndArg.start + 1; p < searchToken.finish; p++)
			{
				if (IsWhitespace(*p))
				{
					interfaceNameEnd = p;
					break;
				}

				if (IsAlphaNumeric(*p))
				{
					// Still in the name
					continue;
				}

				if (*p == '.')
				{
					continue;
				}

				// Something unexpected
				return false;
			}

			if (!interfaceNameEnd)
			{
				return false;
			}

			cstr variableArg = SkipBlankspace({ interfaceNameEnd, interfaceNameAndArg.finish });
			if (!variableArg)
			{
				return false;
			}

			// variables begin with lower case letters a-z
			if (!islower(*variableArg))
			{
				return false;
			}

			cstr variableArgFinish = nullptr;
			for (cstr p = variableArg; p < interfaceNameAndArg.finish; p++)
			{
				if (IsWhitespace(*p))
				{
					variableArgFinish = p;
					break;
				}

				if (IsAlphaNumeric(*p))
				{
					// Still in the name
					continue;
				}

				// Something unexpected
				return false;
			}

			if (!variableArgFinish)
			{
				return false;
			}

			Substring interfaceToken { interfaceNameAndArg.start, interfaceNameEnd };
			Substring variableToken { variableArg, variableArgFinish }; 

			return TryShowCallTipForFactories(interfaceToken, variableToken, editor, calltip);
		}

		bool TryFindAndShowCallTipForMethods(ISexyEditor& editor, cr_substring searchToken, cr_substring doc)
		{
			// Potentially a method call
			cstr methodSeparator = Rococo::Strings::ReverseFind('.', searchToken);
			if (!methodSeparator || *methodSeparator != '.')
			{
				return false;
			}

			if (methodSeparator >= searchToken.finish || !isupper(methodSeparator[1]))
			{
				return false;
			}

			// Potential method name, with left of separator being the interface variable

			Substring candidateInDoc = GetSearchTermInDoc(editor, searchToken, doc);

			Substring methodName{ methodSeparator + 1, searchToken.finish - 1 };

			Rococo::Sex::Inference::TypeInference type;
			bool isThis;
			type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(isThis, candidateInDoc, doc);

			if (type.declarationType)
			{
				Substring branchType = type.declarationType;
				Substring subsearch = searchToken;

				for (;;)
				{
					cstr firstDot = ForwardFind('.', subsearch);
					if (firstDot == methodSeparator)
					{
						return TryShowCallTipForMethods(branchType, methodName, editor);
					}
					else
					{
						subsearch = { firstDot + 1, subsearch.finish };
						Substring subType = GetSubType(branchType, firstDot + 1, subsearch, doc);
						if (!subType)
						{
							return false;
						}

						branchType = subType;
					}
				}
			}

			return false;
		}

		Substring CachedDoc(ISexyEditor& editor)
		{
			int64 nCharsAndNull = editor.GetDocLength();
			src_buffer.resize(nCharsAndNull + 1);
			editor.GetText(nCharsAndNull, src_buffer.data());
			src_buffer[nCharsAndNull] = 0;

			Substring doc;
			doc.start = src_buffer.data();
			doc.finish = doc.start + nCharsAndNull;

			return doc;
		}

		Substring GetCurrentLine(ISexyEditor& editor)
		{
			EditorLine currentLine(src_line, sizeof src_line);
			if (!editor.TryGetCurrentLine(currentLine))
			{
				return Substring{nullptr,nullptr};
			}

			return Substring { src_line, src_line + strlen(src_line) };
		}

		Substring GetSearchTokenWithinLine(cr_substring editorLine, const EditorCursor& cursor, cstr& activationPoint)
		{
			int64 caretColumn = cursor.CaretColumnNumber();
			if (caretColumn == 0)
			{
				return Substring::Null();
			}

			activationPoint = editorLine.start + caretColumn - 1;

			cstr openingToken = Rococo::Sex::Inference::GetFirstNonTokenPointerFromRight(editorLine, activationPoint);
			if (openingToken == nullptr)
			{
				openingToken = editorLine.start;
			}
			else
			{
				openingToken++; // this takes us into the alphanumeric string
			}

			Substring searchToken = Rococo::Sex::Inference::GetFirstTokenFromLeft({ openingToken, activationPoint + 1 });
			if (!searchToken)
			{
				return Substring::Null();
			}
			return { searchToken.start, activationPoint + 1 };
		}

		int64 LinePointerToDocPosition(const EditorCursor& cursor, cr_substring line, cstr linePointer)
		{
			return linePointer - line.start + cursor.lineStartPosition;
		}

		WideFilePath fullPathCache;

		static bool IsBlank(cr_substring sample)
		{
			for (cstr p = sample.start + 1; p < sample.finish; p++)
			{
				if (!isblank(*p))
				{
					return false;
				}
			}

			return true;
		}

		template<class LAMBDA>
		static void ForEachCompoundDirectiveWithAtLeastOneArg(const fstring fdirective, cr_substring sample, LAMBDA lambda)
		{
			Substring cursor = sample;

			while (true)
			{
				cstr directive = Strings::FindSubstring(cursor, fdirective);
				if (!directive)
				{
					break;
				}

				if (directive + fdirective.length >= sample.finish)
				{
					break;
				}

				Substring preUsing{ cursor.start, directive };
				cstr openParenthesis = Strings::ReverseFind('(', preUsing);

				cursor.start += fdirective.length + 2;

				if (openParenthesis)
				{
					Substring innerPadding{ openParenthesis + 1, directive };
					if (IsBlank(innerPadding))
					{
						// We have matched (<blankspace> <directive>...)

						if (isblank(directive[fdirective.length]))
						{
							// Our directive has blankspace after it: (<blankspace> <directive> <blankspace> ...)

							cstr nextArg = Strings::SkipBlankspace({ directive + fdirective.length, sample.finish });
							if (nextArg)
							{
								for (cstr p = nextArg + 1; p < sample.finish; p++)
								{
									if (isblank(*p) || *p == ')')
									{
										// We matched (<directive> <arg>)
										Substring arg{ nextArg, p };
										lambda(arg);

										cursor.start = p + 1;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		void PopulateImplicitNamespaces(cr_substring doc)
		{
			auto* implicits = GetDatabase().GetRootNamespace().ImplicitNamespaces();
			implicits->ClearImplicitNamespaces();

			ForEachCompoundDirectiveWithAtLeastOneArg("using"_fstring, doc, [this, &implicits](cr_substring fqNamespaceArg)
				{
					char nsBuffer[NAMESPACE_MAX_LENGTH];
					if (fqNamespaceArg.TryCopyWithoutTruncate(nsBuffer, sizeof nsBuffer))
					{
						bool unused = implicits->AddImplicitNamespace(nsBuffer);
						UNUSED(unused);
					}
				}
			);
		}

		std::vector<char> docTemp;

		int GetLineNumber(cstr doc, cstr target)
		{
			int lineNumber = 1;
			for (cstr p = doc; p < target; p++)
			{
				if (*p == '\n')
				{
					lineNumber++;
				}
			}
			return lineNumber;
		}

		void GotoDefinitionOfLowerCaseItem(cr_substring substringDoc, cstr doc, ISexyEditor& editor)
		{
			Sex::Inference::FaultTolerantSexyTypeInferenceEngine engine(doc);
			auto inference = engine.InferLocalVariableVariableType(substringDoc);
	
			if (inference.declarationType && inference.declarationVariable)
			{
				char type[256];
				inference.declarationType.CopyWithTruncate(type, sizeof type);

				char var[256];
				inference.declarationVariable.CopyWithTruncate(var, sizeof var);

				int lineNumber = GetLineNumber(doc, inference.declarationType.start);

				char docToken[256];
				substringDoc.CopyWithTruncate(docToken, sizeof docToken);

				editor.GotoDefinition(docToken, "<this>", lineNumber);
			}
		}

		void GotoDefinitionOfUpperCaseItem(cr_substring substringDoc, ISexyEditor& editor)
		{
			if (!substringDoc)
			{
				editor.GotoDefinition("?", "Huh?", 1);
				return;
			}

			char docToken[256];
			substringDoc.CopyWithTruncate(docToken, sizeof docToken);

			char fqName[256];
			if (!substringDoc.TryCopyWithoutTruncate(fqName, sizeof fqName))
			{
				return;
			}

			auto* type = GetDatabase().FindType(fqName);
			if (type)
			{
				auto* localType = type->LocalType();
				if (localType)
				{
					editor.GotoDefinition(docToken, localType->SourcePath(), localType->LineNumber());
				}

				return;
			}

			auto* pInterface = GetDatabase().FindInterface(fqName);
			if (pInterface)
			{
				editor.GotoDefinition(docToken, pInterface->SourcePath(), pInterface->GetDefinition().Start().y);
				return;
			}

			auto* f = GetDatabase().FindFunction(fqName);
			if (f)
			{
				auto* localFunction = f->LocalFunction();
				if (localFunction)
				{
					editor.GotoDefinition(docToken, localFunction->SourcePath(), localFunction->LineNumber());
					return;
				}
			}
		}

		void ExpandSearchTokenToRight(REF Substring& searchToken, cr_substring doc)
		{
			cstr p = searchToken.end();
			for (;p < doc.end(); p++)
			{
				if (IsAlphaNumeric(*p))
				{
					continue;
				}	
				else
				{
					// Blankspace or some other break such as / or .
					break;
				}
			}

			searchToken.finish = p;
		}

		Substring GetDocSubstringAtCaret(ISexyEditor& editor)
		{
			Substring substringLine = GetCurrentLine(editor);
			if (!substringLine)
			{
				return Substring::Null();
			}

			EditorCursor cursor;
			editor.GetCursor(cursor);

			Substring doc = CachedDoc(editor);

			cstr activationPoint;
			Substring searchToken = GetSearchTokenWithinLine(substringLine, cursor, activationPoint);

			if (!searchToken)
			{
				return Substring::Null();
			}

			int64 docPos = LinePointerToDocPosition(cursor, substringLine, searchToken.start);
			if (docPos < 0)
			{
				return Substring::Null();
			}

			size_t len = Length(searchToken);
			Substring s{ doc.start + docPos, doc.start + docPos + len };
			return s;
		}
	
		void GotoDefinitionOfSelectedToken(ISexyEditor& editor) override
		{		
			Substring substringDoc = GetDocSubstringAtCaret(editor);
			if (!substringDoc)
			{
				return;
			}
			ExpandSearchTokenToRight(REF substringDoc, CachedDoc(editor));
		
			if (!substringDoc)
			{
				editor.GotoDefinition("?", "Huh?", 1);
				return;
			}

			if (Eq("this", substringDoc))
			{
				// TODO - send us to the class
			}
			else if (islower(*substringDoc.start))
			{
				Substring doc = CachedDoc(editor);
				// token could be a keyword or variable identifier
				GotoDefinitionOfLowerCaseItem(substringDoc, doc.start, editor);
			}
			else if (isupper(*substringDoc.start))
			{
				// token could be a type, namespace or function name
				GotoDefinitionOfUpperCaseItem(substringDoc, editor);
			}
			else
			{
				editor.GotoDefinition("?", "Ayup?", 1);
			}
		}

		void UpdateAutoComplete(ISexyEditor& editor, crwstr fullPath) override
		{
			if (fullPath)
			{
				if (!Eq(fullPathCache, fullPath))
				{
					Format(fullPathCache, L"%s", fullPath);
				
					try
					{
						workspace.Rebase(fullPathCache);

						// The IDE hides its frame here, as it always did on a new file path. Other hosts may simply record it
						host.OnEditorFileChanged(fullPathCache);
					}
					catch (IException& ex)
					{
						// The IDE shows "SexyStudio threw an exception during autocomplete" here, as it always did
						host.OnEngineError(ex);
						return;
					}
				}
			}

			Substring substringLine = GetCurrentLine(editor);
			if (!substringLine)
			{
				return;
			}
	
			EditorCursor cursor;
			editor.GetCursor(cursor);

			Substring doc = CachedDoc(editor);

			cstr activationPoint;
			Substring searchToken = GetSearchTokenWithinLine(substringLine, cursor, activationPoint);

			if (!searchToken)
			{
				return;
			}

			char activationChar = *activationPoint;

			autoComplete_Replacement_StartPosition = LinePointerToDocPosition(cursor, substringLine, searchToken.start);

			// Day@ or Sys.Time.Day@, optionally followed by part of a value name. This is the only place ' starts a completion
			if (TryAddQualifiedEnumsToAutocomplete(editor, substringLine, activationPoint, cursor, doc))
			{
				return;
			}

			if (activationChar == '.' && activationPoint > substringLine.start + 1 && isblank(activationPoint[-1]))
			{
				int64 displacementFromCaret = activationPoint - searchToken.start + 1;
				// We may have something like this: 'day = .', which is an assignment to something of type day that looks to be an enumeration around type day. 
				// We want to list .Tuesday et al.
				TryAddEnumsToAutocomplete(editor, searchToken, displacementFromCaret, doc);
			}
			else if 	// If blinking caret follows period or alphanumeric such as: Sys._ or Sys_, then we want to complete the dot.
				(IsAlphaNumeric(activationChar) || activationChar == '.' || (isblank(activationChar) && *searchToken.start == '#'))
			{
				int64 displacementFromCaret = activationPoint - searchToken.start + 1;

				PopulateImplicitNamespaces(doc);

				if (!TryAddTokenOptionsToAutocomplete(editor, searchToken, displacementFromCaret, doc))
				{
					autoComplete_Replacement_StartPosition = 0;
				}
			}
			else if (activationChar == ' ' || activationChar == '\t')
			{
				// Potentially we have a method or function call followed by a space, which is a prompt to show the function arguments

				if (activationPoint > substringLine.start && IsAlphaNumeric(activationPoint[-1]))
				{
					if (isupper(*searchToken.start))
					{
						PopulateImplicitNamespaces(doc);

						// Potentially a function call
						ShowFunctionArgumentsForType(editor, searchToken);	
					}
					else if (islower(*searchToken.start))
					{
						PopulateImplicitNamespaces(doc);

						if (!TryFindAndShowCallTipForMethods(editor, searchToken, doc))
						{
							TryFindAndShowCallTipForFactories(editor, substringLine, doc, *this);
						}
					}
				}
			}
		}

		// Buffer should be 1024 bytes
		void GetHintForCandidate(cr_substring prefix, char args[1024]) override
		{
			GetDatabase().GetHintForCandidate(prefix, args);
		}
	};
}

namespace Rococo::SexyStudio
{
	SEXYSTUDIO_API ISexyCompletionEngine* CreateSexyCompletionEngine(ISexyWorkspace& workspace, IEngineHost& host)
	{
		return new SexyCompletionEngine(workspace, host);
	}
}
