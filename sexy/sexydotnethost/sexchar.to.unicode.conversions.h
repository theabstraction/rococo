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
		size_t neededLen = wcslen(input)+1;
		if (neededLen > bufferCapacity)
		{
			return false;
		}

		memcpy(output, input, 2 * neededLen);
		output[neededLen] = 0;
		return true;
	}

	int CopySexCharToUnicode(wchar_t* output, size_t bufferCapacity, csexstr input)
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

	int CopySexCharToUnicode(wchar_t* output, size_t bufferCapacity, csexstr input)
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

	void ProtectedFormatValue(IScriptSystem& ss, wchar_t* unicodeValue, size_t bufferLen, VARTYPE type, const void* pVariableData)
	{
		switch(type)
		{
		case VARTYPE_Bad:
			_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"Bad type");
			break;
		case VARTYPE_Bool:
			{
				const int32 value = *(const int32*) pVariableData;
				if (value == 0 || value == 1) _snwprintf_s(unicodeValue, bufferLen, bufferLen, value == 1 ? L"true" :  L"false");
				else  _snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%d (%8.8x)", value, value);
			}
			break;
		case VARTYPE_Derivative:
			_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"");
			break;
		case VARTYPE_Int32:
			{
				const int32* pValue = (const int32*) pVariableData;
				_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%d (%8.8x)", *pValue, *pValue);
			}
			break;
		case VARTYPE_Int64:
			{
				const int64* pValue = (const int64*) pVariableData;
				_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%lld (%8llx)", *pValue, *pValue);
			}
			break;
		case VARTYPE_Float32:
			{
				const float32* pValue = (const float32*) pVariableData;
				_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%g", *pValue);
			}
			break;
		case VARTYPE_Float64:
			{
				const float64* pValue = (const float64*) pVariableData;
				_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%lg", *pValue);
			}
			break;
		case VARTYPE_Pointer:
			{
				void **ppData = (void**) pVariableData;
				const void* ptr = *ppData;
				csexstr symbol = ss.GetSymbol(ptr);
				if (symbol == NULL)
				{
					_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"%p", ptr);
				}
				else
				{
					CopySexCharToUnicode(unicodeValue, bufferLen, symbol);
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
				_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"Id: %llu. SF: 0x%llX", pValue->id, (size_t) pValue->parentSF);
			}
			break;
		default:
			_snwprintf_s(unicodeValue, bufferLen, bufferLen, L"Unknown type");
		}
	}

	void FormatValue(IScriptSystem& ss, wchar_t* unicodeValue, size_t bufferLen, VARTYPE type, const void* pVariableData)
	{
		__try
		{
			ProtectedFormatValue(ss, unicodeValue, bufferLen, type, pVariableData);
		}
		__except(1)
		{
			_snwprintf_s(unicodeValue, bufferLen, _TRUNCATE, L"Bad pointer");
		}
	}
}