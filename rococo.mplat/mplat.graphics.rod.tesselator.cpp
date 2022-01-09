#include <rococo.mplat.h>
#include <vector>
#include <rococo.rings.inl>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	struct ObjectQuad
	{
		ObjectVertex a;
		ObjectVertex b;
		ObjectVertex c;
		ObjectVertex d;
	};

	struct RodTesselator : public IRodTesselatorSupervisor
	{
		IMeshBuilder& meshes;
		std::vector<Vec2> vertices;
		std::vector<ObjectVertex> tempBuffer0;
		std::vector<ObjectVertex> tempBuffer1;
		std::vector<ObjectVertex> tempBuffer2;

		bool smoothNormals = true;
		MaterialVertexData top { RGBAb(255, 255, 255), 0, 0 };
		MaterialVertexData middle { RGBAb(128, 128, 128), 0, 0.1f };
		MaterialVertexData bottom{ RGBAb(0, 0, 0), 0, 0.2f };
		float uvScale = 1.0f;

		std::vector<VertexTriangle>  triangles;

		Vec3 direction{ 0, 0, 1 };
		Vec3 bitangent{ 1, 0, 0 };
		Vec3 origin { 0, 0, 0 };

		void AddQuad(const ObjectQuad& q)
		{
			triangles.push_back({ q.a, q.b, q.c });
			triangles.push_back({ q.c, q.d, q.a });
		}

		RodTesselator(IMeshBuilder& _meshes): meshes(_meshes)
		{

		}

		struct BlendList
		{
			int32 boneIndex;
			std::vector<float> weights;
		};

		std::vector<BlendList> blendLists;

		void SetBlendWeightByHeight(int32 boneIndex, float lowerValue, float upperValue) override
		{
			float minZValue = std::numeric_limits<float>::max();
			float maxZValue = -std::numeric_limits<float>::max();

			for (auto& t : triangles)
			{
				minZValue = min(t.a.position.z, minZValue);
				maxZValue = max(t.a.position.z, maxZValue);
				minZValue = min(t.b.position.z, minZValue);
				maxZValue = max(t.b.position.z, maxZValue);
				minZValue = min(t.c.position.z, minZValue);
				maxZValue = max(t.c.position.z, maxZValue);
			}

			float Zspan = maxZValue - minZValue;

			if (!Zspan)
			{
				Throw(0, "The Z span was zero");
			}

			BlendList b;
			b.boneIndex = boneIndex;
			b.weights.reserve(triangles.size() * 3);

			for (auto& t : triangles)
			{
				float qA = (t.a.position.z - minZValue) / Zspan;
				float weightA = (qA - lowerValue) * (upperValue - lowerValue);
				b.weights.push_back(weightA);

				float qB = (t.b.position.z - minZValue) / Zspan;
				float weightB = (qB - lowerValue) * (upperValue - lowerValue);
				b.weights.push_back(weightB);

				float qC = (t.c.position.z - minZValue) / Zspan;
				float weightC = (qC - lowerValue) * (upperValue - lowerValue);
				b.weights.push_back(weightC);
			}

			blendLists.push_back(b);
		}

		VertexTriangle* begin() override
		{
			return triangles.empty() ? nullptr : triangles.data();
		}

		VertexTriangle* end() override
		{
			return triangles.empty() ? nullptr : triangles.data() + triangles.size();
		}

		void Free() override
		{
			delete this;
		}

		Vec3 Transform(cr_vec3 v)
		{
			return { v.x + origin.x, v.y + origin.y, v.z + origin.z };
		}

		void AddBox(Metres length, const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d) override
		{
			float z0 = 0;
			float z1 = length;
 
			Vec3 usw = Transform({ d.x, d.y, z1 });
			Vec3 use = Transform({ c.x, c.y, z1 });
			Vec3 unw = Transform({ a.x, a.y, z1 });
			Vec3 une = Transform({ b.x, b.y, z1 });
			Vec3 lse = Transform({ c.x, c.y, z0 });
			Vec3 lsw = Transform({ d.x, d.y, z0 });
			Vec3 lne = Transform({ b.x, b.y, z0 });
			Vec3 lnw = Transform({ a.x, a.y, z0 });

			float vB = 0;
			float vT = length * uvScale;
			float uSW = 0;
			float uSE = Length(lse - lsw) * uvScale;

			MaterialVertexData topMat = { top.colour, middle.materialId, top.gloss };
			MaterialVertexData botMat = { bottom.colour, middle.materialId, bottom.gloss };

			Vec3 Ns{ 0,-1, 0 };

			ObjectQuad south
			{
				{usw,Ns,{uSW, vT }, topMat },
				{use,Ns,{uSE, vT }, topMat },
				{lse,Ns,{uSE, vB }, botMat },
				{lsw,Ns,{uSW, vB }, botMat },
			};

			AddQuad(south);

			float uNE = uSE + Length(une - use) * uvScale;

			Vec3 Ne{ 1, 0, 0 };
			ObjectQuad east
			{
				{ use,Ne,{ uSE, vT }, topMat },
				{ une,Ne,{ uNE, vT }, topMat },
				{ lne,Ne,{ uNE, vB }, botMat },
				{ lse,Ne,{ uSE, vB }, botMat },
			};
			AddQuad(east);

			Vec3 Nn{ 0,1, 0 };

			float uNW = uNE + Length(une - unw) * uvScale;

			ObjectQuad north
			{
				{ une,Nn,{ uNE, vT }, topMat },
				{ unw,Nn,{ uNW, vT }, topMat },
				{ lnw,Nn,{ uNW, vB }, botMat },
				{ lne,Nn,{ uNE, vB }, botMat },
			};
			AddQuad(north);

			float uE = uNW + Length(une - use) * uvScale;

			Vec3 Nw{ -1, 0, 0 };

			ObjectQuad west
			{ 
				{ unw, Nw, {uNW, vT}, topMat },
				{ usw, Nw, {uE,  vT}, topMat },
				{ lsw, Nw, {uE,  vB}, botMat },
				{ lnw, Nw, {uNW, vB}, botMat }
			};
			AddQuad(west);

			Vec3 Nl{ 0,0,-1 };

			float du = Length(lnw - lne) * uvScale;
			float dv = Length(lnw - lsw) * uvScale;

			ObjectQuad lower
			{
				{ lsw, Nl,{ 0,  dv }, bottom },
				{ lse, Nl,{ du, dv }, bottom },
				{ lne, Nl,{ du,  0 }, bottom },
				{ lnw, Nl,{ 0,   0 }, bottom }
			};
			AddQuad(lower);

			Vec3 Nu{ 0,0,1 };

			ObjectQuad upper
			{ 
				{ unw,  Nu,{ 0,   0 }, top },
				{ une,  Nu,{ du,  0 }, top },
				{ use,  Nu,{ du,  dv }, top },
				{ usw,  Nu,{ 0,   dv }, top },
			};

			AddQuad(upper);

			origin += direction * length;
		}

		void Advance(Metres distance)
		{
			origin += direction * distance;
		}

		void AddPyramid(Metres length, const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d) override
		{
			float z0 = 0;
			float z1 = length;

			Vec3 apx = Transform({ 0,0, z1 });
			Vec3 se = Transform({ c.x, c.y, z0 });
			Vec3 sw = Transform({ d.x, d.y, z0 });
			Vec3 ne = Transform({ b.x, b.y, z0 });
			Vec3 nw = Transform({ a.x, a.y, z0 });

			float vB = 0;
			float vT = length * uvScale;
			float uSW = 0;
			float uSE = Length(se - sw) * uvScale;

			MaterialVertexData topMat = { top.colour, middle.materialId, top.gloss };
			MaterialVertexData botMat = { bottom.colour, middle.materialId, bottom.gloss };

			Vec3 Ns = Normalize ( Cross(apx - se, sw - se) );

			VertexTriangle south
			{
				{ apx,Ns,{ uSW, vT }, topMat },
				{ se, Ns,{ uSE, vT }, topMat },
				{ sw, Ns,{ uSE, vB }, topMat }
			};

			triangles.push_back(south);

			float uNE = uSE + Length(ne - se) * uvScale;

			Vec3 Ne = Normalize ( Cross (apx - ne, se - ne ) );
			VertexTriangle east
			{
				{ apx,Ne,{ uSE, vT }, topMat },
				{ ne, Ne,{ uNE, vT }, topMat },
				{ se, Ne,{ uNE, vB }, topMat }
			};

			triangles.push_back(east);

			Vec3 Nn = Normalize ( Cross (apx - ne, ne - nw) );

			float uNW = uNE + Length(ne - nw) * uvScale;

			VertexTriangle north
			{
				{ apx,Nn,{ uNE, vT }, topMat },
				{ nw, Nn, { uNW, vT }, topMat },
				{ ne, Nn,{ uNE, vB }, topMat },
			};
			triangles.push_back(north);

			float uE = uNW + Length(ne - se) * uvScale;

			Vec3 Nw = Normalize(Cross(apx - sw, nw - sw));

			VertexTriangle west
			{
				{ apx, Nw,{ uNW, vT }, topMat },
				{ sw, Nw,{ uE,  vT }, topMat },
				{ nw, Nw,{ uE,  vB }, topMat }
			};
			triangles.push_back(west);

			Vec3 Nl{ 0,0,-1 };

			float du = Length(nw - ne) * uvScale;
			float dv = Length(nw - sw) * uvScale;

			ObjectQuad lower
			{
				{ sw, Nl,{ 0,  dv }, bottom },
				{ se, Nl,{ du, dv }, bottom },
				{ ne, Nl,{ du,  0 }, bottom },
				{ nw, Nl,{ 0,   0 }, bottom }
			};
			AddQuad(lower);

			origin += direction * length;
		}

		void AddPrism(Metres length, const Vec2& a, const Vec2& b, const Vec2& c) override
		{
			float z0 = 0;
			float z1 = length;

			Vec3 ua = Transform({ a.x, a.y, z1 });
			Vec3 ub = Transform({ b.x, b.y, z1 });
			Vec3 uc = Transform({ c.x, c.y, z1 });
			Vec3 la = Transform({ a.x, a.y, z0 });
			Vec3 lb = Transform({ b.x, b.y, z0 });
			Vec3 lc = Transform({ c.x, c.y, z0 });

			float vB = 0;
			float vT = length * uvScale;

			float uAtA = 0;
			float uAtB = Length(ub - ua) * uvScale;
			float uAtC = Length(ub - uc) * uvScale + uAtB;

			MaterialVertexData topMat = { top.colour, middle.materialId, top.gloss };
			MaterialVertexData botMat = { bottom.colour, middle.materialId, bottom.gloss };

			Vec3 Nab = Normalize(Cross(ub - ua, lb - ub));

			ObjectQuad AB
			{
				{ ub,Nab,{ uAtB, vT }, topMat },
				{ ua,Nab,{ uAtA, vT }, topMat },
				{ la,Nab,{ uAtA, vB }, botMat },
				{ lb,Nab,{ uAtB, vB }, botMat },
			};

			AddQuad(AB);

			Vec3 Nbc = Normalize(Cross(ub - uc, ub - lb));

			ObjectQuad BC
			{
				{ uc, Nbc, { uAtC, vT }, topMat },
				{ ub, Nbc, { uAtB, vT }, topMat },
				{ lb, Nbc, { uAtB, vB }, botMat },
				{ lc, Nbc, { uAtC, vB }, botMat },
			};
			AddQuad(BC);

			Vec3 Nac = Normalize(Cross(uc - ua, uc - lc));

			ObjectQuad AC
			{
				{ ua, Nac, { uAtA, vT }, topMat },
				{ uc, Nac, { uAtC, vT }, topMat },
				{ lc, Nac, { uAtC, vB }, botMat },
				{ la, Nac, { uAtA, vB }, botMat },
			};
			AddQuad(AC);

			Vec3 Nl{ 0,0,-1 };

			VertexTriangle lower
			{
				{ la, Nl, { la.x * uvScale,  la.y * uvScale }, bottom },
				{ lc, Nl, { lc.x * uvScale,  lc.y * uvScale }, bottom },
				{ lb, Nl,{ lb.x * uvScale,   lb.y * uvScale }, bottom },
			};
			
			triangles.push_back(lower);

			Vec3 Nu{ 0,0,1 };

			VertexTriangle upper
			{
				{ ua,  Nu,{ ua.x * uvScale,  ua.y * uvScale }, top },
				{ ub,  Nu,{ ub.x * uvScale,  ub.y * uvScale }, top },
				{ uc,  Nu,{ uc.x * uvScale,  uc.y * uvScale }, top }
			};

			triangles.push_back(lower);

			origin += direction * length;
		}

		void AddSphere(Metres radius, int32 nRings, int32 nDivs) override
		{
			if (nRings < 4)
			{
				Throw(0, "RodTesselator::AddSphere(...) -> nRings needds to be >= 4");
			}

			if (nDivs < 3)
			{
				Throw(0, "RodTesselator::AddSphere(...) -> nDivs needds to be >= 3");
			}

			float dh = 2.0f * radius / nRings;
			auto dTheta = Degrees{ 360.0f / nDivs };

			float yTop = radius;
			float yBottom = -radius;

			float y0 = yBottom + dh;

			float V = 0;

			float theta0 = 0;
			float theta = 0;
			float s0 = 0;
			float c0 = 1.0f;

			origin += direction * radius;

			AddHats(nDivs, radius, dh, true, true, true);

			for (int32 ring = 1; ring < nDivs - 1; ring++)
			{	
				float DV = AddRingSliceFromSphere(y0, dh, radius, nDivs, V, true, ring * dh / radius);
				y0 = y0 + dh;
				V += DV;
			}

			origin += direction * radius;
		}

		void AddHats(int nDivs, float radius, float dh, bool facesOutwards, bool renderTop, bool renderBottom)
		{
			if (radius <= 0)
			{
				Throw(0, "RodTesselator::AddHats(...): radius was not positive");
			}

			auto dTheta = Degrees{ 360.0f / nDivs };
			float theta = 0;
			float theta0 = 0;

			float s0 = 0;
			float c0 = 1.0f;

			float yBottom = -radius;
			float y0 = yBottom + dh;

			for (int div = 0; div < nDivs; ++div)
			{
				theta += dTheta;

				float s1 = Sin(Degrees{ theta });
				float c1 = Cos(Degrees{ theta });

				float R1 = sqrtf(radius*radius - y0*y0);

				Triangle q{
					{ 0, 0, -radius },
					{ c0 * R1, s0 * R1, y0 },
					{ c1 * R1, s1 * R1, y0 }
				};

				Triangle Nq
				{
					Normalize(q.A),
					Normalize(q.B),
					Normalize(q.C)
				};

				float DR = R1;
				float DH = dh;
				float DV = dh / radius;

				float uscale = sqrtf(1 - Sq(1 - DV));
				float ub = theta / 360.0f;
				float uc = theta0 / 360.0f;

				ub *= uscale;
				uc *= uscale;

				ub -= (uscale * 0.5f);
				uc -= (uscale * 0.5f);

				ub += 0.5f;
				uc += 0.5f;

				if (renderBottom)
				{
					if (facesOutwards)
					{
						VertexTriangle vt
						{
							{ q.A + origin, Nq.A,{ 0.5f, 0 }, middle },
							{ q.B + origin, Nq.B,{ ub * uscale, DV }, middle },
							{ q.C + origin, Nq.C,{ uc * uscale, DV }, middle },
						};

						triangles.push_back(vt);
					}
					else
					{
						VertexTriangle vt
						{
							{ q.C + origin, -Nq.C,{ uc * uscale, DV }, middle },
							{ q.B + origin, -Nq.B,{ ub * uscale, DV }, middle },
							{ q.A + origin, -Nq.A,{ 0.5f, 0 }, middle }
						};

						triangles.push_back(vt);
					}
				}

				float yTop = radius - dh;

				R1 = sqrtf(radius*radius - yTop*yTop);

				q = {
					{ 0, 0, radius },
					{ c1 * R1, s1 * R1, yTop },
					{ c0 * R1, s0 * R1, yTop }
				};

				Nq =
				{
					Normalize(q.A),
					Normalize(q.B),
					Normalize(q.C)
				};

				if (renderTop)
				{
					if (facesOutwards)
					{
						VertexTriangle vt =
						{
							{ q.A + origin, Nq.A,{ 0.5f, 0 }, middle },
							{ q.B + origin, Nq.B,{ ub, DV }, middle },
							{ q.C + origin, Nq.C,{ uc, DV }, middle },
						};

						triangles.push_back(vt);
					}
					else
					{
						VertexTriangle vt =
						{
							{ q.C + origin, -Nq.C,{ uc, DV }, middle },
							{ q.B + origin, -Nq.B,{ ub, DV }, middle },
							{ q.A + origin, -Nq.A,{ 0.5f, 0 }, middle },
						};

						triangles.push_back(vt);
					}
				}

				theta0 = theta;

				s0 = s1;
				c0 = c1;
			}
		}

		float AddRingSliceFromSphere(float y0, float dh, float radius, int nDivs, float V, bool facesOutwards, float uFactor)
		{
			float y1 = y0 + dh;

			auto dTheta = Degrees{ 360.0f / nDivs };
			float theta = 0;
			float theta0 = 0;

			float R0 = sqrtf(radius*radius - y0*y0);
			float R1 = sqrtf(radius*radius - y1*y1);

			float DR = fabsf(R0 - R1);
			float DH = dh;
			float DV = 0.5f * sqrtf(DH*DH + DR*DR) / radius;

			float s0 = 0;
			float c0 = 1.0f;

			for (int div = 0; div < nDivs; ++div)
			{
				theta += dTheta;

				float s1 = Sin(Degrees{ theta });
				float c1 = Cos(Degrees{ theta });

				Quad q{
					{ c0 * R1, s0 * R1, y1 },
					{ c1 * R1, s1 * R1, y1 },
					{ c1 * R0, s1 * R0, y0 },
					{ c0 * R0, s0 * R0, y0 }
				};

				Quad Nq
				{
					Normalize(q.a),
					Normalize(q.b),
					Normalize(q.c),
					Normalize(q.d),
				};

				float ua = theta0 / 360.0f;
				float ub = theta / 360.0f;
				float uc = theta / 360.0f;
				float ud = theta0 / 360.0f;

				float f = 1 - fabsf(uFactor - 1);

				float uscale = sqrtf(1 - Sq(1 - f));

				ua *= uscale;
				ub *= uscale;
				uc *= uscale;
				ud *= uscale;

				ua -= 0.5f * uscale;
				ub -= 0.5f * uscale;
				uc -= 0.5f * uscale;
				ud -= 0.5f * uscale;

				if (facesOutwards)
				{
					ObjectQuad oq
					{
						{ q.a + origin, Nq.a,{ ua, V + DV }, middle },
						{ q.b + origin, Nq.b,{ ub, V + DV }, middle },
						{ q.c + origin, Nq.c,{ uc, V }, middle },
						{ q.d + origin, Nq.d,{ ud, V }, middle },
					};

					AddQuad(oq);
				}
				else
				{

					ObjectQuad oq
					{
						{ q.d + origin, -Nq.d,{ ua, V }, middle },
						{ q.c + origin, -Nq.c,{ ub, V }, middle },
						{ q.b + origin, -Nq.b,{ uc, V + DV }, middle },
						{ q.a + origin, -Nq.a,{ ud, V + DV }, middle },
					};

					AddQuad(oq);
				}

				theta0 = theta;

				s0 = s1;
				c0 = c1;
			}

			return DV;
		}

		void JoinAndTesselate(const ObjectVertex* srcRing, const ObjectVertex* dstRing, size_t nRings, float u0, float u1, float tubeRadius)
		{
			Ring<ObjectVertex> src(srcRing, nRings);
			Ring<ObjectVertex> dst(dstRing, nRings);

			float v0 = 0;
			float dv = uvScale * 2.8f * PI() * tubeRadius / nRings;

			for (size_t i = 0; i < nRings; ++i)
			{
				float v1 = v0 + dv;

				ObjectQuad oq;
				oq.a = src[i + 1];
				oq.b = dst[i + 1];
				oq.c = dst[i];
				oq.d = src[i];

				oq.a.uv.x = oq.d.uv.x = u0;
				oq.b.uv.x = oq.c.uv.x = u1;

				oq.d.uv.y = oq.c.uv.y = v0;
				oq.a.uv.y = oq.b.uv.y = v1;

				oq.a.position += origin;
				oq.b.position += origin;
				oq.c.position += origin;
				oq.d.position += origin;

				triangles.push_back({ oq.a, oq.b, oq.c });
				triangles.push_back({ oq.c, oq.d, oq.a });

				v0 = v1;
			}
		}

		void AddTorus(Metres innerRadius, Metres outerRadius, int32 nRings, int32 nDivs) override
		{
			if (nDivs < 3 || nDivs > 100)
			{
				Throw(0, "RodTesselator::AddTorus(...): nDvis must be >= 3 and <= 100");
			}

			if (nRings < 3 || nRings > 100)
			{
				Throw(0, "RodTesselator::AddTorus(...): nRings must be >= 3 and <= 100");
			}

			float radius = 0.5f * (innerRadius + outerRadius);

			auto dTheta = Degrees{ 360.0f / nDivs };

			auto dPhi = Degrees{ 360.0f / nRings };

			float cTheta0 = 1;
			float sTheta0 = 0;

			Degrees theta = { 0 };

			Metres tubeRadius { outerRadius - innerRadius };

			Degrees phi = { 0 };

			float cPhi0 = 1;
			float sPhi0 = 0;

			tempBuffer0.clear();
			tempBuffer1.clear();
			tempBuffer0.reserve(nRings);
			tempBuffer1.resize(nRings);
			tempBuffer2.resize(nRings);

			for (int j = 0; j < nRings; ++j)
			{
				phi.degrees += dPhi;

				float cPhi1 = Cos(phi);
				float sPhi1 = Sin(phi);

				ObjectVertex v;
				v.position =
				{
					tubeRadius * cPhi0,
					0,
					tubeRadius * sPhi0,
				};

				v.material = middle;
				v.normal = { cPhi0, 0, sPhi0 };

				tempBuffer0.push_back(v);

				cPhi0 = cPhi1;
				sPhi0 = sPhi1;
			}

			const ObjectVertex* ring = tempBuffer0.data();
			ObjectVertex* srcRing = tempBuffer1.data();
			ObjectVertex* dstRing = tempBuffer2.data();

			auto T0 = Matrix4x4::Translate(Vec3{ radius,0,0 });
			for (size_t i = 0; i < nRings; ++i)
			{
				TransformPosition(T0, ring[i].position, srcRing[i].position);
				TransformDirection(T0, ring[i].normal, srcRing[i].normal);
				srcRing[i].material = dstRing[i].material = middle;
			}

			float u0 = 0;
			float du = uvScale * radius * 2 * Rococo::PI() / nDivs;

			for (int32 i = 0; i < nDivs; ++i)
			{
				theta.degrees += dTheta;
				Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);

				Vec3 DR
				{
					radius * Cos(theta),
					radius * Sin(theta),
					0
				};

				Matrix4x4 T = Matrix4x4::Translate(DR);
				Matrix4x4 RT = T * Rz;

				for (size_t i = 0; i < nRings; ++i)
				{
					TransformPosition(RT, ring[i].position, dstRing[i].position);
					TransformDirection(RT, ring[i].normal, dstRing[i].normal);
				}

				float u = u0 + du;

				JoinAndTesselate(srcRing, dstRing, nRings, u0, u, tubeRadius);

				u0 = u;

				std::swap(srcRing, dstRing);
			}

			origin += direction * 2 * tubeRadius;
		}

		void AddHollowDisc(float yOuter, float yInner, float outerRadius, float interRadius, int32 nDivs, float V, float DV)
		{
			auto dTheta = Degrees{ 360.0f / nDivs };
			float theta = 0;
			float theta0 = 0;

			float y1 = yOuter;
			float y0 = yInner;

			float R1 = outerRadius;
			float R0 = interRadius;

			float DR = R1 - R0;

			float s0 = 0;
			float c0 = 1.0f;

			for (int div = 0; div < nDivs; ++div)
			{
				theta += dTheta;

				float s1 = Sin(Degrees{ theta });
				float c1 = Cos(Degrees{ theta });

				Quad q{
					{ c1 * R1, s1 * R1, y1 },
					{ c0 * R1, s0 * R1, y1 },
					{ c0 * R0, s0 * R0, y0 },
					{ c1 * R0, s1 * R0, y0 }			
				};

				float dy = y1 - y0;
				float gz = dy;

				Vec3 na = Normalize({ -c1 * gz, -s1 * gz, DR });
				Vec3 nb = Normalize({ -c0 * gz, -s0 * gz, DR });
				Vec3 nc = Normalize({ -c0 * gz, -s0 * gz, DR });
				Vec3 nd = Normalize({ -c1 * gz, -s1 * gz, DR });

				Vec2 ua = Vec2 { q.a.x, q.a.y } * uvScale;
				Vec2 ub = Vec2 { q.b.x, q.b.y } * uvScale;
				Vec2 uc = Vec2 { q.c.x, q.c.y } * uvScale;
				Vec2 ud = Vec2 { q.d.x, q.d.y } * uvScale;

				ObjectQuad oq
				{
					{ q.a + origin, na, ua, top },
					{ q.b + origin, nb, ub, top },
					{ q.c + origin, nc, uc, top },
					{ q.d + origin, nd, ud, top },
				};

				AddQuad(oq);

				theta0 = theta;

				s0 = s1;
				c0 = c1;
			}
		}

		void AddBowl(Metres radius1, Metres radius2, int32 nRings, int32 startRing, int32 endRing, int32 nDivs) override
		{
			if (nRings < 4)
			{
				Throw(0, "RodTesselator::AddBowl(...) -> nRings needds to be >= 4");
			}

			if (nDivs < 3)
			{
				Throw(0, "RodTesselator::AddBowl(...) -> nDivs needds to be >= 3");
			}

			float outerRadius = radius1;
			float innerRadius = radius2;

			if (outerRadius < innerRadius) std::swap(outerRadius, innerRadius);

			float dh = 2.0f * outerRadius / nRings;
			auto dTheta = Degrees{ 360.0f / nDivs };

			float yTop = outerRadius;
			float yBottom = -outerRadius;

			float y0 = yBottom + dh;

			float V = 0;

			float theta0 = 0;
			float theta = 0;
			float s0 = 0;
			float c0 = 1.0f;

			float startDy = dh * (endRing - startRing + 2) * 0.5f;
			origin += direction * startDy;

			if (startRing == 0)
			{
				AddHats(nDivs, outerRadius, dh, true, false, true);
			}

			if (endRing >= nRings - 1)
			{
				AddHats(nDivs, outerRadius, dh, true, true, false);
			}

			for (int32 ring = max(1,startRing); ring < min(endRing, nRings - 1); ring++)
			{
				float DV = AddRingSliceFromSphere(y0, dh, outerRadius, nDivs, V, true, ring * dh / outerRadius);
				y0 = y0 + dh;
				V += DV;
			}

			float yOuter = y0;

			dh = 2.0f * innerRadius / nRings;
			y0 = -innerRadius + dh;

			V = 0;

			if (startRing == 0)
			{
				AddHats(nDivs, innerRadius, dh, false, false, true);
			}

			if (endRing >= nRings - 1)
			{
				AddHats(nDivs, innerRadius, dh, false, true, false);
			}

			startDy = dh * (startRing - 1);
			y0 += startDy;

			for (int32 ring = max(1, startRing); ring < min(endRing, nRings - 1); ring++)
			{
				float DV = AddRingSliceFromSphere(y0, dh, innerRadius, nDivs, V, false, ring * dh / outerRadius);
				y0 = y0 + dh;
				V += DV;
			}

			float yInner = y0;

			float DV = outerRadius - innerRadius;

			if (endRing < nRings - 1)
			{
				float R0 = sqrtf(outerRadius*outerRadius - yOuter*yOuter);
				float R1 = sqrtf(innerRadius*innerRadius - yInner*yInner);

				AddHollowDisc(yOuter, yInner, R0, R1, nDivs, V, DV);
			}

			origin += direction * outerRadius;
		}

		void AddDisc(Metres radius, int32 nDivs, cr_vec3 normal)
		{
			float dTheta = Degrees{ 360.0f / nDivs };
			float theta0 = 0;

			float s0 = 0;
			float c0 = 1;

			for (int div = 0; div < nDivs; ++div)
			{
				float theta = theta0 + dTheta;

				float s1 = Sin(Degrees{ theta });
				float c1 = Cos(Degrees{ theta });

				Triangle t
				{
					{ 0,    0,     0 },
					{ c1 * radius, s1 * radius, 0 },
					{ c0 * radius, s0 * radius, 0 }
				};

				Vec2 uvB = { t.B.x * uvScale * 0.25f, t.B.y * uvScale * 0.25f };
				Vec2 uvC = { t.C.x * uvScale * 0.25f, t.C.y * uvScale * 0.25f };

				VertexTriangle vt
				{
					{ t.A + origin, normal, {0,0}, middle },
					{ t.B + origin, normal, uvB,  middle },
					{ t.C + origin, normal, uvC,  middle }
				};

				if (normal.z < 0)
				{
					std::swap(vt.b, vt.c);
				}

				triangles.push_back(vt);

				s0 = s1;
				c0 = c1;

				theta0 = theta;
			}
		}

		void AddTube(Metres length, Metres bottomRadius, Metres topRadius, int32 nDivs) override
		{
			if (nDivs < 3)
			{
				Throw(0, "RodTesselator::AddTube(...): nDivs must be >= 3");
			}

			AddDisc(bottomRadius, nDivs, { 0,0,-1 });

			float dTheta = Degrees{ 360.0f / nDivs };
			float theta0 = 0;

			float s0 = 0;
			float c0 = 1;

			for (int div = 0; div < nDivs; ++div)
			{
				float theta = theta0 + dTheta;

				float s1 = Sin(Degrees{ theta });
				float c1 = Cos(Degrees{ theta });

				Quad q{
					{ c0 * topRadius, s0 * topRadius, length },
					{ c1 * topRadius, s1 * topRadius, length },
					{ c1 * bottomRadius, s1 * bottomRadius, 0 },
					{ c0 * bottomRadius, s0 * bottomRadius, 0 }
				};

				struct
				{
					float R1;
					float R0;
					float length;

					Vec3 operator()(float sinTheta, float cosTheta)
					{
						Vec3 gradTheta = { -sinTheta, cosTheta };
						Vec3 gradLength = Vec3{ cosTheta, sinTheta, 0 } *(float)(R0 - R1) + Vec3{0, 0, 1};
						return Normalize(Cross (gradTheta, gradLength));
					}
				} normal;

				normal.length = length;
				normal.R0 = bottomRadius;
				normal.R1 = topRadius;

				Quad Nq
				{
					normal(s0, c0),
					normal(s1, c1),
					normal(s1, c1),
					normal(s0, c0),
				};

				float DV = length * uvScale / (2.0f * PI());

				float rad = 0.5f * uvScale *(topRadius + bottomRadius);

				float ua = rad * theta0 / 360.0f;
				float ub = rad * theta / 360.0f;
				float uc = rad * theta / 360.0f;
				float ud = rad * theta0 / 360.0f;

				ObjectQuad oq
				{
					{ q.a + origin, Nq.a,{ ua, DV }, middle },
					{ q.b + origin, Nq.b,{ ub, DV }, middle },
					{ q.c + origin, Nq.c,{ uc, 0 }, middle },
					{ q.d + origin, Nq.d,{ ud, 0 }, middle },
				};

				AddQuad(oq);

				s0 = s1;
				c0 = c1;

				theta0 = theta;
			}

			origin.z += length;

			AddDisc(topRadius, nDivs, { 0,0,1 });
		}

		void AddVertex(cr_vec2 v) override
		{
			vertices.push_back(v);
		}

		void Clear() override
		{
			triangles.clear();
			direction = { 0, 0, 1 };
			bitangent = { 1, 0, 0 };
			origin = { 0,0,0 };
			vertices.clear();
			blendLists.clear();
		}

		void ClearVertices() override
		{
			vertices.clear();
		}

		void CloseLoop() override
		{
			if (vertices.size() < 2)
			{
				Throw(0, "RodTesselator::CloseLoop(): vertex count must be >= 2");
			}

			vertices.push_back(vertices[0]);
		}

		void Destruct() override
		{
			delete this;
		}

		boolean32 PopNextTriangle(VertexTriangle& t) override
		{
			if (triangles.empty())
			{
				t.a = t.b = t.c = { 0 };
				return false;
			}

			t = triangles.back();
			triangles.pop_back();
			return true;
		}

		void CopyToMeshBuilder(const fstring& meshName, boolean32 preserveMesh, boolean32 isInvisible, boolean32 castsShadows)
		{
			switch (blendLists.size())
			{
			case 0:
				break;
			case 2:
				if (blendLists[0].weights.size() != triangles.size() * 3)
				{
					Throw(0, "Rod Tesselator: Cannot copy to mesh builder. The number of weights in blendLists[0] does not match the number of vertices in the triangle list");
				}
				if (blendLists[1].weights.size() != triangles.size() * 3)
				{
					Throw(0, "Rod Tesselator: Cannot copy to mesh builder. The number of weights in blendLists[1] does not match the number of vertices in the triangle list");
				}
				break;
			default:
				Throw(0, "Rod Tesselator: Expecting two blend lists. %llu were supplied", blendLists.size());
			}

			meshes.Begin(meshName);

			for (auto& t : triangles)
			{
				meshes.AddTriangleEx(t);
			}

			if (blendLists.size() >= 2)
			{
				int index0 = blendLists[0].boneIndex;
				int index1 = blendLists[1].boneIndex;

				for (size_t i = 0; i < triangles.size(); i++)
				{
					BoneWeights weightsA, weightsB, weightsC;
					weightsA.bone0.index = (float)index0;
					weightsA.bone1.index = (float)index1;
					weightsA.bone0.weight = blendLists[0].weights[3 * i];
					weightsA.bone1.weight = blendLists[1].weights[3 * i];
					weightsB.bone0.index = (float)index0;
					weightsB.bone1.index = (float)index1;
					weightsB.bone0.weight = blendLists[0].weights[3 * i + 1];
					weightsB.bone1.weight = blendLists[1].weights[3 * i + 1];
					weightsC.bone0.index = (float)index0;
					weightsC.bone1.index = (float)index1;
					weightsC.bone0.weight = blendLists[0].weights[3 * i + 2];
					weightsC.bone1.weight = blendLists[1].weights[3 * i + 2];

					meshes.AddBoneWeights(weightsA, weightsB, weightsC);
				}
			}

			meshes.End(preserveMesh, isInvisible);
			meshes.SetShadowCasting(meshName, castsShadows);
		}

		void GetOrigin(Vec3& origin)
		{
			origin = this->origin;
		}

		void RaiseBox(Metres length)
		{
			if (vertices.size() < 2)
			{
				Throw(0, "RodTesselator::RaiseBox(...): Need at least 3 vertices to raise a box");
			}

			float u0 = 0;

			for (size_t i = 0; i < vertices.size() - 1; ++i)
			{
				Vec2 c2d = vertices[i];
				Vec2 d2d = vertices[i + 1];

				float u1 = u0 + Length(d2d - c2d);

				Vec3 a{ d2d.x, d2d.y, length };
				Vec3 b{ c2d.x, c2d.y, length };
				Vec3 c{ c2d.x, c2d.y, 0 };
				Vec3 d{ d2d.x, d2d.y, 0 };	

				ObjectQuad q;
				q.a.position = a + origin;
				q.b.position = b + origin;
				q.c.position = c + origin;
				q.d.position = d + origin;

				q.a.normal = q.b.normal = q.c.normal = q.d.normal = Normalize(Cross(a - b, c - b));

				q.a.uv.y = q.b.uv.y = length * uvScale;
				q.c.uv.y = q.d.uv.y = 0;

				q.a.uv.x = q.d.uv.x = u0;
				q.b.uv.x = q.c.uv.x = u1;

				q.a.material.colour = q.b.material.colour = top.colour;
				q.d.material.colour = q.c.material.colour = bottom.colour;

				q.a.material.gloss = q.b.material.gloss = top.gloss;
				q.d.material.gloss = q.c.material.gloss = bottom.gloss;

				q.a.material.materialId = q.b.material.materialId = middle.materialId;
				q.d.material.materialId = q.c.material.materialId = middle.materialId;

				AddQuad(q);

				u1 = u0;
			}

			origin += direction * length;
		}

		void RaisePyramid(Metres length)
		{
			if (vertices.size() < 2)
			{
				Throw(0, "RodTesselator::RaiseBox(...): Need at least 3 vertices to raise a box");
			}

			float u0 = 0;

			for (size_t i = 0; i < vertices.size() - 1; ++i)
			{
				Vec2 c2d = vertices[i];
				Vec2 d2d = vertices[i + 1];

				float u1 = u0 + Length(d2d - c2d);

				Vec3 o{ 0, 0, length };
				Vec3 c{ c2d.x, c2d.y, 0 };
				Vec3 d{ d2d.x, d2d.y, 0 };

				VertexTriangle q;
				q.a.position = o + origin;
				q.b.position = c + origin;
				q.c.position = d + origin;

				q.a.normal = q.b.normal = q.c.normal = Normalize(Cross(o - c, d - c));

				q.a.uv.y = q.b.uv.y = length * uvScale;
				q.c.uv.y = 0;

				q.a.uv.x = u0;
				q.b.uv.x = q.c.uv.x = u1;

				q.a.material.colour = q.b.material.colour = top.colour;
				q.c.material.colour = bottom.colour;

				q.a.material.gloss = q.b.material.gloss = top.gloss;
				q.c.material.gloss = bottom.gloss;

				q.a.material.materialId = q.b.material.materialId = middle.materialId;
				q.c.material.materialId = middle.materialId;

				triangles.push_back(q);

				u1 = u0;
			}

			origin += direction * length;
		}

		void Scale(float sx, float sy, float sz) override
		{
			for (auto& t : triangles)
			{
				t.a.position.x *= sx;
				t.a.position.y *= sy;
				t.a.position.z *= sz;
				t.b.position.x *= sx;
				t.b.position.y *= sy;
				t.b.position.z *= sz;
				t.c.position.x *= sx;
				t.c.position.y *= sy;
				t.c.position.z *= sz;

				t.a.normal.x /= sx;
				t.a.normal.y /= sy;
				t.a.normal.z /= sz;
				t.b.normal.x /= sx;
				t.b.normal.y /= sy;
				t.b.normal.z /= sz;
				t.c.normal.x /= sx;
				t.c.normal.y /= sy;
				t.c.normal.z /= sz;

				t.a.normal = Normalize(t.a.normal);
				t.b.normal = Normalize(t.b.normal);
				t.c.normal = Normalize(t.c.normal);
			}
		}

		void SetMaterialBottom(const MaterialVertexData& bottom) override
		{
			this->bottom = bottom;
		}

		void SetMaterialMiddle(const MaterialVertexData& middle) override
		{
			this->middle = middle;
		}

		void SetMaterialTop(const MaterialVertexData& top) override
		{
			this->top = top;
		}

		void SetOrigin(cr_vec3 origin) override
		{
			this->origin = origin;
		}

		void UseFaceNormals() override
		{
			smoothNormals = false;
		}

		void UseSmoothNormals() override
		{
			smoothNormals = true;
		}

		void SetUVScale(float uvScale) override
		{
			this->uvScale = uvScale;
		}

		void TransformVertices(const Matrix4x4& m) override
		{
			for (auto& t : triangles)
			{
				Vec3 newPosition;
				Vec3 newNormal;

				TransformPosition(m, t.a.position, newPosition);
				TransformDirection(m, t.a.normal, newNormal);

				t.a.position = newPosition;
				t.a.normal = newNormal;

				TransformPosition(m, t.b.position, newPosition);
				TransformDirection(m, t.b.normal, newNormal);

				t.b.position = newPosition;
				t.b.normal = newNormal;

				TransformPosition(m, t.c.position, newPosition);
				TransformDirection(m, t.c.normal, newNormal);

				t.c.position = newPosition;
				t.c.normal = newNormal;
			}
		}
	};
}

namespace Rococo::Graphics
{
	IRodTesselatorSupervisor* CreateRodTesselator(IMeshBuilder& meshes)
	{
		return new ANON::RodTesselator(meshes);
	}

	IRodTesselatorSupervisor* CreateIsolatedRodTesselator()
	{
		struct NullMeshBuilder : IMeshBuilder
		{
			void AddMesh(const Matrix4x4& transform, const fstring& sourceName) override {}
			void AddTriangleEx(const VertexTriangle& t) override {}
			void AddTriangle(const ObjectVertex& a, const ObjectVertex& b, const ObjectVertex& c) override {}
			void AddBoneWeights(const BoneWeights& a, const BoneWeights& b, const BoneWeights& c) override {}
			void AddPhysicsHull(const Triangle& t) override {}

			void Begin(const fstring& meshName) override 
			{
				Throw(0, "Isolated rod tesselators do not support exporting to the mesh builder");
			}

			void End(boolean32 preserveCopy, boolean32 invisible) override {}
			void Clear() override {}
			void Delete(const fstring& fqName) override {}
			void SetShadowCasting(const fstring& fqName, boolean32 isActive) override {}
			void SetSpecialAmbientShader(const fstring& fqName, const fstring& vs, const fstring& ps, boolean32 alphaBlending) override {}
			void SetSpecialSpotlightShader(const fstring& fqName, const fstring& vs, const fstring& ps, boolean32 alphaBlending) override {}
			void Span(Vec3& span, const fstring& fqName) override {}
		};
			
		static NullMeshBuilder s_NullMeshBuilder;

		return new ANON::RodTesselator(s_NullMeshBuilder);
	}
}
