#include "ReflectedGameOptionsBuilder.h"

#include <rococo.great.sex.h>
#include <rococo.game.options.h>
#include <GameOptionBuilder.h>

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

			ReflectedGameOptions(UObject& object) : optionObject(&object)
			{

			}

			ReflectedGameOptions(const ReflectedGameOptions& src)
			{
				optionObject = src.optionObject;
			}

			virtual ~ReflectedGameOptions()
			{

			}

			void OnError(UFunction* method, FProperty* property, FString msg)
			{
				if (optionObject->Implements<URococoGameOptionBuilder>())
				{
					auto builder = TScriptInterface<IRococoGameOptionBuilder>(optionObject);
					builder->Execute_OnError(optionObject, method->GetFName().ToString(), property ? property->GetFName().ToString() : TEXT(""), msg);
				}
			}

			void AddChoiceMethodElseLogError(UFunction* method)
			{
				int index = 0;

				for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i, ++index)
				{
					FProperty* property = *i;

					if (!property->HasAnyPropertyFlags(CPF_Parm))
					{
						// Not a parameter, probably a local variable
						continue;
					}

					if (!property->HasAnyPropertyFlags(CPF_OutParm))
					{
						OnError(method, property, TEXT("Expecting no inputs"));
					}

					if (index == 0)
					{
						auto* sp = CastField<FStrProperty>(property);
						if (!sp)
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type FString"));
							return;
						}
					}
					else if (index == 1)
					{
						auto* sp = CastField<FStructProperty>(property);
						if (!sp)
						{
							OnError(method, property, TEXT("Expecting second method argument to be a struct of type FRococoGameOptionChoice"));
						}

						if (sp->Struct != FRococoGameOptionChoice::StaticStruct())
						{
							// Signature mismatch
							OnError(method, property, TEXT("Expecting first method argument to be of type FRococoGameOptionChoice"));
							return;
						}
					}
					else
					{
						OnError(method, property, TEXT("Expecting only two method arguments"));
						return;
					}
				}

				addMethods.Add(method);
			}

			void AddMethod(UFunction* method)
			{
				if (method->HasAnyFunctionFlags(EFunctionFlags::FUNC_Static))
				{
					OnError(method, nullptr, TEXT("Expecting method to NOT be static"));
					return;
				}

				FString methodName = method->GetFName().ToString();
				UE_LOG(LogReflectedBuilder, Display, TEXT("method %s"), *methodName);

				if (methodName.Len() > asciiBuffer.Num())
				{
					asciiBuffer.SetNum(methodName.Len());
				}

				if (methodName.StartsWith(addChoicePrefix))
				{
					AddChoiceMethodElseLogError(method);
				}
			}

			void ToAscii(char* buffer, size_t capacity, const FString& s)
			{
				if (buffer == nullptr || capacity == 0)
				{
					return;
				}

				if (s.Len() >= capacity)
				{
					snprintf(buffer, capacity, "<truncated!>");
					return;
				}

				int i = 0;
				for (; i < s.Len(); i++)
				{
					buffer[i] = (char)s[i];
				}

				buffer[i] = 0;
			}

			const char* GetVolatileAsciiTrailingString(const FString& s, const FString& prefix)
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

				for (auto* method : addMethods)
				{
					FString methodName = method->GetFName().ToString();
					const char* choiceId = GetVolatileAsciiTrailingString(methodName, addChoicePrefix);
					if (choiceId)
					{
						struct FRococoGameOptionChoicePackage
						{
							FString currentChoice;
							FRococoGameOptionChoice spec;
						};

						FRococoGameOptionChoicePackage args;

						optionObject->ProcessEvent(method, &args);

						auto& c = builder.AddChoice(choiceId);

						for (auto& item : args.spec.Items)
						{
							char choiceName[256];
							ToAscii(choiceName, sizeof choiceName, item.Id);

							char choiceHint[256];
							ToAscii(choiceHint, sizeof choiceHint, item.Hint);

							char choiceText[128];
							ToAscii(choiceText, sizeof choiceText, item.Text);

							c.AddChoice(choiceName, choiceText, choiceHint);
						}

						char currentChoice[128];
						ToAscii(currentChoice, sizeof currentChoice, args.currentChoice);

						c.SetActiveChoice(currentChoice);

						char choiceHint[256];
						ToAscii(choiceHint, sizeof choiceHint, args.spec.Hint);
						c.SetHint(choiceHint);

						char choiceTitle[256];
						ToAscii(choiceTitle, sizeof choiceTitle, args.spec.Title);

						c.SetTitle(choiceTitle);
					}
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

				ReflectedGameOptions options(object);
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