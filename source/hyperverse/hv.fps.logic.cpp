#include "hv.h"
#include "hv.events.h"
#include <rococo.hashtable.h>
#include <rococo.maths.h>
#include <rococo.clock.h>
#include "../rococo.mplat/rococo.script.types.h"
#include <algorithm>
#include <rococo.textures.h>
#include <rococo.strings.h>
#include <rococo.time.h>

using namespace HV;
using namespace HV::Events;
using namespace HV::Events::Player;

using namespace Rococo::Entities;
using namespace Rococo::Graphics;

typedef std::unordered_map<ISector*, int32> TSectorSet;

const Textures::BitmapLocation nullBitmap { {0,0,0,0},0,{0,0} };

static const std::vector<const char*> wide_credits =
{
	"Hyperverse", "by", "Mark Anthony Taylor", "Copyright (c) 2020. All rights reserved",
	"Email: mark.anthony.taylor@gmail.com", "", "Press E to toggle editor"
};

static const std::vector<const char*> narrow_credits =
{
	"Hyperverse", "by", "Mark Anthony Taylor", "Copyright (c) 2020", "All rights reserved",
	"Email:","mark.anthony.taylor@gmail.com"
};

void Insert(TSectorSet& set, ISector* sector)
{
	set.insert(std::make_pair(sector, 0));
}

bool IsPresent(const TSectorSet& set, ISector* sector)
{
	auto i = set.find(sector);
	return i != set.end();
}

struct ICursorMonitor
{
	virtual void SetCursorObject(ID_OBJECT id) = 0;
};

class InformationPopulator : public IUIElement
{
	IUIElement& parent;
	bool enable = false;
	IObjectManager& objects;
	GuiRect lastRect;
	ID_FONT idFont;
	Platform& platform;
public:
	InformationPopulator(IObjectManager& refObjects, IUIElement& refParent, Platform& _platform) :
		objects(refObjects), parent(refParent), platform(_platform)
	{
	}

	void Enable(bool enable)
	{
		this->enable = enable;
	}

	bool OnKeyboardEvent(const KeyboardEvent& key) override
	{
		return false;
	}

	void OnRawMouseEvent(const MouseEvent& ev) override
	{
	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{
		parent.OnMouseMove(cursorPos, delta, dWheel);
	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		parent.OnMouseLClick(cursorPos, clickedDown);
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{
		parent.OnMouseRClick(cursorPos, clickedDown);
	}

	void RenderInfo(IGuiRenderContext& g, ID_OBJECT objectId)
	{
		RGBAb white(255, 255, 255, 255);
		RGBAb dull(0, 0, 0, 192);
		Graphics::DrawRectangle(g, lastRect, dull, dull);

		if (!idFont)
		{
			idFont = platform.plumbing.utilities.GetHQFonts().GetSysFont(Graphics::HQFont::InfoFont);
		}

		if (!objectId) return;

		auto obj = objects.GetObject(objectId);

		if (!obj.prototype) return;

		char desc[4096];
		FormatEquipmentInfo(desc, sizeof(desc), *obj.prototype);

		Graphics::RenderHQParagraph(g, idFont, lastRect, desc, white);

		Graphics::DrawBorderAround(g, lastRect, { 1,1 }, white, white);
	}

	void Render(IGuiRenderContext& g, const GuiRect& absRect) override
	{
		lastRect = absRect;
	}
};

class EyeGlassPopulator : public IUIElement, public ICursorMonitor
{
	IUIElement& parent;
	bool enable = false;
	ID_OBJECT cursorObjectId;
	IObjectManager& objects;
	GuiRect lastRect;
	InformationPopulator& infoPopulator;
public:
	EyeGlassPopulator(IObjectManager& refObjects, IUIElement& refParent, InformationPopulator& refInfoPopulator):
		objects(refObjects), parent(refParent), infoPopulator(refInfoPopulator)
	{

	}

	void SetCursorObject(ID_OBJECT id) override
	{
		cursorObjectId = id;
	}

	void Enable(bool enable)
	{
		this->enable = enable;
	}

	bool OnKeyboardEvent(const KeyboardEvent& key) override
	{
		return false;
	}

	void OnRawMouseEvent(const MouseEvent& ev) override
	{
	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{
		parent.OnMouseMove(cursorPos, delta, dWheel);
	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		parent.OnMouseLClick(cursorPos, clickedDown);
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{
		parent.OnMouseRClick(cursorPos, clickedDown);
	}

	void Render(IGuiRenderContext& g, const GuiRect& absRect) override
	{
		if (!enable) return;

		lastRect = absRect;

		GuiMetrics metrics;
		g.Renderer().GetGuiMetrics(metrics);

		if (cursorObjectId && IsPointInRect(metrics.cursorPosition, lastRect))
		{
			infoPopulator.RenderInfo(g, cursorObjectId);
		}
	}
};

void RenderItemBitmapMax(IGuiRenderContext& g, const Textures::BitmapLocation& bitmap, const Vec2& pos, Degrees rotationTheta)
{
	SpriteVertexData svd;
	svd.lerpBitmapToColour = 0.0f;
	svd.textureIndex = (float)bitmap.textureIndex;
	svd.textureToMatLerpFactor = 0.0f;
	svd.matIndex = 0;

	GuiRectf txUV = Dequantize(bitmap.txUV);

	GuiVertex a, b, c, d;
	a.colour = b.colour = c.colour = d.colour = RGBAb(0, 0, 0, 0);
	a.vd.fontBlend = b.vd.fontBlend = c.vd.fontBlend = d.vd.fontBlend = 0;
	a.vd.uv = { txUV.left, txUV.bottom };
	b.vd.uv = { txUV.left, txUV.top };
	c.vd.uv = { txUV.right, txUV.top };
	d.vd.uv = { txUV.right, txUV.bottom };
	a.sd = b.sd = c.sd = d.sd = svd;

	GuiRectf rect{ pos.x, pos.y, pos.x + Width(txUV), pos.y + Height(txUV) };
	// Horizontal items align to the top and then rotate 45 degrees
	a.pos = { rect.left, rect.bottom };
	b.pos = { rect.left, rect.top };
	c.pos = { rect.right, rect.top };
	d.pos = { rect.right, rect.bottom };

	Vec2 centreOfRotation = b.pos + Vec2{ 0.5f * Height(rect), 0.5f * Height(rect) };

	a.pos -= centreOfRotation;
	b.pos -= centreOfRotation;
	c.pos -= centreOfRotation;
	d.pos -= centreOfRotation;

	Matrix2x2 R = Matrix2x2::RotateAnticlockwise(rotationTheta);

	a.pos = R * a.pos;
	b.pos = R * b.pos;
	c.pos = R * c.pos;
	d.pos = R * d.pos;

	a.pos += centreOfRotation;
	b.pos += centreOfRotation;
	c.pos += centreOfRotation;
	d.pos += centreOfRotation;

	GuiTriangle t1, t2;
	t1.a = a;
	t1.b = b;
	t1.c = c;
	t2.a = c;
	t2.b = d;
	t2.c = a;
	g.AddTriangle(&t1.a);
	g.AddTriangle(&t2.a);
}

class InventoryPopulator : public IUIElement
{
	IInventoryArray& inventory;
	IObjectManager& objects;
	IUIElement& parent;
	ICursorMonitor& monitor;

	int32 selectedObjectIndex = -1;
	Textures::BitmapLocation cursorBitmap = nullBitmap;
	bool enable = false;

	void SetCursorToBitmap(const Textures::BitmapLocation& bitmap)
	{
		cursorBitmap = bitmap;
	}
public:
	InventoryPopulator(IInventoryArray& refInventory, IObjectManager& refObjects, IUIElement& refParent, ICursorMonitor& refMonitor):
		inventory(refInventory), objects(refObjects), parent(refParent), monitor(refMonitor)
	{
	}

	void Enable(bool enable)
	{
		this->enable = enable;
	}

	bool OnKeyboardEvent(const KeyboardEvent& key) override
	{
		return false;
	}

	void OnRawMouseEvent(const MouseEvent& ev) override
	{
	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{
		parent.OnMouseMove(cursorPos, delta, dWheel);
	}

	void Advance(const IUltraClock& clock)
	{
		if (unsheatheAngle > -25.0f)
		{
			unsheatheAngle -= 450.0f * clock.DT();
		}
		else
		{
			unsheatheAngle = -25.0f;
		}
	}

	float unsheatheAngle = 45.0f;

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		int32 index = inventory.GetIndexAt(cursorPos);
		if (!clickedDown && index >= 0)
		{
			if (selectedObjectIndex >= 0)
			{
				ID_OBJECT sourceItem{ (uint64)inventory.Id(selectedObjectIndex) };

				int64 targetFlags = inventory.Flags(index);
				auto ref = objects.GetObject(sourceItem);
				if (ref.prototype && (targetFlags == 0 || ref.prototype->CanFitSlot(targetFlags)))
				{
					inventory.Swap(index, selectedObjectIndex);
					selectedObjectIndex = -1;
					SetCursorToBitmap(nullBitmap);
					monitor.SetCursorObject(ID_OBJECT::Invalid());
				}
			}
			else
			{
				ID_OBJECT objId{ (uint64)inventory.Id(index) };
				auto obj = objects.GetObject(objId);
				if (obj.prototype != nullptr)
				{
					unsheatheAngle = 45.0f;
					selectedObjectIndex = index;
					SetCursorToBitmap(obj.prototype->Bitmap());
					monitor.SetCursorObject(objId);
				}
			}
		}
		else
		{
			parent.OnMouseLClick(cursorPos, clickedDown);
		}
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{
		parent.OnMouseRClick(cursorPos, clickedDown);
	}

	void RenderItemBitmap(IGuiRenderContext& g, const Textures::BitmapLocation& bitmap, const GuiRect& absRect)
	{
		if (bitmap.pixelSpan.x == bitmap.pixelSpan.y)
		{
			// Square bitmaps stretch to fit
			Graphics::StretchBitmap(g, bitmap, absRect);
			return;
		}

		SpriteVertexData svd;
		svd.lerpBitmapToColour = 0.0f;
		svd.textureIndex = (float)bitmap.textureIndex;
		svd.textureToMatLerpFactor = 0.0f;
		svd.matIndex = 0;

		GuiRectf txUV = Dequantize(bitmap.txUV);

		GuiVertex a, b, c, d;
		a.colour = b.colour = c.colour = d.colour = RGBAb(0, 0, 0, 0);
		a.vd.fontBlend = b.vd.fontBlend = c.vd.fontBlend = d.vd.fontBlend = 0;
		a.vd.uv = { txUV.left, txUV.bottom };
		b.vd.uv = { txUV.left, txUV.top };
		c.vd.uv = { txUV.right, txUV.top };
		d.vd.uv = { txUV.right, txUV.bottom };
		a.sd = b.sd = c.sd = d.sd = svd;

		if (bitmap.pixelSpan.x > bitmap.pixelSpan.y)
		{
			GuiRectf rect = Dequantize(absRect);
			float deltaY = bitmap.pixelSpan.y * Height(rect) / bitmap.pixelSpan.x;
			// Horizontal items align to the top and then rotate 45 degrees
			a.pos = { rect.left, rect.top + deltaY };
			b.pos = { rect.left, rect.top};
			c.pos = { rect.right, rect.top};
			d.pos = { rect.right, rect.top + deltaY };

			Vec2 centreOfRotation = b.pos + Vec2{ 0.5f * deltaY, 0.5f * deltaY };

			a.pos -= centreOfRotation;
			b.pos -= centreOfRotation;
			c.pos -= centreOfRotation;
			d.pos -= centreOfRotation;

			Matrix2x2 R = Matrix2x2::RotateAnticlockwise(45_degrees);

			a.pos = R * a.pos;
			b.pos = R * b.pos;
			c.pos = R * c.pos;
			d.pos = R * d.pos;

			a.pos += centreOfRotation;
			b.pos += centreOfRotation;
			c.pos += centreOfRotation;
			d.pos += centreOfRotation;

			float L = Width(rect);
			float dL = L * (1.0f - 0.5f * sqrtf(2)) * 0.25f;

			a.pos += Vec2{ dL, dL };
			b.pos += Vec2{ dL, dL };
			c.pos += Vec2{ dL, dL };
			d.pos += Vec2{ dL, dL };

			GuiTriangle t1, t2;
			t1.a = a;
			t1.b = b;
			t1.c = c;
			t2.a = c;
			t2.b = d;
			t2.c = a;
			g.AddTriangle(&t1.a);
			g.AddTriangle(&t2.a);
		}
	}

	void RenderInventory(IGuiRenderContext& g, const GuiRect& absRect)
	{
		GuiMetrics metrics;
		g.Renderer().GetGuiMetrics(metrics);

		int32 nDolls = inventory.DollCount();

		for (int32 i = 0; i < nDolls; ++i)
		{
			GuiRect rect;

			struct : IStringPopulator
			{
				U8FilePath pingPath;

				void Populate(cstr text)override
				{
					Format(pingPath, "%s", text);
				}
			} pop;

			Textures::BitmapLocation bitmap;
			inventory.GetDoll(i, rect, pop, bitmap);

			if (bitmap.txUV.left == bitmap.txUV.right)
			{
				if (!g.Gui().SpriteBuilder().TryGetBitmapLocation(pop.pingPath, bitmap))
				{
					Throw(0, "The ping path [%s] for inventory doll %d did not correspond to an image", pop.pingPath.buf, i);
				}
				else
				{
					inventory.SetDollBitmap(i, bitmap);
				}
			}
			else
			{
				Graphics::StretchBitmap(g, bitmap, rect);
			}
		}

		int32 nItems = inventory.NumberOfItems();

		for (int32 i = 0; i < nItems; ++i)
		{
			GuiRect rect;
			inventory.GetRect(i, OUT rect);

			bool isLit = IsPointInRect(metrics.cursorPosition, rect);

			ID_OBJECT id{ (uint64)inventory.Id(i) };
			ObjectRef obj = objects.GetObject(id);

			if (obj.prototype != nullptr)
			{
				RGBAb black = RGBAb(0, 0, 0, 192);
				Graphics::DrawRectangle(g, rect, black, black);
				RenderItemBitmap(g, obj.prototype->Bitmap(), rect);
			}
			else
			{
				RGBAb t1 = isLit ? RGBAb(255, 0, 0, 64) : RGBAb(192, 0, 0, 32);
				RGBAb t2 = isLit ? RGBAb(0, 0, 224, 64) : RGBAb(0, 0, 160, 32);
				Graphics::DrawRectangle(g, rect, t1, t2);
			}

			RGBAb diag1 = isLit ? RGBAb(255, 255, 255, 255) : RGBAb(192, 192, 192, 255);
			RGBAb diag2 = isLit ? RGBAb(224, 224, 224, 224) : RGBAb(160, 160, 160, 255);
			Graphics::DrawBorderAround(g, rect, { 1,1 }, diag1, diag2);
		}
	}

	void Render(IGuiRenderContext& g, const GuiRect& absRect) override
	{
		if (enable) RenderInventory(g, absRect);

		if (selectedObjectIndex >= 0 && cursorBitmap.txUV.left != cursorBitmap.txUV.right)
		{
			GuiMetrics metrics;
			g.Renderer().GetGuiMetrics(metrics);

			int64 delta = Rococo::Time::TickCount() % Rococo::Time::TickHz();
			float t = delta / (float) Rococo::Time::TickHz();
			float s = sinf(2.0f * 3.14159f * t);
			Degrees sr{ unsheatheAngle + 1.0f * s };

			RenderItemBitmapMax(g, cursorBitmap, Dequantize(metrics.cursorPosition), sr);
		}
	}
};

class FPSControl
{
private:
   void OnForward(bool start)
   {
      isMovingForward = start;
      isAutoRun = false;
   }

   void OnBackward(bool start)
   {
      isMovingBackward = start;
      isAutoRun = false;
   }


   void OnStraffeLeft(bool start)
   {
      isMovingLeft = start;
   }


   void OnStraffeRight(bool start)
   {
      isMovingRight = start;
   }

   void OnJump(bool start)
   {

   }

   void OnAutoRun(bool start)
   {
      isAutoRun = true;
   }

public:
   typedef void (FPSControl::*ACTION_FUNCTION)(bool start);
   static stringmap<ACTION_FUNCTION> nameToAction;

   bool isMovingForward{ false };
   bool isMovingBackward{ false };
   bool isMovingLeft{ false };
   bool isMovingRight{ false };
   bool isAutoRun{ false };

   float headingDelta{ 0 };
   float elevationDelta{ 0 };

   Vec3 speeds{ 0,0,0 };

   FPSControl()
   {
   }

   void Clear()
   {
      isMovingBackward = isMovingForward = isMovingLeft = isMovingRight = false;
      headingDelta = elevationDelta = 0;
   }

   bool Append(PlayerActionEvent& ev)
   {
      auto i = nameToAction.find(ev.Name);
      if (i != nameToAction.end())
      {
         ((*this).*(i->second))(ev.start);
         return true;
      }
      else
      {
         return false;
      }
   }

   void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
   {
      headingDelta += 0.25f * delta.x;
      elevationDelta -= delta.y;
   }

   void GetPlayerDelta(ID_ENTITY playerId, float dt, Entities::MoveMobileArgs& mm, Degrees& viewElevationDelta)
   {
      float forwardDelta = 0;
      if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
      if (isMovingBackward) forwardDelta -= 1.0f;

      float straffeDelta = 0;
      if (isMovingLeft) straffeDelta -= 1.0f;
      if (isMovingRight) straffeDelta += 1.0f;

      if (forwardDelta != 0 || straffeDelta != 0 || headingDelta != 0)
      {
         mm.fowardDelta = dt * ((forwardDelta > 0) ? forwardDelta * speeds.x : forwardDelta * speeds.z);
         mm.straffeDelta = dt * straffeDelta * speeds.y;
         mm.entityId = playerId;
         mm.delta = { Degrees{ headingDelta }, Degrees{ 0 }, Degrees{ 0 } };
         headingDelta = 0;
      }

      viewElevationDelta = Degrees{ -elevationDelta };
      elevationDelta = 0;
   }
};

stringmap<FPSControl::ACTION_FUNCTION> FPSControl::nameToAction =
{
   { "move.fps.forward",         &FPSControl::OnForward },
   { "move.fps.backward",        &FPSControl::OnBackward },
   { "move.fps.straffeleft",     &FPSControl::OnStraffeLeft },
   { "move.fps.strafferight",    &FPSControl::OnStraffeRight },
   { "move.fps.jump",            &FPSControl::OnJump },
   { "move.fps.autorun",         &FPSControl::OnAutoRun }
};

struct FPSGameLogic : public IFPSGameModeSupervisor, public IUIElement, public IScenePopulator, public IPropertyTarget
{
	Platform& platform;
	IPlayerSupervisor& players;
	ISectors& sectors;
	FPSControl fpsControl;
	bool isCursorActive = true;
	RGBAb ambientLight = RGBAb(10,10,10,255);
	float fogConstant = -0.1f;
	IPropertyHost* host = nullptr;
	IObjectManager& objects;
	InformationPopulator infoPopulator;
	EyeGlassPopulator eyeGlassPopulator;
	InventoryPopulator inventoryPopulator;

	void Assign(IPropertyHost* host) override
	{
		this->host = host;
	}

	void GetProperties(cstr category, IBloodyPropertySetEditor& editor)  override
	{
		if (Eq(category, "Ambient"))
		{
			editor.AddColour("Ambient light", &ambientLight);
			editor.AddFloat("Fog constant", &fogConstant, -0.2f, -0.01f);
		}
	}

	void NotifyChanged(BloodyNotifyArgs& args)  override
	{

	}

	IPropertyTarget* GetPropertyTarget()
	{
		return this;
	}

	FPSGameLogic(Platform& _platform, IPlayerSupervisor& _players, ISectors& _sectors, IObjectManager& _objects) :
		platform(_platform), players(_players), sectors(_sectors), objects(_objects), 
		infoPopulator(objects, *this, _platform),
		eyeGlassPopulator(objects, *this, infoPopulator),
		inventoryPopulator(*players.GetPlayer(0)->GetInventory(), objects, *this, eyeGlassPopulator)
	{
		fpsControl.speeds = Vec3{ 10.0f, 5.0f, 5.0f };
		platform.graphics.scene.SetPopulator(this);

		platform.graphics.gui.RegisterPopulator("fps", this);
		platform.graphics.gui.RegisterPopulator("fps.information", &infoPopulator);
		platform.graphics.gui.RegisterPopulator("fps.inventory", &inventoryPopulator);
		platform.graphics.gui.RegisterPopulator("fps.eye_glass", &eyeGlassPopulator);

	}

	~FPSGameLogic()
	{
		platform.graphics.gui.UnregisterPopulator(this);
		platform.graphics.scene.SetPopulator(nullptr);
	}

	void Activate()
	{
		isCursorActive = false;
		platform.graphics.renderer.SetCursorVisibility(false);
		fpsControl.Clear();

		// Assume the editor has invalidates the tags, so flag to rebuild
		sectors.Tags().Invalidate();
	}

	void Deactivate()
	{
		isCursorActive = true;
		platform.graphics.renderer.SetCursorVisibility(true);
		fpsControl.Clear();
	}

	bool Append(PlayerActionEvent& ev)
	{
		return fpsControl.Append(ev);
	}

	void Free() override
	{
		delete this;
	}

	struct SectorToScene : public IEventCallback<VisibleSector>, public IEventCallback<const ID_ENTITY>
	{
		ISceneBuilder& builder;
		std::vector<ISector*>& foundSectors;

		virtual void OnEvent(const ID_ENTITY& id)
		{
			builder.AddStatics(id);
		}

		virtual void OnEvent(VisibleSector& sv)
		{
			sv.sector.ForEveryObjectInSector(*this);
			foundSectors.push_back(&sv.sector);
		}

		SectorToScene(ISceneBuilder& _builder, std::vector<ISector*>& _foundSectors) :
			builder(_builder), foundSectors(_foundSectors) {}
	};

	typedef std::vector<ISector*> TSectorVector;

	TSectorVector illuminatedRooms;
	TSectorVector shadowCasterSectors;

	void PopulateShadowCasters(ISceneBuilder& sb, const DepthRenderData& drd)  override
	{
		sb.Clear();
		shadowCasterSectors.clear();
		SectorToScene addToScene(sb, shadowCasterSectors);
		auto nSectors = sectors.ForEverySectorVisibleBy(drd.worldToScreen, drd.eye, drd.direction, addToScene);
		if (nSectors == 0)
		{
			// Nothing rendered
		}
	}

	TSectorSet visibleSectorsThisTimestep;

	void ComputeVisibleSectorsThisTimestep()
	{
		visibleSectorsThisTimestep.clear();

		Vec3 eye;
		platform.graphics.camera.GetPosition(eye);

		Matrix4x4 world;
		platform.graphics.camera.GetWorld(world);

		Matrix4x4 camera;
		platform.graphics.camera.GetWorldAndProj(camera);

		Vec3 dir{ -world.row2.x, -world.row2.y, -world.row2.z };

		struct : IEventCallback<VisibleSector>
		{
			TSectorSet* visibleSectorsThisTimestep;
			ISector* home{ nullptr };

			void OnEvent(VisibleSector& v) override
			{
				if (!home) home = &v.sector;
				Insert(*visibleSectorsThisTimestep, &v.sector);
			}
		} builder;

		builder.visibleSectorsThisTimestep = &visibleSectorsThisTimestep;

		auto nSectors = sectors.ForEverySectorVisibleBy(camera, eye, dir, builder);
		if (nSectors == 0)
		{
			// Nothing rendered
		}

		if (builder.home)
		{
			builder.home->SyncEnvironmentMapToSector();
		}
	}

	TSectorVector visibleSectors;

	void PopulateScene(ISceneBuilder& sb) override
	{
		sb.Clear();
		visibleSectors.clear();

		SectorToScene addToScene(sb, visibleSectors);
		for (auto s : visibleSectorsThisTimestep)
		{
			addToScene.OnEvent(VisibleSector{ *s.first });
		}

		platform.graphics.renderer.Particles().ClearPlasma();
		platform.graphics.renderer.Particles().ClearFog();

		struct :IEventCallback<const ID_ENTITY>
		{
			Platform* platform;

			void OnEvent(const ID_ENTITY& id) override
			{
				platform->world.particles.GetParticles(id, platform->graphics.renderer);
			}
		} addParticles;

		addParticles.platform = &platform;

		if (!visibleSectors.empty())
		{
			// Add particles in curent sector and neighbouring visible sectors

			auto& s0 = *visibleSectors[0];
			s0.ForEveryObjectInSector(addParticles);

			size_t gapCount;
			auto gaps = s0.Gaps(gapCount);
			for (size_t i = 0; i < gapCount; ++i)
			{
				auto j = std::find(visibleSectors.begin(), visibleSectors.end(), gaps->other);
				if (j != visibleSectors.end())
				{
					(*j)->ForEveryObjectInSector(addParticles);
				}
			}
		}
	}

	bool IsPathAcrossGap(cr_vec3 start, cr_vec3 end, ISector& from, ISector& to)
	{
		size_t count;
		auto gaps = from.Gaps(count);

		for (size_t i = 0; i < count; ++i)
		{
			auto& gap = gaps[i];
			if (gap.other == &to)
			{
				Edge edge{ ToVec3(gap.a, 0), ToVec3(gap.b, 0) };
				Sphere playerSphere{ Flatten(start), 0.9_metres };
				auto c = Rococo::CollideEdgeAndSphere(edge, playerSphere, Flatten(end));

				if (c.contactType == ContactType_Edge || c.contactType == ContactType_Penetration)
				{
					return true;
				}
			}
		}

		return false;
	}

	struct CollisionParameters
	{
		Vec3 start;
		Vec3 end;
		ID_ENTITY playerId;
		float dt;
		float jumpSpeed;
	};

	Vec3 ToVec3(cr_vec2 p, float z)
	{
		return Vec3{ p.x, p.y, z };
	}

	Vec3 Flatten(cr_vec3 p)
	{
		return Vec3{ p.x, p.y, 0 };
	}

	Vec3 ApplyElasticCollision(Vec3 normal, const CollisionParameters& cp, float t, cr_vec3 touchPoint, const Sphere& sphere)
	{
		// N.B normal must oppose our direction
		Vec3 trajectory = cp.end - cp.start;

		if (Dot(normal, trajectory) > 0)
		{
			normal = -normal;
		}

		// m is mass of sphere.
		// Incoming velocity v is (end - start)
		// Outgoing velocity V is (Vx Vy)

		// For elastic collision, |v| = |V|, v.v = V.V

		// Momentum impulse = m.J.normal where J is the scalar magnitude of the impulse

		// mV = mv + m.J.normal

		// V = v + J.normal
		// V.V = v.v + J.J + 2.v.normal.J. Now consider v.v - V.V = 0:
		// J.J = -2.J.(v.normal)
		// J = -2.v.normal

		float J = -2.0f * Dot(trajectory, normal);

		t = 0.5f; // That should stop feedback

		Vec3 bounceTrajectory = trajectory + J * normal;
		Vec3 delta = (t * trajectory) + (1.0f - t) * bounceTrajectory;
		Vec3 bounceTo = cp.start + delta;
		return bounceTo;
	}

	Vec3 ComputeWallCollision(const CollisionParameters& cp, cr_vec2 p, cr_vec2 q, float& t)
	{
		Edge edge{ ToVec3(p, 0), ToVec3(q, 0) };
		Sphere playerSphere{ Flatten(cp.start), 0.9_metres };
		auto c = Rococo::CollideEdgeAndSphere(edge, playerSphere, Flatten(cp.end));

		if (c.contactType == ContactType_Penetration)
		{
			// Tangled in the mesh
			Vec3 displacement = c.touchPoint - Flatten(cp.start);

			// We could be leaving a line behind (safe), or we have further penetration (bad)
			if (Dot(displacement, cp.end - cp.start) > 0)
			{
				// Bad, we are making penetration worse
				t = 0;
				return cp.start;
			}
			else
			{
				// Leaving point behind
				t = 1.0f;
				return cp.end;
			}

			return cp.start;
		}

		if (c.contactType == ContactType_None)
		{
			t = 1.0f;
			return cp.end;
		}

		t = c.t;

		if (c.t > 1)
		{
			t = 1.0f;
			return cp.end;
		}

		if (c.contactType == ContactType_Edge) // see diagrams/collision.sphere.vs.edge.png
		{
			/*  Collision normal = | (q -p) *  K | */
			Vec3 pq = ToVec3(q - p, 0);

			Vec3 N = Cross(pq, Vec3{ 0, 0, 1 });

			float modN = Length(N);
			if (modN < 0.01f)
			{
				if (Rococo::OS::IsDebugging())
				{
					// Fix the geometry
					Rococo::OS::TripDebugger();
				}

				t = 1.1f;
				return cp.end;
			}

			Vec3 normal = N * (1.0f / modN);

			return ApplyElasticCollision(normal, cp, c.t, c.touchPoint, playerSphere);
		}
		else if (c.contactType == ContactType_Vertex)
		{
			/*   Collision normal = | (C - p)|  */
			Vec3 C = Flatten(c.t * (cp.end - cp.start) + cp.start);
			Vec3 N = C - c.touchPoint;

			float modN = Length(N);
			if (modN < 0.01f)
			{
				// Degenerate case, stop collision
				return cp.start;
			}

			if (Dot(N, cp.end - cp.start) < 0)
			{
				Vec3 normal = N * (1.0f / modN);
				t = c.t;
				return ApplyElasticCollision(normal, cp, c.t, c.touchPoint, playerSphere);
			}
			else
			{
				t = 1.0f;
				return cp.end;
			}
		}
		else
		{
			t = 0;
			// Degenerate case, stop collision
			return cp.start;
		}

		t = 1.0f;
		return cp.end;
	}

	Vec3 ComputeWallCollisionInSector(const CollisionParameters& cp, ISector& sector, bool scanGaps = true)
	{
		if (cp.start == cp.end) return cp.end;

		size_t count;
		auto segments = sector.GetWallSegments(count);

		size_t nVertices;
		auto v = sector.WallVertices(nVertices);

		float collisionTime = 1.0f;
		Vec3 destinationPoint = cp.end;

		for (size_t i = 0; i < count; ++i)
		{
			auto& s = segments[i];
			s.perimeterIndexStart;
			s.perimeterIndexEnd;

			auto p = v[s.perimeterIndexStart];
			auto q = v[s.perimeterIndexEnd];

			float ithTime = 1.0f;
			Vec3 ithResult = ComputeWallCollision(cp, p, q, ithTime);

			if (ithTime < collisionTime)
			{
				collisionTime = ithTime;
				destinationPoint = ithResult;
			}
		}

		size_t barrierCount;
		auto b = sector.Barriers(barrierCount);
		for (size_t i = 0; i < barrierCount; ++i)
		{
			float ithTime = 1.0f;
			Vec3 ithResult = ComputeWallCollision(cp, b[i].p, b[i].q, ithTime);

			if (ithTime < collisionTime)
			{
				collisionTime = ithTime;
				destinationPoint = ithResult;
			}
		}

		if (scanGaps)
		{
			auto* gaps = sector.Gaps(count);
			for (size_t i = 0; i < count; ++i)
			{
				auto& g = gaps[i];

				float ithTime = 1.0f;
				Vec3 ithResult = ComputeWallCollision(cp, g.a, g.b, ithTime);
				if (ithTime < collisionTime)
				{
					// A gap collision means player bounding sphere has entered a neighbouring sector
					// So we must check that we have not interfaced with materials in that sector

					Vec3 foreignTarget = ComputeWallCollisionInSector(cp, *g.other, false);
					if (foreignTarget != cp.end)
					{
						// Stop the movement dead
						return cp.start;
					}

					if (!IsAscendable(cp.start, cp.end, sector, *g.other))
					{
						collisionTime = ithTime;
						destinationPoint = ithResult;
					}
				}
			}
		}

		return destinationPoint;
	}

	bool IsAscendable(cr_vec3 start, cr_vec3 end, ISector& from, ISector& to)
	{
		if (from.IsCorridor() || to.IsCorridor())
		{
			// Corridors are asendable at both ends.
			return true;
		}

		float ground = end.z;

		float z0 = to.Z0();
		float z1 = to.Z1();

		const float maxClimbableHeight = 1.65f;

		if ((ground > z0 - maxClimbableHeight) && end.z < (z1 - 2.48_metres))
		{
			return true;
		}

		return false;
	}

	Vec3 ComputeFloorCollisionInSector(const CollisionParameters& cp, ISector& sector, float& jumpSpeed)
	{
		float h = GetHeightAtPointInSector(cp.end, sector);
		if (cp.end.z < h)
		{
			jumpSpeed = 0.0f;
			return Vec3{ cp.end.x, cp.end.y, h };
		}
		else if (cp.end.z > h)
		{
			Vec3 g = { 0, 0, -9.81f };
			Vec3 afterGravity = cp.end + (0.5f * cp.dt * cp.dt * g) + (cp.dt * Vec3{ 0, 0, cp.jumpSpeed });

			if (afterGravity.z < h)
			{
				jumpSpeed = 0.0f;
				return Vec3{ cp.end.x, cp.end.y, h };
			}
			else
			{
				jumpSpeed = cp.jumpSpeed + g.z * cp.dt;
				return afterGravity;
			}
		}

		jumpSpeed = cp.jumpSpeed;
		return cp.end;
	}

	Vec3 GetRandomPointInSector(ISector& sector)
	{
		auto f = sector.FloorVertices();

		size_t nTriangles = f.VertexCount / 3;

		size_t index = rand() % nTriangles;

		auto p1 = f.v[3 * index].position;
		auto p2 = f.v[3 * index + 1].position;
		auto p3 = f.v[3 * index + 2].position;

		float t = 0.5f * (rand() / (float)RAND_MAX) + 0.25f; // Gives random between 0.25 and 0.75

		Vec3 mid1_2 = 0.5f * (p1 + p2);
		Vec3 mid = Lerp(mid1_2, p3, t);
		return mid;
	}

	Vec3 CorrectPosition(const CollisionParameters& cp, float& jumpSpeed)
	{
		jumpSpeed = cp.jumpSpeed;

		auto* fromSector = GetFirstSectorContainingPoint({ cp.start.x, cp.start.y }, sectors);
		if (!fromSector)
		{
			// Generally it is a bad thing for the start point to be outside all sectors
			// We must have made a mistake setting up that position
			// First sector is usually the entrance, so port to there
			if (sectors.begin() != sectors.end())
			{
				ISector* firstSector = *sectors.begin();
				if (firstSector->FloorVertices().VertexCount)
				{
					Vec3 p = GetRandomPointInSector(*firstSector);
					CollisionParameters cp2 = { p, p, cp.playerId, cp.dt, cp.jumpSpeed };
					return ComputeFloorCollisionInSector(cp2, *firstSector, jumpSpeed);
				}
				else
				{
					return cp.start;
				}
			}
			else
			{
				return cp.start;
			}
		}

		auto* toSector = GetFirstSectorContainingPoint({ cp.end.x, cp.end.y }, sectors);
		if (!toSector)
		{
			// Totally disallow any movement that would take us out of the sector cosmology
			return cp.start;
		}

		if (fromSector != toSector)
		{
			// Sector transition. We are only allowed to transition through a gap that connects sectors

			auto result = ComputeWallCollisionInSector(cp, *fromSector);
			if (result != cp.end)
			{
				// If we are transitioning but we first collide with walls in the sector from which we leave, then stop things short
				return cp.start;
			}

			result = ComputeWallCollisionInSector(cp, *toSector);
			if (result != cp.end)
			{
				// If we are transitioning but we collide with walls in the sector to from we head, then stop things short
				return  cp.start;
			}

			// Okay no collisions, but we still have to be moving through a gap
			if (!IsPathAcrossGap(cp.start, cp.end, *fromSector, *toSector))
			{
				return cp.start;
			}

			if (!IsAscendable(cp.start, cp.end, *fromSector, *toSector))
			{
				return cp.start;
			}

			Vec3 finalPos = ComputeFloorCollisionInSector(cp, *toSector, jumpSpeed);
			return finalPos;
		}
		else
		{
			auto result = ComputeWallCollisionInSector(cp, *fromSector);

			result.z = cp.start.z;
			CollisionParameters cp2 = { cp.start, result, cp.playerId, cp.dt, cp.jumpSpeed };
			return ComputeFloorCollisionInSector(cp2, *fromSector, jumpSpeed);
		}
	}

	bool IsLightVisible(const LightSpec& light)
	{
		illuminatedRooms.clear();

		struct : IEventCallback<VisibleSector>
		{
			TSectorVector* illuminatedRooms;
			void OnEvent(VisibleSector& v)
			{
				illuminatedRooms->push_back(&v.sector);
			}
		} build;

		build.illuminatedRooms = &illuminatedRooms;

		Vec3 normDirection;
		if (!TryNormalize(light.direction, normDirection))
		{
			return false;
		}

		Matrix4x4 directionToCameraRot = RotateDirectionToNegZ(normDirection);
		Matrix4x4 cameraToDirectionRot = TransposeMatrix(directionToCameraRot);	
		Matrix4x4 worldToCamera = directionToCameraRot * Matrix4x4::Translate(-light.position);

		Matrix4x4 cameraToScreen = Matrix4x4::GetRHProjectionMatrix(light.fov, 1.0f, light.nearPlane, light.farPlane);

		Matrix4x4 worldToScreen = cameraToScreen * worldToCamera;

		sectors.ForEverySectorVisibleBy(worldToScreen, light.position, normDirection, build);

		bool foundOne = false;

		for (auto s : illuminatedRooms)
		{
			if (IsPresent(visibleSectorsThisTimestep, s))
			{
				return true;
			}
		}

		return false;
	}

	void UpdatePlayer(const IUltraClock& clock)
	{
		float dt = clock.DT();

		auto* player = players.GetPlayer(0);
		auto id = player->GetPlayerEntity();

		Degrees viewElevationDelta;

		Entities::MoveMobileArgs mm;
		fpsControl.GetPlayerDelta(id, dt, mm, viewElevationDelta);

		mm.fowardDelta *= Sq(player->DuckFactor());
		mm.straffeDelta *= Sq(player->DuckFactor());

		auto pe = platform.graphics.instances.ECS().GetBodyComponent(id);
		if (!pe)
		{
			Throw(0, "Expecting player entity");
		}

		Vec3 before = pe->Model().GetPosition();

		platform.world.mobiles.TryMoveMobile(mm);

		Vec3 after = pe->Model().GetPosition();

		CollisionParameters cp{ before, after, id, dt, player->JumpSpeed() };
		Vec3 final = CorrectPosition(cp, player->JumpSpeed());

		float dz = final.z - before.z;

		if (dz < 0)
		{
			// Going down
			player->DuckFactor() = 1.0f;
		}
		else if (dz == 0)
		{

		}
		else
		{
			player->DuckFactor() = 1.0f - dz / player->Height();
			if (player->DuckFactor() < 0.5f)
			{
				player->DuckFactor() = 0.5f;
			}
		}

		Matrix4x4 model = pe->Model();
		model.SetPosition(final);
		pe->SetModel(model);

		Vec3 playerPosToCamera = Vec3{ 0, 0, player->Height() * player->DuckFactor() };

		if (player->DuckFactor() < 1.0f)
		{
			player->DuckFactor() += 0.25f * dt;

			if (player->DuckFactor() > 1.0f)
			{
				player->DuckFactor() = 1.0f;
			}
		}

		Vec3 playerPosToLight= Vec3{ 0.2f, 0, player->Height() * player->DuckFactor() - 0.75f };

		FPSAngles angles;
		platform.world.mobiles.GetAngles(id, angles);

		auto Rz = Matrix4x4::RotateRHAnticlockwiseZ(angles.heading);

		Vec3 playerPosToLightWorld;
		TransformPositions(&playerPosToLight, 1, Rz, &playerPosToLightWorld);

		platform.graphics.camera.ElevateView(id, viewElevationDelta, playerPosToCamera);

		Matrix4x4 m;
		platform.graphics.camera.GetWorld(m);
		Vec3 dir{ -m.row2.x, -m.row2.y, -m.row2.z };

		LightSpec light;

		float red = ambientLight.red / 255.0f;
		float green = ambientLight.green / 255.0f;
		float blue = ambientLight.blue / 255.0f;

		light.ambience = RGBA(red, green, blue, 1.0f);
		light.diffuse = RGBA(4.0f, 4.0f, 4.0f, 1.0f);
		light.direction = dir;
		light.position = final + playerPosToLightWorld;
		light.cutoffPower = 32.0f;
		light.cutoffAngle = 30_degrees;
		light.fov = 90_degrees;
		light.attenuation = -0.35f;
		light.nearPlane = 0.02_metres;
		light.farPlane = 50_metres;
		light.fogConstant = fogConstant;

		lightBuilder.push_back(light);

		Vec3 eye;
		platform.graphics.camera.GetPosition(eye);

		Matrix4x4 world;
		platform.graphics.camera.GetWorld(world);

		Matrix4x4 camera;
		platform.graphics.camera.GetWorldAndProj(camera);

		struct : IEventCallback<VisibleSector>
		{
			FPSGameLogic* This;

			void OnEvent(VisibleSector& v) override
			{
				size_t nLights;
				auto sl = v.sector.Lights(nLights);
				for (size_t i = 0; i < nLights; ++i)
				{
					This->lightBuilder.push_back(sl[i]);
				}
			}
		} addLightsFromeSector;

		addLightsFromeSector.This = this;

		auto nSectors = sectors.ForEverySectorVisibleBy(camera, eye, dir, addLightsFromeSector);
		if (nSectors == 0)
		{
			// Nothing rendered
		}

		struct
		{
			bool operator ()(const LightSpec& a, const LightSpec& b) const
			{
				return LengthSq(a.position - eye) < LengthSq(b.position - eye);
			}

			Vec3 eye;
		} byDistanceFromPlayer{ final };

		std::sort(lightBuilder.begin(), lightBuilder.end(), byDistanceFromPlayer);

		platform.graphics.scene.Builder().ClearLights();

		int32 targetIndex = 0;

		for (int32 i = 0; i < lightBuilder.size(); ++i)
		{
			if (IsLightVisible(lightBuilder[i]))
			{
				platform.graphics.scene.Builder().SetLight(lightBuilder[i], targetIndex++);
			}
		}

		lightBuilder.clear();

		auto* homeSector = GetFirstSectorContainingPoint(Vec2{ eye.x, eye.y }, sectors );
		if (homeSector)
		{
			homeSector->Contents().NotifySectorPlayerIsInSector(clock);
		}
	}

	std::vector<LightSpec> lightBuilder;

	float spinningLightTheta = 0;

	void UpdateAI(const IUltraClock& clock) override
	{
		UpdatePlayer(clock);
		ComputeVisibleSectorsThisTimestep();

		inventoryPopulator.Advance(clock);
	}

	bool overlayInventory = false;

	bool OnKeyboardEvent(const KeyboardEvent& k)
	{
		Key key = platform.hardware.keyboard.GetKeyFromEvent(k);
		auto* action = platform.hardware.keyboard.GetAction(key.KeyName);
		if (action)
		{
			if (!key.isPressed && Eq(action, "gui.editor.toggle"))
			{
				TEventArgs<bool> enable;
				enable.value = true;
				platform.plumbing.publisher.Publish(enable, evEnableEditor);
				return true;
			}

			if (!key.isPressed && Eq(action, "gui.inventory.toggle"))
			{
				overlayInventory = !overlayInventory;
				inventoryPopulator.Enable(overlayInventory);
				eyeGlassPopulator.Enable(overlayInventory);
				infoPopulator.Enable(overlayInventory);	
			}

			HV::Events::Player::PlayerActionEvent pae;
			pae.Name = action;
			pae.start = key.isPressed;
			return Append(pae);
		}

		return false;
	}

	void OnRawMouseEvent(const MouseEvent& me) override
	{

	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
	{
		auto* player = players.GetPlayer(0);
		if (!isCursorActive) fpsControl.OnMouseMove(cursorPos, delta, dWheel);
	}

	void UseAnythingAtCrosshair()
	{
		Vec3 eye;
		platform.graphics.camera.GetPosition(eye);

		auto* s = GetFirstSectorContainingPoint((Vec2&) eye, sectors);
		if (s != nullptr)
		{
			Matrix4x4 world;
			platform.graphics.camera.GetWorld(world);
			Vec3 dir = world.GetWorldToCameraForwardDirection();
			if (!s->UseAnythingAt(eye, dir, 1.5_metres))
			{
				size_t nGaps;
				auto* gaps = s->Gaps(nGaps);
				for (size_t i = 0; i < nGaps; ++i)
				{
					auto& g = gaps[i];
					if (g.other->UseAnythingAt(eye, dir, 1.0_metres))
					{
						break;
					}
				}
			}
		}
	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		if (!clickedDown) return;

		if (overlayInventory)
		{
		}
		else
		{
			UseAnythingAtCrosshair();
		}
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
	{
		if (!clickedDown)
		{
			isCursorActive = !isCursorActive;
			platform.graphics.renderer.SetCursorVisibility(isCursorActive);
		}
	}

	void RenderCrosshair(IGuiRenderContext& g, const GuiRect& absRect)
	{
		GuiMetrics metrics;
		g.Renderer().GetGuiMetrics(metrics);

		Vec2i c{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };

		Graphics::DrawLine(g, 1, c - Vec2i{ 1, 0 }, c + Vec2i{ 0, 1 }, RGBAb(255, 255, 255, 255));
		Graphics::DrawLine(g, 1, c - Vec2i{ 0, 1 }, c + Vec2i{ 1, 0 }, RGBAb(255, 255, 255, 255));
	}

	void RenderXbox360Data(IGuiRenderContext& g)
	{
		ID_FONT idFont = platform.plumbing.utilities.GetHQFonts().GetSysFont(Graphics::HQFont::TitleFont);
		if (idFont)
		{
			GuiMetrics metrics;
			g.Renderer().GetGuiMetrics(metrics);
				
			GuiRect line1 = { 10, 20, metrics.screenSpan.x, 50 };

			GuiRect line = line1;

			struct : IEventCallback<cstr>
			{
				GuiRect line;
				ID_FONT idFont;
				RGBAb white = RGBAb(255, 255, 255, 255);
				IGuiRenderContext* g;

				void OnEvent(cstr text) override
				{
					Graphics::RenderHQText_LeftAligned_VCentre(*g, idFont, line, text, white);
					line.top += 75;
					line.bottom += 75;
				}
			} renderLine;

			renderLine.idFont = idFont;
			renderLine.line = line1;
			renderLine.g = &g;

			Joysticks::Joystick_XBOX360 x;
			if (!platform.hardware.xbox360joystick.TryGet(0, x))
			{
				renderLine.OnEvent("Waiting for Xbox360 controller 0");
			}
			else
			{
				platform.hardware.xbox360joystick.EnumerateStateAsText(x, renderLine);
			}
		}
	}

	void RenderSplash(IGuiRenderContext& g)
	{
		/* enable font loading in app.created.sxy to make this work */

		ID_FONT idFont = platform.plumbing.utilities.GetHQFonts().GetSysFont(Graphics::HQFont::TitleFont);
		if (idFont)
		{
			GuiMetrics metrics;
			g.Renderer().GetGuiMetrics(metrics);
			auto white = RGBAb(255, 255, 255, 255);

			int32 hDelta;

			auto& credits = metrics.screenSpan.x < 1800 ? narrow_credits : wide_credits;
			hDelta = metrics.screenSpan.x < 1800 ? 85 : 100;

			int32 topBorder = (metrics.screenSpan.y - (int32)credits.size() * hDelta) >> 1;

			GuiRect line1 = { 0, topBorder, metrics.screenSpan.x, topBorder + hDelta };

			GuiRect line = line1;

			for (auto* credit : credits)
			{
				Graphics::RenderCentred(g, idFont, line, credit, white);
				line.top += hDelta;
				line.bottom += hDelta;
			}
		}
	}

	void Render(IGuiRenderContext& g, const GuiRect& absRect) override
	{
		if (sectors.begin() == sectors.end())
		{
			if (platform.graphics.gui.Count() == 1)
			{
				if (!overlayInventory)
				{
					RenderSplash(g);
				}
				//RenderXbox360Data(g);
			}
		}
		else
		{
			if (!overlayInventory)
			{
				RenderCrosshair(g, absRect);
			}
		}
	}

	void ClearCache() override
	{
		illuminatedRooms.clear();
		shadowCasterSectors.clear();
		visibleSectorsThisTimestep.clear();
		visibleSectors.clear();
	}
};

namespace HV
{
	IFPSGameModeSupervisor* CreateFPSGameLogic(Platform& platform, IPlayerSupervisor& players, ISectors& sectors, IObjectManager& objects)
	{
		return new FPSGameLogic(platform, players, sectors, objects);
	}
}