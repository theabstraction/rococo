#pragma once

#include <rococo.types.h>
#include <rococo.gui.retained.h>

#ifndef ROCOCO_GREAT_SEX_API
# define ROCOCO_GREAT_SEX_API ROCOCO_API_IMPORT
#endif

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
	};

	typedef void (*FN_GREAT_SEX_ON_WIDGET)(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& widgetDefinition, Rococo::Gui::IGRWidget& parent);

	ROCOCO_INTERFACE IGreatSexGeneratorSupervisor : IGreatSexGenerator
	{
		virtual void AddHandler(cstr fqName, FN_GREAT_SEX_ON_WIDGET f) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_GREAT_SEX_API IGreatSexGeneratorSupervisor* CreateGreatSexGenerator(IAllocator& sexmlAllocator);
}