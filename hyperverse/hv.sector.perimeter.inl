namespace HV
{
	typedef std::vector<Vec2> TPerimeter;

	int32 GetPerimeterIndex(Vec2 a, const TPerimeter& perimeter)
	{
		for (int32 i = 0; i < perimeter.size(); ++i)
		{
			if (perimeter[i] == a)
			{
				return i;
			}
		}

		return -1;
	}

	bool IsAxisAlignedRectangular(const TPerimeter& floorPerimeter)
	{
		Ring<Vec2> ring(&floorPerimeter[0], floorPerimeter.size());

		for (size_t i = 0; i < floorPerimeter.size(); ++i)
		{
			auto p = ring[i];
			auto q = ring[i + 1];
			auto r = ring[i + 2];

			Vec2 pq = q - p;
			Vec2 qr = r - q;

			if (pq.x != 0 && pq.y != 0)
			{
				return false;
			}

			if (qr.x != 0 && qr.y != 0)
			{
				return false;
			}

			float x = Cross(pq, qr);
			if (x > 0)
			{
				return false;
			}
		}

		return true;
	}

	Segment GetSegment(Vec2 p, Vec2 q, const TPerimeter& perimeter)
	{
		if (!perimeter.empty())
		{
			int32 index_p = GetPerimeterIndex(p, perimeter);
			int32 index_q = GetPerimeterIndex(q, perimeter);

			if (index_p >= 0 && index_q >= 0)
			{
				if (((index_p + 1) % perimeter.size()) == index_q)
				{
					return{ index_p, index_q };
				}
				if (((index_q + 1) % perimeter.size()) == index_p)
				{
					return{ index_q, index_p };
				}
			}
		}
		else
		{
			// Decoupled from the co-sectors
		}

		return{ -1,-1 };
	}

	boolean32 TryGetAsRectangle(GuiRectf & rect, const TPerimeter& perimeter)
	{
		auto GetTangent = [](Vec2 a, Vec2 b)-> Vec2
		{
			Vec2 tangent = Normalize(b - a);
			tangent.x = roundf(tangent.x);
			tangent.y = roundf(tangent.y);
			return tangent;
		};

		int32 tangentIndex = 0;
		Vec2 tangents[4] = { {0,0}, {0,0}, {0,0}, {0,0} };
		Vec2 corners[4] = { {0,0}, {0,0}, {0,0}, {0,0} };

		rect = { -1,-1,-1,-1 };

		Ring<Vec2> vertices(perimeter.data(), perimeter.size());

		tangents[0] = GetTangent(vertices[0], vertices[1]);
		if (tangents[0].x != 0 && tangents[0].y != 0)
		{
			// Our rectangle must be axis-aligned, so one component of tangent
			// has to be zero
			return false;
		}

		size_t indexOfLastChange = 0;

		for (size_t i = 1; i < vertices.ElementCount(); ++i)
		{
			Vec2 start = vertices[i];
			Vec2 end = vertices[i + 1];
			Vec2 tangent = GetTangent(start, end);

			if (tangent.x != 0 && tangent.y != 0)
			{
				// Our rectangle must be axis-aligned, so one component of tangent
				// has to be zero
				return false;
			}

			if (tangents[tangentIndex] != tangent)
			{
				if (tangentIndex == 3)
				{
					return false; // Rectangles have only got 4 tangents
				}

				tangentIndex++;

				corners[tangentIndex] = start;
				tangents[tangentIndex] = tangent;

				indexOfLastChange = i;
			}
		}

		if (tangentIndex != 3)
		{
			return false;
		}

		Vec2 lastTangent = tangents[3];

		for (size_t i = indexOfLastChange; i < indexOfLastChange + vertices.ElementCount(); ++i)
		{
			Vec2 start = vertices[i];
			Vec2 end = vertices[i + 1];
			Vec2 tangent = GetTangent(start, end);

			if (lastTangent != tangent)
			{
				corners[0] = start;
				break;
			}
		}

		rect.left = min(corners[0].x, corners[2].x);
		rect.right = max(corners[0].x, corners[2].x);
		rect.top = max(corners[0].y, corners[2].y);
		rect.bottom = min(corners[0].y, corners[2].y);

		return true;
	}

	struct RayCrossingsEnum
	{
		int count = 0;
		bool fromVertex = false;
	};

	RayCrossingsEnum CountRayCrossingsThroughSector(Vec2 a, Vec2 dir, const TPerimeter& floorPerimeter)
	{
		Ring<Vec2> ring(&floorPerimeter[0], floorPerimeter.size());

		RayCrossingsEnum rce;

		if (GetPerimeterIndex(a, floorPerimeter) >= 0)
		{
			rce.fromVertex = true;
		}

		for (size_t i = 0; i < floorPerimeter.size(); ++i)
		{
			Vec2 p = ring[i];
			Vec2 q = ring[i + 1];

			float t, u;
			if (GetLineIntersect(a, a + dir, p, q, t, u))
			{
				if (u >= 0 && u <= 1)
				{
					if (t == 0 || u == 0 || u == 1)
					{
						rce.fromVertex = true;
					}
					else if (t > 0)
					{
						rce.count++;
					}
				}
			}
		}

		return rce;
	}

	bool DoesSegmentCrossPerimeter(Vec2 p, Vec2 q, const TPerimeter& floorPerimeter)
	{
		size_t nVertices = floorPerimeter.size();
		for (size_t i = 0; i <= nVertices; ++i)
		{
			auto c = GetRingElement(i, &floorPerimeter[0], nVertices);
			auto d = GetRingElement(i + 1, &floorPerimeter[0], nVertices);

			float t, u;
			if (GetLineIntersect(p, q, c, d, t, u))
			{
				if (u > 0 && u < 1 && t > 0 && t < 1)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool Is4PointRectangular(const TPerimeter& floorPerimeter)
	{
		if (floorPerimeter.size() != 4) return false;

		Vec2 ab = floorPerimeter[1] - floorPerimeter[0];
		Vec2 bc = floorPerimeter[2] - floorPerimeter[1];
		Vec2 cd = floorPerimeter[3] - floorPerimeter[2];
		Vec2 da = floorPerimeter[0] - floorPerimeter[3];

		if (Dot(ab, bc) != 0)
		{
			return false;
		}

		if (Dot(bc, cd) != 0)
		{
			return false;
		}

		if (Dot(cd, da) != 0)
		{
			return false;
		}

		if (Dot(da, ab) != 0)
		{
			return false;
		}

		return true;
	}
}
