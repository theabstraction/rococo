namespace Rococo { namespace Audio { 
	struct AudioSource3D
	{
		int32 priority;
		float volume;
		Vec3 position;
		Vec3 dopplerVelocity;
		float msDelay;
	};
}}
