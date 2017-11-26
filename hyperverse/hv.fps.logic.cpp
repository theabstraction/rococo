#include "hv.h"
#include "hv.events.h"
#include <unordered_map>
#include <rococo.maths.h>

#include <algorithm>

using namespace HV;
using namespace HV::Events;
using namespace HV::Events::Player;

using namespace Rococo::Entities;
using namespace Rococo::Graphics;

typedef std::unordered_map<ISector*, int32> TSectorSet;

void Insert(TSectorSet& set, ISector* sector)
{
	set.insert(std::make_pair(sector, 0));
}

bool IsPresent(const TSectorSet& set, ISector* sector)
{
	auto i = set.find(sector);
	return i != set.end();
}

class FPSControl
{
private:
   void OnForward(bool start)
   {
      isMovingForward = start;
      isAutoRun = false;
   }

   void OnBackward(bool start)
   {
      isMovingBackward = start;
      isAutoRun = false;
   }


   void OnStraffeLeft(bool start)
   {
      isMovingLeft = start;
   }


   void OnStraffeRight(bool start)
   {
      isMovingRight = start;
   }

   void OnJump(bool start)
   {

   }

   void OnAutoRun(bool start)
   {
      isAutoRun = true;
   }

public:
   typedef void (FPSControl::*ACTION_FUNCTION)(bool start);
   static std::unordered_map<std::string, ACTION_FUNCTION> nameToAction;

   bool isMovingForward{ false };
   bool isMovingBackward{ false };
   bool isMovingLeft{ false };
   bool isMovingRight{ false };
   bool isAutoRun{ false };

   float headingDelta{ 0 };
   float elevationDelta{ 0 };

   Vec3 speeds{ 0,0,0 };

   FPSControl()
   {
   }

   void Clear()
   {
      isMovingBackward = isMovingForward = isMovingLeft = isMovingRight = false;
      headingDelta = elevationDelta = 0;
   }

   bool Append(OnPlayerActionEvent& ev)
   {
      auto i = nameToAction.find(ev.Name);
      if (i != nameToAction.end())
      {
         ((*this).*(i->second))(ev.start);
         return true;
      }
      else
      {
         return false;
      }
   }

   void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
   {
      headingDelta += 0.25f * delta.x;
      elevationDelta -= delta.y;
   }

   void GetPlayerDelta(ID_ENTITY playerId, float dt, Entities::MoveMobileArgs& mm, Degrees& viewElevationDelta)
   {
      float forwardDelta = 0;
      if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
      if (isMovingBackward) forwardDelta -= 1.0f;

      float straffeDelta = 0;
      if (isMovingLeft) straffeDelta -= 1.0f;
      if (isMovingRight) straffeDelta += 1.0f;

      if (forwardDelta != 0 || straffeDelta != 0 || headingDelta != 0)
      {
         mm.fowardDelta = dt * ((forwardDelta > 0) ? forwardDelta * speeds.x : forwardDelta * speeds.z);
         mm.straffeDelta = dt * straffeDelta * speeds.y;
         mm.entityId = playerId;
         mm.delta = { Degrees{ headingDelta }, Degrees{ 0 }, Degrees{ 0 } };
         headingDelta = 0;
      }

      viewElevationDelta = Degrees{ -elevationDelta };
      elevationDelta = 0;
   }
};

std::unordered_map<std::string, FPSControl::ACTION_FUNCTION> FPSControl::nameToAction =
{
   { "move.fps.forward",         &FPSControl::OnForward },
   { "move.fps.backward",        &FPSControl::OnBackward },
   { "move.fps.straffeleft",     &FPSControl::OnStraffeLeft },
   { "move.fps.strafferight",    &FPSControl::OnStraffeRight },
   { "move.fps.jump",            &FPSControl::OnJump },
   { "move.fps.autorun",         &FPSControl::OnAutoRun }
};

struct FPSGameLogic : public IGameModeSupervisor, public IUIElement, public IScenePopulator
{
	Cosmos& e;

	FPSControl fpsControl;

	bool isCursorActive = true;

	FPSGameLogic(Cosmos& _e) : e(_e)
	{
		fpsControl.speeds = Vec3{ 10.0f, 5.0f, 5.0f };

		e.platform.scene.SetPopulator(this);
	}

	~FPSGameLogic()
	{
		e.platform.gui.UnregisterPopulator(this);
		e.platform.scene.SetPopulator(nullptr);
	}

	void Activate()
	{
		isCursorActive = false;
		e.platform.renderer.SetCursorVisibility(false);
		fpsControl.Clear();
	}

	void Deactivate()
	{
		isCursorActive = true;
		e.platform.renderer.SetCursorVisibility(true);
		fpsControl.Clear();
	}

	bool Append(OnPlayerActionEvent& ev)
	{
		return fpsControl.Append(ev);
	}

	void Free() override
	{
		delete this;
	}

	struct SectorToScene : public IEventCallback<VisibleSector>, public IEventCallback<const ID_ENTITY>
	{
		ISceneBuilder& builder;
		std::vector<ISector*>& foundSectors;

		virtual void OnEvent(const ID_ENTITY& id)
		{
			builder.AddStatics(id);
		}

		virtual void OnEvent(VisibleSector& sv)
		{
			sv.sector.ForEveryObjectInSector(*this);
			foundSectors.push_back(&sv.sector);
		}

		SectorToScene(ISceneBuilder& _builder, std::vector<ISector*>& _foundSectors) :
			builder(_builder), foundSectors(_foundSectors) {}
	};

	typedef std::vector<ISector*> TSectorVector;

	TSectorVector illuminatedRooms;
	TSectorVector shadowCasterSectors;

	virtual void PopulateShadowCasters(ISceneBuilder& sb, const DepthRenderData& drd)  override
	{
		sb.Clear();
		shadowCasterSectors.clear();
		SectorToScene addToScene(sb, shadowCasterSectors);
		auto nSectors = e.sectors.ForEverySectorVisibleBy(drd.worldToScreen, drd.eye, drd.direction, addToScene);
		if (nSectors == 0)
		{
			// Nothing rendered
		}
	}

	TSectorSet visibleSectorsThisTimestep;

	void ComputeVisibleSectorsThisTimestep()
	{
		visibleSectorsThisTimestep.clear();

		Vec3 eye;
		e.platform.camera.GetPosition(eye);

		Matrix4x4 world;
		e.platform.camera.GetWorld(world);

		Matrix4x4 camera;
		e.platform.camera.GetWorldAndProj(camera);

		Vec3 dir{ -world.row2.x, -world.row2.y, -world.row2.z };

		struct : IEventCallback<VisibleSector>
		{
			TSectorSet* visibleSectorsThisTimestep;
			ISector* home{ nullptr };

			void OnEvent(VisibleSector& v) override
			{
				if (!home) home = &v.sector;
				Insert(*visibleSectorsThisTimestep, &v.sector);
			}
		} builder;

		builder.visibleSectorsThisTimestep = &visibleSectorsThisTimestep;

		auto nSectors = e.sectors.ForEverySectorVisibleBy(camera, eye, dir, builder);
		if (nSectors == 0)
		{
			// Nothing rendered
		}

		if (builder.home)
		{
			builder.home->SyncEnvironmentMapToSector();
		}
	}

	TSectorVector visibleSectors;

	void PopulateScene(ISceneBuilder& sb) override
	{
		sb.Clear();
		visibleSectors.clear();

		SectorToScene addToScene(sb, visibleSectors);
		for (auto s : visibleSectorsThisTimestep)
		{
			addToScene.OnEvent(VisibleSector{ *s.first });
		}

		e.platform.renderer.ClearParticles();

		struct :IEventCallback<const ID_ENTITY>
		{
			Platform* platform;

			void OnEvent(const ID_ENTITY& id) override
			{
				platform->particles.GetParticles(id, platform->renderer);
			}
		} addParticles;

		addParticles.platform = &e.platform;

		if (!visibleSectors.empty())
		{
			// Add particles in curent sector and neighbouring visible sectors

			auto& s0 = *visibleSectors[0];
			s0.ForEveryObjectInSector(addParticles);

			size_t gapCount;
			auto gaps = s0.Gaps(gapCount);
			for (size_t i = 0; i < gapCount; ++i)
			{
				auto j = std::find(visibleSectors.begin(), visibleSectors.end(), gaps->other);
				if (j != visibleSectors.end())
				{
					(*j)->ForEveryObjectInSector(addParticles);
				}
			}
		}
	}

	bool IsPathAcrossGap(cr_vec3 start, cr_vec3 end, ISector& from, ISector& to)
	{
		size_t count;
		auto gaps = from.Gaps(count);

		for (size_t i = 0; i < count; ++i)
		{
			auto& gap = gaps[i];
			if (gap.other == &to)
			{
				Edge edge{ ToVec3(gap.a, 0), ToVec3(gap.b, 0) };
				Sphere playerSphere{ Flatten(start), 0.9_metres };
				auto c = Rococo::CollideEdgeAndSphere(edge, playerSphere, Flatten(end));

				if (c.contactType == ContactType_Edge || c.contactType == ContactType_Penetration)
				{
					return true;
				}
			}
		}

		return false;
	}

	struct CollisionParameters
	{
		Vec3 start;
		Vec3 end;
		ID_ENTITY playerId;
		float dt;
		float jumpSpeed;
	};

	Vec3 ToVec3(cr_vec2 p, float z)
	{
		return Vec3{ p.x, p.y, z };
	}

	Vec3 Flatten(cr_vec3 p)
	{
		return Vec3{ p.x, p.y, 0 };
	}

	Vec3 ApplyElasticCollision(Vec3 normal, const CollisionParameters& cp, float t, cr_vec3 touchPoint, const Sphere& sphere)
	{
		// N.B normal must oppose our direction
		Vec3 trajectory = cp.end - cp.start;

		if (Dot(normal, trajectory) > 0)
		{
			normal = -normal;
		}

		// m is mass of sphere.
		// Incoming velocity v is (end - start)
		// Outgoing velocity V is (Vx Vy)

		// For elastic collision, |v| = |V|, v.v = V.V

		// Momentum impulse = m.J.normal where J is the scalar magnitude of the impulse

		// mV = mv + m.J.normal

		// V = v + J.normal
		// V.V = v.v + J.J + 2.v.normal.J. Now consider v.v - V.V = 0:
		// J.J = -2.J.(v.normal)
		// J = -2.v.normal

		float J = -2.0f * Dot(trajectory, normal);

		t = 0.5f; // That should stop feedback

		Vec3 bounceTrajectory = trajectory + J * normal;
		Vec3 delta = (t * trajectory) + (1.0f - t) * bounceTrajectory;
		Vec3 bounceTo = cp.start + delta;
		return bounceTo;
	}

	Vec3 ComputeWallCollision(const CollisionParameters& cp, cr_vec2 p, cr_vec2 q, float& t)
	{
		Edge edge{ ToVec3(p, 0), ToVec3(q, 0) };
		Sphere playerSphere{ Flatten(cp.start), 0.9_metres };
		auto c = Rococo::CollideEdgeAndSphere(edge, playerSphere, Flatten(cp.end));

		if (c.contactType == ContactType_Penetration)
		{
			// Tangled in the mesh
			Vec3 displacement = c.touchPoint - Flatten(cp.start);

			// We could be leaving a line behind (safe), or we have further penetration (bad)
			if (Dot(displacement, cp.end - cp.start) > 0)
			{
				// Bad, we are making penetration worse
				t = 0;
				return cp.start;
			}
			else
			{
				// Leaving point behind
				t = 1.0f;
				return cp.end;
			}

			return cp.start;
		}

		if (c.contactType == ContactType_None)
		{
			t = 1.0f;
			return cp.end;
		}

		t = c.t;

		if (c.t > 1)
		{
			t = 1.0f;
			return cp.end;
		}

		if (c.contactType == ContactType_Edge) // see diagrams/collision.sphere.vs.edge.png
		{
			/*  Collision normal = | (q -p) *  K | */
			Vec3 pq = ToVec3(q - p, 0);

			Vec3 N = Cross(pq, Vec3{ 0, 0, 1 });

			float modN = Length(N);
			if (modN < 0.01f)
			{
				if (Rococo::OS::IsDebugging())
				{
					// Fix the geometry
					Rococo::OS::TripDebugger();
				}

				t = 1.1f;
				return cp.end;
			}

			Vec3 normal = N * (1.0f / modN);

			return ApplyElasticCollision(normal, cp, c.t, c.touchPoint, playerSphere);
		}
		else if (c.contactType == ContactType_Vertex)
		{
			/*   Collision normal = | (C - p)|  */
			Vec3 C = Flatten(c.t * (cp.end - cp.start) + cp.start);
			Vec3 N = C - c.touchPoint;

			float modN = Length(N);
			if (modN < 0.01f)
			{
				// Degenerate case, stop collision
				return cp.start;
			}

			if (Dot(N, cp.end - cp.start) < 0)
			{
				Vec3 normal = N * (1.0f / modN);
				t = c.t;
				return ApplyElasticCollision(normal, cp, c.t, c.touchPoint, playerSphere);
			}
			else
			{
				t = 1.0f;
				return cp.end;
			}
		}
		else
		{
			t = 0;
			// Degenerate case, stop collision
			return cp.start;
		}

		t = 1.0f;
		return cp.end;
	}

	Vec3 ComputeWallCollisionInSector(const CollisionParameters& cp, ISector& sector, bool scanGaps = true)
	{
		if (cp.start == cp.end) return cp.end;

		size_t count;
		auto segments = sector.GetWallSegments(count);

		size_t nVertices;
		auto v = sector.WallVertices(nVertices);

		float collisionTime = 1.0f;
		Vec3 destinationPoint = cp.end;

		for (size_t i = 0; i < count; ++i)
		{
			auto& s = segments[i];
			s.perimeterIndexStart;
			s.perimeterIndexEnd;

			auto p = v[s.perimeterIndexStart];
			auto q = v[s.perimeterIndexEnd];

			float ithTime = 1.0f;
			Vec3 ithResult = ComputeWallCollision(cp, p, q, ithTime);

			if (ithTime < collisionTime)
			{
				collisionTime = ithTime;
				destinationPoint = ithResult;
			}
		}

		size_t barrierCount;
		auto b = sector.Barriers(barrierCount);
		for (size_t i = 0; i < barrierCount; ++i)
		{
			float ithTime = 1.0f;
			Vec3 ithResult = ComputeWallCollision(cp, b[i].p, b[i].q, ithTime);

			if (ithTime < collisionTime)
			{
				collisionTime = ithTime;
				destinationPoint = ithResult;
			}
		}

		if (scanGaps)
		{
			auto* gaps = sector.Gaps(count);
			for (size_t i = 0; i < count; ++i)
			{
				auto& g = gaps[i];

				float ithTime = 1.0f;
				Vec3 ithResult = ComputeWallCollision(cp, g.a, g.b, ithTime);
				if (ithTime < collisionTime)
				{
					// A gap collision means player bounding sphere has entered a neighbouring sector
					// So we must check that we have not interfaced with materials in that sector

					Vec3 foreignTarget = ComputeWallCollisionInSector(cp, *g.other, false);
					if (foreignTarget != cp.end)
					{
						// Stop the movement dead
						return cp.start;
					}

					if (!IsAscendable(cp.start, cp.end, sector, *g.other))
					{
						collisionTime = ithTime;
						destinationPoint = ithResult;
					}
				}
			}
		}

		return destinationPoint;
	}

	bool IsAscendable(cr_vec3 start, cr_vec3 end, ISector& from, ISector& to)
	{
		if (from.IsCorridor() || to.IsCorridor())
		{
			// Corridors are asendable at both ends.
			return true;
		}

		float ground = end.z;

		float z0 = to.Z0();
		float z1 = to.Z1();

		const float maxClimbableHeight = 1.65f;

		if ((ground > z0 - maxClimbableHeight) && end.z < (z1 - 2.48_metres))
		{
			return true;
		}

		return false;
	}

	Vec3 ComputeFloorCollisionInSector(const CollisionParameters& cp, ISector& sector, float& jumpSpeed)
	{
		float h = GetHeightAtPointInSector(cp.end, sector);
		if (cp.end.z < h)
		{
			jumpSpeed = 0.0f;
			return Vec3{ cp.end.x, cp.end.y, h };
		}
		else if (cp.end.z > h)
		{
			Vec3 g = { 0, 0, -9.81f };
			Vec3 afterGravity = cp.end + (0.5f * cp.dt * cp.dt * g) + (cp.dt * Vec3{ 0, 0, cp.jumpSpeed });

			if (afterGravity.z < h)
			{
				jumpSpeed = 0.0f;
				return Vec3{ cp.end.x, cp.end.y, h };
			}
			else
			{
				jumpSpeed = cp.jumpSpeed + g.z * cp.dt;
				return afterGravity;
			}
		}

		jumpSpeed = cp.jumpSpeed;
		return cp.end;
	}

	Vec3 GetRandomPointInSector(ISector& sector)
	{
		auto f = sector.FloorVertices();

		size_t nTriangles = f.VertexCount / 3;

		size_t index = rand() % nTriangles;

		auto p1 = f.v[3 * index].position;
		auto p2 = f.v[3 * index + 1].position;
		auto p3 = f.v[3 * index + 2].position;

		float t = 0.5f * (rand() / (float)RAND_MAX) + 0.25f; // Gives random between 0.25 and 0.75

		Vec3 mid1_2 = 0.5f * (p1 + p2);
		Vec3 mid = Lerp(mid1_2, p3, t);
		return mid;
	}

	Vec3 CorrectPosition(const CollisionParameters& cp, float& jumpSpeed)
	{
		jumpSpeed = cp.jumpSpeed;

		auto* fromSector = e.sectors.GetFirstSectorContainingPoint({ cp.start.x, cp.start.y });
		if (!fromSector)
		{
			// Generally it is a bad thing for the start point to be outside all sectors
			// We must have made a mistake setting up that position
			// First sector is usually the entrance, so port to there
			if (e.sectors.begin() != e.sectors.end())
			{
				ISector* firstSector = *e.sectors.begin();
				Vec3 p = GetRandomPointInSector(*firstSector);
				CollisionParameters cp2 = { p, p, cp.playerId, cp.dt, cp.jumpSpeed };
				return ComputeFloorCollisionInSector(cp2, *firstSector, jumpSpeed);
			}
			else
			{
				return cp.start;
			}
		}

		auto* toSector = e.sectors.GetFirstSectorContainingPoint({ cp.end.x, cp.end.y });
		if (!toSector)
		{
			// Totally disallow any movement that would take us out of the sector cosmology
			return cp.start;
		}

		if (fromSector != toSector)
		{
			// Sector transition. We are only allowed to transition through a gap that connects sectors

			auto result = ComputeWallCollisionInSector(cp, *fromSector);
			if (result != cp.end)
			{
				// If we are transitioning but we first collide with walls in the sector from which we leave, then stop things short
				return cp.start;
			}

			result = ComputeWallCollisionInSector(cp, *toSector);
			if (result != cp.end)
			{
				// If we are transitioning but we collide with walls in the sector to from we head, then stop things short
				return  cp.start;
			}

			// Okay no collisions, but we still have to be moving through a gap
			if (!IsPathAcrossGap(cp.start, cp.end, *fromSector, *toSector))
			{
				return cp.start;
			}

			if (!IsAscendable(cp.start, cp.end, *fromSector, *toSector))
			{
				return cp.start;
			}

			Vec3 finalPos = ComputeFloorCollisionInSector(cp, *toSector, jumpSpeed);
			return finalPos;
		}
		else
		{
			auto result = ComputeWallCollisionInSector(cp, *fromSector);

			result.z = cp.start.z;
			CollisionParameters cp2 = { cp.start, result, cp.playerId, cp.dt, cp.jumpSpeed };
			return ComputeFloorCollisionInSector(cp2, *fromSector, jumpSpeed);
		}
	}

	bool IsLightVisible(const LightSpec& light)
	{
		illuminatedRooms.clear();

		struct : IEventCallback<VisibleSector>
		{
			TSectorVector* illuminatedRooms;
			void OnEvent(VisibleSector& v)
			{
				illuminatedRooms->push_back(&v.sector);
			}
		} build;

		build.illuminatedRooms = &illuminatedRooms;

		Vec3 normDirection;
		if (!TryNormalize(light.direction, normDirection))
		{
			return false;
		}

		Matrix4x4 directionToCameraRot = RotateDirectionToNegZ(normDirection);
		Matrix4x4 cameraToDirectionRot = TransposeMatrix(directionToCameraRot);	
		Matrix4x4 worldToCamera = directionToCameraRot * Matrix4x4::Translate(-light.position);

		Matrix4x4 cameraToScreen = Matrix4x4::GetRHProjectionMatrix(light.fov, 1.0f, light.nearPlane, light.farPlane);

		Matrix4x4 worldToScreen = cameraToScreen * worldToCamera;

		e.sectors.ForEverySectorVisibleBy(worldToScreen, light.position, normDirection, build);

		bool foundOne = false;

		for (auto s : illuminatedRooms)
		{
			if (IsPresent(visibleSectorsThisTimestep, s))
			{
				return true;
			}
		}

		return false;
	}

	void UpdatePlayer(float dt)
	{
		auto* player = e.players.GetPlayer(0);
		auto id = player->GetPlayerEntity();

		Degrees viewElevationDelta;

		Entities::MoveMobileArgs mm;
		fpsControl.GetPlayerDelta(id, dt, mm, viewElevationDelta);

		mm.fowardDelta *= Sq(player->DuckFactor());
		mm.straffeDelta *= Sq(player->DuckFactor());

		auto pe = e.platform.instances.GetEntity(id);

		Vec3 before = pe->Position();

		e.platform.mobiles.TryMoveMobile(mm);

		Vec3 after = pe->Position();

		CollisionParameters cp{ before, after, id, dt, player->JumpSpeed() };
		Vec3 final = CorrectPosition(cp, player->JumpSpeed());

		float dz = final.z - before.z;

		if (dz < 0)
		{
			// Going down
			player->DuckFactor() = 1.0f;
		}
		else if (dz == 0)
		{

		}
		else
		{
			player->DuckFactor() = 1.0f - dz / player->Height();
			if (player->DuckFactor() < 0.5f)
			{
				player->DuckFactor() = 0.5f;
			}
		}

		pe->Model().SetPosition(final);

		Vec3 playerPosToCamera = Vec3{ 0, 0, player->Height() * player->DuckFactor() };

		if (player->DuckFactor() < 1.0f)
		{
			player->DuckFactor() += 0.25f * dt;

			if (player->DuckFactor() > 1.0f)
			{
				player->DuckFactor() = 1.0f;
			}
		}

		Vec3 playerPosToLight= Vec3{ 0.2f, 0, player->Height() * player->DuckFactor() - 0.75f };

		FPSAngles angles;
		e.platform.mobiles.GetAngles(id, angles);

		auto Rz = Matrix4x4::RotateRHAnticlockwiseZ(angles.heading);

		Vec3 playerPosToLightWorld;
		TransformPositions(&playerPosToLight, 1, Rz, &playerPosToLightWorld);

		e.platform.camera.ElevateView(id, viewElevationDelta, playerPosToCamera);

		Matrix4x4 m;
		e.platform.camera.GetWorld(m);
		Vec3 dir{ -m.row2.x, -m.row2.y, -m.row2.z };

		LightSpec light;
		light.ambience = RGBA(0.0625f, 0.0625f, 0.0625f, 1.0f);
		light.diffuse = RGBA(4.0f, 4.0f, 4.0f, 1.0f);
		light.direction = dir;
		light.position = final + playerPosToLightWorld;
		light.cutoffPower = 32.0f;
		light.cutoffAngle = 30_degrees;
		light.fov = 90_degrees;
		light.attenuation = -0.35f;
		light.nearPlane = 0.02_metres;
		light.farPlane = 50_metres;
		light.fogConstant = -0.1f;

		lightBuilder.push_back(light);

		Vec3 eye;
		e.platform.camera.GetPosition(eye);

		Matrix4x4 world;
		e.platform.camera.GetWorld(world);

		Matrix4x4 camera;
		e.platform.camera.GetWorldAndProj(camera);

		struct : IEventCallback<VisibleSector>
		{
			FPSGameLogic* This;

			virtual void OnEvent(VisibleSector& v)
			{
				size_t nLights;
				auto sl = v.sector.Lights(nLights);
				for (size_t i = 0; i < nLights; ++i)
				{
					This->lightBuilder.push_back(sl[i]);
				}
			}
		} addLightsFromeSector;

		addLightsFromeSector.This = this;

		auto nSectors = e.sectors.ForEverySectorVisibleBy(camera, eye, dir, addLightsFromeSector);
		if (nSectors == 0)
		{
			// Nothing rendered
		}

		struct
		{
			bool operator ()(const LightSpec& a, const LightSpec& b) const
			{
				return LengthSq(a.position - eye) < LengthSq(b.position - eye);
			}

			Vec3 eye;
		} byDistanceFromPlayer{ final };

		std::sort(lightBuilder.begin(), lightBuilder.end(), byDistanceFromPlayer);

		e.platform.scene.Builder().ClearLights();

		int32 targetIndex = 0;

		for (int32 i = 0; i < lightBuilder.size(); ++i)
		{
			if (IsLightVisible(lightBuilder[i]))
			{
				e.platform.scene.Builder().SetLight(lightBuilder[i], targetIndex++);
			}
		}

		lightBuilder.clear();
	}

	std::vector<LightSpec> lightBuilder;

	float spinningLightTheta = 0;

	void UpdateAI(const IUltraClock& clock) override
	{
		e.platform.gui.RegisterPopulator("fps", this);
		UpdatePlayer(clock.DT());
		ComputeVisibleSectorsThisTimestep();
	}

	bool OnKeyboardEvent(const KeyboardEvent& k)
	{
		Key key = e.platform.keyboard.GetKeyFromEvent(k);
		auto* action = e.platform.keyboard.GetAction(key.KeyName);
		if (action)
		{
			HV::Events::Player::OnPlayerActionEvent pae;
			pae.Name = action;
			pae.start = key.isPressed;
			return Append(pae);
		}

		return false;
	}

	void OnRawMouseEvent(const MouseEvent& me) override
	{

	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{
		auto* player = e.players.GetPlayer(0);

		if (dWheel != 0)
		{
			if (e.editor.IsScrollLocked())
			{
				auto id = player->GetPlayerEntity();
				auto pos = e.platform.instances.GetEntity(id)->Position();
			}
		}

		if (!isCursorActive) fpsControl.OnMouseMove(cursorPos, delta, dWheel);
	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{

	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{
		if (!clickedDown)
		{
			isCursorActive = !isCursorActive;

			e.platform.renderer.SetCursorVisibility(isCursorActive);
		}
	}

	void Render(IGuiRenderContext& rc, const GuiRect& absRect) override
	{

	}
};

namespace HV
{
	IGameModeSupervisor* CreateFPSGameLogic(Cosmos& e)
	{
		return new FPSGameLogic(e);
	}
}