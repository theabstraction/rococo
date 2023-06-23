#include "hv.h"
#include <vector>
#include <algorithm>
#include <rococo.strings.h>
#include <rococo.random.h>
#include <rococo.clock.h>

using namespace HV::HVMaths;
using namespace Rococo::Strings;

namespace
{
	using namespace HV;

	void SortQuadsByArea(std::vector<Quad>& quads)
	{
		struct
		{
			static float AreaSq(const Quad& q)
			{
				Vec3 ab = q.b - q.a;
				Vec3 bc = q.c - q.b;
				Vec3 K = Cross(ab, bc);
				return LengthSq(K);
			}

			bool operator()(const Quad& p, const Quad& q) const
			{
				return AreaSq(p) > AreaSq(q);
			}
		} byAreaDescending;
		std::sort(quads.begin(), quads.end(), byAreaDescending);
	}

	struct Component
	{
		HString name;
		HString meshName;
		ID_ENTITY id;
	};

	bool operator == (const Component& a, const fstring& b)
	{
		return a.name.length() == b.length && Eq(a.name.c_str(), b);
	}

	class SC: public ISectorContents
	{
	private:
		Platform& platform;
		ISector& sector;

		struct SceneryBind
		{
			ID_ENTITY id;
			AABB worldBounds;
			std::vector<Quad> levelSurfaces;
		};

		std::vector<ID_ENTITY> managedEntities;
		std::vector<SceneryBind> scenery;
		std::vector<SceneryBind*> randomizedSceneryList;

		Random::Shuffler shuffler;

		struct PlacementCandidate
		{
			AABB worldBounds;
			Matrix4x4 model;
		};

		std::vector<PlacementCandidate> candidates;

		std::vector<Component> components;

		ISectorAIBuilderSupervisor& ai;
	public:
		SC(Platform& _platform, ISector& _sector, ISectorAIBuilderSupervisor& _ai) : 
			platform(_platform), sector(_sector), shuffler(_sector.Id() + 5550), ai(_ai)
		{}

		void ClearAllComponents() override
		{
			components.clear();
		}

		void ClearComponents(const fstring& componentName) override
		{
			for (auto& c : components)
			{
				platform.graphics.instances.Delete(c.id);
			}

			components.erase(std::remove(components.begin(), components.end(), componentName), components.end());
		}

		void AddComponent(cr_m4x4 model, cstr componentName, cstr meshName) override
		{
			auto id = platform.graphics.instances.AddBody(to_fstring(meshName), model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());;

			for (auto& c : components)
			{
				if (Eq(componentName, c.name.c_str()) && Eq(meshName, c.meshName.c_str()))
				{
					platform.graphics.instances.Delete(c.id);
					c.id = id;
					return;
				}
			}

			Component c{ componentName, meshName, id };
			components.push_back(c);
		}

		ID_ENTITY AddItemToLargestSquare(const fstring& meshName, int addItemFlags, const HV::ObjectCreationSpec& obs) override
		{
			SectorSquares squares = sector.Squares();

			if (sector.IsCorridor() || squares.first == squares.end) return ID_ENTITY::Invalid();

			auto& aabb = *squares.first;

			ID_SYS_MESH meshId;
			AABB bounds;
			if (!platform.graphics.meshes.TryGetByName(meshName, meshId, bounds)) return ID_ENTITY::Invalid();

			AABB worldBounds;
			Matrix4x4 model;
			const float z0 = sector.Z0();
			const float z1 = sector.Z1();
			if (!TryGetRandomTransformation(model, worldBounds, addItemFlags & (int) AddItemFlags::RandomHeading, addItemFlags & (int) AddItemFlags::RandomPosition, bounds, aabb, z0, z1))
			{
				return ID_ENTITY::Invalid();
			}

			auto id = platform.graphics.instances.AddBody(meshName, model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());
			scenery.push_back({ id,worldBounds });
			return id;
		}

		void ClearManagedEntities()
		{
			managedEntities.clear();
		}

		void DeleteItemsWithMesh(const fstring& prefix) override
		{
			struct
			{
				Platform& platform;
				const fstring prefix;
				bool operator()(const ID_ENTITY id) const
				{
					auto body = platform.graphics.instances.ECS().GetBodyComponent(id);
					if (body)
					{
						auto meshId = body->Mesh();
						auto name = platform.graphics.meshes.GetName(meshId);
						if (StartsWith(name, prefix))
						{
							return true;
						}
					}
					return false;
				}

				bool operator()(const SceneryBind& bind) const
				{
					return operator()(bind.id);
				}
			} meshPrefixed{ platform, prefix };
			auto i = std::remove_if(managedEntities.begin(), managedEntities.end(), meshPrefixed);
			managedEntities.erase(i, managedEntities.end());

			auto j = std::remove_if(scenery.begin(), scenery.end(), meshPrefixed);
			scenery.erase(j, scenery.end());
		}

		void DeleteScenery() override
		{
			for (auto s : scenery)
			{
				platform.graphics.instances.Delete(s.id);
			}

			scenery.clear();
		}

		ID_ENTITY AddSceneryAroundObject(const fstring& mesh, ID_ENTITY centrePieceId, const HV::InsertItemSpec& iis, const HV::ObjectCreationSpec& ocs) override
		{
			SectorSquares squares = sector.Squares();
			if (sector.IsCorridor() || squares.first == squares.end) return ID_ENTITY::Invalid();

			auto& aabb = *squares.first;

			AABB centreBounds;
			if (!TryGetScenery(centrePieceId, centreBounds))
			{
				return ID_ENTITY::Invalid();
			}

			auto body = platform.graphics.instances.ECS().GetBodyComponent(centrePieceId);
			if (!body)
			{
				return ID_ENTITY::Invalid();
			}

			ID_SYS_MESH meshId;
			AABB bounds;
			if (!platform.graphics.meshes.TryGetByName(mesh, meshId, bounds)) return ID_ENTITY::Invalid();

			candidates.clear();

			const float z0 = sector.Z0();
			const float z1 = sector.Z1();

			for (int i = 0; i < 100; i++)
			{
				AABB worldBounds;
				Matrix4x4 model;
				if (!TryGetRandomTransformation(model, worldBounds, iis.insertFlags & (int) AddItemFlags::RandomHeading, iis.insertFlags & (int) AddItemFlags::RandomPosition, bounds, aabb, z0, z1))
				{
					return ID_ENTITY::Invalid();
				}

				if (!DoesSceneryCollide(worldBounds))
				{
					candidates.push_back({ worldBounds, model });
				}
			}

			if (candidates.empty())
			{
				return ID_ENTITY::Invalid();
			}

			struct
			{
				Vec3 centrePieceOrigin;
				bool operator ()(const PlacementCandidate& a, const PlacementCandidate& b) const
				{
					auto Pa = a.worldBounds.Centre();
					auto Pb = b.worldBounds.Centre();

					float Rao = LengthSq(Pa - centrePieceOrigin);
					float Rbo = LengthSq(Pb - centrePieceOrigin);

					return Rao < Rbo;
				}
			} sortByRangeFromCentrePiece;

			struct
			{
				float optimalRangeSq;
				Vec3 centrePieceOrigin;

				bool operator ()(const PlacementCandidate& a, const PlacementCandidate& b) const
				{
					auto Pa = a.worldBounds.Centre();
					auto Pb = b.worldBounds.Centre();

					Vec3 ao = centrePieceOrigin - Pa;
					Vec3 bo = centrePieceOrigin - Pb;

					float Rao = LengthSq(ao);
					float Rbo = LengthSq(bo);

					if (Rao < optimalRangeSq && Rbo < optimalRangeSq)
					{
						Vec3 ao_dir;
						Vec3 bo_dir;
						if (!TryNormalize(ao, ao_dir)) ao_dir = ao;
						if (!TryNormalize(bo, bo_dir)) bo_dir = bo;

						Vec3 aDir = a.model.GetForwardDirection();
						Vec3 bDir = b.model.GetForwardDirection();

						return Dot(aDir, ao_dir) > Dot(bDir, bo_dir);
					}
					else
					{
						return Rao < Rbo;
					}
				}
			} sortByRangeFromCentrePieceAndOrientation;

			bool sortByOrientation = iis.insertFlags & (int) AddItemFlags::AlignEdge;

			if (sortByOrientation)
			{
				sortByRangeFromCentrePieceAndOrientation.optimalRangeSq = Sq(iis.maxDistance.value);
				sortByRangeFromCentrePieceAndOrientation.centrePieceOrigin = body->Model().GetPosition();
				std::sort(candidates.begin(), candidates.end(), sortByRangeFromCentrePieceAndOrientation);
			}
			else
			{
				sortByRangeFromCentrePiece.centrePieceOrigin = body->Model().GetPosition();
				std::sort(candidates.begin(), candidates.end(), sortByRangeFromCentrePiece);
			}

			auto id = platform.graphics.instances.AddBody(mesh, candidates[0].model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());
			scenery.push_back({ id,candidates[0].worldBounds });

			return id;
		}

		void ForEveryObjectInContent(IEventCallback<const ID_ENTITY>& cb) override
		{
			for (const auto& c : components)
			{
				cb.OnEvent(c.id);
			}

			for (auto& s : scenery)
			{
				cb.OnEvent(s.id);
			}

			for (auto& m : managedEntities)
			{
				cb.OnEvent(m);
			}
		}

		void Free() override
		{
			delete this;
		}

		void ManageEntity(ID_ENTITY id) override
		{
			managedEntities.push_back(id);
		}

		void UseUpFacingQuadsOnScenery(ID_ENTITY id) override
		{
			for (auto& i : scenery)
			{
				if (i.id == id)
				{
					i.levelSurfaces.clear();

					auto body = platform.graphics.instances.ECS().GetBodyComponent(id);
					if (body)
					{
						const Vec3 up{ 0, 0, 1 };
						auto& model = body->Model();
						size_t triangleCount = 0;
						auto tris = platform.graphics.meshes.GetTriangles(body->Mesh(), triangleCount);

						if (triangleCount > 1)
						{
							for (auto k = 0; k < triangleCount - 1; ++k)
							{
								auto& t0 = tris[k];
								auto& t1 = tris[k + 1];
								if (HVMaths::IsTriangleFacingUp(model, t0) && HVMaths::IsTriangleFacingUp(model, t1))
								{
									// We may have a quad, if so then t0 forms abc and t1 forms cda
									if (t0.a.position == t1.c.position && t0.c.position == t1.a.position)
									{
										Quad q;
										q.a = t0.a.position;
										q.b = t0.b.position;
										q.c = t0.c.position;
										q.d = t1.b.position;
										i.levelSurfaces.push_back(q);
										k++;
									}
								}
							}
						}

						SortQuadsByArea(i.levelSurfaces);
					}
				}
			}
		}

		bool DoesSceneryCollide(const AABB& aabb) const override
		{
			for (auto& s : scenery)
			{
				if (s.worldBounds.Intersects(aabb))
				{
					return true;
				}
			}

			return false;
		}

		bool TryGetScenery(ID_ENTITY id, AABB& worldBounds) const override
		{
			for (auto& cp : scenery)
			{
				if (cp.id == id)
				{
					worldBounds = cp.worldBounds;
					return true;
				}
			}

			return false;
		}

		bool TryPlaceItemOnQuad(const Quad& qModel, ID_ENTITY quadsEntityId, ID_ENTITY itemId)
		{
			const float z0 = sector.Z0();
			const float z1 = sector.Z1();

			auto quadBody = platform.graphics.instances.ECS().GetBodyComponent(quadsEntityId);
			auto item = platform.graphics.instances.ECS().GetBodyComponent(itemId);
			if (quadBody && item)
			{
				if (IsQuadRectangular(qModel) && qModel.a.x == qModel.b.x || qModel.a.y == qModel.b.y)
				{
					Quad qWorld;
					TransformPositions(&qModel.a, 4, quadBody->Model(), &qWorld.a);

					auto bounds = platform.graphics.meshes.Bounds(item->Mesh());
					if (bounds.minXYZ.x < bounds.maxXYZ.x)
					{
						AABB2d minSquare;
						minSquare << AsVec2(qModel.a) << AsVec2(qModel.b) << AsVec2(qModel.c) << AsVec2(qModel.d);

						Matrix4x4 randomModel;
						AABB worldBounds;
						if (TryGetRandomTransformation(randomModel, worldBounds, true, true, bounds, minSquare, qModel.a.z, z1 - z0))
						{
							ManageEntity(itemId);
							item->SetModel(quadBody->Model() * randomModel);
							return true;
						}
					}
				}
			}

			return false;
		}

		boolean32 PlaceItemOnUpFacingQuad(ID_ENTITY id) override
		{
			randomizedSceneryList.clear();

			for (auto& i : scenery)
			{
				randomizedSceneryList.push_back(&i);
			}

			std::shuffle(randomizedSceneryList.begin(), randomizedSceneryList.end(), shuffler);

			for (auto& i : randomizedSceneryList)
			{
				if (!i->levelSurfaces.empty())
				{
					auto& q = i->levelSurfaces[Roll::x((uint32)i->levelSurfaces.size())];
					if (TryPlaceItemOnQuad(q, i->id, id))
					{
						return true;
					}
				}
			}

			return false;
		}

		float doorElevation = 0;
		float doorDirection = 0.0f;

		float leverElevation = 0_degrees;
		float leverOmega = 90_degrees;

		const float leverSpeed = 180_degrees;

		const Degrees LEVER_UP_ANGLE = 45_degrees;
		const Degrees LEVER_DOWN_ANGLE = -45_degrees;

		void LowerScenery() override
		{
			for (auto c : components)
			{
				if (Eq(c.name.c_str(), "door.body"))
				{
					doorDirection = -1.0f;
					break;
				}
				else if (Eq(c.name.c_str(), "wall.lever.base"))
				{
					for (auto d : components)
					{
						if (Eq(d.name.c_str(), "wall.lever"))
						{
							leverOmega = leverSpeed;
							break;
						}
					}
				}
			}
		}

		void RaiseScenery() override
		{
			for (auto c : components)
			{
				if (Eq(c.name.c_str(), "door.body"))
				{
					doorDirection = 1.0f;
					break;
				}
				else if (Eq(c.name.c_str(), "wall.lever.base"))
				{
					for (auto d : components)
					{
						if (Eq(d.name.c_str(), "wall.lever"))
						{
							leverOmega = -leverSpeed;
							break;
						}
					}
				}
			}
		}

		float padIntrusion = 0;
		float padDirection = 0.0f;

		const Metres DOOR_MAX_ELEVATION = 3.9_metres;
		const MetresPerSecond DOOR_ELEVATION_SPEED = 0.4_mps;

		const Metres PAD_MAX_INTRUSION = 0.1_metres;
		const MetresPerSecond PAD_DESCENT_SPEED = 0.40_mps;
		const MetresPerSecond PAD_ASCENT_SPEED = 1.0_mps;

		void ToggleElevation() override
		{
			for (auto c : components)
			{
				if (Eq(c.name.c_str(), "door.body"))
				{
					if (doorDirection == 0.0f)
					{
						if (doorElevation > 1.5f)
						{
							doorDirection = -DOOR_ELEVATION_SPEED;
						}
						else
						{
							doorDirection = DOOR_ELEVATION_SPEED;
						}
					}
					else
					{
						doorDirection *= -1.0f;
					}
					break;
				}
				else if (Eq(c.name.c_str(), "wall.lever.base"))
				{
					for (auto d : components)
					{
						if (Eq(d.name.c_str(), "wall.lever"))
						{
							if (leverOmega == 0.0f)
							{
								if (leverElevation < 0)
								{
									leverOmega = leverSpeed;
								}
								else
								{
									leverOmega = -leverSpeed;
								}
							}
							else
							{
								leverOmega *= -1.0f;
							}
							break;
						}
					}
				}
			}
		}

		bool TraversalBlocked() const override
		{
			return doorElevation < 2.0_metres;
		}

		void UpdateDoor(ID_ENTITY idDoor, const IUltraClock& clock)
		{
			auto door = platform.world.ECS.GetBodyComponent(idDoor);
			if (!door) return;

			doorElevation += doorDirection * clock.DT();

			if (doorElevation < 0)
			{
				doorDirection = 0.0f;
				doorElevation = 0.0f;
			}

			if (doorElevation > DOOR_MAX_ELEVATION)
			{
				doorDirection = 0.0f;
				doorElevation = DOOR_MAX_ELEVATION;
			}

			Matrix4x4 model = door->Model();
			model.row2.w = doorElevation;
			door->SetModel(model);
		}

		OS::ticks lastPlayerOccupiedTime = 0;

		void UpdatePressurePad(ID_ENTITY idPressurePad, const IUltraClock& clock)
		{
			auto pad = platform.world.ECS.GetBodyComponent(idPressurePad);
			if (!pad) return;

			padIntrusion += padDirection * clock.DT();

			if (lastPlayerOccupiedTime >= clock.FrameStart())
			{
				padDirection = PAD_DESCENT_SPEED;
			}
			else
			{
				padDirection = -PAD_ASCENT_SPEED;
			}

			if (padIntrusion > PAD_MAX_INTRUSION)
			{
				ai.Trigger(TriggerType::Pressed);
				padIntrusion = PAD_MAX_INTRUSION;
			}

			if (padIntrusion < 0)
			{
				ai.Trigger(TriggerType::Depressed);
				padIntrusion = 0;
			}

			Matrix4x4 model = pad->Model();
			model.row2.w = -padIntrusion;

			pad->SetModel(model);
		}

		void UpdateLever(ID_ENTITY idLeverBase, ID_ENTITY idLever, const IUltraClock& clock)
		{
			auto base = platform.world.ECS.GetBodyComponent(idLeverBase);
			if (!base) return;

			auto lever = platform.world.ECS.GetBodyComponent(idLever);
			if (!lever) return;

			size_t nTriangles;
			auto* tris = platform.graphics.meshes.GetTriangles(base->Mesh(), nTriangles);
			if (!tris) return;

			auto& t = *tris;
			Vec3 pos = (t.a.position + t.c.position) * 0.5f;
			Vec3 normalU = Cross(t.b.position - t.a.position, t.c.position - t.a.position);
			if (LengthSq(normalU) <= 0)
			{
				return;
			}

			leverElevation += leverOmega * clock.DT();
			if (leverElevation < LEVER_DOWN_ANGLE)
			{
				leverElevation = LEVER_DOWN_ANGLE;
				ai.Trigger(TriggerType::Depressed);
				leverOmega = 0;
			}
			else if (leverElevation > LEVER_UP_ANGLE)
			{
				leverElevation = LEVER_UP_ANGLE;
				ai.Trigger(TriggerType::Pressed);
				leverOmega = 0;
			}

			Vec3 normal = Normalize(normalU);
			Vec3 tangent = Normalize(t.a.position - t.b.position);
			Vec3 bitangent = Cross(normal, tangent);

			Matrix4x4 model =
			{
			  { tangent.x, -bitangent.x, -normal.x,  0 },
			  { tangent.y, -bitangent.y, -normal.y,  0 },
			  { tangent.z, -bitangent.z, -normal.z,  0 },
			  {         0,        0,           0, 1.0f },
			};

			auto Ry = Matrix4x4::RotateRHAnticlockwiseY(Degrees{ -leverElevation });

			Matrix4x4 T = Ry * model;
			T.row0.w = pos.x;
			T.row1.w = pos.y;
			T.row2.w = pos.z;

			lever->SetModel(T);
		}

		void NotifySectorPlayerIsInSector(const IUltraClock& clock) override
		{
			lastPlayerOccupiedTime = clock.FrameStart();
		}

		bool TryClickButton(ID_ENTITY idButton, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach) override
		{
			auto button = platform.world.ECS.GetBodyComponent(idButton);
			auto idMesh = button->Mesh();
			cr_m4x4 model = button->Model();

			size_t nTriangles;
			auto* triangles = platform.graphics.meshes.GetPhysicsHull(idMesh, nTriangles);
			if (triangles)
			{
				for (size_t i = 0; i < nTriangles; ++i)
				{
					const auto& T = triangles[i];

					Triangle ABC;
					TransformPositions(&T.A, 3, model, &ABC.A);

					Collision c = Rococo::CollideLineAndTriangle(ABC, probePoint, probeDirection);

					if (c.contactType == ContactType_Face)
					{
						if (c.t > 0 && c.t < reach)
						{
							if (Dot(probeDirection, T.EdgeCrossProduct()) < 0)
							{
								ClickButton();
								return true;
							}
						}
					}
				}
			}

			return false;
		}


		void ClickButton() override
		{
			if (doorDirection == 0)
			{
				if (doorElevation > 1.5f)
				{
					doorDirection = -DOOR_ELEVATION_SPEED;
				}
				else
				{
					doorDirection = DOOR_ELEVATION_SPEED;
				}
			}
		}

		void ClickLever() override
		{	
			if (leverElevation < 0)
			{
				leverOmega = leverSpeed;
			}
			else
			{
				leverOmega = -leverSpeed;
			}
		}

		void ForEachComponent(IEventCallback<const ComponentRef>& cb) override
		{
			for (auto& c : components)
			{
				cb.OnEvent({ c.name.c_str(), c.id });
			}
		}

		void OnTick(const IUltraClock& clock) override
		{
			for (auto c : components)
			{
				if (Eq(c.name.c_str(), "door.body"))
				{
					UpdateDoor(c.id, clock);
					break;
				}
				else if (Eq(c.name.c_str(), "pressure_pad"))
				{
					UpdatePressurePad(c.id, clock);
					break;
				}
				else if (Eq(c.name.c_str(), "wall.lever.base"))
				{
					for (auto d : components)
					{
						if (Eq(d.name.c_str(), "wall.lever"))
						{
							UpdateLever(c.id, d.id, clock);
							break;
						}
					}
				}
			}
		}
	};
}

namespace HV
{
	ISectorContents* CreateSectorContents(Platform& platform, ISector& sector, ISectorAIBuilderSupervisor& ai)
	{
		return new SC(platform, sector, ai);
	}
}