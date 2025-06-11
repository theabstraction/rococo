#include <rococo.xaudio2.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Audio;

namespace AudioAnon
{
	inline X3DAUDIO_VECTOR RHS_RightNorthUp_To_LHS_RightUpNorth(cr_vec3 p)
	{
		return X3DAUDIO_VECTOR{ p.x, p.z, p.y };
	}

	struct Audio3DEmitter: IAudio3DEmitterSupervisor
	{
		X3DAUDIO_EMITTER e;
		EmitterDSP dsp;
		DWORD channelMask;
		EmitterSoundMatrix matrix = { 0 };
		EmitterSoundMatrix delays = { 0 };

		Audio3DEmitter()
		{
			channelMask = 0;

			memset(&e, 0, sizeof e);
			e.ChannelCount = 1;
			e.CurveDistanceScaler = 1.0f;
			e.DopplerScaler = 1.0f;

			memset(&dsp, 0, sizeof dsp);

			dsp.SrcChannelCount = 1;
			dsp.pMatrixCoefficients = &matrix;
			dsp.pDelayTimes = &delays;
		}

		virtual ~Audio3DEmitter()
		{

		}

		const EmitterDSP& Dsp() const
		{
			static_assert(sizeof EmitterDSP == sizeof X3DAUDIO_DSP_SETTINGS);
			return reinterpret_cast<const EmitterDSP&>(dsp);
		}

		void Free() override
		{
			delete this;
		}

		void SetFrame(const Audio3DObjectFrame& frame) override
		{
			e.OrientFront = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.facingDirection);
			e.OrientTop = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.upDirection);
			e.Position = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.position);
			e.Velocity = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.dopplerVelocity);
		}
	};

	struct Audio3DEngine: IAudio3DSupervisor
	{
		// N.B -> the handle does not allocate storage or resources, so there is no clean up method
		X3DAUDIO_HANDLE hX3D = { 0 };
		X3DAUDIO_LISTENER listener;
		IXAudio2MasteringVoice& master;

		enum class Output
		{
			Stereo,
			Dolby51,
			Dolby71
		} output;

		Audio3DEngine(float speedOfSound, IXAudio2MasteringVoice& refMaster):
			master(refMaster)
		{
			UpdateSpeakerConfig(speedOfSound);

			Audio3DObjectFrame frame;
			frame.facingDirection = { 0, 1.0f, 0 };
			frame.upDirection = { 0, 0.0f, 1.0f };
			frame.position = { 0, 0, 0 }; 
			frame.dopplerVelocity = { 0, 0, 0 };
			SetListenerFrame(frame);
		}

		void UpdateSpeakerConfig(float speedOfSound) override
		{
			DWORD dwChannelMask;
			HRESULT hr = master.GetChannelMask(&dwChannelMask);
			if FAILED(hr)
			{
				Throw(hr, "%s: master.GetChannelMask(&dwChannelMask) failed. Cannot initialize audio engine", __ROCOCO_FUNCTION__);
			}

			switch(dwChannelMask)
			{
			case 0x03:
				output = Output::Stereo;
				break;
			case 0x3F:
				output = Output::Dolby51;
				break;
			case 0xFF:
				output = Output::Dolby71;
				break;
			default:
				Throw(0, "%s: master.GetChannelMask(&dwChannelMask) failed. Cannot initialize 3D audio engine with channel mask 0x%X", __ROCOCO_FUNCTION__, dwChannelMask);
			}

			hr = X3DAudioInitialize(dwChannelMask, speedOfSound, OUT hX3D);
			if FAILED(hr)
			{
				Throw(hr, "%s: master.GetChannelMask(&dwChannelMask) failed. Cannot initialize audio engine", __ROCOCO_FUNCTION__);
			}
		}

		// The upVector must be orthoNormal to the facingVector
		void SetListenerFrame(const Audio3DObjectFrame& frame)
		{
			if (fabsf(Dot(frame.facingDirection, frame.upDirection)) >= 0.01f)
			{
				Throw(0, "%s: The facingVector and the upVector were not orthonormal", __ROCOCO_FUNCTION__);
			}

			listener.OrientFront = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.facingDirection);
			listener.OrientTop = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.upDirection);
			listener.Position = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.position);
			listener.Velocity = RHS_RightNorthUp_To_LHS_RightUpNorth(frame.dopplerVelocity);
			listener.pCone = nullptr;			
		}

		void ComputeDSP(IAudio3DEmitter& emitter) override
		{
			int flags = 0;
			auto& audio3DEmitter = static_cast<Audio3DEmitter&>(emitter);

			for (auto& speaker : audio3DEmitter.matrix.all.speakers)
			{
				speaker = 1.0f;
			}

			switch (output)
			{
			case Output::Stereo:
				audio3DEmitter.dsp.DstChannelCount = 2;
				break;
			case Output::Dolby51:
				audio3DEmitter.dsp.DstChannelCount = 6;
				break;
			case Output::Dolby71:
				audio3DEmitter.dsp.DstChannelCount = 8;
				break;
			}

			flags |= X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DELAY | X3DAUDIO_CALCULATE_DOPPLER; // | X3DAUDIO_CALCULATE_LPF_REVERB | X3DAUDIO_CALCULATE_REVERB;
			// flags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE | X3DAUDIO_CALCULATE_EMITTER_ANGLE;
			
			X3DAudioCalculate(hX3D, &listener, &audio3DEmitter.e, flags, IN OUT reinterpret_cast<X3DAUDIO_DSP_SETTINGS*>(&audio3DEmitter.dsp));
		}

		IAudio3DEmitterSupervisor* CreateEmitter()
		{
			return new Audio3DEmitter();
		}

		~Audio3DEngine()
		{

		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API IAudio3DSupervisor* CreateX3D(float speedOfSoundInMetresPerSecond, IXAudio2MasteringVoice& master)
	{
		return new AudioAnon::Audio3DEngine(speedOfSoundInMetresPerSecond, master);
	}
}