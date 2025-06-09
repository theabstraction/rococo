#include "rococo.GR.UE5.h"
#include "RococoGuiAPI.h"
#include <rococo.gui.retained.ex.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.maths.i32.h>
#include <rococo.hashtable.h>
#include <rococo.ui.h>
#include <rococo.io.h>
#include <rococo.vector.ex.h>
#include "SlateRenderContext.h"
#include <rococo.great.sex.h>
#include <Fonts/FontMeasure.h>
#include <Engine/Font.h>
#include <rococo.os.h>
#include "../Public/RococoFontSet.h"

#include <../rococo.gui.retained/rococo.gr.image-loading.inl>

namespace Rococo
{
	inline FVector2f ToFVector2f(Vec2i v)
	{
		return FVector2f((float)v.x, (float)v.y);
	}

	inline Vec2i ToVec2i(FVector2f v)
	{
		return Vec2i((int)v.X, (int)v.Y);
	}

	FVector2f SlateRenderContext::ToSlatePosition(Rococo::Vec2i pos)
	{
		return geometry.LocalToAbsolute(ToFVector2f(pos));
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
		Vec2i span{ 8, 8 };

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
					this->span = span;
				}
			);
		}

		UE5_GR_Image(cstr _hint, UTexture2D* _imageTexture) :
			hint(_hint), imageTexture(_imageTexture)
		{
			span = Vec2i{ imageTexture->GetSizeX(), imageTexture->GetSizeY() };
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
			return span;
		}
	};

	const FSlateFontInfo& GetFont(UE5_GR_Custodian& custodian, GRFontId fontId);
	Vec2i EvaluateMinimalSpan(UE5_GR_Custodian& custodian, GRFontId fontId, const FText& localizedText, Vec2i extraSpan);
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

		void DrawText(const FSlateFontInfo& fontInfo, const GuiRect& absRect, RGBAb colour, cstr format, ...)
		{
			char buffer[1024];

			va_list args;
			va_start(args, format);
			Strings::SafeVFormat(buffer, sizeof buffer, format, args);
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
				DrawText(f, absRect, RGBAb(255, 255, 255), "%s", errCapture.message.c_str());

				absRect.top += lineHeight;
				DrawText(f, absRect, RGBAb(255, 255, 255), "Source File: %s", errCapture.filename.c_str());

				Vec2i s = errCapture.startPos;
				Vec2i e = errCapture.endPos;

				if (s.x != 0 || s.y != 0 || e.x != 0 || e.y != 0)
				{
					absRect.top += lineHeight;
					DrawText(f, absRect, RGBAb(255, 255, 255), "From line %d (pos %d) to line %d (pos %d)", s.y, s.x, e.x, e.y);
				}

				if (errCapture.errorCode != 0)
				{
					absRect.top += lineHeight;

					char err[256];
					Rococo::OS::FormatErrorMessage(err, sizeof err, errCapture.errorCode);

					DrawText(f, absRect, RGBAb(255, 255, 255), "%s", err);
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
			GuiRect innerRect = GetAlignedRect(alignment, absRect, noSpacing, image.span);
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

		//	ClipContext clip(rc, visibleRect);

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

		void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, const CaretSpec& caret) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::NoGamma : ESlateDrawEffect::DisabledEffect;

			FPaintGeometry ue5Rect = ToUE5Rect(clipRect, rc.geometry);

			FString localizedText(text);
			const FSlateFontInfo& fontInfo = GetFont(custodian, fontId);
			FSlateDrawElement::MakeText(rc.drawElements, (uint32) ++rc.layerId, ue5Rect, localizedText, fontInfo, drawEffects, ToLinearColor(colour));

			/*
			rc->FlushLayer();
			rc->SetScissorRect(lastScissorRect);

			// If there is nothing to display, then render a space character, which will give the caret a rectangle to work with
			fstring editText = text.length > 0 ? text : " "_fstring;

			alignment.Remove(EGRAlignment::Right).Add(EGRAlignment::Left);
			int32 iAlignment = GRAlignment_To_RococoAlignment(alignment);

			auto& metrics = rc->Resources().HQFontsResources().GetFontMetrics(To_ID_FONT(fontId));

			struct : IEventCallback<GlyphContext>
			{
				int32 charPos = 0;
				int32 caretPos = 0;
				int32 caretWidth = 0;
				Vec2i caretStart = { 0,0 };
				Vec2i caretEnd = { 0,0 };
				Vec2i lastBottomRight = { 0,0 };

				void OnEvent(GlyphContext& glyph) override
				{
					if (charPos == caretPos)
					{
						caretStart = BottomLeft(glyph.outputRect);
						caretEnd = BottomRight(glyph.outputRect);
					}

					lastBottomRight = BottomRight(glyph.outputRect);

					charPos++;
				}
			} glyphCallback;

			glyphCallback.caretPos = caret.CaretPos;
			glyphCallback.caretWidth = metrics.imgWidth;

			RGBAb transparent(0, 0, 0, 0);

			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, To_ID_FONT(fontId), editText, transparent, spacing, &glyphCallback);

			int32 dxShift = 0;

			if (glyphCallback.caretEnd.x <= glyphCallback.caretStart.x)
			{
				glyphCallback.caretStart = glyphCallback.lastBottomRight;
				glyphCallback.caretEnd = glyphCallback.caretStart + Vec2i{ metrics.imgWidth, 0 };
			}

			if (glyphCallback.caretEnd.x >= clipRect.right)
			{
				dxShift = (clipRect.right - glyphCallback.caretEnd.x);
			}

			glyphCallback.charPos = 0;
			glyphCallback.caretStart = glyphCallback.caretEnd = { 0,0 };

			Rococo::Graphics::RenderHQText(clipRect, iAlignment, *rc, To_ID_FONT(fontId), editText, colour, spacing, &glyphCallback, dxShift);

			if (glyphCallback.caretEnd.x <= glyphCallback.caretStart.x)
			{
				glyphCallback.caretStart = glyphCallback.lastBottomRight;
				glyphCallback.caretEnd = glyphCallback.caretStart + Vec2i{ metrics.imgWidth, 0 };
			}

			auto ticks = Rococo::Time::TickCount();
			auto dticks = ticks % Rococo::Time::TickHz();

			float dt = dticks / (float)Rococo::Time::TickHz();

			RGBAb blinkColour = dt > 0.5f ? caret.CaretColour1 : caret.CaretColour2;
			Rococo::Graphics::DrawLine(*rc, 1, glyphCallback.caretStart, glyphCallback.caretEnd, blinkColour);

			rc->FlushLayer();
			rc->ClearScissorRect();

			*/
		}

		TArray<FSlateVertex> triangleVertices;
		TArray<CodeSkipSizeType> triangleIndices;

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

		void DrawText(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			if (!lastScissorRect.IsNormalized())
			{
				return;
			}

			if (!AreRectsOverlapped(lastScissorRect, targetRect))
			{
				return;			
			}

			GuiRect cliprect = IntersectNormalizedRects(lastScissorRect, targetRect);

			ClipContext clip(rc, cliprect);

			auto drawEffects = rc.bEnabled ? ESlateDrawEffect::NoGamma : ESlateDrawEffect::DisabledEffect;

			const FSlateFontInfo& fontInfo = GetFont(custodian, fontId);
			if (fontInfo.HasValidFont())
			{
				const FText& localizedText = MapAsciiToLocalizedText(custodian, text);
				Vec2i textLocalSpan = EvaluateMinimalSpan(custodian, fontId, localizedText, Vec2i {0,0});
				Vec2i textPixelSpan = Vec2i{ (int) (textLocalSpan.x * rc.geometry.Scale), (int) (textLocalSpan.y / rc.geometry.Scale) };
				GuiRect textRect = GetAlignedRect(alignment, targetRect, spacing, textLocalSpan);
				FPaintGeometry textGeometryInPixelSpace(rc.ToSlatePosition(TopLeft(textRect)), ToFVector2f(textPixelSpan), 1.0f);
				FSlateDrawElement::MakeText(rc.drawElements, (uint32)++rc.layerId, textGeometryInPixelSpace, localizedText, fontInfo, drawEffects, ToLinearColor(colour));
			}
			else
			{
				FPaintGeometry ue5Rect = ToUE5Rect(targetRect, rc.geometry);
				auto errColour = ToLinearColor(RGBAb(255,255,0));
				FSlateColorBrush errorBrush(errColour);
				FSlateDrawElement::MakeBox(rc.drawElements, (uint32)++rc.layerId, ue5Rect, &errorBrush, drawEffects, FLinearColor::White);
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
		if (span.x > 0 && span.y > 0)
		{
			static_cast<UE5_GR_Renderer&>(g).RenderTexture(panel, alignment, spacing, isStretched, *this, { (float)span.x, (float)span.y }, span);
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

		UE5_GR_Custodian(TMap<FString, UTexture2D*>& _mapPathToImageTexture, const FSoftObjectPath& font) :
			fontMeasureService(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()),
			mapPathToImageTexture(_mapPathToImageTexture),
			fontAsset(font.ToString().Len() == 0 ? FSoftObjectPath("/Game/UI/Fonts/DA_RococoFonts") : font)
		{
			ue5os = IO::GetIOS();
			installation = IO::CreateInstallation(TEXT("UE5-rococo-content-def.txt"), *ue5os);
		}

		virtual ~UE5_GR_Custodian()
		{

		}

		void Free() override
		{
			delete this;
		}

		ErrorCapture errCapture;


		void AddLoadError(Rococo::GreatSex::LoadFrameException& err)
		{
			errCapture.startPos = err.startPos;
			errCapture.endPos = err.endPos;
			errCapture.filename = err.filename;
			errCapture.message = err.message;
			errCapture.errorCode = err.errorCode;
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
			return "XBOX";
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
			Strings::StackStringBuilder sb(fontName, sizeof fontName);
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

		GRFontId BindFontId(const FontSpec& spec) override
		{
			for (const PersistentFontSpec& pspec : fontSpecs)
			{
				if (pspec.spec == spec)
				{
					return pspec.fontId;
				}
			}

			PersistentFontSpec newPSpec{ spec.FontName, spec, (GRFontId)(++nextFontId) };

			FSlateFontInfo newFont = MatchFont(spec, zoomLevel);
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

			if (false && currentContext != nullptr)
			{
				return 1 + (int)(fontInfo.Size * currentContext->geometry.Scale);
			}
			else
			{
				return fontInfo.Size;
			}
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

		float PixelSpanToLocalSpaceSpanRatio() const
		{
			if (currentContext && currentContext->geometry.Scale != 0.0f)
			{
				return 1.0f / currentContext->geometry.Scale;
			}

			return 1.0f;
		}

		// Return local space span of the text string in the given font with extra pixelSpan specified in the final argument
		Vec2i EvaluateMinimalSpan(GRFontId fontId, const FText& text, Vec2i extraSpan) const
		{
			auto& font = GetFont(fontId);	
			FVector2f pixelSpan = fontMeasureService->Measure(text, font, 1.0f) + ToFVector2f(extraSpan);
			FVector2f localSpaceSpan = pixelSpan * PixelSpanToLocalSpaceSpanRatio();
			return ToVec2i(localSpaceSpan) + Vec2i(1,1);
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

		void RecordWidget(IGRWidget& widget) override
		{
			history.push_back(&widget);
		}

		void RouteKeyboardEvent(const KeyboardEvent& key) override
		{
			if (!grSystem)
			{
				Throw(0, "call method Bind(IGRSystemSupervisor& grSystem) before invoking " __FUNCTION__);
			}
			GRKeyEvent keyEvent{ *this, eventCount, key };
			lastRoutingStatus = grSystem->RouteKeyEvent(keyEvent);
			if (lastRoutingStatus == EGREventRouting::NextHandler)
			{
				grSystem->ApplyKeyGlobally(keyEvent);
			}
		}

		EGRCursorIcon currentIcon = EGRCursorIcon::Arrow;

		Vec2i lastCursorPos = { -1000000, -1000000 };

		void RouteMouseEvent(const MouseEvent& me, const GRKeyContextFlags& context) override
		{
			if (!grSystem)
			{
				Throw(0, "call method Bind(IGRSystemSupervisor& grSystem) before invoking " __FUNCTION__);
			}

			static_assert(sizeof GRCursorClick == sizeof uint16);

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
		}

		void RaiseError(const Sex::ISExpression* associatedSExpression, EGRErrorCode, cstr function, cstr format, ...) override
		{
			char message[1024];
			va_list args;
			va_start(args, format);
			Strings::SafeVFormat(message, sizeof message, format, args);
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
			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::Top).Add(EGRAlignment::Left);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment, Vec2i{ 0,0 }, "TopLeft - The quick brown fox jumps over the lazy dog"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment2;
			alignment2.Add(EGRAlignment::VCentre).Add(EGRAlignment::Left);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment2, Vec2i{ 0,0 }, "CentreLeft - She sells sea shells by the sea shore"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment3;
			alignment3.Add(EGRAlignment::Bottom).Add(EGRAlignment::Left);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment3, Vec2i{ 0,0 }, "BottomLeft - The cost of sausages was lost on the hostages"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment4;
			alignment4.Add(EGRAlignment::VCentre).Add(EGRAlignment::Right);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment4, Vec2i{ 0,0 }, "VCentreRight - Excalibur!"_fstring, RGBAb(255, 255, 255));

			GRAlignmentFlags alignment5;
			alignment5.Add(EGRAlignment::Bottom).Add(EGRAlignment::HCentre);
			renderer.DrawText(GRFontId::NONE, renderer.lastLocalSizeScreenDimensions, alignment5, Vec2i{ 0,0 }, "Bottom-HCentre - Lord, what fools these mortals be!"_fstring, RGBAb(255, 255, 255));
		}

		void Render(SlateRenderContext& rc) override
		{
			AutoRenderContext syncRCToThis(*this, rc);

			if (!grSystem)
			{
				Throw(0, "call method Bind(IGRSystemSupervisor& grSystem) before invoking " __FUNCTION__);
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
	ROCOCOGUI_API IUE5_GRCustodianSupervisor* Create_UE5_GRCustodian(TMap<FString, UTexture2D*>& mapPathToImageTexture, const FSoftObjectPath& font)
	{
		return new Rococo::Gui::UE5::Implementation::UE5_GR_Custodian(mapPathToImageTexture, font);
	}
}
