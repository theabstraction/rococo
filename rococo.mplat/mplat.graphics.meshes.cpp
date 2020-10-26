#include <rococo.mplat.h>
#include <rococo.hashtable.h>
#include <vector>
#include <algorithm>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;

   struct MeshBinding
   {
	   ID_SYS_MESH id; // Mesh id as managed by the renderer
	   AABB bounds; // Bounding box surrounding mesh
	   ObjectVertex* pVertexArray;
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

	   MeshBindingEx(MeshBindingEx&& other) :
		   bind(other.bind),
		   name(other.name)
	   {

	   }

	   MeshBindingEx& operator = (const MeshBindingEx&& other)
	   {
		   bind = other.bind;
		   name = other.name;
		   return *this;
	   }
   };

   struct MeshBuilder : public Rococo::Graphics::IMeshBuilderSupervisor, IMathsVenue
   {
      stringmap<MeshBinding*> meshes;
	  std::unordered_map<ID_SYS_MESH, MeshBindingEx, ID_SYS_MESH> idToName;
	  char name[MAX_FQ_NAME_LEN + 1] = { 0 };
      std::vector<ObjectVertex> vertices;
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
			  renderer.DeleteMesh(mesh->id);
			  delete[] mesh->pVertexArray;
			  delete mesh;
		  }
	  }

      void Clear() override
      {
         name[0] = 0;
         vertices.clear();
		 physicsHull.clear();
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

		  renderer.SetShadowCasting(i->second->id, isActive);
	  }

      void Delete(const fstring& fqName) override
      {
         auto i = meshes.find((cstr) fqName);
         if (i != meshes.end())
         {
            auto mesh = i->second;
            renderer.DeleteMesh(mesh->id);
			delete[] mesh->pVertexArray;
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
			  renderer.GetMeshDesc(desc, m.item.second->id);

			  auto& b = m.item.second->bounds;
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
		  auto i = meshes.find((cstr)mesh);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::AddMesh(..., %s) fail. Mesh name not recognized", (cstr)mesh);
		  }

		  auto* v = i->second->pVertexArray;

		  if (*name == 0) Throw(0, "Call MeshBuilder.Begin() first");

		  for (size_t j = 0; j < i->second->nVertices; j++)
		  {
			  auto v0 = v[j];
			  ObjectVertex v1 = v0;
			  TransformPosition(transform, v0.position, v1.position);
			  TransformDirection(transform, v0.normal, v1.normal);
			  vertices.push_back(v1);
		  }
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
         const ObjectVertex* v = vertices.empty() ? nullptr : (const ObjectVertex*)&vertices[0];

		 AABB boundingBox;

		 for (auto& v : vertices)
		 {
			 boundingBox << v.position;
		 }

		 ObjectVertex* backup = nullptr;

		 if (preserveCopy)
		 {
			 backup = new ObjectVertex[vertices.size()];
			 memcpy(backup, &vertices[0], vertices.size() * sizeof(ObjectVertex));
		 }

         auto i = meshes.find(name);
         if (i != meshes.end())
         {
			i->second->bounds = boundingBox;
            renderer.UpdateMesh(i->second->id, v, (uint32)vertices.size());

			if (preserveCopy)
			{
				i->second->nVertices = vertices.size();
				delete[] i->second->pVertexArray;
				i->second->pVertexArray = backup;
				i->second->physicsHull = physicsHull;
			}
			else if (!physicsHull.empty())
			{
				i->second->nVertices = 0;
				delete[] i->second->pVertexArray;
				i->second->pVertexArray = nullptr;
				i->second->physicsHull = physicsHull;
			}
		 }
		 else
		 {
			 auto id = renderer.CreateTriangleMesh(invisible ? nullptr : v, invisible ? 0 : (uint32)vertices.size());
			 auto binding = new MeshBinding{ id, boundingBox, backup, vertices.size(), physicsHull };
			 meshes[name] = binding;
			 idToName[id] = MeshBindingEx( binding, name );
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
			  buffer.Resize(count * 256 + 256);

			  StackStringBuilder sb((char*)buffer.GetData(), buffer.Length());

			  sb << "Material Id # " << i->second->id.value << "," << name << "\n\n";
			  sb << "TriangleId,VertexId,X,Y,Z,U,V,Nx,Ny,Nz,Colour,Mat,Gloss\n\n";

			  for (size_t k = 0; k < count; ++k)
			  {
				  auto& v = i->second->pVertexArray[k];
				  sb << k / 3 << "," << k << "," << v.position.x << "," << v.position.y << "," << v.position.z << ",";
				  sb << v.uv.x << "," << v.uv.y << "," << v.normal.x << "," << v.normal.y << "," << v.normal.z << ",";
				  sb.AppendFormat("0x%8.8X", *(int*)(&v.material.colour));
				  sb << ",";
				  sb << v.material.materialId << "," << v.material.gloss << "\n";
			  }
		  }
		  else
		  {
			  buffer.Resize(0);
			  return;
		  }
	  }

	  void Span(Vec3& span, const fstring& name) override
	  {
		  auto i = meshes.find((cstr)name);
		  if (i == meshes.end())
		  {
			  Throw(0, "MeshBuilder::Span(...%s): mesh not found", (cstr)name);
		  }

		  span = i->second->bounds.Span();
	  }

	  bool TryGetByName(cstr name, ID_SYS_MESH& id, AABB& bounds) override
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