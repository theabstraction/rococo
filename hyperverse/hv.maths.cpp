#include "hv.h"

namespace HV::HVMaths
{
	bool IsTriangleFacingUp(cr_m4x4 model, const VertexTriangle& t)
	{
		Vec3 up{ 0, 0, 1 };
		Vec3 Na, Nb, Nc;
		TransformDirection(model, t.a.normal, Na);
		TransformDirection(model, t.a.normal, Nb);
		TransformDirection(model, t.a.normal, Nc);
		return (Na == up && Nb == up && Nc == up);
	}

	void Expand(AABB2d& rect, Metres ds)
	{
		rect.left -= ds;
		rect.right += ds;
		rect.bottom -= ds;
		rect.top += ds;
	}

	bool TryGetOrientationAndBoundsToFitBoxInAnother(Matrix4x4& Rz, AABB& newBounds, const AABB& bounds, cr_vec2 containerSpan, Degrees theta)
	{
		Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);

		newBounds = bounds.RotateBounds(Rz);

		Vec3 newSpan = newBounds.Span();

		if (newSpan.x <= containerSpan.x && newSpan.y <= containerSpan.y)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool GetRandomOrientationAndBoundsToFitBoxInAnother(Matrix4x4& Rz, AABB& newBounds, const AABB& bounds, cr_vec2 containerSpan, int32 guesses)
	{
		for (int i = 0; i < guesses; ++i)
		{
			Degrees theta{ Roll::x(360) * 1.0f };
			return TryGetOrientationAndBoundsToFitBoxInAnother(Rz, newBounds, bounds, containerSpan, theta);
		}

		return false;
	}

	bool TryGetRotationToFit(Matrix4x4& Rz, bool randomizeHeading, const AABB& bounds, cr_vec2 containerSpan)
	{
		Vec3 objectSpan = bounds.Span();

		if (randomizeHeading)
		{
			AABB newBounds;
			return GetRandomOrientationAndBoundsToFitBoxInAnother(Rz, newBounds, bounds, containerSpan, 30);
		}
		else
		{
			int delta = Roll::x(4);
			for (int i = 0; i < 4; ++i)
			{
				Degrees angle{ fmodf(90.0f * (i + delta), 360.0f) };
				AABB newBounds;
				if (TryGetOrientationAndBoundsToFitBoxInAnother(Rz, newBounds, bounds, containerSpan, angle))
				{
					return true;
				}
			}

			return false;
		}
	}

	bool TryGetRandomTransformation(Matrix4x4& model, AABB& worldBounds, bool randomHeading, bool randomizePosition, const AABB& bounds, const AABB2d& container, float z0, float z1)
	{
		Vec2 containerSpan = container.Span();
		Vec3 objectSpan = bounds.Span();

		if (objectSpan.z > (z1 - z0))
		{
			return false;
		}

		Matrix4x4 Rz;
		if (!TryGetRotationToFit(Rz, randomHeading, bounds, containerSpan))
		{
			return false;
		}
		else
		{
			AABB newBounds = bounds.RotateBounds(Rz);
			Vec3 newSpan = newBounds.Span();

			float dx = containerSpan.x - newSpan.x;
			float dy = containerSpan.y - newSpan.y;

			float x0 = !randomizePosition ? dx * 0.5f : Roll::AnyOf(0, dx);
			float y0 = !randomizePosition ? dy * 0.5f : Roll::AnyOf(0, dy);

			Vec3 originDisplacement = Vec3{ x0, y0, 0 } - newBounds.minXYZ;
			Vec2 tileBottomLeft = { container.left, container.bottom };
			Vec3 position = Vec3{ tileBottomLeft.x, tileBottomLeft.y, z0 + 0.001f } + originDisplacement;

			auto T = Matrix4x4::Translate(position);

			model = T * Rz;

			Vec3 tileOrigin{ tileBottomLeft.x, tileBottomLeft.y, 0 };

			worldBounds = AABB();
			worldBounds << (tileOrigin + Vec3{ x0, y0, z0 });
			worldBounds << (tileOrigin + Vec3{ x0 + newSpan.x, y0 + newSpan.y, z0 + newSpan.z });

			return true;
		}
	}

	bool IsQuadRectangular(const Quad& q)
	{
		Vec3 ab = q.b - q.a;
		Vec3 bc = q.c - q.b;
		Vec3 cd = q.d - q.c;
		Vec3 da = q.a - q.d;

		if (Dot(ab, bc) == 0 && Dot(bc, cd) == 0 && Dot(cd, da) == 0 && Dot(da, ab) == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}