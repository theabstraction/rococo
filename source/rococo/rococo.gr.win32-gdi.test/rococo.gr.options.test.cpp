#include <rococo.gr.client.h>
#include <rococo.gui.retained.h>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

void RunMessageLoop(IGRClientWindow& client);
void BuildMenus(IGRWidgetMainFrame& frame);
void BuildUpperRightToolbar(IGRWidgetMainFrame& frame);
void UseTestColourScheme(IGRWidgetMainFrame& frame);

void TestOptions(IGRClientWindow& client, IGRSystem& gr)
{
	GRIdWidget mainFrame{ "Main-Frame" };
	auto& frame = gr.BindFrame(mainFrame);
	//frame.SetTitleBarHeight(30);

	frame.ClientArea().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 255, 0, 0), GRGenerateIntensities());

	auto& scheme = gr.Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);

	BuildMenus(frame);

	BuildUpperRightToolbar(frame);

	RunMessageLoop(client);
}

ROCOCO_INTERFACE IInqury
{
	virtual void SetTitle(cstr title) = 0;
};

ROCOCO_INTERFACE IChoiceInquiry: IInqury
{
	virtual void AddChoice(cstr choiceName) = 0;
	virtual void SetActiveChoice(cstr choiceName) = 0;
};

ROCOCO_INTERFACE IBoolInquiry : IInqury
{
	virtual void SetActiveValue(bool value) = 0;
};

ROCOCO_INTERFACE IScalarInquiry : IInqury
{
	virtual void SetActiveValue(double value) = 0;
	virtual void SetRange(double minValue, double maxValue) = 0;
};

enum class EInquiryType
{
	Choice,
	Boolean,
	Scalar
};

template<class T>
struct InquiryFunctionDescriptor
{
	typedef void (T::*FN_ChoiceInquireFunction)(IChoiceInquiry&);
	typedef void (T::*FN_BoolInquireFunction)(IBoolInquiry&);
	typedef void (T::*FN_ScalarInquireFunction)(IScalarInquiry&);

	union
	{
		FN_BoolInquireFunction BoolInquiryFunction = nullptr;
		FN_ChoiceInquireFunction ChoiceInquiryFunction;
		FN_ScalarInquireFunction ScalarInquiryFunction;
	} Functions;

	HString Name;
	EInquiryType type;
	bool isReadOnly;
};

template<class T>
struct OptionSelectedFunctionDescriptor
{
	typedef void (T::*FN_OnBoolOptionSelected)(bool value);
	typedef void (T::*FN_OnScalarOptionSelected)(double value);
	typedef void (T::*FN_OnChoiceSelected)(cstr value);

	union
	{
		FN_OnBoolOptionSelected BoolSelectedFunction = nullptr;
		FN_OnScalarOptionSelected ScalarSelectedFunction;
		FN_OnChoiceSelected ChoiceSelectedFunction;
	} Functions;

	HString Name;
	EInquiryType type;
};

template<class T>
struct OptionDatabase
{
	std::vector<InquiryFunctionDescriptor<T>> inquiryFunctions;
	std::vector<OptionSelectedFunctionDescriptor<T>> optionSelectedFunctions;

	void AddInquiry(cstr name, typename InquiryFunctionDescriptor<T>::FN_BoolInquireFunction inquiry, typename OptionSelectedFunctionDescriptor<T>::FN_OnBoolOptionSelected* onSelect)
	{
		InquiryFunctionDescriptor<T> q;
		q.Functions.BoolInquiryFunction = inquiry;
		q.Name = name;
		q.type = EInquiryType::Bool;
		q.isReadOnly = onSelect == nullptr;
		inquiryFunctions.push_back(q);

		if (onSelect != nullptr)
		{
			OptionSelectedFunctionDescriptor<T> r;
			r.Functions.BoolSelectedFunction = onSelect;
			r.Name = name;
			r.type = q.type;
			optionSelectedFunctions.push_back(r);
		}
	}

	void AddInquiry(cstr name, typename InquiryFunctionDescriptor<T>::FN_ChoiceInquireFunction inquiry, typename OptionSelectedFunctionDescriptor<T>::FN_OnChoiceSelected onSelect)
	{
		InquiryFunctionDescriptor<T> q;
		q.Functions.ChoiceInquiryFunction = inquiry;
		q.Name = name;
		q.type = EInquiryType::Choice;
		q.isReadOnly = onSelect == nullptr;
		inquiryFunctions.push_back(q);

		if (onSelect != nullptr)
		{
			OptionSelectedFunctionDescriptor<T> r;
			r.Functions.ChoiceSelectedFunction = onSelect;
			r.Name = name;
			r.type = q.type;
			optionSelectedFunctions.push_back(r);
		}
	}

	void AddInquiry(cstr name, typename InquiryFunctionDescriptor<T>::FN_ScalarInquireFunction option, typename OptionSelectedFunctionDescriptor<T>::FN_OnScalarOptionSelected onSelect)
	{
		InquiryFunctionDescriptor<T> q;
		q.Functions.ScalarInquiryFunction = inquiry;
		q.Name = name;
		q.type = EInquiryType::Scalar;
		q.isReadOnly = onSelect == nullptr;
		inquiryFunctions.push_back(q);

		if (onSelect != nullptr)
		{
			OptionSelectedFunctionDescriptor<T> r;
			r.Functions.ScalarSelectedFunction = onSelect;
			r.Name = name;
			r.type = type;
			optionSelectedFunctions.push_back(r);
		}
	}
};

ROCOCO_INTERFACE IGameOptionsBuilder
{
	virtual void AddGameOption() = 0;
};

#define ADD_GAME_OPTIONS(BUILDER, FUNCTION_SUFFIX)

struct GraphicsOptions: IGameOptionsBuilder
{
	OptionDatabase<GraphicsOptions> db;

	HString activeScreenMode = "Fullscreen";
	bool isFSAAEnabled = false;
	double musicVolume = 0.25;

	void GetScreenMode(IChoiceInquiry& inquiry)
	{
		inquiry.SetTitle("Screen Mode");
		inquiry.AddChoice("Fullscreen");
		inquiry.AddChoice("Windowed");
		inquiry.AddChoice("Fullscreen Windowed");
		inquiry.SetActiveChoice(activeScreenMode);
	}

	void SetScreenMode(cstr choice)
	{
		activeScreenMode = choice;
	}

	void GetFSAA(IBoolInquiry& inquiry)
	{
		inquiry.SetTitle("Fullscreen Anti-Aliasing");
		inquiry.SetActiveValue(isFSAAEnabled);
	}

	void SetFSAA(bool value)
	{
		isFSAAEnabled = value;
	}

	void GetMusicVolume(IScalarInquiry& inquiry)
	{
		inquiry.SetTitle("Music Volume");
		inquiry.SetRange(0, 100.0);
		inquiry.SetActiveValue(musicVolume);
	}

	void SetMusicVolume(double value)
	{
		musicVolume = value;
	}

	void AddOptions(IGameOptionsBuilder& builder)
	{
		db.AddInquiry("ScreenMode", &GraphicsOptions::GetScreenMode, &GraphicsOptions::SetScreenMode);
	}
};

void TestGameOptions(IGRClientWindow& client)
{
	TestOptions(client, client.GRSystem());
}

void TestWidgets(IGRClientWindow& client)
{
	TestGameOptions(client);
}