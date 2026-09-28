enum DCO_ERadioKind
{
	INFO,
	GO,
	WARNING,
	REPORT
}

class DCO_PlayerComms
{
	static void SendToGameMasters(string title, string text)
	{
		if (!Replication.IsServer())
			return;

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;

		array<SCR_EditorManagerEntity> editors = {};
		core.GetEditorEntities(editors);
		foreach (SCR_EditorManagerEntity editor : editors)
		{
			if (!editor || editor.IsLimited())
				continue;

			SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(editor.GetPlayerID());
			if (comp)
				comp.DCO_SendContact(title, text, false);
		}
	}

	static void ShowRadio(string title, string text, DCO_ERadioKind kind)
	{
		Color c = Color.FromRGBA(243, 178, 49, 255);
		string sound = SCR_SoundEvent.HINT;
		switch (kind)
		{
			case DCO_ERadioKind.GO:
				c = Color.FromRGBA(110, 214, 96, 255);
				sound = SCR_SoundEvent.TASK_ACCEPT;
				break;
			case DCO_ERadioKind.WARNING:
				c = Color.FromRGBA(222, 72, 60, 255);
				sound = SCR_SoundEvent.ACTION_FAILED;
				break;
			case DCO_ERadioKind.REPORT:
				c = Color.FromRGBA(90, 170, 240, 255);
				sound = SCR_SoundEvent.TASK_CREATED;
				break;
		}
		DCO_OrderBanner.Show(title, text, "", "", c, sound);
	}

	static string GetCallsign(SCR_AIGroup grp)
	{
		if (!grp)
			return "Unknown";

		SCR_CallsignGroupComponent cs = SCR_CallsignGroupComponent.Cast(grp.FindComponent(SCR_CallsignGroupComponent));
		if (cs)
		{
			string company, platoon, squad, character, format;
			if (cs.GetCallsignNames(company, platoon, squad, character, format))
			{
				string s = WidgetManager.Translate(format, company, platoon, squad, character);
				if (!s.IsEmpty() && !s.StartsWith("#"))
					return s;
			}
		}
		return "Squad " + grp.GetGroupID();
	}

	static string Bearing(vector from, vector to)
	{
		vector d = to - from;
		float yaw = Math.Atan2(d[0], d[2]) * Math.RAD2DEG;
		if (yaw < 0)
			yaw += 360;
		int sector = Math.Round(yaw / 45);
		array<string> names = {"north", "north-east", "east", "south-east", "south", "south-west", "west", "north-west"};
		return names[sector % 8];
	}

	static string Grid(vector pos)
	{
		return SCR_MapEntity.GetGridLabel(pos);
	}
}
