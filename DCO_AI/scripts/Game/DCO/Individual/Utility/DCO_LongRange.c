class DCO_LongRange
{
	protected static const float SHOT_WINDOW_MS = 4500;
	protected static const float PAUSE_MIN_MS = 2000;
	protected static const float PAUSE_MAX_MS = 3500;
	protected static const float GROUP_SLOT_MS = 700;
	static const float RIFLE_FAR_FIRE_RATE = 0.3;
	static const float MG_FAR_FIRE_RATE = 0.5;

	protected static ref map<IEntity, float> s_mWindowEnd = new map<IEntity, float>();
	protected static ref map<IEntity, float> s_mNextWindow = new map<IEntity, float>();

	static float EffectiveRange(EWeaponType wt)
	{
		float rifle = 350;
		float mg = 700;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
		{
			rifle = cfg.GetRifleEffectiveRange();
			mg = cfg.GetMGEffectiveRange();
		}
		float night = DCO_Night.EngageMultiplier();
		rifle *= night;
		mg *= night;

		switch (wt)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_SNIPERRIFLE:
				return mg;
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_FRAGGRENADE:
			case EWeaponType.WT_SMOKEGRENADE:
				return float.MAX;
		}
		return rifle;
	}

	static float CapFireRate(float fireRate, float dist, EWeaponType wt)
	{
		if (dist <= EffectiveRange(wt))
			return fireRate;
		if (wt == EWeaponType.WT_MACHINEGUN)
			return Math.Min(fireRate, MG_FAR_FIRE_RATE);
		return Math.Min(fireRate, RIFLE_FAR_FIRE_RATE);
	}

	static bool ShouldHold(SCR_AIUtilityComponent u)
	{
		int pt = DCO_Perf.Begin();
		bool hold = DoShouldHold(u);
		DCO_Perf.End("agent_long_range", pt);
		return hold;
	}

	protected static bool DoShouldHold(SCR_AIUtilityComponent u)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || !cfg.GetLongRangeHold() || !u || !u.m_CombatComponent || !u.m_ThreatSystem)
			return false;

		if (u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
			return false;

		if (DCO_PostureCombatUtility.GetPosture(u) == DCO_GroupTactics.AGGRESIVE)
			return false;

		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t)
			return false;

		return t.GetDistance() > EffectiveRange(u.m_CombatComponent.GetSelectedWeaponType());
	}

	static bool IsShotWindow(IEntity shooter)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		float next;
		s_mNextWindow.Find(shooter, next);
		if (now >= next)
		{
			if (s_mNextWindow.Count() > 512)
			{
				s_mNextWindow.Clear();
				s_mWindowEnd.Clear();
			}
			float start = now;
			if (next <= 0)
				start = now + GroupOrder(shooter) * GROUP_SLOT_MS;
			float end = start + SHOT_WINDOW_MS;
			s_mWindowEnd.Set(shooter, end);
			s_mNextWindow.Set(shooter, end + Math.RandomFloat(PAUSE_MIN_MS, PAUSE_MAX_MS));
			return now >= start;
		}
		float windowEnd = s_mWindowEnd.Get(shooter);
		return now < windowEnd && now >= windowEnd - SHOT_WINDOW_MS;
	}

	protected static int GroupOrder(IEntity shooter)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(shooter.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return 0;
		AIGroup grp = ctrl.GetAIAgent().GetParentGroup();
		if (!grp)
			return 0;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int idx = agents.Find(ctrl.GetAIAgent());
		if (idx < 0)
			return 0;
		return idx % 6;
	}

	static void ApplyStance(SCR_AIUtilityComponent u, bool visible)
	{
		if (!u.m_CombatComponent)
			return;
		SCR_CharacterControllerComponent ctrl = u.m_CombatComponent.GetCharacterController();
		if (!ctrl)
			return;

		ECharacterStance want = ECharacterStance.CROUCH;
		if (visible && HasProneLOS(u))
			want = ECharacterStance.PRONE;

		if (ctrl.GetStance() == want)
			return;

		if (want == ECharacterStance.PRONE)
			ctrl.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOPRONE);
		else
			ctrl.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOCROUCH);
	}

	protected static bool HasProneLOS(SCR_AIUtilityComponent u)
	{
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		IEntity me = u.m_OwnerEntity;
		if (!t || !me || !t.GetTargetEntity())
			return false;

		TraceParam tp = new TraceParam();
		tp.Start = me.GetOrigin() + "0 0.4 0";
		tp.End = t.GetTargetEntity().GetOrigin() + "0 1 0";
		tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		tp.Exclude = me;
		tp.LayerMask = EPhysicsLayerPresets.Projectile;
		DCO_Perf.Count("t:DCO_LongRange");
		float frac = GetGame().GetWorld().TraceMove(tp, null);
		return frac >= 0.98 || tp.TraceEnt == t.GetTargetEntity();
	}
}
