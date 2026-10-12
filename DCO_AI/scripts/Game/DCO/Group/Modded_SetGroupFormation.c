modded class SCR_AISetGroupFormation
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		ENodeResult r = super.EOnTaskSimulate(owner, dt);
		if (m_SettingsComponent && m_SettingsComponent.GetCurrentSetting(SCR_AIGroupFormationSettingBase))
		{
			DCO_Perf.Count("fm:bt_setting_kept");
			return r;
		}
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
		if (!g || g.DCO_GetFormation() == "")
			return r;
		DCO_Formation.Apply(SCR_AIGroup.Cast(owner), g.DCO_GetFormation());
		DCO_Perf.Count("fm:bt_reapply");
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(AIGroup.Cast(owner)), string.Format("formation_bt_reapply grp=%1 vanilla=%2 dco=%3", DCO_Log.IdGroup(AIGroup.Cast(owner)), typename.EnumToString(SCR_EAIGroupFormation, m_eFormation), g.DCO_GetFormation()));
		return r;
	}
}
