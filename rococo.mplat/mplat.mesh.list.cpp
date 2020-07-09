#include <rococo.mplat.h>
#include <sexy.types.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Sex;

#include <vector>

struct FileVertex
{
	Vec3 position;
	Vec3 normal;
};

void Check(cr_sex s, cstr checkString)
{
	fstring sanityCheck = GetAtomicArg(s);
	if (!Eq(sanityCheck, checkString)) Throw(s, "Expected string '%s'", checkString);
}

float AsF32(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_ATOMIC)
	{
		Throw(s, "Expecting 32-bit floating point value at this position");
	}

	cstr text = s.String()->Buffer;

	// Microsoft's _atoflt does not create floating point exceptions
	// and could be faster that atof, as it is for 32-bit floats, rather than 64-bit doubles
	_CRT_FLOAT result;
	int code = _atoflt(&result, s.String()->Buffer);
	if (code != 0)
	{
		Throw(s, "Could not interpret string as a 32-bit floating point number");
	}
	return result.f;
}

int32 AsI32(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_ATOMIC)
	{
		Throw(s, "Expecting 32-bit int value at this position");
	}

	cstr text = s.String()->Buffer;
	return atoi(text);
}

MaterialId ParseMat(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_ATOMIC)
	{
		Throw(s, "Expecting 'mat<index>' at this position");
	}

	cstr text = s.String()->Buffer;
	if (Compare(text, "mat", 3) != 0)
	{
		Throw(s, "Expecting 'mat<index>' at this position. Prefix 'mat' not found");
	}

	int index = atoi(text + 3);
	return (MaterialId) index;
}

void ParsePolyVertex(ObjectVertex& v, cr_sex sPolyVertex, const std::vector<FileVertex>& vertexArray)
{
	if (sPolyVertex.Type() == EXPRESSION_TYPE_ATOMIC)
	{
		int vertexIndex = AsI32(sPolyVertex);
		if (vertexIndex < 0 || vertexIndex >= vertexArray.size())
		{
			Throw(sPolyVertex, "Index out of range. Max size is %llu", vertexArray.size() - 1);
		}

		auto& vertex = vertexArray[vertexIndex];
		v.normal = vertex.normal;
		v.position = vertex.position;
		v.uv = { 0,0 };
	}
	else
	{
		if (sPolyVertex.NumberOfElements() != 3) Throw(sPolyVertex, "Expecting (<index> u v)");

		int vertexIndex = AsI32(sPolyVertex[0]);
		if (vertexIndex < 0 || vertexIndex >= vertexArray.size())
		{
			Throw(sPolyVertex[0], "Index out of range. Max size is %llu", vertexArray.size() - 1);
		}

		auto& vertex = vertexArray[vertexIndex];
		v.normal = vertex.normal;
		v.position = vertex.position;

		float txu = AsF32(sPolyVertex[1]);
		float txv = AsF32(sPolyVertex[2]);

		v.uv = { txu, txv };
	}
}

fstring GetStringLiteral(cr_sex s)
{
	if (s.Type() != EXPRESSION_TYPE_STRING_LITERAL) Throw(s, "Expecting string literal");
	return fstring{ s.String()->Buffer, s.String()->Length };
}

void LoadMesh(Platform& platform, cr_sex s)
{
	// (mesh <name> (vertices ...)(polygons ...)))

	if (s.NumberOfElements() != 4) Throw(s, "Expected (mesh <name> (vertices ...)(polygons ...)))");

	Check(s[0], "mesh");

	fstring name = GetStringLiteral(s[1]);

	static std::vector<FileVertex> vertexArray;
	vertexArray.clear();

	cr_sex sVertices = s[2];
	if (sVertices.NumberOfElements() < 2)
	{
		Throw(sVertices, "Expected (vertices ...)");
	}

	Check(sVertices[0], "vertices");

	for (int32 i = 1; i < sVertices.NumberOfElements(); ++i)
	{
		FileVertex v;
		cr_sex sVertex = sVertices[i];
		if (sVertex.NumberOfElements() != 7) Throw(sVertex, "Expected (<index> px py pz nx ny nz)");
		v.position.x = AsF32(sVertex[1]);
		v.position.y = AsF32(sVertex[2]);
		v.position.z = AsF32(sVertex[3]);
		v.normal.x = AsF32(sVertex[4]);
		v.normal.y = AsF32(sVertex[5]);
		v.normal.z = AsF32(sVertex[6]);
		vertexArray.push_back(v);
	}

	cr_sex sPolygons = s[3];
	if (sPolygons.NumberOfElements() < 2)
	{
		Throw(sPolygons, "Expected (polygons ...)");
	}

	Check(sPolygons[0], "polygons");

	platform.meshes.Clear();
	platform.meshes.Begin(name);

	for (int32 i = 1; i < sPolygons.NumberOfElements(); ++i)
	{
		cr_sex sPoly = sPolygons[i];
		// (<index> matId v1 v2 v3 [v4]))
		int nElements = sPoly.NumberOfElements();

		if (nElements < 5 || nElements > 7) 
			Throw(sPoly, "Expecting polygon def (<index> matId vindex1 vindex2 vindex3 [vindex4] [vindex5])");

		MaterialId matId = ParseMat(sPoly[1]);

		ObjectVertex v[5];
		v[0].material = MaterialVertexData{ 0xFFFFFFFF, matId, 0 };
		v[1].material = v[0].material;
		v[2].material = v[0].material;
		v[3].material = v[0].material;
		v[4].material = v[0].material;

		for (int32 j = 0; j < 3; j++)
		{
			ParsePolyVertex(v[j], sPoly[j + 2], vertexArray);
		}

		if (nElements == 5)
		{
			const auto& tri = (VertexTriangle&)v[0];
			platform.meshes.AddTriangleEx(tri);
		}
		else if (nElements == 6)
		{
			// Quad
			ParsePolyVertex(v[3], sPoly[5], vertexArray);

			platform.meshes.AddTriangle(v[0], v[1], v[2]);
			platform.meshes.AddTriangle(v[2], v[3], v[0]);
		}
		else if (nElements == 7)
		{
			// Pentagon
			ParsePolyVertex(v[3], sPoly[5], vertexArray);
			ParsePolyVertex(v[4], sPoly[6], vertexArray);

			platform.meshes.AddTriangle(v[0], v[1], v[2]);
			platform.meshes.AddTriangle(v[0], v[2], v[3]);
			platform.meshes.AddTriangle(v[0], v[3], v[4]);
		}
		else
		{
			Throw(sPoly, "Expecting 3, 4 or 5 vertices. Found %d", nElements - 2);
		}
	}

	platform.meshes.End(false, false);
}

namespace Rococo
{
	namespace M
	{
		void LoadMeshesFromSExpression(Platform& platform, cr_sex s)
		{
			// (ISExpression meshes = (' (mesh ...)(mesh ...)...))
			//           s =          ^^^^^^^^^^^^^^^^^^^^^^^^^^^
			// Ergo s[0] is ' character
			int32 nMeshes = s.NumberOfElements() - 1;
			for (int32 i = 0; i < nMeshes; ++i)
			{
				cr_sex smesh = s[i + 1];
				LoadMesh(platform, smesh);
			}
		}
	}
}