modded class AICommander_BaseComponent
{
	DCO_ROEEntry DCO_GetROEEntry(DCO_EGroupTask task)
	{
		DCO_ROETable t = GetROETable();
		if (!t)
			return null;
		return t.Get(task);
	}

	void DCO_SetROEEntry(DCO_EGroupTask task, int mode, float dist, float distEnRoute, int playerID)
	{
		DCO_ROEEntry cur = DCO_GetROEEntry(task);
		if (!m_aROEBase)
			m_aROEBase = {};
		DCO_ROEEntry e;
		foreach (DCO_ROEEntry b : m_aROEBase)
		{
			if (b && b.m_eTask == task)
			{
				e = b;
				break;
			}
		}
		if (!e)
		{
			e = new DCO_ROEEntry();
			e.m_eTask = task;
			e.m_eMode = DCO_EROEMode.TIGHT;
			e.m_fEngageDist = 150;
			e.m_fEngageDistEnRoute = 150;
			if (cur && cur.m_eTask == task)
			{
				e.m_eMode = cur.m_eMode;
				e.m_fEngageDist = cur.m_fEngageDist;
				e.m_fEngageDistEnRoute = cur.m_fEngageDistEnRoute;
			}
			m_aROEBase.Insert(e);
		}
		if (mode >= 0)
			e.m_eMode = Math.ClampInt(mode, 0, 2);
		if (dist >= 0)
			e.m_fEngageDist = dist;
		if (distEnRoute >= 0)
			e.m_fEngageDistEnRoute = distEnRoute;
		m_ROE = null;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("roe_edit cmd=%1 task=%2 mode=%3 dist=%4 dist_moving=%5 gm=%6", GetCommanderUID(), typename.EnumToString(DCO_EGroupTask, task), typename.EnumToString(DCO_EROEMode, e.m_eMode), Math.Round(e.m_fEngageDist), Math.Round(e.m_fEngageDistEnRoute), playerID));
	}

	int DCO_GetGarrisonModeDefault()
	{
		return m_eGarrisonMode;
	}

	void DCO_SetGarrisonModeDefault(int mode, int playerID)
	{
		m_eGarrisonMode = Math.ClampInt(mode, 0, 2);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("gm_set_garrison_mode cmd=%1 mode=%2 gm=%3", GetCommanderUID(), typename.EnumToString(DCO_EGarrisonMode, m_eGarrisonMode), playerID));
	}
}
