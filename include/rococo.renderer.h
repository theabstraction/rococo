#ifndef ROCOCO_RENDERER_H
#define ROCOCO_RENDERER_H

#include <rococo.types.h>
#include <rococo.maths.h>
#include <rococo.io.h>

namespace Rococo
{
	namespace Fonts
	{
		struct IDrawTextJob;
	}

	struct IException;

	void ShowErrorBox(IException& ex, const wchar_t* caption);

	struct GuiVertex
	{
		float x;
		float y;
		float saturation;
		float fontBlend;
		RGBAb colour;
		float u;
		float v;
		float unused;
	};

	struct ObjectVertex
	{
		Vec3 position;
		Vec3 normal;
		RGBAb emissiveColour;
		RGBAb diffuseColour;
	};

	struct IRenderer;

	struct IGuiRenderContext
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;

		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual IRenderer& Renderer() = 0;
	};

	typedef size_t ID_VERTEX_SHADER;
	typedef size_t ID_PIXEL_SHADER;
	typedef size_t ID_MESH;

	typedef int64 ticks;

	struct IUltraClock
	{
		virtual ticks Hz() const = 0;			// Number of ticks per seconds
		virtual ticks TickStart() const = 0;	// The time of the current tick
		virtual ticks Start() const = 0;		// The time at which the mainloop started
		virtual ticks TickDelta() const = 0;	// The time between the previous tick and the current tick.
	};

	struct RGBA
	{
		float red;
		float green;
		float blue;
		float alpha;

		RGBA(float _r, float _g, float _b, float _a = 1.0f) : red(_r), green(_g), blue(_b), alpha(_a) {}
	};

	struct MouseEvent
	{
		uint16 flags;

		union {
			uint32 buttons;
			struct {
				uint16  buttonFlags;
				uint16  buttonData;
			};
		};

		uint32 ulRawButtons;
		int32 dx;
		int32 dy;
	};

	struct KeyboardEvent
	{
		uint16 scanCode;
		uint16 Flags;
		uint16 Reserved;
		uint16 VKey;
		uint32 Message;
	};

	struct ObjectInstance
	{
		Matrix4x4 orientation;
	};

	struct IRenderContext
	{
		virtual void Draw(ID_MESH id, const ObjectInstance* instance, uint32 nInstances) = 0;
		virtual IRenderer& Renderer() = 0;
	};

	struct IScene
	{
		virtual RGBA GetClearColour() const = 0;
		virtual void GetWorldMatrix(Matrix4x4& worldMatrix) const = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0;
	};

	struct IApp
	{
		virtual uint32 OnTick(const IUltraClock& clock) = 0;
		virtual void Free() = 0;
		virtual void OnCreated() = 0;
		virtual void OnKeyboardEvent(const KeyboardEvent& k) = 0;
		virtual void OnMouseEvent(const MouseEvent& me) = 0;
	};

	struct GuiMetrics
	{
		Vec2i cursorPosition;
		Vec2i screenSpan;
	};

	struct IRenderer
	{
		virtual ID_VERTEX_SHADER CreateGuiVertexShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength) = 0;
		virtual ID_VERTEX_SHADER CreateObjectVertexShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength) = 0;
		virtual ID_PIXEL_SHADER CreatePixelShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength) = 0;

		virtual void Render(IScene& scene) = 0;
		virtual void UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) = 0;
		virtual void GetGuiMetrics(GuiMetrics& metrics) const = 0;
		
		virtual ID_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices) = 0;
		virtual void UpdateMesh(ID_MESH rendererId, const ObjectVertex* vertices, uint32 nVertices) = 0;
	};

	struct IAppFactory
	{
		virtual IApp* CreateApp(IRenderer& renderer) = 0;
	};
}

#endif