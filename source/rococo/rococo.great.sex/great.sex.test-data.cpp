#include <rococo.great.sex.h>
#include <rococo.game.options.ex.h>
#include <rococo.strings.h>

using namespace Rococo::GreatSex;
using namespace Rococo::Strings;
using namespace Rococo::Game::Options;

static const char* GEN_HINT_FROM_PARENT_AND_CHOICE = "$*$: ";


namespace Rococo::GreatSex::TestData
{
	struct AudioOptions : IGameOptions
	{
		OptionDatabase<AudioOptions> db;

		double musicVolume = 25;
		double fxVolume = 20;
		double narrationVolume = 40;
		HString speakerConfig = "2";

		AudioOptions() : db(*this)
		{

		}

		virtual ~AudioOptions()
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
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
			inquiry.HideBackgroundWhenPopulated(true);
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
			inquiry.HideBackgroundWhenPopulated(true);
		}

		void SetFXVolume(double value)
		{
			fxVolume = value;
		}

		void GetNarrationVolume(IScalarInquiry& inquiry)
		{
			inquiry.SetTitle("Narration Volume");
			inquiry.SetRange(0, 100.0, 0.5);
			inquiry.SetActiveValue(narrationVolume);
			inquiry.SetHint("Set narrator's voice volume. 0 is off, 100.0 is maximum");
			inquiry.SetDecimalPlaces(0);
			inquiry.HideBackgroundWhenPopulated(true);
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

		HString activeScreenMode = "Fullscreen";
		HString shadowQuality = "2";
		HString landscapeQuality = "1";
		HString reflectionAlgorithm = "1";
		HString monitor = "1";
		HString resolution = "1920x1080";
		HString textureQuality = "1";
		HString waterQuality = "1";
		bool isFSAAEnabled = false;

		GraphicsOptions() : db(*this)
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
		}

		void GetScreenMode(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Screen Mode");
			inquiry.AddChoice("Fullscreen", "Fullscreen", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("Windowed", "Windowed", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("Fullscreen Windowed", "Fullscreen Windowed", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(activeScreenMode);
			inquiry.SetHint("Set screen mode");
		}

		void SetScreenMode(cstr choice)
		{
			activeScreenMode = choice;
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

		void GetActiveMonitor(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Active Monitor");
			inquiry.AddChoice("1", "1 - 1920x1080", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2", "2 - 3840x2160", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(monitor);
			inquiry.SetHint("Set which monitor to present the game in fullscreen");
		}

		void SetActiveMonitor(cstr value)
		{
			monitor = value;
		}

		void GetFullscreenResolution(IChoiceInquiry& inquiry)
		{
			inquiry.SetTitle("Fullscreen Resolution");
			inquiry.AddChoice("1.1920x1080", "1920 x 1080 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("2.1920x1200", "1920 x 1200 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("3.2560x1536", "2560 x 1536 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("4.3840x2160", "3840 x 2160 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("5.1366x768", "1366 x 768 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("6.1280x1024", "1280 x 1024 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("7.1024x768", "1024 x 768 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("8.800x600", "800 x 600 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("9.640x480", "640 x 480 60Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("10.1920x1080", "1920 x 1080 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("11.1920x1200", "1920 x 1200 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("12.2560x1536", "2560 x 1536 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("13.3840x2160", "3840 x 2160 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("14.1366x768", "1366 x 768 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("15.1280x1024", "1280 x 1024 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("16.1024x768", "1024 x 768 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("17.800x600", "800 x 600 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("18.640x480", "640 x 480 144Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("19.1920x1080", "1920 x 1080 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("20.1920x1200", "1920 x 1200 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("21.2560x1536", "2560 x 1536 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("22.3840x2160", "3840 x 2160 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("23.1366x768", "1366 x 768 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("24.1280x1024", "1280 x 1024 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("25.1024x768", "1024 x 768 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("26.800x600", "800 x 600 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.AddChoice("27.640x480", "640 x 480 200Hz", GEN_HINT_FROM_PARENT_AND_CHOICE);
			inquiry.SetActiveChoice(resolution);
			inquiry.SetHint("Set full screen resolution and frame rate");
		}

		void SetFullscreenResolution(cstr choice)
		{
			resolution = choice;
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
			ADD_GAME_OPTIONS(db, GraphicsOptions, ScreenMode)
				ADD_GAME_OPTIONS(db, GraphicsOptions, FSAA)
				ADD_GAME_OPTIONS(db, GraphicsOptions, ShadowQuality)
				ADD_GAME_OPTIONS(db, GraphicsOptions, LandscapeQuality)
				ADD_GAME_OPTIONS(db, GraphicsOptions, ReflectionAlgorithm)
				ADD_GAME_OPTIONS(db, GraphicsOptions, ActiveMonitor)
				ADD_GAME_OPTIONS(db, GraphicsOptions, FullscreenResolution)
				ADD_GAME_OPTIONS(db, GraphicsOptions, TextureQuality)
				ADD_GAME_OPTIONS(db, GraphicsOptions, WaterQuality)
				db.Build(builder);
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

		void OnTick(float dt) override
		{
			UNUSED(dt);
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
			inquiry.SetRange(1, 10, 0.5);
			inquiry.SetActiveValue(cursorResponsiveness);
			inquiry.SetHint("Set scaling of mouse movement to cursor movement");
			inquiry.SetDecimalPlaces(1);
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

		HString startDifficulty = "Easy";
		HString gameDifficulty = "Easy";
		HString playerName = "Geoff";

		GameplayOptions() : db(*this)
		{

		}

		virtual ~GameplayOptions()
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
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

		void OnTick(float dt) override
		{
			UNUSED(dt);
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

	IGameOptions& GetUIOptions()
	{
		return s_UIOptions;
	}

	IGameOptions& GetGraphicsOptions()
	{
		return s_GraphicsOptions;
	}

	IGameOptions& GetAudioOptions()
	{
		return s_AudioOptions;
	}

	IGameOptions& GetGameplayOptions()
	{
		return s_gameplayOptions;
	}

	IGameOptions& GetMultiplayerOptions()
	{
		return s_MultiplayerOptions;
	}
}

namespace Rococo::GreatSex
{
	ROCOCO_GREAT_SEX_API void AddTestOptions(IGreatSexGenerator& generator)
	{
		generator.AddOptions(TestData::GetGraphicsOptions(), "GraphicsOptions");
		generator.AddOptions(TestData::GetAudioOptions(), "AudioOptions");
		generator.AddOptions(TestData::GetUIOptions(), "UIOptions");
		generator.AddOptions(TestData::GetGameplayOptions(), "GameplayOptions");
		generator.AddOptions(TestData::GetMultiplayerOptions(), "MultiplayerOptions");
	}
}