class DCO_Util
{
	protected static bool s_bTeardown;

	static void SetTeardown(bool on)
	{
		s_bTeardown = on;
	}

	static bool Teardown(string where)
	{
		if (!s_bTeardown)
			return false;
		DCO_Perf.Count("teardown_skip:" + where);
		return true;
	}

	static bool HasEditorLabel(IEntity ent, EEditableEntityLabel label)
	{
		if (!ent)
			return false;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(ent.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return false;
		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(editable.GetInfo());
		return info && info.HasEntityLabel(label);
	}

	static bool HasConsumable(IEntity ent, SCR_EConsumableType a, SCR_EConsumableType b = SCR_EConsumableType.NONE, SCR_EConsumableType c = SCR_EConsumableType.NONE)
	{
		if (!ent)
			return false;
		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(ent.FindComponent(InventoryStorageManagerComponent));
		if (!inv)
			return false;

		array<IEntity> items = {};
		inv.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;
			SCR_ConsumableItemComponent consumable = SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
			if (!consumable)
				continue;
			SCR_EConsumableType type = consumable.GetConsumableType();
			if (type == SCR_EConsumableType.NONE)
				continue;
			if (type == a || type == b || type == c)
				return true;
		}
		return false;
	}

	static bool IsRealMedic(IEntity ent)
	{
		if (HasEditorLabel(ent, EEditableEntityLabel.ROLE_MEDIC))
			return true;
		return HasConsumable(ent, SCR_EConsumableType.SALINE, SCR_EConsumableType.MED_KIT);
	}

	static bool IsBleeding(IEntity ent)
	{
		if (!ent)
			return false;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(ent.FindComponent(SCR_CharacterDamageManagerComponent));
		return dm && dm.IsBleeding();
	}

	static bool HasBandage(IEntity ent)
	{
		return HasConsumable(ent, SCR_EConsumableType.BANDAGE);
	}

	static bool HasManpackRadio(IEntity ent)
	{
		if (!ent)
			return false;
		if (HasEditorLabel(ent, EEditableEntityLabel.ROLE_RADIOOPERATOR))
			return true;
		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.Cast(ent.FindComponent(SCR_GadgetManagerComponent));
		return gadgets && gadgets.GetGadgetByType(EGadgetType.RADIO_BACKPACK);
	}

	static float NowS()
	{
		BaseWorld w = GetGame().GetWorld();
		if (!w)
			return 0;
		return w.GetWorldTime() / 1000.0;
	}

	static vector Surface(vector p)
	{
		p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);
		return p;
	}

	protected static ref array<IEntity> s_aQueryBuffer;
	protected static array<IEntity> GetQueryBuffer()
	{
		if (!s_aQueryBuffer)
			s_aQueryBuffer = {};
		return s_aQueryBuffer;
	}

	static bool HasFriendlyNear(IEntity self, vector pos, float radius, string perfSource, bool wallShields = false)
	{
		GetQueryBuffer().Clear();
		if (!self)
			return false;

		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;

		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();

		DCO_Perf.Count(perfSource);
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, null, QueryBufferCallback, EQueryEntitiesFlags.DYNAMIC);

		bool found = false;
		foreach (IEntity ent : GetQueryBuffer())
		{
			if (!ent || ent == self)
				continue;

			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (fac && fac.GetAffiliatedFaction() && fac.GetAffiliatedFaction().GetFactionKey() == myFactionKey)
			{
				if (wallShields && TraceFrac(pos + Vector(0, 0.5, 0), ent.GetOrigin() + Vector(0, 1.2, 0), ent, TraceFlags.WORLD, -1, perfSource) < 0.99)
					continue;
				found = true;
				break;
			}
		}

		GetQueryBuffer().Clear();
		return found;
	}

	static float CombatCohesionDist(AIGroup group, IEntity me, IEntity leader, out bool urban)
	{
		urban = SCR_CoverManagerComponent.IsEntityInsideBuilding(me) || SCR_CoverManagerComponent.IsEntityInsideBuilding(leader);
		float d = 30.0;
		if (urban)
			d = 15.0;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(group);
		if (g)
		{
			d = g.GetCombatCohesionOpen();
			if (urban)
				d = g.GetCombatCohesionUrban();
		}
		DCO_GroupConfigComponent cfg = DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
		if (cfg)
			d *= cfg.GetCohesionMult();
		return d;
	}

	static bool OutsideCombatCohesion(AIAgent agent, IEntity me, out vector leaderPos, out float regroupRadius)
	{
		leaderPos = vector.Zero;
		regroupRadius = 0;
		if (!agent || !me)
			return false;
		AIGroup group = agent.GetParentGroup();
		if (!group || agent == group.GetLeaderAgent() || SCR_DefendWaypoint.Cast(group.GetCurrentWaypoint()))
			return false;
		IEntity leader = group.GetLeaderEntity();
		if (!leader)
			return false;

		SCR_AIGroupUtilityComponent agu;
		SCR_AIGroup sg = SCR_AIGroup.Cast(group);
		if (sg)
			agu = sg.GetGroupUtilityComponent();
		DCO_GroupUtilityComponent gu;
		if (agu)
			gu = agu.DCO_GetUtil();
		string susp;
		if (gu)
		{
			if (gu.m_iDCOTRPhase == DCO_ETRPhase.EXECUTING)
				susp = "tr_executing";
			else if (gu.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
				susp = "support_by_fire";
			else if (gu.GetTask() == DCO_EGroupTask.FLANK)
				susp = "flank";
			else if (gu.m_DCOBreak)
				susp = "break_contact";
		}
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(agent);
		if (susp.IsEmpty() && ca && ca.m_UtilityComponent && DCO_BoundBehavior.Cast(ca.m_UtilityComponent.GetCurrentAction()))
			susp = "bounding";
		if (!susp.IsEmpty())
		{
			DCO_Perf.Count("cohesion:suspended:" + susp);
			return false;
		}

		leaderPos = leader.GetOrigin();
		bool urban;
		float maxDist = CombatCohesionDist(group, me, leader, urban);
		bool team = false;
		if (agu && agu.m_FireteamMgr && agu.m_FireteamMgr.GetFireteamCount() >= 2)
		{
			SCR_AIGroupFireteam ft = agu.m_FireteamMgr.FindFireteam(agent);
			if (ft && ft.GetMemberCount() >= 2)
			{
				array<AIAgent> mates = {};
				ft.GetMembers(mates);
				AIAgent teamLead;
				foreach (AIAgent m : mates)
				{
					if (m && IsAlive(m.GetControlledEntity()))
					{
						teamLead = m;
						break;
					}
				}
				if (teamLead)
				{
					team = true;
					if (teamLead == agent)
						maxDist = 50.0;
					else
					{
						leaderPos = teamLead.GetControlledEntity().GetOrigin();
						maxDist = 22.0;
						if (urban)
							maxDist = 15.0;
					}
				}
			}
		}
		regroupRadius = maxDist * 0.5;
		if (vector.Distance(leaderPos, me.GetOrigin()) <= maxDist)
			return false;
		if (team)
			DCO_Perf.Count("cohesion:team_regroup");
		else if (urban)
			DCO_Perf.Count("combat_cohesion_regroup:urban");
		else
			DCO_Perf.Count("combat_cohesion_regroup:open");
		return true;
	}

	static bool HasSquadmateInFireLine(IEntity self, vector target, float width)
	{
		IEntity blocker;
		return FireLaneCheck(self, target, width, blocker) != 0;
	}

	static bool IsMGShooter(IEntity self)
	{
		if (!self)
			return false;
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(self.FindComponent(BaseWeaponManagerComponent));
		return wm && wm.GetCurrentWeapon() && wm.GetCurrentWeapon().GetWeaponType() == EWeaponType.WT_MACHINEGUN;
	}

	static float VolumeRadius(SCR_AISuppressionVolumeBase v)
	{
		SCR_AISuppressionVolumeBox box = SCR_AISuppressionVolumeBox.Cast(v);
		if (box)
			return Math.Max(box.m_vBBMax[0] - box.m_vBBMin[0], box.m_vBBMax[2] - box.m_vBBMin[2]) * 0.5;
		SCR_AISuppressionVolumeSphere sph = SCR_AISuppressionVolumeSphere.Cast(v);
		if (sph)
			return sph.m_fRadius;
		return 0;
	}

	static int FireLaneCheck(IEntity self, vector target, float minWidth, out IEntity blocker)
	{
		blocker = null;
		if (!self)
			return 0;
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			RecordLaneWidth(self, minWidth);
		AIControlComponent ctrl = AIControlComponent.Cast(self.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return 0;
		AIGroup grp = ctrl.GetControlAIAgent().GetParentGroup();
		if (!grp)
			return 0;

		float muzzleY = -10000;
		ChimeraCharacter selfCh = ChimeraCharacter.Cast(self);
		if (selfCh && selfCh.IsInVehicle() && selfCh.GetCompartmentAccessComponent())
		{
			BaseCompartmentSlot seat = selfCh.GetCompartmentAccessComponent().GetCompartment();
			if (seat && seat.GetType() == ECompartmentType.PILOT)
			{
				DCO_Perf.Count("ff_lane:skip_driver");
				return 0;
			}
			if (seat && seat.GetType() == ECompartmentType.TURRET)
				muzzleY = self.GetOrigin()[1] + 1.0;
		}
		float aimY = target[1] + 1.0;
		if (muzzleY < -9000 && selfCh && !selfCh.IsInVehicle() && DCO_Ctx.TuneFor(self, "ff.vert", 1.0) > 0 && selfCh.GetCharacterController())
		{
			ECharacterStance myStance = selfCh.GetCharacterController().GetStance();
			muzzleY = self.GetOrigin()[1] + 1.5;
			if (myStance == ECharacterStance.CROUCH)
				muzzleY = self.GetOrigin()[1] + 1.0;
			else if (myStance == ECharacterStance.PRONE)
				muzzleY = self.GetOrigin()[1] + 0.3;
		}
		if (DCO_Ctx.TuneFor(self, "air.on", 0.0) > 0)
		{
			float horiz = vector.DistanceXZ(self.GetOrigin(), target);
			float rise = target[1] - self.GetOrigin()[1] - 1.5;
			if (horiz > 1.0 && Math.Atan2(rise, horiz) * Math.RAD2DEG > DCO_Ctx.TuneFor(self, "air.lane_skip_deg", 10.0))
			{
				DCO_Perf.Count("air:lane_skip");
				return 0;
			}
			if (muzzleY < -9000)
				muzzleY = self.GetOrigin()[1] + 1.5;
		}

		vector from = self.GetOrigin();
		vector dir = target - from;
		dir[1] = 0;
		float len = dir.Length();
		if (len < 1.0)
			return 0;
		dir = dir * (1.0 / len);
		bool mg = IsMGShooter(self);
		float tanA = 0.035;
		if (mg)
			tanA = 0.06;
		float volR = Math.Max(minWidth, 0);
		float reach = len + 20.0;
		if (len < DCO_Ctx.TuneFor(self, "rt.close_m", 30.0))
		{
			reach = len + 5.0;
			DCO_Perf.Count("ff_lane:near_reach");
		}

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int r = LaneScan(self, agents, from, dir, reach, len, volR, tanA, mg, blocker, muzzleY, aimY);
		if (r != 0)
			return r;

		r = LaneScanNear(self, grp, from, dir, reach, len, volR, tanA, mg, blocker, muzzleY, aimY);
		if (r != 0)
			return r;
		DCO_GroupUtilityComponent own = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!own || !own.GetMyCommander())
			return 0;
		float pre = (reach + 60.0) * (reach + 60.0);
		foreach (DCO_GroupUtilityComponent g : own.GetMyCommander().GetOwnedGroups())
		{
			if (!g || g == own || vector.DistanceSqXZ(g.GetOwner().GetOrigin(), from) > pre)
				continue;
			AIGroup og = AIGroup.Cast(g.GetOwner());
			if (!og)
				continue;
			agents.Clear();
			og.GetAgents(agents);
			r = LaneScan(self, agents, from, dir, reach, len, volR, tanA, mg, blocker, muzzleY, aimY);
			if (r == 1)
				return 2;
			if (r != 0)
				return r;
		}
		IEntity nf = DCO_NoFireAreas.Block(own, self, from, target);
		if (nf)
		{
			blocker = nf;
			return 4;
		}
		return 0;
	}

	static float LaneHalfWidth(float along, float len, float volR, float tanA)
	{
		float w = Math.Max(1.2, along * tanA);
		if (along >= len - volR)
			w = Math.Max(w, volR);
		return w;
	}

	protected static bool LineClearsHead(IEntity e, float along, float len, float muzzleY, float aimY)
	{
		if (muzzleY < -9000 || len < 1.0)
			return false;
		float lineY = muzzleY + (aimY - muzzleY) * Math.Clamp(along / len, 0, 1);
		float head = 1.8;
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (ch && ch.GetCharacterController())
		{
			ECharacterStance st = ch.GetCharacterController().GetStance();
			if (st == ECharacterStance.CROUCH)
				head = 1.2;
			else if (st == ECharacterStance.PRONE)
				head = 0.4;
		}
		return lineY > e.GetOrigin()[1] + head + 0.5;
	}

	protected static ref array<IEntity> s_aLaneNear;

	protected static bool OnLaneNear(IEntity e)
	{
		if (ChimeraCharacter.Cast(e))
			s_aLaneNear.Insert(e);
		return true;
	}

	protected static int LaneScanNear(IEntity self, AIGroup grp, vector from, vector dir, float reach, float len, float volR, float tanA, bool mg, out IEntity blocker, float muzzleY, float aimY)
	{
		FactionAffiliationComponent sfa = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!sfa || !sfa.GetAffiliatedFaction())
			return 0;
		Faction myFac = sfa.GetAffiliatedFaction();
		float near = Math.Min(reach, 60.0);
		s_aLaneNear = {};
		DCO_Perf.Count("q:DCO_FireLaneNear");
		GetGame().GetWorld().QueryEntitiesBySphere(from + dir * (near * 0.5), near * 0.5 + volR + 3.0, OnLaneNear);
		array<AIAgent> others = {};
		foreach (IEntity e : s_aLaneNear)
		{
			if (e == self)
				continue;
			FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
			if (!fa || !fa.GetAffiliatedFaction() || (fa.GetAffiliatedFaction() != myFac && !myFac.IsFactionFriendly(fa.GetAffiliatedFaction())))
				continue;
			AIControlComponent ac = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
			AIAgent ag;
			if (ac)
				ag = ac.GetControlAIAgent();
			if (ag && ag.GetParentGroup() == grp)
				continue;
			int code = LaneTest(self, e, from, dir, reach, len, volR, tanA, mg, muzzleY, aimY);
			if (code == 0)
				continue;
			blocker = e;
			s_aLaneNear = null;
			if (code == 1)
				return 2;
			return code;
		}
		s_aLaneNear = null;
		return 0;
	}

	protected static int LaneTest(IEntity self, IEntity e, vector from, vector dir, float reach, float len, float volR, float tanA, bool mg, float muzzleY, float aimY)
	{
		if (!IsNotDead(e))
			return 0;
		IEntity selfVeh = CompartmentAccessComponent.GetVehicleIn(self);
		if (selfVeh && CompartmentAccessComponent.GetVehicleIn(e) == selfVeh)
			return 0;
		vector rel = e.GetOrigin() - from;
		rel[1] = 0;
		float along = vector.Dot(rel, dir);
		if (along <= 0)
			return 0;
		float side = (rel - dir * along).Length();
		bool mgZone = mg && along <= 30.0 && side <= 1.0 + along * 0.08;
		bool lane = along >= 0.5 && along <= reach && side < LaneHalfWidth(along, len, volR, tanA);
		if (!mgZone && !lane)
			return 0;
		if (LineClearsHead(e, along, len, muzzleY, aimY))
		{
			DCO_Perf.Count("ff_lane:vert_clear");
			return 0;
		}
		if (mgZone)
			return 3;
		return 1;
	}

	protected static int LaneScan(IEntity self, array<AIAgent> agents, vector from, vector dir, float reach, float len, float volR, float tanA, bool mg, out IEntity blocker, float muzzleY, float aimY)
	{
		IEntity selfVeh = CompartmentAccessComponent.GetVehicleIn(self);
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || e == self || !IsNotDead(e))
				continue;
			if (selfVeh && CompartmentAccessComponent.GetVehicleIn(e) == selfVeh)
			{
				DCO_Perf.Count("ff_lane:skip_same_vehicle");
				continue;
			}
			vector rel = e.GetOrigin() - from;
			rel[1] = 0;
			float along = vector.Dot(rel, dir);
			if (along <= 0)
				continue;
			float side = (rel - dir * along).Length();
			bool inZone = (mg && along <= 30.0 && side <= 1.0 + along * 0.08) || (along >= 0.5 && along <= reach && side < LaneHalfWidth(along, len, volR, tanA));
			if (inZone && LineClearsHead(e, along, len, muzzleY, aimY))
			{
				if (selfVeh)
					DCO_Perf.Count("ff_lane:turret_clear");
				else
					DCO_Perf.Count("ff_lane:vert_clear");
				continue;
			}
			if (mg && along <= 30.0 && side <= 1.0 + along * 0.08)
			{
				blocker = e;
				return 3;
			}
			if (along >= 0.5 && along <= reach && side < LaneHalfWidth(along, len, volR, tanA))
			{
				blocker = e;
				return 1;
			}
		}
		return 0;
	}

	static string LaneReason(int r)
	{
		if (r == 1)
			return "own";
		if (r == 2)
			return "other_group";
		if (r == 3)
			return "mg_zone";
		if (r == 4)
			return "nofire_area";
		return "none";
	}

	protected static ref map<IEntity, float> s_mLaneLog;

	static void LaneEvent(IEntity shooter, vector target, int reason, IEntity blocker, string action)
	{
		if (!shooter || reason == 0)
			return;
		DCO_Perf.Count("ff_lane:" + LaneReason(reason));
		if (action == "switch")
			DCO_Perf.Count("ff_lane:switch_target");
		else if (action == "slide")
			DCO_Perf.Count("ff_lane:mg_slide");
		else
			DCO_Perf.Count("ff_lane:cease");
		if (action == "cease" && DCO_Log.On(DCO_ELogChannel.TACTIC, DCO_ELogLevel.DEBUG))
			RecordCease(shooter, blocker);
		if (!DCO_Log.On(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!s_mLaneLog)
			s_mLaneLog = new map<IEntity, float>();
		float last;
		if (s_mLaneLog.Find(shooter, last) && now - last < 10000.0)
			return;
		if (s_mLaneLog.Count() > 500)
			s_mLaneLog.Clear();
		s_mLaneLog.Set(shooter, now);

		string grpId = "-";
		string role = "other";
		AIControlComponent ctrl = AIControlComponent.Cast(shooter.FindComponent(AIControlComponent));
		if (ctrl && ctrl.GetControlAIAgent() && ctrl.GetControlAIAgent().GetParentGroup())
		{
			AIGroup sg = ctrl.GetControlAIAgent().GetParentGroup();
			grpId = DCO_Log.IdGroup(sg);
			DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(sg.FindComponent(DCO_GroupUtilityComponent));
			if (gu && gu.m_iDCOTRPhase >= 0)
			{
				if (gu.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
					role = "fix";
				else if (gu.GetTask() == DCO_EGroupTask.ATTACK)
					role = "assault";
			}
		}
		string weapon = "rifle";
		if (IsMGShooter(shooter))
			weapon = "mg";
		string blockGrp = "-";
		float blockDist = 0;
		if (blocker)
		{
			blockDist = vector.DistanceXZ(blocker.GetOrigin(), shooter.GetOrigin());
			AIControlComponent bc = AIControlComponent.Cast(blocker.FindComponent(AIControlComponent));
			if (bc && bc.GetControlAIAgent() && bc.GetControlAIAgent().GetParentGroup())
				blockGrp = DCO_Log.IdGroup(bc.GetControlAIAgent().GetParentGroup());
		}
		DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(shooter), string.Format("ff_lane_block grp=%1 role=%2 weapon=%3 dist=%4 blocker=%5 blocker_dist=%6 reason=%7 action=%8",
			grpId, role, weapon, Math.Round(vector.DistanceXZ(shooter.GetOrigin(), target)), blockGrp, Math.Round(blockDist), LaneReason(reason), action));
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			LaneGeo(shooter, target, reason, blocker);
	}

	protected static bool QueryBufferCallback(IEntity ent)
	{
		if (ent)
			GetQueryBuffer().Insert(ent);
		return true;
	}

	static bool IsAlive(IEntity ent)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch)
			return false;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		return ctrl && ctrl.GetLifeState() == ECharacterLifeState.ALIVE;
	}

	static bool IsNotDead(IEntity ent)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch)
			return false;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		return ctrl && ctrl.GetLifeState() != ECharacterLifeState.DEAD;
	}

	protected static ref TraceParam s_Trace;
	protected static int s_iTraceDefFlags;
	protected static int s_iTraceDefMask;
	protected static IEntity s_TraceHit;

	static float TraceFrac(vector from, vector to, IEntity exclude, int flags, int layerMask, string perfSource)
	{
		if (!s_Trace)
		{
			s_Trace = new TraceParam();
			s_iTraceDefFlags = s_Trace.Flags;
			s_iTraceDefMask = s_Trace.LayerMask;
		}

		s_Trace.Start = from;
		s_Trace.End = to;
		s_Trace.Exclude = exclude;
		s_Trace.ExcludeArray = null;
		s_Trace.TraceEnt = null;
		if (flags >= 0)
			s_Trace.Flags = flags;
		else
			s_Trace.Flags = s_iTraceDefFlags;
		if (layerMask >= 0)
			s_Trace.LayerMask = layerMask;
		else
			s_Trace.LayerMask = s_iTraceDefMask;

		if (!perfSource.IsEmpty())
			DCO_Perf.Count(perfSource);
		float frac = GetGame().GetWorld().TraceMove(s_Trace, null);
		s_TraceHit = s_Trace.TraceEnt;
		return frac;
	}

	protected static ref map<IEntity, float> s_mCeaseAt;
	protected static ref map<IEntity, IEntity> s_mCeaseBy;
	protected static ref map<IEntity, float> s_mLaneW;

	static string XZ(vector v)
	{
		int x = Math.Round(v[0]);
		int z = Math.Round(v[2]);
		return x.ToString() + "," + z.ToString();
	}

	static string PrefabShort(IEntity e)
	{
		if (!e || !e.GetPrefabData())
			return "-";
		string n = e.GetPrefabData().GetPrefabName();
		int slash = n.LastIndexOf("/");
		if (slash >= 0)
			n = n.Substring(slash + 1, n.Length() - slash - 1);
		n.Replace(".et", "");
		return n;
	}

	static int B01(bool b)
	{
		if (b)
			return 1;
		return 0;
	}

	static string HitKind(IEntity hit)
	{
		if (!hit)
			return "terrain";
		IEntity root = hit.GetRootParent();
		if (!root)
			root = hit;
		if (BaseTree.Cast(hit) || BaseTree.Cast(root))
			return "tree";
		if (Vehicle.Cast(root))
			return "vehicle";
		if (ChimeraCharacter.Cast(root))
			return "other";
		string cn = root.ClassName();
		if (cn.Contains("Terrain"))
			return "terrain";
		if (cn.Contains("Building") || cn.Contains("House"))
			return "building";
		EntityPrefabData pd = root.GetPrefabData();
		if (pd)
		{
			string path = pd.GetPrefabName();
			if (path.Contains("Buildings") || path.Contains("Structures/Houses"))
				return "building";
		}
		return "other";
	}

	protected static void RecordCease(IEntity shooter, IEntity blocker)
	{
		if (!s_mCeaseAt)
		{
			s_mCeaseAt = new map<IEntity, float>();
			s_mCeaseBy = new map<IEntity, IEntity>();
		}
		if (s_mCeaseAt.Count() > 500)
		{
			s_mCeaseAt.Clear();
			s_mCeaseBy.Clear();
		}
		s_mCeaseAt.Set(shooter, GetGame().GetWorld().GetWorldTime());
		s_mCeaseBy.Set(shooter, blocker);
	}

	static bool CeasedRecently(IEntity shooter, float window_ms, out IEntity blocker)
	{
		blocker = null;
		float at = -1;
		if (!shooter || !s_mCeaseAt || !s_mCeaseAt.Find(shooter, at))
			return false;
		if (GetGame().GetWorld().GetWorldTime() - at > window_ms)
			return false;
		blocker = s_mCeaseBy.Get(shooter);
		return true;
	}

	protected static void RecordLaneWidth(IEntity self, float minWidth)
	{
		if (!s_mLaneW)
			s_mLaneW = new map<IEntity, float>();
		if (s_mLaneW.Count() > 500)
			s_mLaneW.Clear();
		s_mLaneW.Set(self, minWidth);
	}

	protected static AIGroup GroupOfEnt(IEntity e)
	{
		if (!e)
			return null;
		AIControlComponent c = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (!c || !c.GetControlAIAgent())
			return null;
		return c.GetControlAIAgent().GetParentGroup();
	}

	protected static string LaneSrc(IEntity shooter)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(shooter.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return "-";
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(ctrl.GetControlAIAgent().FindComponent(SCR_AIUtilityComponent));
		if (!u || !u.GetCurrentAction())
			return "-";
		AIActionBase act = u.GetCurrentAction();
		if (SCR_AISuppressGroupClusterBehavior.Cast(act))
			return "group_suppress";
		if (SCR_AISuppressBehavior.Cast(act))
			return "suppress";
		if (SCR_AIAttackBehavior.Cast(act))
			return "attack";
		return act.ClassName();
	}

	protected static string StanceName(IEntity e)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch || !ch.GetCharacterController())
			return "-";
		return typename.EnumToString(ECharacterStance, ch.GetCharacterController().GetStance());
	}

	protected static string BlockerRole(IEntity shooter, IEntity blocker)
	{
		if (!blocker)
			return "-";
		AIGroup bg = GroupOfEnt(blocker);
		if (!bg)
			return "lain";
		if (bg == GroupOfEnt(shooter))
			return "sama_regu";
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(bg.FindComponent(DCO_GroupUtilityComponent));
		if (gu && gu.m_iDCOTRPhase >= 0)
		{
			if (gu.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
				return "fix";
			if (gu.GetTask() == DCO_EGroupTask.ATTACK)
				return "assault";
		}
		return "lain";
	}

	protected static string LaneZone(int reason, bool atTarget)
	{
		if (reason == 3)
			return "mg_front";
		if (atTarget)
			return "target_volume";
		return "lane";
	}

	protected static void LaneGeo(IEntity shooter, vector target, int reason, IEntity blocker)
	{
		vector from = shooter.GetOrigin();
		vector dir = target - from;
		dir[1] = 0;
		float tgtD = dir.Length();
		if (tgtD > 0.01)
			dir = dir * (1.0 / tgtD);
		float along = -1;
		float side = -1;
		if (blocker)
		{
			vector rel = blocker.GetOrigin() - from;
			rel[1] = 0;
			along = vector.Dot(rel, dir);
			side = (rel - dir * along).Length();
		}
		float minW = -1;
		if (s_mLaneW && s_mLaneW.Contains(shooter))
			minW = s_mLaneW.Get(shooter);
		float tanA = 0.035;
		if (IsMGShooter(shooter))
			tanA = 0.06;
		float halfW = LaneHalfWidth(Math.Max(along, 0), tgtD, Math.Max(minW, 0), tanA);
		bool atTarget = along >= tgtD - Math.Max(minW, 0);
		if (reason == 3)
			halfW = 1.0 + Math.Max(along, 0) * 0.08;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(shooter), string.Format("ff_lane_geo shooter=%1 along=%2 side=%3 half_w=%4 min_w=%5 tgt_d=%6 src=%7 shooter_stance=%8 blocker_stance=%9",
			DCO_Log.IdUnit(shooter), along.ToString(-1, 1), side.ToString(-1, 1), halfW.ToString(-1, 1), minW.ToString(-1, 1), Math.Round(tgtD), LaneSrc(shooter), StanceName(shooter), StanceName(blocker)) + " blocker_role=" + BlockerRole(shooter, blocker) + " vol_r=" + Math.Max(minW, 0).ToString(-1, 1) + " zone=" + LaneZone(reason, atTarget));
	}

	static IEntity LastTraceEnt()
	{
		return s_TraceHit;
	}

	static bool TraceClear(vector from, vector to, IEntity exclude, float minFrac, int flags, int layerMask, IEntity acceptHitRoot, string perfSource)
	{
		if (TraceFrac(from, to, exclude, flags, layerMask, perfSource) >= minFrac)
			return true;
		return acceptHitRoot && s_TraceHit && s_TraceHit.GetRootParent() == acceptHitRoot.GetRootParent();
	}

	protected static float s_fNightChecked_ms = -1;
	protected static bool s_bNight;

	static bool IsNight()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return false;

		float now = world.GetWorldTime();
		if (s_fNightChecked_ms >= 0 && now - s_fNightChecked_ms <= 10000.0 && now >= s_fNightChecked_ms)
			return s_bNight;

		s_fNightChecked_ms = now;
		s_bNight = false;
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		float sunrise, sunset;
		if (manager && manager.GetSunriseHour(sunrise) && manager.GetSunsetHour(sunset))
		{
			float hour = manager.GetTimeOfTheDay();
			s_bNight = hour < sunrise || hour > sunset;
		}
		return s_bNight;
	}

	static bool IsPlayerGroup(SCR_AIGroup grp)
	{
		if (!grp)
			return false;
		IEntity leader = grp.GetLeaderEntity();
		if (grp.GetLeaderID() > 0 || SCR_CharacterHelper.IsPlayer(leader) || (!leader && grp.GetTotalPlayerCount() > 0))
			return true;
		array<int> ids = grp.GetPlayerIDs();
		return ids && !ids.IsEmpty();
	}

	static FactionKey FactionKeyOf(IEntity ent, bool useDefault)
	{
		if (!ent)
			return string.Empty;
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return string.Empty;
		Faction f = fac.GetAffiliatedFaction();
		if (!f && useDefault)
			f = fac.GetDefaultAffiliatedFaction();
		if (!f)
			return string.Empty;
		return f.GetFactionKey();
	}

	static SCR_EAICombatMoveDirection PickCoverDirection(float weightBack, float weightLeft, float weightRight, float weightAnywhere)
	{
		float wBack     = Math.Max(0.0, weightBack);
		float wLeft     = Math.Max(0.0, weightLeft);
		float wRight    = Math.Max(0.0, weightRight);
		float wAnywhere = Math.Max(0.0, weightAnywhere);

		float total = wBack + wLeft + wRight + wAnywhere;
		if (total <= 0)
			return SCR_EAICombatMoveDirection.BACKWARD;

		float r = Math.RandomFloat(0, total);
		if (r < wBack)
			return SCR_EAICombatMoveDirection.BACKWARD;
		r -= wBack;
		if (r < wLeft)
			return SCR_EAICombatMoveDirection.LEFT;
		r -= wLeft;
		if (r < wRight)
			return SCR_EAICombatMoveDirection.RIGHT;
		return SCR_EAICombatMoveDirection.ANYWHERE;
	}

}
