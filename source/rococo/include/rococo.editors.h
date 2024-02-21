#pragma once

#include <rococo.types.h>

namespace Rococo::Editors
{
	ROCOCO_INTERFACE ISuperListEvents
	{
		virtual void OnDoubleClickAtSelection(size_t index) = 0;
		virtual void OnReturnAtSelection(size_t index) = 0;
	};

	ROCOCO_INTERFACE ISuperListSpec
	{
		virtual ISuperListEvents& EventHandler() = 0;
	};

	ROCOCO_INTERFACE ISuperListBuilder
	{
		virtual void AddColumn(cstr name, int pixelWidth) = 0;
		virtual void AddColumnWithMaxWidth(cstr name) = 0;
		virtual void AddKeyValue(cstr key, cstr value) = 0;

		// Selects the first item with matching key
		virtual void Select(cstr key) = 0;
	};

	ROCOCO_INTERFACE ISuperComboBuilder
	{
		virtual void HidePopup() = 0;
		virtual void SetSelection(cstr key) = 0;
	};
}