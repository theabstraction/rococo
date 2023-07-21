// rococo.audio.dllmain.cpp : Defines the entry point for the DLL application.
#include <rococo.audio.h>
#include <rococo.os.win32.h>
#include <sexy.script.h>
#include <sexy.vm.cpu.h>
#include <rococo.allocators.h>

#ifdef _WIN32
#  pragma comment(lib, "rococo.util.lib")
#  pragma comment(lib, "rococo.maths.lib")
#endif

//////////////////////// XAUDIO2 and Media Foundation stuff for audio decoding ////////////////////
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "XAudio2.lib")

using namespace Rococo;
using namespace Rococo::Memory;

IAllocatorSupervisor* audioAllocator = nullptr;

void* AllocateThroughDll(size_t nBytes)
{
    if (audioAllocator != nullptr)
    {
        return audioAllocator->Allocate(nBytes);
    }

    OS::TripDebugger();
    throw std::bad_alloc();
}

void FreeThroughDLL(void* buffer)
{
    if (audioAllocator != nullptr)
    {
        audioAllocator->FreeData(buffer);
        return;
    }

    OS::TripDebugger();
}

namespace Rococo::Audio
{
    void FreeAudioAllocator();
}

static void* FirstTimeAudioAllocator(size_t nBytes);
static void FirstTimeAudioDeallocator(void* buffer);

static AllocatorFunctions audioAllocatorFunctions = { FirstTimeAudioAllocator, FirstTimeAudioDeallocator };

static void* FirstTimeAudioAllocator(size_t nBytes)
{
    if (!audioAllocator)
    {
        audioAllocator = Rococo::Memory::CreateBlockAllocator(128, 0, "Audio");
        atexit(Rococo::Audio::FreeAudioAllocator);
    }
    audioAllocatorFunctions = { AllocateThroughDll , FreeThroughDLL };
    return audioAllocatorFunctions.Allocate(nBytes);
}

static void FirstTimeAudioDeallocator(void* buffer)
{
    if (!audioAllocator)
    {
        audioAllocator = Rococo::Memory::CreateBlockAllocator(128, 0, "Audio");
        atexit(Rococo::Audio::FreeAudioAllocator);
    }
    audioAllocatorFunctions = { AllocateThroughDll , FreeThroughDLL };
    audioAllocatorFunctions.Free(buffer);
}


namespace Rococo::Audio
{
    Rococo::Memory::AllocatorFunctions GetAudioAllocators()
    {
        if (!audioAllocator)
        {
            audioAllocator = Rococo::Memory::CreateBlockAllocator(128, 0, "Audio");
            atexit(Rococo::Audio::FreeAudioAllocator);
        }
        audioAllocatorFunctions = { AllocateThroughDll, FreeThroughDLL };
        return audioAllocatorFunctions;
    }

    void FreeAudioAllocator()
    {
        if (audioAllocator)
        {
            audioAllocator->Free();
            audioAllocator = nullptr;
        }
    }
}


BOOL APIENTRY DllMain( HMODULE /* hModule */, DWORD ul_reason_for_call, LPVOID /* lpReserved */)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:    
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// Frees memory allocated with AllocatorFunctions::Allocate
typedef void (*FN_FREE_MEMORY)(void* buffer);

Rococo::Audio::IAudio* FactoryConstructRococoAudioGetAudio(Rococo::Audio::IAudio* _context)
{
    return _context;
}

#include "rococo.audio.sxh.inl"

namespace  Rococo::Audio
{
    ROCOCO_AUDIO_API void DLL_AddNativeCalls_RococoAudioIAudio(Rococo::Script::IPublicScriptSystem& ss, Rococo::Audio::IAudio* nceContext)
    {
        return AddNativeCalls_RococoAudioIAudio(ss, nceContext);
    }
}
