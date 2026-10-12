class DCO_BattleUnit
{
	IEntity m_Ent;
	string m_sId;
	string m_sGrp;
	string m_sRoles;
	string m_sFac;
	float m_fBorn;
	float m_fDied = -1;
	int m_iKills;
	int m_iFF;
	int m_iHits;
	float m_fLastHit_ms = -100000;
	int m_iWounded;
	int m_iHealed;
	float m_fDist;
	vector m_vLast;
	int m_iSamples;
	int m_iCover;
	int m_iMove;
	int m_iProne;
	string m_sCause = "-";
}

class DCO_BattleGroup
{
	AIGroup m_Grp;
	string m_sId;
	string m_sName;
	string m_sFac;
	int m_iStart;
	int m_iLost;
	int m_iKills;
	string m_sTasks;
	string m_sTR = "-";
	int m_iObjTaken;
	int m_iAlive;
}

class DCO_Battle
{
	protected static ref map<string, ref array<string>> s_mRec;
	protected static ref map<IEntity, ref DCO_BattleUnit> s_mUnits;
	protected static ref map<AIGroup, ref DCO_BattleGroup> s_mGroups;
	protected static ref map<IEntity, string> s_mOrderTag;
	protected static ref map<IEntity, float> s_mOrderAt;
	protected static ref map<IEntity, bool> s_mDone;
	protected static ref map<string, string> s_mObjCmd;
	protected static ref map<string, string> s_mCmdTactics;
	protected static ref map<string, string> s_mObjStart;
	protected static ref map<string, float> s_mIntelAt;
	protected static ref map<string, int> s_mIntelId;
	protected static int s_iIntelSeq;
	protected static float s_fStart = -1;
	protected static bool s_bHooked;
	protected static bool s_bEnded;

	protected static void Init()
	{
		if (s_mRec)
			return;
		s_mRec = new map<string, ref array<string>>();
		s_mUnits = new map<IEntity, ref DCO_BattleUnit>();
		s_mGroups = new map<AIGroup, ref DCO_BattleGroup>();
		s_mOrderTag = new map<IEntity, string>();
		s_mOrderAt = new map<IEntity, float>();
		s_mDone = new map<IEntity, bool>();
		s_mObjCmd = new map<string, string>();
		s_mCmdTactics = new map<string, string>();
		s_mObjStart = new map<string, string>();
		s_mIntelAt = new map<string, float>();
		s_mIntelId = new map<string, int>();
	}

	static void Start()
	{
		if (!Replication.IsServer() || s_fStart >= 0 || DCO_Log.Mode() != DCO_ELogMode.FULL)
			return;
		Init();
		s_fStart = DCO_Util.NowS();
		s_bEnded = false;
		GetGame().GetCallqueue().CallLater(Sample, 5000, true);
	}

	protected static void Hook()
	{
		if (s_bHooked)
			return;
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
			return;
		gm.GetOnControllableDestroyed().Insert(OnDestroyed);
		s_bHooked = true;
	}

	protected static void OnDestroyed(notnull SCR_InstigatorContextData data)
	{
		IEntity victim = data.GetVictimEntity();
		if (!victim || !ChimeraCharacter.Cast(victim))
			return;
		GetGame().GetCallqueue().CallLater(DeathCtx, 0, false, victim, data.GetKillerEntity());
	}

	static void NoteOrder(IEntity ent, string tag)
	{
		if (!ent || !Replication.IsServer())
			return;
		Init();
		if (s_mOrderTag.Count() > 3000)
		{
			s_mOrderTag.Clear();
			s_mOrderAt.Clear();
		}
		s_mOrderTag.Set(ent, tag);
		s_mOrderAt.Set(ent, DCO_Util.NowS());
	}

	protected static string Prefab(IEntity e)
	{
		if (!e || !e.GetPrefabData())
			return "-";
		return FilePath.StripExtension(FilePath.StripPath(e.GetPrefabData().GetPrefabName()));
	}

	protected static SCR_AIUtilityComponent UtilOf(IEntity e)
	{
		if (!e)
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return null;
		return SCR_AIUtilityComponent.Cast(ctrl.GetControlAIAgent().FindComponent(SCR_AIUtilityComponent));
	}

	protected static AIGroup GroupOf(IEntity e)
	{
		if (!e)
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetControlAIAgent())
			return null;
		return ctrl.GetControlAIAgent().GetParentGroup();
	}

	static string RoleNames(int mask)
	{
		string s;
		if (mask & 64)
			s = Join(s, "ar");
		else if (mask & 1)
			s = Join(s, "mg");
		if (mask & 2)
			s = Join(s, "at");
		if (mask & 4)
			s = Join(s, "marksman");
		if (mask & 8)
			s = Join(s, "gl");
		if (mask & 16)
			s = Join(s, "rto");
		if (mask & 32)
			s = Join(s, "leader");
		if (s.IsEmpty())
			return "rifle";
		return s;
	}

	protected static string Join(string a, string b)
	{
		if (a.IsEmpty())
			return b;
		return a + "|" + b;
	}

	protected static string Stance(IEntity e)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch || !ch.GetCharacterController())
			return "-";
		return typename.EnumToString(ECharacterStance, ch.GetCharacterController().GetStance());
	}

	protected static float Speed(IEntity e)
	{
		if (!e || !e.GetPhysics())
			return 0;
		return e.GetPhysics().GetVelocity().Length();
	}

	protected static bool InVehicle(IEntity e)
	{
		if (Vehicle.Cast(e))
			return true;
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		return ch && ch.IsInVehicle();
	}

	protected static string CoverKind(IEntity victim, IEntity killer, SCR_AIUtilityComponent vu, bool indoor)
	{
		if (indoor)
			return "building";
		if (!vu || !vu.m_CombatMoveState || !vu.m_CombatMoveState.IsInValidCover())
			return "none";
		vector from = victim.GetOrigin() + Vector(0, 1.0, 0);
		vector dir = vector.Forward;
		if (killer)
			dir = DCO_Specialist.Flat(killer.GetOrigin() - victim.GetOrigin());
		float f = DCO_Util.TraceFrac(from, from + dir * 2.5, victim, TraceFlags.WORLD | TraceFlags.ENTS, -1, "");
		if (f >= 0.99)
			return "object";
		string k = DCO_Util.HitKind(DCO_Util.LastTraceEnt());
		if (k == "terrain" || k == "building" || k == "vehicle")
			return k;
		return "object";
	}

	protected static string TacticOf(DCO_GroupUtilityComponent g)
	{
		if (!g || !g.GetMyCommander() || !g.GetGroupObjective())
			return "-";
		AICommander_BaseComponent cmd = g.GetMyCommander();
		DCO_TacticPlan tp = cmd.GetTactics().GetPlan(g.GetGroupObjective());
		if (tp && tp.m_aGroups.Contains(g))
			return DCO_CommanderTactics.Name(tp.m_eTactic);
		DCO_DefensePlan dp = cmd.GetDefenseTactics().GetPlan(g.GetGroupObjective());
		if (dp)
			return "DEF_" + DCO_DefenseTactics.Name(dp.m_eTactic);
		return "-";
	}

	protected static int EnemiesWithLos(IEntity victim, SCR_AIUtilityComponent vu)
	{
		if (!vu || !vu.m_PerceptionComponent)
			return 0;
		array<BaseTarget> targets = {};
		vu.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		int n = 0;
		int checks = 0;
		vector aim = victim.GetOrigin() + Vector(0, 1.2, 0);
		foreach (BaseTarget t : targets)
		{
			if (checks >= 8)
				break;
			IEntity te = null;
			if (t)
				te = t.GetTargetEntity();
			if (!te)
				continue;
			checks++;
			if (DCO_Util.TraceFrac(te.GetOrigin() + Vector(0, 1.5, 0), aim, te, TraceFlags.WORLD, -1, "") >= 0.98)
				n++;
		}
		return n;
	}

	protected static float SeenAge(SCR_AIUtilityComponent vu, IEntity killer)
	{
		if (!vu || !vu.m_PerceptionComponent || !killer)
			return -1;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return -1;
		IEntity kroot = killer.GetRootParent();
		array<BaseTarget> targets = {};
		vu.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		array<BaseTarget> det = {};
		vu.m_PerceptionComponent.GetTargetsList(det, ETargetCategory.DETECTED);
		targets.InsertAll(det);
		foreach (BaseTarget t : targets)
		{
			if (!t || !t.GetTargetEntity())
				continue;
			IEntity te = t.GetTargetEntity();
			if (te != killer && te != kroot && te.GetRootParent() != kroot)
				continue;
			float seen = t.GetTimeLastSeen();
			if (seen <= 0)
				return -1;
			return pm.GetTime() - seen;
		}
		return -1;
	}

	protected static float NearestFriend(IEntity victim, string fac)
	{
		AIWorld aw = GetGame().GetAIWorld();
		if (!aw)
			return -1;
		array<AIAgent> agents = {};
		aw.GetAIAgents(agents);
		float best = -1;
		foreach (AIAgent a : agents)
		{
			IEntity e = null;
			if (a)
				e = a.GetControlledEntity();
			if (!e || e == victim || !DCO_Util.IsAlive(e) || DCO_Util.FactionKeyOf(e, true) != fac)
				continue;
			float d = vector.Distance(e.GetOrigin(), victim.GetOrigin());
			if (best < 0 || d < best)
				best = d;
		}
		return best;
	}

	protected static string Cause(bool ff, string dmg, string src, bool kveh, float dist, bool vindoor, bool kindoor, bool bleed, string medic, int seen, float seenAge, bool vmoving, string vcover, bool vwindow, int enemiesLos)
	{
		if (dmg == "COLLISION")
			return "vehicle_collision";
		if (ff)
			return "fratricide";
		string low = src;
		low.ToLower();
		if (dist < 0 && (low.IsEmpty() || low == "-") && dmg != "EXPLOSIVE")
			return "no_attacker";
		if (low.Contains("mortar") || low.Contains("artillery") || low.Contains("shell") || low.Contains("howitzer"))
			return "idf";
		if (dmg == "EXPLOSIVE" || low.Contains("grenade") || low.Contains("rocket") || low.Contains("rpg") || low.Contains("vog") || low.Contains("m433") || low.Contains("40mm"))
			return "grenade";
		if (kveh)
			return "vehicle_fire";
		if (dist >= 0 && dist <= 15.0 && (vindoor || kindoor))
			return "cqb";
		if (bleed && medic == "none")
			return "bleed_no_medic";
		if (seen == 0 || seenAge > 10.0)
			return "unseen_shooter";
		if (vmoving && vcover == "none")
			return "moving_open";
		if (vwindow)
			return "exposed_window";
		if (vcover == "none")
			return "static_open";
		if (enemiesLos >= 3)
			return "in_cover_outgunned";
		return "in_cover";
	}

	static void DeathCtx(IEntity victim, IEntity killer)
	{
		if (!victim || !Replication.IsServer())
			return;
		Init();
		if (s_mDone.Contains(victim))
			return;
		if (s_mDone.Count() > 5000)
			s_mDone.Clear();
		s_mDone.Set(victim, true);
		float now = DCO_Util.NowS();

		SCR_AIUtilityComponent vu = UtilOf(victim);
		AIGroup vg = GroupOf(victim);
		DCO_GroupUtilityComponent vgu = null;
		if (vg)
			vgu = DCO_GroupUtilityComponent.Cast(vg.FindComponent(DCO_GroupUtilityComponent));
		string vfac = DCO_Util.FactionKeyOf(victim, true);
		string vgrp = "-";
		if (vg)
			vgrp = DCO_Log.IdGroup(vg);
		else if (SCR_CharacterHelper.IsPlayer(victim))
			vgrp = "player";
		int vroleMask = 0;
		if (vu)
			vroleMask = DCO_Roles.Get(vu);
		else
			vroleMask = DCO_Roles.Compute(victim, null);

		string vtask = "-";
		string vphase = "-";
		string vtrRole = "-";
		string vtrPhase = "-";
		string vobj = "-";
		if (vgu)
		{
			vtask = typename.EnumToString(DCO_EGroupTask, vgu.GetTask());
			vphase = typename.EnumToString(DCO_ETaskPhase, vgu.GetPhase());
			if (vgu.m_iDCOTRPhase >= 0)
			{
				vtrPhase = typename.EnumToString(DCO_ETRPhase, vgu.m_iDCOTRPhase);
				if (vgu.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
					vtrRole = "fix";
				else if (vgu.GetTask() == DCO_EGroupTask.ATTACK)
					vtrRole = "assault";
			}
			if (vgu.GetGroupObjective())
				vobj = vgu.GetGroupObjective().GetLogName();
		}
		string vbeh = "-";
		if (vu && vu.GetCurrentAction())
			vbeh = vu.GetCurrentAction().ClassName();
		string vact = "-";
		SCR_AIGroupUtilityComponent vgUtil = null;
		if (vg)
			vgUtil = SCR_AIGroupUtilityComponent.Cast(vg.FindComponent(SCR_AIGroupUtilityComponent));
		if (vgUtil && vgUtil.GetCurrentAction())
			vact = vgUtil.GetCurrentAction().ClassName();
		string vorder = "-";
		float vorderAge = -1;
		if (s_mOrderTag.Find(victim, vorder))
			vorderAge = now - s_mOrderAt.Get(victim);
		else
			vorder = "-";

		float vspeed = Speed(victim);
		bool vmoving = vspeed > 0.5;
		bool vindoor = SCR_CoverManagerComponent.IsEntityInsideBuilding(victim);
		string vcover = CoverKind(victim, killer, vu, vindoor);
		bool vwindow = false;
		if (vgu && vgu.GetGarrisonActivity() && vu)
			vwindow = vgu.GetGarrisonActivity().DCO_AtWindow(vu.GetAIAgent());

		string vthreat = "-";
		string vsupp = "-";
		string vmorale = "-";
		bool vpinned = false;
		string vsuppLvl = "-";
		float vstill = -1;
		if (vu)
		{
			vsuppLvl = DCO_Stress.LevelName(DCO_Stress.Level(vu));
			if (vu.m_CombatMoveState)
				vstill = vu.m_CombatMoveState.m_fTimerStopped_s;
			if (vu.m_ThreatSystem)
			{
				vthreat = typename.EnumToString(EAIThreatState, vu.m_ThreatSystem.GetState());
				vsupp = vu.m_ThreatSystem.GetSuppressionMeasure().ToString(-1, 2);
			}
			if (vu.GetMoraleSystem())
				vmorale = typename.EnumToString(moraleState, vu.GetMoraleSystem().GetState());
			if (vu.m_CombatComponent)
				vpinned = vu.m_CombatComponent.DCO_IsPinned();
		}

		SCR_CharacterDamageManagerComponent vdm = SCR_CharacterDamageManagerComponent.Cast(victim.FindComponent(SCR_CharacterDamageManagerComponent));
		float hpBefore = -1;
		int hits10 = 0;
		string hitzone = "-";
		string dmg = "-";
		string src = "-";
		float woundToDeath = -1;
		bool bleed = false;
		if (vdm)
		{
			hpBefore = vdm.DCO_HpBefore();
			hits10 = vdm.DCO_HitsWithin(10.0);
			hitzone = vdm.DCO_LastHitZone();
			dmg = vdm.DCO_FatalType();
			src = vdm.DCO_FatalSrc();
			if (vdm.DCO_FatalAt() >= 0)
				woundToDeath = now - vdm.DCO_FatalAt();
			bleed = vdm.DCO_LastWasBleed();
		}
		string medic = DCO_MedicDispatcher.StatusFor(victim);

		float vleadD = -1;
		if (vg && vg.GetLeaderEntity() && vg.GetLeaderEntity() != victim)
			vleadD = vector.Distance(vg.GetLeaderEntity().GetOrigin(), victim.GetOrigin());
		float vfriendD = NearestFriend(victim, vfac);
		int enemiesLos = EnemiesWithLos(victim, vu);

		string killerId = "-";
		string kname = "-";
		string kgrp = "-";
		string kroles = "-";
		string kbeh = "-";
		string kstance = "-";
		bool kindoor = false;
		bool kveh = false;
		float dist = -1;
		float dz = 0;
		bool ff = false;
		AIGroup kg = null;
		if (killer && killer != victim)
		{
			kveh = InVehicle(killer);
			if (Vehicle.Cast(killer))
				killerId = DCO_Log.IdVeh(killer);
			else
				killerId = DCO_Log.IdUnit(killer);
			kname = Prefab(killer);
			kg = GroupOf(killer);
			if (kg)
				kgrp = DCO_Log.IdGroup(kg);
			else if (SCR_CharacterHelper.IsPlayer(killer))
				kgrp = "player";
			SCR_AIUtilityComponent ku = UtilOf(killer);
			if (ku)
			{
				kroles = RoleNames(DCO_Roles.Get(ku));
				if (ku.GetCurrentAction())
					kbeh = ku.GetCurrentAction().ClassName();
			}
			else if (ChimeraCharacter.Cast(killer))
				kroles = RoleNames(DCO_Roles.Compute(killer, null));
			kstance = Stance(killer);
			kindoor = SCR_CoverManagerComponent.IsEntityInsideBuilding(killer);
			dist = vector.Distance(killer.GetOrigin(), victim.GetOrigin());
			dz = killer.GetOrigin()[1] - victim.GetOrigin()[1];
			ff = DCO_Util.FactionKeyOf(killer, true) == vfac && !vfac.IsEmpty();
		}
		float seenAge = SeenAge(vu, killer);
		int seen = 0;
		if (seenAge >= 0)
			seen = 1;
		string cause = Cause(ff, dmg, src, kveh, dist, vindoor, kindoor, bleed, medic, seen, seenAge, vmoving, vcover, vwindow, enemiesLos);
		bool vinveh;
		string vseat = "-";
		ChimeraCharacter vch = ChimeraCharacter.Cast(victim);
		if (vch && vch.IsInVehicle() && vch.GetCompartmentAccessComponent())
		{
			vinveh = true;
			BaseCompartmentSlot vslot = vch.GetCompartmentAccessComponent().GetCompartment();
			if (vslot)
				vseat = typename.EnumToString(ECompartmentType, vslot.GetType());
		}

		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-",
			string.Format("death_ctx victim=%1 vname=%2 vfac=%3 vgrp=%4 vroles=%5 vtask=%6 vphase=%7 vtr_role=%8 vtr_phase=%9", DCO_Log.IdUnit(victim), Prefab(victim), vfac, vgrp, RoleNames(vroleMask), vtask, vphase, vtrRole, vtrPhase)
			+ string.Format(" vtactic=%1 vobj=%2 vbeh=%3 vact=%4 vorder=%5 vorder_age_s=%6 vstance=%7 vmoving=%8 vspeed=%9", TacticOf(vgu), vobj, vbeh, vact, vorder, Math.Round(vorderAge), Stance(victim), DCO_Util.B01(vmoving), vspeed.ToString(-1, 1))
			+ string.Format(" vcover=%1 vindoor=%2 vwindow=%3 vthreat=%4 vsupp=%5 vmorale=%6 vpinned=%7 vhp_before=%8 hits10=%9", vcover, DCO_Util.B01(vindoor), DCO_Util.B01(vwindow), vthreat, vsupp, vmorale, DCO_Util.B01(vpinned), hpBefore.ToString(-1, 2), hits10)
			+ string.Format(" hitzone=%1 vlead_d=%2 vfriend_d=%3 enemies_los=%4 killer=%5 kname=%6 kgrp=%7 kroles=%8 kbeh=%9", hitzone, Math.Round(vleadD), Math.Round(vfriendD), enemiesLos, killerId, kname, kgrp, kroles, kbeh)
			+ string.Format(" kstance=%1 kindoor=%2 kveh=%3 dist=%4 dz=%5 seen=%6 seen_age_s=%7 dmg=%8 src=%9", kstance, DCO_Util.B01(kindoor), DCO_Util.B01(kveh), Math.Round(dist), dz.ToString(-1, 1), seen, Math.Round(seenAge), dmg, src)
			+ string.Format(" wound_to_death_s=%1 bleed=%2 medic=%3 ff=%4 cause=%5 pos=%6 kpos=%7 vsupp_lvl=%8 vstill_s=%9", Math.Round(woundToDeath), DCO_Util.B01(bleed), medic, DCO_Util.B01(ff), cause, DCO_Util.XZ(victim.GetOrigin()), KPos(killer), vsuppLvl, vstill.ToString(-1, 1))
			+ string.Format(" vzone=%1 vinveh=%2 vseat=%3 vcid=%4", DCO_Engage.ZoneOf(vu), DCO_Util.B01(vinveh), vseat, DCO_CoverId.Brief(vu, victim))
			+ SmartActionInfo(vu));
		DCO_Perf.Count("death_ctx:" + cause);

		DCO_BattleUnit bu = s_mUnits.Get(victim);
		if (bu)
		{
			bu.m_fDied = now;
			bu.m_sCause = cause;
		}
		if (vg && s_mGroups.Contains(vg))
			s_mGroups.Get(vg).m_iLost++;
		if (killer && s_mUnits.Contains(killer))
		{
			if (ff)
				s_mUnits.Get(killer).m_iFF++;
			else
				s_mUnits.Get(killer).m_iKills++;
		}
		if (kg && !ff && s_mGroups.Contains(kg))
			s_mGroups.Get(kg).m_iKills++;
	}

	protected static string KPos(IEntity killer)
	{
		if (!killer)
			return "-";
		return DCO_Util.XZ(killer.GetOrigin());
	}

	static int GroupKills(AIGroup g)
	{
		if (!g || !s_mGroups || !s_mGroups.Contains(g))
			return 0;
		return s_mGroups.Get(g).m_iKills;
	}

	static int UnitKills(IEntity e)
	{
		if (!e || !s_mUnits || !s_mUnits.Contains(e))
			return 0;
		return s_mUnits.Get(e).m_iKills;
	}

	static int UnitHits(IEntity e)
	{
		if (!e || !s_mUnits || !s_mUnits.Contains(e))
			return 0;
		return s_mUnits.Get(e).m_iHits;
	}

	static void OnHit(IEntity victim, IEntity attacker)
	{
		if (!attacker || !victim || !s_mUnits)
			return;
		DCO_BattleUnit au = s_mUnits.Get(attacker);
		if (au && DCO_Util.FactionKeyOf(victim, true) != au.m_sFac)
		{
			au.m_iHits++;
			au.m_fLastHit_ms = GetGame().GetWorld().GetWorldTime();
		}
	}

	static float LastHitAt(IEntity e)
	{
		if (!e || !s_mUnits || !s_mUnits.Contains(e))
			return -100000.0;
		return s_mUnits.Get(e).m_fLastHit_ms;
	}

	static void OnWounded(IEntity victim)
	{
		if (!victim || !s_mUnits)
			return;
		DCO_BattleUnit u = s_mUnits.Get(victim);
		if (u)
			u.m_iWounded++;
	}

	static void OnHealed(IEntity patient)
	{
		if (!patient || !s_mUnits)
			return;
		DCO_BattleUnit u = s_mUnits.Get(patient);
		if (u)
			u.m_iHealed++;
	}

	static void OnTask(AIGroup g, DCO_EGroupTask task)
	{
		if (!g || !Replication.IsServer())
			return;
		Init();
		DCO_BattleGroup bg = TrackGroup(g);
		if (!bg)
			return;
		string entry = typename.EnumToString(DCO_EGroupTask, task) + "@" + Math.Round(DCO_Util.NowS() - s_fStart).ToString();
		if (bg.m_sTasks.Length() < 600)
			bg.m_sTasks = Join(bg.m_sTasks, entry);
	}

	protected static DCO_BattleGroup TrackGroup(AIGroup g)
	{
		DCO_BattleGroup bg = s_mGroups.Get(g);
		if (bg)
			return bg;
		if (s_mGroups.Count() > 600)
			return null;
		bg = new DCO_BattleGroup();
		bg.m_Grp = g;
		bg.m_sId = DCO_Log.IdGroup(g);
		bg.m_sName = g.GetName();
		bg.m_sFac = "-";
		SCR_AIGroup sg = SCR_AIGroup.Cast(g);
		if (sg && sg.GetFaction())
			bg.m_sFac = sg.GetFaction().GetFactionKey();
		bg.m_iStart = g.GetAgentsCount();
		s_mGroups.Set(g, bg);
		return bg;
	}

	protected static void Sample()
	{
		if (s_bEnded)
			return;
		Hook();
		AIWorld aw = GetGame().GetAIWorld();
		if (!aw)
			return;
		float now = DCO_Util.NowS();
		array<AIAgent> agents = {};
		aw.GetAIAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca)
				continue;
			IEntity e = ca.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			AIGroup g = a.GetParentGroup();
			if (g)
			{
				DCO_BattleGroup tg = TrackGroup(g);
				if (tg)
					tg.m_iStart = Math.Max(tg.m_iStart, g.GetAgentsCount());
			}
			DCO_BattleUnit u = s_mUnits.Get(e);
			if (!u)
			{
				if (s_mUnits.Count() > 3000)
					continue;
				u = new DCO_BattleUnit();
				u.m_Ent = e;
				u.m_sId = DCO_Log.IdUnit(e);
				u.m_sFac = DCO_Util.FactionKeyOf(e, true);
				u.m_sGrp = "-";
				if (g)
					u.m_sGrp = DCO_Log.IdGroup(g);
				u.m_sRoles = "-";
				if (ca.m_UtilityComponent)
					u.m_sRoles = RoleNames(DCO_Roles.Get(ca.m_UtilityComponent));
				u.m_fBorn = now;
				u.m_vLast = e.GetOrigin();
				DCO_ShotCounter.Track(e);
				s_mUnits.Set(e, u);
			}
			u.m_fDist += vector.DistanceXZ(u.m_vLast, e.GetOrigin());
			u.m_vLast = e.GetOrigin();
			u.m_iSamples++;
			if (Speed(e) > 0.5)
				u.m_iMove++;
			if (Stance(e) == "PRONE")
				u.m_iProne++;
			SCR_AIUtilityComponent su = ca.m_UtilityComponent;
			if (su && su.m_CombatMoveState && su.m_CombatMoveState.IsInValidCover())
				u.m_iCover++;
		}
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr && s_mObjStart.IsEmpty())
		{
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (o && o.GetOwner())
					s_mObjStart.Set(o.GetLogName(), OwnerOf(o));
			}
		}
	}

	protected static string OwnerOf(CMD_AICommanderObjectiveComponent o)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return "-";
		array<Faction> facs = {};
		fm.GetFactionsList(facs);
		foreach (Faction f : facs)
		{
			if (f && o.IsCapturedBy(f.GetFactionKey()))
				return f.GetFactionKey();
		}
		return "none";
	}

	static void Intel(AICommander_BaseComponent cmd, CMD_ThreatEntry t, bool isNew)
	{
		if (!cmd || !t || !Replication.IsServer())
			return;
		Init();
		string key = cmd.GetCommanderUID() + "|" + DCO_Util.XZ(t.m_vPosition);
		if (isNew)
		{
			s_iIntelSeq++;
			key = cmd.GetCommanderUID() + "|" + s_iIntelSeq.ToString();
		}
		float now = DCO_Util.NowS();
		float last = -1000;
		if (!isNew && s_mIntelAt.Find(key, last) && now - last < 15.0)
			return;
		if (s_mIntelAt.Count() > 2000)
			s_mIntelAt.Clear();
		s_mIntelAt.Set(key, now);
		string kind = "inf";
		if (t.m_bArtillery)
			kind = "arty";
		else if (t.m_iVehicle > 0 || t.m_iArmor > 0 || t.m_bArmorSeen)
			kind = "veh";
		string source = "report";
		if (t.m_iReporterPid > 0)
			source = "player_report";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("cmd_intel cmd=%1 new=%2 pos=%3 kind=%4 size=%5 source=%6 conf=%7",
			cmd.GetCommanderUID(), DCO_Util.B01(isNew), DCO_Util.XZ(t.m_vPosition), kind, t.m_iEstimatedEnemyCount, source, t.m_fReportQuality.ToString(-1, 2)));
	}

	protected static ref map<string, string> s_mLastOrder;
	protected static ref map<string, float> s_mLastOrderAt;
	protected static ref map<string, vector> s_mLastOrderPos;
	protected static ref map<string, string> s_mLastOrderKind;

	protected static void NoteOrder(SCR_AIGroup grp, string order, vector pos, int seq, string tactic)
	{
		if (seq != 0)
			return;
		if (!s_mLastOrder)
		{
			s_mLastOrder = new map<string, string>();
			s_mLastOrderAt = new map<string, float>();
			s_mLastOrderPos = new map<string, vector>();
			s_mLastOrderKind = new map<string, string>();
		}
		if (s_mLastOrder.Count() > 300)
		{
			s_mLastOrder.Clear();
			s_mLastOrderAt.Clear();
			s_mLastOrderPos.Clear();
			s_mLastOrderKind.Clear();
		}
		string id = DCO_Log.IdGroup(grp);
		float now = DCO_Util.NowS();
		string cur = string.Format("%1@%2/%3", order, DCO_Util.XZ(pos), tactic);
		string last;
		float lastAt;
		vector lastPos;
		string lastKind;
		bool same = s_mLastOrderPos.Find(id, lastPos) && s_mLastOrderKind.Find(id, lastKind) && lastKind == order && vector.DistanceXZ(lastPos, pos) <= 30.0;
		if (!same && lastKind == "move" && order == "transport")
		{
			same = true;
			DCO_Perf.Count("cmd:order_move_to_transport");
		}
		if (same && s_mLastOrderAt.Find(id, lastAt) && now - lastAt <= 2.0)
			DCO_Perf.Count("cmd:order_reissue_same_spot");
		if (!same && s_mLastOrder.Find(id, last) && s_mLastOrderAt.Find(id, lastAt) && now - lastAt <= 2.0 && last != cur)
		{
			DCO_Perf.Count("cmd:order_overwrite");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("grp_order_overwrite grp=%1 old=%2 new=%3 dt_ms=%4", id, last, cur, Math.Round((now - lastAt) * 1000.0)));
		}
		s_mLastOrder.Set(id, cur);
		s_mLastOrderAt.Set(id, now);
		s_mLastOrderPos.Set(id, pos);
		s_mLastOrderKind.Set(id, order);
	}

	static void OnWaypoint(SCR_AIGroup grp, AIWaypoint wp)
	{
		if (!grp || !wp || !Replication.IsServer())
			return;
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!gu || !gu.GetMyCommander() || gu.IsPlayerGroup())
			return;
		string order = "move";
		DCO_EGroupTask task = gu.GetTask();
		if (DCO_GarrisonWaypoint.Cast(wp))
			order = "garrison";
		else if (SCR_DefendWaypoint.Cast(wp))
			order = "defend";
		else if (SCR_SuppressWaypoint.Cast(wp))
			order = "suppress";
		else if (SCR_BoardingWaypoint.Cast(wp) || SCR_BoardingEntityWaypoint.Cast(wp) || wp.ClassName().Contains("GetOut") || wp.ClassName().Contains("GetIn"))
			order = "transport";
		else if (gu.IsRetreating())
			order = "withdraw";
		else if (task == DCO_EGroupTask.ATTACK)
			order = "attack";
		else if (task == DCO_EGroupTask.FLANK)
			order = "flank";
		else if (task == DCO_EGroupTask.RECON)
			order = "recon";
		else if (task == DCO_EGroupTask.DEFEND || task == DCO_EGroupTask.GARRISON)
			order = "defend";
		string obj = "-";
		if (gu.GetGroupObjective())
			obj = gu.GetGroupObjective().GetLogName();
		string role = "-";
		array<AIWaypoint> wps = {};
		grp.GetWaypoints(wps);
		int seq = Math.Max(wps.Count() - 1, 0);
		string why = typename.EnumToString(DCO_EGroupTask, task) + "/" + typename.EnumToString(DCO_ETaskPhase, gu.GetPhase());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("cmd_order cmd=%1 grp=%2 order=%3 wp=%4 pos=%5 obj=%6 tactic=%7 role=%8 why=%9",
			gu.GetMyCommander().GetCommanderUID(), DCO_Log.IdGroup(grp), order, wp.ClassName(), DCO_Util.XZ(wp.GetOrigin()), obj, TacticOf(gu), role, why)
			+ string.Format(" seq=%1 d_grp=%2", seq, Math.Round(vector.DistanceXZ(grp.GetOrigin(), wp.GetOrigin()))));
		NoteOrder(grp, order, wp.GetOrigin(), seq, TacticOf(gu));
	}

	static void Place(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, AIGroup grp, string kind, vector pos, float facingDeg, string building, string slot, string sector, string defense)
	{
		if (!Replication.IsServer())
			return;
		string c = "-";
		if (cmd)
			c = cmd.GetCommanderUID();
		string o = "-";
		if (obj)
			o = obj.GetLogName();
		string g = "-";
		if (grp)
			g = DCO_Log.IdGroup(grp);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_place cmd=%1 obj=%2 grp=%3 kind=%4 pos=%5 facing=%6 building=%7 slot=%8 sector=%9", c, o, g, kind, DCO_Util.XZ(pos), Math.Round(facingDeg), building, slot, sector)
			+ " defense=" + defense);
	}

	static void Capture(string text)
	{
		if (!s_mRec || s_bEnded)
			return;
		int sp = text.IndexOf(" ");
		if (sp <= 0)
			return;
		string kind = text.Substring(0, sp);
		switch (kind)
		{
			case "cmd_order":
			case "def_place":
			case "death_ctx":
			case "tactic_result":
			case "cmd_think":
			case "cmd_intel":
			case "unit_summary":
			case "grp_summary":
			case "cmd_summary":
			case "learn_def_episode":
				break;
			case "tactic_chosen":
				NoteTactic(text);
				return;
			case "tr_end":
				NoteTR(text);
				return;
			default:
				return;
		}
		if (kind == "tactic_result")
			NoteResult(text);
		array<string> rows = s_mRec.Get(kind);
		if (!rows)
		{
			rows = {};
			s_mRec.Set(kind, rows);
		}
		if (rows.Count() < 20000)
			rows.Insert(ToJson(text));
	}

	protected static string Val(string text, string key)
	{
		int i = text.IndexOf(" " + key + "=");
		if (i < 0)
			return "";
		int st = i + key.Length() + 2;
		int en = text.IndexOfFrom(st, " ");
		if (en < 0)
			en = text.Length();
		return text.Substring(st, en - st);
	}

	protected static void NoteTactic(string text)
	{
		string cmd = Val(text, "cmd");
		string obj = Val(text, "obj");
		if (!obj.IsEmpty() && !cmd.IsEmpty())
			s_mObjCmd.Set(obj, cmd);
	}

	protected static void NoteResult(string text)
	{
		string obj = Val(text, "obj");
		string cmd = s_mObjCmd.Get(obj);
		string entry = Val(text, "tactic") + ":" + Val(text, "result") + "@" + obj;
		if (cmd.IsEmpty())
			cmd = "-";
		string prev = s_mCmdTactics.Get(cmd);
		if (prev.Length() < 800)
			s_mCmdTactics.Set(cmd, Join(prev, entry));
		if (Val(text, "result") == "captured")
		{
			foreach (AIGroup g, DCO_BattleGroup bg : s_mGroups)
			{
				if (!g || !bg)
					continue;
				DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(g.FindComponent(DCO_GroupUtilityComponent));
				if (gu && gu.GetGroupObjective() && gu.GetGroupObjective().GetLogName() == obj)
					bg.m_iObjTaken++;
			}
		}
	}

	protected static void NoteTR(string text)
	{
		string name = Val(text, "grp");
		string role = Val(text, "role");
		string why = Val(text, "why");
		foreach (AIGroup g, DCO_BattleGroup bg : s_mGroups)
		{
			if (bg && bg.m_sName == name)
			{
				bg.m_sTR = role + ":" + why;
				return;
			}
		}
	}

	protected static bool IsNumber(string v)
	{
		if (v.IsEmpty() || v == "-" || v == ".")
			return false;
		for (int i = 0; i < v.Length(); i++)
		{
			string ch = v.Get(i);
			string digits = "0123456789.-";
			if (digits.IndexOf(ch) < 0)
				return false;
			if (ch == "-" && i > 0)
				return false;
		}
		return true;
	}

	protected static string Esc(string v)
	{
		string r = v;
		r.Replace("\\", "\\\\");
		r.Replace("\"", "\\\"");
		return r;
	}

	protected static string ToJson(string text)
	{
		array<string> parts = {};
		text.Split(" ", parts, true);
		string j = "{\"t\":" + Math.Round(DCO_Util.NowS() - s_fStart).ToString() + ",\"event\":\"" + Esc(parts[0]) + "\"";
		for (int i = 1; i < parts.Count(); i++)
		{
			string p = parts[i];
			int eq = p.IndexOf("=");
			if (eq <= 0)
				continue;
			string k = p.Substring(0, eq);
			string v = p.Substring(eq + 1, p.Length() - eq - 1);
			if (IsNumber(v))
				j += ",\"" + Esc(k) + "\":" + v;
			else
				j += ",\"" + Esc(k) + "\":\"" + Esc(v) + "\"";
		}
		return j + "}";
	}

	static void Wounded(ChimeraCharacter ch, IEntity attacker, string state, string dmg, string src)
	{
		if (!ch || !Replication.IsServer())
			return;
		Init();
		OnWounded(ch);
		AIGroup g = GroupOf(ch);
		string grp = "-";
		if (g)
			grp = DCO_Log.IdGroup(g);
		SCR_AIUtilityComponent u = UtilOf(ch);
		string beh = "-";
		if (u && u.GetCurrentAction())
			beh = u.GetCurrentAction().ClassName();
		bool indoor = SCR_CoverManagerComponent.IsEntityInsideBuilding(ch);
		string att = "-";
		float dist = -1;
		if (attacker && attacker != ch)
		{
			att = DCO_Log.IdUnit(attacker);
			dist = vector.Distance(attacker.GetOrigin(), ch.GetOrigin());
		}
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("unit_wounded unit=%1 grp=%2 state=%3 dmg=%4 src=%5 attacker=%6 dist=%7 vbeh=%8 vstance=%9", DCO_Log.IdUnit(ch), grp, state, dmg, src, att, Math.Round(dist), beh, Stance(ch))
			+ string.Format(" vcover=%1 vmoving=%2", CoverKind(ch, attacker, u, indoor), DCO_Util.B01(Speed(ch) > 0.5))
			+ SmartActionInfo(u));
	}

	protected static string SmartActionInfo(SCR_AIUtilityComponent u)
	{
		if (!u)
			return " vsa=- vsa_pos=-";
		SCR_AIPerformActionBehavior pa = SCR_AIPerformActionBehavior.Cast(u.GetCurrentAction());
		if (!pa || !pa.m_SmartActionComponent || !pa.m_SmartActionComponent.m_Value)
			return " vsa=- vsa_pos=-";
		SCR_AISmartActionComponent sa = pa.m_SmartActionComponent.m_Value;
		IEntity owner = sa.GetOwner();
		if (!owner)
			return " vsa=no_owner vsa_pos=-";
		string tags = "-";
		AISmartActionComponent asa = AISmartActionComponent.Cast(sa);
		if (asa)
		{
			array<string> tagList = {};
			asa.GetTags(tagList);
			if (!tagList.IsEmpty())
			{
				tags = tagList[0];
				for (int ti = 1; ti < tagList.Count(); ti++)
					tags += "|" + tagList[ti];
			}
		}
		string parent = "-";
		if (owner.GetParent())
			parent = owner.GetParent().ClassName() + ":" + Prefab(owner.GetParent());
		return string.Format(" vsa=%1 vsa_pos=%2 vsa_cls=%3/%4 vsa_tags=%5 vsa_parent=%6", Prefab(owner), DCO_Util.XZ(owner.GetOrigin()), owner.ClassName(), sa.ClassName(), tags, parent);
	}

	protected static string DCO_UnitState(IEntity e, DCO_BattleUnit u)
	{
		if (u.m_fDied >= 0)
			return "dead";
		if (!e)
			return "gone";
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch || !ch.GetCharacterController())
			return "gone";
		ECharacterLifeState ls = ch.GetCharacterController().GetLifeState();
		if (ls == ECharacterLifeState.ALIVE)
			return "alive";
		if (ls == ECharacterLifeState.INCAPACITATED)
			return "uncon";
		return "dead_unlogged";
	}

	static void End(string why)
	{
		if (!Replication.IsServer() || !s_mRec || s_bEnded)
			return;
		Sample();
		float now = DCO_Util.NowS();
		foreach (IEntity e, DCO_BattleUnit u : s_mUnits)
		{
			if (!u)
				continue;
			bool alive = u.m_fDied < 0 && e && DCO_Util.IsAlive(e);
			float end = now;
			if (u.m_fDied >= 0)
				end = u.m_fDied;
			float samples = Math.Max(u.m_iSamples, 1);
			int rounds = 0;
			if (e)
				rounds = DCO_ShotCounter.Total(e);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("unit_summary unit=%1 grp=%2 fac=%3 roles=%4 alive=%5 life_s=%6 rounds=%7 hits=%8 kills=%9", u.m_sId, u.m_sGrp, u.m_sFac, u.m_sRoles, DCO_Util.B01(alive), Math.Round(end - u.m_fBorn), rounds, u.m_iHits, u.m_iKills)
				+ string.Format(" ff_kills=%1 wounded=%2 healed=%3 dist_m=%4 cover_pct=%5 move_pct=%6 prone_pct=%7 cause=%8", u.m_iFF, u.m_iWounded, u.m_iHealed, Math.Round(u.m_fDist), Math.Round(100.0 * u.m_iCover / samples), Math.Round(100.0 * u.m_iMove / samples), Math.Round(100.0 * u.m_iProne / samples), u.m_sCause)
				+ " state=" + DCO_UnitState(e, u));
		}
		map<string, int> facLoss = new map<string, int>();
		map<string, int> facKill = new map<string, int>();
		map<string, int> facGrpLost = new map<string, int>();
		foreach (AIGroup g, DCO_BattleGroup bg : s_mGroups)
		{
			if (!bg)
				continue;
			int alive = 0;
			if (g)
				alive = g.GetAgentsCount();
			string tasks = bg.m_sTasks;
			if (tasks.IsEmpty())
				tasks = "-";
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("grp_summary grp=%1 name=%2 fac=%3 start_units=%4 alive=%5 lost=%6 tasks=%7 tr=%8 kills=%9", bg.m_sId, bg.m_sName, bg.m_sFac, bg.m_iStart, alive, bg.m_iLost, tasks, bg.m_sTR, bg.m_iKills)
				+ string.Format(" losses=%1 objectives_taken=%2", bg.m_iLost, bg.m_iObjTaken));
			facLoss.Set(bg.m_sFac, facLoss.Get(bg.m_sFac) + bg.m_iLost);
			facKill.Set(bg.m_sFac, facKill.Get(bg.m_sFac) + bg.m_iKills);
			if (alive == 0)
				facGrpLost.Set(bg.m_sFac, facGrpLost.Get(bg.m_sFac) + 1);
		}
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		string objEnd;
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (o && o.GetOwner())
					objEnd = Join(objEnd, o.GetLogName() + ":" + OwnerOf(o));
			}
			foreach (AICommander_BaseComponent c : mgr.m_aCommander)
			{
				if (!c)
					continue;
				string fk = c.GetCommanderFactionKey();
				string tactics = s_mCmdTactics.Get(c.GetCommanderUID());
				if (tactics.IsEmpty())
					tactics = "-";
				if (objEnd.IsEmpty())
					objEnd = "-";
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(c), string.Format("cmd_summary cmd=%1 fac=%2 mode=%3 tactics=%4 objectives=%5 losses=%6 kills=%7 groups_lost=%8",
					c.GetCommanderUID(), fk, typename.EnumToString(CMD_ECommanderMode, c.GetCommanderMode()), tactics, objEnd, facLoss.Get(fk), facKill.Get(fk), facGrpLost.Get(fk)));
			}
		}
		WriteJson(why, now, mgr);
		s_bEnded = true;
		GetGame().GetCallqueue().Remove(Sample);
	}

	protected static void WriteJson(string why, float now, AICommander_ManagerComponent mgr)
	{
		string dir = DCO_Log.GetSessionDir();
		if (dir.IsEmpty())
			return;
		string path = dir + "/battle.json";
		FileHandle f = FileIO.OpenFile(path, FileMode.WRITE);
		if (!f)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "battle_export_fail path=" + path);
			return;
		}
		int bytes = 0;
		string head = "{\"world\":\"" + Esc(GetGame().GetWorldFile()) + "\",\"why\":\"" + why + "\",\"duration_s\":" + Math.Round(now - s_fStart).ToString() + ",";
		f.WriteLine(head);
		bytes += head.Length();
		string cmds = "\"commanders\":[";
		string objs = "\"objectives\":[";
		if (mgr)
		{
			bool first = true;
			foreach (AICommander_BaseComponent c : mgr.m_aCommander)
			{
				if (!c)
					continue;
				string doctrine = "-";
				if (c.GetDoctrine())
					doctrine = c.GetDoctrine().ClassName();
				if (!first)
					cmds += ",";
				first = false;
				cmds += "{\"uid\":\"" + Esc(c.GetCommanderUID()) + "\",\"faction\":\"" + c.GetCommanderFactionKey() + "\",\"mode\":\"" + typename.EnumToString(CMD_ECommanderMode, c.GetCommanderMode()) + "\",\"doctrine\":\"" + Esc(doctrine) + "\",\"aggression\":" + c.GetAggression().ToString(-1, 2) + ",\"risk\":" + c.GetRiskTaking().ToString(-1, 2) + ",\"patience\":" + c.GetPatience().ToString(-1, 2) + "}";
			}
			bool firstO = true;
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (!o || !o.GetOwner())
					continue;
				if (!firstO)
					objs += ",";
				firstO = false;
				vector op = o.GetOwner().GetOrigin();
				string start = s_mObjStart.Get(o.GetLogName());
				if (start.IsEmpty())
					start = "-";
				objs += "{\"name\":\"" + Esc(o.GetLogName()) + "\",\"x\":" + Math.Round(op[0]).ToString() + ",\"z\":" + Math.Round(op[2]).ToString() + ",\"radius\":" + Math.Round(o.GetRadius()).ToString() + ",\"owner_start\":\"" + start + "\",\"owner_end\":\"" + OwnerOf(o) + "\"}";
			}
		}
		cmds += "],";
		objs += "]";
		f.WriteLine(cmds);
		f.WriteLine(objs);
		bytes += cmds.Length() + objs.Length();
		string counts;
		foreach (string kind, array<string> rows : s_mRec)
		{
			if (!rows)
				continue;
			f.WriteLine(",\"" + kind + "\":[");
			foreach (int i, string r : rows)
			{
				string line = r;
				if (i < rows.Count() - 1)
					line += ",";
				f.WriteLine(line);
				bytes += line.Length();
			}
			f.WriteLine("]");
			counts = Join(counts, kind + "=" + rows.Count().ToString());
		}
		f.WriteLine("}");
		f.Close();
		if (counts.IsEmpty())
			counts = "-";
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("battle_export path=%1 bytes=%2 counts=%3", path, bytes, counts));
	}
}
