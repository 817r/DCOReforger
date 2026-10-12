class DCO_LeanKeeper
{
	protected static ref array<SCR_AICombatComponent> s_aActive;

	static void Register(SCR_AICombatComponent comp)
	{
		if (!comp)
			return;
		if (!s_aActive)
			s_aActive = {};
		if (s_aActive.Contains(comp))
			return;
		s_aActive.Insert(comp);
		if (s_aActive.Count() == 1)
			GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	static void Unregister(SCR_AICombatComponent comp)
	{
		if (!s_aActive)
			return;
		s_aActive.RemoveItem(comp);
		if (s_aActive.IsEmpty())
			GetGame().GetCallqueue().Remove(Tick);
	}

	static int Count()
	{
		if (!s_aActive)
			return 0;
		return s_aActive.Count();
	}

	protected static void Tick()
	{
		for (int i = s_aActive.Count() - 1; i >= 0; i--)
		{
			SCR_AICombatComponent comp = s_aActive[i];
			if (!comp || !comp.DCO_KeepLean())
				s_aActive.Remove(i);
		}
		if (s_aActive.IsEmpty())
			GetGame().GetCallqueue().Remove(Tick);
	}
}

