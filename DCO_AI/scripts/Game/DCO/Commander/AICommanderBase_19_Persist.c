modded class AICommander_BaseComponent
{
	void DCO_SaveState(SaveContext context)
	{
		int mode = m_eCommanderMode;
		context.WriteValue("uid", m_sCommanderUID);
		context.WriteValue("faction", m_sFactionKey);
		context.WriteValue("mode", mode);
		context.WriteValue("stock_init", m_bStockInit);
		context.WriteValue("stock_left", m_iStockLeft);
		context.WriteValue("stock_start", m_iStockStart);
		context.WriteValue("stock_pts", m_iPointsStock);
		context.WriteValue("pts_start", m_iPointsStart);
	}

	string DCO_LoadState(LoadContext context)
	{
		string uid;
		string fk;
		int mode = m_eCommanderMode;
		bool stockInit;
		int left = m_iStockLeft;
		int start = m_iStockStart;
		int pts = m_iPointsStock;
		int ptsStart = m_iPointsStart;
		if (!context.ReadValue("uid", uid))
			return string.Empty;
		context.ReadValue("faction", fk);
		context.ReadValue("mode", mode);
		context.ReadValue("stock_init", stockInit);
		context.ReadValue("stock_left", left);
		context.ReadValue("stock_start", start);
		context.ReadValue("stock_pts", pts);
		context.ReadValue("pts_start", ptsStart);
		string oldUid = m_sCommanderUID;
		if (!uid.IsEmpty() && uid != m_sCommanderUID)
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			bool renamed = mgr && mgr.RenameCommander(this, uid);
			if (!renamed)
			{
				DCO_Perf.Count("persist:cmd_uid_kept");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("persist_cmd_uid_kept cmd=%1 saved_uid=%2 reason=taken_or_no_manager", m_sCommanderUID, uid));
			}
		}
		if (!fk.IsEmpty() && fk != m_sFactionKey)
			SetCommanderFactionKey(fk);
		SetCommanderMode(mode);
		if (stockInit)
		{
			m_bStockInit = true;
			m_iStockLeft = left;
			m_iStockStart = start;
		}
		m_iPointsStock = pts;
		m_iPointsStart = ptsStart;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("persist_cmd_state cmd=%1 old_uid=%2 faction=%3 mode=%4 stock_left=%5 stock_init=%6 stock_pts=%7", m_sCommanderUID, oldUid, m_sFactionKey, typename.EnumToString(CMD_ECommanderMode, m_eCommanderMode), left, DCO_Util.B01(stockInit), pts));
		return fk;
	}
}
