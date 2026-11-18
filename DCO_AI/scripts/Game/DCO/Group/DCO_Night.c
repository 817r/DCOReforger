class DCO_Night
{

	protected static ref array<vector> s_aFlarePos;
	protected static array<vector> GetFlarePos()
	{
		if (!s_aFlarePos)
			s_aFlarePos = {};
		return s_aFlarePos;
	}
	protected static ref array<float> s_aFlareTime;
	protected static array<float> GetFlareTime()
	{
		if (!s_aFlareTime)
			s_aFlareTime = {};
		return s_aFlareTime;
	}
	static bool IsActive()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		return cfg && cfg.GetNightEnabled() && DCO_Util.IsNight();
	}

	static float EngageMultiplier()
	{
		if (!IsActive())
			return 1.0;
		return DCO_GlobalAIComponent.GetInstance().GetNightEngageMul();
	}

	static bool FlareNear(vector pos, float now_ms, float radius = 150.0)
	{
		float radiusSq = radius * radius;
		for (int i = GetFlarePos().Count() - 1; i >= 0; i--)
		{
			if (now_ms - GetFlareTime()[i] > 40000.0 || now_ms < GetFlareTime()[i])
			{
				GetFlarePos().Remove(i);
				GetFlareTime().Remove(i);
				continue;
			}
			if (vector.DistanceSqXZ(GetFlarePos()[i], pos) <= radiusSq)
				return true;
		}
		return false;
	}

	static void NoteFlare(vector pos, float now_ms)
	{
		GetFlarePos().Insert(pos);
		GetFlareTime().Insert(now_ms);
	}

	static bool IsDark(SCR_AIGroupTargetCluster c)
	{
		int n = 0;
		foreach (SCR_AITargetInfo t : c.m_aTargets)
		{
			if (!t || !t.m_Entity)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;

			PerceivableComponent perc = PerceivableComponent.Cast(t.m_Entity.FindComponent(PerceivableComponent));
			if (perc && perc.GetIlluminationFactor() >= 0.2)
				return false;
			n++;
		}
		return n > 0;
	}

	static void ApplyLights(notnull SCR_AIGroup grp, bool stealth, DCO_CQBClear cqb)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(grp);
		if (!cfg || !cfg.GetNightLightDiscipline())
			return;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(ent);
			if (!gm)
				continue;

			IEntity gadget = gm.GetGadgetByType(EGadgetType.FLASHLIGHT);
			if (!gadget)
				continue;
			SCR_FlashlightComponent fl = SCR_FlashlightComponent.Cast(gadget.FindComponent(SCR_FlashlightComponent));
			if (!fl)
				continue;

			bool want = !stealth && cqb && cqb.IsEntry(a) && DCO_CoverManagerComponent.DCO_GetBuildingAt(ent) != null;
			if (fl.IsToggledOn() == want)
				continue;

			gm.AskToggleGadget(fl, want);
			if (cqb)
			{
				string state = "off";
				if (want)
					state = "on";
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("light_%1 grp=%2 unit=%3 cqb=1", state, grp, ent));
			}
		}
	}

	protected static ref map<IEntity, float> s_mHeadlightsOff;
	protected static map<IEntity, float> GetHeadlightsOff()
	{
		if (!s_mHeadlightsOff)
			s_mHeadlightsOff = new map<IEntity, float>();
		return s_mHeadlightsOff;
	}

	static void ApplyHeadlights(notnull SCR_AIGroup grp, SCR_AIGroupPerception perc)
	{
		IEntity veh = DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity());
		if (!veh || !perc)
			return;

		BaseLightManagerComponent lights = BaseLightManagerComponent.Cast(veh.FindComponent(BaseLightManagerComponent));
		if (!lights || (!lights.GetLightsState(ELightType.Head) && !lights.GetLightsState(ELightType.HiBeam)))
			return;

		float rangeSq = 800.0 * 800.0;
		vector p = veh.GetOrigin();
		bool threat = false;
		foreach (SCR_AITargetInfo t : perc.m_aTargets)
		{
			if (t && (t.m_eCategory == EAITargetInfoCategory.DETECTED || t.m_eCategory == EAITargetInfoCategory.IDENTIFIED)
				&& vector.DistanceSqXZ(t.m_vWorldPos, p) <= rangeSq)
			{
				threat = true;
				break;
			}
		}
		if (!threat)
			return;

		lights.SetLightsState(ELightType.Head, false);
		lights.SetLightsState(ELightType.HiBeam, false);

		float now = GetGame().GetWorld().GetWorldTime();
		if (now - GetHeadlightsOff().Get(veh) > 60000)
		{
			GetHeadlightsOff().Set(veh, now);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("headlights_off veh=%1 grp=%2", veh, grp));
		}
	}
}
