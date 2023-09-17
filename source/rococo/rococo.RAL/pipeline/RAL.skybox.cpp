#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

struct RAL_Skybox : IRAL_Skybox
{
	IRAL& ral;
	IRenderStates& renderStates;

	ID_SYS_MESH skyMeshId;
	ID_VERTEX_SHADER idObjSkyVS;
	ID_PIXEL_SHADER idObjSkyPS;

	RAL_Skybox(IRAL& _ral, IRenderStates& _renderStates) : ral(_ral), renderStates(_renderStates)
	{
		VertexElement skyVertexElements[] =
		{
			VertexElement { "position", 0, VertexElementFormat::Float3, 0 },
			VertexElement { nullptr,    0 ,VertexElementFormat::Float3, 0 }
		};

		idObjSkyVS = ral.Shaders().CreateVertexShader("!shaders/compiled/skybox.vs", skyVertexElements);
		idObjSkyPS = ral.Shaders().CreatePixelShader("!shaders/compiled/skybox.ps");
	}

	void Free() override
	{
		delete this;
	}

	void RenderSkyBox(IScene& scene) override
	{
		ID_CUBE_TEXTURE cubeId = scene.GetSkyboxCubeId();

		if (!skyMeshId)
		{
			SkyVertex topNW{ -1.0f, 1.0f, 1.0f };
			SkyVertex topNE{ 1.0f, 1.0f, 1.0f };
			SkyVertex topSW{ -1.0f,-1.0f, 1.0f };
			SkyVertex topSE{ 1.0f,-1.0f, 1.0f };
			SkyVertex botNW{ -1.0f, 1.0f,-1.0f };
			SkyVertex botNE{ 1.0f, 1.0f,-1.0f };
			SkyVertex botSW{ -1.0f,-1.0f,-1.0f };
			SkyVertex botSE{ 1.0f,-1.0f,-1.0f };

			SkyVertex skyboxVertices[36] =
			{
				topSW, topNW, topNE, // top,
				topNE, topSE, topSW, // top,
				botSW, botNW, botNE, // bottom,
				botNE, botSE, botSW, // bottom,
				topNW, topSW, botSW, // West
				botSW, botNW, topNW, // West
				topNE, topSE, botSE, // East
				botSE, botNE, topNE, // East
				topNW, topNE, botNE, // North
				botNE, botNW, topNW, // North
				topSW, topSE, botSE, // South
				botSE, botSW, topSW, // South
			};

			skyMeshId = ral.Meshes().CreateSkyMesh(skyboxVertices, sizeof(skyboxVertices) / sizeof(SkyVertex));
		}

		if (ral.Shaders().UseShaders(idObjSkyVS, idObjSkyPS))
		{
			ral.ClearBoundVertexBufferArray();
			ral.BindVertexBuffer(skyMeshId, sizeof(SkyVertex), 0);
			ral.CommitBoundVertexBuffers();

			renderStates.SetShaderTexture(TXUNIT_ENV_MAP, cubeId);
			renderStates.SetDrawTopology(PrimitiveTopology::TRIANGLELIST);
			renderStates.UseSkyRasterizer();
			renderStates.DisableWritesOnDepthState();
			renderStates.DisableBlend();

			ral.Draw(36, 0);

			renderStates.ResetSamplersToDefaults();
		}
		else
		{
			Throw(0, "DX11Renderer::RenderSkybox failed. Error setting sky shaders");
		}
	}
};

namespace Rococo::RAL
{
	IRAL_Skybox* CreateRALSkybox(IRAL& ral, IRenderStates& renderStates)
	{
		return new RAL_Skybox(ral, renderStates);
	}
}