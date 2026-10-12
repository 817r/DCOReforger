class DCO_Hearing
{
	protected static ref array<IEntity> s_aVehicles;
	protected static ref array<IEntity> s_aSnapEnt;
	protected static ref array<vector> s_aSnapPos;
	protected static ref array<float> s_aSnapRange;
	protected static ref array<Faction> s_aSnapFaction;
	protected static ref array<int> s_aSnapType;
	protected static ref array<vector> s_aDoorPos;
	protected static ref array<float> s_aDoorTime;
	protected static ref array<Faction> s_aDoorFaction;
	protected static float s_fSnapTime_ms = -1;
	protected static float s_fBudgetFrame_ms = -1;
	protected static int s_iBudgetUsed;
	protected static ref array<IEntity> s_aVoiceEnt;
	protected static ref array<float> s_aVoiceLast;
	protected static ref array<float> s_aVoiceStart;
	protected static ref array<bool> s_aVoiceRadio;
	protected static ref array<float> s_aVoiceTotal;
	protected static float s_fEnvAt_ms = -1;
	protected static float s_fEnv = 1;

	static void RegisterVehicle(IEntity veh)
	{
		if (!veh || !Replication.IsServer())
			return;
		if (!s_aVehicles)
			s_aVehicles = {};
		if (!s_aVehicles.Contains(veh))
			s_aVehicles.Insert(veh);
	}

	static void UnregisterVehicle(IEntity veh)
	{
		if (s_aVehicles)
			s_aVehicles.RemoveItem(veh);
	}

	static void OnDoorUsed(IEntity door, IEntity user)
	{
		if (!door || !user || !Replication.IsServer() || !DCO_GlobalAIComponent.BehaviorOn(DCO_EBehavior.HEARING))
			return;
		Faction f = FactionOf(user);
		if (!f)
			return;
		if (!s_aDoorPos)
		{
			s_aDoorPos = {};
			s_aDoorTime = {};
			s_aDoorFaction = {};
		}
		if (s_aDoorPos.Count() >= 32)
		{
			s_aDoorPos.Remove(0);
			s_aDoorTime.Remove(0);
			s_aDoorFaction.Remove(0);
		}
		s_aDoorPos.Insert(door.GetOrigin());
		s_aDoorTime.Insert(GetGame().GetWorld().GetWorldTime());
		s_aDoorFaction.Insert(f);
	}

	static void OnVoice(IEntity speaker, bool radio)
	{
		if (!speaker || !Replication.IsServer() || !DCO_GlobalAIComponent.BehaviorOn(DCO_EBehavior.PLAYER_VOICE) || !DCO_Util.IsNotDead(speaker))
			return;
		if (!s_aVoiceEnt)
		{
			s_aVoiceEnt = {};
			s_aVoiceLast = {};
			s_aVoiceStart = {};
			s_aVoiceRadio = {};
			s_aVoiceTotal = {};
		}
		float now = GetGame().GetWorld().GetWorldTime();
		int idx = s_aVoiceEnt.Find(speaker);
		if (idx < 0)
		{
			if (s_aVoiceEnt.Count() >= 32)
				return;
			s_aVoiceEnt.Insert(speaker);
			s_aVoiceLast.Insert(now);
			s_aVoiceStart.Insert(now);
			s_aVoiceRadio.Insert(radio);
			s_aVoiceTotal.Insert(0);
			return;
		}
		float gap = now - s_aVoiceLast[idx];
		if (gap > 30000.0)
			s_aVoiceTotal[idx] = 0;
		else if (gap <= 3000.0)
			s_aVoiceTotal[idx] = Math.Min(s_aVoiceTotal[idx] + gap * 0.001, 30.0);
		if (gap > 3000.0)
			s_aVoiceStart[idx] = now;
		s_aVoiceLast[idx] = now;
		s_aVoiceRadio[idx] = radio;
	}

	protected static bool EngineNear(vector pos)
	{
		if (!s_aVehicles)
			return false;
		foreach (IEntity veh : s_aVehicles)
		{
			if (!veh || vector.DistanceSq(veh.GetOrigin(), pos) > 900.0)
				continue;
			BaseVehicleControllerComponent vc = BaseVehicleControllerComponent.Cast(veh.FindComponent(BaseVehicleControllerComponent));
			if (vc && vc.IsEngineOn())
				return true;
		}
		return false;
	}

	protected static float EnvFactor(bool safe)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (s_fEnvAt_ms < 0 || now - s_fEnvAt_ms > 10000.0 || now < s_fEnvAt_ms)
		{
			s_fEnvAt_ms = now;
			s_fEnv = 1;
			ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
			TimeAndWeatherManagerEntity manager;
			if (world)
				manager = world.GetTimeAndWeatherManager();
			if (manager && (manager.GetRainIntensity() > 0.3 || manager.GetWindSpeed() > 8.0))
				s_fEnv = 0.7;
		}
		float env = s_fEnv;
		if (safe && env >= 1 && DCO_Util.IsNight())
			env = 1.3;
		return env;
	}

	static bool ListenVoice(SCR_AIGroup grp, bool contactMode, out IEntity outSource, out vector outPos, out Faction outFaction, out bool outBearingOnly, out float outNoise, out IEntity outListener)
	{
		if (!s_aVoiceEnt || s_aVoiceEnt.IsEmpty())
			return false;
		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			return false;
		Faction mine = FactionOf(leader);
		if (!mine)
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		array<AIAgent> members = {};
		grp.GetAgents(members);

		for (int i = s_aVoiceEnt.Count() - 1; i >= 0; i--)
		{
			IEntity spk = s_aVoiceEnt[i];
			if (!spk || now - s_aVoiceLast[i] > 30000.0 || !DCO_Util.IsNotDead(spk))
			{
				s_aVoiceEnt.Remove(i);
				s_aVoiceLast.Remove(i);
				s_aVoiceStart.Remove(i);
				s_aVoiceRadio.Remove(i);
				s_aVoiceTotal.Remove(i);
				continue;
			}
			if (now - s_aVoiceLast[i] > 2500.0)
				continue;
			Faction f = FactionOf(spk);
			if (!f || !mine.IsFactionEnemy(f))
				continue;

			vector sp = spk.GetOrigin();
			float loud = 1.0;
			if (s_aVoiceRadio[i])
				loud = 0.7;

			IEntity best;
			AIAgent bestAgent;
			float bestD = float.MAX;
			foreach (AIAgent m : members)
			{
				IEntity me = m.GetControlledEntity();
				if (!me || !DCO_Util.IsAlive(me))
					continue;
				float d = vector.Distance(me.GetOrigin(), sp);
				if (d < bestD)
				{
					bestD = d;
					best = me;
					bestAgent = m;
				}
			}
			if (!best)
				continue;

			SCR_ChimeraAIAgent ba = SCR_ChimeraAIAgent.Cast(bestAgent);
			bool safe = !(ba && ba.m_UtilityComponent && ba.m_UtilityComponent.m_ThreatSystem && ba.m_UtilityComponent.m_ThreatSystem.GetState() != EAIThreatState.SAFE);

			float deaf = DCO_Explosion.DeafFactor(best);
			float env = EnvFactor(safe);
			float far = 25.0 * loud * deaf * env;
			float sure = 8.0 * loud * deaf * env;
			if (contactMode)
				far = Math.Min(far, 15.0);
			if (bestD > far)
				continue;

			DCO_Perf.Count("hear:voice_try");
			float p = 1.0;
			if (bestD > sure)
				p = 1.0 - (bestD - sure) / Math.Max(far - sure, 0.1);

			IEntity bldSpk = SCR_CoverManagerComponent.DCO_GetBuildingAt(spk);
			IEntity bldMe = SCR_CoverManagerComponent.DCO_GetBuildingAt(best);
			bool sameBuilding = bldSpk && bldSpk == bldMe;
			if (sameBuilding && Math.AbsFloat(sp[1] - best.GetOrigin()[1]) < 2.5)
				p = 1.0;
			else
			{
				if (sameBuilding)
					p *= 0.6;
				else if (DCO_Util.TraceFrac(best.GetOrigin() + Vector(0, 1.6, 0), sp + Vector(0, 1.6, 0), best, TraceFlags.WORLD, -1, "t:DCO_HearVoice") < 0.99)
					p *= 0.5;
				ChimeraCharacter bc = ChimeraCharacter.Cast(best);
				if (bc && bc.IsInVehicle())
					p *= 0.3;
				else if (EngineNear(best.GetOrigin()))
					p *= 0.4;
				if (!safe && !contactMode)
					p *= 0.3;
				float talk = (now - s_aVoiceStart[i]) * 0.001;
				p *= Math.Clamp(0.35 + talk * 0.25, 0.35, 1.0);
			}

			if (Math.RandomFloat01() >= p)
				continue;

			float total = s_aVoiceTotal[i];
			outNoise = Math.Lerp(8.0, 2.0, Math.Clamp(total / 10.0, 0, 1));
			outBearingOnly = !sameBuilding && (total < 2.0 || bestD > far * 0.7);
			vector lp = best.GetOrigin();
			if (outBearingOnly)
			{
				vector dir = sp - lp;
				dir[1] = 0;
				float yaw = Math.Atan2(dir[0], dir[2]) + Math.RandomFloat(-25, 25) * Math.DEG2RAD;
				float band = bestD * Math.RandomFloat(0.7, 1.3);
				outPos = DCO_Util.Surface(lp + Vector(Math.Sin(yaw), 0, Math.Cos(yaw)) * band);
				outNoise = Math.Max(bestD * 0.45, 10.0);
				DCO_Perf.Count("voice:bearing_only");
			}
			else
				outPos = DCO_Util.Surface(sp + Vector(Math.RandomFloat(-outNoise, outNoise), 0, Math.RandomFloat(-outNoise, outNoise)));

			outSource = spk;
			outFaction = f;
			outListener = best;
			DCO_Perf.Count("hear:voice");
			return true;
		}
		return false;
	}

	static bool TakeBudget()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now != s_fBudgetFrame_ms)
		{
			s_fBudgetFrame_ms = now;
			s_iBudgetUsed = 0;
		}
		if (s_iBudgetUsed >= 4)
			return false;
		s_iBudgetUsed++;
		return true;
	}

	protected static Faction FactionOf(IEntity ent)
	{
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!fac)
			return null;
		return fac.GetAffiliatedFaction();
	}

	static bool V2()
	{
		return DCO_Ctx.TuneG("hear.v2", 0.0) > 0;
	}

	static string TypeName(int t)
	{
		switch (t)
		{
			case 0: return "step";
			case 1: return "engine";
			case 2: return "reload";
			case 3: return "door";
			case 4: return "voice";
		}
		return "other";
	}

	static Faction Fac(IEntity ent)
	{
		return FactionOf(ent);
	}

	protected static void AddSnap(IEntity ent, vector pos, float range, Faction f, int type = 0)
	{
		s_aSnapEnt.Insert(ent);
		s_aSnapPos.Insert(pos);
		s_aSnapRange.Insert(range);
		s_aSnapFaction.Insert(f);
		s_aSnapType.Insert(type);
	}

	protected static void AddCharacterV2(ChimeraCharacter ch, CharacterControllerComponent ctrl)
	{
		float speed = 0;
		if (ch.GetPhysics())
			speed = ch.GetPhysics().GetVelocity().Length();
		float range = 0;
		int type = 0;
		if (speed > 5.0)
			range = 40.0;
		else if (speed > 3.0)
			range = 30.0;
		else if (speed > 0.8)
			range = 12.0;
		ECharacterStance stc = ctrl.GetStance();
		if (stc == ECharacterStance.CROUCH)
			range = range * 0.5;
		else if (stc == ECharacterStance.PRONE)
			range = range * 0.3;
		if (range > 0)
		{
			int env = DCO_Ctx.CellEnv(ch.GetOrigin(), false);
			if (env == DCO_EEnv.URBAN || env == DCO_EEnv.INDOOR)
				range = range * 1.2;
			else if (env == DCO_EEnv.FOREST)
				range = range * 0.9;
		}
		SCR_CharacterControllerComponent sc = SCR_CharacterControllerComponent.Cast(ctrl);
		if (sc && sc.IsReloading() && range < 8.0)
		{
			range = 8.0;
			type = 2;
		}
		if (range <= 0)
			return;
		Faction f = FactionOf(ch);
		if (f)
			AddSnap(ch, ch.GetOrigin(), range, f, type);
	}

	static float Mask(SCR_AIUtilityComponent u, IEntity me, out string why)
	{
		why = "-";
		bool safe = !(u && u.m_ThreatSystem && u.m_ThreatSystem.GetState() != EAIThreatState.SAFE);
		float m = EnvFactor(safe) * DCO_Explosion.DeafFactor(me);
		if (DCO_Ctx.TuneForU(u, "hear.mask", 0.0) <= 0 || !u)
			return m;
		float now = GetGame().GetWorld().GetWorldTime();
		float since = now - u.m_fDCOGunfireAt;
		if (since < 3000.0)
		{
			m = m * 0.4;
			why = "gunfire";
		}
		else if (since < 10000.0)
		{
			m = m * 0.7;
			why = "gunfire_recent";
		}
		if (EngineNear(me.GetOrigin()))
		{
			m = m * 0.5;
			why = why + "+engine";
		}
		return m;
	}

	static int SnapCount()
	{
		if (!s_aSnapEnt)
			return 0;
		return s_aSnapEnt.Count();
	}

	static bool ListenAt(IEntity me, Faction mine, float mask, out IEntity outSource, out vector outPos, out Faction outFaction, out int outType, out float outDist, out float outRange, out float outGap)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		RefreshSnapshot(now);
		vector lp = me.GetOrigin();
		float best = 1.0;
		bool found;
		outGap = 100000.0;
		foreach (int i, IEntity ent : s_aSnapEnt)
		{
			if (!ent || ent == me || !mine.IsFactionEnemy(s_aSnapFaction[i]))
				continue;
			float r = s_aSnapRange[i] * mask;
			float d = vector.DistanceXZ(lp, s_aSnapPos[i]);
			outGap = Math.Min(outGap, d - r);
			if (d > r || r <= 0)
				continue;
			float score = d / r;
			if (score >= best)
				continue;
			best = score;
			found = true;
			outSource = ent;
			outFaction = s_aSnapFaction[i];
			outType = s_aSnapType[i];
			outDist = d;
			outRange = r;
			float jit = 2.0 + 0.1 * d;
			if (outType == 1)
				jit = 40.0;
			else if (outType == 2)
				jit = 3.0;
			outPos = s_aSnapPos[i] + Vector(Math.RandomFloat(-jit, jit), 0, Math.RandomFloat(-jit, jit));
		}
		if (!found && s_aDoorPos)
		{
			for (int dd = s_aDoorPos.Count() - 1; dd >= 0; dd--)
			{
				if (now - s_aDoorTime[dd] > 5000 || !mine.IsFactionEnemy(s_aDoorFaction[dd]))
					continue;
				float doorR = 20.0 * mask;
				float dd2 = vector.DistanceXZ(lp, s_aDoorPos[dd]);
				if (dd2 > doorR)
					continue;
				outSource = null;
				outPos = s_aDoorPos[dd];
				outFaction = s_aDoorFaction[dd];
				outType = 3;
				outDist = dd2;
				outRange = doorR;
				found = true;
				break;
			}
		}
		if (found)
			outPos = DCO_Util.Surface(outPos);
		return found;
	}

	protected static void AddCharacter(IEntity ent)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch || ch.IsInVehicle() || !DCO_Util.IsAlive(ch))
			return;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (V2())
		{
			AddCharacterV2(ch, ctrl);
			return;
		}
		if (ctrl.GetStance() != ECharacterStance.STAND)
			return;
		float speed = 0;
		if (ch.GetPhysics())
			speed = ch.GetPhysics().GetVelocity().Length();
		float range = 0;
		if (speed > 3.0)
			range = 30.0;
		else if (speed > 0.8)
			range = 12.0;
		if (range <= 0)
			return;
		Faction f = FactionOf(ch);
		if (f)
			AddSnap(ch, ch.GetOrigin(), range, f);
	}

	protected static void RefreshSnapshot(float now)
	{
		if (s_fSnapTime_ms >= 0 && now - s_fSnapTime_ms < 1000 && now >= s_fSnapTime_ms)
			return;
		s_fSnapTime_ms = now;
		if (!s_aSnapEnt)
		{
			s_aSnapEnt = {};
			s_aSnapPos = {};
			s_aSnapRange = {};
			s_aSnapFaction = {};
			s_aSnapType = {};
		}
		s_aSnapEnt.Clear();
		s_aSnapPos.Clear();
		s_aSnapRange.Clear();
		s_aSnapFaction.Clear();
		s_aSnapType.Clear();

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld)
		{
			array<AIAgent> agents = {};
			aiWorld.GetAIAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (SCR_ChimeraAIAgent.Cast(a))
					AddCharacter(a.GetControlledEntity());
			}
		}

		array<int> pids = {};
		GetGame().GetPlayerManager().GetPlayers(pids);
		foreach (int pid : pids)
			AddCharacter(GetGame().GetPlayerManager().GetPlayerControlledEntity(pid));

		if (!s_aVehicles)
			return;
		for (int i = s_aVehicles.Count() - 1; i >= 0; i--)
		{
			IEntity veh = s_aVehicles[i];
			if (!veh)
			{
				s_aVehicles.Remove(i);
				continue;
			}
			BaseVehicleControllerComponent vc = BaseVehicleControllerComponent.Cast(veh.FindComponent(BaseVehicleControllerComponent));
			if (!vc || !vc.IsEngineOn())
				continue;
			Faction vf = FactionOf(veh);
			if (!vf)
				continue;
			float range = 400.0;
			if (DCO_VehicleCombat.IsArmored(veh))
				range = 700.0;
			AddSnap(veh, veh.GetOrigin(), range, vf, 1);
		}
	}

	static bool Listen(SCR_AIGroup grp, out IEntity outSource, out vector outPos, out Faction outFaction)
	{
		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			return false;
		Faction mine = FactionOf(leader);
		if (!mine)
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		RefreshSnapshot(now);

		vector lp = leader.GetOrigin();
		float bestSq = float.MAX;
		float deaf = DCO_Explosion.DeafFactor(leader);
		foreach (int i, IEntity ent : s_aSnapEnt)
		{
			if (!ent || !mine.IsFactionEnemy(s_aSnapFaction[i]))
				continue;
			float dSq = vector.DistanceSqXZ(lp, s_aSnapPos[i]);
			float r = s_aSnapRange[i] * deaf;
			if (dSq > r * r || dSq >= bestSq)
				continue;
			bestSq = dSq;
			outSource = ent;
			outFaction = s_aSnapFaction[i];
			outPos = s_aSnapPos[i];
			if (r > 100)
				outPos = outPos + Vector(Math.RandomFloat(-40, 40), 0, Math.RandomFloat(-40, 40));
			else
				outPos = outPos + Vector(Math.RandomFloat(-4, 4), 0, Math.RandomFloat(-4, 4));
		}

		if (!outSource && s_aDoorPos)
		{
			array<AIAgent> members = {};
			grp.GetAgents(members);
			for (int d = s_aDoorPos.Count() - 1; d >= 0; d--)
			{
				if (now - s_aDoorTime[d] > 5000 || !mine.IsFactionEnemy(s_aDoorFaction[d]))
					continue;
				foreach (AIAgent m : members)
				{
					IEntity me = m.GetControlledEntity();
					if (me && vector.DistanceSq(me.GetOrigin(), s_aDoorPos[d]) < 225)
					{
						outPos = s_aDoorPos[d];
						outFaction = s_aDoorFaction[d];
						return true;
					}
				}
			}
		}

		if (outSource)
			outPos = DCO_Util.Surface(outPos);
		return outSource != null;
	}
}

modded class VehicleControllerComponent
{
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		DCO_Hearing.RegisterVehicle(owner);
	}

	override protected void OnDelete(IEntity owner)
	{
		DCO_Hearing.UnregisterVehicle(owner);
		super.OnDelete(owner);
	}
}

modded class SCR_DoorUserAction
{
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);
		DCO_Hearing.OnDoorUsed(pOwnerEntity, pUserEntity);
	}
}

modded class SCR_VoNComponent
{
	protected float m_fDCOVoiceSent_ms = -10000;

	override protected event void OnCapture(BaseTransceiver transmitter)
	{
		super.OnCapture(transmitter);
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOVoiceSent_ms < 1500.0 && now >= m_fDCOVoiceSent_ms)
			return;
		m_fDCOVoiceSent_ms = now;
		SCR_PlayerControllerGroupComponent pc = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
		if (pc)
			pc.DCO_SendVoice(transmitter != null);
	}
}
