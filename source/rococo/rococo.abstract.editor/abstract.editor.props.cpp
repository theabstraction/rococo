#include <rococo.abstract.editor.win32.h>
#include <rococo.window.h>
#include <vector>
#include <string>

using namespace Rococo;
using namespace Rococo::Windows;

namespace ANON
{
	struct IProperty
	{
		// Represents the property in the panel and returns the containing rectangle in the panel co-ordinates
		// The yOffset parameter determines the y co-ordinate of the top left of the panel
		virtual GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset) = 0;
	};

	struct IPropertySupervisor : IProperty
	{
		virtual void Free() = 0;
	};

	struct StringProperty : IPropertySupervisor
	{
		size_t capacity;
		std::string initialString;
		std::string displayName;

		enum { HARD_CAP = 32767 };

		StringProperty(int _capacity, cstr _displayName, cstr _initialString):
			initialString(_initialString), capacity(32767), displayName(_displayName)
		{
			if (_capacity > 0 && _capacity < HARD_CAP)
			{
				capacity = (size_t) _capacity;
			}
		}

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset)
		{
			Rococo::Windows::AddLabel(panel, GuiRect{ 4, yOffset, 98, yOffset + 20 }, displayName.c_str(), 1, WS_VISIBLE, 0);
			auto* editor = Rococo::Windows::AddEditor(panel, GuiRect{ 100, yOffset, 196, yOffset + 20 },  initialString.c_str(), 2, WS_VISIBLE, 0);
			SendMessageA(*editor, EM_SETLIMITTEXT, max(initialString.length() + 1, capacity), 0);
			GuiRect totalRect{ 4, yOffset + 10, 196, yOffset + 20 };
			return totalRect;
		}
	};

	struct Properties : Rococo::Abedit::IUIPropertiesSupervisor
	{
		IParentWindowSupervisor& panelArea;

		std::vector<IPropertySupervisor*> properties;

		Properties(IParentWindowSupervisor& _panelArea) : panelArea(_panelArea)
		{
			properties.push_back(new StringProperty(12, "Element #1", "Hydrogen"));
			properties.push_back(new StringProperty(12, "Element #2", "Oxygen"));
		}

		~Properties()
		{
			for (auto* p : properties)
			{
				p->Free();
			}
		}

		void Free() override
		{
			delete this;
		}

		void Populate()
		{
			int lastY = 2;

			for (auto* p : properties)
			{
				GuiRect rect = p->AddToPanel(panelArea, lastY);
				lastY = rect.bottom + 2;
			}
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IUIPropertiesSupervisor* CreateProperties(Rococo::Windows::IParentWindowSupervisor& panelArea)
	{
		return new ANON::Properties(panelArea);
	}
}