#include "dystopia.h"
#include "dystopia.post.h"
#include "rococo.strings.h"
#include <vector>
#include <algorithm>
#include "rococo.maths.h"

#include <unordered_map>

namespace Dystopia
{
	struct HistoricEvent: IHistoricEvent
	{
		std::string title;
		std::string body;
		HistoricEventLayout layout;
		HistoricEventType type;

		virtual cstr Title() const { return title.c_str(); }
		virtual cstr Body()  const { return body.c_str(); }
		virtual HistoricEventType Type() const { return type;  }
		virtual HistoricEventLayout& Layout() { return layout; }
	};
}

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	class Goal_MeetObject : public IGoalSupervisor, public Post::IRecipient
	{
		ID_ENTITY a;
		ID_ENTITY b;

		std::string title;
		std::string body;

		Metres completionRadius;
		GoalState state;

		Environment& e;

		Post::Subscribtion<AdvanceTimestepEvent> sub_Timestep;
	public:
		Goal_MeetObject(Environment& _e, Metres _radius, cstr _title, cstr _body, ID_ENTITY _a, ID_ENTITY _b) :
			e(_e),
			state(GoalState_Pending),
			completionRadius(_radius),
			title(_title),
			body(_body),
			a(_a),
			b(_b)
		{
		}

		virtual void NotifyPrecursorInvalid()
		{
			state = GoalState_Failed;
		}

		virtual void Start()
		{
			if (state == GoalState_Pending)
			{
				state = GoalState_Ongoing;
				sub_Timestep.Start(e.postbox, this);
			}
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* timestepEvent = Post::InterpretAs<AdvanceTimestepEvent>(mail);
			if (timestepEvent)
			{
				OnTimestep(timestepEvent->gameTime, timestepEvent->dt);
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Title() const { return title.c_str(); }
		virtual cstr Body() const { return body.c_str(); }

		void OnTimestep(float gameTime, float dt)
		{
			if (state != GoalState_Ongoing) return;

			const Vec3* posA, *posB;
			posA = e.level.TryGetPosition(a);
			posB = e.level.TryGetPosition(b);

			if (!posA || !posB)
			{
				state = GoalState_Failed;
				return;
			}

			if (IsInRange(*posA - *posB, completionRadius))
			{
				state = GoalState_Complete;
				return;
			}
		}

		virtual GoalState State() const
		{
			return state;
		}
	};	
	
	struct GoalFinished
	{
		ID_GOAL goalId;
		bool wasCompleteElseFailed;
	};

	struct CompletionFunction
	{
		ArchetypeCallback fn;
		ID_ENTITY a;
		ID_ENTITY b;
	};

	struct GoalBinding
	{
		std::vector<GoalFinished> completionRequirements;
		IGoalSupervisor* goal;
		CompletionFunction onComplete;
	};

	typedef std::unordered_map<ID_GOAL, GoalBinding, ID_GOAL> TGoals;
	typedef std::vector<HistoricEvent> THistory;
	typedef std::vector<GoalFinished> TAbattoir;

	void NotifyFinishedGoal(TGoals& goals, ID_GOAL precursorId, bool wasSuccessful)
	{
		for (auto git = goals.begin(); git != goals.end(); ++git)
		{
			GoalBinding& g = git->second;

			auto erasePoint = std::remove_if(g.completionRequirements.begin(), g.completionRequirements.end(),
				[precursorId, wasSuccessful](const GoalFinished& requirement)
				{
					return requirement.goalId == precursorId && requirement.wasCompleteElseFailed == wasSuccessful;
				}
			);

			g.completionRequirements.erase(erasePoint, g.completionRequirements.end());

			for (auto& req : g.completionRequirements)
			{
				if (req.goalId == precursorId)
				{
					// Not completed
					if (git->second.goal->State() != GoalState_Failed) git->second.goal->NotifyPrecursorInvalid();
				}
			}
		}
	}

	void UpdateGoals(TGoals& goals, THistory& history, Environment& e, TAbattoir& abattoir)
	{
		for (auto& git : goals)
		{
			IGoalSupervisor& g = *git.second.goal;
			if (g.State() == GoalState_Complete)
			{
				rchar newBody[4096];
				SafeFormat(newBody, _TRUNCATE, L"This goal was completed - \"%s\"", g.Body());
				HistoricEvent he;
				he.title = g.Title();
				he.body = newBody;
				he.layout = { 0,0,0 };
				he.type = HistoricEventType_CompletedGoal;
				history.push_back(he);
				abattoir.push_back({ git.first, true });
				e.gui.Add3DHint(e.level.GetPosition(e.level.GetPlayerId()), L"Goal complete!"_fstring, 2.0_seconds);

				if (git.second.onComplete.fn.byteCodeId != 0)
				{
					struct : IArgEnumerator
					{
						virtual void PushArgs(IArgStack& args)
						{
							args.PushInt64((int64)(size_t)b);
							args.PushInt64((int64)(size_t)a);
						}

						virtual void PopOutputs(IOutputStack& args)
						{

						}

						ID_ENTITY a;
						ID_ENTITY b;
					} args;

					args.a = git.second.onComplete.a;
					args.b = git.second.onComplete.b;
					e.levelLoader.ExecuteLevelFunction(git.second.onComplete.fn, args);
				}
			}
			else if (g.State() == GoalState_Failed)
			{
				rchar newBody[4096];
				SafeFormat(newBody, _TRUNCATE, L"You failed to complete this goal - \"%s\"", g.Body());
				HistoricEvent he;
				he.title = g.Title();
				he.body = newBody;
				he.layout = { 0, 0, 0 };
				he.type = HistoricEventType_FailedGoal;
				history.push_back(he);
				abattoir.push_back({ git.first, false });
				e.gui.Add3DHint(e.level.GetPosition(e.level.GetPlayerId()), L"Goal failed!"_fstring, 2.0_seconds);
			}
			else if (g.State() == GoalState_Pending && git.second.completionRequirements.empty())
			{
				g.Start();
			}
		}

		for (auto& j : abattoir)
		{
			NotifyFinishedGoal(goals, j.goalId, j.wasCompleteElseFailed);
			auto i = goals.find(j.goalId);
			i->second.goal->Free();
			goals.erase(i);
		}

		abattoir.clear();
	}

	ID_GOAL GenNextGoalId()
	{
		static ID_GOAL id(0);
		id = ID_GOAL(id.value + 1);
		return id;
	}

	class Journal : public IJournalSupervisor, public Post::IRecipient, public IMutableVectorEnumerator<IHistoricEvent>
	{
		Environment& e;
		THistory history;
		TGoals goalById;
		TAbattoir abattoir;
	public:
		Journal(Environment& _e) : e(_e)
		{
		}

		~Journal()
		{
			Clear();
		}

		virtual void Clear()
		{
			for (auto& g : goalById)
			{
				g.second.goal->Free();
			}

			goalById.clear();
			abattoir.clear();
			history.clear();
		}

		virtual IHistoricEvent& operator[] (size_t index)
		{
			return history[index];
		}

		virtual size_t Count() const
		{
			return history.size();
		}

		virtual void Enumerate(IMutableEnumerator<IHistoricEvent>& cb)
		{
			for (auto& event : history)
			{
				cb(event);
			}
		}

		virtual IMutableVectorEnumerator<IHistoricEvent>& History()
		{
			return *this;
		}

		virtual void EnumerateGoals(IEnumerator<IGoal>& cb)
		{
			for (auto& g : goalById)
			{
				if (g.second.goal->State() == GoalState_Ongoing)
				{
					cb(*g.second.goal);
				}
			}
		}

		virtual void EnumerateEvents(IEnumerator<IHistoricEvent>& cb)
		{
			for (auto& h : history)
			{
				cb(h);
			}
		}

		virtual void PostConstruct()
		{
			
		}

		virtual void UpdateGoals()
		{
			::UpdateGoals(goalById, history, e, abattoir);
		}

		virtual void OnPost(const Mail& mail)
		{

		}

		virtual ID_GOAL AddGoalMeet(const fstring& title, const fstring& body, ID_ENTITY a, ID_ENTITY b, Metres radius, ArchetypeCallback twoInt64InputFunction)
		{
			auto* goal = CreateGoal_MeetObject(e, radius, title, body, a, b);
			auto id = GenNextGoalId();
			auto g = goalById.insert(std::make_pair(id, GoalBinding()));
			g.first->second.goal = goal;
			g.first->second.onComplete.fn = twoInt64InputFunction;
			g.first->second.onComplete.a = a;
			g.first->second.onComplete.b = b;
			return id;
		}

		virtual void CompleteFirst(ID_GOAL forGoalId, ID_GOAL precursorId)
		{
			auto& target = goalById.find(forGoalId);
			if (target == goalById.end())
			{
				Throw(0, L"Cannot find forGoalId #I64U in call to Journal::CompleteFirst(...)", forGoalId.value);
			}

			auto& precursor = goalById.find(precursorId);
			if (precursor == goalById.end())
			{
				Throw(0, L"Cannot find precusorId #I64U in call to Journal::CompleteFirst(...)", precursorId.value);
			}

			target->second.completionRequirements.push_back({ precursorId, true });
		}

		virtual void FailFirst(ID_GOAL forGoalId, ID_GOAL precursorId)
		{
			auto& target = goalById.find(forGoalId);
			if (target == goalById.end())
			{
				Throw(0, L"Cannot find forGoalId #I64U in call to Journal::FailFirst(...)", forGoalId.value);
			}

			auto& precursor = goalById.find(precursorId);
			if (precursor == goalById.end())
			{
				Throw(0, L"Cannot find precusorId #I64U in call to Journal::FailFirst(...)", precursorId.value);
			}

			target->second.completionRequirements.push_back({ precursorId, false });
		}

		virtual void AddHistory(const fstring& title, const fstring& body)
		{
			HistoricEvent he;
			he.title = title.buffer;
			he.body = body.buffer;
			he.layout = { 0, 0, 0 };
			he.type = HistoricEventType_Narrative;
			history.push_back(he);
		}

		virtual bool IsReadyForRender() const
		{
			return history.back().layout.yEndOfBodyPixelRow != 0;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Dystopia
{
	IJournalSupervisor* CreateJournal(Environment& e)
	{
		return new Journal(e);
	}

	IGoalSupervisor* CreateGoal_MeetObject(Environment& e, Metres radius, cstr title, cstr body, ID_ENTITY a, ID_ENTITY b)
	{
		return new Goal_MeetObject(e, radius, title, body, a, b);
	}
}