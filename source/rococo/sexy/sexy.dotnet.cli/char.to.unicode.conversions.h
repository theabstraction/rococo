#include <stdio.h>

namespace
{
	bool CopyUnicodeTochar(char* output, size_t bufferCapacity, const Char* input);

#ifdef char_IS_WIDE
# define charENCODING "UNICODE"
	bool IsSexUnicode = true;
	ISourceCode* AddUnicodeModule(const Byte* input, IScriptSystem& ss, int moduleLength, cstr name)
	{
		cstr wideInput = (cstr) input;
      return ss.SParser().DuplicateSourceBuffer(wideInput, moduleLength, Vec2i{ 0,0 }, name);
	}

	ISourceCode* AddAsciiModule(const Byte* input, IScriptSystem& ss, int moduleLength, cstr name)
	{
		char* tempBuffer = new char[moduleLength];
		for(int i = 0; i < moduleLength; ++i)
		{
			tempBuffer[i] = input[i];
		}

		ISourceCode* sc = ss.SParser().DuplicateSourceBuffer(tempBuffer, moduleLength, Vec2i{ 0,0 }, name);
		delete tempBuffer;
		return sc;
	}

	bool CopyUnicodeTochar(char* output, size_t bufferCapacity, const Char* input)
	{
		size_t neededLen = rlen(input)+1;
		if (neededLen > bufferCapacity)
		{
			return false;
		}

		memcpy(output, input, 2 * neededLen);
		output[neededLen] = 0;
		return true;
	}

	int CopycharToUnicode(rchar* output, size_t bufferCapacity, cstr input)
	{
		size_t len = StringLength(input);
		if (len >= bufferCapacity)
		{
			return 0;
		}

		memcpy(output, input, 2*len+2);

		return (int) len;
	}

	int CopyAsciiToTochar(char* output, size_t bufferCapacity, const char* input)
	{
		size_t len = StringLength(input);
		if (len >= bufferCapacity)
		{
			return 0;
		}

		for(size_t i = 0; i < len; ++i)
		{
			output[i] = input[i];
		}

		output[len] = 0;

		return (int) len;
	}
#else
# define charENCODING "ASCII"
	bool IsSexUnicode = false;
	ISourceCode* AddUnicodeModule(const Byte* input, IScriptSystem& ss, int moduleLength, const wchar_t* name)
	{
		char* tempBuffer = new char[moduleLength];
		if (!CopyUnicodeTochar(tempBuffer, moduleLength, (const Char*) input))
		{
			delete tempBuffer;
			return NULL;
		}
		else
		{
			char asciiName[1024];
			SafeFormat(asciiName, 1024, "%S", name);
			ISourceCode* sc = ss.SParser().DuplicateSourceBuffer(tempBuffer, moduleLength, Vec2i{ 0,0 }, asciiName);
			delete tempBuffer;
			return sc;
		}		
	}

	ISourceCode* AddAsciiModule(const Byte* input, IScriptSystem& ss, int moduleLength, const wchar_t* name)
	{
		char asciiName[1024];
		SafeFormat(asciiName, 1024, "%S", name);
		return ss.SParser().DuplicateSourceBuffer((cstr) input, moduleLength, Vec2i{ 0,0 }, asciiName);
	}

   int CopycharToUnicode(wchar_t* output, size_t bufferCapacity, cstr input)
   {
      return _snwprintf_s(output, bufferCapacity, _TRUNCATE, L"%S", input);
   }

	bool CopyUnicodeTochar(char* output, size_t bufferCapacity, const Char* input)
	{
		size_t neededLen = wcslen(input)+1;
		if (neededLen > bufferCapacity)
		{
			return false;
		}

		for(size_t i = 0; i <= neededLen; ++i)
		{
			Char c = input[i];
			if (c > 127)
			{
				return false; // Requires unicode build of the Sexy library, or filename to be alphanumeric
			}

			output[i] = (char) input[i];
		}

		output[neededLen] = 0;
		return true;
	}

	int CopycharToUnicode(char* output, size_t bufferCapacity, cstr input)
	{
		size_t len = StringLength(input);
		if (len >= bufferCapacity)
		{
			return NULL;
		}

		for(size_t i = 0; i <= len; ++i)
		{
			output[i] = input[i];
		}

		return (int) len;
	}
#endif

   void CopyAsciiToTochar(char* output, size_t bufferCapacity, const char* input)
   {
      strncpy_s(output, bufferCapacity, input, _TRUNCATE);
   }

	void ProtectedFormatValue(IScriptSystem& ss, char* rvalue, size_t bufferLen, VARTYPE type, const void* pVariableData)
	{
		switch(type)
		{
		case VARTYPE_Bad:
         SafeFormat(rvalue, bufferLen, ("Bad type"));
			break;
		case VARTYPE_Bool:
			{
				const int32 value = *(const int32*) pVariableData;
				if (value == 0 || value == 1) SafeFormat(rvalue, bufferLen, value == 1 ? ("true") : ("false"));
				else  SafeFormat(rvalue, bufferLen, ("%d (%8.8x)"), value, value);
			}
			break;
		case VARTYPE_Array:
		case VARTYPE_List:
		case VARTYPE_Map:
			SafeFormat(rvalue, bufferLen, "0x%llX", (int64)pVariableData);
			break;
		case VARTYPE_Derivative:
			SafeFormat(rvalue, bufferLen, "%llX -> %llX", (int64) pVariableData, (int64) *(void**)pVariableData);
			break;
		case VARTYPE_Int32:
			{
				const int32* pValue = (const int32*) pVariableData;
            SafeFormat(rvalue, bufferLen, ("%d (%8.8X)"), *pValue, *pValue);
			}
			break;
		case VARTYPE_Int64:
			{
				const int64* pValue = (const int64*) pVariableData;
            SafeFormat(rvalue, bufferLen, ("%lld (%8llX)"), *pValue, *pValue);
			}
			break;
		case VARTYPE_Float32:
			{
				const float32* pValue = (const float32*) pVariableData;
            SafeFormat(rvalue, bufferLen, ("%g"), *pValue);
			}
			break;
		case VARTYPE_Float64:
			{
				const float64* pValue = (const float64*) pVariableData;
            SafeFormat(rvalue, bufferLen, ("%lg"), *pValue);
			}
			break;
		case VARTYPE_Pointer:
			{
				void **ppData = (void**) pVariableData;
				const void* ptr = *ppData;
				cstr symbol = ss.GetSymbol(ptr);
				if (symbol == NULL)
				{
               SafeFormat(rvalue, bufferLen, ("%p"), ptr);
				}
				else
				{
					memcpy_s(rvalue, sizeof(char) * bufferLen, symbol, sizeof(char) * bufferLen);
				}
			}
			break;
		case VARTYPE_Closure:
			{
				struct Closure
				{
					size_t id;
					void* parentSF;
				};

				const Closure* pValue = (const Closure*)pVariableData;
            SafeFormat(rvalue, bufferLen, ("Id: %llu. SF: 0x%8llX"), pValue->id, (size_t) pValue->parentSF);
			}
			break;
		default:
         SafeFormat(rvalue, bufferLen, ("Unknown type"));
		}
	}

	void FormatValue(IScriptSystem& ss, wchar_t* unicodeValue, size_t bufferLen, VARTYPE type, const void* pVariableData)
	{
		__try
		{
         char rvalue[256];
			ProtectedFormatValue(ss, rvalue, bufferLen, type, pVariableData);

         _snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%S", rvalue);
		}
		__except(1)
		{
         _snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%s", L"Bad pointer");
		}
	}
}