#include "hv.h"
#include <vector>
#include <algorithm>
#include <rococo.strings.h>
#include <rococo.random.h>

using namespace HV::HVMaths;

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
	public:
		SC(Platform& _platform, ISector& _sector) : 
			platform(_platform), sector(_sector), shuffler(_sector.Id() + 5550)
		{}

		ID_ENTITY AddItemToLargestSquare(const fstring& meshName, int addItemFlags, const HV::ObjectCreationSpec& obs) override
		{
			SectorSquares squares = sector.Squares();

			if (sector.IsCorridor() || squares.first == squares.end) return ID_ENTITY::Invalid();

			auto& aabb = *squares.first;

			ID_SYS_MESH meshId;
			AABB bounds;
			if (!platform.meshes.TryGetByName(meshName, meshId, bounds)) return ID_ENTITY::Invalid();

			AABB worldBounds;
			Matrix4x4 model;
			const float z0 = sector.Z0();
			const float z1 = sector.Z1();
			if (!TryGetRandomTransformation(model, worldBounds, addItemFlags & AddItemFlags_RandomHeading, addItemFlags & AddItemFlags_RandomPosition, bounds, aabb, z0, z1))
			{
				return ID_ENTITY::Invalid();
			}

			auto id = platform.instances.AddBody(meshName, model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());
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
					auto* e = platform.instances.GetEntity(id);
					if (e)
					{
						auto meshId = e->MeshId();
						auto name = platform.meshes.GetName(meshId);
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
				platform.instances.Delete(s.id);
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

			auto* e = platform.instances.GetEntity(centrePieceId);
			if (e == nullptr)
			{
				return ID_ENTITY::Invalid();
			}

			ID_SYS_MESH meshId;
			AABB bounds;
			if (!platform.meshes.TryGetByName(mesh, meshId, bounds)) return ID_ENTITY::Invalid();

			candidates.clear();

			const float z0 = sector.Z0();
			const float z1 = sector.Z1();

			for (int i = 0; i < 100; i++)
			{
				AABB worldBounds;
				Matrix4x4 model;
				if (!TryGetRandomTransformation(model, worldBounds, iis.insertFlags & AddItemFlags_RandomHeading, iis.insertFlags & AddItemFlags_RandomPosition, bounds, aabb, z0, z1))
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

			bool sortByOrientation = iis.insertFlags & AddItemFlags_AlignEdge;

			if (sortByOrientation)
			{
				sortByRangeFromCentrePieceAndOrientation.optimalRangeSq = Sq(iis.maxDistance.value);
				sortByRangeFromCentrePieceAndOrientation.centrePieceOrigin = e->Position();
				std::sort(candidates.begin(), candidates.end(), sortByRangeFromCentrePieceAndOrientation);
			}
			else
			{
				sortByRangeFromCentrePiece.centrePieceOrigin = e->Position();
				std::sort(candidates.begin(), candidates.end(), sortByRangeFromCentrePiece);
			}

			auto id = platform.instances.AddBody(mesh, candidates[0].model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());
			scenery.push_back({ id,candidates[0].worldBounds });

			return id;
		}

		void ForEveryObjectInContent(IEventCallback<const ID_ENTITY>& cb) override
		{
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

					auto* e = platform.instances.GetEntity(id);
					if (e)
					{
						const Vec3 up{ 0, 0, 1 };
						auto& model = e->Model();
						size_t triangleCount = 0;
						auto tris = platform.meshes.GetTriangles(e->MeshId(), triangleCount);

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

			auto e = platform.instances.GetEntity(quadsEntityId);
			auto item = platform.instances.GetEntity(itemId);
			if (e && item)
			{
				if (IsQuadRectangular(qModel) && qModel.a.x == qModel.b.x || qModel.a.y == qModel.b.y)
				{
					Quad qWorld;
					TransformPositions(&qModel.a, 4, e->Model(), &qWorld.a);

					auto bounds = platform.meshes.Bounds(item->MeshId());
					if (bounds.minXYZ.x < bounds.maxXYZ.x)
					{
						AABB2d minSquare;
						minSquare << AsVec2(qModel.a) << AsVec2(qModel.b) << AsVec2(qModel.c) << AsVec2(qModel.d);

						Matrix4x4 randomModel;
						AABB worldBounds;
						if (TryGetRandomTransformation(randomModel, worldBounds, true, true, bounds, minSquare, qModel.a.z, z1 - z0))
						{
							ManageEntity(itemId);
							item->Model() = e->Model() * randomModel;
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

	};
}

namespace HV
{
	ISectorContents* CreateSectorContents(Platform& platform, ISector& sector)
	{
		return new SC(platform, sector);
	}
}