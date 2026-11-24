// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.

// The SexyStudio workspace: projects (sexystudio.config.sexml), their databases, the blank defaults and the user defaults file.
// This is part of the GUI-free database layer: it must not include or forward declare window, widget or IDE types. Errors are thrown or reported to handlers, never shown.

#include "sexystudio.api.h"
#include "sexystudio.workspace.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.functional.h>
#include <rococo.sexml.h>
#include <rococo.hashtable.h>
#include <vector>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

namespace Rococo::SexyStudio
{
	namespace
	{
		cstr CONFIG_FILENAME = "sexystudio.config.sexml";
		cstr CONFIG_DEFAULT_FULLNAME = "Config.Default.FullName";

		struct SearchPathDesc
		{
			HString pingPath;
			bool isActive;
		};

		// The default config values: what the workspace presents when no project is selected. No search paths, no packages, no config file
		struct BlankConfig final : IFactoryConfig
		{
			cstr GetPackage(size_t index) const override
			{
				UNUSED(index);
				return nullptr;
			}

			cstr GetProjectPath() const override
			{
				return "";
			}

			crwstr ConfigPath() const override
			{
				return nullptr;
			}

			SearchPathDescAtom GetSearchPath(size_t index) const override
			{
				UNUSED(index);
				return { nullptr, false };
			}

			void SetSearchPathActivity(size_t index, bool isActive) override
			{
				UNUSED(index);
				UNUSED(isActive);
			}

			void SetPackageSelected(cstr pingPath, bool isSelected) override
			{
				UNUSED(pingPath);
				UNUSED(isSelected);
			}
		};

		// A project is a sexystudio.config.sexml: content directory, project path, search paths and selected packages, and the database populated from them
		struct Project final : IFactoryConfig
		{
			AutoFree<ISexyDatabaseSupervisor> database;
			HString content;
			HString projectPath;
			WideFilePath configPath;
			std::vector<SearchPathDesc> searchPaths;
			std::vector<HString> packages;

			// Loads the config. Throws if it cannot be parsed or its content directory does not exist. Does not create or populate the database
			Project(cstr sexyStudioConfigSexml)
			{
				Format(configPath, L"%hs", sexyStudioConfigSexml);

				Rococo::OS::LoadSXMLBySysPath(sexyStudioConfigSexml,
					[this, sexyStudioConfigSexml](const ISEXMLDirectiveList& topLevelDirectives)
					{
						size_t startIndex = 0;
						auto& dDirectories = GetDirective(topLevelDirectives, "Directories", IN OUT startIndex);

						content = AsString(dDirectories["content"]).c_str();

						const fstring here = "$HERE$"_fstring;

						// We expect in many cases the config sexml will be in a descendant of the content directory
						// The container directory for the config sexml is given the macro $HERE$,
						// thus if content is say $ROCOCO$content and the config is $ROCOCO$content/config/sexystudio.config.sexml, then $HERE$ will map to $ROCOCO$content/config/,
						// so the content directory will be representable with $HERE$../

						if (StartsWith(content, here))
						{
							U8FilePath wHere;
							Assign(wHere, sexyStudioConfigSexml);
							IO::MakeContainerDirectory(wHere.buf);

							U8FilePath expandedContent;
							Format(expandedContent, "%s%s", wHere.buf, content.c_str() + here.length);

							IO::NormalizePath(expandedContent);

							content = expandedContent;
						}

						projectPath = AsString(dDirectories["project"]).c_str();

						searchPaths.clear();

						size_t searchIndex = 0;
						while (auto* pSearchPath = FindDirective(dDirectories.Children(), "SearchPath", IN OUT searchIndex))
						{
							auto& dSearchPath = *pSearchPath;
							cstr path = AsString(dSearchPath["path"]).c_str();
							bool isActive = AsBool(dSearchPath["isActive"]);
							searchPaths.push_back({ path, isActive });
						}

						auto& dPackages = GetDirective(topLevelDirectives, "Packages", IN OUT startIndex);
						auto& aPackages = AsStringList(dPackages["selected-packages"]);
						for (int i = 0; i < (int) aPackages.NumberOfElements(); i++)
						{
							fstring path = aPackages[i];
							packages.push_back((cstr)path);
						}
					}
				);

				if (!IO::IsDirectory(content))
				{
					Throw(0, "Expecting Directories/content in %s to be an existant directory.\nDirectory %s does not exist", sexyStudioConfigSexml, content.c_str());
				}
			}

			void Free()
			{
				delete this;
			}

			crwstr ConfigPath() const override
			{
				return configPath;
			}

			cstr GetProjectPath() const override
			{
				return projectPath;
			}

			SearchPathDescAtom GetSearchPath(size_t index) const override
			{
				if (index >= searchPaths.size())
				{
					return { nullptr, false };
				}

				auto& i = searchPaths[index];
				return { i.pingPath.c_str(), i.isActive };
			}

			void SetSearchPathActivity(size_t index, bool isActive) override
			{
				if (index < searchPaths.size())
				{
					searchPaths[index].isActive = isActive;
				}
			}

			cstr GetPackage(size_t index) const override
			{
				if (index >= packages.size())
				{
					return nullptr;
				}

				return packages[index].c_str();
			}

			void SetPackageSelected(cstr pingPath, bool isSelected) override
			{
				auto i = std::find_if(packages.begin(), packages.end(), [pingPath](const HString& p) { return Eq(p.c_str(), pingPath); });
				if (isSelected && i == packages.end())
				{
					packages.push_back(pingPath);
				}
				else if (!isSelected && i != packages.end())
				{
					packages.erase(i);
				}
			}

			// Writes the config back to its file. Throws on error. See the constructor for an explanation of macro $HERE$
			void Save()
			{
				crwstr wConfigPath = configPath;
				Rococo::OS::SaveSXMLBySysPath(wConfigPath, true,
					[this, wConfigPath](ISEXMLBuilder& sb)
					{
						sb.AddDirective("Directories");

						U8FilePath u8ConfigPath;
						Format(u8ConfigPath, "%ls", wConfigPath);

						if (StartsWith(u8ConfigPath.buf, content))
						{
							U8FilePath macroedContentPath;
							StackStringBuilder macroBuilder(macroedContentPath.buf, U8FilePath::CAPACITY);
							macroBuilder << "$HERE$";

							U8FilePath reducedConfigPath = u8ConfigPath;
							IO::MakeContainerDirectory(reducedConfigPath.buf);

							for (;;)
							{
								if (!IO::MakeContainerDirectory(reducedConfigPath.buf))
								{
									break;
								}

								if (!StartsWith(reducedConfigPath.buf, content))
								{
									break;
								}

								macroBuilder << "../";
							}

							sb.AddStringLiteral("content", macroedContentPath.buf);
						}
						else
						{
							sb.AddStringLiteral("content", content);
						}

						sb.AddStringLiteral("project", projectPath);
						for (auto& path : searchPaths)
						{
							sb.AddDirective("SearchPath");
							sb.AddStringLiteral("path", path.pingPath);
							sb.AddAtomicAttribute("isActive", path.isActive);
							sb.CloseDirective();
						}
						sb.CloseDirective(); // directories

						sb.AddDirective("Packages");
						sb.OpenListAttribute("selected-packages");

						for (auto& pck : packages)
						{
							sb.AddEscapedStringToList(pck);
						}

						sb.CloseListAttribute(); // selected-packages
						sb.CloseDirective();
					}
				);
			}
		};

		struct SexyWorkspace final : ISexyWorkspace
		{
			HString defaultsSection;
			BlankConfig blankConfig;
			AutoFree<ISexyDatabaseSupervisor> blankDatabase;
			std::vector<AutoFree<Project>> projects;

			// Maps a config path or a .sxy path to the index of its project, or -1 if no project could be found for a .sxy file
			stringmap<int> mapFilenameToProjectIndex;

			int currentProjectIndex = -1;

			std::vector<IWorkspaceObserver*> observers;

			SexyWorkspace(cstr _defaultsSection) : defaultsSection(_defaultsSection), blankDatabase(CreateSexyDatabase(blankConfig))
			{
			}

			void Free() override
			{
				delete this;
			}

			ISexyDatabase& GetDatabase() override
			{
				return (currentProjectIndex == -1) ? *blankDatabase : *projects[currentProjectIndex]->database;
			}

			const ISexyDatabase& GetDatabase() const override
			{
				return (currentProjectIndex == -1) ? *blankDatabase : *projects[currentProjectIndex]->database;
			}

			bool HasProject() const override
			{
				return currentProjectIndex >= 0;
			}

			void Notify(bool contentIsNew)
			{
				// Copy, so that an observer removed by another observer's response does not invalidate the iteration
				auto targets = observers;
				for (auto* o : targets)
				{
					o->OnWorkspaceChanged(GetDatabase(), contentIsNew);
				}
			}

			void AddObserver(IWorkspaceObserver& observer) override
			{
				if (std::find(observers.begin(), observers.end(), &observer) == observers.end())
				{
					observers.push_back(&observer);
				}

				observer.OnWorkspaceChanged(GetDatabase(), HasProject());
			}

			void RemoveObserver(IWorkspaceObserver& observer) override
			{
				auto i = std::remove(observers.begin(), observers.end(), &observer);
				observers.erase(i, observers.end());
			}

			// Selects the project, telling the observers if it changed. [contentIsNew] is true when the project was loaded just now
			void Select(int projectIndex, bool contentIsNew)
			{
				if (projectIndex == -1)
				{
					return;
				}

				if (projectIndex != currentProjectIndex)
				{
					currentProjectIndex = projectIndex;
					Notify(contentIsNew);
				}
			}

			int FindProjectIndexByRoot(cstr filename) const
			{
				for (int index = 0; index < (int)projects.size(); index++)
				{
					if (StartsWith(filename, projects[index]->content))
					{
						return index;
					}
				}

				return -1;
			}

			int FindProjectIndexByConfig(cstr configFilename) const
			{
				for (int index = 0; index < (int)projects.size(); index++)
				{
					U8FilePath projectConfig;
					Assign(projectConfig, projects[index]->configPath);
					if (EqI(configFilename, projectConfig))
					{
						return index;
					}
				}

				return -1;
			}

			// Loads the config, creates the project's database and populates it. Throws if the config cannot be loaded. Returns the new project index
			int AddNewProjectByConfig(cstr sexyStudioConfigSexml)
			{
				AutoFree<Project> project = new Project(sexyStudioConfigSexml);
				project->database = CreateSexyDatabase(*project);
				project->database->SetContentPath(project->content); // This populates the database, once
				projects.push_back(std::move(project));
				return (int)(projects.size() - 1);
			}

			// Searches the containers of the file for a sexystudio.config.sexml and loads the first found. Returns -1 if none could be found or loaded
			int AddNewProjectForFile(cstr fullPathToFile)
			{
				U8FilePath u8Path;
				CopyString(u8Path.buf, u8Path.CAPACITY, fullPathToFile);

				Substring s = Substring::ToSubstring(u8Path);

				// Recurse through container directories looking for sexystudio.config.sexml
				for (;;)
				{
					cstr lastSlash = Strings::ReverseFind(Rococo::IO::DirectorySeparatorChar(), s);
					if (!lastSlash)
					{
						break;
					}

					s = { s.start, lastSlash };

					*const_cast<char*>(lastSlash) = 0;

					U8FilePath sexyStudioConfig;
					Format(sexyStudioConfig, "%s%c%s", s.start, Rococo::IO::DirectorySeparatorChar(), CONFIG_FILENAME);

					if (!IO::IsFileExistant(sexyStudioConfig))
					{
						continue;
					}

					int index = FindProjectIndexByConfig(sexyStudioConfig);
					if (index >= 0)
					{
						return index;
					}

					try
					{
						return AddNewProjectByConfig(sexyStudioConfig);
					}
					catch (...)
					{
						// Auto-complete rebases silently: a broken config is reported when the user selects it by its directory
						break;
					}
				}

				return -1;
			}

			// Returns the project index for the config in the directory, loading the project if necessary. Sets isNew if it was loaded just now
			int GetProjectIndexByConfigDirectory(cstr sysPathToConfigDirectory, OUT bool& isNew)
			{
				isNew = false;

				U8FilePath sexyStudioConfig;
				Format(sexyStudioConfig, "%s%s", sysPathToConfigDirectory, CONFIG_FILENAME);

				if (!IO::IsFileExistant(sexyStudioConfig))
				{
					Throw(0, "Could not find '%s' inside directory '%s'", CONFIG_FILENAME, sysPathToConfigDirectory);
				}

				auto i = mapFilenameToProjectIndex.find(sexyStudioConfig);
				if (i != mapFilenameToProjectIndex.end() && i->second >= 0)
				{
					return i->second;
				}

				int projectIndex = FindProjectIndexByConfig(sexyStudioConfig);
				if (projectIndex < 0)
				{
					projectIndex = AddNewProjectByConfig(sexyStudioConfig); // Throws if the config is bad, so the caller can report it
					isNew = true;
				}

				if (i != mapFilenameToProjectIndex.end())
				{
					i->second = projectIndex;
				}
				else
				{
					mapFilenameToProjectIndex.insert(sexyStudioConfig, projectIndex);
				}

				return projectIndex;
			}

			void SetDirectoryContainingConfigFile(cstr sysPathToConfigDirectory) override
			{
				bool isNew;
				int projectIndex = GetProjectIndexByConfigDirectory(sysPathToConfigDirectory, OUT isNew);
				Select(projectIndex, isNew);
			}

			void ReloadProjectByConfigDirectory(cstr sysPathToConfigDirectory) override
			{
				bool isNew;
				int projectIndex = GetProjectIndexByConfigDirectory(sysPathToConfigDirectory, OUT isNew);
				if (isNew)
				{
					Select(projectIndex, true);
				}
				else
				{
					currentProjectIndex = projectIndex;
					Repopulate();
				}
			}

			void Rebase(crwstr fullPathToSxyFile) override
			{
				if (!EndsWith(fullPathToSxyFile, L".sxy"))
				{
					return;
				}

				U8FilePath u8Path;
				Assign(u8Path, fullPathToSxyFile);

				int projectIndex;

				auto i = mapFilenameToProjectIndex.find(u8Path);
				if (i != mapFilenameToProjectIndex.end())
				{
					projectIndex = i->second;
				}
				else
				{
					bool isNew = false;
					projectIndex = FindProjectIndexByRoot(u8Path);
					if (projectIndex < 0)
					{
						size_t before = projects.size();
						projectIndex = AddNewProjectForFile(u8Path);
						isNew = projects.size() > before;
					}

					mapFilenameToProjectIndex.insert(u8Path, projectIndex);

					if (isNew)
					{
						Select(projectIndex, true);
						return;
					}
				}

				Select(projectIndex, false);
			}

			void Repopulate() override
			{
				if (currentProjectIndex < 0)
				{
					return;
				}

				projects[currentProjectIndex]->database->PopulateViaSearchPaths();
				Notify(true);
			}

			void LoadDefaults() override
			{
				try
				{
					Rococo::OS::LoadUserSEXML(nullptr, defaultsSection,
						[this](const ISEXMLDirectiveList& topLevelDirectives)
						{
							size_t index = 0;
							auto* directive = FindDirective(topLevelDirectives, defaultsSection, REF index);
							if (directive)
							{
								auto& aDefaultConfigFile = (*directive)[CONFIG_DEFAULT_FULLNAME];
								cstr defaultConfigFile = AsString(aDefaultConfigFile).c_str();

								U8FilePath container;
								Assign(container, defaultConfigFile);
								IO::ToSysPath(container.buf);

								if (EndsWith(defaultConfigFile, "\\sexystudio.config.sexml") && IO::IsFileExistant(defaultConfigFile))
								{
									IO::MakeContainerDirectory(container.buf);
									SetDirectoryContainingConfigFile(container.buf);
								}
							}
						}
					);
				}
				catch (...)
				{
					// A missing or invalid defaults file leaves the workspace without a project, as on first use
				}
			}

			void SaveDefaults() override
			{
				if (currentProjectIndex < 0)
				{
					return;
				}

				try
				{
					U8FilePath u8Path;
					Assign(u8Path, projects[currentProjectIndex]->configPath);

					Rococo::OS::SaveUserSEXML(nullptr, defaultsSection, true,
						[this, &u8Path](ISEXMLBuilder& builder)
						{
							builder.AddDirective(defaultsSection);
								builder.AddStringLiteral(CONFIG_DEFAULT_FULLNAME, u8Path);
							builder.CloseDirective();
						}
					);
				}
				catch (...)
				{
					// The defaults are a convenience. Failing to save them loses nothing but the choice of project at the next start
				}
			}

			void SaveProjects(IWorkspaceErrorHandler* onError) override
			{
				for (auto& p : projects)
				{
					try
					{
						p->Save();
					}
					catch (IException& ex)
					{
						if (onError)
						{
							U8FilePath u8Path;
							Assign(u8Path, p->configPath);
							onError->OnWorkspaceError(u8Path, ex);
						}
					}
				}
			}
		};
	}

	SEXYSTUDIO_API ISexyWorkspace* CreateSexyWorkspace(cstr defaultsSection)
	{
		if (defaultsSection == nullptr || *defaultsSection == 0)
		{
			Throw(0, "%s: defaultsSection must name the user defaults file", __ROCOCO_FUNCTION__);
		}

		return new SexyWorkspace(defaultsSection);
	}
}
