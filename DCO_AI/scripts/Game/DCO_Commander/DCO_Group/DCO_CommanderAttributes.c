// === ADDED: GM Commander Attributes ===
// Attribute GM buat entity commander: mode dan enam nilai personality.
//
// Pola dropdown-nya sama dengan DCO_CommanderAssign.c dan attribute DCO lain
// (SCR_BaseFloatValueHolderEditorAttribute + preset yang dibikin di GetEntries),
// jadi gak butuh layout baru selain yang udah dipakai attribute DCO existing.
//
// ReadVariable & WriteVariable jalan di server, GetEntries di client. Preset di
// sini semuanya statis (label hardcoded), jadi gak ada urusan sinkronisasi
// index antara client dan server.

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderBaseAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	protected AICommander_BaseComponent GetCommander(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! Label preset, urut dari index 0. Subclass yang ngisi.
	protected void GetPresetLabels(notnull out array<string> outLabels)
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Nilai float yang disimpan per index. Default: index itu sendiri.
	protected float GetPresetValue(int index)
	{
		return index;
	}

	//------------------------------------------------------------------------------------------------
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();

		array<string> labels = {};
		GetPresetLabels(labels);

		for (int i = 0; i < labels.Count(); i++)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(labels[i]);
			value.SetFloatValue(GetPresetValue(i));
			m_aValues.Insert(value);
		}

		return super.GetEntries(outEntries);
	}
}

//------------------------------------------------------------------------------------------------
//! Commander Mode: OFFENSIVE / DEFENSIVE / BALANCED.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderModeAttribute : DCO_CommanderBaseAttribute
{
	//------------------------------------------------------------------------------------------------
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		// Urutan HARUS sama dengan CMD_ECommanderMode (OFFENSIVE=0, DEFENSIVE=1, BALANCED=2).
		outLabels.Insert("Offensive");
		outLabels.Insert("Defensive");
		outLabels.Insert("Balanced");
	}

	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(cmd.GetCommanderMode());
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return;

		cmd.SetCommanderMode(var.GetInt());
	}
}

//------------------------------------------------------------------------------------------------
//! Base buat enam nilai personality (0.0 - 1.0, lima langkah).
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPersonalityBaseAttribute : DCO_CommanderBaseAttribute
{
	//------------------------------------------------------------------------------------------------
	protected float ReadPersonality(AICommander_BaseComponent cmd)
	{
		return 0.5;
	}

	//------------------------------------------------------------------------------------------------
	protected void WritePersonality(AICommander_BaseComponent cmd, float value)
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Label ujung bawah & atas, diisi subclass biar GM tau artinya.
	protected string GetLowLabel()  { return "rendah"; }
	protected string GetHighLabel() { return "tinggi"; }

	//------------------------------------------------------------------------------------------------
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert(string.Format("0.00 - %1", GetLowLabel()));
		outLabels.Insert("0.25");
		outLabels.Insert("0.50 - seimbang");
		outLabels.Insert("0.75");
		outLabels.Insert(string.Format("1.00 - %1", GetHighLabel()));
	}

	//------------------------------------------------------------------------------------------------
	override protected float GetPresetValue(int index)
	{
		return index * 0.25;
	}

	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;

		// Nilai prefab bisa aja bukan kelipatan 0.25 (atau hasil Random
		// Personality) -- dibulatin ke preset terdekat cuma buat tampilan.
		float value = ReadPersonality(cmd);
		int index = Math.Round(value / 0.25);
		index = Math.ClampInt(index, 0, 4);

		return SCR_BaseEditorAttributeVar.CreateFloat(index * 0.25);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return;

		WritePersonality(cmd, var.GetFloat());
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAggressionAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetAggression(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetAggression(value); }
	override protected string GetLowLabel()  { return "tunggu recon"; }
	override protected string GetHighLabel() { return "serang tanpa recon"; }
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAdaptabilityAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetAdaptability(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetAdaptability(value); }
	override protected string GetLowLabel()  { return "lambat bereaksi"; }
	override protected string GetHighLabel() { return "sangat responsif"; }
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderRiskTakingAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetRiskTaking(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetRiskTaking(value); }
	override protected string GetLowLabel()  { return "butuh intel jelas"; }
	override protected string GetHighLabel() { return "nyerang walau buta"; }
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderResilienceAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetResilience(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetResilience(value); }
	override protected string GetLowLabel()  { return "gampang retreat"; }
	override protected string GetHighLabel() { return "tahan banting"; }
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPatienceAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetPatience(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetPatience(value); }
	override protected string GetLowLabel()  { return "cepet realokasi"; }
	override protected string GetHighLabel() { return "sabar nunggu"; }
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderCombatFocusAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetCombatFocus(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetCombatFocus(value); }
	override protected string GetLowLabel()  { return "fokus objective"; }
	override protected string GetHighLabel() { return "ngejar musuh"; }
}

// DCO_CommanderNameAttribute dicabut: SCR_BaseEditorAttributeVar cuma support
// int/float/bool/vector (gak ada CreateString/GetString) dan vanilla gak punya
// layout edit box buat attribute. Rename tetap bisa lewat
// AICommander_BaseComponent.RenameCommanderFromGM dari script.
// === END ADDED ===
