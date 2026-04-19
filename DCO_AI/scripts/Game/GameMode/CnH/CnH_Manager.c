[EntityEditorProps(category: "GameScripted/Capture", description: "Manager Pusat Untuk Skor Area Capture (Multi-Faction)")]
class CNH_CaptureManagerClass : ScriptComponentClass {}

class CNH_CaptureManager : ScriptComponent
{
    [Attribute("1000", UIWidgets.EditBox, desc: "Skor maksimal untuk menang")]
    int m_iMaxScore;
	
    [Attribute("3", UIWidgets.EditBox, desc: "Jumlah Base dicapture untuk menang")]
    int m_iMaxBaseToCapture;

	ref map<string, int> m_mFactionBaseTaken= new map<string, int>();
    ref map<string, int> m_mFactionScores = new map<string, int>();
    bool m_bGameEnded = false;

    void AddScore(string factionKey, int points)
    {
        if (m_bGameEnded || factionKey == "") return;

        int currentScore = 0;
		if (!m_mFactionScores.Find(factionKey, currentScore))
		{
			m_mFactionScores.Insert(factionKey, currentScore);
			m_mFactionBaseTaken.Insert(factionKey, 1);
		} else
		{
			int baseTaken = 0;
        	currentScore += points;
        	m_mFactionScores.Set(factionKey, currentScore);
			if(m_mFactionBaseTaken.Find(factionKey, baseTaken))
			{
				baseTaken++;
				m_mFactionBaseTaken.Set(factionKey, currentScore);
			}	
		}

        if (currentScore >= m_iMaxScore)
        {
            m_bGameEnded = true;
			FactionManager fManager = GetGame().GetFactionManager();
			Faction fc = fManager.GetFactionByKey(factionKey);
			int fId = fManager.GetFactionIndex(fc);
			EndGame(fId);
        }
    }
	
	void EndGame(int fcId)
	{
		SCR_GameModeEndData endData = SCR_GameModeEndData.CreateSimple(EGameOverTypes.ENDREASON_SCORELIMIT, winnerFactionId: fcId);
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gm.EndGameMode(endData);
	}

    void RemoveScore(string factionKey, int points)
    {
        if (m_bGameEnded || factionKey == "") return;

        int currentScore = 0;
        if (m_mFactionScores.Find(factionKey, currentScore))
        {
            currentScore -= points;

            if (currentScore < 0) 
                currentScore = 0;

            m_mFactionScores.Set(factionKey, currentScore);
			
			int baseTaken = 0;
			if(m_mFactionBaseTaken.Find(factionKey, baseTaken))
			{
				baseTaken--;
				m_mFactionBaseTaken.Set(factionKey, currentScore);
			}	
        }
    }
}