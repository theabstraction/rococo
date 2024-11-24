#include <rococo.mplat.h>
#include <vector>
#include <rococo.maths.h>

using namespace Rococo::Graphics;

namespace Rococo
{
	bool operator == (const RGBAb& a, const RGBAb& b)
	{
		return *(int*)&a == *(int*)&b;
	}

	bool operator == (const MaterialVertexData& a, const MaterialVertexData& b)
	{
		return a.colour == b.colour && a.gloss == b.gloss && a.materialId == b.materialId;
	}
}

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Graphics;

	struct QuadItem
	{
		QuadVertices q;
		MaterialVertexData mat;
	};

	Vec3 GetNormal(const QuadVertices& q)
	{
		return Normalize(Cross(q.positions.b - q.positions.a, q.positions.b - q.positions.c));
	}

	struct QuadStackTesselator : public IQuadStackTesselator
	{
		std::vector<QuadItem> input;
		std::vector<QuadItem> output;
		std::vector<QuadItem> temp;

		Matrix4x4 basis = Matrix4x4::Identity();

		QuadStackTesselator()
		{
		}

		void Clear() override
		{
			input.clear();
			output.clear();
		}

		void ClearInput() override
		{
			input.clear();

		}

		void ClearOutput() override
		{
			output.clear();
		}

		void CopyInputToOutput() override
		{
			for (auto& item : input)
			{
				output.push_back(item);
			}
		}

		void Destruct() override
		{
			delete this;
		}

		void MoveOutputToInput()
		{
			for (auto& item : output)
			{
				input.push_back(item);
			}

			output.clear();
		}

		void MoveOutputToInputWithMat(const MaterialVertexData& mat)
		{
			for (auto& item : output)
			{
				if (item.mat == mat && item.q.colours.a == mat.colour && item.q.colours.c == mat.colour)
				{
					input.push_back(item);
				}
				else
				{
					temp.push_back(item);
				}
			}

			output.clear();

			std::swap(temp, output);
		}

		void MoveOutputToInputWithNormalDotRange(const Vec3& modelNormal, float minDot, float maxDot)
		{
			Vec3 normal;
			TransformDirection(basis, modelNormal, normal);
			for (auto& item : output)
			{
				float d = Dot(item.q.normals.a, normal);
				if (d >= minDot && d <= maxDot)
				{
					input.push_back(item);
				}
				else
				{
					temp.push_back(item);
				}
			}

			output.clear();

			std::swap(temp, output);
		}

		void MoveInputToOutputWithNormalDotRange(const Vec3& modelNormal, float minDot, float maxDot)
		{
			Vec3 normal;
			TransformDirection(basis, modelNormal, normal);
			for (auto& item : input)
			{
				float d = Dot(item.q.normals.a, normal);
				if (d >= minDot && d <= maxDot)
				{
					output.push_back(item);
				}
				else
				{
					temp.push_back(item);
				}
			}

			input.clear();

			std::swap(temp, input);
		}

		void Translate(const Vec3& v)
		{
			for (auto& item : input)
			{
				item.q.positions.a += v;
				item.q.positions.b += v;
				item.q.positions.c += v;
				item.q.positions.d += v;
			}
		}

		enum FaceFilter
		{
			FaceFilter_None = 0,
			FaceFilter_Left = 1,
			FaceFilter_Right = 2,
			FaceFilter_North = 4,
			FaceFilter_South = 8,
			FaceFilter_Top = 16,
			FaceFilter_Bottom = 32
		};

		struct BoxVertices
		{
			Vec3 bottomLeftNorth;
			Vec3 topLeftNorth;
			Vec3 bottomLeftSouth;
			Vec3 topLeftSouth;
			Vec3 bottomRightNorth;
			Vec3 topRightNorth;
			Vec3 bottomRightSouth;
			Vec3 topRightSouth;

			void GetTop(Quad& q) const
			{
				q.a = topLeftNorth;
				q.b = topRightNorth;
				q.c = topRightSouth;
				q.d = topLeftSouth;
			}

			void GetBottom(Quad& q) const
			{
				q.a = bottomLeftSouth;
				q.b = bottomRightSouth;
				q.c = bottomRightNorth;
				q.d = bottomLeftNorth;
			}

			void GetLeft(Quad& q) const
			{
				q.a = topLeftNorth;
				q.b = topLeftSouth;
				q.c = bottomLeftSouth;
				q.d = bottomLeftNorth;
			}

			void GetRight(Quad& q) const
			{
				q.a = topRightSouth;
				q.b = topRightNorth;
				q.c = bottomRightNorth;
				q.d = bottomRightSouth;
			}

			void GetSouth(Quad& q) const
			{
				q.a = topLeftSouth;
				q.b = topRightSouth;
				q.c = bottomRightSouth;
				q.d = bottomLeftSouth;
			}

			void GetNorth(Quad& q) const
			{
				q.a = topRightNorth;
				q.b = topLeftNorth;
				q.c = bottomLeftNorth;
				q.d = bottomRightNorth;
			}
		};

		void NormalizeQuad(QuadVertices& v)
		{
			Vec3 n = GetNormal(v);
			v.normals.a = v.normals.b = v.normals.c = v.normals.d = n;
		}

		Vec2 GenUVsFromVertices(QuadVertices& v, float uvScale, Vec2 origin)
		{
			Vec3 tangent = v.positions.b - v.positions.a;
			Vec3 vert = v.positions.a - v.positions.d;

			Vec2 span = uvScale * Vec2{ Length(tangent), Length(vert) };

			v.uv.left = origin.x;
			v.uv.top = origin.y;
			v.uv.right = origin.x + span.x;
			v.uv.bottom = origin.y + span.y;

			return span;
		}

		void MakeUniformColour(QuadVertices& v, RGBAb colour)
		{
			v.colours.a = v.colours.b = v.colours.c = v.colours.d = colour;
		}

		void AddBox(uint32 filterFlags, const BoxVertices& box, float uvScale, const MaterialVertexData& rodMat)
		{
			Vec2 origin{ 0,0 };
			if ((filterFlags & FaceFilter_Top) != 0)
			{
				QuadItem top;
				top.mat = rodMat;
				MakeUniformColour(top.q, rodMat.colour);
				box.GetTop(top.q.positions);
				NormalizeQuad(top.q);
				auto uvSpan = GenUVsFromVertices(top.q, uvScale, origin);
				origin.y += uvSpan.y;
				output.push_back(top);
			}

			if ((filterFlags & FaceFilter_North) != 0)
			{
				QuadItem north;
				north.mat = rodMat;
				MakeUniformColour(north.q, rodMat.colour);
				box.GetNorth(north.q.positions);
				NormalizeQuad(north.q);
				auto uvSpan = GenUVsFromVertices(north.q, uvScale, origin);
				origin.y += uvSpan.y;
				output.push_back(north);
			}

			if ((filterFlags & FaceFilter_Bottom) != 0)
			{
				QuadItem bottom;
				bottom.mat = rodMat;
				MakeUniformColour(bottom.q, rodMat.colour);
				box.GetBottom(bottom.q.positions);
				NormalizeQuad(bottom.q);
				auto uvSpan = GenUVsFromVertices(bottom.q, uvScale, origin);
				origin.y += uvSpan.y;
				output.push_back(bottom);
			}

			if ((filterFlags & FaceFilter_South) != 0)
			{
				QuadItem south;
				south.mat = rodMat;
				MakeUniformColour(south.q, rodMat.colour);
				box.GetSouth(south.q.positions);
				NormalizeQuad(south.q);
				auto uvSpan = GenUVsFromVertices(south.q, uvScale, origin);
				origin.y += uvSpan.y;
				output.push_back(south);
			}

			if ((filterFlags & FaceFilter_Left) != 0)
			{
				QuadItem left;
				left.mat = rodMat;
				MakeUniformColour(left.q, rodMat.colour);
				box.GetLeft(left.q.positions);
				NormalizeQuad(left.q);
				auto uvSpan = GenUVsFromVertices(left.q, uvScale, origin);
				origin.y += uvSpan.y;
				output.push_back(left);
			}

			if ((filterFlags & FaceFilter_Right) != 0)
			{
				QuadItem right;
				right.mat = rodMat;
				MakeUniformColour(right.q, rodMat.colour);
				box.GetRight(right.q.positions);
				NormalizeQuad(right.q);
				auto uvSpan = GenUVsFromVertices(right.q, uvScale, origin);
				origin.y += uvSpan.y;
				output.push_back(right);
			}
		}

		void AddCuboid(float v0, float v1,  float t0, float t1, float thickness, float uvScale,const MaterialVertexData& rodMat) override
		{
			for (auto& item : input)
			{
				Vec3 dN = thickness * GetNormal(item.q);

				BoxVertices box1;

				box1.bottomLeftNorth = Lerp(item.q.positions.d, item.q.positions.a, v1);
				box1.topLeftNorth = box1.bottomLeftNorth + dN;

				box1.bottomLeftSouth = Lerp(item.q.positions.d, item.q.positions.a, v0);
				box1.topLeftSouth = box1.bottomLeftSouth + dN;

				box1.bottomRightNorth = Lerp(item.q.positions.c, item.q.positions.b, v1);
				box1.topRightNorth = box1.bottomRightNorth + dN;

				box1.bottomRightSouth = Lerp(item.q.positions.c, item.q.positions.b, v0);
				box1.topRightSouth = box1.bottomRightSouth + dN;

				BoxVertices box2;

				box2.topLeftNorth = Lerp(box1.topLeftNorth, box1.topRightNorth, t0);
				box2.topRightNorth = Lerp(box1.topLeftNorth, box1.topRightNorth, t1);
				box2.bottomLeftNorth = Lerp(box1.bottomLeftNorth, box1.bottomRightNorth, t0);
				box2.bottomRightNorth = Lerp(box1.bottomLeftNorth, box1.bottomRightNorth, t1);
				box2.topLeftSouth = Lerp(box1.topLeftSouth, box1.topRightSouth, t0);
				box2.topRightSouth = Lerp(box1.topLeftSouth, box1.topRightSouth, t1);
				box2.bottomLeftSouth = Lerp(box1.bottomLeftSouth, box1.bottomRightSouth, t0);
				box2.bottomRightSouth = Lerp(box1.bottomLeftSouth, box1.bottomRightSouth, t1);

				AddBox(FaceFilter_North | FaceFilter_South | FaceFilter_Top | FaceFilter_Bottom | FaceFilter_Left | FaceFilter_Right, box2, uvScale, rodMat);
			}
		}

		void AddCuboidAbs(Metres dx0, Metres dy0, Metres dx1, Metres dy1, Metres thickness, float uvScale, const MaterialVertexData& rodMat) override
		{
			if (dx0 >= dx1)
			{
				Throw(0, "AddCuboidAbs: dx0 must be < dx1. %f > %f", dx0, dx1);
			}

			if (dy0 <= dy1)
			{
				Throw(0, "AddCuboidAbs: dy0 must be > dy1. %f > %f", dy0, dy1);
			}

			if (thickness < 0)
			{
				Throw(0, "AddCuboidAbs: thickness must be > 0: %f", thickness);
			}

			uint32 flags;

			if (thickness < 0.001)
			{
				thickness = 0.001_metres;
				flags = FaceFilter_Top | FaceFilter_Bottom;
			}
			else
			{
				flags = FaceFilter_North | FaceFilter_South | FaceFilter_Top | FaceFilter_Bottom | FaceFilter_Left | FaceFilter_Right;
			}

			for (auto& item : input)
			{
				Vec3 dN = thickness * GetNormal(item.q);

				BoxVertices box;

				Vec3 centre = 0.5f * (item.q.positions.a + item.q.positions.c);
				Vec3 tangent = Normalize(item.q.positions.b - item.q.positions.a);
				Vec3 vertical = Normalize(item.q.positions.a - item.q.positions.d);

				box.bottomLeftNorth = centre + tangent * dx0 + vertical * dy0;
				box.topLeftNorth    = centre + tangent * dx0 + vertical * dy0 + dN;

				box.bottomLeftSouth = centre + tangent * dx0 + vertical * dy1;
				box.topLeftSouth    = centre + tangent * dx0 + vertical * dy1 + dN;

				box.bottomRightNorth = centre + tangent * dx1 + vertical * dy0;
				box.topRightNorth    = centre + tangent * dx1 + vertical * dy0 + dN;

				box.bottomRightSouth = centre + tangent * dx1 + vertical * dy1;
				box.topRightSouth    = centre + tangent * dx1 + vertical * dy1 + dN;

				AddBox(flags, box, uvScale, rodMat);
			}
		}

		void Intrude(const GuiRectf& window, float depth, float depthUVscale, const MaterialVertexData& rimMat, const MaterialVertexData& innerMat) override
		{
			// See diagram qs.intrude.png
			for (const auto& item : input)
			{
				QuadItem topLeft;
				topLeft.mat = item.mat;
				topLeft.q = item.q;
				topLeft.q.positions.b = Lerp(item.q.positions.a, item.q.positions.b, window.right);
				topLeft.q.normals.b = Lerp(item.q.normals.a, item.q.normals.b, window.right);
				topLeft.q.uv.right = Lerp(item.q.uv.left, item.q.uv.right, window.right);
				topLeft.q.uv.bottom = Lerp(item.q.uv.bottom, item.q.uv.top, window.top);

				topLeft.q.positions.d = Lerp(item.q.positions.d, item.q.positions.a, window.top);
				topLeft.q.normals.d = Lerp(item.q.normals.d, item.q.normals.a, window.top);

				topLeft.q.positions.c = topLeft.q.positions.d + (topLeft.q.positions.b - topLeft.q.positions.a);
				MakeUniformColour(topLeft.q, item.mat.colour);
				output.push_back(topLeft);

				QuadItem topRight;
				topRight.mat = item.mat;
				topRight.q = item.q;
				topRight.q.positions.a = topLeft.q.positions.b;
				topRight.q.normals.a = topLeft.q.normals.b;
				topRight.q.uv.left = topLeft.q.uv.right;
				topRight.q.uv.bottom = Lerp(item.q.uv.bottom, item.q.uv.top, window.bottom);
				topRight.q.positions.b = item.q.positions.b;
				topRight.q.positions.c = Lerp(item.q.positions.c, item.q.positions.b, window.bottom);
				topRight.q.positions.d = topRight.q.positions.c + (item.q.positions.d - item.q.positions.c) * (1.0f - window.right);
				MakeUniformColour(topRight.q, item.mat.colour);
				output.push_back(topRight);

				QuadItem bottomRight;
				bottomRight.mat = item.mat;
				bottomRight.q = item.q;
				bottomRight.q.positions.d = Lerp(item.q.positions.d, item.q.positions.c, window.left);
				bottomRight.q.positions.b = topRight.q.positions.c;
				bottomRight.q.positions.a = bottomRight.q.positions.d + (bottomRight.q.positions.b - bottomRight.q.positions.c);
				bottomRight.q.uv.left = Lerp(item.q.uv.left, item.q.uv.right, window.left);
				bottomRight.q.uv.top = Lerp(item.q.uv.bottom, item.q.uv.top, window.bottom);
				MakeUniformColour(bottomRight.q, item.mat.colour);
				output.push_back(bottomRight);

				QuadItem bottomLeft;
				bottomLeft.q = item.q;
				bottomLeft.mat = item.mat;
				bottomLeft.q.positions.a = topLeft.q.positions.d;
				bottomLeft.q.positions.b = bottomLeft.q.positions.a + window.left * (item.q.positions.b - item.q.positions.a);
				bottomLeft.q.positions.c = bottomRight.q.positions.d;
				bottomLeft.q.uv.right = Lerp(item.q.uv.left, item.q.uv.right, window.left);
				bottomLeft.q.uv.top = Lerp(item.q.uv.bottom, item.q.uv.top, window.top);
				MakeUniformColour(bottomLeft.q, item.mat.colour);
				output.push_back(bottomLeft);

				Vec3 normal = Normalize(item.q.normals.a);
				Vec3 dN = normal * depth;

				if (depth > 0)
				{
					QuadItem innerLeft;
					innerLeft.q = item.q;
					innerLeft.mat = rimMat;

					innerLeft.q.positions.d = bottomLeft.q.positions.b;
					innerLeft.q.positions.a = innerLeft.q.positions.d - dN;
					innerLeft.q.positions.c = bottomRight.q.positions.a;
					innerLeft.q.positions.b = innerLeft.q.positions.c - dN;

					innerLeft.q.uv.bottom = bottomLeft.q.uv.right;
					innerLeft.q.uv.right = bottomRight.q.uv.top;
					innerLeft.q.uv.top = innerLeft.q.uv.bottom + depth * depthUVscale;
					innerLeft.q.uv.left = topLeft.q.uv.bottom;

					Vec3 innerLNorm = GetNormal(innerLeft.q);

					innerLeft.q.normals.a = innerLeft.q.normals.b = innerLeft.q.normals.c = innerLeft.q.normals.d = innerLNorm;
					MakeUniformColour(innerLeft.q, rimMat.colour);
					output.push_back(innerLeft);

					QuadItem innerRight;
					innerRight.q = item.q;
					innerRight.mat = rimMat;

					innerRight.q.positions.c = topLeft.q.positions.c;
					innerRight.q.positions.d = topRight.q.positions.d;
					innerRight.q.positions.a = innerRight.q.positions.d - dN;
					innerRight.q.positions.b = innerRight.q.positions.c - dN;

					innerRight.q.uv.right = bottomRight.q.uv.top;
					innerRight.q.uv.left = topLeft.q.uv.bottom;
					innerRight.q.uv.top = topRight.q.uv.left;
					innerRight.q.uv.bottom = topRight.q.uv.left - depth * depthUVscale;

					innerRight.q.normals.a = innerRight.q.normals.b = innerRight.q.normals.c = innerRight.q.normals.d = -innerLNorm;
					MakeUniformColour(innerRight.q, rimMat.colour);
					output.push_back(innerRight);

					QuadItem bottom;
					bottom.q = item.q;
					bottom.mat = rimMat;

					bottom.q.positions.c = topRight.q.positions.d;
					bottom.q.positions.d = bottomRight.q.positions.a;
					bottom.q.positions.a = bottom.q.positions.d - dN;
					bottom.q.positions.b = bottom.q.positions.c - dN;

					bottom.q.uv.right = topRight.q.uv.left;
					bottom.q.uv.left = bottomLeft.q.uv.right;
					bottom.q.uv.top = bottomRight.q.uv.top - depth * depthUVscale;
					bottom.q.uv.bottom = bottomRight.q.uv.top ;

					Vec3 bottomNormal = GetNormal(bottom.q);

					bottom.q.normals.a = bottom.q.normals.b = bottom.q.normals.c = bottom.q.normals.d = bottomNormal;
					MakeUniformColour(bottom.q, rimMat.colour);
					output.push_back(bottom);

					QuadItem top;
					top.q = item.q;
					top.mat = rimMat;

					top.q.positions.c = bottomLeft.q.positions.b;
					top.q.positions.d = topLeft.q.positions.c;
					top.q.positions.a = top.q.positions.d - dN;
					top.q.positions.b = top.q.positions.c - dN;

					top.q.uv.right = topRight.q.uv.left;
					top.q.uv.left = bottomLeft.q.uv.right;
					top.q.uv.bottom = topLeft.q.uv.bottom;
					top.q.uv.top = top.q.uv.top + depth * depthUVscale;

					top.q.normals.a = top.q.normals.b = top.q.normals.c = top.q.normals.d = -bottomNormal;
					MakeUniformColour(top.q, rimMat.colour);
					output.push_back(top);
				}

				QuadItem inner;
				inner.q = item.q;
				inner.mat = innerMat;

				inner.q.positions.a = bottomLeft.q.positions.b - dN;
				inner.q.positions.b = topLeft.q.positions.c - dN;
				inner.q.positions.c = topRight.q.positions.d - dN;
				inner.q.positions.d = bottomRight.q.positions.a - dN;

				inner.q.uv.left = bottomLeft.q.uv.right;
				inner.q.uv.right = topRight.q.uv.left;
				inner.q.uv.top = topLeft.q.uv.bottom;
				inner.q.uv.bottom = bottomRight.q.uv.top;

				Vec3 innerLNorm = GetNormal(inner.q);

				inner.q.normals.a = inner.q.normals.b = inner.q.normals.c = inner.q.normals.d = normal;
				MakeUniformColour(inner.q, innerMat.colour);
				output.push_back(inner);
			}

			input.clear();
		}

		void ScaleEdges(float left, float right, float low, float high, boolean32 preserveUVs) override
		{
			for (auto& item : input)
			{
				Vec3 tangent = item.q.positions.b - item.q.positions.a;
				Vec3 vertical = item.q.positions.a - item.q.positions.d;

				Quad scaledQuad;
				scaledQuad.a = item.q.positions.a + tangent * left + vertical * high;
				scaledQuad.b = item.q.positions.b + tangent * right + vertical * high;
				scaledQuad.c = item.q.positions.c + tangent * right + vertical * low;
				scaledQuad.d = item.q.positions.d + tangent * left + vertical * low;

				item.q.positions = scaledQuad;

				if (!preserveUVs)
				{
					GuiRectf scaledUv;
					Vec2 uvSpan = Span(item.q.uv);
					
					scaledUv.left = item.q.uv.left + uvSpan.x * left;
					scaledUv.right = item.q.uv.right + uvSpan.x * right;
					scaledUv.top = item.q.uv.top + uvSpan.y * high;
					scaledUv.bottom = item.q.uv.bottom + uvSpan.y * low;

					item.q.uv = scaledUv;
				}

				output.push_back(item);
			}

			input.clear();
		}

		virtual void SplitThreeColumns(const MaterialVertexData& c1, const MaterialVertexData& c2, const MaterialVertexData& c3, float x0, float x1)
		{
			for (auto& item : input)
			{
				QuadItem left;
				QuadItem middle;
				QuadItem right;

				left.mat = c1;
				middle.mat = c2;
				right.mat = c3;

				left.q = middle.q = right.q = item.q;

				left.q.colours.c = left.q.colours.b = c2.colour;
				right.q.colours.a = right.q.colours.d = c2.colour;

				left.q.colours.a = left.q.colours.d = c1.colour;
				right.q.colours.b = right.q.colours.c = c3.colour;

				middle.q.colours.a = middle.q.colours.b = middle.q.colours.c = middle.q.colours.d = c2.colour;

				left.q.positions.b = Lerp(item.q.positions.a, item.q.positions.b, x0);
				left.q.positions.c = Lerp(item.q.positions.d, item.q.positions.c, x0);

				right.q.positions.a = Lerp(item.q.positions.a, item.q.positions.b, x1);
				right.q.positions.d = Lerp(item.q.positions.d, item.q.positions.c, x1);

				middle.q.positions.a = left.q.positions.b;
				middle.q.positions.b = right.q.positions.a;
				middle.q.positions.d = left.q.positions.c;
				middle.q.positions.c = right.q.positions.d;

				left.q.uv.right = Lerp(item.q.uv.left, item.q.uv.right, x0);
				middle.q.uv.left = left.q.uv.right;
				right.q.uv.left = Lerp(item.q.uv.left, item.q.uv.right, x1);
				middle.q.uv.right = right.q.uv.left;

				output.push_back(left);
				output.push_back(right);
				output.push_back(middle);
			}

			input.clear();
		}

		virtual void SplitThreeRows(const MaterialVertexData& r1, const MaterialVertexData& r2, const MaterialVertexData& r3, float y0, float y1)
		{
			for (auto& item : input)
			{
				QuadItem bottom;
				QuadItem middle;
				QuadItem top;

				bottom.mat = r1;
				middle.mat = r2;
				top.mat = r3;

				top.q = middle.q = bottom.q = item.q;

				bottom.q.colours.a = bottom.q.colours.b = r2.colour;
				top.q.colours.c = top.q.colours.d = r2.colour;

				bottom.q.colours.c = bottom.q.colours.d = r1.colour;
				top.q.colours.a = top.q.colours.b = r3.colour;

				middle.q.colours.a = middle.q.colours.b = middle.q.colours.c = middle.q.colours.d = r2.colour;

				bottom.q.positions.a = Lerp(item.q.positions.d, item.q.positions.a, y0);
				bottom.q.positions.b = Lerp(item.q.positions.c, item.q.positions.b, y0);

				top.q.positions.d = Lerp(item.q.positions.d, item.q.positions.a, y1);
				top.q.positions.c = Lerp(item.q.positions.c, item.q.positions.b, y1);

				middle.q.positions.a = top.q.positions.d;
				middle.q.positions.b = top.q.positions.c;
				middle.q.positions.d = bottom.q.positions.a;
				middle.q.positions.c = bottom.q.positions.b;

				output.push_back(bottom);
				output.push_back(middle);
				output.push_back(top);

				top.q.uv.bottom = Lerp(item.q.uv.bottom, item.q.uv.top, y1);
				middle.q.uv.top = top.q.uv.bottom;
				bottom.q.uv.top = Lerp(item.q.uv.bottom, item.q.uv.top, y0);
				middle.q.uv.bottom = bottom.q.uv.top;
			}

			input.clear();
		}

		void SplitAcrossTangent(float v, RGBAb topColour, RGBAb middleColour, RGBAb lowColour, const MaterialVertexData& topMat, const MaterialVertexData& bottomMat)
		{
			for (auto& item : input)
			{
				QuadItem top;
				QuadItem bottom;

				top.mat = topMat;
				bottom.mat = bottomMat;

				top.q.colours.a = top.q.colours.b = topColour;
				top.q.colours.c = top.q.colours.d = middleColour;

				top.q.normals = item.q.normals;

				top.q.uv = item.q.uv;
				top.q.uv.bottom = Lerp(item.q.uv.bottom, item.q.uv.top, v);

				top.q.positions.a = item.q.positions.a;
				top.q.positions.b = item.q.positions.b;
				top.q.positions.c = Lerp(item.q.positions.c, item.q.positions.b, v);
				top.q.positions.d = Lerp(item.q.positions.d, item.q.positions.a, v);

				bottom.q.colours.a = bottom.q.colours.b = middleColour;
				bottom.q.colours.c = bottom.q.colours.d = lowColour;

				bottom.q.normals = item.q.normals;

				bottom.q.uv = item.q.uv;
				bottom.q.uv.top = top.q.uv.bottom;

				bottom.q.positions.a = top.q.positions.d;
				bottom.q.positions.b = top.q.positions.c;
				bottom.q.positions.c = item.q.positions.c;
				bottom.q.positions.d = item.q.positions.d;

				output.push_back(top);
				output.push_back(bottom);
			}

			input.clear();
		}

		void MoveInputToOutput()
		{
			for (auto& i : input)
			{
				output.push_back(i);
			}

			input.clear();
		}

		void PushQuad(const QuadVertices& quad, const MaterialVertexData& material) override
		{
			input.push_back({ quad,material });
		}

		boolean32 PopOutputAsTriangles(VertexTriangle& topRight, VertexTriangle& bottomLeft) override
		{
			if (output.empty())	return false;

			const auto& item = output.back();

			topRight.a.position = item.q.positions.a;
			topRight.a.normal = item.q.normals.a;
			topRight.a.uv = { item.q.uv.left, item.q.uv.top };
			topRight.a.material = item.mat;
			topRight.a.material.colour = item.q.colours.a;

			topRight.b.position = item.q.positions.b;
			topRight.b.normal = item.q.normals.b;
			topRight.b.uv = { item.q.uv.right, item.q.uv.top };
			topRight.b.material = item.mat;
			topRight.b.material.colour = item.q.colours.b;

			topRight.c.position = item.q.positions.c;
			topRight.c.normal = item.q.normals.c;
			topRight.c.uv = { item.q.uv.right, item.q.uv.bottom };
			topRight.c.material = item.mat;
			topRight.c.material.colour = item.q.colours.c;

			bottomLeft.a.position = item.q.positions.c;
			bottomLeft.a.normal = item.q.normals.c;
			bottomLeft.a.uv = { item.q.uv.right, item.q.uv.bottom };
			bottomLeft.a.material = item.mat;
			bottomLeft.a.material.colour = item.q.colours.c;

			bottomLeft.b.position = item.q.positions.d;
			bottomLeft.b.normal = item.q.normals.d;
			bottomLeft.b.uv = { item.q.uv.left, item.q.uv.bottom };
			bottomLeft.b.material = item.mat;
			bottomLeft.b.material.colour = item.q.colours.d;
			bottomLeft.c = topRight.a;

			output.pop_back();

			return true;
		}

		void SetBasis(cr_vec3 tangent, cr_vec3 normal, cr_vec3 vertical) override
		{
			basis =
				Matrix4x4
				{
					{ tangent.x, normal.x, vertical.x, 0.0f },
					{ tangent.y, normal.y, vertical.y, 0.0f },
					{ tangent.z, normal.z, vertical.z, 0.0f },
					{      0.0f,     0.0f,       0.0f, 1.0f }
				};
		}

		void SetMaterial(const MaterialVertexData& mat)  override
		{
			for (auto& item : input)
			{
				item.mat = mat;
			}
		}

		void SetTextureRect(const GuiRectf& rect) override
		{
			for (auto& item : input)
			{
				item.q.uv = rect;
			}
		}

		void Shrink(const GuiRectf& rect)
		{
			for (auto& item : input)
			{
				Vec3 tangent = item.q.positions.b - item.q.positions.a;
				Vec3 vertical = item.q.positions.a - item.q.positions.d;

				Quad innerQuad;
				innerQuad.a = rect.left * tangent + rect.top * vertical + item.q.positions.d;
				innerQuad.b = rect.right * tangent + rect.top * vertical + item.q.positions.d;
				innerQuad.c = rect.right * tangent + rect.bottom * vertical + item.q.positions.d;
				innerQuad.d = rect.left * tangent + rect.bottom * vertical + item.q.positions.d;

				item.q.positions = innerQuad;

				GuiRectf uv;
				uv.left = Lerp(item.q.uv.left, item.q.uv.right, rect.left);
				uv.right = Lerp(item.q.uv.left, item.q.uv.right, rect.right);
				uv.top = Lerp(item.q.uv.bottom, item.q.uv.top, rect.top);
				uv.bottom = Lerp(item.q.uv.bottom, item.q.uv.top, rect.bottom);

				item.q.uv = uv;
			}
		}

		void TileMosaic(const MaterialVertexData& a, const MaterialVertexData& b, const GuiRectf& uvRect, Metres roughSize) override
		{
			for (auto& item : input)
			{
				Vec3 tangent = item.q.positions.b - item.q.positions.a;
				Vec3 vertical = item.q.positions.a - item.q.positions.d;

				Vec2 span{ Length(tangent), Length(vertical) };

				int DX = min(max((int)(span.x / roughSize), 1), 100);
				int DY = min(max((int)(span.y / roughSize), 1), 100);

				Vec3 bottomLeft = item.q.positions.d;

				Vec3 DT = tangent * (1.0f /DX );
				Vec3 DV = vertical * (1.0f / DY);

				for (int j = 0; j < DY; ++j)
				{
					for (int i = 0; i < DX; ++i)
					{
						QuadItem subItem;
						subItem.mat = ((i + j) % 2) == 0 ? a : b;
						subItem.q.normals = item.q.normals;
						subItem.q.uv = uvRect;

						Vec3 DI0 = DT * (float)i;
						Vec3 DI1 = DT * (float)(i + 1.0f);
						Vec3 DJ0 = DV * (float)j;
						Vec3 DJ1 = DV * (float)(j + 1.0f);

						subItem.q.positions.a = bottomLeft + DI0 + DJ1;
						subItem.q.positions.b = bottomLeft + DI1 + DJ1;
						subItem.q.positions.c = bottomLeft + DI1 + DJ0;
						subItem.q.positions.d = bottomLeft + DI0 + DJ0;

						subItem.q.colours.a = subItem.q.colours.b = subItem.q.colours.c = subItem.q.colours.d = subItem.mat.colour;
						output.push_back(subItem);
					}
				}
			}

			input.clear();
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		IQuadStackTesselator* CreateQuadStackTesselator()
		{
			return new ANON::QuadStackTesselator();
		}
	}
}