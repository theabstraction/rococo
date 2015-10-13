namespace
{
	using namespace Dystopia;
	using namespace Rococo;

	class GameControls
	{
	private:
		uint32 SCANCODE_FORWARD;
		uint32 SCANCODE_BACKWARD;
		uint32 SCANCODE_STRAFFELEFT;
		uint32 SCANCODE_STRAFFERIGHT;
		uint32 SCANCODE_FIRE;

		bool isForward;
		bool isBackward;
		bool isLeft;
		bool isRight;

		Degrees viewTheta;
		bool rbuttonDown;

		int globalScale;

		AutoFree<IKeyboardSupervisor> keyboard;

		int fireCount;
	public:
		GameControls() :
			keyboard(CreateKeyboardMap()),
			viewTheta{ 30.0f },
			rbuttonDown(false),
			isForward(false),
			isBackward(false),
			isLeft(false),
			isRight(false),
			globalScale(4),
			fireCount(0)
		{
			SCANCODE_FORWARD = keyboard->GetScanCode(L"W");
			SCANCODE_BACKWARD = keyboard->GetScanCode(L"S");
			SCANCODE_STRAFFELEFT = keyboard->GetScanCode(L"A");
			SCANCODE_STRAFFERIGHT = keyboard->GetScanCode(L"D");
			SCANCODE_FIRE = keyboard->GetScanCode(L"SPACE");
		}

		int& GetFireCount()
		{
			return fireCount;
		}

		void AppendMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::MouseWheel))
			{
				int32 delta = (int32)(int16)me.buttonData;
				globalScale += delta > 0 ? 1 : -1;
				if (globalScale < 0) globalScale = 0;
				if (globalScale > 10) globalScale = 10;
			}

			if (me.HasFlag(MouseEvent::RDown))
			{
				rbuttonDown = true;
			}
			else if (me.HasFlag(MouseEvent::RUp))
			{
				rbuttonDown = false;
			}

			if (rbuttonDown && me.IsRelative())
			{
				viewTheta.quantity += me.dx;
			}

			if (me.HasFlag(MouseEvent::LUp))
			{

			}
		}

		void AppendKeyboardEvent(const KeyboardEvent& k)
		{
			if (k.IsUp())
			{
				if (k.scanCode == SCANCODE_FORWARD)
				{
					isForward = false;
				}
				else if (k.scanCode == SCANCODE_BACKWARD)
				{
					isBackward = false;
				}
				else if (k.scanCode == SCANCODE_STRAFFELEFT)
				{
					isLeft = false;
				}
				else if (k.scanCode == SCANCODE_STRAFFERIGHT)
				{
					isRight = false;
				}
			}
			else
			{
				if (k.scanCode == SCANCODE_FORWARD)
				{
					isForward = true;
				}
				else if (k.scanCode == SCANCODE_BACKWARD)
				{
					isBackward = true;
				}
				else if (k.scanCode == SCANCODE_STRAFFELEFT)
				{
					isLeft = true;
				}
				else if (k.scanCode == SCANCODE_STRAFFERIGHT)
				{
					isRight = true;
				}
				else if (k.scanCode == SCANCODE_FIRE)
				{
					fireCount++;
				}
			}
		}

		float GlobalScale() const { return (float)globalScale; }

		Degrees ViewTheta() const { return viewTheta; }

		Vec2 GetImpulse() const
		{
			float Fx = (isLeft ? -1.0f : 0.0f) + (isRight ? 1.0f : 0.0f);
			float Fy = (isForward ? 1.0f : 0.0f) + (isBackward ? -1.0f : 0.0f);
			return Vec2{ Fx, Fy };
		}
	};
}