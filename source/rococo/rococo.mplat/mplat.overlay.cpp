#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

static auto evFileUpdated = "OnFileUpdated"_event;


namespace Rococo
{
	namespace MPlatImpl
	{
		IPaneBuilderSupervisor* CreateScriptedPanel(Platform& platform, cstr filename, IScriptCompilationEventHandler* onCompile = nullptr);
	}
}

struct OverlayPane : public IPaneBuilderSupervisor, PaneDelegate, public IUIElement, public IObserver
{
	struct CancelButton : public IUIElement
	{
		Platform& platform;
		OverlayPane* overlay;
		CancelButton(Platform& _platform, OverlayPane* _overlay) :
			overlay(_overlay),
			platform(_platform)
		{
		}

		bool OnKeyboardEvent(const KeyboardEvent&) override
		{
			return false;
		}

		void OnRawMouseEvent(const MouseEvent&) override
		{

		}

		void OnMouseMove(Vec2i, Vec2i, int) override
		{

		}

		void OnMouseLClick(Vec2i, bool clickedDown) override
		{
			if (!clickedDown)
			{
				platform.misc.mathsVisitor.CancelSelect();
				overlay->type = OverlayPane::Type::None;
			}
		}

		void OnMouseRClick(Vec2i, bool clickedDown) override
		{
			if (!clickedDown)
			{
				platform.misc.mathsVisitor.CancelSelect();
				overlay->type = OverlayPane::Type::None;
			}
		}

		void Render(IGuiRenderContext&, const GuiRect&) override
		{

		}
	} textureCancel;

	AutoFree<IPaneBuilderSupervisor> tabbedPanel;
	AutoFree<IPaneBuilderSupervisor> txFocusPanel;
	Platform& platform;
	EventIdRef stnId = "selected.texture.name"_event;
	EventIdRef matClickedId = "overlay.select.material"_event;
	EventIdRef texClickedId = "overlay.select.texture"_event;
	EventIdRef meshClickedId = "overlay.select.mesh"_event;
	EventIdRef matDescClicked = "mplat.pane.textout.click"_event;

	enum class Type
	{
		None,
		Texture,
		Material
	} type = Type::None;

	std::string name;
	std::string value;

	OverlayPane(Platform& _platform) :
		platform(_platform),
		textureCancel(_platform, this),
		tabbedPanel(Rococo::MPlatImpl::CreateScriptedPanel(_platform, "!scripts/panel.overlay.sxy")),
		txFocusPanel(Rococo::MPlatImpl::CreateScriptedPanel(_platform, "!scripts/panel.texture.sxy"))
	{
		current = tabbedPanel;
		platform.graphics.gui.RegisterPopulator("texture_view", this);
		platform.graphics.gui.RegisterPopulator("texture_cancel", &textureCancel);
		platform.plumbing.publisher.Subscribe(this, stnId);
		platform.plumbing.publisher.Subscribe(this, matClickedId);
		platform.plumbing.publisher.Subscribe(this, texClickedId);
		platform.plumbing.publisher.Subscribe(this, meshClickedId);
		platform.plumbing.publisher.Subscribe(this, matDescClicked);
	}

	~OverlayPane()
	{
		platform.graphics.gui.UnregisterPopulator(&textureCancel);
		platform.graphics.gui.UnregisterPopulator(this);
		platform.plumbing.publisher.Unsubscribe(this);
	}

	void OnEvent(Event& ev) override
	{
		if (stnId == ev)
		{
			auto& toe = As<TextOutputEvent>(ev);
			if (toe.isGetting)
			{
				if (!name.empty())
				{
					SafeFormat(toe.text, sizeof(toe.text), " %s: %s", name.c_str(), value.c_str());
				}
				else
				{
					*toe.text = 0;
				}
			}
		}
		else if (matClickedId == ev)
		{
			auto& mc = As<VisitorItemClickedEvent>(ev);
			name = mc.key;
			value = mc.value;
			type = OverlayPane::Type::Material;
		}
		else if (texClickedId == ev)
		{
			auto& mc = As<VisitorItemClickedEvent>(ev);
			name = mc.key;
			value = mc.value;
			type = OverlayPane::Type::Texture;
		}
		else if (meshClickedId == ev)
		{
			auto& mc = As<VisitorItemClickedEvent>(ev);

			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(64_kilobytes);
			platform.graphics.meshes.SaveCSV(mc.key, *buffer);
			if (buffer->Length() > 0)
			{
				OS::SaveClipBoardText((cstr)buffer->GetData(), platform.os.mainWindow);
				platform.graphics.gui.LogMessage("Copied %s CSV to clipboard", mc.key);
			}
			type = OverlayPane::Type::None;
		}
		else if (matDescClicked == ev && !name.empty())
		{
			auto& args = As<TextOutputClickedEvent>(ev);
			if (Eq(args.key, "overlay.texture.desc"))
			{
				if (StartsWith(name.c_str(), "MatId "))
				{
					int index = atoi(name.c_str() + 6);
					MaterialId id = (MaterialId)index;
					cstr fullname = platform.graphics.renderer.Materials().GetMaterialTextureName(id);
					if (fullname != nullptr)
					{
						WideFilePath sysPath;
						platform.os.installation.ConvertPingPathToSysPath(fullname, sysPath);
						Rococo::OS::EditImageFile(platform.os.mainWindow, sysPath);
					}
				}
			}
		}
	}

	void Free() override
	{
		delete this;
	}

	bool OnKeyboardEvent(const KeyboardEvent&) override
	{
		return false;
	}

	void OnRawMouseEvent(const MouseEvent&) override
	{

	}

	void OnMouseMove(Vec2i, Vec2i, int) override
	{

	}

	void OnMouseLClick(Vec2i, bool) override
	{

	}

	void OnMouseRClick(Vec2i, bool) override
	{

	}

	void Render(IGuiRenderContext& rc, const GuiRect& absRect) override
	{
		if (type == OverlayPane::Type::Material && !name.empty())
		{
			cstr key = name.c_str();
			if (StartsWith(key, "MatId "))
			{
				int index = atoi(key + 6);
				MaterialId id = (MaterialId)index;
				Graphics::RenderBitmap_ShrinkAndPreserveAspectRatio(rc, id, absRect);
			}
		}
		if (type == OverlayPane::Type::Texture && name.size() > 4)
		{
			cstr key = name.c_str();
			int index = atoi(key + 5);

			ID_TEXTURE id(index);

			TextureDesc desc;
			if (!platform.graphics.renderer.Textures().TryGetTextureDesc(desc, id))
			{
				type = OverlayPane::Type::None;
				return;
			}

			SpriteVertexData ignore{ 0.0f, 0.0f, 0.0f, 0.0f };
			RGBAb none(0, 0, 0, 0);

			GuiVertex quad[6] =
			{
				{
					{ (float)absRect.left, (float)absRect.top },
					{ { 0, 0 }, 0 },
					ignore,
					none
				},
				{
					{ (float)absRect.right, (float)absRect.top },
					{ { 1, 0 }, 0 },
					ignore,
					none
				},
				{
					{ (float)absRect.right,(float)absRect.bottom },
					{ { 1, 1 }, 0 },
					ignore,
					none
				},
				{
					{ (float)absRect.right, (float)absRect.bottom },
					{ { 1, 1 }, 0 },
					ignore,
					none
				},
				{
					{ (float)absRect.left, (float)absRect.bottom },
					{ { 0, 1 }, 0 },
					ignore,
					none
				},
				{
					{ (float)absRect.left, (float)absRect.top },
					{ { 0, 0 }, 0 },
					ignore,
					none
				}
			};

			if (desc.format == TextureFormat_32_BIT_FLOAT)
			{
				rc.DrawCustomTexturedMesh(absRect, id, "!r32f.ps", quad, 6);
			}
			else if (desc.format == TextureFormat_RGBA_32_BIT)
			{
				rc.DrawCustomTexturedMesh(absRect, id, "!gui.texture.ps", quad, 6);
			}
			else
			{
				type = OverlayPane::Type::None;
			}
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		if (type != OverlayPane::Type::None)
		{
			current = txFocusPanel;
		}
		else
		{
			current = tabbedPanel;
		}

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		GuiRect fullScreen = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
		SetRect(fullScreen);

		return current->Render(grc, topLeft, modality);
	}

	void SetBkImage(const fstring&) override
	{
		Throw(0, "Not implemented");
	}

	IPaneSupervisor* Supervisor() override
	{
		return this;
	}

	Rococo::GUI::IPaneContainer* Root() override
	{
		return PaneDelegate::Root();
	}
};


namespace Rococo
{
	namespace MPlatImpl
	{
		IPaneBuilderSupervisor* CreateDebuggingOverlay(Platform& platform)
		{
			return new OverlayPane(platform);
		}
	}
}