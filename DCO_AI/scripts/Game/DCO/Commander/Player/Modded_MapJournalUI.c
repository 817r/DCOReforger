modded class SCR_MapJournalUI
{
	override void SetJournalVisibility(bool visibility)
	{
		if (m_ToolMenuEntry)
			m_ToolMenuEntry.SetActive(visibility);

		if (!m_wJournalFrame)
			return;

		m_wJournalFrame.SetVisible(visibility);
		if (visibility)
			FocusOnFirstEntry();
	}
}
