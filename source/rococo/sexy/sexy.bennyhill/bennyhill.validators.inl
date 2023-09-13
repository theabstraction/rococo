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

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;

	void ValidateSexyType(cr_sex s, cstr name)
	{
		if (!IsCapital(name[0]))
		{
			Throw(s, ("Expecting a capital letter in the first position"));
		}

		for(const char* p = name + 1; *p != 0; p++)
		{
			if (*p == '.')
			{
				if (p[1] == '.' || p[1] == 0) Throw(s, ("All namespace separators must be surrounded by alphanumerics"));
			}
			else if (!IsAlphaNumeric(*p))
			{
				Throw(s, ("Expecting an alphanumeric character in each element of the string after the first"));
			}
		}
	}

	void ValidateFQSexyFunction(cr_sex s, cstr name)
	{
		// Types have the same rules as functions
		ValidateSexyType(s, name);
	}

	void ValidateSexyVariable(cr_sex s, cstr name)
	{
		if (!Rococo::IsLowerCase(name[0]))
		{
			Throw(s, ("Expecting a lower case letter in the first position"));
		}

		for(const char* p = name; *p != 0; p++)
		{
			if (!IsAlphaNumeric(*p))
			{
				Throw(s, ("Expecting an alphanumeric in character is each element of the string after the first"));
			}
		}

		if (AreEqual(name, ("hObject"))) Throw(s, ("hObject is a reserved variable name"));
		if (AreEqual(name, ("this"))) Throw(s, ("this is a reserved keyword"));
	}

	void ValidateSexyType(cr_sex s)
	{
		if (!IsAtomic(s) && !IsStringLiteral(s)) Throw(s, ("Expecting sexy type name"));
		ValidateSexyType(s, s.c_str());
	}

	void ValidateSexyVariable(cr_sex s)
	{
		if (!IsAtomic(s) && !IsStringLiteral(s)) Throw(s, ("Expecting sexy variable name"));
		ValidateSexyVariable(s, s.c_str());
	}

	void ValidateCPPType(cr_sex s, cstr name)
	{
		if (!IsAlphaNumeric(*name) && *name != '_')
		{
			Throw(s, ("Expecting an alphanumeric or underscore character in first element of the string"));
		}

		for(const char* p = name+1; *p != 0; p++)
		{
			if (!IsAlphaNumeric(*p) && *p != '_' && *p != '.')
			{
				Throw(s, ("Expecting an alphanumeric or underscore character in each element of the string after the first"));
			}

			if (*p == '.' && (p[1] == '.' || p[1] == 0)) Throw(s, ("Expecting alphanumeric or underscore character after namespace separator"));
		}
	}

	void ValidateNamespace(cr_sex s, cstr ns)
	{
		NamespaceSplitter splitter(ns);

		cstr head, tail;
		if (splitter.SplitTail(head, tail))
		{
			ValidateSexyType(s, tail);
			ValidateNamespace(s, head);
		}
		else
		{
			ValidateSexyType(s, ns);
		}
	}

	void ValidateFQSexyInterface(cr_sex s)
	{
		if (!IsAtomic(s)) Throw(s, ("Expecting fully qualified sexy interface name as atomic token"));
		cstr name = s.c_str();

		NamespaceSplitter splitter(name);

		cstr ns, interf;
		if (!splitter.SplitTail(ns, interf))
		{
			Throw(s, ("Expecting fully qualified sexy interface name as atomic token. No namespace delimited by '.' was found"));
		}

		ValidateSexyType(s, interf);
		ValidateNamespace(s, ns);
	}

	void ValidateCPPNamespace(cr_sex s, cstr ns)
	{
		NamespaceSplitter splitter(ns);

		cstr head, tail;
		if (splitter.SplitTail(head, tail))
		{
			ValidateCPPType(s, tail);
			ValidateCPPNamespace(s, head);
		}
		else
		{
			ValidateCPPType(s, ns);
		}
	}

	void ValidateFQCppStruct(cr_sex s)
	{
		if (!IsAtomic(s)) Throw(s, ("Expecting fully qualified struct name as atomic token"));
		cstr name = s.c_str();

		NamespaceSplitter splitter(name);

		cstr ns, interf;
		if (!splitter.SplitTail(ns, interf))
		{
			ValidateCPPType(s, name);
		}
		else
		{
			ValidateCPPType(s, interf);
			ValidateCPPNamespace(s, ns);
		}
	}

	void ValidateFileSpec(cr_sex directive)
	{
		if (directive.NumberOfElements() == 2)
		{
			cr_sex fullpath = directive.GetElement(1);
			if (!IsAtomic(fullpath) && !IsStringLiteral(fullpath))
			{
				Throw(fullpath, ("Expecting atomic or string literal path definition"));
			}
		}
		else if (directive.NumberOfElements() == 3)
		{
			cr_sex pathRoot = directive.GetElement(1);
			if (!IsAtomic(pathRoot) && !IsStringLiteral(pathRoot))
			{
				Throw(pathRoot, ("Expecting atomic or string literal in path root definition"));
			}

			cr_sex filename = directive.GetElement(2);
			if (!IsAtomic(filename) && !IsStringLiteral(filename))
			{
				Throw(filename, ("Expecting atomic or string literal in filename definition"));
			}
		}
		else
		{
			Throw(directive, ("Either 2 or 3 elements must appear in a file spec directive"));
		}
	}
}