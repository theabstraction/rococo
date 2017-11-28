	struct MaterialVertexData
	{
		RGBAb colour;
		MaterialId id;
		float gloss;
	};
	struct ObjectVertex
	{
		Vec3 position;
		Vec3 normal;
		Vec2 uv;
		MaterialVertexData mat;
	};
	struct LightSpec
	{
		Vec3 position;
		Vec3 direction;
		Degrees fov;
		RGBA diffuse;
		RGBA ambience;
		Degrees cutoffAngle;
		float cutoffPower;
		float attenuation;
	};
	struct QuadColours
	{
		RGBAb a;
		RGBAb b;
		RGBAb c;
		RGBAb d;
	};
	struct QuadVertices
	{
		Quad positions;
		GuiRectf uv;
		Quad normals;
		QuadColours colours;
	};
	struct VertexTriangle
	{
		ObjectVertex a;
		ObjectVertex b;
		ObjectVertex c;
	};
	struct AABB2d
	{
		float left;
		float bottom;
		float right;
		float top;
	};
	struct FlameDef
	{
		Metres minStartParticleSize;
		Metres maxStartParticleSize;
		Metres minEndParticleSize;
		Metres maxEndParticleSize;
		int32 particleCount;
		Seconds minLifeSpan;
		Seconds maxLifeSpan;
		float initialVelocityRange;
		float initialSpawnPosRange;
		float jetSpeed;
		Metres attractorHeight;
		Metres attractorMaxRange;
		Metres attractorMinRange;
		Metres attractorSpawnPosRange;
		Seconds attractorAIduration;
		float attractorResetProbability;
		float attractorDriftFactor;
		float attractorPerturbFactor;
		float attractorForce;
	};
namespace Rococo { namespace Graphics { 
	struct SampleStateDef
	{
		Rococo::Graphics::SampleMethod method;
		Rococo::Graphics::SampleFilter u;
		Rococo::Graphics::SampleFilter v;
		Rococo::Graphics::SampleFilter w;
		RGBA borderColour;
	};
}}
