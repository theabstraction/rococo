// rococo.audio.dllmain.cpp : Defines the entry point for the DLL application.
#include <rococo.audio.h>
#include <rococo.os.win32.global-ns.h>
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

namespace Rococo::Audio
{
    void InitOggMemoryHandlers();
}

BOOL APIENTRY DllMain( HMODULE /* hModule */, DWORD ul_reason_for_call, LPVOID /* lpReserved */)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:    
        Rococo::Audio::InitOggMemoryHandlers();
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

#include "Audio.sxh.inl"

namespace  Rococo::Audio
{
    ROCOCO_AUDIO_API void DLL_AddNativeCalls_RococoAudioIAudio(Rococo::Script::IPublicScriptSystem& ss, Rococo::Audio::IAudio* nceContext)
    {
        return Interop::AddNativeCalls_RococoAudioIAudio(ss, nceContext);
    }
}
