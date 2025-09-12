#include "RococoTestFPSGameOptions.h"
#include "RococoUE5.h"
#include <CoreMinimal.h>
#include <rococo.great.sex.h>
#include <rococo.strings.h>
#include <rococo.game.options.ex.h>
#include <RococoGuiAPI.h>
#include <GameOptionBuilder.h>
#include <ReflectedGameOptionsBuilder.h>
#include <GameFramework/GameUserSettings.h>
#include <Logging/LogMacros.h>

DECLARE_LOG_CATEGORY_EXTERN(TestFPSOptions, Error, All);
DEFINE_LOG_CATEGORY(TestFPSOptions);

#ifdef _WIN32
#define local_sscanf sscanf_s
#else
#define local_sscanf sscanf
#endif

using namespace Rococo;
using namespace Rococo::GreatSex;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;

static const char* GEN_HINT_FROM_PARENT_AND_CHOICE = "$*$: ";

namespace Rococo
{
	ROCOCO_API void ConvertFStringToUTF8Buffer(TArray<uint8>& buffer, const FString& src);
}

struct ByRefreshThenWidthAndHeight
{
	bool operator () (const FScreenResolutionRHI& a, const FScreenResolutionRHI& b) const
	{
		if (b.RefreshRate < a.RefreshRate)
		{
			return true;
		}

		if (b.RefreshRate > a.RefreshRate)
		{
			return false;
		}

		if (b.Width < a.Width)
		{
			return true;
		}

		if (b.Width > b.Width)
		{
			return false;
		}

		return b.Height < a.Height;
	}
};

inline bool operator == (const FScreenResolutionRHI& a, const FScreenResolutionRHI& b)
{
	return a.Height == b.Height && a.Width == b.Width && a.RefreshRate == b.RefreshRate;
}

namespace RococoTestFPS::Implementation
{
	struct AudioOptions : IGameOptions
	{
		OptionDatabase<AudioOptions> db;

		double musicVolume = 25;
		double fxVolume = 20;
		double narrationVolume = 40;
		Rococo::Strings::HString speakerConfig = "2";

		AudioOptions() : db(*this)
		{

		}

		virtual ~AudioOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept() override
		{

		}

		void Revert() override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetMusicVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Music Volume");
			inquiry.SetRange(0, 100.0, 1.0);
			inquiry.SetActiveValue(musicVolume);
			inquiry.SetHint("Set music volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
		}

		void SetMusicVolume(double value)
		{
			musicVolume = value;
		}

		void GetFXVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("FX Volume");
			inquiry.SetRange(0, 100.0, 1.0);
			inquiry.SetActiveValue(fxVolume);
			inquiry.SetHint("Set Special FX volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
		}

		void SetFXVolume(double value)
		{
			fxVolume = value;
		}

		void GetNarrationVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Narration Volume");
			inquiry.SetRange(0, 100.0, 1.0);
			inquiry.SetActiveValue(narrationVolume);
			inquiry.SetHint("Set narrator's voice volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
		}

		void SetNarrationVolume(double value)
		{
			narrationVolume = value;
		}

		void GetSpeakerConfiguration(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Speaker Configuration");
			inquiry.AddChoice("2", "2 (Stereo speakers)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2_1", "2.1 (Stereo + Subwoofer)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5_1", "5.1 Dolby Surround", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("7_1", "7.1 Dolby Surround", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2H", "Stereo Headphones", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(speakerConfig);
			inquiry.SetHint("Set sound set-up.");
		}

		void SetSpeakerConfiguration(cstr choice)
		{
			speakerConfig = choice;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, AudioOptions, MusicVolume)
			ADD_GAME_OPTIONS(db, AudioOptions, FXVolume)
			ADD_GAME_OPTIONS(db, AudioOptions, NarrationVolume)
			ADD_GAME_OPTIONS(db, AudioOptions, SpeakerConfiguration)
			db.Build(builder);
		}
	};

	struct GraphicsOptions : IGameOptions
	{
		OptionDatabase<GraphicsOptions> db;

		virtual ~GraphicsOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept() override
		{

		}

		void Revert() override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		Rococo::Strings::HString activeScreenMode = "";
		Rococo::Strings::HString shadowQuality = "2";
		Rococo::Strings::HString landscapeQuality = "1";
		Rococo::Strings::HString reflectionAlgorithm = "1";

		// String representation of the one-based monitor index. Internally monitors are labelled with zero-based indices, so the parsed value should be decremented before use
		Rococo::Strings::HString monitor = "";
		Rococo::Strings::HString resolution = "1920x1080";
		Rococo::Strings::HString textureQuality = "1";
		Rococo::Strings::HString waterQuality = "1";
		bool isFSAAEnabled = false;

		GraphicsOptions() : db(*this)
		{
			
		}

		struct ModeDesc
		{
			cstr shortName;
			cstr desc;
			EWindowMode::Type ue5Type;
		};

		ModeDesc modeDesc[3] = {
			{ "Fullscreen", "Fullscreen", EWindowMode::Fullscreen },
			{ "Windowed", "Windowed", EWindowMode::Windowed },
			{ "FWindowed", "Fullscreen Windowed", EWindowMode::WindowedFullscreen }
		};

		void GetScreenMode(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Screen Mode");

			for (auto& mode : modeDesc)
			{
				inquiry.AddChoice(mode.shortName, mode.desc, GEN_HINT_FROM_PARENT_AND_CHOICE);
			}

			if (activeScreenMode.length() == 0)
			{
				auto& window = *GEngine->GameViewport->GetWindow();
				
				UGameUserSettings* settings = GEngine->GetGameUserSettings();

				for (auto& desc : modeDesc)
				{
					if (desc.ue5Type == settings->GetFullscreenMode())
					{
						activeScreenMode = desc.shortName;
						break;
					}
				}
			}

			inquiry.SetActiveChoice(activeScreenMode);
			inquiry.SetHint("Set screen mode");
		}

		void SetScreenMode(cstr choice)
		{
			activeScreenMode = choice;

			for (auto& mode : modeDesc)
			{
				if (Eq(mode.shortName, choice))
				{
					modality = mode.ue5Type;
					break;
				}
			}

			FMonitorInfo targetMonitor;
			if (TryGetCurrentTargetMonitor(OUT targetMonitor))
			{
				SyncVideoModeToCurrentTargetResolution(&targetMonitor);
			}
		}

		void GetFSAA(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("Fullscreen Anti-Aliasing");
			inquiry.SetActiveValue(isFSAAEnabled);
			inquiry.SetHint("Enable or disable full screen anti-aliasing");
		}

		void SetFSAA(bool value)
		{
			isFSAAEnabled = value;
		}

		void GetShadowQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Shadow Quality");
			inquiry.AddChoice("1", "512 x 512 1pt (low)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "1024 x 1024 4pt (medium)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "1024 x 1024 16pt (high)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "2048 x 2048 4pt (very high)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "2048 x 2048 16pt (ultra)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(shadowQuality);
			inquiry.SetHint("Set the quality of shadows. Higher settings may reduce frame-rate");
		}

		void SetShadowQuality(cstr value)
		{
			shadowQuality = value;
		}

		void GetLandscapeQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Landscape Quality");
			inquiry.AddChoice("1", "Low", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Medium", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "High", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Ultra", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Ultra (Experimental)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(landscapeQuality);
			inquiry.SetHint("Set the quality of landscape rendering. Higher settings may reduce frame-rate");
		}

		void SetLandscapeQuality(cstr value)
		{
			landscapeQuality = value;
		}

		void GetReflectionAlgorithm(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Reflection Algorithm");
			inquiry.AddChoice("1", "Gloss (Minimal)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Global Environmental Mapping (Low)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "Local Environmental Mapping (Medium)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Dynamic Environmental Mapping (High)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Raytracing (Ultra)", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(reflectionAlgorithm);
			inquiry.SetHint("Set the quality of reflections. Higher settings may reduce frame-rate");
		}

		void SetReflectionAlgorithm(cstr value)
		{
			reflectionAlgorithm = value;
		}


		// Initialize the monitor value and returns the zero-based index of the active monitor
		int SyncMonitorStateToActiveMonitor()
		{
			if (monitor.length() == 0)
			{
				int activeMonitor = GetMonitorByConfig();
				if (activeMonitor >= 0)
				{
					Format(monitor, "%d", activeMonitor + 1);
					return activeMonitor;
				}

				FDisplayMetrics displayMetrics;
				FSlateApplication::Get().GetInitialDisplayMetrics(OUT displayMetrics);

				auto& window = *GEngine->GameViewport->GetWindow();

				for (int i = 0; i < displayMetrics.MonitorInfo.Num(); i++)
				{
					const FMonitorInfo& m = displayMetrics.MonitorInfo[i];
					if (window.GetPositionInScreen() == FVector2D(m.WorkArea.Left, m.WorkArea.Top))
					{
						Format(monitor, "%d", i + 1);
						return i;
					}
				}
			}

			monitor = "1";
			return 0;
		}

		enum { MAX_MONITORS = 8 }; // Keep things sane. In 2075, when the average number of monitors on a PC > 8 think about updating this

		// Returns -1 on failure
		int GetMonitorByConfig()
		{
			FDisplayMetrics displayMetrics;
			FSlateApplication::Get().GetInitialDisplayMetrics(displayMetrics);

			int targetIndex = -1;

			FString configsActiveMonitorName;
			bool hasName = GConfig->GetString(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorName"), REF configsActiveMonitorName, GGameIni);

			if (hasName)
			{
				for (int i = 0; i < displayMetrics.MonitorInfo.Num() && i < MAX_MONITORS; i++)
				{
					auto& m = displayMetrics.MonitorInfo[i];
					if (m.Name == configsActiveMonitorName)
					{
						if (targetIndex == -1)
						{
							targetIndex = i;
						}
						else
						{
							// Duplicate, so the name does not uniquely identify a monitor, it is of no use to us
							targetIndex = -1;
							break;
						}
					}
				}
			}

			if (targetIndex != -1)
			{
				return targetIndex;
			}

			int configsActiveMonitorIndex;
			bool hasIndex = GConfig->GetInt(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorIndex"), REF configsActiveMonitorIndex, GGameIni);

			if (hasIndex)
			{
				if (displayMetrics.MonitorInfo.IsValidIndex(configsActiveMonitorIndex))
				{
					return configsActiveMonitorIndex;
				}
			}

			return -1;
		}

		void GetActiveMonitor(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Active Monitor");

			FDisplayMetrics displayMetrics;
			FSlateApplication::Get().GetInitialDisplayMetrics(displayMetrics);

			enum { MAX_MONITORS = 8 };

			for (int i = 0; i < displayMetrics.MonitorInfo.Num() && i < MAX_MONITORS; i++)
			{
				auto& m = displayMetrics.MonitorInfo[i];

				char choiceName[8];
				SafeFormat(choiceName, "%d", i + 1);

				TArray<uint8> buffer;
				Rococo::ConvertFStringToUTF8Buffer(buffer, m.Name);

				char desc[64];
				SafeFormat(desc, "%d - %dx%d [%s]", i + 1, m.NativeWidth, m.NativeHeight, buffer.GetData());

				inquiry.AddChoice(choiceName, desc, GEN_HINT_FROM_PARENT_AND_CHOICE);
			}

			SyncMonitorStateToActiveMonitor();

			inquiry.SetActiveChoice(monitor);
			inquiry.SetHint("Set which monitor to present the game in fullscreen");
		}

		void UpdateFullscreen(const FMonitorInfo& selectedMonitor)
		{
			UGameUserSettings* settings = GEngine->GetGameUserSettings();
			if (settings->GetFullscreenMode() == EWindowMode::Fullscreen)
			{
				FIntPoint currentResolution = settings->GetScreenResolution();

				if (selectedMonitor.NativeWidth < (int) currentTargetResolution.Width || selectedMonitor.NativeHeight < (int) currentTargetResolution.Height)
				{
					currentTargetResolution.Width = selectedMonitor.NativeWidth;
					currentTargetResolution.Height = selectedMonitor.NativeHeight;
					currentTargetResolution.RefreshRate = DEFAULT_REFRESH_RATE;
				}

				SyncVideoModeToCurrentTargetResolution(&selectedMonitor);
			}
		}

		void SyncMonitorToIndex(int monitorIndex, bool saveToConfig)
		{
			FDisplayMetrics displayMetrics;
			FSlateApplication::Get().GetInitialDisplayMetrics(displayMetrics);

			if (displayMetrics.MonitorInfo.IsValidIndex(monitorIndex))
			{
				const FMonitorInfo& targetMonitor = displayMetrics.MonitorInfo[monitorIndex];

				if (saveToConfig)
				{
					GConfig->SetString(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorName"), *targetMonitor.Name, GGameIni);
					GConfig->SetInt(TEXT("/Script/Rococo.Graphics"), TEXT("ActiveMonitorIndex"), monitorIndex, GGameIni);
					GConfig->Flush(false, GGameIni);
				}

				auto& window = *GEngine->GameViewport->GetWindow();
				window.MoveWindowTo(FVector2D(targetMonitor.DisplayRect.Left, targetMonitor.DisplayRect.Top));
				window.Resize(FVector2D(targetMonitor.NativeWidth, targetMonitor.NativeHeight));

				UpdateFullscreen(targetMonitor);
			}
		}

		void SetActiveMonitor(cstr value)
		{
			if (!Eq(monitor, value))
			{
				monitor = value;

				int monitorIndex = value[0] - 49;
				SyncMonitorToIndex(monitorIndex, true);
			}

		}

		bool TrySetCurrentTo(int width, int height, int hz, const FScreenResolutionArray& knownResolutions, REF IChoiceInquiry& inquiry)
		{
			int index = 0;
			for (auto& r : knownResolutions)
			{
				if (r.Width == width && r.Height == height && r.RefreshRate == hz)
				{
					currentTargetResolution.Width = width;
					currentTargetResolution.Height = height;
					currentTargetResolution.RefreshRate = hz;

					char choiceName[16];
					SafeFormat(choiceName, "%d %d %d", index + 1, r.Width, r.Height);
					inquiry.SetActiveChoice(choiceName);
					return true;
				}

				index++;
			}

			return false;
		}

		const enum {DEFAULT_REFRESH_RATE = 60, DEFAULT_HREZ = 1920, DEFAULT_VREZ = 1080};

		void ComputeDefaults(const FScreenResolutionArray& knownResolutions, REF IChoiceInquiry& inquiry)
		{
			FDisplayMetrics m;
			FDisplayMetrics::RebuildDisplayMetrics(OUT m);

			currentTargetResolution.Width = m.PrimaryDisplayWidth;
			currentTargetResolution.Height = m.PrimaryDisplayHeight;
			currentTargetResolution.RefreshRate = 60;

			if (TrySetCurrentTo(m.PrimaryDisplayWidth, m.PrimaryDisplayHeight, DEFAULT_REFRESH_RATE, knownResolutions, inquiry))
			{
				return;
			}

			if (TrySetCurrentTo(DEFAULT_HREZ, DEFAULT_VREZ, DEFAULT_REFRESH_RATE, knownResolutions, inquiry))
			{
				return;
			}

			char defaultText[32];
			SafeFormat(defaultText, "%d x %d %dHz", DEFAULT_HREZ, DEFAULT_VREZ, DEFAULT_REFRESH_RATE);

			cstr defaultChoice = "default";
			inquiry.AddChoice(defaultChoice, defaultText, GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(defaultChoice);
		}

		FScreenResolutionRHI currentTargetResolution;
		TArray<FScreenResolutionRHI> resolutions;

		void GetFullscreenResolution(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Fullscreen Resolution");

			TArray<FScreenResolutionRHI> duplicatedResolutions;
			resolutions.Empty();

			ByRefreshThenWidthAndHeight byRefreshThenWidthAndHeight;
			
			bool ignoreRefreshRate = false;

			int currentIndex = -1;

			const int minHRez = 1280;
			const int minHz = 59;

			if (RHIGetAvailableResolutions(duplicatedResolutions, ignoreRefreshRate))
			{
				FMonitorInfo targetMonitor;
				if (TryGetCurrentTargetMonitor(OUT targetMonitor))
				{
					duplicatedResolutions.RemoveAll(
						[&targetMonitor]
						(const FScreenResolutionRHI& r) -> bool
						{
							return r.Width > (uint32) targetMonitor.NativeWidth || r.Height > (uint32) targetMonitor.NativeHeight;
						}
					);
				}

				for (auto& r : duplicatedResolutions)
				{
					resolutions.AddUnique(r);
				}

				resolutions.Sort(byRefreshThenWidthAndHeight);

				int index = 0;
				for (const FScreenResolutionRHI& r : resolutions)
				{
					char choiceName[16];
					SafeFormat(choiceName, "%d %d %d", index, r.Width, r.Height);

					char choiceDesc[32];
					SafeFormat(choiceDesc, "%d x %d %dHz", r.Width, r.Height, r.RefreshRate);

					if (Eq(resolution, choiceName))
					{
						currentIndex = index;
					}

					index++;

					if (r.Width >= minHRez && r.RefreshRate >= minHz)
					{
						inquiry.AddChoice(choiceName, choiceDesc, GEN_HINT_FROM_PARENT_AND_CHOICE);
					}
				}
			}

			if (currentIndex == -1)
			{
				ComputeDefaults(IN resolutions, REF inquiry);
			}

			inquiry.SetHint("Set full screen resolution and frame rate");
		}

		[[nodiscard]] bool TryGetCurrentTargetMonitor(OUT FMonitorInfo& currentMonitor)
		{
			FDisplayMetrics displayMetrics;
			FSlateApplication::Get().GetInitialDisplayMetrics(displayMetrics);

			auto& window = *GEngine->GameViewport->GetWindow();

			for (const FMonitorInfo& m : displayMetrics.MonitorInfo)
			{
				if (window.GetPositionInScreen() == FVector2D(m.WorkArea.Left, m.WorkArea.Top))
				{
					currentMonitor = m;
					return true;
				}
			}

			for (const FMonitorInfo& m : displayMetrics.MonitorInfo)
			{
				if (m.bIsPrimary)
				{
					currentMonitor = m;
					return true;
				}
			}

			return false;
		}

		void SetFullscreenResolution(cstr choice)
		{
			resolution = choice;

			currentTargetResolution.Width = DEFAULT_HREZ;
			currentTargetResolution.Height = DEFAULT_VREZ;
			currentTargetResolution.RefreshRate = DEFAULT_REFRESH_RATE;

			if (!Eq(choice, "default"))
			{
				int index;
				int width;
				int height;

				if (3 == local_sscanf(choice, "%d%d%d", &index, &width, &height))
				{
					if (index >= 0 && index < resolutions.Num())
					{
						const auto& r = resolutions[index];
						if (r.Width == width && r.Height == height)
						{
							currentTargetResolution.Width = width;
							currentTargetResolution.Height = height;
							currentTargetResolution.RefreshRate = r.RefreshRate;
						}
					}
				}
			}

			FMonitorInfo currentMonitor;
			if (TryGetCurrentTargetMonitor(OUT currentMonitor))
			{
				SyncVideoModeToCurrentTargetResolution(&currentMonitor);
			}
		}

		EWindowMode::Type modality = EWindowMode::WindowedFullscreen;

		void SyncVideoModeToCurrentTargetResolution(const FMonitorInfo* selectedMonitor)
		{
			if (selectedMonitor != nullptr)
			{
				if (selectedMonitor->NativeWidth < (int) currentTargetResolution.Width || selectedMonitor->NativeHeight < (int) currentTargetResolution.Height)
				{
					currentTargetResolution.Width = selectedMonitor->NativeWidth;
					currentTargetResolution.Height = selectedMonitor->NativeHeight;
				}
			}			

			UGameUserSettings* settings = GEngine->GetGameUserSettings();
			settings->SetScreenResolution(FIntPoint(currentTargetResolution.Width, currentTargetResolution.Height));
			settings->SetFrameRateLimit(currentTargetResolution.RefreshRate);
			settings->SetFullscreenMode(modality);
			settings->ApplySettings(true);
			settings->ConfirmVideoMode();
			settings->RequestUIUpdate();
		}

		void GetTextureQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Texture Quality");
			inquiry.AddChoice("1", "Low 256x256", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Medium 512x512", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "High 1024x1024 ", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Very High 2048x2048", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Ultra 4096x2096", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(textureQuality);
			inquiry.SetHint("Set the texture sizes. Higher settings may slow frame rate");
		}

		void SetTextureQuality(cstr choice)
		{
			textureQuality = choice;
		}

		void GetWaterQuality(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Water Quality");
			inquiry.AddChoice("1", "Low - flat", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "Medium - ripples", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3", "High - ripples & refraction", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4", "Very High - full physics", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5", "Ultra - physics and reflections", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(waterQuality);
			inquiry.SetHint("Set the quality of water rendering. Higher settings may slow frame rate");
		}

		void SetWaterQuality(cstr choice)
		{
			waterQuality = choice;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, GraphicsOptions, ActiveMonitor)
			ADD_GAME_OPTIONS(db, GraphicsOptions, ScreenMode)
			ADD_GAME_OPTIONS(db, GraphicsOptions, FullscreenResolution)
			ADD_GAME_OPTIONS(db, GraphicsOptions, FSAA)
			ADD_GAME_OPTIONS(db, GraphicsOptions, ShadowQuality)
			ADD_GAME_OPTIONS(db, GraphicsOptions, LandscapeQuality)
			ADD_GAME_OPTIONS(db, GraphicsOptions, ReflectionAlgorithm)
			ADD_GAME_OPTIONS(db, GraphicsOptions, TextureQuality)
			ADD_GAME_OPTIONS(db, GraphicsOptions, WaterQuality)
			db.Build(builder);
		}

		void SyncGraphicsToConfig()
		{
			if (!GEngine->GameViewport || !GEngine->GameViewport->GetWindow())
			{
				UE_LOG(TestFPSOptions, Error, TEXT("%hs: viewport window does not exist at time of invocation"), __func__);
				return;
			}

			int monitorIndex = SyncMonitorStateToActiveMonitor();

			// Config has determined our index, so we do not have to save it
			SyncMonitorToIndex(monitorIndex, false);
		}
	};

	struct UIOptions : IGameOptions
	{
		OptionDatabase<UIOptions> db;

		double cursorResponsiveness = 3.0;
		bool isYAxisInverted = false;

		UIOptions() : db(*this)
		{

		}

		virtual ~UIOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept() override
		{

		}

		void Revert() override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetCursorResponsiveness(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Mouse Sensitivity");
			inquiry.SetRange(1, 10, 1);
			inquiry.SetActiveValue(cursorResponsiveness);
			inquiry.SetHint("Set scaling of mouse movement to cursor movement");
		}

		void SetCursorResponsiveness(double value)
		{
			cursorResponsiveness = value;
		}

		void GetInvertYAxis(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("Invert Y-Axis");
			inquiry.SetActiveValue(isYAxisInverted);
			inquiry.SetHint("Reverse the response to player ascent to the joystick direction");
		}

		void SetInvertYAxis(bool value)
		{
			isYAxisInverted = value;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, UIOptions, CursorResponsiveness)
			ADD_GAME_OPTIONS(db, UIOptions, InvertYAxis)
			db.Build(builder);
		}
	};

	struct GameplayOptions : IGameOptions
	{
		OptionDatabase<GameplayOptions> db;

		Rococo::Strings::HString startDifficulty = "Easy";
		Rococo::Strings::HString gameDifficulty = "Easy";
		Rococo::Strings::HString playerName = "Geoff";

		GameplayOptions() : db(*this)
		{

		}

		virtual ~GameplayOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept() override
		{

		}

		void Revert() override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetStartingDifficulty(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Starting Difficulty");
			inquiry.AddChoice("Easy", "Easy", "Recruits cannot die during training");
			inquiry.AddChoice("Medium", "Medium", "Recruits have three lives to complete training");
			inquiry.AddChoice("Hard", "Realistic", "Recruits can die during training");
			inquiry.AddChoice("Ironman", "Ironman", "No save game slots during training");

			inquiry.SetActiveChoice(startDifficulty);
			inquiry.SetHint("Set the difficulty of the training mission");
		}

		void SetStartingDifficulty(cstr value)
		{
			startDifficulty = value;
		}

		void GetGameDifficulty(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Game Difficulty");
			inquiry.AddChoice("Easy", "Easy", "On death you respawn at the last checkpoint");
			inquiry.AddChoice("Medium", "Medium", "You respawn on death, but lose 25% xp");
			inquiry.AddChoice("Hard", "Realistic", "No respawns on death");
			inquiry.AddChoice("Ironman", "Ironman", "Save game deleted on death... and no respawn on death");

			inquiry.SetActiveChoice(gameDifficulty);
			inquiry.SetHint("Set the difficulty of the main game");
		}

		void SetGameDifficulty(cstr value)
		{
			gameDifficulty = value;
		}

		void SetPlayerName(cstr value)
		{
			if (*value == 0)
			{
				playerName = "Geoff";
				return;
			}

			playerName = value;
		}

		void GetPlayerName(IStringInquiry& inquiry)
		{
			inquiry.SetTitle("Player Name");
			inquiry.SetActiveValue(playerName);
			inquiry.SetHint("Set the name of your player's avatar");
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, GameplayOptions, StartingDifficulty)
			ADD_GAME_OPTIONS(db, GameplayOptions, GameDifficulty)
			ADD_GAME_OPTIONS_STRING(db, GameplayOptions, PlayerName, 32)

			db.Build(builder);
		}
	};

	struct MultiplayerOptions : IGameOptions
	{
		OptionDatabase<MultiplayerOptions> db;

		bool hostGame = false;
		bool useUDP = true;

		MultiplayerOptions() : db(*this)
		{

		}

		virtual ~MultiplayerOptions()
		{

		}

		IOptionDatabase& DB() override
		{
			return db;
		}

		void Accept() override
		{

		}

		void Revert() override
		{

		}

		bool IsModified() const override
		{
			return true;
		}

		void GetHostGame(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("Host Game");
			inquiry.SetActiveValue(hostGame);
		}

		void SetHostGame(bool value)
		{
			hostGame = value;
		}

		void GetUseUDP(IBoolInquiry& inquiry)
		{
			inquiry.SetTitle("UDP");
			inquiry.SetActiveValue(useUDP);
		}

		void SetUseUDP(bool value)
		{
			useUDP = value;
		}

		void AddOptions(IGameOptionsBuilder& builder) override
		{
			ADD_GAME_OPTIONS(db, MultiplayerOptions, HostGame)
			ADD_GAME_OPTIONS(db, MultiplayerOptions, UseUDP)
			db.Build(builder);
		}
	};

	GraphicsOptions s_GraphicsOptions;
	AudioOptions s_AudioOptions;
	UIOptions s_UIOptions;
	GameplayOptions s_gameplayOptions;
	MultiplayerOptions s_MultiplayerOptions;
}

namespace RococoTestFPS
{
	void PrepGenerator(IReflectedGameOptionsBuilder& optionsBuilder, const TArray<UObject*>& context, Rococo::GreatSex::IGreatSexGenerator& generator)
	{
		using namespace RococoTestFPS::Implementation;

		generator.AddOptions(s_GraphicsOptions, "GraphicsOptions");
		generator.AddOptions(s_AudioOptions, "AudioOptions");
		generator.AddOptions(s_UIOptions, "UIOptions");
		generator.AddOptions(s_gameplayOptions, "GameplayOptions");
		generator.AddOptions(s_MultiplayerOptions, "MultiplayerOptions");

		for (auto* object : context)
		{
			if (object->Implements<URococoGameOptionBuilder>())
			{
				auto builder = TScriptInterface<IRococoGameOptionBuilder>(object);
				FString optionCategory = builder->Execute_GetOptionId(object);
				optionsBuilder.ReflectIntoGenerator(*object, optionCategory, generator);
			}
		}
	}

	void InitGlobalOptions()
	{
		Rococo::Gui::SetGlobalPrepGenerator(PrepGenerator);
	}
}

double URococoTestFPSGameOptionsLibrary::GetMusicVolume()
{
	return RococoTestFPS::Implementation::s_AudioOptions.musicVolume / 100.0;
}

double URococoTestFPSGameOptionsLibrary::GetNarrationVolume()
{
	return RococoTestFPS::Implementation::s_AudioOptions.narrationVolume / 100.0;
}

double URococoTestFPSGameOptionsLibrary::GetFXVolume()
{
	return RococoTestFPS::Implementation::s_AudioOptions.fxVolume / 100.0;
}

void URococoTestFPSGameOptionsLibrary::SyncGraphicsToConfig()
{
	RococoTestFPS::Implementation::s_GraphicsOptions.SyncGraphicsToConfig();
}