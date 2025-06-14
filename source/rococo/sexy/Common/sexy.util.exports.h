#pragma once

#include <sexy.types.h>

#ifndef SEXYUTIL_API
# error "define SEXYUTIL_API in the compiler environment"
#endif

namespace Rococo::Memory
{
#ifdef USE_STD_ALLOCATOR_FOR_SEXY
    FORCE_INLINE void* AllocateSexyMemory(size_t nBytes)
    {
        return new char[nBytes];
    }

    FORCE_INLINE void FreeSexyMemory(void* pBuffer, size_t nBytes)
    {
        UNUSED(nBytes);
        delete[](char*) pBuffer;
    }

    FORCE_INLINE void FreeSexyUnknownMemory(void* pBuffer)
    {
        delete[](char*) pBuffer;
    }

    template<class T>
    T* AllocateSexyPointers(size_t numberOfPointers)
    {
        return new T[numberOfPointers];
    }

    template<class T>
    void FreeSexyPointers(T* buffer)
    {
        delete[] buffer;
    }
#else
    SEXYUTIL_API void* AllocateSexyMemory(size_t nBytes);
    SEXYUTIL_API void FreeSexyMemory(void* pBuffer, size_t nBytes);
    SEXYUTIL_API void FreeSexyUnknownMemory(void* pBuffer);

    template<class T>
    T* AllocateSexyPointers(size_t numberOfPointers)
    {
        return (T*)AllocateSexyMemory(sizeof(T*) * numberOfPointers);
    }

    template<class T>
    void FreeSexyPointers(T* buffer)
    {
        FreeSexyUnknownMemory(buffer);
    }
#endif
    SEXYUTIL_API IAllocator& GetSexyAllocator();
    SEXYUTIL_API void SetSexyAllocator(IAllocator* allocator);
}

namespace Rococo::Variants
{
	SEXYUTIL_API SexyVarType GetBestCastType(SexyVarType a, SexyVarType b);
	SEXYUTIL_API bool TryRecast(OUT VariantValue& end, IN const VariantValue& original, SexyVarType orignalType, SexyVarType endType);
}

namespace Rococo
{
    SEXYUTIL_API sexstring CreateSexString(cstr src, int32 length = -1);
    SEXYUTIL_API void FreeSexString(sexstring s);
}

namespace Rococo::Script
{
    SEXYUTIL_API void EnumerateRegisters(Rococo::VM::CPU& cpu, Rococo::Debugger::IRegisterEnumerationCallback& cb);
    struct NativeCallEnvironment;
}

namespace Rococo::Compiler
{
    struct FastStringBuilder;

    struct VirtualTable;
    typedef VirtualTable** InterfacePointer;
}

namespace Rococo::Helpers // Used by Benny Hill to simplify native function integration
{
    class StringPopulator : public Strings::IStringPopulator
    {
        Rococo::Compiler::FastStringBuilder* builder;
    public:
        SEXYUTIL_API StringPopulator(Script::NativeCallEnvironment& _nce, Compiler::InterfacePointer pInterface);
        void Populate(cstr text) override;
    };

    SEXYUTIL_API const Compiler::IStructure& GetDefaultProxy(cstr fqNS, cstr interfaceName, cstr proxyName, Script::IPublicScriptSystem& ss);
}