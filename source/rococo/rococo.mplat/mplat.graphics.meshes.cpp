#include <rococo.mplat.h>
#include <rococo.hashtable.h>
#include <components/rococo.components.body.h>
#include <rococo.io.h>
#include <vector>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

namespace
{


   struct MeshBinding
   {
	   ID_SYS_MESH id; // Mesh id as managed by the renderer
	   AABB bounds; // Bounding box surrounding mesh
	   ObjectVertex* pVertexArray;
	   BoneWeights* pWeightArray;
	   size_t nVertices;
	   std::vector<Triangle> physicsHull;
   };

   struct MeshBindingEx
   {
	   MeshBinding* bind;
	   HString name;

	   MeshBindingEx():
		   bind(nullptr)
	   {

	   }

	   MeshBindingEx(MeshBinding* _bind,cstr _name) :
		   bind(_bind), name(_name)
	   {

	   }

	   MeshBindingEx(const MeshBindingEx& other):
		   bind(other.bind),
		   name(other.name)
	   {

	   }

	   MeshBindingEx(MeshBindingEx&& other) noexcept :
		   bind(other.bind),
		   name(other.name)
	   {

	   }

	   MeshBindingEx& operator = (const MeshBindingEx&& other) noexcept
	   {
		   bind = other.bind;
		   name = other.name;
		   return *this;
	   }
   };

   struct MeshBuilder : public Rococo::Graphics::IMeshBuilderSupervisor, IMathsVenue, Rococo::Components::Body::IBodyMeshDictionary
   {
      stringmap<MeshBinding*> meshes;
	  std::unordered_map<ID_SYS_MESH, MeshBindingEx, ID_SYS_MESH> idToName;
	  char name[MAX_FQ_NAME_LEN + 1] = { 0 };
      std::vector<ObjectVertex> vertices;
	  std::vector<BoneWeights> weights;
	  std::vector<Triangle> physicsHull;
      IRenderer& renderer;

      MeshBuilder(IRenderer& _renderer) : renderer(_renderer)
      {
      }

	  ~MeshBuilder()
	  {
		  for (auto& i : meshes)
		  {
			  auto mesh = i.second;
			  renderer.Meshes().DeleteMesh(mesh->id);
			  delete[] mesh->pVertexArray;
			  delete[] mesh->pWeightArray;
			  delete mesh;
		  }
	  }

	  Rococo::Components::Body::IBodyMeshDictionary& MeshDictionary()
	  {
		  return *this;
	  }

	  void AddQuad(const Rococo::QuadVertices& q) override
	  {
		  UNUSED(q);
	  }

      void Clear() override
      {
         name[0] = 0;
         vertices.clear();
		 physicsHull.clear();
		 weights.clear();
      }

	  const VertexTriangle* GetTriangles(ID_SYS_MESH id, size_t& nTriangles) const override
	  {
		  auto i = idToName.find(id);
		  if (i == idToName.end() || i->second.bind->pVertexArray == nullptr)
		  {
			  nTriangles = 0;
			  return nullptr;
		  }

		  nTriangles = i->second.bind->nVertices / 3;
		  return (const VertexTriangle*)i->second.bind->pVertexArray;
	  }

	  const Triangle* GetPhysicsHull(ID_SYS_MESH id, size_t& nTriangles) const override
	  {
		  auto i = idToName.find(id);
		  if (i == idToName.end() || i->second.bind->physicsHull.empty())
		  {
			  nTriangles = 0;
			  return nullptr;
		  }

		  nTriangles = i->second.bind->physicsHull.size();
		  return (const Triangle*)i->second.bind->physicsHull.data();
	  }

	  AABB Bounds(ID_SYS_MESH id) const override
	  {
		  auto i = idToName.find(id);
		  if (i == idToName.end())
		  {
			  return AABB();
		  }
		  else
		  {
			  return i->second.bind->bounds;
		  }
	  }

	  const fstring GetName(ID_SYS_MESH id) const override
	  {
		  auto i = idToName.find(id);
		  return i != idToName.end() ? fstring { i->second.name, (int32) i->second.name.length() } : fstring{ "", 0 };
	  }

	  void SetShadowCasting(const fstring& meshName, boolean32 isActive) override
	  {
		  auto i = meshes.find((cstr)meshName);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::SetShadowCasating('%s') - mesh unknown. %s", (cstr)meshName);
		  }

		  renderer.Meshes().SetShadowCasting(i->second->id, isActive);
	  }

      void Delete(const fstring& fqName) override
      {
         auto i = meshes.find((cstr) fqName);
         if (i != meshes.end())
         {
            auto mesh = i->second;
            renderer.Meshes().DeleteMesh(mesh->id);
			delete[] mesh->pVertexArray;
			delete[] mesh->pWeightArray;
            meshes.erase(i);
			idToName.erase(mesh->id);
			delete mesh;
         }
      }

	  struct NameBinding
	  {
		  std::pair<HString, MeshBinding*> item;

		  NameBinding(cstr text, MeshBinding* mesh):
			  item(text,mesh)
		  {

		  }

		  bool operator < (const NameBinding& other) const
		  {
			  return strcmp(item.first, other.item.first) < 0;
		  }

		  bool operator == (const NameBinding& other) const
		  {
			  return Eq(item.first, other.item.first);
		  }
	  };

	  std::vector<NameBinding> names;

	  void ShowVenue(IMathsVisitor& visitor)
	  {
		  for (auto& m : meshes)
		  {
			  names.push_back( NameBinding(m.first,m.second) );
		  }

		  std::sort(names.begin(), names.end());

		  for(auto& m: names)
		  {
			  char desc[256];
			  renderer.Meshes().GetMeshDesc(desc, m.item.second->id);

		//	  auto& b = m.item.second->bounds;
			  visitor.ShowSelectableString("overlay.select.mesh", (cstr) m.item.first, "%5llu %s", m.item.second->id.value, desc);
		//	  visitor.ShowString(" -> bounds", "(%f, %f, %f) to (%f, %f, %f)", b.minXYZ.x, b.minXYZ.y, b.minXYZ.z, b.maxXYZ.x, b.maxXYZ.y, b.maxXYZ.z);
		  }

		  names.clear();
	  }

      void Free() override
      {
         delete this;
      }

	  IMathsVenue* Venue()
	  {
		  return this;
	  }

      void Begin(const fstring& fqName) override
      {
         if (*name != 0)
         {
            Throw(0, "Call MeshBuilder.End() first");
         }

         StackStringBuilder sb(name, sizeof(name));
         sb << fqName;
      }

	  void AddMesh(cr_m4x4 transform, const fstring& mesh)
	  {
		  if (*name == 0) Throw(0, "Call MeshBuilder.Begin() first");

		  auto i = meshes.find((cstr)mesh);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::AddMesh(..., %s) fail. Mesh name not recognized", (cstr)mesh);
		  }

		  if (i->second->pWeightArray != nullptr)
		  {
			  Throw(0, "Cannot AddMesh(..., %s) as the the vertices are weighted", mesh.buffer);
		  }

		  auto* v = i->second->pVertexArray;

		  for (size_t j = 0; j < i->second->nVertices; j++)
		  {
			  auto v0 = v[j];
			  ObjectVertex v1 = v0;
			  TransformPosition(transform, v0.position, v1.position);
			  TransformDirection(transform, v0.normal, v1.normal);
			  vertices.push_back(v1);
		  }
	  }

	  void AddBoneWeights(const BoneWeights& a, const BoneWeights& b, const BoneWeights& c) override
	  {
		  weights.push_back(a);
		  weights.push_back(b);
		  weights.push_back(c);
	  }

      void AddTriangle(const ObjectVertex& a, const ObjectVertex& b, const ObjectVertex& c) override
      {
         if (*name == 0) Throw(0, "MeshBuilder::AddTriangle - Call MeshBuilder.Begin() first");
         vertices.push_back(a);
         vertices.push_back(b);
         vertices.push_back(c);
      }

	  void AddTriangleEx(const VertexTriangle& t)
	  {
		  if (*name == 0) Throw(0, "MeshBuilder::AddTriangleEx - Call MeshBuilder.Begin() first");
		  vertices.push_back(t.a);
		  vertices.push_back(t.b);
		  vertices.push_back(t.c);
	  }

	  void End(boolean32 preserveCopy, boolean32 invisible) override
	  {
		  if (name[0] == 0)
		  {
			  Throw(0, "No name defined. Missing begin() ?");
		  }

		  const ObjectVertex* v = vertices.empty() ? nullptr : (const ObjectVertex*)&vertices[0];

		  if (!weights.empty() && weights.size() != vertices.size())
		  {
			  size_t nWeights = weights.size();
			  size_t nVertices = vertices.size();
			  Clear();
			  Throw(0, "Error creating mesh [%s]: weights defined, but %llu weights and %llu vertices???", name, nWeights, nVertices);
		  }

		  AABB boundingBox;

		  for (auto& vx : vertices)
		  {
			  boundingBox << vx.position;
		  }

		  ObjectVertex* backup = nullptr;
		  BoneWeights* backupWeights = nullptr;

		  if (preserveCopy)
		  {
			  backupWeights = weights.empty() ? nullptr : new BoneWeights[weights.size()];
			  if (backupWeights)
			  {
				  memcpy(backupWeights, weights.data(), weights.size() * sizeof(BoneWeights));
			  }
			  backup = new ObjectVertex[vertices.size()];
			  memcpy(backup, &vertices[0], vertices.size() * sizeof(ObjectVertex));
		  }

		  const BoneWeights* pWeights = weights.empty() ? nullptr : weights.data();

		  auto i = meshes.find(name);
		  if (i != meshes.end())
		  {
			  i->second->bounds = boundingBox;
			  renderer.Meshes().UpdateMesh(i->second->id, v, (uint32)vertices.size(), pWeights);

			  if (preserveCopy)
			  {
				  i->second->nVertices = vertices.size();
				  delete[] i->second->pVertexArray;
				  delete[] i->second->pWeightArray;
				  i->second->pVertexArray = backup;
				  i->second->pWeightArray = backupWeights;
				  i->second->physicsHull = physicsHull;
			  }
			  else if (!physicsHull.empty())
			  {
				  i->second->nVertices = 0;
				  delete[] i->second->pVertexArray;
				  i->second->pVertexArray = nullptr;
				  i->second->pWeightArray = nullptr;
				  i->second->physicsHull = physicsHull;
			  }
		  }
		  else
		  {
			  auto id = renderer.Meshes().CreateTriangleMesh(invisible ? nullptr : v, invisible ? 0 : (uint32)vertices.size(), pWeights);
			  auto binding = new MeshBinding{ id, boundingBox, backup, backupWeights, vertices.size(), physicsHull };
			  meshes[name] = binding;
			  idToName[id] = MeshBindingEx(binding, name);
		  }

		  Clear();
	  }

	  void SetSpecialAmbientShader(const fstring& fqName, const fstring& vsAmbientPingPath, const fstring& psAmbientPingPath, boolean32 alphaBlending)
	  {
		  auto i = meshes.find((cstr)fqName);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::SetSpecialAmbientShader(...%s): mesh not found", (cstr)name);
		  }

		  renderer.SetSpecialAmbientShader(i->second->id, vsAmbientPingPath, psAmbientPingPath, alphaBlending);
	  }

	  void SetSpecialSpotlightShader(const fstring& fqName, const fstring& vsSpotlightPingPath, const fstring& psSpotlightPingPath, boolean32 alphaBlending)
	  {
		  auto i = meshes.find((cstr)fqName);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::SetSpecialSpotlightShader(...%s): mesh not found", (cstr)name);
		  }

		  renderer.SetSpecialSpotlightShader(i->second->id, vsSpotlightPingPath, psSpotlightPingPath, alphaBlending);
	  }

	  void SaveCSV(cstr name, IExpandingBuffer& buffer) override
	  {
		  auto i = meshes.find((cstr)name);
		  if (i == meshes.end())
		  {
			  buffer.Resize(0);
			  return;
		  }

		  if (i->second->pVertexArray)
		  {
			  auto count = i->second->nVertices;
			  buffer.Resize(count * 256 + 256 + weights.size() * 128);

			  StackStringBuilder sb((char*)buffer.GetData(), buffer.Length());

			  sb << "Material Id # " << i->second->id.value << "," << name << "\n\n";
			  sb << "TriangleId,VertexId,X,Y,Z,U,V,Nx,Ny,Nz,Colour,Mat,Gloss\n\n";

			  for (size_t k = 0; k < count; ++k)
			  {
				  const auto& v = i->second->pVertexArray[k];
				  sb << k / 3 << "," << k << "," << v.position.x << "," << v.position.y << "," << v.position.z << ",";
				  sb << v.uv.x << "," << v.uv.y << "," << v.normal.x << "," << v.normal.y << "," << v.normal.z << ",";
				  sb.AppendFormat("0x%8.8X", *(int*)(&v.material.colour));
				  sb << ",";
				  sb << v.material.materialId << "," << v.material.gloss << "\n";
			  }

			  if (i->second->pWeightArray)
			  {
				  sb << "BoneWeights\n\n";
				  sb << "TriangleId,--Blank--,Index,weight,--Blank--,Index,weight)\n\n";
				  for (size_t k = 0; k < count; ++k)
				  {
					  const auto& w = i->second->pWeightArray[k];
					  sb.AppendFormat("%0.4d,\t\t,%0.0f,%4.4f,\t\t,%0.0f,%4.4f\n", k/3, w.bone0.index, w.bone0.weight, w.bone1.index, w.bone1.weight);
				  }
			  }
		  }
		  else
		  {
			  buffer.Resize(0);
			  return;
		  }
	  }

	  void Span(OUT Vec3& span, const fstring& name) override
	  {
		  auto i = meshes.find((cstr)name);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::Span(...%s): mesh not found", (cstr)name);
		  }

		  span = i->second->bounds.Span();
	  }

	  bool TryGetByName(cstr name, ID_SYS_MESH& id, AABB& bounds) const override
	  {
		  auto i = meshes.find(name);
		  if (i == meshes.end())
		  {
			  id = ID_SYS_MESH::Invalid();
			  bounds = AABB();
			  return false;
		  }
		  else
		  {
			  id = i->second->id;
			  bounds = i->second->bounds;
			  return true;
		  }
	  }

	  bool TryGetByName(const fstring& name, OUT Rococo::Components::Body::BodyMeshEntry& mesh) const override
	  {
		  return TryGetByName(name, OUT mesh.sysId, OUT mesh.bounds);
	  }

	  void AddPhysicsHull(const Triangle& t) override
	  {
		  if (*name == 0) Throw(0, "MeshBuilder::AddPhysicsHull - Call MeshBuilder.Begin() first");

		  Vec3 n = t.EdgeCrossProduct();

		  const float EPSILON = 1.0e-16f;
		  if (LengthSq(n) < EPSILON)
		  {
			  Throw(0, "MeshBuilder::AddPhysicsHull degenerate triangle. Normal length squared was less than %g", EPSILON);
		  }

		  physicsHull.push_back(t);
	  }
   };
}

namespace Rococo
{
   namespace Graphics
   {
      IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer)
      {
         return new MeshBuilder(renderer);
      }
   }
}