[EntityEditorProps(category: "GameCNHipted/Capture", description: "Zone/Area untuk dicapture (AI & Multi-Faction)")]
class CNH_CaptureAreaClass : ScriptComponentClass {}

class CNH_CaptureArea : ScriptComponent
{
    [Attribute("50", UIWidgets.EditBox, desc: "Radius Area Capture (meter)")]
    float m_fRadius;

    [Attribute("2", UIWidgets.EditBox, desc: "Poin ke Manager tiap tick")]
    int m_iPointsPerTick;

    [Attribute("10", UIWidgets.EditBox, desc: "Jeda Waktu Cek Area (detik)")]
    float m_fTickInterval;

    [Attribute("5", UIWidgets.EditBox, desc: "Kecepatan Capture per tick")]
    int m_iCaptureSpeed;
	
    [Attribute("50", UIWidgets.EditBox, desc: "Kecepatan Capture per tick")]
    int m_iSectorLossPenalty;

    float m_fCaptureProgress = 0;
    string m_sCurrentOwner = "";
    string m_sCapturingFaction = "";

    ref map<string, int> m_mFactionCounts = new map<string, int>();
    
    CNH_CaptureManager m_Manager;

    override void EOnInit(IEntity owner)
    {
        if (!GetGame().InPlayMode()) return;

        IEntity managerEnt = GetGame().GetWorld().FindEntityByName("CaptureManager");
        if (managerEnt)
            m_Manager = CNH_CaptureManager.Cast(managerEnt.FindComponent(CNH_CaptureManager));

        GetGame().GetCallqueue().CallLater(CaptureTick, m_fTickInterval * 1000, true);
    }

    void CaptureTick()
    {
        m_mFactionCounts.Clear();

        GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin(), m_fRadius, ProcessEntity, FilterEntity, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.STATIC);

        string dominantFaction = "";
        int highestCount = 0;
        bool isContested = false;

        for (int i = 0; i < m_mFactionCounts.Count(); i++)
        {
            string factionKey = m_mFactionCounts.GetKey(i);
            int count = m_mFactionCounts.GetElement(i);

            if (count > highestCount)
            {
                highestCount = count;
                dominantFaction = factionKey;
                isContested = false;
            }
            else if (count == highestCount && count > 0)
            {
                isContested = true;
            }
        }

        if (highestCount > 0 && !isContested)
        {
            if (m_sCapturingFaction == dominantFaction)
            {
                if (m_fCaptureProgress < 100)
                {
                    m_fCaptureProgress = Math.Clamp(m_fCaptureProgress + m_iCaptureSpeed, 0, 100);
                    if (m_fCaptureProgress == 100 && m_sCurrentOwner != dominantFaction)
                    {
                        m_sCurrentOwner = dominantFaction;
                    }
                }
            }
			else
            {
                m_fCaptureProgress = Math.Clamp(m_fCaptureProgress - m_iCaptureSpeed, 0, 100);
                
                if (m_fCaptureProgress == 0)
                {
                    m_sCapturingFaction = dominantFaction;
                    if (m_sCurrentOwner != "")
                    {
                        if (m_Manager)
                        {
                            m_Manager.RemoveScore(m_sCurrentOwner, m_iSectorLossPenalty); 
                        }
                        
                        m_sCurrentOwner = "";
                    }
                }
            }
        }

        if (m_Manager && m_sCurrentOwner != "")
        {
            m_Manager.AddScore(m_sCurrentOwner, m_iPointsPerTick);
        }
    }
	
    bool FilterEntity(IEntity ent)
    {
        return ent.Type().IsInherited(ChimeraCharacter);
    }

    bool ProcessEntity(IEntity ent)
    {
        DamageManagerComponent damageManager = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
        if (damageManager && damageManager.GetState() == EDamageState.DESTROYED) 
            return true; 

        FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
        if (factionComp)
        {
            Faction faction = factionComp.GetAffiliatedFaction();
            if (faction)
            {
                string factionKey = faction.GetFactionKey();
                
                int currentCount = 0;
                m_mFactionCounts.Find(factionKey, currentCount);
                m_mFactionCounts.Set(factionKey, currentCount + 1);
            }
        }
        return true;
    }

    void ~CNH_CaptureArea()
    {
        if (GetGame() && GetGame().GetCallqueue())
            GetGame().GetCallqueue().Remove(CaptureTick);
    }
}