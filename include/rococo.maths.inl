#pragma once

#include <rococo.maths.h>

namespace Rococo
{
	inline Vec3 GetPosition(const Matrix4x4& m)
	{
		return Vec3{ m.row0.w, m.row1.w, m.row2.w };
	}

	inline void SetPosition(Matrix4x4& m, const Vec3& pos)
	{
		m.row0.w = pos.x;
		m.row1.w = pos.y;
		m.row2.w = pos.z;
	}
}