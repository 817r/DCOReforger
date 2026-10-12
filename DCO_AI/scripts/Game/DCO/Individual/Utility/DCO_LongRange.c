class DCO_LongRange
{

	protected static ref map<IEntity, float> s_mWindowEnd;
	protected static map<IEntity, float> GetWindowEnd()
	{
		if (!s_mWindowEnd)
			s_mWindowEnd = new map<IEntity, float>();
		return s_mWindowEnd;
	}
	protected static ref map<IEntity, float> s_mNextWindow;
	protected static map<IEntity, float> GetNextWindow()
	{
		if (!s_mNextWindow)
			s_mNextWindow = new map<IEntity, float>();
		return s_mNextWindow;
	}

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
			return Math.Min(fireRate, 0.5);
		return Math.Min(fireRate, 0.3);
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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForUtility(u);
		if (!cfg || !cfg.GetLongRangeHold() || !u || !u.m_CombatComponent || !u.m_ThreatSystem)
			return false;

		if (u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
			return false;

		if (DCO_PostureCombatUtility.GetPosture(u) == DCO_GroupTactics.AGGRESIVE)
			return false;

		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t)
			return false;

		if (DCO_Air.IsAirTarget(t))
		{
			DCO_Perf.Count("air:long_skip");
			return false;
		}

		return t.GetDistance() > EffectiveRange(u.m_CombatComponent.GetSelectedWeaponType());
	}

	static bool IsShotWindow(IEntity shooter)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		float next;
		GetNextWindow().Find(shooter, next);
		if (now >= next)
		{
			if (GetNextWindow().Count() > 512)
			{
				GetNextWindow().Clear();
				GetWindowEnd().Clear();
			}
			float start = now;
			if (next <= 0)
				start = now + GroupOrder(shooter) * 700.0;
			float end = start + 4500.0;
			GetWindowEnd().Set(shooter, end);
			GetNextWindow().Set(shooter, end + Math.RandomFloat(2000.0, 3500.0));
			return now >= start;
		}
		float windowEnd = GetWindowEnd().Get(shooter);
		return now < windowEnd && now >= windowEnd - 4500.0;
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
		if (visible && HasProneLOS(u) && DCO_CQC.ProneRoll(u.m_OwnerEntity))
			want = ECharacterStance.PRONE;

		if (ctrl.GetStance() == want || DCO_Nav.Busy(ctrl, "longrange_stance"))
			return;

		if (want == ECharacterStance.PRONE)
			DCO_Stance.Request(ctrl, ECharacterStanceChange.STANCECHANGE_TOPRONE, "longrange");
		else
			DCO_Stance.Request(ctrl, ECharacterStanceChange.STANCECHANGE_TOCROUCH, "longrange");
	}

	protected static bool HasProneLOS(SCR_AIUtilityComponent u)
	{
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		IEntity me = u.m_OwnerEntity;
		if (!t || !me || !t.GetTargetEntity())
			return false;

		float frac = DCO_Util.TraceFrac(me.GetOrigin() + "0 0.4 0", t.GetTargetEntity().GetOrigin() + "0 1 0", me, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerPresets.Projectile, "t:DCO_LongRange");
		return frac >= 0.98 || DCO_Util.LastTraceEnt() == t.GetTargetEntity();
	}
}
