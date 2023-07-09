#include <rococo.mplat.h>
#include <rococo.maths.h>
#include <rococo.time.h>
#include <unordered_map>
#include <random>

using namespace Rococo;
using namespace Rococo::Entities;

namespace ANON
{
	std::default_random_engine rng;

	struct SpectrumBand
	{
		float relativeLifeTime;
		RGBA keyColour;
	};

	bool operator < (const SpectrumBand& b, float f)
	{
		return b.relativeLifeTime < f;
	}

	typedef std::vector<SpectrumBand> Spectra;

	void SetSpectrum(Spectra& dest, const Spectra& src)
	{
		dest.clear();
		dest.reserve(src.size());
		for (auto& i : src)
		{
			dest.push_back(i);
		}
	}

	uint8 ToUint8(float f)
	{
		return (uint8)(int)(255.0f * f);
	}

	RGBAb ToRGBAb(const RGBA& c)
	{
		return RGBAb(ToUint8(c.red), ToUint8(c.green), ToUint8(c.blue), ToUint8(c.alpha));
	}

	RGBAb InterpolateColour(const Spectra& spectra, float relativeLifeLeft, const SpectrumBand& left, const SpectrumBand& right)
	{
		if (relativeLifeLeft <= left.relativeLifeTime)
		{
			return ToRGBAb(left.keyColour);
		}
		else if (relativeLifeLeft >= right.relativeLifeTime)
		{
			return ToRGBAb(right.keyColour);
		}

		float t = (relativeLifeLeft - left.relativeLifeTime) / (right.relativeLifeTime - left.relativeLifeTime);

		return ToRGBAb(
			RGBA(
				Lerp(left.keyColour.red, right.keyColour.red, t),
				Lerp(left.keyColour.green, right.keyColour.green, t),
				Lerp(left.keyColour.blue, right.keyColour.blue, t),
				Lerp(left.keyColour.alpha, right.keyColour.alpha, t)
				));

	}

	RGBAb SearchColour(const Spectra& spectra, float relativeLifeLeft, size_t start, size_t end)
	{
		size_t middle = (start + end) >> 1;
		if (middle == start)
		{
			return InterpolateColour(spectra, relativeLifeLeft, spectra[start], spectra[end]);
		}
		else if (spectra[middle] < relativeLifeLeft)
		{
			return SearchColour(spectra, relativeLifeLeft, middle, end);
		}
		else
		{
			return SearchColour(spectra, relativeLifeLeft, start, middle);
		}
	}

	RGBAb GetColour(const Spectra& spectra, float relativeLifeLeft)
	{
		if (spectra.empty()) return RGBAb(255, 255, 255);

		size_t start = 0;
		size_t end = spectra.size();
		size_t middle = (start + end) >> 1;

		if (spectra[middle] < relativeLifeLeft)
		{
			return SearchColour(spectra, relativeLifeLeft, middle, end);
		}
		else
		{
			return SearchColour(spectra, relativeLifeLeft, start, middle);
		}
	}

	void Seed(int64 value)
	{
		if (value == 0)
		{
			value = Time::TickCount();
		}

		uint32 a = (uint32)(0x00000000FFFFFFFF & value);
		uint32 b = (uint32)(value >> 32);
		rng.seed(a ^ b);
	}

	int32 Rand(int32 modulus)
	{
		if (modulus <= 2) Throw(0, "Rococo::Random::Rand(): Bad modulus - %d", modulus);
		return rng() % modulus;
	}

	float AnyFloat(float minValue, float maxValue)
	{
		float32 range = maxValue - minValue;
		float f = rng() / (float)rng.max();
		return range * f + minValue;
	}

	ROCOCO_INTERFACE ICloud
	{
		virtual void Free() = 0;
		virtual void GetParticles(IRenderer& renderer, cr_vec3 origin) = 0;
		virtual void SetSpectrum(const Spectra& spectra) = 0;
	};

	struct Dust : public ICloud
	{
		Vec3 origin = { 0,0,0 };
		Time::ticks lastTick = 0;
		RGBAb colour;
		float velCountdown = 0;
		Metres meanParticleSize;
		Metres range;
		Metres minHeight;
		Metres maxHeight;
		Vec3 windDirection{ 0,0,0 };
		Spectra spectra;

		struct DustParticle : public ParticleVertex
		{
			Vec3 velocity{ 0,0,-0.1f };
			float life{ 0 };
		};

		std::vector<DustParticle> dust;

		Dust(int32 nParticles, Metres _meanParticleSize, Metres _range, Metres _minHeight, Metres _maxHeight, RGBAb _colour) :
			dust(nParticles),
			range(_range),
			minHeight(_minHeight),
			maxHeight(_maxHeight), 
			meanParticleSize(_meanParticleSize),
			colour(_colour)
		{
			for (auto& d : dust)
			{
				Phoenix(d);
			}
		}

		void SetSpectrum(const Spectra& spectra)
		{
			ANON::SetSpectrum(this->spectra, spectra);
		}

		void Decimate(float dt)
		{
			for (auto& d : dust)
			{
				d.life -= dt;
				if (d.life < 0 || d.worldPosition.z < minHeight || d.worldPosition.z > maxHeight)
				{
					Phoenix(d);
				}
			}
		}

		void ApplyForces(float dt)
		{
			for (auto& d : dust)
			{
				velCountdown -= dt;
				if (velCountdown < 0)
				{
					velCountdown = 0.125f;

					d.velocity += { AnyFloat(-0.1f, 0.1f), AnyFloat(-0.1f, 0.1f), AnyFloat(0.025f, -0.05f) };
				}
			}
		}

		void FollowTrajectories(float dt)
		{
			for (auto& d : dust)
			{
				d.worldPosition += d.velocity * dt;
			}
		}

		void AdvanceSimulation(float dt)
		{
			Decimate(dt);
			ApplyForces(dt);
			FollowTrajectories(dt);
		}

		void Phoenix(DustParticle& d)
		{
			d.worldPosition.x = AnyFloat(-range, range);
			d.worldPosition.y = AnyFloat(-range, range);
			d.worldPosition.z = AnyFloat(minHeight, maxHeight);
			d.velocity = { 0,0,-0.05f };

			float s = AnyFloat(0.5f, 1.5f);

			d.geometry = { s * meanParticleSize, 0, 0, 0 };
			d.colour = colour;
			d.worldPosition += origin;
			d.life = AnyFloat(3.0f, 5.0f);
		}

		void Free() override
		{
			delete this;
		}

		void GetParticles(IRenderer& renderer, const Vec3& origin) override
		{
			this->origin = origin;

			Time::ticks now = Time::TickCount();

			auto delta = now - lastTick;

			Time::ticks hz = Time::TickHz();

			Time::ticks tenMs = hz / 100;

			if (delta > tenMs)
			{
				lastTick = now;

				float dt = min(0.1f, delta / (float)hz);
				AdvanceSimulation(dt);
			}

			for (auto& d : dust)
			{
				renderer.Particles().AddFog(d);
			}
		}
	};

	struct Flame : public ICloud
	{
		const FlameDef def;
		Vec3 origin = Vec3{ 0,0,0 };
		Vec3 attractor = Vec3{ 0,0,1 };
		float nextTurbulence = 0;
		Spectra spectra;

		void SetSpectrum(const Spectra& spectra)
		{
			ANON::SetSpectrum(this->spectra, spectra);
		}

		Time::ticks lastTick = 0;

		struct Fireball
		{
			ParticleVertex p;
			Vec3 velocity;
			float startRadius;
			float endRadius;
			float life = 0.5f;
		};

		std::vector<Fireball> fire;

		Flame(const FlameDef& _def) :
			def(_def),
			fire(_def.particleCount)
		{
			for (auto& fb : fire)
			{
				Phoenix(fb);
				fb.life = AnyFloat(0.1f, 0.5f);
				fb.p.geometry = { 0.1f,0,0,0 };
			}
		}

		void Phoenix(Fireball& fb)
		{
			fb.life = AnyFloat(def.minLifeSpan, def.maxLifeSpan);
			fb.p.worldPosition.x = AnyFloat(-1.0f, 1.0f) * def.initialSpawnPosRange;
			fb.p.worldPosition.y = AnyFloat(-1.0f, 1.0f) * def.initialSpawnPosRange;
			fb.p.worldPosition.z = AnyFloat(-0.01f, 0.01f);
			fb.velocity.x = AnyFloat(-1.0f, 1.0f) * def.initialVelocityRange;
			fb.velocity.y = AnyFloat(-1.0f, 1.0f)* def.initialVelocityRange;
			fb.velocity.z = def.jetSpeed;

			Vec3 current = attractor - fb.p.worldPosition - origin;

			fb.velocity = Lerp(current, fb.velocity, 0.7f);

			fb.p.worldPosition += origin;
			fb.p.colour = RGBAb(0, 0, 255, 32);
			fb.startRadius = AnyFloat(def.minStartParticleSize, def.maxStartParticleSize);
			fb.endRadius = AnyFloat(def.minEndParticleSize, def.maxEndParticleSize);
		}

		void ResetAttractor()
		{
			this->attractor = origin + Vec3{ AnyFloat(-1.0f,1.0f), AnyFloat(-1.0f,1.0f), 0.0f } *def.attractorSpawnPosRange + Vec3{ 0, 0, def.attractorHeight };
		}

		void UpdateTurbulence(float dt)
		{
			float range = Length(origin - attractor);
			if (range > def.attractorMaxRange || range < def.attractorMinRange)
			{
				ResetAttractor();
			}

			nextTurbulence += dt;

			if (nextTurbulence > def.attractorAIduration)
			{
				nextTurbulence = 0;

				float choice = AnyFloat(0, 1.0f);
				if (choice < def.attractorResetProbability)
				{
					ResetAttractor();
				}
			}

			Vec3 delta = attractor - origin;
			this->attractor += def.attractorDriftFactor * dt * Vec3{ delta.x, delta.y, 0.0f };
			this->attractor += Vec3{ AnyFloat(-1.0f,1.0f), AnyFloat(-1.0f,1.0f), 0.0f } * def.attractorPerturbFactor;
		}

		void Cool(float dt)
		{
			for (auto& fb : fire)
			{
				float t = max(0.0f, 1.0f - (0.51f - fb.life) * 2.0f); // 0 to 1
				fb.p.colour = GetColour(spectra, t);
				fb.p.geometry.x = Lerp(fb.endRadius, fb.startRadius, t);
			}
		}

		void Decimate(float dt)
		{
			for (auto& fb : fire)
			{
				fb.life -= dt;
				if (fb.life < 0)
				{
					Phoenix(fb);
				}
			}
		}

		void ApplyForces(float dt)
		{
			for (auto& fb : fire)
			{
				Vec3 toAttractor = Normalize(attractor - fb.p.worldPosition);
				Vec3 convection{ toAttractor.x, toAttractor.y, 0 };
				fb.velocity += def.attractorForce * dt * convection;
			}
		}

		void FollowTrajectories(float dt)
		{
			for (auto& fb : fire)
			{
				fb.p.worldPosition += fb.velocity * dt;
			}
		}

		void AdvanceSimulation(float dt)
		{
			UpdateTurbulence(dt);
			Decimate(dt);
			ApplyForces(dt);
			FollowTrajectories(dt);
			Cool(dt);
		}

		void Free() override
		{
			delete this;
		}

		void GetParticles(IRenderer& renderer, const Vec3& origin) override
		{
			this->origin = origin;

			Time::ticks now = Time::TickCount();

			auto delta = now - lastTick;

			Time::ticks hz = Time::TickHz();

			Time::ticks tenMs = hz / 100;

			if (delta > tenMs)
			{
				lastTick = now;

				float dt = min(0.1f, delta / (float)hz);
				AdvanceSimulation(dt);
			}

			auto& particles = renderer.Particles();

			for (auto& fb : fire)
			{
				particles.AddPlasma(fb.p);
			}
		}
	};

	struct ParticleSystem : public IParticleSystemSupervisor
	{
		IRenderer& renderer;
		IInstances& instances;

		std::unordered_map<ID_ENTITY, ICloud*, Hash<ID_ENTITY>> clouds;

		ParticleSystem(IRenderer& _renderer, IInstances& _instances): renderer(_renderer), instances(_instances)
		{
		
		}

		~ParticleSystem()
		{
			Clear();
		}

		void Free() override
		{
			delete this;
		}

		void AddDust(int32 particles, Metres meanParticleSize, Metres range, Metres minHeight, Metres maxHeight, RGBAb colour, ID_ENTITY id) override
		{
			Snuff(id);

			auto d = new Dust(particles, meanParticleSize, range, minHeight, maxHeight, colour);

			clouds[id] = d;
		}

		void AddVerticalFlame(const FlameDef& flameDef, ID_ENTITY id) override
		{
			Snuff(id);

			auto f = new Flame(flameDef);

			clouds[id] = f;
		}

		void Clear() override
		{
			for (auto& c : clouds)
			{
				c.second->Free();
			}

			clouds.clear();
		}

		void Snuff(ID_ENTITY id) override
		{
			auto i = clouds.find(id);
			if (i != clouds.end())
			{
				i->second->Free();
				clouds.erase(i);
			}
		}

		void GetParticles(ID_ENTITY id, IRenderer& renderer) override
		{
			auto i = clouds.find(id);
			if (i != clouds.end())
			{
				Matrix4x4 model;
				if (instances.TryGetModelToWorldMatrix(id, model))
				{
					i->second->GetParticles(renderer, model.GetPosition());
				}
			}
		}

		Spectra spectra;
		bool dirty = true;

		void ApplySpectrum(ID_ENTITY id) override
		{
			if (dirty)
			{
				struct
				{
					bool operator ()(const SpectrumBand& a, const SpectrumBand& b)
					{
						return a.relativeLifeTime < b.relativeLifeTime;
					}
				} byTime;
				std::sort(spectra.begin(), spectra.end(), byTime);
			}

			auto i = clouds.find(id);
			if (i != clouds.end())
			{
				i->second->SetSpectrum(spectra);
			}
		}

		void ClearSpectrum() override
		{
			spectra.clear();
		}

		void SetSpectrum(const RGBA& colour, float relativeLifeTime)  override
		{
			if (relativeLifeTime < 0 || relativeLifeTime > 1.0f)
			{
				Throw(0, "ParticleSystem::SetSpectrum(...): [relativeLifeTime] domain is [0,1] ");
			}

			dirty = true;

			for (auto& b : spectra)
			{
				if (b.relativeLifeTime == relativeLifeTime)
				{
					b.keyColour = colour;
					return;
				}
			}

			spectra.push_back({ relativeLifeTime, colour });
		}
	};
}

namespace Rococo
{
	namespace Entities
	{
		IParticleSystemSupervisor* CreateParticleSystem(IRenderer& renderer, IInstances& instances)
		{
			return new ANON::ParticleSystem(renderer, instances);
		}
	}
}