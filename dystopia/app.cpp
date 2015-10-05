#include <rococo.renderer.h>
#include <rococo.io.h>

#include <vector>

#include "dystopia.text.h"

#include "rococo.geometry.inl"

#include <DirectXMath.h>

#include "meshes.h"

using namespace Dystopia;
using namespace Rococo;
using namespace Rococo::Fonts;

namespace
{
	float GetAspectRatio(const IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);
		float aspectRatio = metrics.screenSpan.y / float(metrics.screenSpan.x);
		return aspectRatio;
	}

	ID_MESH CreateCubeMesh(IRenderer& renderer)
	{
		Vec3 upperSW{ -0.5f, -0.5f, 0.5f };
		Vec3 upperNW{ -0.5f,  0.5f, 0.5f };
		Vec3 upperNE{ 0.5f,  0.5f, 0.5f };
		Vec3 upperSE{ 0.5f, -0.5f, 0.5f };

		Vec3 lowerSW{ -0.5f, -0.5f, -0.5f };
		Vec3 lowerNW{ -0.5f,  0.5f, -0.5f };
		Vec3 lowerNE{ 0.5f,  0.5f, -0.5f };
		Vec3 lowerSE{ 0.5f, -0.5f, -0.5f };

		Vec3 up{ 0, 0, 1 }, down{ 0, 0, -1 }, west{ -1, 0, 0 }, east{ 1, 0, 0 }, south{ 0, -1, 0 }, north{ 0, 1, 0 };
		RGBAb magenta(0xFF, 0x00, 0xFF);
		RGBAb white(0xFF, 0xFF, 0xFF);
		RGBAb cyan(0x00, 0xFF, 0xFF);
		RGBAb yellow(0xFF, 0xFF, 0x00);
		RGBAb blue(0x00, 0x00, 0xFF);
		RGBAb green(0x00, 0xFF, 0x00);
		RGBAb red(0xFF, 0x00, 0x00);

		ObjectVertex unitCube[] =
		{
			// Top face
			ObjectVertex{ upperSW, up, magenta,  white },
			ObjectVertex{ upperNW, up, magenta,  white },
			ObjectVertex{ upperSE, up, magenta,  white },
			ObjectVertex{ upperNW, up, magenta,  white },
			ObjectVertex{ upperNE, up, magenta,  white },
			ObjectVertex{ upperSE, up, magenta,  white },

			// Bottom face	
			ObjectVertex{ lowerNW, down, cyan, white },
			ObjectVertex{ lowerSW, down, cyan, white },
			ObjectVertex{ lowerSE, down, cyan, white },
			ObjectVertex{ lowerSE, down, cyan, white },
			ObjectVertex{ lowerNE, down, cyan, white },
			ObjectVertex{ lowerNW, down, cyan, white },

			// West face
			ObjectVertex{ upperNW, west, yellow, white },
			ObjectVertex{ upperSW, west, yellow, white },
			ObjectVertex{ lowerNW, west, yellow, white },
			ObjectVertex{ upperSW, west, yellow, white },
			ObjectVertex{ lowerSW, west, yellow, white },
			ObjectVertex{ lowerNW, west, yellow, white },

			// East face
			ObjectVertex{ upperSE, east, blue, white },
			ObjectVertex{ upperNE, east, blue, white },
			ObjectVertex{ lowerNE, east, blue, white },
			ObjectVertex{ lowerSE, east, blue, white },
			ObjectVertex{ upperSE, east, blue, white },
			ObjectVertex{ lowerNE, east, blue, white },

			// South face
			ObjectVertex{ upperSW, south, green, white },
			ObjectVertex{ upperSE, south, green, white },
			ObjectVertex{ lowerSW, south, green, white },
			ObjectVertex{ lowerSW, south, green, white },
			ObjectVertex{ upperSE, south, green, white },
			ObjectVertex{ lowerSE, south, green, white },

			// North face
			ObjectVertex{ upperNE, north, red, white },
			ObjectVertex{ upperNW, north, red, white },
			ObjectVertex{ lowerNW, north, red, white },
			ObjectVertex{ upperNE, north, red, white },
			ObjectVertex{ lowerNW, north, red, white },
			ObjectVertex{ lowerNE, north, red, white }
		};

		return renderer.CreateTriangleMesh(unitCube, sizeof(unitCube) / sizeof(ObjectVertex));
	}

	ID_MESH CreateRoadMesh(IRenderer& renderer)
	{
		Vec3 SW{ -3.0f, -100.0f, 0.01f };
		Vec3 NW{ -3.0f,  100.0f, 0.01f };
		Vec3 NE{  3.0f,  100.0f, 0.01f };
		Vec3 SE{  3.0f, -100.0f, 0.01f };

		Vec3 up{ 0, 0, 1 };
		
		RGBAb tarmac(128, 128, 128);
		RGBAb white(255, 255, 255);

		ObjectVertex roadVertices[] =
		{
			// Top face
			ObjectVertex{ SW, up, tarmac,  white },
			ObjectVertex{ NW, up, tarmac,  white },
			ObjectVertex{ SE, up, tarmac,  white },
			ObjectVertex{ NW, up, tarmac,  white },
			ObjectVertex{ NE, up, tarmac,  white },
			ObjectVertex{ SE, up, tarmac,  white }
		};

		return renderer.CreateTriangleMesh(roadVertices, sizeof(roadVertices) / sizeof(ObjectVertex));
	}

	void DrawTestTriangles(IGuiRenderContext& rc)
	{
		GuiVertex q0[6] =
		{
			{ 0.0f,600.0f, 1.0f, 0.0f,{ 255,0,0,255 }, 0.0f, 0.0f, 0 }, // bottom left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,0,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 0.0f,  0.0f, 1.0f, 0.0f,{ 0,0,255,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 0.0f,  0.0f, 1.0f, 0.0f,{ 255,255,0,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,255,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 800.0f,  0.0f, 1.0f, 0.0f,{ 255,0,255,255 }, 0.0f, 0.0f, 0 }  // top right
		};

		rc.AddTriangle(q0);
		rc.AddTriangle(q0 + 3);
	}

	void DrawGuiMetrics(IGuiRenderContext& rc)
	{
		GuiMetrics metrics;
		rc.Renderer().GetGuiMetrics(metrics);

		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"Mouse: (%d,%d). Screen(%d,%d)", metrics.cursorPosition.x, metrics.cursorPosition.y, metrics.screenSpan.x, metrics.screenSpan.y);

		RenderHorizontalCentredText(rc, info, RGBAb{ 255, 255, 255, 255 }, 1, Vec2i(25, 25));
	}

	class DystopiaApp : public IApp, private IScene
	{
	private:
		IRenderer& renderer;
		IOS& os;

		int globalScale;
		Degrees viewTheta;
		bool rbuttonDown;

		ID_MESH cubeMesh;
		ID_MESH roadMesh;
		ID_MESH humanMesh;

		AutoFree<IMeshLoader> meshLoader;
	public:
		DystopiaApp(IRenderer& _renderer, IOS& _os) : 
			renderer(_renderer), os(_os), globalScale(4), viewTheta{ 30.0f }, rbuttonDown(false) , meshLoader(CreateMeshLoader(_renderer, _os))
		{
		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnCreated()
		{
			cubeMesh = CreateCubeMesh(renderer);
			roadMesh = CreateRoadMesh(renderer);
			humanMesh = meshLoader->LoadMesh(L"!mesh\\human.sxy");
		}

		virtual uint32 OnTick(const IUltraClock& clock)
		{
			renderer.Render(*this);
			return 5;
		}

		virtual RGBA GetClearColour() const
		{
			return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
		}

		virtual void GetWorldMatrix(Matrix4x4& worldMatrix) const
		{
			using namespace DirectX;

			Degrees phi{ -45.0f };

			float g = 0.04f * powf(1.25f, (float)globalScale);

			XMMATRIX aspectCorrect = XMMatrixScaling(g * GetAspectRatio(renderer), g, g);
			XMMATRIX rotZ = XMMatrixRotationZ(ToRadians(viewTheta));
			XMMATRIX rotX = XMMatrixRotationX(ToRadians(phi));

			XMMATRIX t = XMMatrixMultiply(XMMatrixMultiply(rotZ, rotX), aspectCorrect);

			XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &worldMatrix, t);
		}


		virtual void RenderGui(IGuiRenderContext& gr)
		{
			// DrawTestTriangles(gr);
			DrawGuiMetrics(gr);
		}

		void RenderPlayer(IRenderContext& rc)
		{
			ObjectInstance identity
			{
				Vec4{ 1,0,0,0 },
				Vec4{ 0,1,0,0 },
				Vec4{ 0,0,1,0 },
				Vec4{ 0,0,0,1 }
			};

			rc.Draw(humanMesh, &identity, 1);
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			ObjectInstance identity
			{
				Vec4{ 1,0,0,0 },
				Vec4{ 0,1,0,0 }, 
				Vec4{ 0,0,1,0 }, 
				Vec4{ 0,0,0,1 }
			};

			ObjectInstance offroad[]
			{
				{
					Vec4{ 1.0f, 0.0f, 0.0f, -3.5f },
					Vec4{ 0.0f, 1.0f, 0.0f,  0.0f },
					Vec4{ 0.0f, 0.0f, 1.0f,  0.5f },
					Vec4{ 0.0f, 0.0f, 0.0f,  1.0f }
				},
				{
					Vec4{ 2.0f, 0.0f, 0.0f,  4.0f },
					Vec4{ 0.0f, 2.0f, 0.0f,  0.0f },
					Vec4{ 0.0f, 0.0f, 2.0f,  1.0f },
					Vec4{ 0.0f, 0.0f, 0.0f,  1.0f }
				}
			};

			rc.Draw(roadMesh, &identity, 1);
			rc.Draw(cubeMesh, offroad, sizeof(offroad)/ sizeof(ObjectInstance));

			RenderPlayer(rc);
		}

		MouseEvent lastEvent;

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			lastEvent = me;

			enum { MouseEvent_MouseWheel = 0x0400, MouseEvent_RDown = 0x0004, MouseEvent_RUp = 0x0008 };

			if ((me.buttonFlags & MouseEvent_MouseWheel) != 0)
			{
				int32 delta = (int32)(int16)me.buttonData;
				globalScale += delta > 0 ? 1 : -1;
				if (globalScale < 0) globalScale = 0;
				if (globalScale > 10) globalScale = 10;
			}

			if ((me.buttonFlags == MouseEvent_RDown) != 0)
			{
				rbuttonDown = true;
			}
			else if ((me.buttonFlags == MouseEvent_RUp) != 0)
			{
				rbuttonDown = false;
			}

			if (rbuttonDown)
			{
				viewTheta.quantity += me.dx;
			}
		}

		virtual void OnKeyboardEvent(const KeyboardEvent& k)
		{

		}
	};
}

namespace Dystopia
{
	IApp* CreateDystopiaApp(IRenderer& renderer, IOS& os)
	{
		return new DystopiaApp(renderer, os);
	}
}