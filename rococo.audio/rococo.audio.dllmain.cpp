// rococo.audio.dllmain.cpp : Defines the entry point for the DLL application.
#include <rococo.audio.h>
#include <rococo.os.win32.h>
#include <sexy.script.h>
#include <sexy.vm.cpu.h>

#ifdef _WIN32
# ifdef _DEBUG
#  pragma comment(lib, "rococo.util.debug.lib")
#  pragma comment(lib, "rococo.maths.debug.lib")
# else
#  pragma comment(lib, "rococo.util.lib")
#  pragma comment(lib, "rococo.maths.lib")
# endif
#endif

//////////////////////// XAUDIO2 and Media Foundation stuff for audio decoding ////////////////////
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "XAudio2.lib")

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

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
