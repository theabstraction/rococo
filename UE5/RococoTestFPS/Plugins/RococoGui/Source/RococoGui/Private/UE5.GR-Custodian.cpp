#include "rococo.GR.UE5.h"
#include "RococoGuiAPI.h"
#include <rococo.gui.retained.ex.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.maths.i32.h>
#include <rococo.hashtable.h>
#include <rococo.ui.h>
#include <rococo.io.h>
#include "rococo.GR.UE5.h"
#include <rococo.vector.ex.h>
#include "SlateRenderContext.h"
#include <rococo.great.sex.h>
#include <Fonts/FontMeasure.h>
#include <Engine/Font.h>
#include <rococo.os.h>
#include <rococo.vkeys.h>
#include "../Public/RococoFontSet.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Layout/Clipping.h"
#include "Layout/Geometry.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/DrawElementTypes.h"
#include "Rendering/DrawElements.h"
#include <Framework/Application/SlateApplication.h>
#include <rococo.gr.image-loading.inl>

namespace Rococo
{
	inline FVector2f ToFVector2f(Vec2i v)
	{
		return FVector2f((float)v.x, (float)v.y);
	}

	inline Vec2i ToVec2i(FVector2f v)
	{
		return Vec2i{ (int)v.X, (int)v.Y };
	}

	FVector2f SlateRenderContext::ToSlatePosition(Rococo::Vec2i pos)
	{
		return geometry.LocalToAbsolute(ToFVector2f(pos));
	}

	TMap<FName, uint16> mapJStickToVirtualCode;

	void MapJoystickCode(const TCHAR* pName, uint16 keyboardCode)
	{
		FString sName(pName);
		FName name(sName);
		mapJStickToVirtualCode.Add(name, keyboardCode);
	}

	uint16 GetJoystickVKey(const FName& name)
	{
		auto* pVCode = mapJStickToVirtualCode.Find(name);
		return pVCode ? *pVCode : 0;
	}

	void BindStandardXBOXControlsToVKeys()
	{
		if (mapJStickToVirtualCode.Num() > 0)
		{
			return;
		}

		MapJoystickCode(TEXT("Gamepad_FaceButton_Right"), Rococo::IO::VirtualKeys::VKCode_ESCAPE);
		MapJoystickCode(TEXT("Gamepad_FaceButton_Bottom"), Rococo::IO::VirtualKeys::VKCode_ENTER);
		MapJoystickCode(TEXT("Gamepad_LeftShoulder"), Rococo::IO::VirtualKeys::VKCode_TAB);
		MapJoystickCode(TEXT("Gamepad_RightShoulder"), Rococo::IO::VirtualKeys::VKCode_ANTITAB);
		MapJoystickCode(TEXT("Gamepad_DPad_Up"), Rococo::IO::VirtualKeys::VKCode_UP);
		MapJoystickCode(TEXT("Gamepad_DPad_Down"), Rococo::IO::VirtualKeys::VKCode_DOWN);
		MapJoystickCode(TEXT("Gamepad_DPad_Left"), Rococo::IO::VirtualKeys::VKCode_LEFT);
		MapJoystickCode(TEXT("Gamepad_DPad_Right"), Rococo::IO::VirtualKeys::VKCode_RIGHT);
		MapJoystickCode(TEXT("Gamepad_FaceButton_Left"), Rococo::IO::VirtualKeys::VKCode_PGDOWN);
		MapJoystickCode(TEXT("Gamepad_FaceButton_Top"), Rococo::IO::VirtualKeys::VKCode_PGUP);
	}
}

namespace Rococo
{
	const Matrix2x2 Matrix2x2_RotateAnticlockwise(Radians phi)
	{
		float sina = sinf(phi);
		float cosa = cosf(phi);
		return Matrix2x2{ { cosa, -sina }, { sina, cosa } };
	}
}

namespace Rococo::OS
{
	ROCOCO_API Windows::IWindow& WindowOwner();
}

namespace Rococo::Gui
{
	struct ErrorCapture
	{
		Rococo::Strings::HString filename;
		Rococo::Strings::HString message;
		int errorCode = 0;
		Vec2i startPos = { 0,0 };
		Vec2i endPos = { 0,0 };
	};

	bool IsPointInRect(Vec2i p, const GuiRect& rect)
	{
		return (p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom);
	}

	ROCOCO_GUI_RETAINED_API EGREventRouting TranslateToEditor(
		Windows::IWindow& ownerWindow,
		const GRKeyEvent& keyEvent,
		IGREditorMicromanager& manager,
		Strings::ICharBuilder& builder);

	constexpr FLinearColor NoTint()
	{
		return FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	}

	FVector2f SlateSpan(const GuiRect& absRect)
	{
		return FVector2f((float)Width(absRect), (float)Height(absRect));
	}

	inline bool operator == (const FontSpec& a, const FontSpec& b)
	{
		return
			a.Bold == b.Bold &&
			a.CharHeight == b.CharHeight &&
			a.CharSet == b.CharSet &&
			Strings::Eq(a.FontName, b.FontName) &&
			a.Italic == b.Italic &&
			a.Underlined == b.Underlined;
	}

}

namespace Rococo::Gui::UE5::Implementation
{
	float ToFloat(uint8 j)
	{
		return j / 255.0f;
	}

	float ToSFloat(uint8 j)
	{
		float v = j / 255.0f;
		return powf(v, 2.21f);
	}

	FLinearColor ToLinearColor(RGBAb colour)
	{
		return FLinearColor(ToSFloat(colour.red), ToSFloat(colour.green), ToSFloat(colour.blue), ToFloat(colour.alpha));
	}

	FPaintGeometry ToUE5Rect(const GuiRect& absRect, const FGeometry& parentGeometry)
	{
		FVector2f localSize = ToFVector2f(Span(absRect));
		FSlateLayoutTransform offset = FSlateLayoutTransform(ToFVector2f(TopLeft(absRect)));
		return parentGeometry.ToPaintGeometry(localSize, offset);
	}

	enum class ERenderTaskType
	{
		Edge
	};

	struct RenderTask
	{
		ERenderTaskType type;
		GuiRect target;
		RGBAb colour1;
		RGBAb colour2;
	};

	struct UE5_GR_Custodian;

	struct UE5_GR_Image : IGRImageSupervisor
	{
		Vec2i imageSpan{ 8, 8 };

		FString hint;

		UTexture2D* imageTexture = nullptr;
		FSlateImageBrush* imageStretchBrush = nullptr;
		FSlateImageBrush* imageNoStretchBrush = nullptr;

		UE5_GR_Image(cstr _hint, cstr _imagePath, IO::IInstallation& installation): 
			hint(_hint)
		{
			Rococo::Gui::Implementation::Load32bitRGBAbImage(_imagePath, installation,
				[this, _imagePath]
				(Vec2i span, const RGBAb* imageBuffer)
				{
					FName imageName(_imagePath);
					// We load the UTexture with the image buffer, set the span and format to RGBAb 32-bit
					TConstArrayView64<uint8> imageData((uint8*)(imageBuffer), span.x * span.y * sizeof(RGBAb));
					imageTexture = UTexture2D::CreateTransient(span.x, span.y, EPixelFormat::PF_R8G8B8A8, imageName, imageData);
					FSlateColor noTint(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));
					imageStretchBrush = new FSlateImageBrush(imageTexture, UE::Slate::FDeprecateVector2DParameter((float)span.x, (float)span.y), noTint, ESlateBrushTileType::NoTile);
					imageStretchBrush->Margin = FMargin(0, 0);
					imageStretchBrush->DrawAs = ESlateBrushDrawType::Box;
					imageNoStretchBrush = new FSlateImageBrush(imageTexture, UE::Slate::FDeprecateVector2DParameter((float)span.x, (float)span.y), noTint, ESlateBrushTileType::Both);
					this->imageSpan = span;
				}
			);
		}

		UE5_GR_Image(cstr _hint, UTexture2D* _imageTexture) :
			hint(_hint), imageTexture(_imageTexture)
		{
			imageSpan = Vec2i{ imageTexture->GetSizeX(), imageTexture->GetSizeY() };
		}


		virtual ~UE5_GR_Image()
		{
			delete imageStretchBrush;
			delete imageNoStretchBrush;
		}

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, IGRRenderContext& g) override;

		void Free() override
		{
			delete this;
		}

		Vec2i Span() const override
		{
			return imageSpan;
		}
	};

	const FSlateFontInfo& GetFont(UE5_GR_Custodian& custodian, GRFontId fontId);
	Vec2i EvaluateMinimalSpan(UE5_GR_Custodian& custodian, GRFontId fontId, const FText& localizedText, Vec2i extraSpan);
	Vec2i EvaluateMinimalSpan(UE5_GR_Custodian& custodian, GRFontId fontId, const FString& text, Vec2i extraSpan, int startIndex, int endIndex);
	const FText& MapAsciiToLocalizedText(UE5_GR_Custodian& custodian, cstr text);

	GuiRect GetAlignedRect(GRAlignmentFlags alignment, const GuiRect& containerRect, Vec2i spacing, Vec2i innerRectSpan)
	{
		int32 x = 0;

		const GuiRect& rect = containerRect;

		if (alignment.IsLeft())
		{
			x = rect.left + spacing.x;
		}
		else if (alignment.IsRight())
		{
			x = rect.right - spacing.x - innerRectSpan.x;
		}
		else
		{
			x = (rect.left + rect.right - innerRectSpan.x) >> 1;
		}

		int y = 0;

		if (alignment.IsTop())
		{
			y = rect.top + spacing.y;
		}
		else if (alignment.IsBottom())
		{
			y = rect.bottom - spacing.y - innerRectSpan.y;
		}
		else
		{
			y = (rect.top + rect.bottom - innerRectSpan.y) >> 1;
			// A slight difference here between UE5 and GDI rendering. 
			// The UE5 fonts in small buttons look too high, so allow padding to shift them, even when centred
			y += spacing.y;
		}

		return GuiRect{ x, y, x + innerRectSpan.x, y + innerRectSpan.y };
	}

	struct UE5_GR_Renderer : IGRRenderContext
	{
		GuiRect lastLocalSizeScreenDimensions;
		Vec2i cursorPos{ -1000,-1000 };
		std::vector<RenderTask> lastTasks;
		SlateRenderContext& rc;
		UE5_GR_Custodian& custodian;

		UE5_GR_Renderer(SlateRenderContext& _rc, UE5_GR_Custodian& _custodian) : rc(_rc), custodian(_custodian)
		{
			auto localSize = _rc.geometry.GetLocalSize();
			lastLocalSizeScreenDimensions.left = 0;
			lastLocalSizeScreenDimensions.top = 0;
			lastLocalSizeScreenDimensions.right = localSize.X;
			lastLocalSizeScreenDimensions.bottom = localSize.Y;

			BindStandardXBOXControlsToVKeys();
		}

		virtual ~UE5_GR_Renderer()
		{

		}

		IGRFonts& Fonts() override;
		IGRImages& Images() override;

		FText ToText(const Rococo::Strings::HString& s)
		{
			FString sText(s.c_str());
			return FText::FromString(sText);
		}

		FSlateLayoutTransform LocalShift(const GuiRect& localRect)
		{
			float dxShift = (float)(localRect.left - lastLocalSizeScreenDimensions.left);
			float dyShift = (float)(localRect.top - lastLocalSizeScreenDimensions.top);
			return FSlateLayoutTransform(FVector2f(dxShift, dyShift));
		}

		FPaintGeometry AsGeometry(const GuiRect& localRect)
		{
			return rc.geometry.ToPaintGeometry(SlateSpan(localRect), LocalShift(localRect));
		}

		void DrawErrorText(const FSlateFontInfo& fontInfo, const GuiRect& absRect, RGBAb colour, cstr format, ...)
		{
			char buffer[1024];

			va_list args;
			va_start(args, format);
			Strings::SafeVFormat(buffer, sizeof(buffer), format, args);
			va_end(args);

			auto lcolor = ToLinearColor(colour);

			FSlateDrawElement::MakeText(rc.drawElements, (uint32)++rc.layerId, AsGeometry(absRect), MapAsciiToLocalizedText(custodian, buffer), fontInfo, ESlateDrawEffect::NoGamma, lcolor);
		}

		void DrawError(const ErrorCapture& errCapture)
		{
			auto drawEffects = ESlateDrawEffect::None;

			const FSlateFontInfo& f = GetFont(custodian, GRFontId::NONE);

			if (f.HasValidFont())
			{
				FSlateColorBrush backBrush(FLinearColor::Black);
				FSlateDrawElement::MakeBox(rc.drawElements, (uint32)++rc.layerId, rc.geometry.ToPaintGeometry(), &backBrush, drawEffects, FLinearColor::Black);

				int lineHeight = 1.5 * f.Size;

				GuiRect absRect = lastLocalSizeScreenDimensions;

				if (Height(absRect) > 400)
				{
					// The top right in debug mode is overwritten with mouse control hint text, so we displace below this message to make the message legible.
					absRect.top += 200;
				}

				absRect.left += lineHeight;
				absRect.top += lineHeight;
				DrawErrorText(f, absRect, RGBAb(255, 255, 255), "%s", errCapture.message.c_str());

				absRect.top += lineHeight;
				DrawErrorText(f, absRect, RGBAb(255, 255, 255), "Source File: %s", errCapture.filename.c_str());

				Vec2i s = errCapture.startPos;
				Vec2i e = errCapture.endPos;

				if (s.x != 0 || s.y != 0 || e.x != 0 || e.y != 0)
				{
					absRect.top += lineHeight;
					DrawErrorText(f, absRect, RGBAb(255, 255, 255), "From line %d (pos %d) to line %d (pos %d)", s.y, s.x, e.x, e.y);
				}

				if (errCapture.errorCode != 0)
				{
					absRect.top += lineHeight;

					char err[256];
					Rococo::OS::FormatErrorMessage(err, sizeof(err), errCapture.errorCode);

					DrawErrorText(f, absRect, RGBAb(255, 255, 255), "%s", err);
				}
			}
			else
			{
				auto errColour = ToLinearColor(RGBAb(255, 255, 0));
				FSlateColorBrush errorBrush(errColour);
				FSlateDrawElement::MakeBox(rc.drawElements, (uint32)++rc.layerId, rc.geometry.ToPaintGeometry(), &errorBrush, drawEffects, errColour);
			}
		}

		void DrawLastItems()
		{
			for (auto& task : lastTasks)
			{
				switch (task.type)
				{
				case ERenderTaskType::Edge:
					DrawRectEdge(task.target, task.colour1, task.colour2, EGRRectStyle::SHARP, 4);
					break;
				}
			}

			lastTasks.clear();
		}

		void DrawTriangleFacingUp(const GuiRect& container, RGBAb colour)
		{
			GRTriangle t;

			t.a.colour = colour;
			t.a.position = Vec2i { ((container.left + container.right) >> 1), container.top };
			t.b.colour = colour;
			t.b.position = BottomRight(container);
			t.c.colour = colour;
			t.c.position = BottomLeft(container);
			
			AddTriangle(t);
		}

		void DrawTriangleFacingDown(const GuiRect& container, RGBAb colour)
		{
			GRTriangle t;

			t.a.colour = colour;
			t.a.position = Vec2i{ ((container.left + container.right) >> 1), container.bottom };
			t.b.colour = colour;
			t.b.position = TopRight(container);
			t.c.colour = colour;
			t.c.position = TopLeft(container);

			AddTriangle(t);
		}

		void DrawDirectionArrow(const GuiRect& absRect, RGBAb colour, Degrees heading) override
		{
			GuiRect clipRect = IntersectNormalizedRects(absRect, lastScissorRect);
			ClipContext clip(rc, clipRect);

			++rc.layerId;

			if (heading.degrees == 0)
			{
				DrawTriangleFacingUp(absRect, colour);
			}
			else
			{
				DrawTriangleFacingDown(absRect, colour);
			}

			CommitTrianglesForRendering_NoLayerInc();
		}

		void DrawImageStretched(IGRImage& _image, const GuiRect& absRect) override
		{
			auto& image = static_cast<UE5_GR_Image&>(_image);
			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			FPaintGeometry ue5Rect = ToUE5Rect(absRect, rc.geometry);
			FSlateDrawElement::MakeBox(rc.drawElements, ++rc.layerId, ue5Rect, image.imageStretchBrush, drawEffects, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}

		void DrawImageUnstretched(IGRImage& _image, const GuiRect& absRect, GRAlignmentFlags alignment)  override
		{
			auto& image = static_cast<UE5_GR_Image&>(_image);
			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			Vec2i noSpacing{ 0,0 };
			GuiRect innerRect = GetAlignedRect(alignment, absRect, noSpacing, image.imageSpan);
			FPaintGeometry ue5Rect = ToUE5Rect(innerRect, rc.geometry);
			FSlateDrawElement::MakeBox(rc.drawElements, ++rc.layerId, ue5Rect, image.imageNoStretchBrush, drawEffects, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}

		static inline Vec2 FlipY(Vec2 p)
		{
			return { p.x, -p.y };
		}

		void AddArc(RGBAb colour, Vec2i origin, int radius, Degrees startAngle, Degrees sweepAngle)
		{
			if (colour.alpha == 0)
			{
				return;
			}

			/*
			|      / P
			|     /
			|    /
			|   /
			|  /
			| /
			|/
			O-------------------------
			*/

			Vec2 fOrigin{ (float)origin.x, (float)origin.y };

			float arcLength = radius * (sweepAngle.degrees / 360.0f) * 2.0f * PI();

			float pixelsPerSegment = 2.0;
			int nDivisions = clamp((int)(arcLength / pixelsPerSegment), 1, 4096);

			Vec2 ri{ (float)radius, 0 }; // The vector i scaled by r, the radius

			Degrees theta0 = startAngle;
			Degrees dTheta = Degrees{ sweepAngle / nDivisions };

			for (int d = 0; d < nDivisions; d++)
			{
				const Matrix2x2 rotStart = Matrix2x2_RotateAnticlockwise(theta0);

				Degrees endAngle = Degrees{ theta0.degrees + dTheta.degrees };

				const Matrix2x2 rotEnd = Matrix2x2_RotateAnticlockwise(endAngle);

				Vec2 start = FlipY(rotStart * ri) + fOrigin;
				Vec2 end = FlipY(rotEnd * ri) + fOrigin;

				GRTriangle t =
				{
					{ origin,  colour },
					{ Quantize(start),    colour },
					{ Quantize(end),      colour }
				};

				AddTriangle(t);

				theta0.degrees += dTheta.degrees;
			}
		}

		void DrawSharpRect_NoLayerInc(const GuiRect& absRect, RGBAb colour)
		{
			ESlateDrawEffect drawEffects = rc.bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

			auto ue5Colour = ToLinearColor(colour);
			FSlateColorBrush solidBrush(ue5Colour);

			auto ue5Rect = AsGeometry(absRect);

			FSlateDrawElement::MakeBox(OUT rc.drawElements,
				rc.layerId,
				ue5Rect,
				&solidBrush,
				drawEffects,
				ue5Colour
			);
		}

		void DrawRoundedRect_NoLayerInc(const GuiRect& rect, RGBAb colour, int cornerRadius)
		{
			ClipContext clip(rc, lastScissorRect);

			GuiRect centreRect;
			centreRect.left = rect.left + cornerRadius;
			centreRect.top = rect.top;
			centreRect.right = rect.right - cornerRadius;
			centreRect.bottom = rect.bottom;
			if (centreRect.IsNormalized())
			{
				DrawSharpRect_NoLayerInc(centreRect, colour);
			}

			GuiRect leftRect;
			leftRect.left = rect.left;
			leftRect.top = rect.top + cornerRadius;
			leftRect.right = rect.left + cornerRadius;
			leftRect.bottom = rect.bottom - cornerRadius;
			if (leftRect.IsNormalized())
			{
				DrawSharpRect_NoLayerInc(leftRect, colour);
			}

			GuiRect rightRect;
			rightRect.left = rect.right - cornerRadius;
			rightRect.top = rect.top + cornerRadius;
			rightRect.right = rect.right;
			rightRect.bottom = rect.bottom - cornerRadius;
			if (rightRect.IsNormalized())
			{
				DrawSharpRect_NoLayerInc(rightRect, colour);
			}

			Vec2i leftTop{ rect.left + cornerRadius, rect.top + cornerRadius };
			AddArc(colour, leftTop, cornerRadius, 90_degrees, 90_degrees);

			Vec2i rightTop{ rect.right - cornerRadius, rect.top + cornerRadius };
			AddArc(colour, rightTop, cornerRadius, 0_degrees, 90_degrees);

			Vec2i leftBottom{ rect.left + cornerRadius, rect.bottom - cornerRadius };
			AddArc(colour, leftBottom, cornerRadius, 180_degrees, 90_degrees);

			Vec2i rightBottom{ rect.right - cornerRadius, rect.bottom - cornerRadius };
			AddArc(colour, rightBottom, cornerRadius, 270_degrees, 90_degrees);

			CommitTrianglesForRendering_NoLayerInc();
		}

		void DrawRect(const GuiRect& absRect, RGBAb colour, EGRRectStyle rectStyle, int cornerRadius) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			if (colour.alpha == 0)
			{
				return;
			}

			GuiRect visibleRect = IntersectNormalizedRects(absRect, lastScissorRect);
			if (!visibleRect.IsNormalized())
			{
				return;
			}

			++rc.layerId;

			switch (rectStyle)
			{
				case EGRRectStyle::SHARP:
					DrawSharpRect_NoLayerInc(visibleRect, colour);
					break;
				case EGRRectStyle::ROUNDED_WITH_BLUR:
				case EGRRectStyle::ROUNDED:
					DrawRoundedRect_NoLayerInc(absRect, colour, cornerRadius);
					break;
			}
		}

		TArray<FVector2D> pointBuilder;

		void DrawLine(Vec2i start, Vec2i end, RGBAb colour) override
		{
			ClipContext clip(rc, lastScissorRect);

			pointBuilder.Empty();
			pointBuilder.Add(FVector2D(start.x, start.y));
			pointBuilder.Add(FVector2D(end.x, end.y));
			FSlateDrawElement::MakeLines(rc.drawElements, ++rc.layerId, rc.geometry.ToPaintGeometry(), pointBuilder, ESlateDrawEffect::None, ToLinearColor(colour));
			pointBuilder.Empty();
		}

		void DrawLine_NoLayerInc(Vec2i start, Vec2i end, RGBAb colour)
		{
			pointBuilder.Empty();
			pointBuilder.Add(FVector2D(start.x, start.y));
			pointBuilder.Add(FVector2D(end.x, end.y));
			FSlateDrawElement::MakeLines(rc.drawElements, rc.layerId, rc.geometry.ToPaintGeometry(), pointBuilder, ESlateDrawEffect::None, ToLinearColor(colour));
			pointBuilder.Empty();
		}

		void DrawBorderAround(const GuiRect& absRect, Vec2i width, RGBAb colour1, RGBAb colour2)
		{
			FLinearColor topLeftColour = ToLinearColor(colour1);
			pointBuilder.Empty();
			pointBuilder.Add(FVector2D(absRect.left + 1.0, absRect.bottom));
			pointBuilder.Add(FVector2D(absRect.left + 1.0, absRect.top + 1.0));
			pointBuilder.Add(FVector2D(absRect.right, absRect.top + 1.0));
			FSlateDrawElement::MakeLines(rc.drawElements, ++rc.layerId, rc.geometry.ToPaintGeometry(), pointBuilder, ESlateDrawEffect::None, topLeftColour);

			FLinearColor bottomRightColour = ToLinearColor(colour2);
			pointBuilder.Empty();
			pointBuilder.Add(FVector2D(absRect.right - 1.0, absRect.top + 1.0));
			pointBuilder.Add(FVector2D(absRect.right - 1.0, absRect.bottom - 1.0));
			pointBuilder.Add(FVector2D(absRect.left, absRect.bottom - 1.0));
			FSlateDrawElement::MakeLines(rc.drawElements, rc.layerId, rc.geometry.ToPaintGeometry(), pointBuilder, ESlateDrawEffect::None, bottomRightColour);
		}

		void DrawArcEdge_NoLayerInc(Vec2i origin, int radius, Degrees startAngle, Degrees sweepAngle, RGBAb colour)
		{
			/*
			|      / P
			|     /
			|    /
			|   /
			|  /
			| /
			|/
			O-------------------------
			*/

			Vec2 fOrigin{ (float)origin.x, (float)origin.y };

			float arcLength = radius * (sweepAngle.degrees / 360.0f) * 2.0f * PI();

			float pixelsPerSegment = 2.0;
			int nDivisions = clamp((int)(arcLength / pixelsPerSegment), 1, 4096);

			Vec2 ri{ (float)radius, 0 }; // The vector i scaled by r, the radius

			Degrees theta0 = startAngle;
			Degrees dTheta = Degrees{ sweepAngle / nDivisions };

			for (int d = 0; d < nDivisions; d++)
			{
				const Matrix2x2 rotStart = Matrix2x2_RotateAnticlockwise(theta0);

				Degrees endAngle = Degrees{ theta0.degrees + dTheta.degrees };

				const Matrix2x2 rotEnd = Matrix2x2_RotateAnticlockwise(endAngle);

				Vec2 start = FlipY(rotStart * ri) + fOrigin;
				Vec2 end = FlipY(rotEnd * ri) + fOrigin;

				DrawLine_NoLayerInc(Quantize(start), Quantize(end), colour);

				theta0.degrees += dTheta.degrees;
			}
		}

		void DrawRoundedEdge(const GuiRect& rect, RGBAb colour, int cornerRadius)
		{
			rc.layerId++;

			{
				Vec2i leftTop{ rect.left + cornerRadius, rect.top };
				Vec2i rightTop{ rect.right - cornerRadius - 1, rect.top };
				Vec2i leftBottom{ rect.left + cornerRadius, rect.bottom - 1 };
				Vec2i rightBottom{ rect.right - cornerRadius - 1, rect.bottom - 1 };
				DrawLine_NoLayerInc(leftTop, rightTop, colour);
				DrawLine_NoLayerInc(leftBottom, rightBottom, colour);
			}


			{
				Vec2i leftTop{ rect.left, rect.top + cornerRadius };
				Vec2i rightTop{ rect.right - 1, rect.top + cornerRadius };
				Vec2i leftBottom{ rect.left, rect.bottom - cornerRadius - 1 };
				Vec2i rightBottom{ rect.right - 1, rect.bottom - cornerRadius - 1 };
				DrawLine_NoLayerInc(rightTop, rightBottom, colour);
				DrawLine_NoLayerInc(leftTop, leftBottom, colour);
			}


			Vec2i leftTop{ rect.left + cornerRadius, rect.top + cornerRadius };
			DrawArcEdge_NoLayerInc(leftTop, cornerRadius, 90_degrees, 90_degrees, colour);

			Vec2i rightTop{ rect.right - cornerRadius - 1, rect.top + cornerRadius };
			DrawArcEdge_NoLayerInc(rightTop, cornerRadius, 0_degrees, 90_degrees, colour);

			Vec2i leftBottom{ rect.left + cornerRadius, rect.bottom - cornerRadius - 1 };
			DrawArcEdge_NoLayerInc(leftBottom, cornerRadius, 180_degrees, 90_degrees, colour);

			Vec2i rightBottom{ rect.right - cornerRadius - 1, rect.bottom - cornerRadius - 1 };
			DrawArcEdge_NoLayerInc(rightBottom, cornerRadius, 270_degrees, 90_degrees, colour);
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2, EGRRectStyle rectStyle, int cornerRadius) override
		{
			GuiRect visibleRect = IntersectNormalizedRects(absRect, lastScissorRect);
			if (!visibleRect.IsNormalized())
			{
				return;
			}

			ClipContext clip(rc, visibleRect);

			switch (rectStyle)
			{
			case EGRRectStyle::SHARP:
			{
				DrawBorderAround(absRect, Vec2i{ 1,1 }, colour1, colour2);
				break;
			}
			case EGRRectStyle::ROUNDED:
			case EGRRectStyle::ROUNDED_WITH_BLUR:
				DrawRoundedEdge(absRect, colour1, cornerRadius);
				break;
			}
		}

		void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			GuiRect visibleRect = IntersectNormalizedRects(absRect, lastScissorRect);
			if (visibleRect.IsNormalized())
			{
				RenderTask task{ ERenderTaskType::Edge, visibleRect, colour1, colour2 };
				lastTasks.push_back(task);
			}
		}

		void DrawEditableText(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, const CaretSpec& caret) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::NoGamma : ESlateDrawEffect::DisabledEffect;

			GuiRect scissorRect;
			if (TryGetScissorRect(OUT scissorRect))
			{
				if (!AreRectsOverlapped(scissorRect, targetRect))
				{
					return;
				}
			}

			GuiRect clipRect = IntersectRectWithScissors(targetRect);

			ClipContext clip(rc, clipRect);

			const FText& localizedText = MapAsciiToLocalizedText(custodian, text);
			const FSlateFontInfo& fontInfo = GetFont(custodian, fontId);

			FString localizedString = localizedText.ToString();

			Vec2i leftCaretPosSpan = EvaluateMinimalSpan(custodian, fontId, localizedString, Vec2i{ 0,0 }, 0, caret.CaretPos);
			Vec2i rightCaretPosSpan = EvaluateMinimalSpan(custodian, fontId, localizedString, Vec2i{ 0,0 }, caret.CaretPos, localizedString.Len());

			static const FText letterA = FText::FromString(TEXT("A"));

			Vec2i leftCaretPosSpanPlus1 = 
				caret.CaretPos >= localizedString.Len() ? 
				leftCaretPosSpan + EvaluateMinimalSpan(custodian, fontId, letterA, Vec2i{0,0})
				:
				EvaluateMinimalSpan(custodian, fontId, localizedString, Vec2i{ 0, 0 }, 0, caret.CaretPos+1);

			Vec2i textLocalSpan = { leftCaretPosSpan.x + rightCaretPosSpan.x, leftCaretPosSpan.y };
			
			FVector2f absSize = rc.geometry.GetAbsoluteSize();
			FVector2f localSize = rc.geometry.GetLocalSize();

			FVector2f ratio{ 1.0f, 1.0f };

			if (localSize.X > 0)
			{
				ratio.X = absSize.X / localSize.X;
			}

			if (localSize.Y > 0)
			{
				ratio.Y = absSize.Y / localSize.Y;
			}

			Vec2i textPixelSpan{ textLocalSpan.x * ratio.X, textLocalSpan.y * ratio.Y };
			Vec2i topLeft = ToVec2i(rc.geometry.LocalToAbsolute(ToFVector2f(TopLeft(targetRect))));
			Vec2i bottomRight = ToVec2i(rc.geometry.LocalToAbsolute(ToFVector2f(BottomRight(targetRect))));
			GuiRect pixelRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
			GuiRect textRect = GetAlignedRect(alignment, pixelRect, spacing, textPixelSpan);

			FPaintGeometry textGeometryInPixelSpace(ToFVector2f(TopLeft(textRect)), ToFVector2f(Span(textRect)), 1.0f);

			if (alignment.HasAllFlags(EGRAlignment::DrawAlignmentEdge))
			{
				auto edgeColour = ToLinearColor(alignmentEdgeColour);
				FSlateColorBrush edgeBrush(ToLinearColor(RGBAb(255, 255, 255, 255)));
				FSlateDrawElement::MakeBox(rc.drawElements, (uint32)++rc.layerId, textGeometryInPixelSpace, &edgeBrush, drawEffects, edgeColour);
			}

			FSlateDrawElement::MakeText(rc.drawElements, (uint32)++rc.layerId, textGeometryInPixelSpace, localizedText, fontInfo, drawEffects, ToLinearColor(colour));
			
			pointBuilder.Empty();
			pointBuilder.Add(FVector2D(ratio.X * (float) leftCaretPosSpan.x, ratio.Y * leftCaretPosSpan.y - 4));
			pointBuilder.Add(FVector2D(ratio.X * (float) leftCaretPosSpanPlus1.x, ratio.Y * leftCaretPosSpan.y - 4));
			FSlateDrawElement::MakeLines(rc.drawElements, ++rc.layerId, textGeometryInPixelSpace, pointBuilder, ESlateDrawEffect::None, ToLinearColor(colour));
			pointBuilder.Empty();
		}

		TArray<FSlateVertex> triangleVertices;
		TArray<SlateIndex> triangleIndices;

		void MakeSlateVertex(OUT FSlateVertex& v, IN const GRVertex& src)
		{
			v.TexCoords[0] = 0;
			v.TexCoords[1] = 0;
			v.TexCoords[2] = 0;
			v.TexCoords[3] = 0;
			v.MaterialTexCoords.X = 0;
			v.MaterialTexCoords.Y = 0;
			v.PixelSize[0] = 0;
			v.PixelSize[1] = 0;
			v.Color = FColor(src.colour.red, src.colour.green, src.colour.blue, src.colour.alpha);
			v.SecondaryColor = FColor(0, 0, 0, 0);
			v.Position = rc.ToSlatePosition(src.position);
		}

		FSlateColorBrush solidBrushForMeshing = FColor(255,255,255,255);

		FSlateResourceHandle hBlendedBrushResource;

		void AddTriangle(const GRTriangle& t)
		{
			FSlateVertex v;
			MakeSlateVertex(OUT v, IN t.a);
			triangleVertices.Add(v);

			MakeSlateVertex(OUT v, IN t.b);
			triangleVertices.Add(v);

			MakeSlateVertex(OUT v, IN t.c);
			triangleVertices.Add(v);

			int index = triangleIndices.Num();

			triangleIndices.Add(index++);
			triangleIndices.Add(index++);
			triangleIndices.Add(index++);
		}

		void CommitTrianglesForRendering_NoLayerInc()
		{
			if (triangleIndices.Num() == 0)
			{
				return;
			}

			if (!hBlendedBrushResource.IsValid())
			{
				FSlateColorBrush solidBrush(FColor(1.0f, 1.0f, 1.0f, 1.0f));
				hBlendedBrushResource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(solidBrushForMeshing);
			}

			ISlateUpdatableInstanceBuffer* instanceBuffer = nullptr;
			FSlateDrawElement::MakeCustomVerts(rc.drawElements, rc.layerId, hBlendedBrushResource, triangleVertices, triangleIndices, instanceBuffer, 0, 0);

			triangleIndices.Empty();
			triangleVertices.Empty();
		}

		void DrawTriangles(const GRTriangle* triangles, size_t nTriangles) override
		{
			if (nTriangles == 0)
			{
				return;
			}

			triangleVertices.Empty();
			triangleIndices.Empty();

			FSlateVertex v;

			uint32 index = 0;

			for (size_t i = 0; i < nTriangles; i++)
			{
				auto& t = triangles[i];

				MakeSlateVertex(OUT v, IN t.a);
				triangleVertices.Add(v);

				MakeSlateVertex(OUT v, IN t.b);
				triangleVertices.Add(v);

				MakeSlateVertex(OUT v, IN t.c);
				triangleVertices.Add(v);

				triangleIndices.Add(index++);
				triangleIndices.Add(index++);
				triangleIndices.Add(index++);
			}

			++rc.layerId;
			CommitTrianglesForRendering_NoLayerInc();
		}

		void DrawParagraph(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			DrawText(fontId, targetRect, alignment, spacing, text, colour);
		}

		struct ClipContext
		{
			FSlateClippingZone zone;
			SlateRenderContext& rc;

			ClipContext(SlateRenderContext& _rc, const GuiRect& rect): rc(_rc)
			{
				zone.BottomLeft = rc.ToSlatePosition(BottomLeft(rect));
				zone.BottomRight = rc.ToSlatePosition(BottomRight(rect));
				zone.TopLeft = rc.ToSlatePosition(TopLeft(rect));
				zone.TopRight = rc.ToSlatePosition(TopRight(rect));
				rc.drawElements.PushClip(zone);
			}

			~ClipContext()
			{
				rc.drawElements.PopClip();
			}
		};

		inline bool IsNullRect(const GuiRect& rect) const
		{
			return rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0;
		}

		GuiRect IntersectRectWithScissors(const GuiRect& targetRect)
		{
			if (IsNullRect(lastScissorRect))
			{
				return targetRect;
			}

			return IntersectNormalizedRects(lastScissorRect, targetRect);
		}

		RGBAb alignmentEdgeColour{ 255,255,255,255 };

		void SetAlignmentEdgeColour(RGBAb colour)
		{
			this->alignmentEdgeColour = colour;
		}

		void DrawText(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			GuiRect scissorRect;
			if (TryGetScissorRect(OUT scissorRect))
			{
				if (!AreRectsOverlapped(scissorRect, targetRect))
				{
					return;
				}
			}

			GuiRect cliprect = IntersectRectWithScissors(targetRect);

			ClipContext clip(rc, cliprect);

			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::NoGamma : ESlateDrawEffect::DisabledEffect;

			const FSlateFontInfo& fontInfo = GetFont(custodian, fontId);
			if (fontInfo.HasValidFont())
			{
				const FText& localizedText = MapAsciiToLocalizedText(custodian, text);
				Vec2i textLocalSpan = EvaluateMinimalSpan(custodian, fontId, localizedText, Vec2i {0,0});
				// Vec2i textPixelSpan = Vec2i{ (int) (textLocalSpan.x * rc.geometry.Scale), (int) (textLocalSpan.y * rc.geometry.Scale) };

				FVector2f absSize = rc.geometry.GetAbsoluteSize();
				FVector2f localSize = rc.geometry.GetLocalSize();

				FVector2f ratio { 1.0f, 1.0f };

				if (localSize.X > 0)
				{
					ratio.X = absSize.X / localSize.X;
				}

				if (localSize.Y > 0)
				{
					ratio.Y = absSize.Y / localSize.Y;
				}

				Vec2i textPixelSpan { textLocalSpan.x * ratio.X, textLocalSpan.y * ratio.Y };
				Vec2i topLeft = ToVec2i(rc.geometry.LocalToAbsolute(ToFVector2f(TopLeft(targetRect))));
				Vec2i bottomRight = ToVec2i(rc.geometry.LocalToAbsolute(ToFVector2f(BottomRight(targetRect))));
				GuiRect pixelRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
				GuiRect textRect = GetAlignedRect(alignment, pixelRect, spacing, textPixelSpan);
				FPaintGeometry textGeometryInPixelSpace(ToFVector2f(TopLeft(textRect)), ToFVector2f(Span(textRect)), 1.0f);

				if (alignment.HasAllFlags(EGRAlignment::DrawAlignmentEdge))
				{
					auto edgeColour = ToLinearColor(alignmentEdgeColour);
					FSlateColorBrush edgeBrush(ToLinearColor(RGBAb(255,255,255,255)));
					FSlateDrawElement::MakeBox(rc.drawElements, (uint32)++rc.layerId, textGeometryInPixelSpace, &edgeBrush, drawEffects, edgeColour);
				}

				FSlateDrawElement::MakeText(rc.drawElements, (uint32)++rc.layerId, textGeometryInPixelSpace, localizedText, fontInfo, drawEffects, ToLinearColor(colour));
			}
			else
			{
				FPaintGeometry ue5Rect = ToUE5Rect(targetRect, rc.geometry);
				auto errColour = ToLinearColor(RGBAb(255,255,0));
				FSlateColorBrush errorBrush(ToLinearColor(RGBAb(255, 255, 255, 255)));
				FSlateDrawElement::MakeBox(rc.drawElements, (uint32)++rc.layerId, ue5Rect, &errorBrush, drawEffects, errColour);
			}
		}

		void Flush() override
		{
		}

		Vec2i CursorHoverPoint() const override
		{
			return cursorPos;
		}

		bool IsHovered(IGRPanel& panel) const override
		{
			return Rococo::Gui::IsPointInRect(cursorPos, panel.AbsRect());
		}

		GuiRect ScreenDimensions() const override
		{
			return lastLocalSizeScreenDimensions;
		}

		GuiRect lastScissorRect;

		void EnableScissors(const GuiRect& scissorRect) override
		{
			lastScissorRect = scissorRect;
		}

		void DisableScissors() override
		{
			lastScissorRect = GuiRect{ 0,0,0,0 };
		}

		bool TryGetScissorRect(GuiRect& scissorRect) const override
		{
			scissorRect = lastScissorRect;
			return lastScissorRect.IsNormalized();
		}

		void RenderTexture(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, UE5_GR_Image& image, UE::Slate::FDeprecateVector2DParameter span, Vec2i iSpan)
		{
			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			FPaintGeometry ue5Rect = ToUE5Rect(isStretched ? panel.AbsRect() : GetAlignedRect(alignment, panel.AbsRect(), spacing, iSpan), rc.geometry);
			FSlateImageBrush* imgBrush = isStretched ? image.imageStretchBrush : image.imageNoStretchBrush;
			FSlateDrawElement::MakeBox(rc.drawElements, ++rc.layerId, ue5Rect, imgBrush, drawEffects, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}
	};

	bool UE5_GR_Image::Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, IGRRenderContext& g)
	{
		if (imageSpan.x > 0 && imageSpan.y > 0)
		{
			static_cast<UE5_GR_Renderer&>(g).RenderTexture(panel, alignment, spacing, isStretched, *this, { (float)imageSpan.x, (float)imageSpan.y }, imageSpan);
			return true;
		}
		else
		{
			return false;
		}
	}

	const stringmap<cstr> macroToPingPath =
	{
		{ "$(COLLAPSER_EXPAND)", "!textures/toolbars/MAT/expanded.tif" },
		{ "$(COLLAPSER_COLLAPSE)", "!textures/toolbars/MAT/collapsed.tif" },
		{ "$(COLLAPSER_ELEMENT_EXPAND)", "!textures/toolbars/MAT/expanded.tif" },
		{ "$(COLLAPSER_ELEMENT_INLINE)",  "!textures/toolbars/MAT/collapsed.tif" },
	};

	struct UE5_GR_Custodian : IUE5_GRCustodianSupervisor, IGREventHistory, IGRFonts, IGRImages
	{
		// Debugging materials:
		std::vector<IGRWidget*> history;
		EGREventRouting lastRoutingStatus = EGREventRouting::Terminate;
		int64 eventCount = 0;

		IGRSystemSupervisor* grSystem = nullptr;

		AutoFree<Rococo::IO::IOSSupervisor> ue5os;
		AutoFree<Rococo::IO::IInstallationSupervisor> installation;

		const TSharedRef<FSlateFontMeasure> fontMeasureService;

		TMap<FString, IGRImageSupervisor*> mapPathToImage;
		TMap<FString, UTexture2D*>& mapPathToImageTexture;

		FSoftObjectPath fontAsset;

		TObjectPtr<URococoFontSet> fontSet;

		IUE5_GlobalFontMetrics& fontMetrics;

		TObjectPtr<UObject> worldObject;

		UE5_GR_Custodian(UObject* _worldObject, TMap<FString, UTexture2D*>& _mapPathToImageTexture, const FSoftObjectPath& font, IUE5_GlobalFontMetrics& _fontMetrics) :
			fontMeasureService(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()),
			mapPathToImageTexture(_mapPathToImageTexture),
			fontAsset(font.ToString().Len() == 0 ? FSoftObjectPath("/Game/UI/Fonts/DA_RococoFonts") : font),
			fontMetrics(_fontMetrics),
			worldObject(_worldObject)
		{
			ue5os = IO::GetIOS();
			installation = IO::CreateInstallation(TEXT("UE5-rococo-content-def.txt"), *ue5os);
		}

		virtual ~UE5_GR_Custodian()
		{
			if (fontSet)
			{
				fontSet->RemoveFromRoot();
			}
		}

		void FocusDefaultTab() override
		{
			if (grSystem)
			{
				grSystem->FocusDefaultTab();
			}
		}

		void OnTick(float dt) override
		{
			if (grSystem)
			{
				grSystem->OnTick(dt);
			}
		}

		bool enableToScreen = false;
		bool enableToLogFile = false;

		void Log(const char* format, ...) override
		{
			if (!enableToLogFile && !enableToScreen)
			{
				return;
			}

			char asciiText[256];

			va_list args;
			va_start(args, format);
			Rococo::Strings::SafeVFormat(asciiText, sizeof asciiText, format, args);
			va_end(args);

			static FName msgKey(TEXT("UE5_GR_Custodian.Log"));
			UKismetSystemLibrary::PrintString(worldObject, FString::Printf(TEXT("%hs"), asciiText), enableToScreen, enableToLogFile, FLinearColor::White, 2.5f, msgKey);
		}

		void SetLogging(bool _enableToScreen, bool _enableToLogFile)
		{
			this->enableToScreen = _enableToScreen;
			this->enableToLogFile = _enableToLogFile;
		}

		void OnFocusChanged(IGRPanel* panel) override
		{
			FString msg;
			if (!panel)
			{
				Log("OnFocusChanged - no focus");
			}
			else
			{
				Log("OnFocusChanged: [%lld] %hs", panel->Id(), panel->Desc());
			}
		}

		void Free() override
		{
			delete this;
		}

		ErrorCapture errCapture;

		void AddLoadError(Rococo::GreatSex::LoadFrameException& err) override
		{
			errCapture.startPos = err.startPos;
			errCapture.endPos = err.endPos;
			errCapture.filename = err.filename;
			errCapture.message = err.message;
			errCapture.errorCode = err.errorCode;
		}

		void ClearLastLoadFrameError() override
		{
			errCapture.startPos = { 0,0 };
			errCapture.endPos = { 0,0 };
			errCapture.filename = "";
			errCapture.message = "";
			errCapture.errorCode = 0;
		}

		void Bind(IGRSystemSupervisor& _grSystem)
		{
			grSystem = &_grSystem;
		}

		IO::IInstallation& Installation() override
		{
			return *installation;
		}

		Windows::IWindow& Owner()
		{
			return Rococo::OS::WindowOwner();
		}

		float ZoomLevel() const override
		{
			return this->zoomLevel;
		}

		void AlertNoActionForKey() override
		{

		}

		IGRFonts& Fonts() override
		{
			return *this;
		}

		cstr GetLastKnownControlType() const override
		{
			return lastKnownControlType;
		}

		int64 nextFontId = 1;

		struct PersistentFontSpec
		{
			Strings::HString name;
			FontSpec spec;
			GRFontId fontId;
		};

		TArray<PersistentFontSpec> fontSpecs;

		FName FormatFontName(const FontSpec& spec)
		{
			char fontName[256];
			Strings::StackStringBuilder sb(fontName, sizeof(fontName));
			sb << spec.FontName;

			if (spec.Bold || spec.Italic || spec.Underlined)
			{
				sb << "_";
			}

			if (spec.Bold) sb << "Bold";
			if (spec.Italic) sb << "Italic";
			if (spec.Underlined) sb << "Underlined";

			return FName(fontName);
		}

		void ThrowIfTypefaceIsNotAMemberOfFont(FName originalName, FName typefaceName, const UFont& font)
		{
			const FCompositeFont* compositeFont = font.GetCompositeFont();
			if (!compositeFont)
			{
				Throw(0, "Expecting composite font inside of %ls", *font.GetClass()->GetPathName());
			}

			if (compositeFont->DefaultTypeface.Fonts.Num() == 0)
			{
				Throw(0, "No fonts specified in %ls", *font.GetClass()->GetPathName());
			}

			for (auto& typeface : compositeFont->DefaultTypeface.Fonts)
			{
				if (typeface.Font.GetFontFaceAsset()->GetFName() == typefaceName)
				{
					return;
				}
			}

			if (originalName != typefaceName)
			{
				Throw(0, "Rococo requires a typeface %ls", *originalName.ToString());
			}
			else
			{
				Throw(0, "Rococo mapped a typeface %ls to target %ls, but the target was not found.", *typefaceName.ToString(), *originalName.ToString());
			}
		}

		FSlateFontInfo MatchFont(const FontSpec& spec, float scaleFactor)
		{
			if (!fontSet)
			{
				UObject* oFontSet = fontAsset.TryLoad();
				if (!oFontSet)
				{
					Throw(0, "Font asset could not be loaded");
				}

				fontSet = Cast<URococoFontSet>(oFontSet);
				if (!fontSet)
				{
					Throw(0, "Font asset was not URococoFontSet: %s", *oFontSet->GetClass()->GetFName().ToString());
				}

				fontSet->AddToRoot();
			}

			try
			{
				auto* bpFontSet = fontSet.Get();

				UFont* font = bpFontSet->GetFontAsset();
				if (!font)
				{
					Throw(0, "FontAsset inside of FontSet was blank");
				}

				FName fontName = FormatFontName(spec);
				FName mappedName = bpFontSet->MapTypeface(fontName);
				FName resultantName = (mappedName == NAME_None) ? fontName : mappedName;

				ThrowIfTypefaceIsNotAMemberOfFont(fontName, resultantName, *font);

				return FSlateFontInfo(font, (int) (spec.CharHeight * scaleFactor), resultantName);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "%s: (font asset: %ls)", ex.Message(), *fontAsset.ToString());
			}
		}

		GRFontId BindFontId(const FontSpec& rawSpec) override
		{
			FontSpec spec = rawSpec;
			spec.CharHeight = fontMetrics.GetUE5PointSize(rawSpec.CharHeight);

			for (const PersistentFontSpec& pspec : fontSpecs)
			{
				if (pspec.spec == spec)
				{
					return pspec.fontId;
				}
			}

			PersistentFontSpec newPSpec{ spec.FontName, spec, (GRFontId)(++nextFontId) };

			FSlateFontInfo newFont = MatchFont(newPSpec.spec, zoomLevel);
			fontMap.Add(newPSpec.fontId, newFont);
			fontSpecs.Push(newPSpec);
			fontSpecs.Last().spec.FontName = fontSpecs.Last().name;
			return newPSpec.fontId;
		}

		TMap<GRFontId, FSlateFontInfo> fontMap;
		mutable FSlateFontInfo defaultFont;

		const FSlateFontInfo& GetFont(GRFontId id) const
		{
			auto* pFont = fontMap.Find(id);
			if (pFont)
			{
				return *pFont;
			}

			if (!defaultFont.HasValidFont())
			{
				defaultFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
			}
			return defaultFont;
		}

		int GetFontHeight(GRFontId id) const override
		{
			const FSlateFontInfo& fontInfo = GetFont(id);
			
			Vec2i span = EvaluateMinimalSpan(id, "A"_fstring, { 0,0 });
			return span.y;
		}

		void UpdateFontMap(float scalingFactor)
		{
			for (const auto& persistentSpec : fontSpecs)
			{
				FSlateFontInfo scaledFont = MatchFont(persistentSpec.spec, zoomLevel);
				fontMap[persistentSpec.fontId] = scaledFont;
			}
		}

		float zoomLevel = 1.0f;

		void SetUIZoom(float _zoomLevel) override
		{
			float newZoomLevel = clamp(1.0f, _zoomLevel, 100.0f);
			if (newZoomLevel != this->zoomLevel)
			{
				this->zoomLevel = newZoomLevel;
				UpdateFontMap(newZoomLevel);
			}
		}

		IGRImageSupervisor* CreateImageFromPath(cstr debugHint, cstr codedImagePath) override
		{
			WideFilePath sysPath;
			installation->ConvertPingPathToSysPath(codedImagePath, OUT sysPath);

			FString sKey(sysPath.buf);

			auto** ppImage = mapPathToImage.Find(sKey);
			if (ppImage != nullptr)
			{ 
				return *ppImage;
			}

			UE5_GR_Image* image;

			auto** ppTexture = mapPathToImageTexture.Find(sKey);
			if (ppTexture != nullptr)
			{
				image = new UE5_GR_Image(debugHint, *ppTexture);
			}
			else
			{
				image = new UE5_GR_Image(debugHint, codedImagePath, *installation);
				mapPathToImageTexture.Add(sKey, image->imageTexture);
			}

			mapPathToImage.Add(sKey, image);
			return image;
		}

		Vec2 PixelSpanToLocalSpaceSpanRatio() const
		{
			Vec2 ratio { 1.0f, 1.0f };

			if (currentContext)
			{
				auto& g = currentContext->geometry;
				if (g.GetAbsoluteSize().X != 0.0f)
				{
					ratio.x = g.GetLocalSize().X / g.GetAbsoluteSize().X;
				}

				if (g.GetAbsoluteSize().Y != 0.0f)
				{
					ratio.y = g.GetLocalSize().Y / g.GetAbsoluteSize().Y;
				}
			}

			return ratio;
		}

		// Return local space span of the text string in the given font with extra pixelSpan specified in the final argument
		Vec2i EvaluateMinimalSpan(GRFontId fontId, const FText& text, Vec2i extraSpan) const
		{
			auto& font = GetFont(fontId);	
			FVector2f pixelSpan = fontMeasureService->Measure(text, font, 1.0f);
			Vec2 r = PixelSpanToLocalSpaceSpanRatio();;
			FVector2f localSpaceSpan(pixelSpan.X * r.x, pixelSpan.Y * r.y);
			return ToVec2i(localSpaceSpan) + Vec2i{ 1,1 } + extraSpan;
		}

		Vec2i EvaluateMinimalSpan(GRFontId fontId, const FString& text, Vec2i extraSpan, int startIndex, int endIndex) const
		{
			auto& font = GetFont(fontId);
			FVector2f pixelSpan = fontMeasureService->Measure(FStringView(text), startIndex, endIndex, font, 1.0f);
			Vec2 r = PixelSpanToLocalSpaceSpanRatio();;
			FVector2f localSpaceSpan(pixelSpan.X * r.x, pixelSpan.Y * r.y);
			return ToVec2i(localSpaceSpan) + Vec2i{ 1,1 } + extraSpan;
		}

		mutable stringmap<FText> mapAsciiToLocalizedText;

		const FText& MapAsciiToLocalizedText(cstr text) const
		{
			auto i = mapAsciiToLocalizedText.find(text);
			if (i == mapAsciiToLocalizedText.end())
			{
				FString sText(text);
				FText localizedText = FText::FromString(sText);
				i = mapAsciiToLocalizedText.insert(text, localizedText).first;
			}

			return i->second;
		}

		Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text, Vec2i extraSpan) const override
		{
			FText localizedText = MapAsciiToLocalizedText(text);
			return EvaluateMinimalSpan(fontId, localizedText, extraSpan);
		}

		Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text, Vec2i extraSpan, int startIndex, int endIndex) const
		{
			FText localizedText = MapAsciiToLocalizedText(text);
			return EvaluateMinimalSpan(fontId, localizedText.ToString(), extraSpan, startIndex, endIndex);
		}

		Strings::HString lastKnownControlType;

		void SetControlType(cstr _lastKnownControlType) override
		{
			lastKnownControlType = _lastKnownControlType;
		}

		void RecordWidget(IGRWidget& widget) override
		{
			history.push_back(&widget);
		}

		EGREventRouting RouteKeyboardEvent(const KeyboardEvent& key) override
		{
			if (!grSystem)
			{
				Throw(0, "call method Bind(IGRSystemSupervisor& grSystem) before invoking %s", __ROCOCO_FUNCTION__);
			}
			GRKeyEvent keyEvent{ *this, eventCount, key };
			lastRoutingStatus = grSystem->RouteKeyEvent(keyEvent);
			return lastRoutingStatus;
		}

		EGRCursorIcon currentIcon = EGRCursorIcon::Arrow;

		Vec2i lastCursorPos = { -1000000, -1000000 };

		EGREventRouting RouteMouseEvent(const MouseEvent& me, const GRKeyContextFlags& context) override
		{
			if (!grSystem)
			{
				Throw(0, "call method Bind(IGRSystemSupervisor& grSystem) before invoking %s", __ROCOCO_FUNCTION__);
			}

			static_assert(sizeof(GRCursorClick) == sizeof(uint16));

			history.clear();
			if (me.buttonFlags != 0)
			{
				GRCursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(GRCursorClick*)&me.buttonFlags, EGRCursorIcon::Unspecified, (int)(int16)me.buttonData, context };
				lastRoutingStatus = grSystem->RouteCursorClickEvent(cursorEvent);
			}
			else
			{
				lastCursorPos = me.cursorPos;

				GRCursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(GRCursorClick*)&me.buttonFlags, EGRCursorIcon::Arrow, 0, context };
				lastRoutingStatus = grSystem->RouteCursorMoveEvent(cursorEvent);

				if (currentIcon != cursorEvent.nextIcon)
				{
					currentIcon = cursorEvent.nextIcon;

					switch (currentIcon)
					{
					case EGRCursorIcon::Arrow:
						//sysRenderer.GuiResources().SetSysCursor(EWindowCursor_Default);
						break;
					case EGRCursorIcon::LeftAndRightDragger:
						//sysRenderer.GuiResources().SetSysCursor(EWindowCursor_HDrag);
						break;
					}
				}
			}
			eventCount++;

			return lastRoutingStatus;
		}

		void RaiseError(const Sex::ISExpression* associatedSExpression, EGRErrorCode, cstr function, cstr format, ...) override
		{
			char message[1024];
			va_list args;
			va_start(args, format);
			Strings::SafeVFormat(message, sizeof(message), format, args);
			va_end(args);

			if (associatedSExpression)
			{
				Rococo::Sex::Throw(*associatedSExpression, "%s: %s", function, message);
			}
			else
			{
				Throw(0, "%s: %s", function, message);
			}
		}

		struct AutoRenderContext
		{
			UE5_GR_Custodian& custodian;

			AutoRenderContext(UE5_GR_Custodian& _custodian, SlateRenderContext& rc): custodian(_custodian)
			{
				_custodian.currentContext = &rc;
			}

			~AutoRenderContext()
			{
				custodian.currentContext = nullptr;
			}
		};


		SlateRenderContext* currentContext = nullptr;

		void RenderTestText(UE5_GR_Renderer& renderer)
		{
			renderer.SetAlignmentEdgeColour(RGBAb(192, 64, 64, 255));

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::Top).Add(EGRAlignment::Left).Add(EGRAlignment::DrawAlignmentEdge);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment, Vec2i{ 0,0 }, "TopLeft - The quick brown fox jumps over the lazy dog"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment2;
			alignment2.Add(EGRAlignment::VCentre).Add(EGRAlignment::Left).Add(EGRAlignment::DrawAlignmentEdge);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment2, Vec2i{ 0,0 }, "CentreLeft - She sells sea shells by the sea shore"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment3;
			alignment3.Add(EGRAlignment::Bottom).Add(EGRAlignment::Left).Add(EGRAlignment::DrawAlignmentEdge);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment3, Vec2i{ 0,0 }, "BottomLeft - The cost of sausages was lost on the hostages"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment4;
			alignment4.Add(EGRAlignment::VCentre).Add(EGRAlignment::Right).Add(EGRAlignment::DrawAlignmentEdge);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment4, Vec2i{ 0,0 }, "VCentreRight - Excalibur!"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment5;
			alignment5.Add(EGRAlignment::Bottom).Add(EGRAlignment::HCentre).Add(EGRAlignment::DrawAlignmentEdge);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment5, Vec2i{ 0,0 }, "Bottom-HCentre - Lord, what fools these mortals be!"_fstring, RGBAb(255, 255, 255));
		}

		void Render(SlateRenderContext& rc) override
		{
			AutoRenderContext syncRCToThis(*this, rc);

			if (!grSystem)
			{
				Throw(0, "call method Bind(IGRSystemSupervisor& grSystem) before invoking %s", __ROCOCO_FUNCTION__);
			}

			UE5_GR_Renderer renderer(rc, *this);

			renderer.cursorPos = lastCursorPos;

			if (false)
			{
				RenderTestText(renderer);
			}
			else
			{
				if (errCapture.message.length() > 0)
				{
					renderer.DrawError(errCapture);
				}
				else
				{
					grSystem->RenderAllFrames(renderer);
				}
				renderer.DrawLastItems();
			}
		}

		GRAnchorPadding Scale(GRAnchorPadding pixelPadding) override
		{
			if (!currentContext)
			{
				return pixelPadding;
			}

			float scale = currentContext->geometry.Scale;

			float f = scale == 0.0f ? 1.0f : 1.0f / scale;

			return GRAnchorPadding
			{
				(int) (pixelPadding.left   * f),
				(int) (pixelPadding.right  * f),
				(int) (pixelPadding.top    * f),
				(int) (pixelPadding.bottom * f),
			};
		}

		std::vector<char> copyAndPasteBuffer;

		EGREventRouting TranslateToEditor(const GRKeyEvent& keyEvent, IGREditorMicromanager& manager) override
		{
			Strings::CharBuilder builder(copyAndPasteBuffer);
			return Gui::TranslateToEditor(Rococo::OS::WindowOwner(), keyEvent, manager, builder);
		}
	};

	Vec2i EvaluateMinimalSpan(UE5_GR_Custodian& custodian, GRFontId fontId, const FText& localizedText, Vec2i extraSpan)
	{
		return custodian.EvaluateMinimalSpan(fontId, localizedText, extraSpan);
	}

	Vec2i EvaluateMinimalSpan(UE5_GR_Custodian& custodian, GRFontId fontId, const FString& text, Vec2i extraSpan, int startIndex, int endIndex)
	{
		return custodian.EvaluateMinimalSpan(fontId, text, extraSpan, startIndex, endIndex);
	}

	const FText& MapAsciiToLocalizedText(UE5_GR_Custodian& custodian, cstr text)
	{
		return custodian.MapAsciiToLocalizedText(text);
	}

	const FSlateFontInfo& GetFont(UE5_GR_Custodian& custodian, GRFontId fontId)
	{
		return custodian.GetFont(fontId);
	}

	IGRFonts& UE5_GR_Renderer::Fonts()
	{
		return custodian;
	}

	IGRImages& UE5_GR_Renderer::Images()
	{
		return custodian;
	}
}

namespace Rococo::Gui
{
	ROCOCO_GUI_API IUE5_GRCustodianSupervisor* Create_UE5_GRCustodian(UObject* worldObject, TMap<FString, UTexture2D*>& mapPathToImageTexture, const FSoftObjectPath& font, IUE5_GlobalFontMetrics& fontMetrics)
	{
		return new Rococo::Gui::UE5::Implementation::UE5_GR_Custodian(worldObject, mapPathToImageTexture, font, fontMetrics);
	}
}
