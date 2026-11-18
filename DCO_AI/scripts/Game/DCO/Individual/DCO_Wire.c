modded class SCR_AIUtilityComponent
{
	float m_fDCOWireNext_ms;
}

modded class SCR_SpecialCollisionHandlerComponent
{
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		DCO_Wire.Register(owner, this);
	}

	override void OnContactStart(notnull SCR_ChimeraCharacter character)
	{
		super.OnContactStart(character);
		DCO_Wire.Caught(character, GetOwner(), this);
	}
}

class DCO_Wire
{
	protected static ref array<IEntity> s_aWire;
	protected static ref array<vector> s_aCenter;
	protected static ref array<float> s_aRad;
	protected static ref map<IEntity, float> s_mBreachUntil;
	protected static ref map<IEntity, int> s_mBreachCount;

	static void Register(IEntity owner, SCR_SpecialCollisionHandlerComponent comp)
	{
		if (!owner || !comp || !Replication.IsServer())
			return;
		if (!s_aWire)
		{
			s_aWire = {};
			s_aCenter = {};
			s_aRad = {};
		}
		if (s_aWire.Contains(owner))
			return;
		SCR_SpecialCollisionHandlerComponentClass data = SCR_SpecialCollisionHandlerComponentClass.Cast(comp.GetComponentData(owner));
		if (!data)
			return;
		array<SCR_SpecialCollisionDamageEffect> fx = {};
		data.GetSpecialCollisionDamageEffects(fx);
		bool wire;
		foreach (SCR_SpecialCollisionDamageEffect e : fx)
		{
			if (SCR_PersistentBarbedWireDamageEffect.Cast(e))
				wire = true;
		}
		if (!wire)
			return;
		vector mins;
		vector maxs;
		owner.GetBounds(mins, maxs);
		vector mid = (mins + maxs) * 0.5;
		s_aWire.Insert(owner);
		s_aCenter.Insert(ToWorld(owner, mid));
		s_aRad.Insert(vector.DistanceXZ(mins, maxs) * 0.5);
		DCO_Perf.Count("wire:register");
	}

	protected static vector ToLocal(IEntity e, vector pos)
	{
		vector mat[4];
		e.GetWorldTransform(mat);
		vector d = pos - mat[3];
		return Vector(vector.Dot(d, mat[0]), vector.Dot(d, mat[1]), vector.Dot(d, mat[2]));
	}

	protected static vector ToWorld(IEntity e, vector local)
	{
		vector mat[4];
		e.GetWorldTransform(mat);
		return mat[3] + mat[0] * local[0] + mat[1] * local[1] + mat[2] * local[2];
	}

	protected static bool Gone(IEntity e)
	{
		if (!e)
			return true;
		DamageManagerComponent dm = DamageManagerComponent.Cast(e.FindComponent(DamageManagerComponent));
		return dm && dm.GetHealthScaled() <= 0;
	}

	protected static float HpOf(IEntity e)
	{
		if (!e)
			return 0;
		DamageManagerComponent dm = DamageManagerComponent.Cast(e.FindComponent(DamageManagerComponent));
		if (!dm)
			return -1;
		return dm.GetHealthScaled();
	}

	static IEntity Near(vector pos, float r)
	{
		if (!s_aWire)
			return null;
		for (int i = s_aWire.Count() - 1; i >= 0; i--)
		{
			IEntity e = s_aWire[i];
			if (!e)
			{
				s_aWire.Remove(i);
				s_aCenter.Remove(i);
				s_aRad.Remove(i);
				continue;
			}
			float reach = s_aRad[i] + r;
			if (vector.DistanceSqXZ(pos, s_aCenter[i]) > reach * reach)
				continue;
			vector mins;
			vector maxs;
			e.GetBounds(mins, maxs);
			vector l = ToLocal(e, pos);
			if (l[0] < mins[0] - r || l[0] > maxs[0] + r || l[2] < mins[2] - r || l[2] > maxs[2] + r)
				continue;
			if (l[1] < mins[1] - 1.5 || l[1] > maxs[1] + 2.0)
				continue;
			if (Gone(e))
				continue;
			return e;
		}
		return null;
	}

	static bool Avoid(SCR_AIUtilityComponent u, vector pos)
	{
		if (!s_aWire || s_aWire.IsEmpty())
			return false;
		if (DCO_Stress.T(u, "wire.on", 1.0) <= 0)
			return false;
		return Near(pos, 1.5) != null;
	}

	protected static vector ExitPos(IEntity wire, IEntity me, float clearM)
	{
		vector mins;
		vector maxs;
		wire.GetBounds(mins, maxs);
		vector mid = (mins + maxs) * 0.5;
		vector l = ToLocal(wire, me.GetOrigin());
		int thin = 0;
		int along = 2;
		if (maxs[2] - mins[2] < maxs[0] - mins[0])
		{
			thin = 2;
			along = 0;
		}
		float side = l[thin] - mid[thin];
		if (Math.AbsFloat(side) < 0.05 && me.GetPhysics())
		{
			vector ahead = ToLocal(wire, me.GetOrigin() + me.GetPhysics().GetVelocity());
			side = l[thin] - ahead[thin];
		}
		float sign = 1.0;
		if (side < 0)
			sign = -1.0;
		vector ex = l;
		ex[along] = Math.Clamp(l[along], mins[along], maxs[along]);
		ex[thin] = mid[thin] + sign * ((maxs[thin] - mins[thin]) * 0.5 + clearM);
		return DCO_Util.Surface(ToWorld(wire, ex));
	}

	protected static bool GroupAttacking(SCR_AIGroup grp)
	{
		if (!grp || !grp.GetGroupUtilityComponent() || !grp.GetGroupUtilityComponent().DCO_GetUtil())
			return false;
		int t = grp.GetGroupUtilityComponent().DCO_GetUtil().GetTask();
		return t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK;
	}

	protected static bool GroupHasFrag(SCR_AIGroup grp)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_CombatComponent && ca.m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
				return true;
		}
		return false;
	}

	static void Caught(SCR_ChimeraCharacter ch, IEntity wire, SCR_SpecialCollisionHandlerComponent comp)
	{
		if (!ch || !wire || !Replication.IsServer() || !GetGame().GetPlayerManager())
			return;
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(ch) != 0)
			return;
		Register(wire, comp);
		if (!s_aWire || !s_aWire.Contains(wire))
			return;
		SCR_AIUtilityComponent u = DCO_BldLearn.UtilOf(ch);
		if (!u || !u.GetAIAgent() || DCO_Stress.T(u, "wire.on", 1.0) <= 0)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < u.m_fDCOWireNext_ms)
			return;
		u.m_fDCOWireNext_ms = now + 8000.0;

		SCR_AIGroup grp = SCR_AIGroup.Cast(u.GetAIAgent().GetParentGroup());
		bool breach = grp && DCO_Stress.T(u, "wire.breach", 1.0) > 0 && GroupAttacking(grp) && GroupHasFrag(grp);
		float clearM = 3.0;
		if (breach)
			clearM = 16.0;
		vector exitPos = ExitPos(wire, ch, clearM);
		string result = "exit";
		DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(u, exitPos, wire.GetOrigin(), false, ECharacterStance.STAND, 20.0, "wire_exit");
		if (rb)
			rb.DCO_SetMove(ECharacterStance.STAND, EMovementType.WALK, 0, true);
		else
			result = "blocked";
		DCO_Perf.Count("wire:caught_" + result);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ch), string.Format("wire_caught unit=%1 wire=%2 pos=%3 exit=%4 out_m=%5 breach=%6 result=%7 hp=%8",
			DCO_Log.IdUnit(ch), DCO_Util.PrefabShort(wire), DCO_Util.XZ(ch.GetOrigin()), DCO_Util.XZ(exitPos), Math.Round(clearM), DCO_Util.B01(breach), result, HpOf(wire).ToString(-1, 2)));
		if (breach)
			BreachStart(wire, grp, now);
	}

	protected static void BreachStart(IEntity wire, SCR_AIGroup grp, float now)
	{
		if (!s_mBreachUntil)
		{
			s_mBreachUntil = new map<IEntity, float>();
			s_mBreachCount = new map<IEntity, int>();
		}
		float until;
		if (s_mBreachUntil.Find(wire, until) && now < until)
			return;
		if (s_mBreachUntil.Count() > 64)
		{
			s_mBreachUntil.Clear();
			s_mBreachCount.Clear();
		}
		s_mBreachUntil.Set(wire, now + 90000.0);
		GetGame().GetCallqueue().CallLater(BreachTick, 3000, false, wire, grp);
	}

	static void BreachTick(IEntity wire, SCR_AIGroup grp)
	{
		if (!wire || !grp || !s_mBreachUntil || Gone(wire))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		float until;
		if (!s_mBreachUntil.Find(wire, until) || now > until)
			return;
		int thrown;
		s_mBreachCount.Find(wire, thrown);
		if (thrown >= 2)
			return;
		vector mins;
		vector maxs;
		wire.GetBounds(mins, maxs);
		vector aim = DCO_Util.Surface(ToWorld(wire, (mins + maxs) * 0.5));
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_OwnerEntity || !ca.m_UtilityComponent.m_CombatComponent)
				continue;
			SCR_AIUtilityComponent u = ca.m_UtilityComponent;
			if (!DCO_Util.IsAlive(u.m_OwnerEntity) || !u.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
				continue;
			if (DCO_Stress.T(u, "wire.breach", 1.0) <= 0)
				return;
			float d = vector.DistanceXZ(u.m_OwnerEntity.GetOrigin(), aim);
			if (d < 15.0 || d > 35.0)
				continue;
			if (!DCO_GrenadeUtility.CanThrowGrenadeNow(u))
				continue;
			vector throwPos;
			if (!DCO_GrenadeUtility.ResolveThrowPos(u, aim, throwPos))
			{
				DCO_Perf.Count("wire:breach_rej_" + DCO_GrenadeUtility.s_sDCORej);
				continue;
			}
			u.AddAction(new SCR_AIThrowGrenadeToBehavior(u, null, throwPos, EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER));
			DCO_GrenadeUtility.NotifyGrenadeThrown(u, aim);
			s_mBreachCount.Set(wire, thrown + 1);
			float hp = HpOf(wire);
			DCO_Perf.Count("wire:breach");
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("wire_breach unit=%1 grp=%2 wire=%3 dist=%4 n=%5 hp=%6 aim=%7",
				DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Log.IdGroup(grp), DCO_Util.PrefabShort(wire), Math.Round(d), thrown + 1, hp.ToString(-1, 2), DCO_Util.XZ(aim)));
			GetGame().GetCallqueue().CallLater(BreachResult, 9000, false, wire, hp, aim);
			GetGame().GetCallqueue().CallLater(BreachTick, 30000, false, wire, grp);
			return;
		}
		GetGame().GetCallqueue().CallLater(BreachTick, 3000, false, wire, grp);
	}

	static void BreachResult(IEntity wire, float hpBefore, vector aim)
	{
		float hp = HpOf(wire);
		string state = "damaged";
		if (Gone(wire))
			state = "destroyed";
		else if (hp >= hpBefore)
			state = "no_effect";
		DCO_Perf.Count("wire:breach_" + state);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("wire_breach_result pos=%1 hp_before=%2 hp_after=%3 result=%4", DCO_Util.XZ(aim), hpBefore.ToString(-1, 2), hp.ToString(-1, 2), state));
	}
}
