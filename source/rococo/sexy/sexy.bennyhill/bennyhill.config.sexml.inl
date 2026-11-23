#pragma once

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

// The SEXML config front end of BennyHill. It reads config files (*.xc.sexml) with the rococo.sexml library and builds the same model as the xc
// front end, using the shared config builders in bennyhill.config.inl. Each directive corresponds to one xc config item.
// The schema is described in README.sexml.txt, next to this file. The generator (sxh) files are unaffected.

#include <rococo.sexml.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Sex::SEXML;
	using namespace Rococo::Strings;

	// Throws if the directive has an attribute that is not named in [known]
	void ValidateAttributes(cr_sdir directive, std::initializer_list<cstr> known)
	{
		for (size_t i = 0; i < directive.NumberOfAttributes(); i++)
		{
			auto& a = directive.GetAttributeByIndex(i);

			bool isKnown = false;
			for (cstr name : known)
			{
				if (Eq(name, a.Name()))
				{
					isKnown = true;
					break;
				}
			}

			if (!isKnown)
			{
				char knownList[256] = { 0 };
				StackStringBuilder sb(knownList, sizeof knownList);
				for (cstr name : known)
				{
					sb << " " << name;
				}

				Throw(a.S(), "Unknown attribute (%s ...) in (%s ...). Known attributes:%s", a.Name(), directive.FQName(), knownList);
			}
		}
	}

	void ValidateNoChildren(cr_sdir directive)
	{
		if (directive.NumberOfChildren() > 0)
		{
			Throw(directive.GetChild(0).S(), "(%s ...) does not take sub-directives", directive.FQName());
		}
	}

	// Returns the expression of an atomic or string literal attribute value, or nullptr if the attribute is not found
	const ISExpression* FindValue(cr_sdir directive, cstr name)
	{
		auto* a = directive.FindAttributeByName(name);
		if (!a)
		{
			return nullptr;
		}

		auto type = a->Value().Type();
		if (type != SEXMLValueType::Atomic && type != SEXMLValueType::StringLiteral)
		{
			Throw(a->S(), "Expecting (%s <atomic-or-string-literal>)", name);
		}

		return &a->Value().S();
	}

	// Returns the expression of an atomic or string literal attribute value, throwing if the attribute is not found
	cr_sex GetValue(cr_sdir directive, cstr name)
	{
		auto* s = FindValue(directive, name);
		if (!s)
		{
			Throw(directive.S(), "(%s ...) requires the attribute (%s <value>)", directive.FQName(), name);
		}

		return *s;
	}

	// Returns the compound expression of the raw attribute (' <name> <compound-expression>), throwing if the attribute is not found
	cr_sex GetRaw(cr_sdir directive, cstr name)
	{
		auto* a = directive.FindAttributeByName(name);
		if (!a)
		{
			Throw(directive.S(), "(%s ...) requires the raw attribute (' %s (...))", directive.FQName(), name);
		}

		if (a->Value().Type() != SEXMLValueType::Raw || a->S().NumberOfElements() != 3 || !IsCompound(a->S()[2]))
		{
			Throw(a->S(), "Expecting the raw attribute (' %s (...))", name);
		}

		return a->S()[2];
	}

	// Validates (<Directive> (Name <sxh-type>) (SexyType <sxy-type>) (CppType <cpp-type>)), the form of Primitive, Struct and Interface
	void ValidateTypeMapping(cr_sdir directive)
	{
		ValidateAttributes(directive, { "Name", "SexyType", "CppType" });
		ValidateNoChildren(directive);
	}

	// Parses a config file (*.xc.sexml). Each directive corresponds to one xc config item, and the directives are processed in order as in xc:
	// (CppRoot (Path <cpp-root-directory>))
	// (CppException (Type <cpp-exception-type>))
	// (TypeFiles (SexyTypes <sexy-types-path>) (CppTypes <cpp-types-path>))
	// (Primitive (Name <sxh-type>) (SexyType <sxy-type>) (CppType <cpp-type>))
	// (Struct (Name <sxh-type>) (SexyType <sxy-type>) (CppType <cpp-type>))
	// (Interface (Name <sxh-type>) (SexyType <sxy-type>) (CppType <cpp-type>))
	// (DefStruct (Name <sxh-type>) (SexyType <sxy-type>) (CppType <cpp-type>) (' Fields ((<field-type1> <field-name1>) ... (<field-typeN> <field-nameN>))))
	// DefStruct directives are processed after all other directives, as are xc defstruct items
	void ParseSEXMLConfig(cr_sex configRoot, ParseContext& pc)
	{
		AutoFree<ISEXMLRootSupervisor> sexml = CreateSEXMLParser(Rococo::Memory::CheckedAllocator(), configRoot);

		int nStructDefs = 0;

		for (size_t i = 0; i < sexml->NumberOfDirectives(); i++)
		{
			cr_sdir directive = (*sexml)[i];
			cstr name = directive.FQName();
			cr_sex sDirective = directive.S();

			if (Eq(name, "CppRoot"))
			{
				ValidateAttributes(directive, { "Path" });
				ValidateNoChildren(directive);
				if (*pc.cppRootDirectory != 0) Throw(sDirective, ("cpp.root has already been specified"));
				SetCppRoot(GetValue(directive, "Path"), pc);
			}
			else if (Eq(name, "CppException"))
			{
				ValidateAttributes(directive, { "Type" });
				ValidateNoChildren(directive);
				if (*pc.cppException != 0) Throw(sDirective, ("cpp.exception has already been specified"));
				SetCppException(GetValue(directive, "Type"), pc);
			}
			else if (Eq(name, "TypeFiles"))
			{
				ValidateAttributes(directive, { "SexyTypes", "CppTypes" });
				ValidateNoChildren(directive);
				ValidateTypeFilesCanBeSet(sDirective, pc);
				SetTypeFiles(GetValue(directive, "SexyTypes"), GetValue(directive, "CppTypes"), pc);
			}
			else if (Eq(name, "Primitive"))
			{
				ValidateTypeMapping(directive);
				AddPrimitive(sDirective, GetValue(directive, "Name"), GetValue(directive, "SexyType"), GetValue(directive, "CppType"), pc);
			}
			else if (Eq(name, "Struct"))
			{
				ValidateTypeMapping(directive);
				AddStruct(sDirective, GetValue(directive, "Name"), GetValue(directive, "SexyType"), GetValue(directive, "CppType"), pc);
			}
			else if (Eq(name, "Interface"))
			{
				ValidateTypeMapping(directive);
				AddInterfaceDecl(sDirective, GetValue(directive, "Name"), GetValue(directive, "SexyType"), GetValue(directive, "CppType"), pc);
			}
			else if (Eq(name, "DefStruct"))
			{
				ValidateAttributes(directive, { "Name", "SexyType", "CppType", "Fields" });
				ValidateNoChildren(directive);
				nStructDefs++;
			}
			else
			{
				Throw(sDirective, "Unknown directive (%s ...). Expecting one of CppRoot, CppException, TypeFiles, Primitive, Struct, Interface or DefStruct", name);
			}
		}

		if (nStructDefs > 0)
		{
			ValidateTypeFilesForDefStructs(configRoot, pc);

			FileAppender cppTypeAppender(pc.cppTypesFilename);
			FileAppender sexyTypeAppender(pc.sexyTypesFilename);

			AppendTypeFileSignature(cppTypeAppender);

			for (size_t i = 0; i < sexml->NumberOfDirectives(); i++)
			{
				cr_sdir directive = (*sexml)[i];
				if (!Eq(directive.FQName(), "DefStruct"))
				{
					continue;
				}

				cr_sex sDirective = directive.S();
				ValidateDefStructCanBeAdded(sDirective, pc);

				cr_sex sName = GetValue(directive, "Name");
				cr_sex sSexyType = GetValue(directive, "SexyType");
				cr_sex sCppType = GetValue(directive, "CppType");
				if (!IsAtomic(sName) || !IsAtomic(sSexyType) || !IsAtomic(sCppType))
				{
					Throw(sDirective, "Expecting atomic values for Name, SexyType and CppType");
				}

				cr_sex sFields = GetRaw(directive, "Fields");
				if (sFields.NumberOfElements() == 0)
				{
					Throw(sDirective, "Expecting at least one field: (' Fields ((<field-type1> <field-name1>) ... (<field-typeN> <field-nameN>)))");
				}

				AddDefStruct(sDirective, sName, sSexyType, sCppType, sFields, 0, pc, cppTypeAppender, sexyTypeAppender);
			}
		}

		if (*pc.cppRootDirectory == 0) Throw(configRoot, ("cpp.root was not specified"));
	}
}
