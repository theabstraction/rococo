#pragma once

#include <rococo.types.h>

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sexy.types.h>
#include <stdio.h>
#include <sexy.s-parser.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include "sexy.strings.h"
#include <sexy.stdstrings.h>
#include <unordered_map>
#include <unordered_set>

namespace Rococo
{
    enum class EQualifier;

    bool AreEqual(cstr s, const sexstring& t);

    ROCOCO_INTERFACE ITextBuilder
    {
        virtual void WriteString(cstr s) = 0;
    };

    class FileAppender : public ITextBuilder
    {
    private:
        HANDLE hFile;
        cstr filename;
    public:
        FileAppender() = delete;
        FileAppender(const FileAppender& src) = delete;
        FileAppender(cstr _filename);
        ~FileAppender();

        void Append(cstr format, ...);
        void Append(char c);
        void AppendSequence(int count, char c);
        void WriteString(cstr s) override;
    };

    void WriteStandardErrorCode(int errorCode);
    int64 GetLastModifiedDate(const char* path);

    void GetFQCppStructName(char* compressedStructName, char* cppStructName, size_t capacity, cstr fqStructName);

    struct TypeDef
    {
        Rococo::stdstring sexyType;
        Rococo::stdstring cppType;
    };

    typedef std::unordered_map<Rococo::stdstring, TypeDef> TTypeMap;

    class CppType
    {
    private:
        enum { MAX_TOKEN_LEN = 256 };
        char bennyHillDef[MAX_TOKEN_LEN];
        char compressedName[MAX_TOKEN_LEN];
        char fqName[MAX_TOKEN_LEN];

    public:
        CppType()
        {
            bennyHillDef[0] = 0;
            compressedName[0] = 0;
            fqName[0] = 0;
        }

        void Set(cstr bennyHillDef)
        {
            Strings::CopyString(this->bennyHillDef, MAX_TOKEN_LEN, bennyHillDef);
            GetFQCppStructName(compressedName, fqName, 256, bennyHillDef);
        }

        cstr CompressedName() const
        {
            return compressedName;
        }

        cstr FQName() const
        {
            return fqName;
        }

        cstr SexyName() const
        {
            return bennyHillDef;
        }
    };

    typedef std::vector<const Rococo::Sex::ISExpression*, Memory::SexyAllocator<const Sex::ISExpression*>> TExpressions;

    struct InterfaceContext
    {
        enum { MAX_TOKEN_LEN = 256 };
        CppType asCppInterface;
        char asSexyInterface[MAX_TOKEN_LEN];
        char appendSexyFile[_MAX_PATH];
        char appendCppHeaderFile[_MAX_PATH];
        char appendCppImplFile[_MAX_PATH];
        cstr sexyBase = nullptr;
        cstr cppBase = nullptr;

        Strings::HString componentAPINamespace;
        Strings::HString componentShortFriendlyName;
        Strings::HString componentAPIName;

        bool isSingleton; // If true then the context comes from the native registration method, else it comes from the factory.
        CppType nceContext;
        bool hasDestructor;
        TExpressions factories;

        InterfaceContext()
        {
            asSexyInterface[0] = 0;
            appendSexyFile[0] = 0;
            appendCppHeaderFile[0] = 0;
            appendCppImplFile[0] = 0;
            hasDestructor = false;
            isSingleton = false;
        }

        void SetComponent(cstr apiNamespace, cstr shortFriendlyName, cstr apiName)
        {
            componentAPINamespace = apiNamespace;
            componentShortFriendlyName = shortFriendlyName;
            componentAPIName = apiName;
        }
    };

    struct InterfaceDef
    {
        InterfaceContext ic;
        const Rococo::Sex::ISExpression* sdef;
        std::vector<const Sex::ISExpression*, Memory::SexyAllocator<const Sex::ISExpression*>> methodArray;
    };

    struct EnumContext
    {
        enum { MAX_TOKEN_LEN = 256 };
        CppType underlyingType;
        CppType asCppEnum;
        char asSexyEnum[MAX_TOKEN_LEN];
        char appendSexyFile[_MAX_PATH];
        char appendCppHeaderFile[_MAX_PATH];
        char appendCppImplFile[_MAX_PATH];
        std::vector<std::pair<stdstring, int64>, Memory::SexyAllocator<std::pair<stdstring, int64>>> values;

        EnumContext()
        {
            asSexyEnum[0] = 0;
            appendSexyFile[0] = 0;
            appendCppHeaderFile[0] = 0;
            appendCppImplFile[0] = 0;
        }
    };

    struct EnumDef
    {
        EnumContext ec;
        const Rococo::Sex::ISExpression* sdef;
    };

    struct ParseContext
    {
        char scriptInput[_MAX_PATH];              // Full path of the sxh file
        char projectRoot[_MAX_PATH];              // Where the config and sxh files are found
        char contentRoot[_MAX_PATH];              // Where the content directory is found
        char scriptName[_MAX_PATH];               // Extracted from the sxh filename, gives a short name for the script
        char xcPath[_MAX_PATH];                   // The location of the config.xc file
        char scriptInputSansExtension[_MAX_PATH]; // Containing directory of the sxh file
        char cppRootDirectory[_MAX_PATH];         // Root where C++ are to be generated
        char cppTypesFilename[_MAX_PATH];         // C++ types file generated by the sxh file
        char sexyTypesFilename[_MAX_PATH];        // Sexy types file generated by the sxh file
        char cppException[128];                   // cpp exception tyoe caught in native code
        char moduleNamespace[NAMESPACE_MAX_LENGTH]; // The namespace associated with the sxh file itself. Example "Rococo.Audio" would be a good module namespace for audio.sxh. Strategic use limits filename lengths.

        TTypeMap primitives;
        TTypeMap structs;
        std::unordered_map<rstdstring, InterfaceDef*, std::hash<rstdstring>, std::equal_to<rstdstring>, Memory::SexyAllocator<std::pair<const rstdstring, InterfaceDef*>>> interfaces;
        std::vector<EnumDef, Memory::SexyAllocator<EnumDef>> enums;
        mutable std::unordered_map<Rococo::rstdstring, uint32, std::hash<rstdstring>, std::equal_to<rstdstring>, Memory::SexyAllocator<std::pair<const rstdstring, uint32>>> namespaces;
        mutable std::unordered_set<Rococo::rstdstring> prependedFiles;
    };

    Rococo::cstr StringFrom(Rococo::Sex::cr_sex s);
    Rococo::cstr StringFrom(Rococo::Sex::cr_sex command, int elementIndex);

    void AppendCppType(EQualifier qualifier, FileAppender& appender, Sex::cr_sex field, cstr fieldtype, const ParseContext& pc);
}
