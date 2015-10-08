#include "meshes.h"

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

#include <unordered_map>

#include "dystopia.h"

namespace Dystopia
{
	using namespace Rococo;
	using namespace Sexy::Sex;
	ISourceCode* DuplicateSourceCode(IOS& os, ISParser& parser, const IBuffer& rawData, const wchar_t* resourcePath);
	void ThrowSex(cr_sex s, const wchar_t* format, ...);
	void ScanExpression(cr_sex s, const wchar_t* hint, const char* format, ...);
	void ValidateArgument(cr_sex s, const wchar_t* arg);
	int32 GetValue(cr_sex s, int32 minValue, int32 maxValue, const wchar_t* hint);
	Vec3 GetVec3Value(cr_sex x, cr_sex y, cr_sex z);
	float GetValue(cr_sex s, float minValue, float maxValue, const wchar_t* hint);
	RGBAb GetColourValue(cr_sex s);
	void ExecuteSexyScript(size_t nMegabytesCapacity);
}

namespace
{
	using namespace Rococo;
	using namespace Dystopia;
	using namespace Sexy;
	using namespace Sexy::Sex;

	struct MeshDesc
	{
		ID_MESH rendererId;
		int32 editorId;
		std::wstring resourceName;
	};

	typedef std::unordered_map<int, MeshDesc> TMeshes;

	void ParseVertex(ObjectVertex& v, cr_sex sv)
	{
		const ISExpression* svx, *svy, *svz, *snx, *sny, *snz, *semissive, *sdiffuse;
		ScanExpression(sv, L"(vx vy vz nx ny nz emissive diffuse)", "a a a a a a a a", &svx, &svy, &svz, &snx, &sny, &snz, &semissive, &sdiffuse);

		v.position = GetVec3Value(*svx, *svy, *svz);
		v.normal = GetVec3Value(*snx, *sny, *snz);
		v.emissiveColour = GetColourValue(*semissive);
		v.diffuseColour = GetColourValue(*sdiffuse);
	}

	void ParseMeshVertices(TMeshes& meshes, int32 id, IRenderer& renderer, cr_sex meshDef, const wchar_t* resourcePath, bool isReloading)
	{
		size_t nVertices = meshDef.NumberOfElements() - 2;
		std::vector<ObjectVertex> vertices;
		vertices.reserve(nVertices);

		if (nVertices % 3 != 0)
		{
			ThrowSex(meshDef, L"Expecting 3 vertices per triangle");
		}

		ObjectVertex v{ Vec3(0,0,0), Vec3(0,0,0), RGBAb(0,0,0), RGBAb(0,0,0) };

		for (int i = 0; i < nVertices; ++i)
		{
			cr_sex sv = meshDef[i + 2];
			ParseVertex(v, sv);
			vertices.push_back(v);
		}

		if (isReloading)
		{
			auto i = meshes.find(id);
			if (i != meshes.end())
			{
				if (wcscmp(resourcePath, i->second.resourceName.c_str()) == 0)
				{
					renderer.UpdateMesh(i->second.rendererId, &vertices[0], (uint32)vertices.size());
				}
				else
				{
					Throw(0, L"Cannot update mesh %d from %s - it is already defined by another mesh file %s", id, resourcePath, i->second.resourceName.c_str());
				}
				return;
			}
		}

		auto rendererId = renderer.CreateTriangleMesh(&vertices[0], (uint32) vertices.size());
		meshes.insert(std::make_pair(id, MeshDesc{ rendererId, id, resourcePath }));
	}

	void ParseMeshScript(TMeshes& meshes, ISParserTree& tree, IRenderer& renderer, const wchar_t* resourcePath, bool isReloading)
	{
		cr_sex root = tree.Root();
		
		if (root.NumberOfElements() < 1)
		{
			ThrowSex(root, L"No elements in the script file");
		}

		cr_sex version = root[0];

		const ISExpression* quote, *category, *filetype;
		ScanExpression(version, L"(' file.type rococo.dystopia.mesh)", "a a a", &quote, &category, &filetype);

		ValidateArgument(*quote, L"'");
		ValidateArgument(*category, L"file.type");
		ValidateArgument(*filetype, L"rococo.dystopia.mesh");

		for (int i = 1; i < root.NumberOfElements(); ++i)
		{
			cr_sex meshDef = root[i];
			const ISExpression* meshconfirm, *meshId;
			ScanExpression(meshDef, L"(mesh <mesh_id> (vertices))", "a a", &meshconfirm, &meshId);

			ValidateArgument(*meshconfirm, L"mesh");

			int32 id = GetValue(*meshId, 1, 0x7FFFFFFF, L"mesh_id");

			if (!isReloading)
			{
				auto& m = meshes.find(id);
				if (m != meshes.end())
				{
					ThrowSex(*meshId, L"A mesh with id is already defined", id);
				}
			}

			ParseMeshVertices(meshes, id, renderer, meshDef, resourcePath, isReloading);
		}
	}

	class MeshLoader : public IMeshLoader
	{
		Environment& e;
		AutoFree<IExpandingBuffer> meshFileImage;
		AutoFree<IExpandingBuffer> unicodeFileImage;
		TMeshes meshes;

		enum { MAX_MESH_FILE_SIZE = 256 * 1024 };
	public:
		MeshLoader(Environment& _e):
			e(_e), 
			meshFileImage(CreateExpandingBuffer(MAX_MESH_FILE_SIZE)),
			unicodeFileImage(CreateExpandingBuffer(0))
		{
		}

		ID_MESH GetRendererId(int32 editorId)
		{
			auto i = meshes.find(editorId);
			if (i == meshes.end())
			{
				return 0;
			}

			return i->second.rendererId;
		}

		virtual void LoadMeshes(const wchar_t* resourcePath, bool isReloading)
		{
			using namespace Dystopia;

			while (true)
			{
				try
				{
					ProtectedLoadMeshes(resourcePath, isReloading);
					return;
				}
				catch (IException& ex)
				{
					GetOS(e).FireUnstable();
					CMD_ID id = ShowContinueBox(e.renderer.Window(), ex.Message());
					switch (id)
					{
					case CMD_ID_EXIT:
						Throw(ex.ErrorCode(), L"%s", ex.Message());
						break;
					case CMD_ID_RETRY:
						break;
					case CMD_ID_IGNORE:
						return;
					}
				}
			}
		}

		void ProtectedLoadMeshes(const wchar_t* resourcePath, bool isReloading)
		{
			e.installation.LoadResource(resourcePath, *meshFileImage, MAX_MESH_FILE_SIZE - 2);

			size_t len = meshFileImage->Length();

			if (len < 2)
			{
				Throw(0, L"Mesh file was too small: %s", resourcePath);
			}

			char* data = (char*) meshFileImage->GetData();
			data[len] = 0;
			data[len+1] = 0;

			CSParserProxy pp;
			ISParser& parser = pp();

			Auto<ISourceCode> source = DuplicateSourceCode(GetOS(e), parser, *meshFileImage, resourcePath);

			csexstr code = source->SourceStart();

			try
			{
				Auto<ISParserTree> tree = parser.CreateTree(*source);
				ParseMeshScript(meshes, *tree, e.renderer, resourcePath, isReloading);
			}
			catch (Sexy::Sex::ParseException& pex)
			{
				SourcePos p = pex.Start();
				SourcePos q = pex.End();
				Throw(pex.ErrorCode(), L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)", resourcePath, pex.Name(), pex.Message(), p.X, p.Y, q.X, q.Y);
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void UpdateMesh(const wchar_t* filename)
		{
			for (auto i : meshes)
			{
				auto& mesh = i.second;
				if (DoesModifiedFilenameMatchResourceName(filename, mesh.resourceName.c_str()))
				{
					LoadMeshes(mesh.resourceName.c_str(), true);
					return;
				}
			}
		}
	};
}

namespace Dystopia
{
	IMeshLoader* CreateMeshLoader(Environment& e)
	{
		return new MeshLoader(e);
	}
}