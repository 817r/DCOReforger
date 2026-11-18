class DCO_ShotCounter : Managed
{
	protected static ref map<IEntity, ref DCO_ShotCounter> s_mAll;

	protected IEntity m_Owner;
	protected int m_iShots;
	protected int m_iLast;
	protected BaseMagazineComponent m_LastMag;
	protected int m_iLastAmmo;
	protected float m_fLastShot_ms = -100000.0;

	static int Total(IEntity ent)
	{
		DCO_ShotCounter c = Track(ent);
		if (!c)
			return 0;
		return c.m_iShots;
	}

	static float LastShotAt(IEntity ent)
	{
		DCO_ShotCounter c = Track(ent);
		if (!c)
			return -100000.0;
		return c.m_fLastShot_ms;
	}

	static DCO_ShotCounter Track(IEntity ent)
	{
		if (!ent)
			return null;
		if (!s_mAll)
			s_mAll = new map<IEntity, ref DCO_ShotCounter>();
		DCO_ShotCounter c = s_mAll.Get(ent);
		if (c)
			return c;
		if (s_mAll.Count() > 2000)
		{
			map<IEntity, ref DCO_ShotCounter> fresh = new map<IEntity, ref DCO_ShotCounter>();
			foreach (IEntity k, DCO_ShotCounter v : s_mAll)
			{
				if (k)
					fresh.Set(k, v);
			}
			s_mAll = fresh;
		}
		c = new DCO_ShotCounter();
		c.m_Owner = ent;
		s_mAll.Set(ent, c);
		EventHandlerManagerComponent ev = EventHandlerManagerComponent.Cast(ent.FindComponent(EventHandlerManagerComponent));
		if (ev)
			ev.RegisterScriptHandler("OnAmmoCountChanged", c, c.OnAmmoCountChanged);
		return c;
	}

	static int Since(IEntity ent)
	{
		DCO_ShotCounter c = Track(ent);
		if (!c)
			return 0;
		int n = c.m_iShots - c.m_iLast;
		c.m_iLast = c.m_iShots;
		return n;
	}

	void OnAmmoCountChanged(BaseWeaponComponent weapon, BaseMuzzleComponent muzzle, BaseMagazineComponent magazine, int ammoCount, bool isBarrelChambered)
	{
		if (magazine && magazine == m_LastMag && ammoCount < m_iLastAmmo)
		{
			int n = m_iLastAmmo - ammoCount;
			m_iShots += n;
			m_fLastShot_ms = GetGame().GetWorld().GetWorldTime();
			DCO_Perf.CountN("ev:round", n);
		}
		m_LastMag = magazine;
		m_iLastAmmo = ammoCount;
	}
}

modded class SCR_CharacterDamageManagerComponent
{
	protected int m_iDCOLastDmg = -1;
	protected string m_sDCOLastSrc = "-";
	protected int m_iDCOFatalDmg = -1;
	protected string m_sDCOFatalSrc = "-";
	protected float m_fDCOFatalAt = -1;
	protected bool m_bDCOLastBleed;
	protected float m_fDCOHpLast = 1.0;
	protected float m_fDCOHpBefore = 1.0;
	protected string m_sDCOHitZone = "-";
	protected ref array<float> m_aDCOHitAt;
	protected bool m_bDCOWasUncon;
	protected bool m_bDCOWasBleeding;
	protected int m_iDCOHitGroup;
	protected bool m_bDCOLegDown;
	protected float m_fDCOLegDownAt;
	protected float m_fDCOLegCheckAt;

	bool DCO_LastHitLeg()
	{
		return m_iDCOHitGroup == ECharacterHitZoneGroup.LEFTLEG || m_iDCOHitGroup == ECharacterHitZoneGroup.RIGHTLEG;
	}

	void DCO_SetLegDown()
	{
		m_fDCOLegCheckAt = 0;
		if (m_bDCOLegDown)
			return;
		m_bDCOLegDown = true;
		m_fDCOLegDownAt = DCO_Util.NowS();
		DCO_Perf.Count("hit:leg_down");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(GetOwner()), string.Format("leg_down unit=%1 state=start zone=%2", DCO_Log.IdUnit(GetOwner()), m_sDCOHitZone));
	}

	bool DCO_LegDown()
	{
		if (!m_bDCOLegDown)
			return false;
		float now = DCO_Util.NowS();
		if (now - m_fDCOLegCheckAt < 1.0)
			return true;
		m_fDCOLegCheckAt = now;
		if (GetGroupDamageOverTime(ECharacterHitZoneGroup.LEFTLEG, EDamageType.BLEEDING) > 0 || GetGroupDamageOverTime(ECharacterHitZoneGroup.RIGHTLEG, EDamageType.BLEEDING) > 0)
			return true;
		m_bDCOLegDown = false;
		DCO_Perf.Count("hit:leg_down_end");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(GetOwner()), string.Format("leg_down unit=%1 state=end held_s=%2", DCO_Log.IdUnit(GetOwner()), Math.Round(now - m_fDCOLegDownAt)));
		return false;
	}

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		m_iDCOLastDmg = damageContext.damageType;
		m_sDCOLastSrc = "-";
		IEntity src = damageContext.damageSource;
		if (src && src.GetPrefabData())
			m_sDCOLastSrc = FilePath.StripExtension(FilePath.StripPath(src.GetPrefabData().GetPrefabName()));
		if (!Replication.IsServer())
			return;
		float now = DCO_Util.NowS();
		bool finisher = damageContext.damageType == EDamageType.TRUE && !damageContext.damageSource && m_iDCOFatalDmg >= 0;
		if (finisher)
		{
			m_bDCOLastBleed = m_bDCOLastBleed || m_bDCOWasBleeding || m_bDCOWasUncon;
			DCO_Perf.Count("death:finisher_true");
		}
		else
			m_bDCOLastBleed = damageContext.damageType == EDamageType.BLEEDING;
		IEntity attacker = null;
		if (damageContext.instigator)
			attacker = damageContext.instigator.GetInstigatorEntity();
		if (!m_bDCOLastBleed && !finisher)
		{
			m_iDCOFatalDmg = damageContext.damageType;
			m_sDCOFatalSrc = m_sDCOLastSrc;
			m_fDCOFatalAt = now;
			m_fDCOHpBefore = m_fDCOHpLast;
			if (damageContext.struckHitZone)
				m_sDCOHitZone = damageContext.struckHitZone.GetName();
			m_iDCOHitGroup = 0;
			SCR_CharacterHitZone dcoHz = SCR_CharacterHitZone.Cast(damageContext.struckHitZone);
			if (dcoHz)
				m_iDCOHitGroup = dcoHz.GetHitZoneGroup();
			if (!m_aDCOHitAt)
				m_aDCOHitAt = {};
			m_aDCOHitAt.Insert(now);
			if (m_aDCOHitAt.Count() > 20)
				m_aDCOHitAt.RemoveOrdered(0);
			DCO_Dev.OnHit(GetOwner(), attacker);
			DCO_IndLearn.OnDamage(GetOwner(), attacker);
			DCO_Beaten.OnDamage(GetOwner(), attacker);
			DCO_GroupUtilityComponent.DCO_LodWakeFor(GetOwner(), "damage");
			DCO_BtWatch.Combat();
			DCO_PlyLearn.OnDamage(GetOwner(), attacker);
		}
		m_fDCOHpLast = GetHealthScaled();
		DCO_WoundCheck(attacker, damageContext.damageType);
	}

	protected void DCO_WoundCheck(IEntity attacker, int dmgType)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(GetOwner());
		if (!ch)
			return;
		bool uncon = ch.GetCharacterController() && ch.GetCharacterController().GetLifeState() == ECharacterLifeState.INCAPACITATED;
		bool bleeding = IsBleeding();
		string state = "";
		if (uncon && !m_bDCOWasUncon)
			state = "incapacitated";
		else if (bleeding && !m_bDCOWasBleeding)
			state = "bleeding";
		m_bDCOWasUncon = uncon;
		m_bDCOWasBleeding = bleeding;
		if (state.IsEmpty())
			return;
		DCO_Dev.Wounded(ch, attacker, state, typename.EnumToString(EDamageType, dmgType), m_sDCOLastSrc);
	}

	string DCO_LastAmmo()
	{
		if (m_iDCOFatalDmg >= 0)
			return typename.EnumToString(EDamageType, m_iDCOFatalDmg) + ":" + m_sDCOFatalSrc;
		if (m_iDCOLastDmg < 0)
			return "-";
		return typename.EnumToString(EDamageType, m_iDCOLastDmg) + ":" + m_sDCOLastSrc;
	}

	float DCO_HpBefore()		{ return m_fDCOHpBefore; }
	string DCO_LastHitZone()	{ return m_sDCOHitZone; }
	string DCO_FatalSrc()		{ return m_sDCOFatalSrc; }
	float DCO_FatalAt()			{ return m_fDCOFatalAt; }
	bool DCO_LastWasBleed()		{ return m_bDCOLastBleed; }

	string DCO_FatalType()
	{
		if (m_iDCOFatalDmg < 0)
			return "-";
		return typename.EnumToString(EDamageType, m_iDCOFatalDmg);
	}

	int DCO_HitsWithin(float sec)
	{
		if (!m_aDCOHitAt)
			return 0;
		float cut = DCO_Util.NowS() - sec;
		int n = 0;
		foreach (float t : m_aDCOHitAt)
		{
			if (t >= cut)
				n++;
		}
		return n;
	}
}
