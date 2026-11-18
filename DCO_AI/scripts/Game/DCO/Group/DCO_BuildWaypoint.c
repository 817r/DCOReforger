class DCO_BuildWaypointClass : SCR_AIWaypointClass
{
}

class DCO_BuildWaypoint : SCR_AIWaypoint
{
	protected SCR_CampaignBuildingLayoutComponent m_Layout;
	protected IEntity m_Outline;
	protected float m_fNeed;
	protected float m_fCarry;
	protected float m_fLastS = -1;
	protected float m_fStartS = -1;
	protected float m_fLastLogS;
	protected float m_fLastWaitS;
	protected bool m_bPaused;
	protected bool m_bDone;
	protected bool m_bAdopted;
	protected vector m_vCenter;
	protected float m_fRing;
	protected string m_sPauseWhy;
	protected ref array<IEntity> m_aWorking = {};
	protected ref array<IEntity> m_aNoTool = {};
	protected ref map<IEntity, float> m_mRelocAt = new map<IEntity, float>();
	protected ref array<vector> m_aSpots = {};
	protected int m_iSpotCount = -1;
	protected string m_sWaitWhy;
	protected float m_fWaitStartS;
	protected static SCR_CampaignBuildingLayoutComponent s_QueryBest;
	protected static vector s_vQueryFrom;

	override string DCO_SetupInfo()
	{
		return "type=build";
	}

	protected string Who(SCR_AIGroup group)
	{
		return DCO_Log.IdGroup(group) + "/" + group.GetName();
	}

	protected static bool OutlineCb(IEntity e)
	{
		if (!e)
			return true;
		SCR_CampaignBuildingLayoutComponent l = SCR_CampaignBuildingLayoutComponent.Cast(e.FindComponent(SCR_CampaignBuildingLayoutComponent));
		if (!l || l.GetToBuildValue() <= 0)
			return true;
		if (!s_QueryBest || vector.DistanceSq(e.GetOrigin(), s_vQueryFrom) < vector.DistanceSq(s_QueryBest.GetOwner().GetOrigin(), s_vQueryFrom))
			s_QueryBest = l;
		return true;
	}

	protected SCR_CampaignBuildingLayoutComponent FindOutline()
	{
		s_QueryBest = null;
		s_vQueryFrom = GetOrigin();
		GetGame().GetWorld().QueryEntitiesBySphere(GetOrigin(), DCO_Ctx.TuneG("build.search_m", 30.0), OutlineCb);
		SCR_CampaignBuildingLayoutComponent found = s_QueryBest;
		s_QueryBest = null;
		return found;
	}

	protected static IEntity Tool(IEntity ent)
	{
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(ent);
		if (!gm)
			return null;
		return gm.GetGadgetByType(EGadgetType.BUILDING_TOOL);
	}

	protected static string Work(IEntity ent, IEntity tool)
	{
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(ent);
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!gm || !ch)
			return "no_gadget_mgr";
		if (gm.GetHeldGadget() != tool)
		{
			gm.SetGadgetMode(tool, EGadgetMode.IN_HAND);
			return "equip";
		}
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (!ctrl)
			return "no_ctrl";
		if (!ctrl.CanUseItem())
			return "work";
		CharacterAnimationComponent anim = ctrl.GetAnimationComponent();
		if (!anim)
			return "no_anim";
		ItemUseParameters params = new ItemUseParameters();
		params.SetEntity(tool);
		params.SetAllowMovementDuringAction(false);
		params.SetKeepInHandAfterSuccess(true);
		params.SetCommandID(anim.BindCommand("CMD_Item_Action"));
		params.SetCommandIntArg(1);
		if (ctrl.TryUseItemOverrideParams(params))
			return "work";
		return "use_fail";
	}

	protected vector Spot(SCR_AIGroup group, int index, int count)
	{
		if (count != m_iSpotCount)
		{
			m_iSpotCount = count;
			m_aSpots.Clear();
			NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
			int offNav;
			for (int i = 0; i < count; i++)
			{
				float ang = i * 6.2832 / Math.Max(count, 1);
				vector p = DCO_Util.Surface(m_vCenter + Vector(Math.Cos(ang) * m_fRing, 0, Math.Sin(ang) * m_fRing));
				vector snapped;
				if (nav && nav.IsTileLoaded(p) && nav.GetReachablePoint(p, 2.0, snapped))
					p = snapped;
				else
					offNav++;
				m_aSpots.Insert(p);
			}
			DCO_Perf.Count("build:spots");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_spots grp=%1 count=%2 off_navmesh=%3 ring=%4", Who(group), count, offNav, m_fRing.ToString(-1, 1)));
		}
		if (!m_aSpots.IsIndexValid(index))
			return m_vCenter;
		return m_aSpots[index];
	}

	protected void HoldAt(AIAgent a, IEntity e, vector spot, float now)
	{
		float at;
		if (m_mRelocAt.Find(e, at) && now < at)
			return;
		m_mRelocAt.Set(e, now + 20.0);
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
		if (!u)
			return;
		if (DCO_RelocateBehavior.Start(u, spot, m_vCenter, false, ECharacterStance.STAND, 60.0, "build"))
			DCO_Perf.Count("build:hold_spot");
	}

	protected static void Stop(IEntity ent)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch)
			return;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (ctrl && ctrl.GetAnimationComponent() && ctrl.GetAnimationComponent().GetCommandHandler())
			ctrl.GetAnimationComponent().GetCommandHandler().FinishItemUse(true);
		SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(ent);
		IEntity tool = Tool(ent);
		if (gm && tool && gm.GetHeldGadget() == tool)
			gm.SetGadgetMode(tool, EGadgetMode.IN_SLOT);
	}

	protected void StopAll(SCR_AIGroup group)
	{
		foreach (IEntity e : m_aWorking)
		{
			if (e)
				Stop(e);
		}
		if (!m_aWorking.IsEmpty())
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_worker grp=%1 state=stop count=%2", Who(group), m_aWorking.Count()));
		m_aWorking.Clear();
	}

	protected int Workers(SCR_AIGroup group, float now, out int withTool, out string diag)
	{
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		float r = Math.Max(DCO_Ctx.TuneG("build.radius_m", 12.0), m_fRing + 3.0);
		vector site = m_vCenter;
		int n;
		withTool = 0;
		diag = "";
		array<IEntity> still = {};
		array<AIAgent> crew = {};
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity e = a.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			if (!Tool(e))
			{
				if (!m_aNoTool.Contains(e))
				{
					m_aNoTool.Insert(e);
					DCO_Perf.Count("build:no_tool");
					DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_no_tool grp=%1 unit=%2", Who(group), DCO_Log.IdUnit(e)));
				}
				continue;
			}
			crew.Insert(a);
		}
		withTool = crew.Count();
		foreach (int i, AIAgent a : crew)
		{
			IEntity e = a.GetControlledEntity();
			IEntity tool = Tool(e);
			float dist = vector.DistanceXZ(e.GetOrigin(), site);
			string state;
			if (dist > r)
			{
				if (m_aWorking.Contains(e))
					Stop(e);
				HoldAt(a, e, Spot(group, i, withTool), now);
				state = "far" + Math.Round(dist).ToString();
			}
			else
			{
				vector spot = Spot(group, i, withTool);
				if (vector.DistanceXZ(e.GetOrigin(), spot) > 2.5)
					HoldAt(a, e, spot, now);
				state = Work(e, tool);
			}
			diag += " " + DCO_Log.IdUnit(e) + ":" + state;
			if (state != "work")
				continue;
			n++;
			still.Insert(e);
			if (!m_aWorking.Contains(e))
			{
				DCO_Perf.Count("build:worker_work");
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_worker grp=%1 unit=%2 state=work", Who(group), DCO_Log.IdUnit(e)));
			}
		}
		foreach (IEntity w : m_aWorking)
		{
			if (w && !still.Contains(w) && vector.DistanceXZ(w.GetOrigin(), site) <= r)
				still.Insert(w);
		}
		m_aWorking.Copy(still);
		return n;
	}

	protected ENodeResult Wait(SCR_AIGroup group, float now, string why)
	{
		if (m_sWaitWhy != why)
		{
			m_sWaitWhy = why;
			m_fWaitStartS = now;
		}
		float limit = DCO_Ctx.TuneG("build.wait_outline_s", 300.0);
		if (why == "no_tools")
			limit = DCO_Ctx.TuneG("build.wait_tools_s", 30.0);
		if (limit > 0 && now - m_fWaitStartS >= limit)
			return End(group, why);
		if (now - m_fLastWaitS >= 30.0)
		{
			m_fLastWaitS = now;
			DCO_Perf.Count("build:wait_" + why);
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_wait grp=%1 why=%2 wp=%3", Who(group), why, DCO_Log.Pos(GetOrigin())));
		}
		return ENodeResult.RUNNING;
	}

	ENodeResult DCO_Tick(SCR_AIGroup group, SCR_AIGroupUtilityComponent util)
	{
		if (m_bDone)
			return ENodeResult.SUCCESS;
		float now = DCO_Util.NowS();
		if (m_fStartS < 0)
			m_fStartS = now;
		if (m_bAdopted && !m_Layout)
			return End(group, "built");
		if (!m_Layout)
		{
			m_Layout = FindOutline();
			if (!m_Layout)
				return Wait(group, now, "no_outline");
			m_Outline = m_Layout.GetOwner();
			m_bAdopted = true;
			m_fNeed = m_Layout.GetToBuildValue();
			m_fLastS = now;
			vector mn;
			vector mx;
			m_Outline.GetWorldBounds(mn, mx);
			m_vCenter = (mn + mx) * 0.5;
			m_vCenter[1] = m_Outline.GetOrigin()[1];
			m_fRing = Math.Max(vector.DistanceXZ(mn, mx) * 0.5 + 1.0, 3.0);
			DCO_Perf.Count("build:adopt");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_adopt grp=%1 outline=%2 pos=%3 need=%4 progress=%5 ring=%6", Who(group), DCO_Util.PrefabShort(m_Outline), DCO_Log.Pos(m_vCenter), m_fNeed, Math.Round(m_Layout.GetCurrentBuildValue()), m_fRing.ToString(-1, 1)));
			return ENodeResult.RUNNING;
		}
		float dt = 0;
		if (m_fLastS >= 0)
			dt = Math.Clamp(now - m_fLastS, 0, 30);
		m_fLastS = now;
		string pauseWhy;
		if (util.DCO_InContact())
			pauseWhy = "contact";
		if (!pauseWhy.IsEmpty())
		{
			if (!m_bPaused)
			{
				DCO_Perf.Count("build:pause_" + pauseWhy);
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_pause grp=%1 why=%2 progress=%3/%4", Who(group), pauseWhy, Math.Round(m_Layout.GetCurrentBuildValue()), m_fNeed));
				StopAll(group);
			}
			m_bPaused = true;
			m_sPauseWhy = pauseWhy;
			return ENodeResult.RUNNING;
		}
		int withTool;
		string diag;
		int workers = Workers(group, now, withTool, diag);
		if (withTool == 0)
			return Wait(group, now, "no_tools");
		m_sWaitWhy = "";
		if (m_bPaused)
		{
			m_bPaused = false;
			DCO_Perf.Count("build:resume");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_resume grp=%1 workers=%2", Who(group), workers));
		}
		m_fCarry += workers * DCO_Ctx.TuneG("build.rate_per_s", 2.0) * dt;
		int whole = Math.Floor(m_fCarry);
		if (whole >= 1)
		{
			m_Layout.AddBuildingValue(whole);
			m_fCarry -= whole;
		}
		if (now - m_fLastLogS >= 10.0)
		{
			m_fLastLogS = now;
			DCO_Perf.Count("build:tick");
			float cur = 0;
			if (m_Layout)
				cur = m_Layout.GetCurrentBuildValue();
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_tick grp=%1 progress=%2/%3 workers=%4 with_tool=%5 units=%6", Who(group), Math.Round(cur), m_fNeed, workers, withTool, diag));
		}
		return ENodeResult.RUNNING;
	}

	protected ENodeResult End(SCR_AIGroup group, string why)
	{
		m_bDone = true;
		StopAll(group);
		DCO_Perf.Count("build:end_" + why);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("build_end grp=%1 why=%2 need=%3 secs=%4 alive=%5", Who(group), why, m_fNeed, Math.Round(DCO_Util.NowS() - m_fStartS), group.GetAgentsCount()));
		group.CompleteWaypoint(this);
		return ENodeResult.SUCCESS;
	}
}

class DCO_AIBuild : AITaskScripted
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(owner);
		if (!group)
			return ENodeResult.FAIL;
		DCO_BuildWaypoint wp = DCO_BuildWaypoint.Cast(group.GetCurrentWaypoint());
		if (!wp)
			return ENodeResult.FAIL;
		SCR_AIGroupUtilityComponent util = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!util)
			return ENodeResult.FAIL;
		return wp.DCO_Tick(group, util);
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: Build waypoint. Members with an entrenching tool build the composition outline nearest the waypoint (placed from the command tent)."; }
}
