#include "hv.h"
#include <rococo.hashtable.h>

using namespace HV;

namespace
{
	struct JapaneseFrodoMachine : ITagsSupervisor
	{
		ISectors& sectors;

		JapaneseFrodoMachine(ISectors& _sectors):
			sectors(_sectors)
		{

		}

		void Free() override
		{
			delete this;
		}

		typedef stringmap<std::vector<ISector*>> TMapStringToSectors;
		TMapStringToSectors map;

		bool inTagCall = false;

		void Invalidate() override
		{
			if (inTagCall) Throw(0, "Tags.Invalidate(): callback tried to delete hashtable");
			map.clear();
		}

		void RefreshHashtables()
		{
			Invalidate();

			cstr undefined = GET_UNDEFINDED_TAG();

			for (auto* s : sectors)
			{
				auto& tags = s->Tags().EnumTags();
				for (int32 i = 0; i < tags.Count(); ++i)
				{
					char varName[IGlobalVariables::MAX_VARIABLE_NAME_LENGTH];
					tags.GetItem(i, varName, IGlobalVariables::MAX_VARIABLE_NAME_LENGTH);

					if (Eq(varName, undefined))
					{
						continue;
					}

					auto j = map.find(varName);
					if (j == map.end())
					{
						j = map.insert(varName, std::vector<ISector*>()).first;
					}

					j->second.push_back(s);
				}
			}
		}

		void ForEachSectorWithTagProtected(cstr tag, ITagCallback& cb)
		{
			auto i = map.find(tag);
			if (i != map.end())
			{
				int32 count = 0;

				for (auto* sector : i->second)
				{
					TagContext context{ *sector, tag,  count++ };
					cb.OnTag(context);
				}
			}
		}

		void ForEachSectorWithTag(cstr tag, ITagCallback& cb) override
		{
			if (inTagCall) Throw(0, "Tags.ForEachSectorWithTag(%s,...): callback tried to recurse", tag);

			if (map.empty())
			{
				RefreshHashtables();
			}

			try
			{
				inTagCall = true;
				ForEachSectorWithTagProtected(tag, cb);
				inTagCall = false;
			}
			catch (IException&)
			{
				inTagCall = false;
			}
		}
	};
}

namespace HV
{
	ITagsSupervisor* CreateTagsSupervisor(ISectors& sectors)
	{
		return new JapaneseFrodoMachine(sectors);
	}
}