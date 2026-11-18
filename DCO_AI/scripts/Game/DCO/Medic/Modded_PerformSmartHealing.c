modded class SCR_AIPerformSmartHealing
{
	protected int m_iDCOStep;
	protected string m_sDCOStepName;
	protected typename m_DCOActionType;
	protected int m_iDCOGroup;
	protected IEntity m_DCOItem;
	protected IEntity m_DCOPatient;
	protected IEntity m_DCOUser;
	protected ref set<int> m_DCOTqGroups;
	protected ref set<string> m_DCOSkip;
	protected float m_fDCOStepAt_ms;
	protected bool m_bDCOVanilla;
	protected bool m_bDCOHooked;
	protected SCR_CharacterControllerComponent m_DCOCtrl;

	override void OnEnter(AIAgent owner)
	{
		super.OnEnter(owner);
		m_iDCOStep = 0;
		m_bDCOVanilla = false;
		m_DCOItem = null;
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (m_iDCOStep > 0)
			return DCO_StepTick(owner);
		if (!m_bDCOVanilla && DCO_Decide(owner))
			return DCO_StepTick(owner);
		m_bDCOVanilla = true;
		return super.EOnTaskSimulate(owner, dt);
	}

	override void OnAbort(AIAgent owner, Node nodeCausingAbort)
	{
		if (m_iDCOStep > 0)
		{
			DCO_Unhook();
			if (m_iDCOStep == 2 && m_DCOCtrl)
				m_DCOCtrl.GetInputContext().SetCancelItemAction(true);
			DCO_LogStep("abort", "node_abort");
			m_iDCOStep = 0;
		}
		super.OnAbort(owner, nodeCausingAbort);
	}

	protected bool DCO_Decide(AIAgent owner)
	{
		IEntity user = owner.GetControlledEntity();
		if (!user)
			return false;
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (!u || DCO_Stress.T(u, "med.full_on", 1.0) <= 0)
			return false;
		IEntity patient;
		if (!GetVariableIn(PORT_TARGET_ENTITY, patient))
			patient = user;
		ChimeraCharacter pc = ChimeraCharacter.Cast(patient);
		ChimeraCharacter uc = ChimeraCharacter.Cast(user);
		if (!pc || !uc)
			return false;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(pc.GetDamageManager());
		if (!dm)
			return false;
		if (patient != m_DCOPatient)
		{
			m_DCOPatient = patient;
			m_DCOTqGroups = new set<int>();
			m_DCOSkip = new set<string>();
		}
		m_DCOUser = user;
		m_DCOCtrl = SCR_CharacterControllerComponent.Cast(uc.GetCharacterController());
		if (!m_DCOCtrl)
			return false;

		float blood = 1.0;
		if (dm.GetBloodHitZone())
			blood = dm.GetBloodHitZone().GetHealthScaled();
		array<ECharacterHitZoneGroup> limbs = {};
		SCR_CharacterDamageManagerComponent.GetAllLimbs(limbs);
		int bleeding = 0;
		int bleedingFree = 0;
		foreach (ECharacterHitZoneGroup g : limbs)
		{
			if (dm.GetGroupDamageOverTime(g, EDamageType.BLEEDING) <= 0)
				continue;
			bleeding++;
			if (!m_DCOTqGroups.Contains(g))
				bleedingFree++;
		}
		ECharacterHitZoneGroup worst = GetTargetGroup(patient);
		bool unconscious = pc.GetCharacterController() && pc.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE;
		if (!DCO_MedicDispatcher.IsPrimary(user, patient))
			return false;
		if (patient != user && (unconscious || bleeding >= 3))
			DCO_MedicDispatcher.RequestSecond(patient, user);

		if (!m_DCOSkip.Contains("saline") && (unconscious || blood < 0.75) && !dm.GetGroupSalineBagged(ECharacterHitZoneGroup.RIGHTARM))
		{
			IEntity saline = DCO_FindConsumable(user, SCR_EConsumableType.SALINE);
			if (saline)
				return DCO_StartStep("saline", SCR_SalineBagUserAction, -1, saline, blood, bleeding);
		}
		if (bleedingFree == 0 && !m_DCOTqGroups.IsEmpty())
		{
			foreach (int tq : m_DCOTqGroups)
			{
				if (dm.GetGroupTourniquetted(tq))
					return DCO_StartStep("tq_off", SCR_TourniquetUserAction, tq, null, blood, bleeding);
				m_DCOTqGroups.Remove(m_DCOTqGroups.Find(tq));
				break;
			}
		}
		bool heavy = bleeding >= 2 || blood < 0.5;
		if (heavy && !m_DCOSkip.Contains("tq") && worst > 0 && limbs.Contains(worst) && !dm.GetGroupTourniquetted(worst))
		{
			IEntity tqItem = DCO_FindConsumable(user, SCR_EConsumableType.TOURNIQUET);
			if (tqItem)
				return DCO_StartStep("tq", SCR_TourniquetUserAction, worst, tqItem, blood, bleeding);
		}
		return false;
	}

	protected bool DCO_StartStep(string name, typename actionType, int group, IEntity item, float blood, int bleeding)
	{
		m_sDCOStepName = name;
		m_DCOActionType = actionType;
		m_iDCOGroup = group;
		m_DCOItem = item;
		m_fDCOStepAt_ms = GetGame().GetWorld().GetWorldTime();
		m_iDCOStep = 1;
		if (item)
		{
			SCR_GadgetManagerComponent gm = SCR_GadgetManagerComponent.GetGadgetManager(m_DCOUser);
			if (!gm)
			{
				m_iDCOStep = 0;
				return false;
			}
			gm.SetGadgetMode(item, EGadgetMode.IN_HAND);
		}
		DCO_Perf.Count("medic_treat:" + name);
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_DCOUser), string.Format("medic_treat medic=%1 patient=%2 step=%3 group=%4 blood=%5 bleeding=%6 result=start", DCO_Log.IdUnit(m_DCOUser), DCO_Log.IdUnit(m_DCOPatient), name, group, Math.Round(blood * 100), bleeding));
		return true;
	}

	protected ENodeResult DCO_StepTick(AIAgent owner)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_iDCOStep == 1)
		{
			if (m_DCOItem && m_DCOCtrl.GetAttachedGadgetAtLeftHandSlot() != m_DCOItem)
			{
				if (now - m_fDCOStepAt_ms > 4000.0)
					return DCO_StepDone(false, "equip_timeout");
				return ENodeResult.RUNNING;
			}
			SCR_HealingUserAction action = DCO_FindAction();
			if (!action)
				return DCO_StepDone(false, "no_action");
			if (m_sDCOStepName == "tq_off")
			{
				action.PerformAction(m_DCOPatient, m_DCOUser);
				return DCO_StepDone(true, "-");
			}
			DCO_Hook();
			m_iDCOStep = 2;
			m_fDCOStepAt_ms = now;
			action.PerformAction(m_DCOPatient, m_DCOUser);
			return ENodeResult.RUNNING;
		}
		if (m_iDCOStep == 2)
		{
			if (now - m_fDCOStepAt_ms > 15000.0)
				return DCO_StepDone(false, "use_timeout");
			return ENodeResult.RUNNING;
		}
		if (m_iDCOStep == 3)
			return DCO_StepDone(true, "-");
		return DCO_StepDone(false, "use_failed");
	}

	protected ENodeResult DCO_StepDone(bool ok, string why)
	{
		DCO_Unhook();
		if (ok && m_sDCOStepName == "tq")
			m_DCOTqGroups.Insert(m_iDCOGroup);
		if (ok && m_sDCOStepName == "tq_off")
			m_DCOTqGroups.Remove(m_DCOTqGroups.Find(m_iDCOGroup));
		if (!ok)
		{
			m_DCOSkip.Insert(m_sDCOStepName);
			DCO_Perf.Count("medic_treat_fail:" + m_sDCOStepName);
		}
		string result = "ok";
		if (!ok)
			result = "fail";
		DCO_LogStep(result, why);
		m_iDCOStep = 0;
		m_DCOItem = null;
		return ENodeResult.SUCCESS;
	}

	protected void DCO_LogStep(string result, string why)
	{
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_DCOUser), string.Format("medic_treat medic=%1 patient=%2 step=%3 group=%4 result=%5 why=%6", DCO_Log.IdUnit(m_DCOUser), DCO_Log.IdUnit(m_DCOPatient), m_sDCOStepName, m_iDCOGroup, result, why));
	}

	protected SCR_HealingUserAction DCO_FindAction()
	{
		array<BaseUserAction> actions = {};
		GetActions(m_DCOPatient, actions);
		foreach (BaseUserAction base : actions)
		{
			SCR_HealingUserAction a = SCR_HealingUserAction.Cast(base);
			if (!a || a.Type() != m_DCOActionType)
				continue;
			if (m_iDCOGroup >= 0 && a.GetUserActionGroup() != m_iDCOGroup)
				continue;
			if (a.CanBePerformedScript(m_DCOUser))
				return a;
		}
		return null;
	}

	protected void DCO_Hook()
	{
		if (m_bDCOHooked || !m_DCOCtrl)
			return;
		m_DCOCtrl.m_OnItemUseBeganInvoker.Insert(DCO_OnUseBegan);
		m_DCOCtrl.m_OnItemUseEndedInvoker.Insert(DCO_OnUseEnded);
		m_bDCOHooked = true;
	}

	protected void DCO_Unhook()
	{
		if (!m_bDCOHooked || !m_DCOCtrl)
			return;
		m_DCOCtrl.m_OnItemUseBeganInvoker.Remove(DCO_OnUseBegan);
		m_DCOCtrl.m_OnItemUseEndedInvoker.Remove(DCO_OnUseEnded);
		m_bDCOHooked = false;
	}

	void DCO_OnUseBegan(IEntity item, ItemUseParameters animParams)
	{
		if (m_iDCOStep == 2)
			m_fDCOStepAt_ms = GetGame().GetWorld().GetWorldTime();
	}

	void DCO_OnUseEnded(IEntity item, bool actionCompleted, ItemUseParameters animParams)
	{
		if (m_iDCOStep != 2)
			return;
		if (actionCompleted)
			m_iDCOStep = 3;
		else
			m_iDCOStep = 4;
	}

	override protected ECharacterHitZoneGroup GetTargetGroup(IEntity targetEntity)
	{
		ChimeraCharacter c = ChimeraCharacter.Cast(targetEntity);
		if (!c)
			return 0;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(c.GetDamageManager());
		if (!dm)
			return 0;
		array<ECharacterHitZoneGroup> limbs = {};
		SCR_CharacterDamageManagerComponent.GetAllLimbs(limbs);
		array<ECharacterHitZoneGroup> free = {};
		foreach (ECharacterHitZoneGroup g : limbs)
		{
			if (!dm.GetGroupIsBeingHealed(g))
				free.Insert(g);
		}
		if (free.IsEmpty() || free.Count() == limbs.Count())
			return super.GetTargetGroup(targetEntity);
		SCR_CharacterHitZone hz = SCR_CharacterHitZone.Cast(dm.GetMostDOTHitZone(EDamageType.BLEEDING, false, free));
		if (!hz)
			return super.GetTargetGroup(targetEntity);
		DCO_Perf.Count("medic_treat:free_limb");
		return hz.GetHitZoneGroup();
	}

	protected static IEntity DCO_FindConsumable(IEntity ent, SCR_EConsumableType type)
	{
		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(ent.FindComponent(InventoryStorageManagerComponent));
		if (!inv)
			return null;
		array<IEntity> items = {};
		inv.GetItems(items);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;
			SCR_ConsumableItemComponent c = SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent));
			if (c && c.GetConsumableType() == type)
				return item;
		}
		return null;
	}
}
