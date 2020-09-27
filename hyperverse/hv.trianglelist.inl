namespace HV
{
	struct TriangleListBinding : public  ITriangleList
	{
		std::vector<VertexTriangle>& tris;

		TriangleListBinding(std::vector<VertexTriangle>& _tris) : tris(_tris)
		{

		}

		void AddTriangleByVertices(ObjectVertex& a, ObjectVertex& b, ObjectVertex& c) override
		{
			if (a.position == b.position) return;
			if (a.position == c.position) return;
			if (b.position == c.position) return;

			VertexTriangle vt{ a, b, c };
			AddTriangle(vt);
		}

		void AddTriangle(VertexTriangle& abc) override
		{
			enum { MAX = 100000 };
			if (tris.size() > MAX)
			{
				Throw(0, "TriangleListBinding::AddTriangle... maximum %d triangles reached. Be kind to laptop users.", MAX);
			}
			tris.push_back(abc);
		}

		void AddQuad(ObjectVertex& a, ObjectVertex& b, ObjectVertex& c, ObjectVertex& d) override
		{
			AddTriangleByVertices(a, b, c);
			AddTriangleByVertices(c, d, a);
		}

		int32 CountVertices() override
		{
			return (int32)tris.size() * 3;
		}

		int32 CountTriangles()  override
		{
			return (int32)tris.size();
		}
	};
}
