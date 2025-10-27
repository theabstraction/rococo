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

	
#ifdef char_IS_WIDE
# define SexyCreateFile CreateFileW
# define SexyFormatMessage FormatMessageW
#else
# define SexyCreateFile CreateFileA
# define SexyFormatMessage FormatMessageA
#endif

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void FormatSysMessage(char* text, size_t capacity, int msgNumber)
	{
		if (!SexyFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, msgNumber, 0, text, (DWORD) capacity, NULL))
		{
         SafeFormat(text, capacity, ("Code %d ( 0x%x )"), msgNumber, msgNumber);
		}
	}

	void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += WriteToStandardOutput((" %s"), (cstr) s.c_str());
			break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += WriteToStandardOutput((" \"%s\""), (cstr) s.c_str());
			break;
		case EXPRESSION_TYPE_COMPOUND:
			
			totalOutput += WriteToStandardOutput((" ("));

			for(int i = 0; i < s.NumberOfElements(); ++i)
			{
				if (totalOutput > maxOutput) 
				{
					return;
				}
				
				cr_sex child = s.GetElement(i);
				PrintExpression(child, totalOutput, maxOutput);								
			}
			
			totalOutput += WriteToStandardOutput((" )"));
		}
	}

	void PrintParseException(const ParseException& e)
	{
		WriteToStandardOutput(("Parse error\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\nSpecimen: %s\r\n"), e.Name(), e.Start().x, e.Start().y, e.End().x, e.End().y, e.Message(), e.Specimen());

		int depth = 0;
		for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
		{
			if (depth++ > 0) WriteToStandardOutput("Macro expansion %d:\r\n", depth);

			int totalOutput = 0;
			PrintExpression(*s, totalOutput, 1024);

			if (totalOutput > 1024) WriteToStandardOutput(("..."));

			WriteToStandardOutput(("\r\n"));
		}
	}

	struct CLogger: public ILog
	{
		CLogger()
		{
		}

		~CLogger()
		{
		}

		bool TryGetNextException(OUT ParseException& ex)
		{
			if (exceptions.empty()) return false;

			ex = exceptions.back();
			exceptions.pop_back();

			return true;
		}

		int ExceptionCount() const
		{
			return (int32) exceptions.size();
		}

		void Write(cstr text)
		{
			WriteToStandardOutput(text);
		}

		void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) 
		{
			WriteToStandardOutput(("%s: code (0x%x) %d\nMessage: %s\n"), exceptionType, errorCode, errorCode, message);
		}

		void OnJITCompileException(Sex::ParseException& ex)
		{
			PrintParseException(ex);
			// exceptions.push_back(ex);
		}

		typedef std::vector<ParseException> TExceptions;
		TExceptions exceptions;
	};

	void Abort()
	{
		if (IsDebuggerPresent())
			__debugbreak();
		else
			exit(-1); 
	}

	void PrintStructures(IScriptSystem& ss, ILog& log)
	{
		IPublicProgramObject& obj = ss.PublicProgramObject();
		for(int i = 0; i < obj.ModuleCount(); ++i)
		{
			const IModule& m = obj.GetModule(i);

			char msg[256];
         SafeFormat(msg, 256, ("\r\nModule %s"), m.Name());
			log.Write(msg);

			for(int j = 0; j < m.StructCount(); ++j)
			{
				const IStructure& s = m.GetStructure(j);

            SafeFormat(msg, 256, ("\r\nstruct %s - %d bytes"), s.Name(), s.SizeOfStruct());
				log.Write(msg);

				for(int k = 0; k < s.MemberCount(); ++k)
				{
					const IMember& member = s.GetMember(k);
               SafeFormat(msg, 256, ("  %s %s"), member.UnderlyingType()->Name(), member.Name());
					log.Write(msg);
				}
			}
		}
	}	
}