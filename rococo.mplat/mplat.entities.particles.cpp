#include <rococo.mplat.h>
#include <rococo.maths.h>
#include <unordered_map>
#include <random>

using namespace Rococo;
using namespace Rococo::Entities;

namespace ANON
{
	std::default_random_engine rng;

	void Seed(int64 value)
	{
		if (value == 0)
		{
			value = OS::CpuTicks();
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

	ROCOCOAPI ICloud
	{
		virtual void Free() = 0;
		virtual void GetParticles(IRenderer& renderer, cr_vec3 origin) = 0;
	};

	struct Dust : public ICloud
	{
		Vec3 origin = { 0,0,0 };
		OS::ticks lastTick = 0;
		float velCountdown = 0;
		Metres range;
		Metres minHeight;
		Metres maxHeight;
		Vec3 windDirection{ 0,0,0 };

		struct DustParticle : public ParticleVertex
		{
			Vec3 velocity{ 0,0,-0.1f };
			float life{ 0 };
		};

		std::vector<DustParticle> dust;

		Dust(int32 nParticles, Metres _range, Metres _minHeight, Metres _maxHeight) :
			dust(nParticles), range(_range), minHeight(_minHeight), maxHeight(_maxHeight)
		{
			for (auto& d : dust)
			{
				Phoenix(d);
			}
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
			d.geometry = { AnyFloat(0.01f, 0.02f), 0, 0, 0 };
			d.colour = RGBAb(255, 255, 255, 255);
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

			OS::ticks now = OS::CpuTicks();

			auto delta = now - lastTick;

			OS::ticks hz = OS::CpuHz();

			OS::ticks tenMs = hz / 100;

			if (delta > tenMs)
			{
				lastTick = now;

				float dt = min(0.1f, delta / (float)hz);
				AdvanceSimulation(dt);
			}

			for (auto& d : dust)
			{
				uint8 col = Rand(255);
				d.colour = RGBAb(col, col, col, 255);
				renderer.AddParticle(d);
			}
		}
	};

	struct Flame : public ICloud
	{
		const FlameDef def;
		Vec3 origin = Vec3{ 0,0,0 };
		Vec3 attractor = Vec3{ 0,0,1 };
		float nextTurbulence = 0;

		OS::ticks lastTick = 0;

		struct Fireball
		{
			ParticleVertex p;
			Vec3 velocity;
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
				fb.life = 0;
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
			fb.velocity.z = 1.0f;

			Vec3 current = attractor - fb.p.worldPosition - origin;

			fb.velocity = Lerp(current, fb.velocity, 0.7f);

			fb.p.worldPosition += origin;
			fb.p.colour = RGBAb(0, 0, 255, 32);
			fb.p.geometry = { AnyFloat(def.minParticleSize, def.maxParticleSize),0,0,0 };
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

		RGBAb GetLightForTemperature(float temperature, float alpha)
		{
			float red = 0;
			if (temperature > 750.0f)
			{
				red = min(255.0f, temperature - 750.0f);
				if (temperature > 955.0f)
				{
					red *= expf((temperature - 955.0f) / -1000.0f);
				}
			}

			float green = 0;
			if (temperature > 1000.0f)
			{
				green = min(192.0f, 0.5f * (temperature - 1000.0f));
				if (temperature > 1500.0f)
				{
					green *= expf((temperature - 1500.0f) / -1500.0f);
				}
			}

			float blue = 0;
			if (temperature > 1500.0f)
			{
				blue = min(255.0f, 0.25f * (temperature - 1500.0f));
				if (temperature > 2500.0f)
				{
					blue *= expf((temperature - 2500.0f) / -2500.0f);
				}
			}

			return RGBAb((uint8)(int32)red, (uint8)(int32)green, (uint8)(int32)blue, (uint8)(int32)(255.0f * alpha));
		}

		void Cool(float dt)
		{
			for (auto& fb : fire)
			{
				float tempFalloff = max(0.0f, 1.0f - (0.51f - fb.life) * 2.0f); // 0 to 1
				float temperature = tempFalloff * def.temperatureKelvin;
				fb.p.colour = GetLightForTemperature(temperature, tempFalloff);
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

			OS::ticks now = OS::CpuTicks();

			auto delta = now - lastTick;

			OS::ticks hz = OS::CpuHz();

			OS::ticks tenMs = hz / 100;

			ParticleVertex illumination;
			illumination.colour = GetLightForTemperature(def.temperatureKelvin, 0.75f);
			illumination.geometry.x = 0.0001f * def.particleCount;
			illumination.worldPosition = Lerp(origin, attractor, 0.15f);
			//		renderer.AddParticle(illumination);

			if (delta > tenMs)
			{
				lastTick = now;

				float dt = min(0.1f, delta / (float)hz);
				AdvanceSimulation(dt);
			}

			for (auto& fb : fire)
			{
				renderer.AddParticle(fb.p);
			}
		}
	};

	struct ParticleSystem : public IParticleSystemSupervisor
	{
		IRenderer& renderer;
		IInstances& instances;

		std::unordered_map<ID_ENTITY, ICloud*, ID_ENTITY> clouds;

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

		void AddDust(int32 particles, Metres range, Metres minHeight, Metres maxHeight, ID_ENTITY id) override
		{
			Snuff(id);

			auto d = new Dust(particles, range, minHeight, maxHeight);

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