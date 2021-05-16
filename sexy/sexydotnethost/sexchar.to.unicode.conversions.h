#include <stdio.h>

namespace
{
	bool CopyUnicodeToSexChar(SEXCHAR* output, size_t bufferCapacity, const Char* input);

#ifdef SEXCHAR_IS_WIDE
# define SEXCHARENCODING "UNICODE"
	bool IsSexUnicode = true;
	ISourceCode* AddUnicodeModule(const Byte* input, IScriptSystem& ss, int moduleLength, csexstr name)
	{
		csexstr wideInput = (csexstr) input;
      return ss.SParser().DuplicateSourceBuffer(wideInput, moduleLength, Vec2i{ 0,0 }, name);
	}

	ISourceCode* AddAsciiModule(const Byte* input, IScriptSystem& ss, int moduleLength, csexstr name)
	{
		SEXCHAR* tempBuffer = new SEXCHAR[moduleLength];
		for(int i = 0; i < moduleLength; ++i)
		{
			tempBuffer[i] = input[i];
		}

		ISourceCode* sc = ss.SParser().DuplicateSourceBuffer(tempBuffer, moduleLength, Vec2i{ 0,0 }, name);
		delete tempBuffer;
		return sc;
	}

	bool CopyUnicodeToSexChar(SEXCHAR* output, size_t bufferCapacity, const Char* input)
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

	int CopySexCharToUnicode(rchar* output, size_t bufferCapacity, csexstr input)
	{
		size_t len = StringLength(input);
		if (len >= bufferCapacity)
		{
			return 0;
		}

		memcpy(output, input, 2*len+2);

		return (int) len;
	}

	int CopyAsciiToToSEXCHAR(SEXCHAR* output, size_t bufferCapacity, const char* input)
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
# define SEXCHARENCODING "ASCII"
	bool IsSexUnicode = false;
	ISourceCode* AddUnicodeModule(const Byte* input, IScriptSystem& ss, int moduleLength, csexstr name)
	{
		SEXCHAR* tempBuffer = new SEXCHAR[moduleLength];
		if (!CopyUnicodeToSexChar(tempBuffer, moduleLength, (const Char*) input))
		{
			delete tempBuffer;
			return NULL;
		}
		else
		{
			ISourceCode* sc = ss.SParser().DuplicateSourceBuffer(tempBuffer, moduleLength, Vec2i{ 0,0 }, name);
			delete tempBuffer;
			return sc;
		}		
	}

	ISourceCode* AddAsciiModule(const Byte* input, IScriptSystem& ss, int moduleLength, csexstr name)
	{
		return ss.SParser().DuplicateSourceBuffer((csexstr) input, moduleLength, Vec2i{ 0,0 }, name);
	}

	bool CopyUnicodeToSexChar(SEXCHAR* output, size_t bufferCapacity, const Char* input)
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

			output[i] = (SEXCHAR) input[i];
		}

		output[neededLen] = 0;
		return true;
	}

	int CopySexCharToUnicode(rchar* output, size_t bufferCapacity, csexstr input)
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

   void CopyAsciiToToSEXCHAR(SEXCHAR* output, size_t bufferCapacity, const char* input)
   {
      strncpy_s(output, bufferCapacity, input, _TRUNCATE);
   }

	void ProtectedFormatValue(IScriptSystem& ss, rchar* rvalue, size_t bufferLen, VARTYPE type, const void* pVariableData)
	{
		switch(type)
		{
		case VARTYPE_Bad:
			SafeFormat(rvalue, bufferLen, SEXTEXT("Bad type"));
			break;
		case VARTYPE_Bool:
			{
				const int32 value = *(const int32*) pVariableData;
				if (value == 0 || value == 1) SafeFormat(rvalue, bufferLen, value == 1 ? SEXTEXT("true") : SEXTEXT("false"));
				else  SafeFormat(rvalue, bufferLen, SEXTEXT("%d (%8.8x)"), value, value);
			}
			break;
		case VARTYPE_Derivative:
		case VARTYPE_Array:
		case VARTYPE_List:
		case VARTYPE_Map:
			{
				const uint64** pValue = (const uint64*)pVariableData;
				SafeFormat(rvalue, bufferLen, SEXTEXT("-> 0x%llX"), *pValue);
			}
			break;
		case VARTYPE_Int32:
			{
				const int32* pValue = (const int32*) pVariableData;
				SafeFormat(rvalue, bufferLen, SEXTEXT("%d (%8.8x)"), *pValue, *pValue);
			}
			break;
		case VARTYPE_Int64:
			{
				const int64* pValue = (const int64*) pVariableData;
				SafeFormat(rvalue, bufferLen, SEXTEXT("%lld (%8llx)"), *pValue, *pValue);
			}
			break;
		case VARTYPE_Float32:
			{
				const float32* pValue = (const float32*) pVariableData;
				SafeFormat(rvalue, bufferLen, SEXTEXT("%g"), *pValue);
			}
			break;
		case VARTYPE_Float64:
			{
				const float64* pValue = (const float64*) pVariableData;
				SafeFormat(rvalue, bufferLen, SEXTEXT("%lg"), *pValue);
			}
			break;
		case VARTYPE_Pointer:
			{
				void **ppData = (void**) pVariableData;
				const void* ptr = *ppData;
				csexstr symbol = ss.GetSymbol(ptr);
				if (symbol == NULL)
				{
               SafeFormat(rvalue, bufferLen, SEXTEXT("%p"), ptr);
				}
				else
				{
					memcpy_s(rvalue, sizeof(rchar) * bufferLen, symbol, sizeof(rchar) * bufferLen);
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
				SafeFormat(rvalue, bufferLen, SEXTEXT("Id: %llu. SF: 0x%llX"), pValue->id, (size_t) pValue->parentSF);
			}
			break;
		default:
			SafeFormat(rvalue, bufferLen, SEXTEXT("Unknown type"));
		}
	}

	void FormatValue(IScriptSystem& ss, wchar_t* unicodeValue, size_t bufferLen, VARTYPE type, const void* pVariableData)
	{
		__try
		{
         char rvalue[256];
			ProtectedFormatValue(ss, rvalue, bufferLen, type, pVariableData);

         _snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%hs", rvalue);
		}
		__except(1)
		{
         _snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"Bad pointer");
		}
	}
}