#include <rococo.types.h>
#include "dystopia.h"
#include <rococo.io.h>
#include "meshes.h"
#include <vector>
#include <unordered_map>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <sexy.vm.cpu.h>

#include <rococo.maths.h>


#include "component.system.inl"

#include "human.types.h"

#include "dystopia.post.h"

#include "skeleton.h"


using namespace Dystopia;
using namespace Rococo;

namespace
{
	struct Solid
	{
		Matrix4x4 transform;
		RGBA highlightColour;
		Vec3 position;

		// scale gives x y and z scale factors in model co-ordinates
		Vec3 scale;
		// theta gives rotation around z axis, from x to y
		// phi gives rotation around y axis, giving elevation
		Degrees theta;			
		Degrees phi;

		ID_SYS_MESH sysMeshId;
		ID_MESH meshId;
		Metres boundingRadius;
		ID_BITMAP bitmapId;
		uint32 flags;

		void AddFlag(SolidFlags flag) { flags |= flag; }
		bool HasFlag(SolidFlags flag) const { return (flags & flag) != 0; }
		void RemoveFlag(SolidFlags flag) { flags &= ~flag; }
	};

	struct Projectile
	{
		ID_ENTITY attacker;
		Vec3 position;
		Vec3 direction;
		Vec3 velocity;
		float lifeTime;
		float creationTime;
		ID_SYS_MESH bulletMesh;
	};

	struct Human
	{
		HumanType type;
		float nextAIcheck;
		AutoFree<IInventorySupervisor> inventory;
		AutoFree<IHumanAISupervisor> ai;
	};

	Sphere BoundingSphere(const Solid& solid)
	{
		return Sphere{ solid.transform.GetPosition() , solid.boundingRadius };
	}

	template<class ROW> ID_ENTITY GetFirstIntersect(cr_vec3 start, cr_vec3 end, const EntityTable<ROW>& table, const EntityTable<Solid>& solids, IQuadTree& quadTree)
	{
		struct : IObjectEnumerator
		{
			float leastT = 2.0f;
			ID_ENTITY firstTarget;
			const EntityTable<Solid>* solids;
			Vec3 start;
			Vec3 end;
			virtual void OnId(uint64 id)
			{
				const Solid& solid = solids->find(ID_ENTITY(id))->second;
				auto sphere = BoundingSphere(solid);

				float t0, t1;
				if (TryGetIntersectionLineAndSphere(t0, t1, start, end, sphere))
				{
					if (t0 > 0 && t0 < 1 && t0 < leastT)
					{
						leastT = t0;
						firstTarget = ID_ENTITY(id);
					}
				}
			}
		} onId;

		onId.start = start;
		onId.end = end;
		onId.solids = &solids;
		float radius = sqrtf(Square(end.x - start.x) + Square(end.y -start.y));
		Sphere boundingSphere{ start, radius };
		quadTree.EnumerateItems(boundingSphere, onId);

		return onId.firstTarget;
	}

	bool Intersects(cr_vec3 start, cr_vec3 end, ID_ENTITY id, const EntityTable<Solid>& solids)
	{
		const Solid& solid = solids.find(id)->second;
		auto sphere = BoundingSphere(solid);

		float t0, t1;
		if (TryGetIntersectionLineAndSphere(t0, t1, start, end, sphere))
		{
			if (t0 > 0 && t0 < 1)
			{
				return true;
			}
		}

		return false;
	}

	struct Equipment
	{
		IInventorySupervisor* inventory;
	};

	float GetLateralDisplacement(cr_vec3 a, cr_vec3 b)
	{
		return sqrtf(Square(a.x - b.x) + Square(a.y - b.y));
	}

	void TransformCube(const BoundingCube& cube, BoundingCube& rotatedCube, cr_m4x4 transform)
	{
		TransformPositions(cube.topVertices.vertices, 4, transform, rotatedCube.topVertices.vertices);
		TransformPositions(cube.bottomVertices.vertices, 4, transform, rotatedCube.bottomVertices.vertices);

		for (int i = 0; i < 6; ++i)
		{
			rotatedCube.P.planes[i].pointInPlane = transform * cube.P.planes[i].pointInPlane;
			rotatedCube.P.planes[i].normal = Vec4::FromVec3(Normalize(transform * cube.P.planes[i].normal), 0.0f);
		}
	}

	Collision FindIntersectSphereVsSolid(cr_vec3 start, cr_vec3 target, const Solid& obstacle, float projectileRadius, IMeshLoader& meshes)
	{
		struct : IEnumerator<BoundingCube>
		{
			Vec3 start;
			Vec3 target; 
			const Solid* obstacle;
			float radius;
			Collision firstCollision = NoCollision();

			virtual void operator()(const BoundingCube& cube)
			{
				BoundingCube rotatedCube;
				TransformCube(cube, rotatedCube, obstacle->transform);
				auto col = CollideBoundingBoxAndSphere(rotatedCube, Sphere{ start, radius }, target);
				if (col.contactType != ContactType_None)
				{
					firstCollision = col;
				}
			}
		} collideAgainstHull;

		collideAgainstHull.start = start;
		collideAgainstHull.target = target;
		collideAgainstHull.obstacle = &obstacle;
		collideAgainstHull.radius = projectileRadius;

		meshes.ForEachPhysicsHull(obstacle.meshId, collideAgainstHull);

		return collideAgainstHull.firstCollision;
	}

	Collision CollideBodies(cr_vec3 start, cr_vec3 target, Metres projectileRadius, const Solid& obstacle, IMeshLoader& meshes)
	{
		return FindIntersectSphereVsSolid(start, target, obstacle, projectileRadius, meshes);
	}

	Collision CollideWithGeometry(cr_vec3 start, cr_vec3 target, ID_ENTITY projectileId, Metres projectileRadius, IQuadTree& quadTree, EntityTable<Solid>& solids, IMeshLoader& meshes)
	{
		float radius = GetLateralDisplacement(start, target);
		Sphere boundingSphere{ start, radius + projectileRadius };

		struct : IObjectEnumerator
		{
			ID_ENTITY projectileId;
			Metres projectileRadius;
			Solid* target;
			EntityTable<Solid>* solids;
			Vec3 start;
			Vec3 end;
			Collision collision = NoCollision();
			IMeshLoader* meshes;

			virtual void OnId(uint64 idNumber)
			{
				ID_ENTITY obstacleId(idNumber);
				if (obstacleId != projectileId)
				{
					auto& c = solids->find(obstacleId);
					if (c != solids->end() && c->second.HasFlag(SolidFlags_Obstacle))
					{
						try
						{
							auto col = CollideBodies(start, end, projectileRadius, c->second, *meshes);
							if (col.t < collision.t)
							{
								collision = col;
							}
						}
						catch (IException& ex)
						{
							Throw(ex.ErrorCode(), L"Error colliding with body %I64u, against mesh %d:\n %s", (uint64)projectileId, c->second.meshId, ex.Message());
						}
					}
				}
			}
		} onTarget;

		onTarget.projectileId = projectileId;
		onTarget.projectileRadius = projectileRadius;
		onTarget.solids = &solids;
		onTarget.start = start;
		onTarget.end = target;
		onTarget.meshes = &meshes;

		quadTree.EnumerateItems(boundingSphere, onTarget);

		return onTarget.collision;
	}

	class Level : public ILevelSupervisor, public ILevelBuilder, public Post::IRecipient
	{
		Environment& e; // not valid until constructor returns
		IHumanFactory& hf;
		ID_ENTITY idPlayer;
		EntityTable<Solid> solids;
		EntityTable<Projectile> projectiles;
		EntityTable<Human*> enemies;
		EntityTable<Human*> allies;
		EntityTable<Equipment> equipment;
		EntityTable<ISkeletonSupervisor*> skeletons;
		Vec3 groundZeroCursor;
		ID_ENTITY selectedId;
		AutoFree<IQuadTreeSupervisor> quadTree;
		float gameTime;
		float lastDt;

		std::vector<ID_ENTITY> abbatoir;
	public:
		Level(Environment& _e, IHumanFactory& _hf) : 
			e(_e),
			hf(_hf),
			groundZeroCursor{ 0,0,0 },
			quadTree(CreateLooseQuadTree(4096.0_metres, 3.0_metres)),
			gameTime(0),
			lastDt(0)
		{
			
		}

		virtual void OnCreated()
		{
			e.postbox.Subscribe<VerbInventoryChanged>(this);
		}

		~Level()
		{
			Clear();
		}

		void GenerateCity(const fstring& name)
		{
			BuildRandomCity(name, 0, e);
		}

		virtual ILevelBuilder& Builder()
		{
			return *this;
		}

		virtual void Clear()
		{
			FreeTable(allies);
			FreeTable(enemies);
			FreeTable(skeletons);
			solids.clear();
			for (auto i : equipment)
			{
				i.second.inventory->Free();
			}
			equipment.clear();
			quadTree->Clear();
		}

		virtual void Free() { delete this; }

		virtual ID_ENTITY AddAmmunition(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 ammoType, float massPerBullet, float massPerClip, int32 count)
		{
			auto id = AddSolid(transform, editorId, SolidFlags_None);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ massPerClip };
			itemData.bitmapId = e.bitmapCache.Cache(imageFile);
			inv->Swap(0, CreateAmmo({ massPerBullet, ammoType, count }, itemData));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual ID_ENTITY AddRangedWeapon(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageName, float muzzleVelocity, float flightTime, int32 ammoType, float mass)
		{
			auto id = AddSolid(transform, editorId, SolidFlags_None);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ mass };
			itemData.bitmapId = e.bitmapCache.Cache(imageName);
			inv->Swap(0, CreateRangedWeapon({ flightTime, muzzleVelocity, ammoType }, itemData ));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual ID_ENTITY AddArmour(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 bulletProt, int32 dollSlot, float massKg)
		{
			if (dollSlot < 0 || dollSlot >= PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO)
			{
				Throw(0, L"Could not add armour: %s. dollslot was %d. 0...%d", name.buffer, dollSlot, PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO-1);
			}

			if (bulletProt < 0)
			{
				Throw(0, L"Could not add armour: %s. bulletProt was -ve", name.buffer, dollSlot, PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO - 1);
			}

			auto id = AddSolid(transform, editorId, SolidFlags_None);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ massKg };
			itemData.bitmapId = e.bitmapCache.Cache(imageFile);
			itemData.slot = (PAPER_DOLL_SLOT) dollSlot;
			inv->Swap(0, CreateArmour(ArmourValue {(uint32) bulletProt}, itemData));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual bool TryGetEquipment(ID_ENTITY id, EquipmentDesc& desc) const
		{
			auto x = equipment.find(id);
			if (x != equipment.end())
			{
				auto& solid = solids.find(id);
				desc.worldPosition = solid->second.transform.GetPosition();
				desc.inventory = x->second.inventory;
				return true;
			}
			else
			{
				return false;
			}
		}

		virtual void Delete(ID_ENTITY id)
		{
			abbatoir.push_back(id);
		}

		virtual ID_ENTITY AddSolid(const Matrix4x4& transform, ID_MESH meshId, int32 flags)
		{
			Vec3 pos = transform.GetPosition();

			auto id = GenerateEntityId();
			ID_SYS_MESH sysId = e.meshes.GetRendererId(meshId);
			solids.insert(id, 
				/*
					Matrix4x4 transform;
					RGBA highlightColour;
					Vec3 position;

					// scale gives x y and z scale factors in model co-ordinates
					Vec3 scale;
					// theta gives rotation around z axis, from x to y
					// phi gives rotation around y axis, giving elevation
					Degrees theta;
					Degrees phi;

					ID_SYS_MESH sysMeshId;
					ID_MESH meshId;
					Metres boundingRadius;
					ID_BITMAP bitmapId;
					uint32 flags;
				*/
				Solid
				{ 
					transform, {0,0,0,0}, pos, {1,1,1}, 0, 0, sysId, meshId, 1.0_metres, ID_BITMAP::Invalid(), (uint32)flags
				}
			);

			quadTree->AddEntity(Sphere{ transform.GetPosition(), 1.0_metres }, id);
			return id;
		}

		virtual ID_ENTITY AddAlly(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId, SolidFlags_Skeleton);
			auto& s = solids.find(id);
			s->second.boundingRadius = 0.5_metres;
			auto* inv = CreateInventory({ 4, 10 }, true, true);
			auto h = new Human { HumanType_Vigilante, 0.0f, inv, hf.CreateHuman(id, HumanType_Vigilante ) };
			skeletons.insert(id, CreateSkeleton(e));
			ItemData data;
			data.name = L"Bag of stones";
			data.bitmapId = e.bitmapCache.Cache(L"!inventory/bagofstones.tif");
			data.mass = 5.0_kg;
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5_seconds, 10.0_mps, 0 }, data));
			allies.insert(id, h);
			return id;
		}

		virtual ID_ENTITY AddEnemy(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId, SolidFlags_Skeleton);
			auto* inv = CreateInventory({ 3,4 }, true, true);
			auto h = new Human{ HumanType_Bobby, 0.0f, inv, hf.CreateHuman(id, HumanType_Bobby) };
			ItemData data;
			data.name = L"Bag of stones";
			data.bitmapId = e.bitmapCache.Cache(L"!inventory/bagofstones.tif");
			data.mass = 5.0_kg;
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5_seconds, 10.0_mps, 0 }, data));
			enemies.insert(id, h);
			skeletons.insert(id, CreateSkeleton(e));
			return id;
		}

		virtual ID_ENTITY AddProjectile(const ProjectileDef& def, float currentTime)
		{
			auto id = GenerateEntityId();

			Vec3 direction = Normalize(def.velocity);

			projectiles.insert(id, Projectile{ def.attacker, def.origin, direction, def.velocity, def.lifeTime, currentTime, def.bulletMesh });

			return id;
		}

		virtual ID_ENTITY CreateStash(IItem* item, cr_vec3 location)
		{
			ID_MESH stashId(0x40000000);
			if (e.meshes.GetRendererId(stashId) == ID_SYS_MESH::Invalid())
			{
				e.meshes.Load(L"!mesh/stash.sxy"_fstring, stashId);
			}

			Matrix4x4 stashLoc = Matrix4x4::Translate(location + Vec3{ 0,0,0.1f });
			Matrix4x4 scale = Matrix4x4::Scale(0.37f, 0.37f, 0.37f);
			
			auto id = Builder().AddSolid(stashLoc * scale, stashId, SolidFlags_Selectable);
			auto& solid = solids.find(id)->second;

			solid.bitmapId = item ? item->Data().bitmapId : ID_BITMAP::Invalid();

			Equipment eq{ CreateInventory({ 4,4 }, false, false) };
			equipment.insert(id, eq);
			eq.inventory->Swap(0, item); // This is new inventory, so returned item is nullptr

			return id;
		}

		virtual HumanSpec GetHuman(ID_ENTITY id)
		{
			auto i = enemies.find(id);
			if (i == enemies.end())
			{
				i = allies.find(id);
				if (i == allies.end())
				{
					return{ HumanType_None, nullptr, nullptr };
				}
			}

			auto& h = *i->second;
			return{ h.type, h.inventory, h.ai };
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* change = Post::InterpretAs<VerbInventoryChanged>(mail);
			if (change)
			{
				EquipmentDesc eq;
				if (TryGetEquipment(change->containerId, eq))
				{
					if (eq.inventory->EnumerateItems(nullptr) == 0)
					{
						Delete(change->containerId);
					}
				}
			}
		}

		virtual void SetGroundCursorPosition(cr_vec3 groundZero)
		{
			groundZeroCursor = groundZero;
		}

		virtual ID_ENTITY SelectedId()
		{
			return selectedId;
		}

		void UpdateTransform(ID_ENTITY id, Solid& solid)
		{
			if (solid.HasFlag(SolidFlags_IsDirty))
			{
				Matrix4x4 S = Matrix4x4::Scale(solid.scale.x, solid.scale.y, solid.scale.z);
				Matrix4x4 R = Matrix4x4::RotateRHAnticlockwiseX(solid.phi) * Matrix4x4::RotateRHAnticlockwiseZ(solid.theta);
				Matrix4x4 T = Matrix4x4::Translate(solid.position);

				cr_vec3 currentPos = solid.transform.GetPosition();
				if (currentPos != solid.position)
				{
					quadTree->DeleteEntity(Sphere{ currentPos, solid.boundingRadius }, id);
					quadTree->AddEntity(Sphere{ solid.position, solid.boundingRadius }, id);
				}

				solid.transform = T * R * S;

				solid.RemoveFlag(SolidFlags_IsDirty);
			}
		}

		virtual bool TryGetTransform(ID_ENTITY id, Matrix4x4& transform)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				return false;
			}
			else
			{
				UpdateTransform(id, i->second);
				transform = i->second.transform;
				return true;
			}
		}

		virtual void SetHeading(ID_ENTITY id, Degrees theta)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity %I64u", (uint64)id);
			}

			i->second.theta = theta;
			i->second.AddFlag(SolidFlags_IsDirty);
		}

		virtual void SetElevation(ID_ENTITY id, Degrees phi)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id %I64u", (uint64) id);
			}

			i->second.phi = phi;
			i->second.AddFlag(SolidFlags_IsDirty);
		}

		virtual void SetScale(ID_ENTITY id, cr_vec3 scale)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id %I64u", (uint64)id);
			}

			i->second.scale = scale;
			i->second.AddFlag(SolidFlags_IsDirty);
		}

		virtual cr_vec3 GetPosition(ID_ENTITY id) const
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			return i->second.position;
		}

		virtual void SetPosition(ID_ENTITY id, cr_vec3 targetPos)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id %I64u", (uint64)id);
			}

			auto& solid = i->second;
			Vec3 currentPos = solid.transform.GetPosition();
			
			if (targetPos != currentPos)
			{
				quadTree->DeleteEntity(Sphere{ currentPos, solid.boundingRadius }, id);
				quadTree->AddEntity(Sphere { targetPos, solid.boundingRadius } , id);

				solid.position = targetPos;

				solid.transform.row0.w = targetPos.x;
				solid.transform.row1.w = targetPos.y;
				solid.transform.row2.w = targetPos.z;
			}
		}

		// This should only be called by the abbatoir clean up code
		void DeleteSolid(ID_ENTITY id)
		{
			auto i = solids.find(id);
			if (i != solids.end())
			{
				auto& entity = i->second;
				auto& t = i->second.transform;

				if (entity.HasFlag(SolidFlags_Skeleton))
				{
					auto j = skeletons.find(id);
					if (j != skeletons.end())
					{
						j->second->Free();
						skeletons.erase(j);
					}

					auto k = allies.find(id);
					if (k != allies.end())
					{
						k->second->ai->Free();
						k->second->inventory->Free();
						allies.erase(k);
					}

					k = enemies .find(id);
					if (k != enemies.end())
					{
						k->second->ai->Free();
						k->second->inventory->Free();
						enemies.erase(k);
					}
				}
				else
				{
					auto eq = equipment.find(id);
					if (eq != equipment.end())
					{
						eq->second.inventory->Free();
						equipment.erase(eq);
					}
				}

				quadTree->DeleteEntity(Sphere{ t.GetPosition(), entity.boundingRadius }, id);
				solids.erase(i);
			}
		}

		virtual ID_ENTITY SelectedId() const
		{
			return selectedId;
		}

		void RenderSolidsBySearch(IRenderContext& rc)
		{
			cr_vec3 centre = GetPosition(idPlayer);

			struct : IObjectEnumerator
			{
				virtual void OnId(uint64 idValue)
				{
					ID_ENTITY id(idValue);
					auto& i = solids->find(id);
					if (i == solids->end())
					{
						Throw(0, L"Entity #%I64u was registered in the quad tree, but not the solids table. BUG!", idValue);
					}

					auto& entity = i->second;

					This->UpdateTransform(id, i->second);

					cr_vec3 pos = entity.position;
					Vec3 delta = pos - groundZeroCursor;

					float DS2 = Square(delta.x) + Square(delta.y);
					bool isHighlighted = entity.HasFlag(SolidFlags_Selectable) &&  DS2 < Square(entity.boundingRadius);

					if (entity.HasFlag(SolidFlags_Skeleton))
					{
						auto sk = skeletons->find(id);
						if (sk == skeletons->end())
						{
							Throw(0, L"Entity #%I64u was marked as having a skeleton, but none could be found.", idValue);
						}

						sk->second->Render(*rc, ObjectInstance{ entity.transform, entity.highlightColour }, Seconds{ gameTime });
					}
					else
					{
						if (entity.bitmapId != ID_BITMAP::Invalid())
						{
							bitmaps->SetMeshBitmap(*rc, entity.bitmapId);
						}

						if (isHighlighted && !foundItem)
						{
							selectedId = i->first;
							foundItem = true;
							ObjectInstance instance{ entity.transform,{ 1.0f, 1.0f, 1.0f, 0.5f } };
							rc->Draw(entity.sysMeshId, &instance, 1);
						}
						else
						{
							ObjectInstance instance{ entity.transform, entity.highlightColour };
							rc->Draw(entity.sysMeshId, &instance, 1);
						}
					}

					count++;
				}

				IRenderContext* rc;
				EntityTable<Solid>* solids;
				EntityTable<ISkeletonSupervisor*>* skeletons;
				ID_ENTITY selectedId;
				bool foundItem = false;
				Vec3 groundZeroCursor;
				IBitmapCache* bitmaps;
				int count = 0;
				float gameTime;
				float lastDt;
				Level* This;
			} renderSolid;

			renderSolid.This = this;

			renderSolid.groundZeroCursor = groundZeroCursor;
			renderSolid.solids = &solids;
			renderSolid.rc = &rc;
			renderSolid.bitmaps = &e.bitmapCache;
			renderSolid.skeletons = &skeletons;
			renderSolid.gameTime = gameTime;
			renderSolid.lastDt = lastDt;

			quadTree->EnumerateItems(Sphere{ centre, 32.0_metres }, renderSolid);
			
			selectedId = renderSolid.selectedId;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			RenderSolidsBySearch(rc);

			// N.B projectiles are not put in the quad tree, so do not appear in a search
			for (auto& i : projectiles)
			{
				auto& p = i.second;

				ObjectInstance pi{
					Matrix4x4
					{
						Vec4{0.25f,		0,		0,  p.position.x},
						Vec4{0,		0.25f,		0,  p.position.y},
						Vec4{0,			0,	0.25f,  p.position.z},
						Vec4{0,			0,		0,			   1}
					},
					RGBA(0,0,0,0)
				};

				rc.Draw(p.bulletMesh, &pi, 1);
			}
		}

		void UpdateProjectile(Projectile& p, float dt, ID_ENTITY projectileId)
		{
			Vec3 g = { 0, 0, -9.81f };
			Vec3 newPos = dt * p.velocity + p.position + g * dt * dt * 0.5f;
			Vec3 newVelocity = p.velocity + g * dt;
			Vec3 oldPos = p.position;

			auto col = CollideWithGeometry(oldPos, newPos, projectileId, 0.02_metres, *quadTree, solids, e.meshes);
			if (LengthSq(newVelocity) != 0 && col.contactType != ContactType_None)
			{
				if (col.t > 0 && col.t < 1.0f)
				{
					float skinDepth = 0.01f; // Prevent object exactly reaching target to try to avoid numeric issues
					Vec3 collisionPos = oldPos + newVelocity * dt * col.t * (1 - skinDepth);
					Vec3 impulseDisplacement = collisionPos - col.touchPoint;

					Vec3 impulseDirection = Normalize(impulseDisplacement);

					float restitution = 0.25f;
						
					newVelocity = (newVelocity + 2.0f * impulseDirection * Length(newVelocity)) * restitution;
					newPos = collisionPos + dt * newVelocity * (1 - col.t);	

					auto col2 = CollideWithGeometry(collisionPos, newPos, projectileId, 0.02_metres, *quadTree, solids, e.meshes);
					if (col2.contactType != ContactType_None)
					{
						p.lifeTime = 0;
						return;
					}
				}
			}

			p.velocity = newVelocity;
			p.position = newPos;

			if (!TryNormalize(p.velocity, p.direction))
			{
				p.direction = Vec3{ 0, 0, 1 };
			}

			if (p.position.z < 0)
			{
				p.lifeTime = 0;
			}

			if (p.attacker == idPlayer)
			{
				ID_ENTITY idEnemy = GetFirstIntersect(oldPos, newPos, enemies, solids, *quadTree);
				if (idEnemy)
				{
					auto i = enemies.find(idEnemy);
					if (i != enemies.end())
					{
						i->second->ai->OnHit(idPlayer);
					}

					p.lifeTime = 0;
				}
			}
			else
			{
				if (Intersects(oldPos, newPos, idPlayer, solids))
				{
					allies[idPlayer]->ai->OnHit(p.attacker);
					p.lifeTime = 0;
				}
			}
		}

		void UpdatePosition(ID_ENTITY id, float dt, EntityTable<Human*>& humans)
		{
			cr_vec3 pos = GetPosition(id);

			auto i = humans.find(id);
			if (i == humans.end())
			{
				return;
			}
			
			auto skel = skeletons.find(id);
			if (skel == skeletons.end())
			{
				return;
			}
			
			auto j = solids.find(id);
			if (j != solids.end())
			{
				Vec3 direction;
				if (!TryNormalize(i->second->ai->Velocity(), direction))
				{
					skel->second->SetCurrentAnimation(AnimationType_Standstill);
					return;
				}

				skel->second->SetCurrentAnimation(AnimationType_Running);

				float speed = Length(i->second->ai->Velocity());// TODO - refactor AI velocity to incorporate the changes below

				if (speed == 0.0f) return;

				Vec4 velDir = Vec4::FromVec3(direction, 0.0f);

				Vec4 velDirRotated = j->second.transform * velDir;
				if (!TryNormalize(velDirRotated, direction)) return;

				Vec3 newPos = pos + direction * dt * speed;

				auto col = CollideWithGeometry(pos, newPos, id, j->second.boundingRadius, *quadTree, solids, e.meshes);
				if (col.contactType != ContactType_None)
				{
					if (col.t > 0 && col.t < 1.0f)
					{
						float skinDepth = 0.01f; // Prevent object exactly reaching target to try to avoid numeric issues
						Vec3 collisionPos = pos + direction * dt * speed * col.t * (1 - skinDepth);
						Vec3 impulseDisplacement = collisionPos - col.touchPoint;

						Vec3 impulseDirection = Normalize(impulseDisplacement);
						
						float restitution = 0.75f;
						newPos = collisionPos - impulseDirection * dt * restitution * speed * (1 - col.t);

						auto col2 = CollideWithGeometry(collisionPos, newPos, id, j->second.boundingRadius, *quadTree, solids, e.meshes);
						if (col2.contactType != ContactType_None)
						{
							return; // If a second collision occurs in the same timestep prevent motion altogether to avoid race conditions
						}	
					}
				}

				SetPosition(id, newPos);
			}	
		}

		void UpdateEnemy(Human& enemy, ID_ENTITY id, float gameTime, float dt)
		{
			const float oorMax = 1.0f / (float)RAND_MAX;

			UpdatePosition(id, dt, enemies);

			if (enemy.nextAIcheck < gameTime)
			{
				enemy.ai->Update(gameTime, dt);
				enemy.nextAIcheck = gameTime + GenRandomFloat(0.2f, 1.0f);
			}
		}

		virtual IInventory* GetInventory(ID_ENTITY id)
		{
			auto i = enemies.find(id);
			if (i == enemies.end())
			{
				i = allies.find(id);
				if (i == allies.end())
				{
					auto j = equipment.find(id);
					if (j != equipment.end())
					{
						return j->second.inventory;	
					}
					else
					{
						return nullptr;
					}
				}
			}

			return i->second->inventory;
		}

		virtual void UpdateGameTime(float dt)
		{
			gameTime += dt;
			lastDt = dt;

			AdvanceTimestepEvent ate{ dt, gameTime };
			e.postbox.SendDirect(ate);

			auto i = projectiles.begin();
			while (i != projectiles.end())
			{
				auto& p = i->second;
				if (p.creationTime + p.lifeTime < gameTime)
				{
					i = projectiles.erase(i);
				}
				else
				{
					UpdateProjectile(p, dt, i->first);
					i++;
				}
			}

			auto j = enemies.begin();
			while (j != enemies.end())
			{
				auto& enemy = j->second;
				if (enemy->ai->IsAlive())
				{
					UpdateEnemy(*enemy, j->first, gameTime, dt);
					j++;
				}
				else
				{
					abbatoir.push_back(j->first);
					break;
				}
			}

			for (auto dead : abbatoir)
			{
				DeleteSolid(dead);
			}

			abbatoir.clear();

			allies[idPlayer]->ai->Update(gameTime, dt);

			cr_vec3 playerPos = GetPosition(idPlayer);

			UpdatePosition(idPlayer, dt, allies);
		}

		virtual ID_ENTITY GetPlayerId() const
		{
			return idPlayer;
		}

		virtual void SetPlayerId(ID_ENTITY id)
		{
			idPlayer = id;
		}
	};

	class LevelLoader : public ILevelLoader, public IEventCallback<ScriptCompileArgs>
	{
		Environment& e;

	public:
		LevelLoader(Environment& _e):
			e(_e)
		{

		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnEvent(ScriptCompileArgs& args)
		{
			AddNativeCalls_DystopiaIMeshes(args.ss, &e.meshes);
			AddNativeCalls_DystopiaILevelBuilder(args.ss, &e.level);
			AddNativeCalls_DystopiaIGui(args.ss, &e.gui);
		}

		virtual void Load(const wchar_t* resourceName, bool isReloading)
		{
			ExecuteSexyScriptLoop(16384, e.sourceCache, e.debuggerWindow, resourceName, 0, (int32) 16_megabytes, *this);
		}

		virtual void SyncWithModifiedFiles()
		{
			struct : IEventCallback<FileModifiedArgs>
			{
				IMeshLoader* meshLoader;
				IBoneLibrary* boneLib;

				virtual void OnEvent(FileModifiedArgs& args)
				{
					meshLoader->UpdateMesh(args.resourceName);
					boneLib->UpdateLib(args.resourceName);
				}
			} monitor;
			monitor.meshLoader = &e.meshes;
			monitor.boneLib = &e.boneLibrary;

			GetOS(e).EnumerateModifiedFiles(monitor);
		}
	};
}

namespace Dystopia
{
	ILevelLoader* CreateLevelLoader(Environment& e)
	{
		return new LevelLoader(e);
	}

	ILevelSupervisor* CreateLevel(Environment& e, IHumanFactory& hf)
	{
		return new Level(e, hf);
	}
}