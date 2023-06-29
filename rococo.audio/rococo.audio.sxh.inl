// BennyHill generated Sexy native functions for Rococo::Audio::IAudio 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoAudioIAudioSetMP3Music(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _mp3musicPingPath;
		ReadInput(_mp3musicPingPath, _sf, -_offset);
		fstring mp3musicPingPath { _mp3musicPingPath->buffer, _mp3musicPingPath->length };


		Rococo::Audio::IAudio* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMP3Music(mp3musicPingPath);
	}
	void NativeRococoAudioIAudioPlay3DSound(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 forceLevel;
		_offset += sizeof(forceLevel);
		ReadInput(forceLevel, _sf, -_offset);

		int32 priority;
		_offset += sizeof(priority);
		ReadInput(priority, _sf, -_offset);

		Vec3* worldPosition;
		_offset += sizeof(worldPosition);
		ReadInput(worldPosition, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _mp3fxPingPath;
		ReadInput(_mp3fxPingPath, _sf, -_offset);
		fstring mp3fxPingPath { _mp3fxPingPath->buffer, _mp3fxPingPath->length };


		Rococo::Audio::IAudio* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Play3DSound(mp3fxPingPath, *worldPosition, priority, forceLevel);
	}

	void NativeGetHandleForRococoAudioGetAudio(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Audio::IAudio* nceContext = reinterpret_cast<Rococo::Audio::IAudio*>(_nce.context);
		// Uses: Rococo::Audio::IAudio* FactoryConstructRococoAudioGetAudio(Rococo::Audio::IAudio* _context);
		Rococo::Audio::IAudio* pObject = FactoryConstructRococoAudioGetAudio(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Audio
{
	void AddNativeCalls_RococoAudioIAudio(Rococo::Script::IPublicScriptSystem& ss, Rococo::Audio::IAudio* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Audio.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoAudioGetAudio, _nceContext, ("GetHandleForIAudio0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoAudioIAudioSetMP3Music, nullptr, ("IAudioSetMP3Music (Pointer hObject)(Sys.Type.IString mp3musicPingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoAudioIAudioPlay3DSound, nullptr, ("IAudioPlay3DSound (Pointer hObject)(Sys.Type.IString mp3fxPingPath)(Sys.Maths.Vec3 worldPosition)(Int32 priority)(Int32 forceLevel) -> "), __FILE__, __LINE__);
	}
}
