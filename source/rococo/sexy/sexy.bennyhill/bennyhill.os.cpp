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

#include "bennyhill.stdafx.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <unordered_set>

#include "sexy.stdstrings.h"

#include "bennyhill.h"

#include <rococo.api.h>
#include <rococo.os.h>

using namespace Rococo::Strings;

namespace Rococo
{
	FileAppender::FileAppender(cstr _filename) : filename(_filename)
	{
		static std::unordered_set<stdstring> deletedFiles;
		if (deletedFiles.find(_filename) == deletedFiles.end())
		{
			_unlink(_filename);
			deletedFiles.insert(_filename);
		}

		hFile = CreateFileA(_filename, GENERIC_WRITE | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE || hFile == NULL)
		{
			Throw(GetLastError(), "Error opening %s for append", _filename);
		}

		SetFilePointer(hFile, 0, NULL, FILE_END);
	}

	FileAppender::~FileAppender()
	{
		CloseHandle(hFile);
	}

	void FileAppender::Append(cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char buf[256];
		int nChars = SafeVFormat(buf, sizeof(buf), format, args);

		char abuf[256];
		for (int i = 0; i < nChars; ++i)
		{
			abuf[i] = buf[i] > 255 ? '?' : buf[i];
		}

		DWORD nBytesWritten = 0;
		if (!WriteFile(hFile, abuf, nChars, &nBytesWritten, NULL))
		{
			Throw(GetLastError(), "Error writing %s for append", filename);
		}

		if (nBytesWritten < (DWORD)nChars)
		{
			Throw(GetLastError(), "Error writing %u bytes to %s for append", nBytesWritten, filename);
		}
	}

	void FileAppender::Append(char c)
	{
		DWORD nBytesWritten = 0;
		if (!WriteFile(hFile, &c, 1, &nBytesWritten, NULL))
		{
			Throw(GetLastError(), "Error writing %s for append", filename);
		}

		if (nBytesWritten != 1)
		{
			Throw(GetLastError(), "Error writing 1 byte to %s for append", filename);
		}
	}

	void FileAppender::WriteString(cstr text)
	{
		Append("%s", text);
	}

	void FileAppender::AppendSequence(int count, char c)
	{
		for (int i = 0; i < count; ++i)
		{
			Append(c);
		}
	}

	void WriteStandardErrorCode(int errorCode)
	{
		fprintf(stderr, "Error code %d. ", errorCode);
	}

	int64 GetLastModifiedDate(const char* path)
	{
		struct _stat64 s;
		if (_stat64(path, &s))
		{
			return 0;
		}
		else
		{
			return s.st_mtime;
		}
	}
}