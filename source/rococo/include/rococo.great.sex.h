#pragma once

#include <rococo.types.h>
#include <rococo.gui.retained.h>

#ifndef ROCOCO_GREAT_SEX_API
# define ROCOCO_GREAT_SEX_API ROCOCO_API_IMPORT
#endif

namespace Rococo::IO
{
	DECLARE_ROCOCO_INTERFACE ILoadEventsCallback;
}

namespace Rococo::Sex
{
	DECLARE_ROCOCO_INTERFACE ISExpression;

	namespace SEXML
	{
		DECLARE_ROCOCO_INTERFACE ISEXMLDirective;
	}
}

// (G)ui(Re)t(a)ined(T)emplate via SexML
namespace Rococo::GreatSex
{
	ROCOCO_INTERFACE IGreatSexGenerator
	{
		virtual void AppendWidgetTreeFromSexML(const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget& branch) = 0;
		virtual void AppendWidgetTreeFromSexML(const Sex::ISExpression& s, Rococo::Gui::IGRWidget& branch) = 0;
		virtual void GenerateChildren(const Rococo::Sex::SEXML::ISEXMLDirective& widgetDirective, Rococo::Gui::IGRWidget& widget) = 0;
		virtual void SetPanelAttributes(Rococo::Gui::IGRWidget& widget, const Rococo::Sex::SEXML::ISEXMLDirective& widgetDirective) = 0;
	};

	ROCOCO_INTERFACE ISEXMLInserter
	{
		virtual void Insert(cstr filePath, const Sex::ISExpression& s, Gui::IGRWidget& owner) = 0;
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
		virtual void AddColour(cstr id, RGBAb colour, Rococo::Gui::GRRenderState rs) = 0;
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
}