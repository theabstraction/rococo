#include <rococo.mplat.h>
#include <vector>

#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Graphics;

#include "rococo.rings.inl"

namespace
{
	class RimTesselator : public IRimTesselatorSupervisor
	{
		std::vector<Vec2> perimeter;
		std::vector<Triangle2d> topFaces;
		std::vector<Vec2> temp;
		Matrix4x4 transform = Matrix4x4::Identity();
		bool isOriented = false;
	public:
		void Free() override
		{
			delete this;
		}

		void AddPoint(const Vec2& p) override
		{
			perimeter.push_back(p);
			if (isOriented) isOriented = false;
		}

		void AddPointXY(float x, float y) override
		{
			perimeter.push_back({ x,y });
			if (isOriented) isOriented = false;
		}

		void CloseLoop() override
		{
			if (perimeter.size() < 3)
			{
				Throw(0, "RimTesselator::CloseLoop need at least 3 vertices in the perimeter to close the loop");
			}

			if (perimeter[0] == perimeter.back())
			{
				Throw(0, "RimTesselator::CloseLoop - the final vertex matches the first vertex already");
			}

			perimeter.push_back(perimeter[0]);
		}

		void MakeElipse(int32 numberOfSides, float sx, float sy) override
		{
			Clear();

			if (numberOfSides < 2 || numberOfSides > 1440)
			{
				Throw(0, "RimTesselator::MakeElipse(%d, ...), numberOfSides domain [3,1440]");
			}

			float round = 360_degrees;
			float segment = round / numberOfSides;

			for (int32 i = 0; i < numberOfSides; ++i)
			{
				Degrees theta{ i * segment };
				float x = sx * Sin(theta);
				float y = sy * Cos(theta);
				perimeter.push_back({ x,y });
			}

			perimeter.push_back(perimeter[0]);
		}

		void Clear() override
		{
			perimeter.clear();
			topFaces.clear();
			isOriented = false;
		}

		void Scale(float sx, float sy) override
		{
			for (auto& p : perimeter)
			{
				p.x *= sx;
				p.y *= sy;
			}
		}

		int32 PerimeterVertices() override
		{
			return (int32) perimeter.size();
		}

		void Orientate()
		{
			if (!isOriented)
			{
				auto& r = Ring<Vec2>(&perimeter[0], perimeter.size());
				if (!IsClockwiseSequential(r))
				{
					std::reverse(perimeter.begin(), perimeter.end());
				}

				isOriented = true;
			}
		}

		void GetRimQuad(float z0, float z1, int32 index, Quad& quad) override
		{
			if (index < 0) Throw(0, "RimTesselator::GetRimVertex(%d, ...) bad index", index);
			if (perimeter.empty()) Throw(0, "RimTesselator::GetRimVertex - perimenter empty");
			if (!isOriented) Orientate();

			int32 first = index % (int32)perimeter.size();
			int32 next = (index + 1) % (int32)perimeter.size();

			auto p = perimeter[first];
			auto q = perimeter[next];

			Quad raw;

			raw.a = { p.x, p.y, z1 };
			raw.b = { p.x, p.y, z0 };
			raw.c = { q.x, q.y, z0 };
			raw.d = { q.x, q.y, z1 };

			TransformPositions(&raw.a, 4, transform, &quad.a);
		}

		void GetRimVertex(int32 index, Vec2& p) override
		{
			if (index < 0) Throw(0, "RimTesselator::GetRimVertex(%d, ...) bad index", index);
			if (perimeter.empty()) Throw(0, "RimTesselator::GetRimVertex - perimenter empty");
			if (!isOriented) Orientate();
			p = perimeter[index % (int32) perimeter.size()];
		}

		int32 TesselateUniform()
		{
			if (!topFaces.empty()) Throw(0, "The Rim is currently tesselated. Use Clear to delete current faces");
			if (perimeter.empty()) Throw(0, "RimTesselator::TesselateUniform - perimeter empty");
			if (!isOriented) Orientate();

			if (perimeter[0] != perimeter.back())
			{
				Throw(0, "RimTesselator::TesselateUniform - perimeter is not a closed loop");
			}

			temp.clear();
			for (auto& p : perimeter)
			{
				temp.push_back(p);
			}

			RingManipulator<Vec2> ring(&temp[0], temp.size() - 1);

			struct : I2dMeshBuilder
			{
				RimTesselator* This;
				virtual void Append(const Triangle2d& t)
				{
					This->topFaces.push_back(t);
				}
			} builder;
			builder.This = this;
			TesselateByEarClip(builder, ring);

			return (int32) topFaces.size();
		}

		void GetBottomTriangle(int32 index, Triangle& pos, Triangle2d& uv, float z) override
		{
			if (index < 0 || index >= (int32) topFaces.size())
			{
				Throw(0, "RimTesselator::GetBottomTriangle -> Index %d out of bounds. Domain: 0 <= index < %d)", index, (int32)topFaces.size());
			}

			Triangle raw;

			auto& t = topFaces[index];
			raw.B = { t.A.x, t.A.y, z };
			raw.A = { t.B.x, t.B.y, z };
			raw.C = { t.C.x, t.C.y, z };

			TransformPositions(&raw.A, 3, transform, &pos.A);

			uv.B = t.A;
			uv.A = t.B;
			uv.C = t.C;
		}

		void GetTopTriangle(int32 index, Triangle& pos, Triangle2d& uv, float z) override
		{
			if (index < 0 || index >= (int32)topFaces.size())
			{
				Throw(0, "RimTesselator::GetTopTriangle -> Index %d out of bounds. Domain: 0 <= index < %d)", index, (int32)topFaces.size());
			}

			Triangle raw;

			auto& t = topFaces[index];
			raw.A = { t.A.x, t.A.y, z };
			raw.B = { t.B.x, t.B.y, z };
			raw.C = { t.C.x, t.C.y, z };

			TransformPositions(&raw.A, 3, transform, &pos.A);

			uv.A = t.A;
			uv.B = t.B;
			uv.C = t.C;
		}

		void SetTransform(const Matrix4x4& transform)
		{
			this->transform = transform;
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		IRimTesselatorSupervisor* CreateRimTesselator()
		{
			return new RimTesselator();
		}
	}
}