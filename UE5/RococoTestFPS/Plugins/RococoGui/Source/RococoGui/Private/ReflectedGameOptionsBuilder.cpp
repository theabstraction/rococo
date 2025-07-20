#include "ReflectedGameOptionsBuilder.h"

#include <rococo.great.sex.h>
#include <rococo.game.options.h>

DECLARE_LOG_CATEGORY_EXTERN(LogReflectedBuilder, Log, All);
DEFINE_LOG_CATEGORY(LogReflectedBuilder);

using namespace Rococo::Game::Options;

namespace Rococo::GreatSex
{
	namespace Implementation
	{
		static const FString addChoicePrefix = FString(TEXT("AddChoice_"));
		static const FString onChoicePrefix = FString(TEXT("OnChoice_"));

		struct ReflectedGameOptions : IGameOptions, IOptionDatabase
		{
			TObjectPtr<UObject> optionObject;
			TArray<UFunction*> addMethods;
			TArray<UFunction*> handlerMethods;
			TArray<char> asciiBuffer;

			ReflectedGameOptions(UObject* object): optionObject(object)
			{

			}

			ReflectedGameOptions(const ReflectedGameOptions& src)
			{
				optionObject = src.optionObject;
			}

			void AddMethod(UFunction* method)
			{
				FString methodName = method->GetFName().ToString();
				UE_LOG(LogReflectedBuilder, Display, TEXT("method %s"), *methodName);

				if (methodName.Len() > asciiBuffer.Num())
				{
					asciiBuffer.SetNum(methodName.Len());
				}

				if (methodName.StartsWith(addChoicePrefix))
				{
					addMethods.Add(method);
				}

				else if (methodName.StartsWith(onChoicePrefix))
				{
					// Add choice to handlerMethods
				}
			}

			const char* GetAsciiTrailingString(const FString& s, const FString& prefix)
			{
				if (!s.StartsWith(prefix))
				{
					return nullptr;
				}

				int j = 0;

				for (int i = prefix.Len(); i < s.Len(); i++, j++)
				{
					asciiBuffer[j] = (char)s[i];
				}

				asciiBuffer[j] = 0;
				return asciiBuffer.GetData();
			}

			void AddOptions(IGameOptionsBuilder& builder) override
			{
				// Enumerate methods and populate builder

				for(auto* method: addMethods)
				{
					FString methodName = method->GetFName().ToString();
					const char* choiceId = GetAsciiTrailingString(methodName, addChoicePrefix);
					if (choiceId)
					{
						auto& c = builder.AddChoice(choiceId);
						// c.AddChoice(choiceName, choiceText, hint);
						// c.SetActiveChoice(choiceName);
						// c.SetHint(hint);
						// c..SetTitle(title);
					}
			}

			IOptionDatabase& DB() override
			{
				return *this;
			}

			void Accept() override
			{
				// invoke accept on the object
			}

			void Revert() override
			{
				// invoke revert on the object
			}

			bool IsModified() const override
			{
				// invoke isModified on the object
				return false;
			}

			void Invoke(cstr name, cstr choice) override
			{
				// invoke OnChoice on the object
			}

			void Invoke(cstr name, bool boolValue) override
			{
				// Invoke OnBool on the object
			}

			void Invoke(cstr name, double scalarValue) override
			{
				// Invoke OnScalar on the object
			}
		};

		struct ReflectedGameOptionsBuilder : IReflectedGameOptionsBuilder
		{
			TMap<UObject*, ReflectedGameOptions> mapObjectToOptions;

			ReflectedGameOptionsBuilder()
			{

			}

			virtual ~ReflectedGameOptionsBuilder()
			{

			}

			void ReflectIntoGenerator(UObject& object, Rococo::GreatSex::IGreatSexGenerator& generator) override
			{
				UClass* objectClass = object.GetClass();

				UE_LOG(LogReflectedBuilder, Display, TEXT("Reflecting %s"), *objectClass->GetFName().ToString());

				ReflectedGameOptions options(&object);
				auto& refOptions = mapObjectToOptions.Add(&object, options);
				generator.AddOptions(refOptions, "didhum");;

				for (TFieldIterator<UFunction> i(objectClass); i; ++i)
				{
					UFunction* method = *i;
					refOptions.AddMethod(method);
				}
			}

			void AddMethods(UObject& object) override
			{

			}

			void Free() override
			{
				delete this;
			}
		};
	}

	ROCOCOGUI_API IReflectedGameOptionsBuilder* CreateReflectedGameOptionsBuilder()
	{
		return new Implementation::ReflectedGameOptionsBuilder();
	}
}