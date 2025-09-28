#pragma once

#include <rococo.types.h>
#include <rococo.gui.retained.h>

#ifndef ROCOCO_GREAT_SEX_API
# define ROCOCO_GREAT_SEX_API ROCOCO_API_IMPORT
#endif

namespace Rococo::IO
{
	DECLARE_ROCOCO_INTERFACE ILoadEventsCallback;
	DECLARE_ROCOCO_INTERFACE IInstallation;
}

namespace Rococo::Sex
{
	DECLARE_ROCOCO_INTERFACE ISExpression;

	namespace SEXML
	{
		DECLARE_ROCOCO_INTERFACE ISEXMLDirective;
	}
}

namespace Rococo::Game::Options
{
	DECLARE_ROCOCO_INTERFACE IGameOptions;
	DECLARE_ROCOCO_INTERFACE IMissingOptions;
}

// (G)ui(Re)t(a)ined(T)emplate via SexML
namespace Rococo::GreatSex
{
	struct FontQuery
	{
		cstr id;
		cstr familyName;
		int height;
		bool isBold;
		bool isItalic;
	};

	ROCOCO_INTERFACE IGreatSexGenerator
	{
		virtual void AddFont(cstr id, cstr family, int height, bool isBold, bool isItalic) = 0;
		virtual void AddOptions(Game::Options::IGameOptions & options, cstr key) = 0;
		virtual RGBAb GetColour(cstr key, Gui::GRWidgetRenderState rs, const Sex::ISExpression& s) = 0;
		virtual FontQuery GetFont(cstr id, const Sex::ISExpression& s) const = 0;
		virtual void AppendWidgetTreeFromSexML(const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget& branch) = 0;
		virtual void AppendWidgetTreeFromSexML(const Sex::ISExpression& s, Rococo::Gui::IGRWidget& branch) = 0;
		virtual void GenerateChildren(const Rococo::Sex::SEXML::ISEXMLDirective& widgetDirective, Rococo::Gui::IGRWidget& widget) = 0;
		virtual void SetPanelAttributes(Rococo::Gui::IGRWidget& widget, const Rococo::Sex::SEXML::ISEXMLDirective& widgetDirective) = 0;
	};

	ROCOCO_INTERFACE ISEXMLInserter
	{
		virtual void Insert(cstr filePath, const Sex::ISExpression& s, Gui::IGRWidget& owner) = 0;
	};

	ROCOCO_INTERFACE ISEXMLGameOptionsList
	{
		virtual Game::Options::IGameOptions& GetOptions(cstr key, const Sex::ISExpression& src) = 0;
	};

	ROCOCO_INTERFACE ISEXMLWidgetFactory
	{
		virtual void Generate(IGreatSexGenerator & generator, const Rococo::Sex::SEXML::ISEXMLDirective & widgetDefinition, Rococo::Gui::IGRWidget & parent) = 0;
		virtual bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& widgetDefinition) const = 0;
	};

	ROCOCO_INTERFACE ISEXMLWidgetFactorySupervisor : ISEXMLWidgetFactory
	{
		virtual void Free() = 0;
	};

	struct ColourDirectiveBind
	{
		cstr name;
		Rococo::Gui::EGRSchemeColourSurface surface;
	};

	ROCOCO_INTERFACE ISEXMLColourSchemeBuilder
	{
		virtual void AddColour(cstr id, RGBAb colour, Rococo::Gui::GRWidgetRenderState rs) = 0;
	};

	ROCOCO_GREAT_SEX_API ISEXMLWidgetFactorySupervisor* CreateSchemeHandler();
	ROCOCO_GREAT_SEX_API ISEXMLWidgetFactorySupervisor* CreateColourHandler(ISEXMLColourSchemeBuilder& builder);

	ROCOCO_INTERFACE IGreatSexResourceLoader
	{
		virtual void LoadGreatSexResource(cstr resourcePath, Rococo::IO::ILoadEventsCallback& onLoad) = 0;
	};

	ROCOCO_INTERFACE IGreatSexGeneratorSupervisor : IGreatSexGenerator
	{
		// Factory lifetimes are expected to encompass the lifetime of the GreatSexGenerator, so the reference must be valid until after the generator is destructed.
		virtual void AddHandler(cstr fqName, ISEXMLWidgetFactory& f) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_GREAT_SEX_API IGreatSexGeneratorSupervisor* CreateGreatSexGenerator(IAllocator& sexmlAllocator, IGreatSexResourceLoader& resourceLoader);
	ROCOCO_GREAT_SEX_API void AddTestOptions(IGreatSexGenerator& generator);

	struct LoadFrameException
	{
		Vec2i startPos;
		Vec2i endPos;
		cstr filename;
		cstr fileData;
		cstr message;
		int errorCode;
	};

	ROCOCO_GREAT_SEX_API bool LoadFrame(
		IO::IInstallation& installation,
		IAllocator& allocator,
		cstr sexmlPath,
		Gui::IGRWidgetMainFrame& frame,
		IEventCallback<IGreatSexGenerator>& onConstruct,
		IEventCallback<LoadFrameException>& onException);
}