enum DCO_ESetKind
{
	CHECK,
	DROP,
	MULTI,
	SLIDER,
	TEXT
}

enum DCO_ESetAct
{
	NONE,
	TIP,
	FOLDER,
	SUB,
	FAVS,
	CELL,
	STAR,
	RESET,
	MINUS,
	PLUS,
	NUMBOX,
	TEXTBOX,
	SEARCH,
	ONLYMOD,
	RESETALL,
	CANCEL,
	APPLY,
	PRESET,
	EXPORT,
	IMPORT,
	LOG,
	PANEL_CLOSE,
	PRESET_PICK,
	PRESET_LOAD,
	PRESET_SAVE,
	PRESET_DELETE,
	PRESET_NAME,
	CONFIRM,
	LOG_FILTER,
	LOG_REFRESH,
	FACTION,
	PRESET_SAVE_LOCAL,
	MODE,
	HELP,
	SIMPLE,
	METER,
	ROW,
	SLIDERBAR,
	REVERT_ROW,
	REVERT_SEC,
	REVERT_ALL,
	REVERT_SIMPLE,
	CLOSE,
	FIND,
	SPIN,
	CLEAR_SEARCH,
	DESC_ACTION,
	DISCARD,
	DISCARD_BG,
	DISCARD_BACK,
	DISCARD_APPLY,
	SPAWNER,
	CFG_SAVE
}

enum DCO_ESetPanel
{
	NONE,
	PRESETS,
	REVIEW,
	LOG
}

class DCO_SetRow
{
	SCR_BaseEditorAttribute m_Attr;
	string m_sClass;
	int m_iSub;
	string m_sLabel;
	string m_sDesc;
	string m_sSearch;
	int m_iKind;
	ref array<string> m_aNames = {};
	ref array<float> m_aValues = {};
	ref SCR_BaseEditorAttributeEntrySlider m_Slider;
	float m_fMin;
	float m_fMax = 1;
	float m_fStep = 1;
	string m_sFormat = "%1";
	vector m_vOrig;
	vector m_vCur;
	vector m_vDef;
	bool m_bHasDef;
	bool m_bKnownDef;
	bool m_bConflict;
	bool m_bTouched;
	bool m_bDisabled;
	string m_sTextOrig;
	string m_sText;
	bool m_bMember;
	bool m_bInternal;
	bool m_bBeta;
}

class DCO_SetSub
{
	string m_sName;
	string m_sShort;
	string m_sDesc;
	ResourceName m_Category;
	int m_iFolder;
	ref array<int> m_aRows = {};
}

class DCO_SetFolder
{
	string m_sName;
	string m_sDesc;
	string m_sKey;
	bool m_bWorld;
	bool m_bMember;
	int m_iTarget;
	ref array<int> m_aSubs = {};
}

class DCO_SetLine
{
	int m_iType;
	int m_iRow = -1;
	int m_iPart;
	float m_fH;
	string m_sText;
	string m_sHint;
	bool m_bPath;
}

class DCO_SetFactionEdit
{
	string m_sName;
	bool m_bLoaded;
	ref map<string, vector> m_mOrig = new map<string, vector>();
	ref map<string, vector> m_mCur = new map<string, vector>();
}

class DCO_SettingsPrefs
{
	protected static ref array<string> s_aFav;
	protected static ref array<string> s_aClosed;
	protected static int s_iMode;
	protected static bool s_bHelp;

	static void Load()
	{
		if (s_aFav)
			return;
		s_aFav = {};
		s_aClosed = {};
		FileHandle f = FileIO.OpenFile("$profile:DCO/SettingsWindow.txt", FileMode.READ);
		if (!f)
			return;
		string line;
		int guard = 0;
		while (guard < 5000 && f.ReadLine(line) >= 0)
		{
			guard++;
			line = line.Trim();
			if (line.StartsWith("fav:"))
				s_aFav.Insert(line.Substring(4, line.Length() - 4));
			else if (line.StartsWith("closed:"))
				s_aClosed.Insert(line.Substring(7, line.Length() - 7));
			else if (line == "mode:simple")
				s_iMode = 1;
			else if (line == "mode:advanced")
				s_iMode = 2;
			else if (line == "mode:expert")
				s_iMode = 3;
			else if (line == "help:open")
				s_bHelp = true;
		}
		f.Close();
	}

	static void Save()
	{
		if (!s_aFav)
			return;
		FileIO.MakeDirectory("$profile:DCO");
		FileHandle f = FileIO.OpenFile("$profile:DCO/SettingsWindow.txt", FileMode.WRITE);
		if (!f)
			return;
		foreach (string s : s_aFav)
		{
			f.WriteLine("fav:" + s);
		}
		foreach (string s : s_aClosed)
		{
			f.WriteLine("closed:" + s);
		}
		if (s_iMode == 1)
			f.WriteLine("mode:simple");
		else if (s_iMode == 2)
			f.WriteLine("mode:advanced");
		else if (s_iMode == 3)
			f.WriteLine("mode:expert");
		if (s_bHelp)
			f.WriteLine("help:open");
		f.Close();
	}

	static bool IsSimple()
	{
		Load();
		return s_iMode != 2 && s_iMode != 3;
	}

	static bool IsExpert()
	{
		Load();
		return s_iMode == 3;
	}

	static void SetMode(int mode)
	{
		Load();
		s_iMode = mode;
		Save();
	}

	static bool IsHelpOpen()
	{
		Load();
		return s_bHelp;
	}

	static void SetHelpOpen(bool open)
	{
		Load();
		s_bHelp = open;
		Save();
	}

	static bool IsFav(string key)
	{
		Load();
		return s_aFav.Contains(key);
	}

	static void ToggleFav(string key)
	{
		Load();
		int i = s_aFav.Find(key);
		if (i >= 0)
			s_aFav.Remove(i);
		else
			s_aFav.Insert(key);
		Save();
	}

	static bool IsClosed(string key)
	{
		Load();
		return s_aClosed.Contains(key);
	}

	static void ToggleClosed(string key)
	{
		Load();
		int i = s_aClosed.Find(key);
		if (i >= 0)
			s_aClosed.Remove(i);
		else
			s_aClosed.Insert(key);
		Save();
	}
}

class DCO_SetInput : ScriptedWidgetEventHandler
{
	protected DCO_SettingsDialogUI m_Dlg;

	void DCO_SetInput(DCO_SettingsDialogUI dlg)
	{
		m_Dlg = dlg;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Dlg && w)
			m_Dlg.OnClickAt(w.GetUserID(), x);
		return true;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_Dlg && w)
			m_Dlg.OnHover(w.GetUserID(), true);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_Dlg || !w)
			return false;
		if (enterW && enterW.GetUserID() == w.GetUserID())
			return false;
		m_Dlg.OnHover(w.GetUserID(), false);
		return false;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (m_Dlg && w && button == 0)
			m_Dlg.StartDrag(w.GetUserID(), x);
		return false;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (m_Dlg && button == 0)
			m_Dlg.EndDrag();
		return false;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (m_Dlg)
			m_Dlg.OnWheel(x, wheel);
		return true;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (m_Dlg && w)
			m_Dlg.OnEditChange(w, finished);
		return false;
	}

	override bool OnFocusLost(Widget w, int x, int y)
	{
		if (m_Dlg && w && EditBoxWidget.Cast(w))
			m_Dlg.OnEditChange(w, true);
		return false;
	}
}

class DCO_SettingsDialogUI : EditorMenuBase
{
	protected SCR_AttributesManagerEditorComponent m_Manager;
	protected ref DCO_SetInput m_Input;
	protected Widget m_wWindow;
	protected Widget m_wDyn;
	protected Widget m_wFixed;
	protected Widget m_wTop;
	protected Widget m_wTip;
	protected EditBoxWidget m_wSearch;

	protected ref array<ref DCO_SetRow> m_aRows = {};
	protected ref array<ref DCO_SetSub> m_aSubs = {};
	protected ref array<ref DCO_SetFolder> m_aFolders = {};
	protected ref array<ref DCO_SetLine> m_aLines = {};
	protected ref map<string, int> m_mRowByClass = new map<string, int>();

	protected ref array<int> m_aActKind = {};
	protected ref array<int> m_aActA = {};
	protected ref array<int> m_aActB = {};
	protected ref array<string> m_aActTip = {};

	protected int m_iSel;
	protected int m_iScroll;
	protected int m_iTreeScroll;
	protected int m_iMaxScroll;
	protected int m_iMaxTreeScroll;
	protected int m_iItems;
	protected int m_iTargetKind;
	protected string m_sTarget;
	protected string m_sSearch;
	protected bool m_bOnlyMod;
	protected bool m_bEnded;
	protected bool m_bResetAll;
	protected bool m_bQueued;
	protected bool m_bRendering;
	protected int m_iHover;
	protected int m_iHoverAny;
	protected bool m_bMouseDown;

	protected int m_iPanel;
	protected int m_iPanelScroll;
	protected int m_iPanelMax;
	protected ref array<string> m_aPresetNames = {};
	protected ref array<bool> m_aPresetLocal = {};
	protected int m_iPresetSel = -1;
	protected string m_sPresetName;
	protected string m_sSaveName;
	protected string m_sStatus;
	protected bool m_bPresetListReady;
	protected bool m_bConfirmOverwrite;
	protected bool m_bConfirmDelete;
	protected bool m_bStartup;
	protected static bool s_bStartupPick;
	protected ref array<string> m_aLogLines = {};

	static void DCO_RequestStartupPick()
	{
		s_bStartupPick = true;
	}
	protected bool m_bLogReady;
	protected bool m_bLogTargetOnly;
	protected ref array<string> m_aMemberClasses = {};
	protected ref array<vector> m_aMemberValues = {};

	protected string m_sFaction;
	protected ref array<string> m_aFactionKeys = {};
	protected ref array<string> m_aFactionNames = {};
	protected ref map<string, ref DCO_SetFactionEdit> m_mFaction = new map<string, ref DCO_SetFactionEdit>();
	protected bool m_bGlobalTarget;

	protected ResourceName m_sCellLayout = "{642F3127BF4E4FE7}UI/layouts/DCO/Settings/DCO_SetCell.layout";
	protected ResourceName m_sEditLayout = "{E4DA007EF3A2B24A}UI/layouts/DCO/Settings/DCO_SetEdit.layout";
	protected ResourceName m_sIconSet = "{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset";
	protected ResourceName m_sIconHeavy = "{1D8ACB21AA21D00C}UI/Textures/DCO/Settings/DCO_Set_Heavy.edds";
	protected ResourceName m_sIconDot = "{EA17F55926BCDC08}UI/Textures/DCO/Settings/DCO_Set_Dot.edds";
	protected ResourceName m_sIconRight = "{76B4D8B8243EFC85}UI/Textures/DCO/Settings/DCO_Set_ArrowRight.edds";
	protected ResourceName m_sIconDown = "{9E45FC6991C8C298}UI/Textures/DCO/Settings/DCO_Set_ArrowDown.edds";
	protected ResourceName m_sIconLeft = "{434044053B0B29AF}UI/Textures/DCO/Settings/DCO_Set_ArrowLeft.edds";

	protected float m_fWinW = 1360;
	protected float m_fWinH = 860;
	protected float m_fPad = 28;
	protected float m_fToolY = 62;
	protected float m_fRuleY = 108;
	protected float m_fBodyY = 124;
	protected float m_fBodyH = 562;
	protected float m_fDescY = 694;
	protected float m_fDescH = 110;
	protected float m_fFootY = 804;
	protected float m_fTreeX = 28;
	protected float m_fTreeW = 230;
	protected float m_fSheetX = 282;
	protected float m_fSheetW = 1040;
	protected float m_fLabelX = 0;
	protected float m_fLabelW = 260;
	protected float m_fCellX = 260;
	protected float m_fCellW = 104;
	protected float m_fCellStep = 107;
	protected float m_fResetX = 1010;
	protected float m_fLineH = 46;
	protected float m_fTreeLineH = 36;
	protected float m_fTileH = 38;
	protected int m_iDragRow = -1;
	protected float m_fDragFrom;
	protected float m_fDragValue;
	protected int m_iDragStart;
	protected bool m_bDragUsed;
	protected ref array<int> m_aSliderRow = {};
	protected ref array<Widget> m_aSliderFill = {};
	protected ref array<Widget> m_aSliderText = {};
	protected ref array<Widget> m_aSliderTick = {};
	protected ref array<float> m_aSliderY = {};
	protected ref array<int> m_aTextRow = {};
	protected ref array<EditBoxWidget> m_aTextBox = {};
	protected bool m_bPad;
	protected bool m_bTreeFollow;
	protected int m_iFocusCell;
	protected int m_iDescSkip;
	protected ref array<float> m_aMeterMax;
	protected int m_iFocusRow = -1;
	protected int m_iShownFirst;
	protected int m_iShownLast;
	protected int m_iClickX;
	protected int m_iDescActId;
	protected Widget m_wDescAct;
	protected EditBoxWidget m_wPresetBox;
	protected bool m_bDiscardOpen;
	protected bool m_bConfirmReset;
	protected ref array<Widget> m_aLineFocusW = {};
	protected ref array<Widget> m_aLineStarW = {};
	protected ref array<int> m_aLineRowIdx = {};
	protected ref array<bool> m_aLineFav = {};
	protected int m_iMember;
	protected bool m_bMembersReady;
	protected int m_iSkippedPlayers;
	protected ref array<int> m_aMemberIds = {};
	protected ref array<string> m_aMemberNames = {};
	protected ref array<ref map<string, vector>> m_aMemberVals = {};
	protected ref map<string, vector> m_mGroupVals = new map<string, vector>();
	protected ref map<int, ref DCO_SetFactionEdit> m_mMemberEdits = new map<int, ref DCO_SetFactionEdit>();
	protected RichTextWidget m_wDescText;
	protected int m_iDescKind;
	protected int m_iDescA;
	protected int m_iDescB;
	protected bool m_bSimple;
	protected bool m_bExpert;
	protected bool m_bHelpOpen;
	protected int m_iHelpScroll;
	protected int m_iHelpMax;
	protected float m_fHelpW = 300;
	protected ref array<int> m_aSimpleIdx = {};
	protected int m_iSpRow = -1;
	protected bool m_bSpPicker;
	protected float m_fSpRefreshAt;
	protected string m_sSpLast;
	protected ref array<string> m_aSpCatalog = {};
	protected bool m_bSpBuilder;
	protected string m_sSpBName;
	protected ref array<int> m_aSpBCount = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	protected int m_iSpBVeh = -1;
	protected ref array<string> m_aSpVehicles = {};
	protected int m_iSpBRole;
	protected int m_iSpBExp = 1;
	protected int m_iSpBMor = 1;
	protected bool m_bSpBConfirm;
	protected ref array<string> m_aSpPresetNames = {};
	protected string m_sSpMsg;

	protected ref Color m_cSurface;
	protected ref Color m_cPanelBg;
	protected ref Color m_cPanel;
	protected ref Color m_cRowBg;
	protected ref Color m_cRule;
	protected ref Color m_cHeader;
	protected ref Color m_cCell;
	protected ref Color m_cTileOn;
	protected ref Color m_cTab;
	protected ref Color m_cWarm;
	protected ref Color m_cOrange;
	protected ref Color m_cText;
	protected ref Color m_cInk;
	protected ref Color m_cWhite;
	protected ref Color m_cDim;
	protected ref Color m_cDark;
	protected ref Color m_cAmber;
	protected ref Color m_cDanger;
	protected ref Color m_cClear;
	protected ref Color m_cTipBg;
	protected ref Color m_cTagBg;
	protected ref Color m_cTagFg;
	protected ref Color m_cTrack;
	protected ref Color m_cPendUp;
	protected ref Color m_cPendDown;
	protected ref Color m_cShade;
	protected ref Color m_cPopBg;
	protected ref Color m_cBeta;

	protected string T(string key)
	{
		return DCO_Radio.Resolve(key, 0, null);
	}

	protected string T2(string key, string a, string va, string b = "", string vb = "", string c = "", string vc = "")
	{
		array<string> p = {a, va};
		if (!b.IsEmpty())
		{
			p.Insert(b);
			p.Insert(vb);
		}
		if (!c.IsEmpty())
		{
			p.Insert(c);
			p.Insert(vc);
		}
		return DCO_Radio.Resolve(key, 0, p);
	}

	protected void InitColors()
	{
		m_cSurface = Color.FromSRGBA(36, 41, 45, 255);
		m_cPanelBg = Color.FromSRGBA(38, 43, 47, 255);
		m_cPanel = Color.FromSRGBA(255, 255, 255, 7);
		m_cRowBg = Color.FromSRGBA(255, 255, 255, 7);
		m_cRule = Color.FromSRGBA(255, 255, 255, 33);
		m_cHeader = Color.FromSRGBA(38, 43, 47, 255);
		m_cCell = Color.FromSRGBA(95, 100, 104, 158);
		m_cTileOn = Color.FromSRGBA(122, 90, 66, 235);
		m_cTab = Color.FromSRGBA(31, 35, 38, 255);
		m_cWarm = Color.FromSRGBA(201, 151, 111, 38);
		m_cOrange = Color.FromSRGBA(201, 151, 111, 255);
		m_cText = Color.FromSRGBA(201, 204, 208, 255);
		m_cInk = Color.FromSRGBA(241, 241, 239, 255);
		m_cWhite = Color.FromSRGBA(255, 255, 255, 255);
		m_cDim = Color.FromSRGBA(143, 149, 154, 255);
		m_cDark = Color.FromSRGBA(32, 33, 31, 255);
		m_cAmber = Color.FromSRGBA(255, 201, 138, 255);
		m_cDanger = Color.FromSRGBA(255, 155, 135, 255);
		m_cClear = Color.FromSRGBA(0, 0, 0, 0);
		m_cTipBg = Color.FromSRGBA(20, 23, 25, 245);
		m_cTagBg = Color.FromSRGBA(255, 255, 255, 18);
		m_cTagFg = Color.FromSRGBA(160, 165, 170, 255);
		m_cTrack = Color.FromSRGBA(255, 255, 255, 30);
		m_cPendUp = Color.FromSRGBA(201, 151, 111, 115);
		m_cPendDown = Color.FromSRGBA(255, 255, 255, 90);
		m_cShade = Color.FromSRGBA(10, 12, 13, 140);
		m_cPopBg = Color.FromSRGBA(42, 48, 52, 255);
		m_cBeta = Color.FromSRGBA(140, 190, 235, 255);
	}

	protected int BetaCount(DCO_SetSub sub, out int total)
	{
		int n;
		total = 0;
		foreach (int r : sub.m_aRows)
		{
			if (!m_aRows.IsIndexValid(r) || m_aRows[r].m_bInternal)
				continue;
			total++;
			if (m_aRows[r].m_bBeta)
				n++;
		}
		return n;
	}

	protected bool SubAllBeta(DCO_SetSub sub)
	{
		int total;
		int n = BetaCount(sub, total);
		return total > 0 && n == total;
	}

	protected bool FolderAllBeta(DCO_SetFolder folder)
	{
		bool any;
		foreach (int fs : folder.m_aSubs)
		{
			if (!m_aSubs.IsIndexValid(fs) || !SubAllBeta(m_aSubs[fs]))
				return false;
			any = true;
		}
		return any;
	}

	protected int BetaRows()
	{
		int n;
		foreach (DCO_SetRow row : m_aRows)
		{
			if (row.m_bBeta && !row.m_bInternal)
				n++;
		}
		return n;
	}

	override void OnMenuOpen()
	{
		InitColors();
		m_Manager = SCR_AttributesManagerEditorComponent.DCO_Get();
		Widget root = GetRootWidget();
		if (root)
		{
			m_wWindow = root.FindAnyWidget("Window");
			m_wDyn = root.FindAnyWidget("Dyn");
			m_wFixed = root.FindAnyWidget("Fixed");
			m_wTop = root.FindAnyWidget("Top");
		}
		if (!m_Manager || !m_wWindow || !m_wDyn || !m_wFixed || !m_wTop)
		{
			CloseSelf();
			return;
		}

		m_Input = new DCO_SetInput(this);
		m_wWindow.AddHandler(m_Input);
		m_Manager.GetOnAttributesConfirm().Insert(OnEditingEnded);
		m_Manager.GetOnAttributesCancel().Insert(OnEditingEnded);
		m_Manager.DCO_GetOnEvent().Insert(OnServerEvent);

		DCO_SettingsPrefs.Load();
		BuildModel();
		BuildSimple();
		m_bSimple = DCO_SettingsPrefs.IsSimple() && !m_aSimpleIdx.IsEmpty();
		m_bExpert = DCO_SettingsPrefs.IsExpert();
		m_bHelpOpen = DCO_SettingsPrefs.IsHelpOpen();
		m_bPad = DCO_SettingsChooserUI.PadInput();
		BuildFixed();
		UpdatePlaceholder();
		UpdateDependencies();
		m_aMeterMax = {};
		for (int meterIndex = 0; meterIndex < 3; meterIndex++)
		{
			m_aMeterMax.Insert(MeterMax(meterIndex));
		}
		LogMeters();
		if (m_iTargetKind == 2)
			m_Manager.DCO_RequestMembers();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_open target=" + DCO_SettingsBaseline.LogText(m_sTarget) + " scope=base mode=" + ModeName() +" items=" + m_iItems.ToString() + " w=" + TotalW().ToString() + " h=" + m_fWinH.ToString() + " input=" + InputTag() + " beta=" + BetaRows().ToString());
		if (s_bStartupPick)
		{
			s_bStartupPick = false;
			if (PresetScope() == "Global")
			{
				m_bStartup = true;
				OpenPresets();
				m_sStatus = T("set_startup_prompt");
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "startup_preset_panel scope=Global");
			}
		}
		DoRender();

		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.AddActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
			im.AddActionListener("DCO_SetUp", EActionTrigger.DOWN, OnKeyUp);
			im.AddActionListener("DCO_SetDown", EActionTrigger.DOWN, OnKeyDown);
			im.AddActionListener("DCO_SetLeft", EActionTrigger.DOWN, OnKeyLeft);
			im.AddActionListener("DCO_SetRight", EActionTrigger.DOWN, OnKeyRight);
			im.AddActionListener("DCO_SetEnter", EActionTrigger.DOWN, OnKeyEnter);
			im.AddActionListener("DCO_SetPreset", EActionTrigger.DOWN, OnKeyPreset);
			im.AddActionListener("DCO_SetRevert", EActionTrigger.DOWN, OnKeyRevert);
			im.AddActionListener("DCO_SetFind", EActionTrigger.DOWN, OnKeyFind);
			im.AddActionListener("DCO_SetLog", EActionTrigger.DOWN, OnKeyLog);
			im.AddActionListener("DCO_SetDiscard", EActionTrigger.DOWN, OnKeyDiscard);
			im.AddActionListener("DCO_SetSave", EActionTrigger.DOWN, OnKeySave);
			im.AddActionListener("DCO_SetExport", EActionTrigger.DOWN, OnKeyExport);
			im.AddActionListener("DCO_SetImport", EActionTrigger.DOWN, OnKeyImport);
			im.AddActionListener("DCO_SetSelect", EActionTrigger.DOWN, OnKeySelect);
			im.AddActionListener("DCO_SetRevertRow", EActionTrigger.DOWN, OnKeyRevertRow);
			im.AddActionListener("DCO_SetSectionPrev", EActionTrigger.DOWN, OnKeySectionPrev);
			im.AddActionListener("DCO_SetSectionNext", EActionTrigger.DOWN, OnKeySectionNext);
			im.AddActionListener("DCO_SetBigPrev", EActionTrigger.DOWN, OnKeyBigPrev);
			im.AddActionListener("DCO_SetBigNext", EActionTrigger.DOWN, OnKeyBigNext);
			im.AddActionListener("DCO_SetMode", EActionTrigger.DOWN, OnKeyMode);
			im.AddActionListener("DCO_SetOnlyChanged", EActionTrigger.DOWN, OnKeyOnlyChanged);
			im.AddActionListener("DCO_SetScrollUp", EActionTrigger.DOWN, OnKeyScrollUp);
			im.AddActionListener("DCO_SetScrollDown", EActionTrigger.DOWN, OnKeyScrollDown);
		}
	}

	override void OnMenuUpdate(float tDelta)
	{
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.ActivateContext("DCO_SettingsDialogContext");
		SpRefresh();
		bool pad = DCO_SettingsChooserUI.PadInput();
		if (pad != m_bPad)
		{
			string fromTag = InputTag();
			m_bPad = pad;
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_input from=" + fromTag + " to=" + InputTag());
			UpdatePlaceholder();
			RequestRender();
		}
		bool down = (Debug.GetMouseState(MouseState.LEFT) & Debug.MB_PRESSED_MASK) != 0;
		if (!down && im && im.GetActionValue("MouseLeft") > 0.5)
		{
			down = true;
			DCO_Perf.Count("set:lmb_action");
		}
		bool pressed = down && !m_bMouseDown;
		m_bMouseDown = down;
		if (m_iDragRow < 0)
		{
			if (pressed && m_iHoverAny > 0 && m_iHoverAny < m_aActKind.Count() && m_aActKind[m_iHoverAny] == DCO_ESetAct.SLIDERBAR)
			{
				int hx, hy;
				WidgetManager.GetMousePos(hx, hy);
				DCO_Perf.Count("set:drag_start_poll");
				StartDrag(m_iHoverAny, hx, "poll");
			}
			else if (pressed)
				LogPress("poll", m_iHoverAny, "hover_not_slider");
			return;
		}
		if (!down)
		{
			EndDrag();
			return;
		}
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		m_iClickX = mx;
		DragTo(ClickLocalX());
	}

	protected void LogPress(string src, int id, string result)
	{
		int kind = -1;
		if (id > 0 && id < m_aActKind.Count())
			kind = m_aActKind[id];
		DCO_Perf.Count("set:press_" + result);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("dco_settings_press src=%1 id=%2 kind=%3 result=%4 panel=%5 discard=%6 drag_row=%7 hover=%8", src, id, kind, result, m_iPanel, m_bDiscardOpen, m_iDragRow, m_iHoverAny));
	}

	void StartDrag(int id, int x, string src = "handler")
	{
		string why;
		if (m_iDragRow >= 0)
			why = "already";
		else if (m_bDiscardOpen)
			why = "discard_open";
		else if (m_iPanel != DCO_ESetPanel.NONE)
			why = "panel_open";
		else if (id <= 0 || id >= m_aActKind.Count())
			why = "bad_id";
		else if (m_aActKind[id] != DCO_ESetAct.SLIDERBAR)
			why = "not_slider";
		else if (!m_aRows.IsIndexValid(m_aActA[id]) || m_aRows[m_aActA[id]].m_bDisabled)
			why = "row_disabled";
		if (!why.IsEmpty())
		{
			if (why != "already")
				LogPress(src, id, why);
			return;
		}
		int r = m_aActA[id];
		LogPress(src, id, "start");
		vector v = ViewValue(m_aRows[r]);
		m_bDragUsed = true;
		m_iDragRow = r;
		m_fDragFrom = v[0];
		m_fDragValue = v[0];
		m_iDragStart = System.GetTickCount();
		SetFocusRow(r);
		SetDesc(DCO_ESetAct.ROW, r, 0);
		m_iClickX = x;
		DragTo(ClickLocalX());
	}

	protected void DragTo(float localX)
	{
		if (!m_aRows.IsIndexValid(m_iDragRow))
			return;
		DCO_SetRow row = m_aRows[m_iDragRow];
		float barW = SliderBarW();
		float barX = m_fSheetX + m_fCellX;
		float t = Math.Clamp((localX - barX) / barW, 0, 1);
		m_fDragValue = Quant(row, row.m_fMin + t * (row.m_fMax - row.m_fMin));
		float shown = 0;
		if (row.m_fMax > row.m_fMin)
			shown = Math.Clamp((m_fDragValue - row.m_fMin) / (row.m_fMax - row.m_fMin), 0, 1);
		int i = m_aSliderRow.Find(m_iDragRow);
		if (i < 0)
			return;
		Widget fill = m_aSliderFill[i];
		if (fill)
		{
			FrameSlot.SetSize(fill, Math.Max(1, barW * shown), 14);
			fill.SetVisible(shown > 0);
		}
		Widget tick = m_aSliderTick[i];
		if (tick)
		{
			FrameSlot.SetPos(tick, barX + barW * shown - 1, m_aSliderY[i] - 3);
			tick.SetVisible(true);
		}
		Widget valueW = m_aSliderText[i];
		if (valueW)
		{
			RichTextWidget text = RichTextWidget.Cast(valueW.FindAnyWidget("TextC"));
			if (text)
				text.SetText(SliderText(row, m_fDragValue));
		}
	}

	void EndDrag()
	{
		if (m_iDragRow < 0)
			return;
		int r = m_iDragRow;
		m_iDragRow = -1;
		if (!m_aRows.IsIndexValid(r))
			return;
		DCO_SetRow row = m_aRows[r];
		int ms = System.GetTickCount() - m_iDragStart;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_slider_drag setting=" + row.m_sClass + " from=" + m_fDragFrom.ToString() + " to=" + m_fDragValue.ToString() + " ms=" + ms.ToString());
		SetCur(row, m_fDragValue);
	}

	override void OnMenuClose()
	{
		m_iDragRow = -1;
		GetGame().GetCallqueue().Remove(DoRender);
		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			im.RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnBack);
			im.RemoveActionListener("DCO_SetUp", EActionTrigger.DOWN, OnKeyUp);
			im.RemoveActionListener("DCO_SetDown", EActionTrigger.DOWN, OnKeyDown);
			im.RemoveActionListener("DCO_SetLeft", EActionTrigger.DOWN, OnKeyLeft);
			im.RemoveActionListener("DCO_SetRight", EActionTrigger.DOWN, OnKeyRight);
			im.RemoveActionListener("DCO_SetEnter", EActionTrigger.DOWN, OnKeyEnter);
			im.RemoveActionListener("DCO_SetPreset", EActionTrigger.DOWN, OnKeyPreset);
			im.RemoveActionListener("DCO_SetRevert", EActionTrigger.DOWN, OnKeyRevert);
			im.RemoveActionListener("DCO_SetFind", EActionTrigger.DOWN, OnKeyFind);
			im.RemoveActionListener("DCO_SetLog", EActionTrigger.DOWN, OnKeyLog);
			im.RemoveActionListener("DCO_SetDiscard", EActionTrigger.DOWN, OnKeyDiscard);
			im.RemoveActionListener("DCO_SetSave", EActionTrigger.DOWN, OnKeySave);
			im.RemoveActionListener("DCO_SetExport", EActionTrigger.DOWN, OnKeyExport);
			im.RemoveActionListener("DCO_SetImport", EActionTrigger.DOWN, OnKeyImport);
			im.RemoveActionListener("DCO_SetSelect", EActionTrigger.DOWN, OnKeySelect);
			im.RemoveActionListener("DCO_SetRevertRow", EActionTrigger.DOWN, OnKeyRevertRow);
			im.RemoveActionListener("DCO_SetSectionPrev", EActionTrigger.DOWN, OnKeySectionPrev);
			im.RemoveActionListener("DCO_SetSectionNext", EActionTrigger.DOWN, OnKeySectionNext);
			im.RemoveActionListener("DCO_SetBigPrev", EActionTrigger.DOWN, OnKeyBigPrev);
			im.RemoveActionListener("DCO_SetBigNext", EActionTrigger.DOWN, OnKeyBigNext);
			im.RemoveActionListener("DCO_SetMode", EActionTrigger.DOWN, OnKeyMode);
			im.RemoveActionListener("DCO_SetOnlyChanged", EActionTrigger.DOWN, OnKeyOnlyChanged);
			im.RemoveActionListener("DCO_SetScrollUp", EActionTrigger.DOWN, OnKeyScrollUp);
			im.RemoveActionListener("DCO_SetScrollDown", EActionTrigger.DOWN, OnKeyScrollDown);
		}
		if (m_Manager)
		{
			m_Manager.GetOnAttributesConfirm().Remove(OnEditingEnded);
			m_Manager.GetOnAttributesCancel().Remove(OnEditingEnded);
			m_Manager.DCO_GetOnEvent().Remove(OnServerEvent);
			if (!m_bEnded && m_Manager.DCO_IsMode())
				m_Manager.CancelEditing();
		}
		DCO_SettingsPrefs.Save();
	}

	protected void OnEditingEnded()
	{
		m_bEnded = true;
		CloseSelf();
	}

	protected void OnBack()
	{
		if (m_iDragRow >= 0)
		{
			EndDrag();
			return;
		}
		if (IsTyping())
		{
			GetGame().GetWorkspace().SetFocusedWidget(null);
			return;
		}
		if (m_bDiscardOpen)
		{
			m_bDiscardOpen = false;
			RequestRender();
			return;
		}
		if (m_iPanel != DCO_ESetPanel.NONE)
		{
			ClosePanel();
			return;
		}
		if (m_bPad && m_bHelpOpen)
		{
			m_bHelpOpen = false;
			m_iHelpScroll = 0;
			DCO_SettingsPrefs.SetHelpOpen(false);
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_help open=false input=pad");
			RequestRender();
			return;
		}
		RequestClose();
	}

	protected string InputTag()
	{
		if (m_bPad)
			return "pad";
		return "kbm";
	}

	protected void UpdatePlaceholder()
	{
		if (!m_wSearch)
			return;
		if (m_bPad)
			m_wSearch.SetPlaceholderText(T("set_search_pad"));
		else
			m_wSearch.SetPlaceholderText(T("set_search"));
	}

	protected void LogRevert(string scope, string setting)
	{
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_revert scope=" + scope + " setting=" + DCO_SettingsBaseline.LogText(setting) + " input=" + InputTag());
	}

	protected void ShowLine(int li)
	{
		if (li < m_iShownFirst)
			m_iScroll = li;
		else if (li > m_iShownLast)
			m_iScroll = Math.ClampInt(m_iScroll + li - m_iShownLast, 0, m_iMaxScroll);
	}

	protected void OnKeySelect()
	{
		if (IsTyping())
			return;
		if (m_bDiscardOpen)
		{
			DiscardApply();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.REVIEW)
		{
			DoApply();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			LoadSelectedPreset();
			return;
		}
		if (m_iPanel != DCO_ESetPanel.NONE || !m_aRows.IsIndexValid(m_iFocusRow))
			return;
		DCO_SetRow row = m_aRows[m_iFocusRow];
		if (row.m_bDisabled)
			return;
		if (row.m_iKind == DCO_ESetKind.CHECK)
		{
			KeyChange(1);
			return;
		}
		if (row.m_iKind == DCO_ESetKind.MULTI && row.m_aValues.IsIndexValid(m_iFocusCell))
		{
			m_iDescKind = DCO_ESetAct.CELL;
			m_iDescA = m_iFocusRow;
			m_iDescB = m_iFocusCell;
			OnCell(row, m_iFocusCell);
			return;
		}
		if (row.m_iKind == DCO_ESetKind.TEXT)
			EditText(m_iFocusRow);
	}

	protected void EditText(int rowIndex)
	{
		int at = m_aTextRow.Find(rowIndex);
		if (at < 0 || !m_aTextBox[at])
			return;
		GetGame().GetWorkspace().SetFocusedWidget(m_aTextBox[at]);
		m_aTextBox[at].ActivateWriteMode();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_text_edit setting=" + m_aRows[rowIndex].m_sClass + " input=" + InputTag());
	}

	protected void OnKeyRevertRow()
	{
		if (IsTyping())
			return;
		if (m_bDiscardOpen)
		{
			DiscardClose();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			SavePreset();
			return;
		}
		if (m_iPanel != DCO_ESetPanel.NONE)
			return;
		if (m_iFocusRow <= -2)
		{
			DCO_SimpleSetting setting = SimpleAt(-2 - m_iFocusRow);
			if (!setting || !SimplePending(setting))
				return;
			array<int> simpleRows = {};
			SimpleRows(setting, simpleRows);
			foreach (int sr : simpleRows)
			{
				RevertRow(m_aRows[sr]);
			}
			LogRevert("simple", setting.m_sName);
		}
		else
		{
			if (!m_aRows.IsIndexValid(m_iFocusRow) || !IsPending(m_aRows[m_iFocusRow]))
				return;
			RevertRow(m_aRows[m_iFocusRow]);
			LogRevert("row", m_aRows[m_iFocusRow].m_sClass);
		}
		UpdateDependencies();
		RequestRender();
	}

	protected void OnKeySectionPrev()
	{
		if (!KeysBlocked())
			SectionStep(-1);
	}

	protected void OnKeySectionNext()
	{
		if (!KeysBlocked())
			SectionStep(1);
	}

	protected void SectionStep(int dir)
	{
		if (m_iPanel != DCO_ESetPanel.NONE || SimpleView())
			return;
		array<int> order = {-1};
		foreach (DCO_SetFolder folder : m_aFolders)
		{
			if (!FolderVisible(folder))
				continue;
			foreach (int s : folder.m_aSubs)
			{
				order.Insert(s);
			}
		}
		int at = order.Find(m_iSel);
		if (at < 0)
			at = 0;
		at = (at + dir + order.Count()) % order.Count();
		int next = order[at];
		foreach (DCO_SetFolder owner : m_aFolders)
		{
			if (owner.m_aSubs.Find(next) >= 0 && DCO_SettingsPrefs.IsClosed(owner.m_sKey))
				DCO_SettingsPrefs.ToggleClosed(owner.m_sKey);
		}
		m_iSel = next;
		m_iScroll = 0;
		m_iFocusRow = -1;
		m_iDescKind = DCO_ESetAct.NONE;
		m_bOnlyMod = false;
		m_bTreeFollow = true;
		ClearSearch();
		RequestRender();
	}

	protected void OnKeyBigPrev()
	{
		if (KeysBlocked())
			return;
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			DeletePreset();
			return;
		}
		KeyChange(-1, true);
	}

	protected void OnKeyBigNext()
	{
		if (KeysBlocked())
			return;
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			if (LocalPresetsSeparate())
				SaveLocalPreset();
			return;
		}
		KeyChange(1, true);
	}

	protected void OnKeyMode()
	{
		if (KeysBlocked() || m_iPanel != DCO_ESetPanel.NONE)
			return;
		int next = 2;
		if (m_bExpert)
			next = 1;
		else if (!m_bSimple)
			next = 3;
		if (next == 1 && m_aSimpleIdx.IsEmpty())
			next = 2;
		SetMode(next);
	}

	protected int CurMode()
	{
		if (m_bSimple)
			return 1;
		if (m_bExpert)
			return 3;
		return 2;
	}

	protected string ModeName()
	{
		if (m_bSimple)
			return "simple";
		if (m_bExpert)
			return "expert";
		return "advanced";
	}

	protected void SetMode(int mode)
	{
		if (mode == 1 && m_aSimpleIdx.IsEmpty())
			mode = 2;
		if (mode == CurMode())
			return;
		m_bSimple = mode == 1;
		m_bExpert = mode == 3;
		m_iScroll = 0;
		m_iFocusRow = -1;
		m_iDescKind = DCO_ESetAct.NONE;
		DCO_SettingsPrefs.SetMode(mode);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_mode mode=" + ModeName() + " input=" + InputTag());
		RequestRender();
	}

	protected void OnKeyOnlyChanged()
	{
		if (KeysBlocked() || m_iPanel != DCO_ESetPanel.NONE)
			return;
		m_bOnlyMod = !m_bOnlyMod;
		m_iScroll = 0;
		m_iFocusRow = -1;
		m_iDescKind = DCO_ESetAct.NONE;
		RequestRender();
	}

	protected void OnKeyScrollUp()
	{
		ScrollStick(-1);
	}

	protected void OnKeyScrollDown()
	{
		ScrollStick(1);
	}

	protected void ScrollStick(int dir)
	{
		if (IsTyping() || m_bDiscardOpen)
			return;
		if (m_iPanel != DCO_ESetPanel.NONE)
		{
			int panel = Math.ClampInt(m_iPanelScroll + dir, 0, m_iPanelMax);
			if (panel == m_iPanelScroll)
				return;
			m_iPanelScroll = panel;
			RequestRender();
			return;
		}
		if (m_bHelpOpen)
		{
			int help = Math.ClampInt(m_iHelpScroll + dir, 0, m_iHelpMax);
			if (help == m_iHelpScroll)
				return;
			m_iHelpScroll = help;
			RequestRender();
			return;
		}
		m_iDescSkip = Math.Max(0, m_iDescSkip + dir);
		if (m_wDescText)
			m_wDescText.SetText(SkipParas(DescText(m_iDescKind, m_iDescA, m_iDescB)));
	}

	protected string SkipParas(string text)
	{
		if (m_iDescSkip <= 0)
			return text;
		array<string> parts = {};
		text.Split("\n", parts, false);
		if (m_iDescSkip > parts.Count() - 1)
			m_iDescSkip = Math.Max(0, parts.Count() - 1);
		string outText;
		for (int i = m_iDescSkip; i < parts.Count(); i++)
		{
			if (!outText.IsEmpty())
				outText += "\n";
			outText += parts[i];
		}
		return outText;
	}

	protected void ResetAllDefaults()
	{
		if (!m_bConfirmReset)
		{
			m_bConfirmReset = true;
			RequestRender();
			return;
		}
		m_bConfirmReset = false;
		if (FactionMode())
		{
			DCO_SetFactionEdit resetEdit = Fac();
			if (resetEdit)
				resetEdit.m_mCur.Clear();
		}
		else
		{
			foreach (DCO_SetRow row : m_aRows)
			{
				if (!row.m_bHasDef || row.m_iKind == DCO_ESetKind.TEXT)
					continue;
				if (!Same(row.m_vCur, row.m_vDef))
				{
					row.m_vCur = row.m_vDef;
					row.m_bTouched = true;
				}
			}
			m_bResetAll = true;
		}
		LogRevert("defaults", "-");
		UpdateDependencies();
		ClosePanel();
	}

	protected void RequestClose()
	{
		if (TotalPending() > 0)
		{
			m_bDiscardOpen = true;
			RequestRender();
			return;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_close pending=0 action=none");
		CloseSelf();
	}

	void HotkeyClose()
	{
		if (IsTyping())
			return;
		if (m_bDiscardOpen)
		{
			m_bDiscardOpen = false;
			RequestRender();
			return;
		}
		RequestClose();
	}

	void OnClickAt(int id, int x)
	{
		m_iClickX = x;
		OnAct(id);
	}

	protected float ClickLocalX()
	{
		float wx, wy;
		m_wWindow.GetScreenPos(wx, wy);
		return GetGame().GetWorkspace().DPIUnscale(m_iClickX - wx);
	}

	protected bool KeysBlocked()
	{
		return IsTyping() || m_bDiscardOpen;
	}

	protected void FocusKeys(notnull array<int> outKeys)
	{
		foreach (DCO_SetLine line : m_aLines)
		{
			if (line.m_iType == 1 && line.m_iPart == 0)
				outKeys.Insert(line.m_iRow);
			else if (line.m_iType == 3)
				outKeys.Insert(-2 - line.m_iRow);
		}
	}

	protected void MoveFocus(int dir)
	{
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			if (m_aPresetNames.IsEmpty())
				return;
			m_iPresetSel = Math.ClampInt(m_iPresetSel + dir, 0, m_aPresetNames.Count() - 1);
			m_sSaveName = m_aPresetNames[m_iPresetSel];
			if (m_iPresetSel < m_iPanelScroll)
				m_iPanelScroll = m_iPresetSel;
			RequestRender();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.LOG || m_iPanel == DCO_ESetPanel.REVIEW)
		{
			ScrollStick(dir);
			return;
		}
		if (m_iPanel != DCO_ESetPanel.NONE)
			return;
		array<int> keys = {};
		FocusKeys(keys);
		if (keys.IsEmpty())
			return;
		int at = keys.Find(m_iFocusRow);
		if (at < 0)
			at = 0;
		else
			at = Math.ClampInt(at + dir, 0, keys.Count() - 1);
		m_iFocusRow = keys[at];
		m_iFocusCell = 0;
		m_iDescSkip = 0;
		int firstLine = -1;
		int lastLine = -1;
		foreach (int li, DCO_SetLine line : m_aLines)
		{
			bool match = (line.m_iType == 1 && line.m_iRow == m_iFocusRow) || (line.m_iType == 3 && -2 - line.m_iRow == m_iFocusRow);
			if (!match)
				continue;
			if (firstLine < 0)
				firstLine = li;
			lastLine = li;
		}
		if (lastLine >= 0 && dir > 0)
			ShowLine(lastLine);
		if (firstLine >= 0)
		{
			if (dir > 0 && firstLine < m_iScroll)
				m_iScroll = firstLine;
			else if (dir <= 0)
				ShowLine(firstLine);
		}
		m_iDescKind = DCO_ESetAct.NONE;
		if (m_aRows.IsIndexValid(m_iFocusRow) && m_aRows[m_iFocusRow].m_iKind == DCO_ESetKind.MULTI)
		{
			m_iDescKind = DCO_ESetAct.CELL;
			m_iDescA = m_iFocusRow;
			m_iDescB = 0;
		}
		RequestRender();
	}

	protected void KeyChange(int dir, bool big = false)
	{
		if (m_iPanel != DCO_ESetPanel.NONE)
			return;
		if (m_iFocusRow <= -2)
		{
			if (big)
				return;
			DCO_SimpleSetting setting = SimpleAt(-2 - m_iFocusRow);
			if (!setting || setting.m_aLevels.IsEmpty())
				return;
			int level = SimpleLevel(setting);
			if (level < 0)
				level = setting.m_aLevels.Count() / 2;
			else
				level = Math.ClampInt(level + dir, 0, setting.m_aLevels.Count() - 1);
			ApplySimpleLevel(-2 - m_iFocusRow, level);
			return;
		}
		if (!m_aRows.IsIndexValid(m_iFocusRow))
			return;
		DCO_SetRow row = m_aRows[m_iFocusRow];
		if (row.m_bDisabled || (big && row.m_iKind != DCO_ESetKind.SLIDER))
			return;
		vector v = ViewValue(row);
		switch (row.m_iKind)
		{
			case DCO_ESetKind.MULTI:
			{
				if (row.m_aValues.IsEmpty())
					break;
				m_iFocusCell = Math.ClampInt(m_iFocusCell + dir, 0, row.m_aValues.Count() - 1);
				m_iDescKind = DCO_ESetAct.CELL;
				m_iDescA = m_iFocusRow;
				m_iDescB = m_iFocusCell;
				m_iDescSkip = 0;
				foreach (int li, DCO_SetLine line : m_aLines)
				{
					if (line.m_iType == 1 && line.m_iRow == m_iFocusRow && line.m_iPart == m_iFocusCell / PerLine(row))
						ShowLine(li);
				}
				RequestRender();
				break;
			}
			case DCO_ESetKind.CHECK:
			{
				float toggled = 1;
				if (v[0] != 0)
					toggled = 0;
				SetCur(row, toggled);
				break;
			}
			case DCO_ESetKind.DROP:
			{
				if (row.m_aValues.IsEmpty())
					break;
				int pos = 0;
				foreach (int e, float value : row.m_aValues)
				{
					if (Math.AbsFloat(value - v[0]) < 0.0001)
						pos = e;
				}
				pos = (pos + dir + row.m_aValues.Count()) % row.m_aValues.Count();
				SetCur(row, row.m_aValues[pos]);
				break;
			}
			case DCO_ESetKind.SLIDER:
			{
				float step = row.m_fStep;
				if (step <= 0)
					step = (row.m_fMax - row.m_fMin) * 0.01;
				if ((row.m_fMax - row.m_fMin) / step > 100)
					step = step * 10;
				if (big || Debug.KeyState(KeyCode.KC_LSHIFT) || Debug.KeyState(KeyCode.KC_RSHIFT))
					step = step * 10;
				float start = v[0];
				if (ShowsMixed(row))
					start = row.m_fMin;
				SetCur(row, Quant(row, start + 1.0 * dir * step));
				break;
			}
		}
	}

	protected void OnKeyUp()
	{
		if (!KeysBlocked())
			MoveFocus(-1);
	}

	protected void OnKeyDown()
	{
		if (!KeysBlocked())
			MoveFocus(1);
	}

	protected void OnKeyLeft()
	{
		if (!KeysBlocked())
			KeyChange(-1);
	}

	protected void OnKeyRight()
	{
		if (!KeysBlocked())
			KeyChange(1);
	}

	protected void OnKeyEnter()
	{
		if (IsTyping())
			return;
		if (m_bDiscardOpen)
		{
			DiscardApply();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.REVIEW)
		{
			DoApply();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			LoadSelectedPreset();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.NONE && TotalPending() > 0)
		{
			m_iPanel = DCO_ESetPanel.REVIEW;
			m_iPanelScroll = 0;
			RequestRender();
		}
	}

	protected void OnKeyPreset()
	{
		if (KeysBlocked())
			return;
		if (m_iPanel == DCO_ESetPanel.PRESETS)
			ClosePanel();
		else
			OpenPresets();
	}

	protected void OnKeyRevert()
	{
		if (KeysBlocked())
			return;
		if (m_iPanel == DCO_ESetPanel.PRESETS)
		{
			ResetAllDefaults();
			return;
		}
		if (m_iPanel == DCO_ESetPanel.NONE && TotalPending() > 0)
			RevertAll();
	}

	protected void OnKeyFind()
	{
		if (KeysBlocked())
			return;
		if (m_iPanel == DCO_ESetPanel.PRESETS && m_wPresetBox)
		{
			GetGame().GetWorkspace().SetFocusedWidget(m_wPresetBox);
			m_wPresetBox.ActivateWriteMode();
			return;
		}
		if (!m_wSearch)
			return;
		if (m_iPanel != DCO_ESetPanel.NONE)
			ClosePanel();
		GetGame().GetWorkspace().SetFocusedWidget(m_wSearch);
		m_wSearch.ActivateWriteMode();
	}

	protected void OnKeyLog()
	{
		if (KeysBlocked())
			return;
		m_iPanel = DCO_ESetPanel.LOG;
		m_iPanelScroll = 0;
		RequestLog();
		RequestRender();
	}

	protected void OnKeyDiscard()
	{
		if (!IsTyping() && m_bDiscardOpen)
			DiscardClose();
	}

	protected void OnKeySave()
	{
		if (!KeysBlocked() && m_iPanel == DCO_ESetPanel.PRESETS)
			SavePreset();
	}

	protected void OnKeyExport()
	{
		if (!KeysBlocked() && m_iPanel == DCO_ESetPanel.PRESETS)
			ExportText();
	}

	protected void OnKeyImport()
	{
		if (!KeysBlocked() && m_iPanel == DCO_ESetPanel.PRESETS)
			ImportText();
	}

	protected void DiscardClose()
	{
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_close pending=" + TotalPending().ToString() + " action=discard");
		m_bDiscardOpen = false;
		CloseSelf();
	}

	protected void DiscardApply()
	{
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_close pending=" + TotalPending().ToString() + " action=apply");
		m_bDiscardOpen = false;
		DoApply();
	}

	protected void RevertRow(DCO_SetRow row)
	{
		if (FactionMode())
		{
			DCO_SetFactionEdit edit = Fac();
			if (!edit)
				return;
			vector orig;
			if (edit.m_mOrig.Find(row.m_sClass, orig))
				edit.m_mCur.Set(row.m_sClass, orig);
			else
				edit.m_mCur.Remove(row.m_sClass);
			return;
		}
		if (MemberScope() && row.m_bMember)
		{
			DCO_SetFactionEdit memberEdit = MemEdit();
			vector memberOrig;
			if (memberEdit && memberEdit.m_mOrig.Find(row.m_sClass, memberOrig))
				memberEdit.m_mCur.Set(row.m_sClass, memberOrig);
			return;
		}
		row.m_vCur = row.m_vOrig;
		row.m_sText = row.m_sTextOrig;
		row.m_bTouched = false;
	}

	protected void RevertAll()
	{
		LogRevert("all", "-");
		foreach (DCO_SetRow row : m_aRows)
		{
			row.m_vCur = row.m_vOrig;
			row.m_sText = row.m_sTextOrig;
			row.m_bTouched = false;
		}
		foreach (string key, DCO_SetFactionEdit edit : m_mFaction)
		{
			if (!edit)
				continue;
			edit.m_mCur.Clear();
			foreach (string className, vector value : edit.m_mOrig)
			{
				edit.m_mCur.Set(className, value);
			}
		}
		foreach (int memberIndex, DCO_SetFactionEdit memberEdit : m_mMemberEdits)
		{
			if (!memberEdit)
				continue;
			memberEdit.m_mCur.Clear();
			foreach (string memberClass, vector memberValue : memberEdit.m_mOrig)
			{
				memberEdit.m_mCur.Set(memberClass, memberValue);
			}
		}
		m_aMemberClasses.Clear();
		m_aMemberValues.Clear();
		m_bResetAll = false;
		UpdateDependencies();
		RequestRender();
	}

	protected bool MemberScope()
	{
		return m_iMember > 0 && m_aMemberIds.IsIndexValid(m_iMember - 1);
	}

	protected DCO_SetFactionEdit MemEdit()
	{
		if (!MemberScope())
			return null;
		DCO_SetFactionEdit edit;
		if (m_mMemberEdits.Find(m_iMember - 1, edit))
			return edit;
		return null;
	}

	protected bool GroupValue(DCO_SetRow row, out vector value)
	{
		if (m_mGroupVals.Find(row.m_sClass, value))
			return true;
		if (row.m_bConflict)
			return false;
		value = row.m_vCur;
		return true;
	}

	protected bool MemberHasOwn(int memberIndex)
	{
		if (!m_aMemberVals.IsIndexValid(memberIndex))
			return false;
		map<string, vector> vals = m_aMemberVals[memberIndex];
		DCO_SetFactionEdit edit;
		m_mMemberEdits.Find(memberIndex, edit);
		foreach (DCO_SetRow row : m_aRows)
		{
			if (!row.m_bMember)
				continue;
			vector groupValue;
			if (!GroupValue(row, groupValue))
				continue;
			vector v;
			bool has = vals.Find(row.m_sClass, v);
			if (edit && edit.m_mCur.Find(row.m_sClass, v))
				has = true;
			if (has && !Same(v, groupValue))
				return true;
		}
		return false;
	}

	protected void SelectMember(int index)
	{
		if (index < 0 || index > m_aMemberIds.Count() || index == m_iMember || m_iItems > 1)
			return;
		m_iMember = index;
		if (MemberScope())
		{
			DCO_SetFactionEdit edit;
			if (!m_mMemberEdits.Find(m_iMember - 1, edit) || !edit)
			{
				edit = new DCO_SetFactionEdit();
				edit.m_sName = m_aMemberNames[m_iMember - 1];
				map<string, vector> vals = m_aMemberVals[m_iMember - 1];
				foreach (string className, vector value : vals)
				{
					edit.m_mOrig.Set(className, value);
					edit.m_mCur.Set(className, value);
				}
				edit.m_bLoaded = true;
				m_mMemberEdits.Set(m_iMember - 1, edit);
			}
		}
		m_iScroll = 0;
		m_iFocusRow = -1;
		m_iDescKind = DCO_ESetAct.NONE;
		FixSelection();
		UpdateDependencies();
		RequestRender();
	}

	protected void OnMembers(string a, string b)
	{
		m_aMemberIds.Clear();
		m_aMemberNames.Clear();
		m_aMemberVals.Clear();
		m_mGroupVals.Clear();
		array<string> entries = {};
		a.Split(";", entries, true);
		foreach (string entry : entries)
		{
			array<string> f = {};
			entry.Split("|", f, false);
			if (f.Count() < 2)
				continue;
			if (f[0] == "S")
			{
				m_iSkippedPlayers = f[1].ToInt();
				continue;
			}
			m_aMemberIds.Insert(f[0].ToInt());
			m_aMemberNames.Insert(f[1]);
		}
		array<string> lines = {};
		b.Split("\n", lines, false);
		if (!lines.IsEmpty())
		{
			array<string> groupKeys = {};
			array<vector> groupValues = {};
			DCO_SettingsCodec.Decode(lines[0], groupKeys, groupValues);
			foreach (int gi, string groupClass : groupKeys)
			{
				m_mGroupVals.Set(groupClass, groupValues[gi]);
			}
		}
		foreach (int mi, int memberId : m_aMemberIds)
		{
			map<string, vector> vals = new map<string, vector>();
			if (lines.IsIndexValid(mi + 1))
			{
				array<string> keys = {};
				array<vector> values = {};
				DCO_SettingsCodec.Decode(lines[mi + 1], keys, values);
				foreach (int ki, string className : keys)
				{
					vals.Set(className, values[ki]);
				}
			}
			m_aMemberVals.Insert(vals);
		}
		BuildMemberRows();
		m_bMembersReady = true;
	}

	protected void BuildMemberRows()
	{
		if (m_aMemberVals.IsEmpty())
			return;
		array<SCR_BaseEditorAttribute> attributes = {};
		m_Manager.DCO_AttributesOfTarget(3, attributes);
		DCO_SettingsTreeConfig cfg = DCO_SettingsTree.Get();
		map<int, int> subByKey = new map<int, int>();
		array<int> keys = {};
		array<int> newRows = {};
		int converted = 0;
		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			string className = attribute.Type().ToString();
			int existingIndex;
			bool exists = m_mRowByClass.Find(className, existingIndex);
			if (exists && (m_aRows[existingIndex].m_bMember || m_aRows[existingIndex].m_iKind == DCO_ESetKind.TEXT))
				continue;
			int key = DCO_SettingsTree.FindSub(attribute.GetCategoryConfig());
			if (key < 0)
				continue;
			bool any = false;
			bool conflict = false;
			vector first;
			foreach (map<string, vector> vals : m_aMemberVals)
			{
				vector v;
				if (!vals.Find(className, v))
					continue;
				if (!any)
				{
					first = v;
					any = true;
				}
				else if (!Same(first, v))
				{
					conflict = true;
				}
			}
			if (!any)
				continue;
			if (exists)
			{
				DCO_SetRow existingRow = m_aRows[existingIndex];
				existingRow.m_bMember = true;
				existingRow.m_vOrig = first;
				if (!existingRow.m_bTouched)
					existingRow.m_vCur = first;
				existingRow.m_bConflict = conflict;
				if (m_aSubs.IsIndexValid(existingRow.m_iSub))
					m_aFolders[m_aSubs[existingRow.m_iSub].m_iFolder].m_bMember = true;
				converted++;
				continue;
			}
			DCO_SetRow row = MakeRow(attribute);
			if (row.m_iKind == DCO_ESetKind.TEXT)
				continue;
			row.m_bMember = true;
			row.m_vOrig = first;
			row.m_vCur = first;
			row.m_vDef = first;
			row.m_bHasDef = false;
			row.m_bConflict = conflict;
			row.m_iSub = key;
			if (!subByKey.Contains(key))
			{
				subByKey.Set(key, -1);
				keys.Insert(key);
			}
			m_mRowByClass.Set(className, m_aRows.Count());
			newRows.Insert(m_aRows.Count());
			m_aRows.Insert(row);
		}
		keys.Sort();
		map<int, int> folderByIndex = new map<int, int>();
		foreach (int sortedKey : keys)
		{
			int fi = sortedKey / 1000;
			int si = sortedKey % 1000;
			DCO_SettingsFolderDef fdef = cfg.m_aFolders[fi];
			int folder;
			if (!folderByIndex.Find(fi, folder))
			{
				DCO_SetFolder f = new DCO_SetFolder();
				f.m_sName = fdef.m_sName;
				f.m_sDesc = fdef.m_sDescription;
				f.m_bWorld = fdef.m_bWorldWide;
				f.m_bMember = true;
				f.m_iTarget = fdef.m_iTarget;
				f.m_sKey = "member:" + fdef.m_sName;
				folder = m_aFolders.Count();
				m_aFolders.Insert(f);
				folderByIndex.Set(fi, folder);
			}
			DCO_SetSub sub = new DCO_SetSub();
			sub.m_iFolder = folder;
			sub.m_sDesc = fdef.m_aSubs[si].m_sDescription;
			sub.m_sName = CategoryName(fdef.m_aSubs[si].m_Category);
			sub.m_Category = fdef.m_aSubs[si].m_Category;
			sub.m_sShort = sub.m_sName;
			int colon = sub.m_sName.IndexOf(": ");
			if (colon >= 0)
				sub.m_sShort = sub.m_sName.Substring(colon + 2, sub.m_sName.Length() - colon - 2);
			subByKey.Set(sortedKey, m_aSubs.Count());
			m_aFolders[folder].m_aSubs.Insert(m_aSubs.Count());
			m_aSubs.Insert(sub);
		}
		foreach (int r : newRows)
		{
			DCO_SetRow newRow = m_aRows[r];
			int subIndex = subByKey.Get(newRow.m_iSub);
			newRow.m_iSub = subIndex;
			m_aSubs[subIndex].m_aRows.Insert(r);
		}
		if (m_iSel < 0 && !m_aSubs.IsEmpty())
			m_iSel = 0;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_member_rows members=" + m_aMemberVals.Count().ToString() + " new=" + newRows.Count().ToString() + " converted=" + converted.ToString());
		UpdateDependencies();
	}

	protected void ClosePanel()
	{
		if (m_bStartup)
		{
			m_bStartup = false;
			m_sStatus = string.Empty;
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "startup_preset_skip reason=user_back result=server_config");
			GetGame().GetCallqueue().CallLater(RequestClose, 0);
		}
		m_bConfirmReset = false;
		m_iPanel = DCO_ESetPanel.NONE;
		m_iPanelScroll = 0;
		m_bConfirmOverwrite = false;
		m_bConfirmDelete = false;
		RequestRender();
	}

	protected void BuildModel()
	{
		array<SCR_BaseEditorAttribute> attributes = {};
		m_Manager.GetEditedAttributes(attributes);
		array<Managed> items = {};
		m_Manager.GetEditedItems(items);
		m_iItems = CountTargets(items);

		DCO_SettingsTreeConfig cfg = DCO_SettingsTree.Get();
		map<int, int> subByKey = new map<int, int>();
		array<int> keys = {};
		array<int> targetCount = {0, 0, 0, 0, 0};

		foreach (SCR_BaseEditorAttribute attribute : attributes)
		{
			int key = DCO_SettingsTree.FindSub(attribute.GetCategoryConfig());
			if (key < 0)
				continue;
			string className = attribute.Type().ToString();
			if (m_mRowByClass.Contains(className))
				continue;
			if (!subByKey.Contains(key))
			{
				subByKey.Set(key, -1);
				keys.Insert(key);
			}
			DCO_SetRow row = MakeRow(attribute);
			row.m_iSub = key;
			m_mRowByClass.Set(className, m_aRows.Count());
			m_aRows.Insert(row);
			int target = cfg.m_aFolders[key / 1000].m_iTarget;
			if (target >= 0 && target < targetCount.Count())
				targetCount[target] = targetCount[target] + 1;
		}

		keys.Sort();
		map<int, int> folderByIndex = new map<int, int>();
		foreach (int key : keys)
		{
			int fi = key / 1000;
			int si = key % 1000;
			DCO_SettingsFolderDef fdef = cfg.m_aFolders[fi];
			int folder;
			if (!folderByIndex.Find(fi, folder))
			{
				DCO_SetFolder f = new DCO_SetFolder();
				f.m_sName = fdef.m_sName;
				f.m_sDesc = fdef.m_sDescription;
				f.m_bWorld = fdef.m_bWorldWide;
				f.m_iTarget = fdef.m_iTarget;
				f.m_sKey = fdef.m_iTarget.ToString() + ":" + fdef.m_sName;
				folder = m_aFolders.Count();
				m_aFolders.Insert(f);
				folderByIndex.Set(fi, folder);
			}
			DCO_SetSub sub = new DCO_SetSub();
			sub.m_iFolder = folder;
			sub.m_sDesc = fdef.m_aSubs[si].m_sDescription;
			sub.m_sName = CategoryName(fdef.m_aSubs[si].m_Category);
			sub.m_Category = fdef.m_aSubs[si].m_Category;
			sub.m_sShort = sub.m_sName;
			int colon = sub.m_sName.IndexOf(": ");
			if (colon >= 0)
				sub.m_sShort = sub.m_sName.Substring(colon + 2, sub.m_sName.Length() - colon - 2);
			subByKey.Set(key, m_aSubs.Count());
			m_aFolders[folder].m_aSubs.Insert(m_aSubs.Count());
			m_aSubs.Insert(sub);
		}

		foreach (int i, DCO_SetRow row : m_aRows)
		{
			int subIndex = subByKey.Get(row.m_iSub);
			row.m_iSub = subIndex;
			if (!row.m_bInternal)
				m_aSubs[subIndex].m_aRows.Insert(i);
		}

		m_iTargetKind = 0;
		int best = -1;
		foreach (int t, int n : targetCount)
		{
			if (n > best)
			{
				best = n;
				m_iTargetKind = t;
			}
		}

		if (m_iItems >= 1)
			m_sTarget = TargetName(items[0]);
		else
			m_sTarget = T("set_global");

		m_bGlobalTarget = m_iItems == 1 && BaseGameMode.Cast(items[0]) != null;
		if (m_bGlobalTarget)
		{
			FactionManager fm = GetGame().GetFactionManager();
			if (fm)
			{
				array<Faction> factions = {};
				fm.GetFactionsList(factions);
				foreach (Faction faction : factions)
				{
					if (!faction || m_aFactionKeys.Count() >= 5)
						continue;
					string factionKey = faction.GetFactionKey();
					string factionName = factionKey;
					SCR_Faction scrFaction = SCR_Faction.Cast(faction);
					if (scrFaction && !scrFaction.GetFactionName().IsEmpty())
						factionName = WidgetManager.Translate(scrFaction.GetFactionName());
					m_aFactionKeys.Insert(factionKey);
					m_aFactionNames.Insert(factionName);
				}
			}
		}

		m_iSel = 0;
		if (m_aSubs.IsEmpty())
			m_iSel = -1;
	}

	protected int CountTargets(array<Managed> items)
	{
		array<AIGroup> groups = {};
		foreach (Managed gi : items)
		{
			SCR_EditableEntityComponent ge = SCR_EditableEntityComponent.Cast(gi);
			if (ge && AIGroup.Cast(ge.GetOwner()))
				groups.Insert(AIGroup.Cast(ge.GetOwner()));
		}
		int n = 0;
		foreach (Managed item : items)
		{
			SCR_EditableEntityComponent e = SCR_EditableEntityComponent.Cast(item);
			if (e && e.GetOwner() && !groups.IsEmpty() && !AIGroup.Cast(e.GetOwner()))
			{
				AIControlComponent ctrl = AIControlComponent.Cast(e.GetOwner().FindComponent(AIControlComponent));
				if (ctrl && ctrl.GetControlAIAgent() && groups.Contains(ctrl.GetControlAIAgent().GetParentGroup()))
					continue;
			}
			n++;
		}
		if (n != items.Count())
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_targets items=" + items.Count().ToString() + " targets=" + n.ToString() + " reason=members_of_selected_group");
		return n;
	}

	protected string TargetName(Managed item)
	{
		if (BaseGameMode.Cast(item))
			return T("set_global");
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return "?";
		string name = WidgetManager.Translate(editable.GetDisplayName());
		if (name.IsEmpty() && editable.GetOwner())
			name = editable.GetOwner().GetName();
		return name;
	}

	protected string CategoryName(ResourceName category)
	{
		Resource res = BaseContainerTools.LoadContainer(category);
		if (res && res.IsValid())
		{
			SCR_EditorAttributeCategory cat = SCR_EditorAttributeCategory.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
			if (cat && cat.GetInfo())
				return WidgetManager.Translate(cat.GetInfo().GetName());
		}
		string path = category;
		int slash = path.LastIndexOf("/");
		if (slash >= 0)
			path = path.Substring(slash + 1, path.Length() - slash - 1);
		path.Replace(".conf", "");
		return path;
	}

	protected DCO_SetRow MakeRow(SCR_BaseEditorAttribute attribute)
	{
		DCO_SetRow row = new DCO_SetRow();
		row.m_Attr = attribute;
		row.m_sClass = attribute.Type().ToString();
		row.m_sLabel = row.m_sClass;
		row.m_bInternal = row.m_sClass == "DCO_CommanderPresetSaveAttribute" || row.m_sClass == "DCO_CommanderPresetLoadAttribute";
		SCR_EditorAttributeUIInfo info = attribute.GetUIInfo();
		if (info)
		{
			row.m_sLabel = WidgetManager.Translate(info.GetName());
			row.m_sDesc = WidgetManager.Translate(info.GetDescription());
		}

		string layout = attribute.GetLayout();
		layout.ToLower();
		if (DCO_NameEditorAttribute.Cast(attribute) || layout.Contains("dco_text"))
			row.m_iKind = DCO_ESetKind.TEXT;
		else if (layout.Contains("slider"))
			row.m_iKind = DCO_ESetKind.SLIDER;
		else if (layout.Contains("multiselection"))
			row.m_iKind = DCO_ESetKind.MULTI;
		else if (layout.Contains("checkbox"))
			row.m_iKind = DCO_ESetKind.CHECK;
		else if (layout.Contains("dropdown") || SCR_BaseFloatValueHolderEditorAttribute.Cast(attribute))
			row.m_iKind = DCO_ESetKind.DROP;
		else
			row.m_iKind = DCO_ESetKind.CHECK;

		array<ref SCR_BaseEditorAttributeEntry> entries = {};
		attribute.GetEntries(entries);
		foreach (SCR_BaseEditorAttributeEntry entry : entries)
		{
			SCR_BaseEditorAttributeEntrySlider slider = SCR_BaseEditorAttributeEntrySlider.Cast(entry);
			if (slider)
			{
				row.m_Slider = slider;
				float sliderMin, sliderMax, sliderStep;
				slider.GetSliderMinMaxStep(sliderMin, sliderMax, sliderStep);
				row.m_fMin = sliderMin;
				row.m_fMax = sliderMax;
				row.m_fStep = sliderStep;
				row.m_sFormat = slider.GetSliderValueFormating();
				continue;
			}
			SCR_BaseEditorAttributeFloatStringValues values = SCR_BaseEditorAttributeFloatStringValues.Cast(entry);
			if (values)
			{
				for (int i = 0, n = values.GetValueCount(); i < n; i++)
				{
					SCR_EditorAttributeFloatStringValueHolder holder = values.GetValuesEntry(i);
					if (!holder)
						continue;
					row.m_aNames.Insert(WidgetManager.Translate(holder.GetName()));
					row.m_aValues.Insert(holder.GetFloatValue());
				}
			}
		}
		row.m_bBeta = DCO_SettingsHelp.IsBeta(row.m_sClass);
		string search = row.m_sLabel + " " + row.m_sDesc;
		if (row.m_bBeta)
			search += " beta experimental";
		foreach (string optionName : row.m_aNames)
		{
			search += " " + optionName;
		}
		search.ToLower();
		row.m_sSearch = search;
		if (row.m_iKind == DCO_ESetKind.SLIDER && !row.m_Slider)
			row.m_iKind = DCO_ESetKind.CHECK;
		if (row.m_iKind == DCO_ESetKind.DROP && row.m_aNames.IsEmpty())
			row.m_iKind = DCO_ESetKind.CHECK;

		SCR_BaseEditorAttributeVar var = attribute.GetVariableOrCopy();
		if (var)
			row.m_vOrig = var.GetVector();
		row.m_vCur = row.m_vOrig;
		row.m_bConflict = attribute.GetIsMultiSelect() && attribute.GetHasConflictingValues();

		vector def;
		if (m_Manager.DCO_GetDefault(attribute, def))
		{
			row.m_vDef = def;
			row.m_bHasDef = true;
			row.m_bKnownDef = true;
		}
		else
		{
			row.m_vDef = row.m_vOrig;
			row.m_bHasDef = !row.m_bConflict;
		}

		if (row.m_iKind == DCO_ESetKind.TEXT)
		{
			DCO_NameEditorAttribute nameAttribute = DCO_NameEditorAttribute.Cast(attribute);
			if (nameAttribute)
				row.m_sTextOrig = nameAttribute.StartTextEditing(m_Manager);
			row.m_sText = row.m_sTextOrig;
			row.m_bHasDef = false;
		}
		return row;
	}

	protected bool Same(vector a, vector b)
	{
		for (int i = 0; i < 3; i++)
		{
			float tol = 0.0001 * Math.Max(1, Math.AbsFloat(b[i]));
			if (Math.AbsFloat(a[i] - b[i]) > tol)
				return false;
		}
		return true;
	}

	protected bool FactionMode()
	{
		return !m_sFaction.IsEmpty();
	}

	protected DCO_SetFactionEdit Fac()
	{
		DCO_SetFactionEdit edit;
		if (m_mFaction.Find(m_sFaction, edit))
			return edit;
		return null;
	}

	protected bool FactionRow(DCO_SetRow row)
	{
		if (row.m_iKind == DCO_ESetKind.TEXT || row.m_iSub < 0)
			return false;
		return !m_aFolders[m_aSubs[row.m_iSub].m_iFolder].m_bWorld;
	}

	protected bool IsOverridden(DCO_SetRow row)
	{
		DCO_SetFactionEdit edit = Fac();
		return edit && edit.m_mCur.Contains(row.m_sClass);
	}

	protected vector ViewValue(DCO_SetRow row)
	{
		if (MemberScope() && row.m_bMember)
		{
			DCO_SetFactionEdit memberEdit = MemEdit();
			vector memberValue;
			if (memberEdit && memberEdit.m_mCur.Find(row.m_sClass, memberValue))
				return memberValue;
			return row.m_vOrig;
		}
		if (!FactionMode())
			return row.m_vCur;
		DCO_SetFactionEdit edit = Fac();
		vector v;
		if (edit && edit.m_mCur.Find(row.m_sClass, v))
			return v;
		vector builtin;
		if (DCO_DoctrineGM.BuiltinRowValue(row.m_Attr, m_sFaction, builtin))
			return builtin;
		return row.m_vOrig;
	}

	protected bool CommanderDocRow(DCO_SetRow row)
	{
		return DCO_CmdDocTableAttribute.Cast(row.m_Attr) != null || DCO_CmdDocPresetAttribute.Cast(row.m_Attr) != null;
	}

	protected void SyncTaskRows(DCO_SetRow row, float value)
	{
		bool fromPreset = row.m_sClass == "DCO_SquadTaskPresetAttribute";
		string other = "DCO_SquadTaskPresetAttribute";
		if (fromPreset)
			other = "DCO_SquadAllowedTasksAttribute";
		int oi = RowByClass(other);
		if (oi < 0)
			return;
		int v = Math.Round(value);
		int want;
		if (fromPreset)
		{
			want = DCO_GroupJobs.PresetMask(v);
			if (want < 0)
				return;
		}
		else
			want = DCO_GroupJobs.PresetOf(v & 511);
		vector ov = ViewValue(m_aRows[oi]);
		int have = Math.Round(ov[0]);
		if (have == want)
			return;
		string src = "tasks";
		if (fromPreset)
			src = "preset";
		int mask = v;
		if (fromPreset)
			mask = want;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("grpjob_ui_sync src=%1 mask=%2 preset=%3 ui=dco", src, mask, DCO_GroupJobs.PresetLabel(DCO_GroupJobs.PresetOf(mask & 511))));
		SetCur(m_aRows[oi], want);
	}

	protected void ApplyPresetRows(DCO_SetRow row, float value)
	{
		if (row.m_sClass == "DCO_SquadTaskPresetAttribute" || row.m_sClass == "DCO_SquadAllowedTasksAttribute")
		{
			SyncTaskRows(row, value);
			return;
		}
		if (row.m_sClass == "DCO_SquadOverrideTaskAttribute")
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("grpjob_ui_gate can_task=%1 ui=dco", DCO_Util.B01(value != 0)));
			return;
		}
		if (row.m_sClass == "DCO_CmdPersonalityPresetAttribute")
		{
			DCO_Personality persona = DCO_Personality.Get(DCO_Personality.NameAt(value));
			array<string> traitClasses = {"DCO_CmdTraitIntelAttribute", "DCO_CmdTraitStrategyAttribute", "DCO_CmdTraitAggressionAttribute", "DCO_CmdTraitLogisticsAttribute", "DCO_CmdTraitSupportAttribute", "DCO_CmdTraitLeadershipAttribute", "DCO_CmdCourageAttribute", "DCO_CmdDecisionAttribute"};
			float courageValue = persona.m_iCourage;
			float decisionValue = persona.m_iDecision;
			array<float> traitValues = {persona.m_fIntel, persona.m_fStrategy, persona.m_fAggression, persona.m_fLogistics, persona.m_fSupport, persona.m_fLeadership, courageValue, decisionValue};
			foreach (int ti, string traitClass : traitClasses)
			{
				int traitRow = RowByClass(traitClass);
				if (traitRow >= 0)
					SetCur(m_aRows[traitRow], traitValues[ti]);
			}
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_preset_rows kind=personality preset=" + DCO_Personality.NameAt(value));
			return;
		}
		bool commander = row.m_sClass == "DCO_CmdDocPresetAttribute";
		if (!commander && row.m_sClass != "DCO_DocPresetAttribute")
			return;
		int presetIndex = value;
		if (commander)
			presetIndex = presetIndex - 1;
		if (presetIndex < 0)
			return;
		string name = DCO_DoctrineGM.PresetNameAt(presetIndex);
		DCO_DoctrinePreset preset = DCO_DoctrineStore.Find(name);
		if (!preset)
			return;
		DCO_Doctrine d = new DCO_Doctrine();
		d.FromPreset(preset, "gm");
		for (int i = 0; i < 15; i++)
		{
			int tableRow = RowByClass(DCO_DoctrineGM.TableClass(i, commander));
			if (tableRow >= 0)
				SetCur(m_aRows[tableRow], d.Mask(i));
		}
		if (!commander)
		{
			array<string> numberClasses = {};
			DCO_DoctrineGM.NumberClasses(numberClasses);
			foreach (int ni, string numberClass : numberClasses)
			{
				int numberRow = RowByClass(numberClass);
				if (numberRow >= 0)
					SetCur(m_aRows[numberRow], d.Number(ni));
			}
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_preset_rows kind=doctrine preset=" + DCO_SettingsBaseline.LogText(name) + " commander=" + commander.ToString() + " faction=" + m_sFaction);
	}

	protected bool FactionPending(DCO_SetFactionEdit edit, string className)
	{
		vector a, b;
		bool inOrig = edit.m_mOrig.Find(className, a);
		bool inCur = edit.m_mCur.Find(className, b);
		if (inOrig != inCur)
			return true;
		return inCur && !Same(a, b);
	}

	protected int FactionChanges(DCO_SetFactionEdit edit)
	{
		int n = 0;
		foreach (string className, vector v : edit.m_mCur)
		{
			if (FactionPending(edit, className))
				n++;
		}
		foreach (string className, vector v : edit.m_mOrig)
		{
			if (!edit.m_mCur.Contains(className))
				n++;
		}
		return n;
	}

	protected bool IsModified(DCO_SetRow row)
	{
		if (MemberScope() && row.m_bMember)
			return !RowInherits(row);
		if (FactionMode())
			return FactionRow(row) && IsOverridden(row);
		if (row.m_iKind == DCO_ESetKind.TEXT)
			return TextPending(row);
		if (row.m_bConflict && !row.m_bTouched)
			return false;
		if (!row.m_bHasDef)
			return false;
		return !Same(row.m_vCur, row.m_vDef);
	}

	protected bool TextPending(DCO_SetRow row)
	{
		string text = row.m_sText.Trim();
		return !text.IsEmpty() && text != row.m_sTextOrig;
	}

	protected bool IsPending(DCO_SetRow row)
	{
		if (MemberScope() && row.m_bMember)
		{
			DCO_SetFactionEdit memberEdit = MemEdit();
			return memberEdit && FactionPending(memberEdit, row.m_sClass);
		}
		if (FactionMode())
		{
			DCO_SetFactionEdit edit = Fac();
			return edit && FactionRow(row) && FactionPending(edit, row.m_sClass);
		}
		if (row.m_iKind == DCO_ESetKind.TEXT)
			return TextPending(row);
		if (!row.m_bTouched)
			return false;
		if (row.m_bConflict)
			return true;
		return !Same(row.m_vCur, row.m_vOrig);
	}

	protected bool IsBasePending(DCO_SetRow row)
	{
		if (row.m_iKind == DCO_ESetKind.TEXT)
			return TextPending(row);
		if (!row.m_bTouched)
			return false;
		if (row.m_bConflict)
			return true;
		return !Same(row.m_vCur, row.m_vOrig);
	}

	protected bool ShowsMixed(DCO_SetRow row)
	{
		if (FactionMode() || (MemberScope() && row.m_bMember))
			return false;
		return row.m_bConflict && !row.m_bTouched;
	}

	protected float Quant(DCO_SetRow row, float v)
	{
		if (row.m_fStep > 0)
			v = row.m_fMin + Math.Round((v - row.m_fMin) / row.m_fStep) * row.m_fStep;
		return Math.Clamp(v, row.m_fMin, row.m_fMax);
	}

	protected float Level(DCO_SetRow row, int i)
	{
		return Quant(row, row.m_fMin + (row.m_fMax - row.m_fMin) * i * 0.25);
	}

	protected bool Near(DCO_SetRow row, float a, float b)
	{
		float tol = 0.0001;
		if (row.m_fStep > 0)
			tol = row.m_fStep * 0.01;
		return Math.AbsFloat(a - b) <= tol;
	}

	protected string NumText(DCO_SetRow row, float v)
	{
		if (row.m_Slider)
			return row.m_Slider.GetText(v);
		return v.ToString();
	}

	protected string SliderText(DCO_SetRow row, float v)
	{
		if (v >= 0 && DCO_SettingsTree.IsPercent(row.m_sClass))
			return Math.Round(v * 100).ToString() + "%";
		string num = NumText(row, v);
		string fmt = row.m_sFormat;
		string tr = WidgetManager.Translate(fmt);
		if (tr.Contains("%1"))
			fmt = tr;
		if (fmt.IsEmpty() || !fmt.Contains("%1"))
			return num;
		return string.Format(fmt, num);
	}

	protected string ValueText(DCO_SetRow row, vector v)
	{
		switch (row.m_iKind)
		{
			case DCO_ESetKind.CHECK:
			{
				if (v[0] != 0)
					return T("set_on");
				return T("set_off");
			}
			case DCO_ESetKind.DROP:
			{
				if (row.m_sClass == "DCO_SquadTaskPresetAttribute" && Math.Round(v[0]) == 6)
				{
					int tasksRow = RowByClass("DCO_SquadAllowedTasksAttribute");
					if (tasksRow >= 0)
					{
						vector tv = ViewValue(m_aRows[tasksRow]);
						int tmask = Math.Round(tv[0]);
						return DCO_GroupJobs.CustomText(tmask);
					}
				}
				foreach (int i, float value : row.m_aValues)
				{
					if (Math.AbsFloat(value - v[0]) < 0.0001)
						return row.m_aNames[i];
				}
				return v[0].ToString();
			}
			case DCO_ESetKind.MULTI:
			{
				string outText;
				int mask = v[0];
				foreach (int i, float value : row.m_aValues)
				{
					if (mask & Bit(row, i))
					{
						if (!outText.IsEmpty())
							outText += ", ";
						outText += row.m_aNames[i];
					}
				}
				if (outText.IsEmpty())
					return "-";
				return outText;
			}
			case DCO_ESetKind.SLIDER:
				return SliderText(row, v[0]);
		}
		return row.m_sText;
	}

	protected int Bit(DCO_SetRow row, int i)
	{
		int bit = row.m_aValues[i];
		if (bit <= 0)
			bit = 1 << i;
		return bit;
	}

	protected int Parts(DCO_SetRow row)
	{
		if (row.m_iKind == DCO_ESetKind.DROP || row.m_iKind == DCO_ESetKind.MULTI)
			return (int)Math.Max(1, (row.m_aNames.Count() + 6) / 7);
		return 1;
	}

	protected bool DropMatches(DCO_SetRow row)
	{
		vector v = ViewValue(row);
		foreach (float value : row.m_aValues)
		{
			if (Math.AbsFloat(value - v[0]) < 0.0001)
				return true;
		}
		return false;
	}

	protected int PerLine(DCO_SetRow row)
	{
		int parts = Parts(row);
		return (int)Math.Max(1, (row.m_aNames.Count() + parts - 1) / parts);
	}

	protected void SetCur(DCO_SetRow row, float value)
	{
		vector v = ViewValue(row);
		v[0] = value;
		if (MemberScope() && row.m_bMember)
		{
			DCO_SetFactionEdit memberEdit = MemEdit();
			if (!memberEdit)
				return;
			memberEdit.m_mCur.Set(row.m_sClass, v);
		}
		else if (FactionMode())
		{
			DCO_SetFactionEdit edit = Fac();
			if (!edit || !FactionRow(row))
				return;
			edit.m_mCur.Set(row.m_sClass, v);
		}
		else
		{
			row.m_vCur = v;
			row.m_bTouched = true;
		}
		ApplyPresetRows(row, value);
		UpdateDependencies();
		RequestRender();
	}

	protected void UpdateDependencies()
	{
		foreach (DCO_SetRow row : m_aRows)
		{
			row.m_bDisabled = false;
		}
		foreach (DCO_SettingsDependencyDef dep : DCO_SettingsTree.Get().m_aDependencies)
		{
			int parentIndex;
			if (!m_mRowByClass.Find(dep.m_sParent, parentIndex))
				continue;
			DCO_SetRow parent = m_aRows[parentIndex];
			vector parentValue = ViewValue(parent);
			if (ShowsMixed(parent) || !DCO_SettingsTree.HasValue(dep.m_sOffValues, parentValue[0]))
				continue;
			array<string> children = {};
			dep.m_sChildren.Split(" ", children, true);
			foreach (string child : children)
			{
				int childIndex;
				if (m_mRowByClass.Find(child, childIndex))
					m_aRows[childIndex].m_bDisabled = true;
			}
		}
		if (FactionMode())
			return;
		foreach (DCO_SetRow row : m_aRows)
		{
			if (!row.m_bMember)
				m_Manager.SetAttributeEnabled(row.m_Attr.Type(), !row.m_bDisabled);
		}
	}

	protected bool SearchMode()
	{
		return !m_sSearch.IsEmpty();
	}

	protected bool RowVisible(DCO_SetRow row)
	{
		if (row.m_bInternal)
			return false;
		if (FactionMode() && !FactionRow(row))
			return false;
		if (MemberScope() && !row.m_bMember)
			return false;
		if (m_bOnlyMod && !IsModified(row) && !IsPending(row))
			return false;
		return true;
	}

	protected bool FolderVisible(DCO_SetFolder folder)
	{
		if (MemberScope() && !folder.m_bMember)
			return false;
		return !FactionMode() || !folder.m_bWorld;
	}

	protected void FixSelection()
	{
		if (m_iSel < 0 || m_iSel >= m_aSubs.Count())
			return;
		if (FolderVisible(m_aFolders[m_aSubs[m_iSel].m_iFolder]))
			return;
		m_iSel = -1;
		foreach (int i, DCO_SetSub sub : m_aSubs)
		{
			if (FolderVisible(m_aFolders[sub.m_iFolder]))
			{
				m_iSel = i;
				return;
			}
		}
	}

	protected void BuildLines()
	{
		m_aLines.Clear();
		if (SimpleView())
		{
			BuildSimpleLines();
			return;
		}
		array<int> rows = {};
		string header;
		bool twoLine = false;
		int expertHidden;
		string carrySec;
		map<int, string> carryAt = new map<int, string>();
		if (SearchMode())
		{
			string q = m_sSearch;
			q.ToLower();
			foreach (int i, DCO_SetRow row : m_aRows)
			{
				if (row.m_sSearch.Contains(q) && RowVisible(row))
					rows.Insert(i);
			}
			header = T("set_search_results");
			twoLine = true;
		}
		else if (m_bOnlyMod)
		{
			foreach (int mi, DCO_SetRow modRow : m_aRows)
			{
				if (RowVisible(modRow))
					rows.Insert(mi);
			}
			twoLine = true;
		}
		else if (m_iSel < 0)
		{
			foreach (int i, DCO_SetRow row : m_aRows)
			{
				if (DCO_SettingsPrefs.IsFav(row.m_sClass) && RowVisible(row))
					rows.Insert(i);
			}
			header = T("set_favorites");
			twoLine = true;
		}
		else if (m_iSel < m_aSubs.Count())
		{
			foreach (int i : m_aSubs[m_iSel].m_aRows)
			{
				if (!RowVisible(m_aRows[i]))
					continue;
				if (!m_bExpert && DCO_SettingsTree.IsExpert(m_aRows[i].m_sClass))
				{
					expertHidden++;
					string hiddenSec = DCO_SettingsTree.SectionTitle(m_aRows[i].m_sClass);
					if (!hiddenSec.IsEmpty())
						carrySec = hiddenSec;
					continue;
				}
				rows.Insert(i);
				if (!carrySec.IsEmpty())
				{
					if (DCO_SettingsTree.SectionTitle(m_aRows[i].m_sClass).IsEmpty())
						carryAt.Set(i, carrySec);
					carrySec = string.Empty;
				}
			}
			header = m_aSubs[m_iSel].m_sName;
		}

		if (m_bOnlyMod && !SearchMode())
			header = T("set_only_mod");
		DCO_SetLine head = new DCO_SetLine();
		head.m_iType = 0;
		head.m_fH = 40;
		head.m_sText = header;
		head.m_sHint = T2("set_n_settings", "n", rows.Count().ToString());
		if (expertHidden > 0)
			head.m_sHint += "  ·  " + T2("set_expert_hidden", "n", expertHidden.ToString());
		m_aLines.Insert(head);
		if (!SearchMode() && !m_bOnlyMod && m_iSel >= 0)
			BuildSpawnerLines();

		if (rows.IsEmpty())
		{
			DCO_SetLine empty = new DCO_SetLine();
			empty.m_iType = 2;
			empty.m_fH = 120;
			if (SearchMode())
				empty.m_sText = T2("set_empty_search", "q", m_sSearch);
			else if (m_bOnlyMod)
				empty.m_sText = T("set_empty_mod");
			else if (m_iSel < 0)
				empty.m_sText = T("set_fav_empty");
			else
				empty.m_sText = T("set_nothing");
			m_aLines.Insert(empty);
			return;
		}

		bool sections = !SearchMode() && !m_bOnlyMod && m_iSel >= 0;
		foreach (int r : rows)
		{
			string secTitle;
			if (sections)
			{
				secTitle = DCO_SettingsTree.SectionTitle(m_aRows[r].m_sClass);
				if (secTitle.IsEmpty())
					carryAt.Find(r, secTitle);
			}
			if (!secTitle.IsEmpty())
			{
				DCO_SetLine sec = new DCO_SetLine();
				sec.m_iType = 4;
				sec.m_fH = 36;
				sec.m_sText = secTitle;
				m_aLines.Insert(sec);
			}
			int parts = Parts(m_aRows[r]);
			for (int p = 0; p < parts; p++)
			{
				DCO_SetLine line = new DCO_SetLine();
				line.m_iType = 1;
				line.m_iRow = r;
				line.m_iPart = p;
				line.m_fH = m_fLineH;
				line.m_bPath = twoLine;
				m_aLines.Insert(line);
			}
		}
	}

	protected int CountModified(array<int> rowIndexes)
	{
		int n = 0;
		foreach (int i : rowIndexes)
		{
			if (IsModified(m_aRows[i]))
				n++;
		}
		return n;
	}

	protected void RequestRender()
	{
		if (m_bQueued)
			return;
		m_bQueued = true;
		GetGame().GetCallqueue().CallLater(DoRender, 0, false);
	}

	protected void ClearActs()
	{
		m_aActKind.Clear();
		m_aActA.Clear();
		m_aActB.Clear();
		m_aActTip.Clear();
		m_aActKind.Insert(DCO_ESetAct.NONE);
		m_aActA.Insert(0);
		m_aActB.Insert(0);
		m_aActTip.Insert(string.Empty);
	}

	protected int Act(int kind, int a, int b, string tip)
	{
		m_aActKind.Insert(kind);
		m_aActA.Insert(a);
		m_aActB.Insert(b);
		m_aActTip.Insert(tip);
		return m_aActKind.Count() - 1;
	}

	protected void Tag(Widget w, int id)
	{
		if (!w)
			return;
		w.SetUserID(id);
		Widget child = w.GetChildren();
		while (child)
		{
			Tag(child, id);
			child = child.GetSibling();
		}
	}

	protected string Bold(string text)
	{
		return "<color rgba=\"232,163,61,255\">" + text + "</color>";
	}

	protected Widget Cell(Widget parent, float x, float y, float w, float h, string text, Color bg, Color fg, int style, int fontSize, int id)
	{
		Widget c = GetGame().GetWorkspace().CreateWidgets(m_sCellLayout, parent);
		if (!c)
			return null;
		FrameSlot.SetPos(c, x, y);
		FrameSlot.SetSize(c, w, h);
		ImageWidget bgw = ImageWidget.Cast(c.FindAnyWidget("Bg"));
		if (bgw)
			bgw.SetColor(bg);
		Widget icon = c.FindAnyWidget("Icon");
		if (icon)
			icon.SetVisible(false);
		if (!fg)
			fg = m_cText;
		array<string> names = {"Text", "TextB", "TextC", "TextCB"};
		foreach (int i, string n : names)
		{
			RichTextWidget t = RichTextWidget.Cast(c.FindAnyWidget(n));
			if (!t)
				continue;
			if (i != style)
			{
				t.SetVisible(false);
				continue;
			}
			t.SetText(text);
			t.SetColor(fg);
			t.SetDesiredFontSize(fontSize);
		}
		Tag(c, id);
		return c;
	}

	protected Widget IconCell(Widget parent, float x, float y, float size, ResourceName texture, string imageName, Color col, int id)
	{
		Widget c = Cell(parent, x, y, size, size, string.Empty, m_cClear, m_cText, 0, 14, id);
		if (!c)
			return null;
		ImageWidget icon = ImageWidget.Cast(c.FindAnyWidget("Icon"));
		if (icon)
		{
			if (imageName.IsEmpty())
				icon.LoadImageTexture(0, texture);
			else
				icon.LoadImageFromSet(0, texture, imageName);
			icon.SetColor(col);
			icon.SetVisible(true);
		}
		return c;
	}

	protected void BuildFixed()
	{
		Widget edit = GetGame().GetWorkspace().CreateWidgets(m_sEditLayout, m_wFixed);
		if (edit)
		{
			FrameSlot.SetPos(edit, m_fPad + 24, m_fToolY + 8);
			FrameSlot.SetSize(edit, 296, 30);
			ImageWidget editBg = ImageWidget.Cast(edit.FindAnyWidget("Bg"));
			if (editBg)
				editBg.SetColor(m_cClear);
			m_wSearch = EditBoxWidget.Cast(edit.FindAnyWidget("Box"));
			if (m_wSearch)
			{
				m_wSearch.SetPlaceholderText(T("set_search"));
				m_wSearch.SetVirtualKeyboardTitle(T("set_search"));
			}
			Tag(edit, 100001);
		}

		m_wTip = Cell(m_wTop, 0, 0, 420, 40, string.Empty, m_cTipBg, m_cText, 0, 16, 0);
		if (m_wTip)
		{
			RichTextWidget t = RichTextWidget.Cast(m_wTip.FindAnyWidget("Text"));
			if (t)
				t.SetTextWrapping(true);
			m_wTip.SetVisible(false);
			m_wTip.SetZOrder(100);
		}
	}

	protected void DoRender()
	{
		m_bQueued = false;
		if (!m_wDyn)
			return;
		m_bRendering = true;
		while (m_wDyn.GetChildren())
		{
			m_wDyn.GetChildren().RemoveFromHierarchy();
		}
		m_bRendering = false;
		ClearActs();
		m_iHover = 0;
		m_aLineFocusW.Clear();
		m_aLineStarW.Clear();
		m_aLineRowIdx.Clear();
		m_aLineFav.Clear();
		m_aSliderRow.Clear();
		m_aSliderFill.Clear();
		m_aSliderText.Clear();
		m_aSliderTick.Clear();
		m_aSliderY.Clear();
		m_aTextRow.Clear();
		m_aTextBox.Clear();
		m_wDescAct = null;
		m_wDescText = null;
		HideTip();

		ResizeWindow();
		RenderChrome();
		RenderTree();
		if (m_iPanel == DCO_ESetPanel.NONE)
			RenderSheet();
		else
			RenderPanel();
		RenderDesc();
		RenderMeters();
		RenderFooter();
		if (m_bHelpOpen)
			RenderHelp();
		if (m_bDiscardOpen)
			RenderDiscard();
	}

	protected Managed SpItem()
	{
		if (!m_Manager)
			return null;
		array<Managed> items = {};
		m_Manager.GetEditedItems(items);
		foreach (Managed it : items)
		{
			if (DCO_SpawnerGM.GetSpawner(it))
				return it;
		}
		return null;
	}

	protected DCO_CommanderSpawnerComponent SpEdited()
	{
		Managed it = SpItem();
		if (!it)
			return null;
		return DCO_SpawnerGM.GetSpawner(it);
	}

	protected AICommander_BaseComponent SpCommander()
	{
		if (!m_Manager)
			return null;
		array<Managed> items = {};
		m_Manager.GetEditedItems(items);
		foreach (Managed it : items)
		{
			SCR_EditableEntityComponent ed = SCR_EditableEntityComponent.Cast(it);
			if (!ed || !ed.GetOwner())
				continue;
			AICommander_BaseComponent c = AICommander_BaseComponent.Cast(ed.GetOwner().FindComponent(AICommander_BaseComponent));
			if (c)
				return c;
		}
		return null;
	}

	protected bool SpSub(string key)
	{
		if (m_iSel < 0 || m_iSel >= m_aSubs.Count())
			return false;
		string cat = m_aSubs[m_iSel].m_Category;
		return cat.Contains(key);
	}

	protected void SpLine(int type, int row, float h, string text)
	{
		DCO_SetLine l = new DCO_SetLine();
		l.m_iType = type;
		l.m_iRow = row;
		l.m_fH = h;
		l.m_sText = text;
		m_aLines.Insert(l);
	}

	protected void PBuildSpawnerPicker(DCO_CommanderSpawnerComponent s)
	{
	}

	protected void PRenderPickRow(DCO_SetLine line, float y)
	{
	}

	protected bool m_bSpShortLogged;

	protected int SpInt(DCO_CommanderSpawnerComponent s, int i)
	{
		string f = s.StatusField(i);
		bool ok = !f.IsEmpty();
		for (int k = 0; ok && k < f.Length(); k++)
		{
			string c = f.Get(k);
			if (!(c == "-" && k == 0) && "0123456789".IndexOf(c) < 0)
				ok = false;
		}
		if (ok)
			return f.ToInt();
		if (!m_bSpShortLogged)
		{
			m_bSpShortLogged = true;
			int n = 0;
			while (n < 40 && !s.StatusField(n).IsEmpty())
				n++;
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("spawner_status_short spawner=%1 field=%2 value='%3' fields=%4 -- status not replicated yet or shorter than expected; using 0", s.Name(), i, f, n));
		}
		DCO_Perf.Count("spawner_ui:status_short");
		return 0;
	}

	protected void BuildSpawnerLines()
	{
		if (SpSub("CommanderLogistics") && SpCommander())
		{
			int n = 0;
			string uid = SpCommander().GetCommanderUID();
			foreach (DCO_CommanderSpawnerComponent cs : DCO_CommanderSpawnerComponent.GetAll())
			{
				if (cs && cs.StatusField(1) == uid)
					n++;
			}
			SpLine(10, -1, 158 + 26 * n, string.Empty);
			return;
		}
		if (!SpSub("CommanderSpawner"))
			return;
		DCO_CommanderSpawnerComponent s = SpEdited();
		if (!s)
			return;
		string poolHead = "Which groups · force pool · changes apply at once, Revert does not undo them";
		if (s.StatusField(16) == "1")
			poolHead = "Which groups · the commander chooses by need, Weight only breaks close calls · changes apply at once";
		SpLine(4, -1, 36, poolHead);
		array<string> pool = {};
		string pstr = s.GetRplPool();
		if (!pstr.IsEmpty())
			pstr.Split(";", pool, true);
		if (!pool.IsEmpty())
			SpLine(6, -1, 32, string.Empty);
		foreach (int i, string pr : pool)
		{
			SpLine(6, i, 36, pr);
			if (i == m_iSpRow)
				SpLine(7, i, 96, pr);
		}
		SpLine(8, -1, 40, s.StatusField(6));
		m_aSpCatalog.Clear();
		if (m_bSpPicker)
		{
			FactionManager fm = GetGame().GetFactionManager();
			SCR_Faction fac;
			if (fm)
				fac = SCR_Faction.Cast(fm.GetFactionByKey(s.GetRplFaction()));
			if (fac)
			{
				SCR_EntityCatalog cat = fac.GetFactionEntityCatalogOfType(EEntityCatalogType.GROUP, false);
				if (cat)
				{
					array<SCR_EntityCatalogEntry> list = {};
					cat.GetEntityList(list);
					foreach (SCR_EntityCatalogEntry ce : list)
					{
						if (!ce || m_aSpCatalog.Count() >= 24)
							continue;
						m_aSpCatalog.Insert(ce.GetPrefab());
						SpLine(9, m_aSpCatalog.Count() - 1, 32, FilePath.StripExtension(FilePath.StripPath(ce.GetPrefab())));
					}
				}
			}
			if (m_aSpCatalog.IsEmpty())
				SpLine(9, -1, 32, "No group catalog for this faction.");
			SpLine(4, -1, 36, "My presets");
			PBuildSpawnerPicker(s);
		}
	}

	protected static string SpType(int i)
	{
		array<string> types = {"SL", "TL", "RIF", "AR", "GL", "AT", "MED", "RTO", "MRK"};
		return types[i];
	}

	protected string SpBMembers()
	{
		string s;
		for (int i = 0; i < 9; i++)
		{
			for (int k = 0; k < m_aSpBCount[i]; k++)
			{
				if (!s.IsEmpty())
					s += ",";
				s += SpType(i);
			}
		}
		return s;
	}

	protected int SpBMen()
	{
		int n;
		foreach (int c : m_aSpBCount)
			n += c;
		return n;
	}

	protected string SpBVehicle()
	{
		if (m_iSpBVeh < 0 || m_iSpBVeh >= m_aSpVehicles.Count())
			return string.Empty;
		return m_aSpVehicles[m_iSpBVeh];
	}

	protected string SpBPack(DCO_CommanderSpawnerComponent s)
	{
		return string.Format("%1|%2|%3|%4|%5|%6|%7", m_sSpBName.Trim(), s.GetRplFaction(), SpBMembers(), SpBVehicle(), m_iSpBRole, m_iSpBExp, m_iSpBMor);
	}

	protected void SpBTemplate(int which)
	{
		for (int i = 0; i < 9; i++)
			m_aSpBCount[i] = 0;
		if (which == 0)
		{
			m_aSpBCount[0] = 1;
			m_aSpBCount[1] = 1;
			m_aSpBCount[2] = 2;
			m_aSpBCount[3] = 1;
			m_aSpBCount[4] = 1;
			m_aSpBCount[5] = 1;
			m_aSpBCount[6] = 1;
		}
		else if (which == 1)
		{
			m_aSpBCount[1] = 1;
			m_aSpBCount[2] = 1;
			m_aSpBCount[3] = 1;
			m_aSpBCount[4] = 1;
		}
		else if (which == 2)
		{
			m_aSpBCount[1] = 1;
			m_aSpBCount[2] = 1;
			m_aSpBCount[5] = 2;
		}
	}

	protected bool SpBuilderAct(int code, int b, DCO_CommanderSpawnerComponent s)
	{
		if (code == 41)
		{
			m_bSpBuilder = !m_bSpBuilder;
			m_bSpBConfirm = false;
			m_sSpMsg = string.Empty;
			if (m_bSpBuilder)
			{
				m_sSpBName = string.Empty;
				SpBTemplate(0);
				m_iSpBVeh = -1;
				m_iSpBRole = 0;
				m_iSpBExp = 1;
				m_iSpBMor = 1;
				m_aSpVehicles.Clear();
				FactionManager fm = GetGame().GetFactionManager();
				SCR_Faction fac;
				if (fm && s)
					fac = SCR_Faction.Cast(fm.GetFactionByKey(s.GetRplFaction()));
				if (fac)
				{
					SCR_EntityCatalog vc = fac.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
					if (vc)
					{
						array<SCR_EntityCatalogEntry> vl = {};
						vc.GetEntityList(vl);
						foreach (SCR_EntityCatalogEntry ve : vl)
						{
							if (ve && m_aSpVehicles.Count() < 16)
								m_aSpVehicles.Insert(ve.GetPrefab());
						}
					}
				}
			}
			return true;
		}
		if (code == 42)
		{
			SpBTemplate(b);
			m_bSpBConfirm = false;
			return true;
		}
		if (code == 43)
		{
			int ty = b / 10000;
			int dv = (b % 10000) - 5000;
			if (ty >= 0 && ty < 9)
				m_aSpBCount[ty] = Math.ClampInt(m_aSpBCount[ty] + dv, 0, 12);
			return true;
		}
		if (code == 44)
		{
			m_iSpBVeh++;
			if (m_iSpBVeh >= m_aSpVehicles.Count())
				m_iSpBVeh = -1;
			return true;
		}
		if (code == 45)
		{
			m_iSpBRole = (m_iSpBRole + 1) % 6;
			return true;
		}
		if (code == 46)
		{
			m_iSpBExp = (m_iSpBExp + 1) % 4;
			return true;
		}
		if (code == 47)
		{
			m_iSpBMor = (m_iSpBMor + 1) % 3;
			return true;
		}
		return false;
	}

	protected void RenderBuilder(DCO_SetLine line, float y, DCO_CommanderSpawnerComponent s)
	{
		float x0 = m_fSheetX + 24;
		float w = m_fSheetW - 48;
		Cell(m_wDyn, x0, y, w, line.m_fH - 6, string.Empty, m_cPanel, m_cText, 0, 10, 0);
		float x = x0 + 12;
		float yy = y + 8;
		Cell(m_wDyn, x, yy, 70, 32, "Name", m_cClear, m_cDim, 0, 16, 0);
		Widget edit = GetGame().GetWorkspace().CreateWidgets(m_sEditLayout, m_wDyn);
		if (edit)
		{
			FrameSlot.SetPos(edit, x + 74, yy);
			FrameSlot.SetSize(edit, 280, 32);
			ImageWidget eb = ImageWidget.Cast(edit.FindAnyWidget("Bg"));
			if (eb)
				eb.SetColor(m_cCell);
			EditBoxWidget box = EditBoxWidget.Cast(edit.FindAnyWidget("Box"));
			if (box)
			{
				box.SetText(m_sSpBName);
				box.SetPlaceholderText("Preset name");
				box.SetVirtualKeyboardTitle("Preset name");
			}
			Tag(edit, Act(DCO_ESetAct.SPAWNER, 40, 0, string.Empty));
		}
		Cell(m_wDyn, x + 366, yy, 150, 32, "Faction " + s.GetRplFaction(), m_cClear, m_cDim, 0, 16, 0);
		float bx = x + 520;
		array<string> tmpl = {"Rifle squad", "Fire team", "AT team", "Empty"};
		foreach (int ti, string tn : tmpl)
			bx = SpButton(bx, yy, 108, tn, 42, ti, m_cCell, m_cText);
		yy += 42;
		for (int i = 0; i < 9; i++)
		{
			int col = i % 3;
			int rowi = i / 3;
			float cx = x + col * 320;
			float cy = yy + rowi * 38;
			Cell(m_wDyn, cx, cy, 170, 32, DCO_GroupPresets.TypeName(SpType(i)), m_cClear, m_cText, 0, 16, 0);
			SpButton(cx + 172, cy, 34, "-", 43, SpEnc(i, -1), m_cCell, m_cText);
			Cell(m_wDyn, cx + 210, cy, 40, 32, m_aSpBCount[i].ToString(), m_cClear, m_cText, 2, 16, 0);
			SpButton(cx + 252, cy, 34, "+", 43, SpEnc(i, 1), m_cCell, m_cText);
		}
		yy += 3 * 38 + 6;
		string veh = "None";
		if (!SpBVehicle().IsEmpty())
			veh = FilePath.StripExtension(FilePath.StripPath(SpBVehicle()));
		bx = SpButton(x, yy, 300, "Vehicle: " + veh, 44, 0, m_cCell, m_cText);
		string members = SpBMembers();
		DCO_ESpawnerRole autoRole = DCO_GroupPresets.AutoRole(members, !SpBVehicle().IsEmpty());
		array<string> roles = {"Auto", "Assault", "Fire team", "AT", "Recon", "Motorized"};
		string roleText = "Role: " + roles[m_iSpBRole];
		if (m_iSpBRole == 0)
			roleText = "Role: " + roles[autoRole] + " · auto";
		bx = SpButton(bx, yy, 230, roleText, 45, 0, m_cCell, m_cText);
		bx = SpButton(bx, yy, 150, DCO_CommanderSpawnerComponent.ExperienceName(m_iSpBExp), 46, 0, m_cCell, m_cText);
		array<string> morales = {"Low morale", "Normal morale", "High morale"};
		SpButton(bx, yy, 170, morales[m_iSpBMor], 47, 0, m_cCell, m_cText);
		yy += 44;
		int men = SpBMen();
		DCO_SpawnerGroupEntry tmp = new DCO_SpawnerGroupEntry();
		tmp.m_eExperience = m_iSpBExp;
		tmp.m_eMorale = m_iSpBMor;
		tmp.m_eShape = DCO_ESpawnerShape.RESTED;
		tmp.m_iCostOverride = -1;
		tmp.m_sVehicle = SpBVehicle();
		int cost = DCO_CommanderSpawnerComponent.CostOf(tmp, men);
		string resolved = roles[autoRole];
		if (m_iSpBRole != 0)
			resolved = roles[m_iSpBRole];
		Cell(m_wDyn, x, yy, w - 24, 28, string.Format("%1 men · %2 · vehicle %3 · cost %4", men, resolved, veh, cost), m_cClear, m_cText, 0, 17, 0);
		yy += 30;
		string check;
		Color checkFg = m_cAmber;
		bool blocked = false;
		if (men == 0)
		{
			check = "Add at least one soldier.";
			blocked = true;
			checkFg = m_cDanger;
		}
		else if (men > 12)
		{
			check = "More than 12 men: split into two presets.";
			blocked = true;
			checkFg = m_cDanger;
		}
		else if (m_aSpBCount[0] + m_aSpBCount[1] == 0)
			check = "No leader: the first rifleman leads.";
		if (!blocked && !SpBVehicle().IsEmpty())
		{
			int pilot;
			int turret;
			int cargo;
			DCO_GroupPresets.SeatsOf(SpBVehicle(), pilot, turret, cargo);
			int seats = pilot + turret + cargo;
			if (seats > 0 && men > seats)
			{
				string seatMsg = string.Format("Not enough seats: %1 men, %2 seats in %3. The rest walk.", men, seats, veh);
				if (check.IsEmpty())
					check = seatMsg;
				else
					check += " " + seatMsg;
			}
			else if (seats > 0 && check.IsEmpty())
			{
				check = string.Format("Seats: %1 (driver %2, gunner %3, passengers %4). A Crewed spawner adds its own driver and gunner; the group uses the passenger seats.", seats, pilot, turret, cargo);
				checkFg = m_cDim;
			}
		}
		if (!m_sSpMsg.IsEmpty())
			check = m_sSpMsg;
		Cell(m_wDyn, x, yy, w - 24, 26, check, m_cClear, checkFg, 0, 16, 0);
		yy += 30;
		string saveText = "Save preset";
		if (m_bSpBConfirm)
			saveText = "Replace " + m_sSpBName.Trim() + "?";
		Color saveBg = m_cOrange;
		if (blocked)
			saveBg = m_cCell;
		bx = SpButton(x, yy, 220, saveText, 48, 0, saveBg, m_cDark);
		SpButton(bx, yy, 220, "Add to this pool only", 49, 0, m_cCell, m_cText);
		Cell(m_wDyn, x + 460, yy, w - 480, 34, "Only who is in the group is saved. Gear changed by hand is not saved; every soldier gets the standard gear for that soldier type.", m_cClear, m_cDim, 0, 14, 0);
	}

	protected void RenderPresetRow(DCO_SetLine line, float y)
	{
		array<string> pp = {};
		line.m_sText.Split("|", pp, false);
		if (pp.Count() < 5)
			return;
		float x0 = m_fSheetX + 32;
		Cell(m_wDyn, x0, y, m_fSheetW - 64, 34, string.Empty, m_cRowBg, m_cText, 0, 10, 0);
		Cell(m_wDyn, x0 + 8, y + 3, 520, 28, string.Format("%1 · %2 men · %3 · %4", pp[0], pp[4], pp[1], pp[2]), m_cClear, m_cText, 0, 16, 0);
		float bx = x0 + 560;
		bx = SpButton(bx, y, 90, "Add", 50, line.m_iRow, m_cOrange, m_cDark);
		bx = SpButton(bx, y, 120, "Rename", 52, line.m_iRow, m_cCell, m_cText);
		SpButton(bx, y, 110, "Delete", 51, line.m_iRow, m_cDanger, m_cWhite);
	}

	protected int SpEnc(int row, int v)
	{
		return row * 10000 + v + 5000;
	}

	protected void SpAct(int code, int b)
	{
		if (code == 61)
		{
			DCO_CommanderSpawnerComponent showSp = SpEdited();
			if (showSp && showSp.GetOwner())
			{
				SCR_ManualCamera cam = SCR_CameraEditorComponent.GetCameraInstance();
				if (cam)
				{
					SCR_TeleportToCursorManualCameraComponent tc = SCR_TeleportToCursorManualCameraComponent.Cast(cam.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
					if (tc)
						tc.TeleportCamera(showSp.GetOwner().GetOrigin());
				}
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "spawner_panel_show spawner=" + showSp.Name());
			}
			return;
		}
		if (code == 30)
		{
			if (m_iSpRow == b)
				m_iSpRow = -1;
			else
				m_iSpRow = b;
			RequestRender();
			return;
		}
		if (code == 31)
		{
			m_bSpPicker = !m_bSpPicker;
			RequestRender();
			return;
		}
		DCO_CommanderSpawnerComponent sp = SpEdited();
		if (SpBuilderAct(code, b, sp))
		{
			RequestRender();
			return;
		}
		Managed item = SpItem();
		if (!item || !m_Manager || !sp)
			return;
		if (code == 48 || code == 49)
		{
			int men = SpBMen();
			if (m_sSpBName.Trim().IsEmpty() && code == 48)
			{
				m_sSpMsg = "Give the preset a name first.";
				RequestRender();
				return;
			}
			if (men == 0 || men > 12)
			{
				RequestRender();
				return;
			}
			if (code == 48 && !m_bSpBConfirm && m_aSpPresetNames.Contains(m_sSpBName.Trim()))
			{
				m_bSpBConfirm = true;
				RequestRender();
				return;
			}
			string packed = SpBPack(sp);
			if (code == 49 && m_sSpBName.Trim().IsEmpty())
				packed = "Custom group" + packed;
			m_Manager.DCO_RequestSpawnerAction(item, 22 + (code - 48), 0, 0, packed);
			m_bSpBuilder = false;
			m_bSpBConfirm = false;
			m_sSpMsg = string.Empty;
			GetGame().GetCallqueue().CallLater(RequestRender, 600, false);
			RequestRender();
			return;
		}
		if (code >= 50 && code <= 52)
		{
			if (b < 0 || b >= m_aSpPresetNames.Count())
				return;
			string pname = m_aSpPresetNames[b];
			if (code == 50)
				m_Manager.DCO_RequestSpawnerAction(item, 21, 0, 0, pname);
			else if (code == 51)
				m_Manager.DCO_RequestSpawnerAction(item, 24, 0, 0, pname);
			else
			{
				if (m_sSpBName.Trim().IsEmpty())
				{
					m_bSpBuilder = true;
					m_sSpMsg = "Type the new name in the builder Name box, then press Rename again.";
					RequestRender();
					return;
				}
				m_Manager.DCO_RequestSpawnerAction(item, 25, 0, 0, pname + "|" + m_sSpBName.Trim());
			}
			GetGame().GetCallqueue().CallLater(RequestRender, 800, false);
			return;
		}
		if (code == 32)
		{
			if (b >= 0 && b < m_aSpCatalog.Count())
				m_Manager.DCO_RequestSpawnerAction(item, 10, 0, 0, m_aSpCatalog[b]);
			m_bSpPicker = false;
		}
		else if (code <= 2)
			m_Manager.DCO_RequestSpawnerAction(item, code, 0, 0, string.Empty);
		else
		{
			int row = b / 10000;
			int v = (b % 10000) - 5000;
			if (code == 18)
				m_iSpRow = -1;
			m_Manager.DCO_RequestSpawnerAction(item, code, row, v, string.Empty);
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("dco_settings_spawner action=%1 b=%2", code, b));
		m_sSpLast = string.Empty;
		GetGame().GetCallqueue().CallLater(RequestRender, 600, false);
		RequestRender();
	}

	protected void SpRefresh()
	{
		if (m_iDragRow >= 0 || IsTyping())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fSpRefreshAt)
			return;
		m_fSpRefreshAt = now + 1500.0;
		string snap;
		if (SpSub("CommanderSpawner"))
		{
			DCO_CommanderSpawnerComponent s = SpEdited();
			if (s)
				snap = s.GetRplStatus() + "#" + s.GetRplPool();
		}
		else if (SpSub("CommanderLogistics") && SpCommander())
		{
			foreach (DCO_CommanderSpawnerComponent cs : DCO_CommanderSpawnerComponent.GetAll())
			{
				if (cs)
					snap += cs.GetRplStatus();
			}
		}
		if (snap == m_sSpLast)
			return;
		m_sSpLast = snap;
		RequestRender();
	}

	protected string SpStateName(int st)
	{
		if (st == DCO_ESpawnerState.WAITING)
			return "Waiting";
		if (st == DCO_ESpawnerState.COOLDOWN)
			return "Cooldown";
		if (st == DCO_ESpawnerState.OFF)
			return "Off";
		if (st == DCO_ESpawnerState.STOPPED)
			return "Stopped";
		if (st < 0)
			return "No data";
		return "Ready";
	}

	protected Color SpStateColor(int st)
	{
		if (st == DCO_ESpawnerState.WAITING)
			return m_cAmber;
		if (st == DCO_ESpawnerState.COOLDOWN)
			return Color.FromSRGBA(59, 130, 196, 255);
		if (st == DCO_ESpawnerState.OFF || st == DCO_ESpawnerState.STOPPED)
			return m_cDanger;
		return Color.FromSRGBA(61, 163, 93, 255);
	}

	protected float SpKV(float x, float y, string k, string v, bool dim)
	{
		Color fg = m_cText;
		if (dim)
			fg = m_cDim;
		Cell(m_wDyn, x + 16, y, 180, 24, k, m_cClear, m_cDim, 0, 16, 0);
		Cell(m_wDyn, x + 200, y, m_fSheetW - 220, 24, v, m_cClear, fg, 0, 16, 0);
		return y + 26;
	}

	protected float SpButton(float x, float y, float w, string text, int code, int b, Color bg, Color fg)
	{
		Cell(m_wDyn, x, y, w, 34, text, bg, fg, 3, 17, Act(DCO_ESetAct.SPAWNER, code, b, string.Empty));
		return x + w + 8;
	}

	protected void RenderSpawnerLine(DCO_SetLine line, float y)
	{
		float x0 = m_fSheetX;
		float w = m_fSheetW;
		DCO_CommanderSpawnerComponent s = SpEdited();
		if (line.m_iType == 10)
		{
			RenderReinforcements(line, y);
			return;
		}
		if (!s)
			return;
		if (line.m_iType == 6)
		{
			if (line.m_iRow < 0)
			{
				SpCols(x0, y, "Group", "Role", "Weight", "Quality", "Arrival", "Cost", "Left", m_cDim, 0);
				return;
			}
			array<string> p = {};
			line.m_sText.Split("|", p, false);
			if (p.Count() < 12)
				return;
			Color fg = m_cText;
			int budget = p[10].ToInt();
			int left = p[9].ToInt();
			string leftText = string.Format("%1 of %2", left, budget);
			if (budget < 0)
				leftText = "no limit";
			else if (left == 0)
			{
				leftText = "used up";
				fg = m_cDim;
			}
			string role = SpRoleName(p[1]);
			if (p[1] == "AUTO" && p.Count() > 12)
				role = SpRoleName(p[12]) + " · auto";
			string arrival = "Now";
			if (p[6].ToInt() > 0)
				arrival = p[6] + " min";
			string cost = p[7];
			if (p[8] == "1")
				cost += " GM";
			Color bg = m_cRowBg;
			if (line.m_iRow == m_iSpRow)
				bg = m_cTileOn;
			Cell(m_wDyn, x0, y, w, line.m_fH - 4, string.Empty, bg, fg, 0, 10, Act(DCO_ESetAct.SPAWNER, 30, line.m_iRow, string.Empty));
			Color weightFg = null;
			SpCols(x0, y, p[0], role, p[2], SpQuality(p[3].ToInt(), p[4].ToInt(), p[5].ToInt()), arrival, cost, leftText, fg, Act(DCO_ESetAct.SPAWNER, 30, line.m_iRow, string.Empty), weightFg);
			return;
		}
		if (line.m_iType == 7)
		{
			array<string> q = {};
			line.m_sText.Split("|", q, false);
			if (q.Count() < 12)
				return;
			int r = line.m_iRow;
			Cell(m_wDyn, x0 + 12, y, w - 24, line.m_fH - 6, string.Empty, m_cPanel, m_cText, 0, 10, 0);
			float ex = x0 + 24;
			float ey = y + 8;
			int bud = q[10].ToInt();
			string countText = bud.ToString();
			if (bud < 0)
				countText = "no limit";
			ex = SpButton(ex, ey, 40, "-", 11, SpEnc(r, -1), m_cCell, m_cText);
			Cell(m_wDyn, ex, ey, 90, 34, "Count " + countText, m_cClear, m_cText, 2, 16, 0);
			ex += 98;
			ex = SpButton(ex, ey, 40, "+", 11, SpEnc(r, 1), m_cCell, m_cText);
			ex = SpButton(ex, ey, 110, "No limit", 20, SpEnc(r, 0), m_cCell, m_cText);
			int wgt = q[2].ToInt();
			ex = SpButton(ex, ey, 40, "-", 19, SpEnc(r, wgt - 1), m_cCell, m_cText);
			Cell(m_wDyn, ex, ey, 100, 34, "Weight " + wgt.ToString(), m_cClear, m_cText, 2, 16, 0);
			ex += 108;
			ex = SpButton(ex, ey, 40, "+", 19, SpEnc(r, wgt + 1), m_cCell, m_cText);
			int cst = q[7].ToInt();
			ex = SpButton(ex, ey, 40, "-", 17, SpEnc(r, Math.Max(cst - 1, 0)), m_cCell, m_cText);
			Cell(m_wDyn, ex, ey, 90, 34, "Cost " + cst.ToString(), m_cClear, m_cText, 2, 16, 0);
			ex += 98;
			ex = SpButton(ex, ey, 40, "+", 17, SpEnc(r, cst + 1), m_cCell, m_cText);
			if (q[8] == "1")
				SpButton(ex, ey, 130, "Back to auto", 17, SpEnc(r, -1), m_cCell, m_cText);
			ex = x0 + 24;
			ey += 42;
			array<string> roles = {"Auto", "Assault", "Fire team", "AT", "Recon", "Motorized"};
			array<string> roleKeys = {"AUTO", "ASSAULT", "FIRE_TEAM", "AT", "RECON", "MOTORIZED"};
			int roleIdx = Math.Max(roleKeys.Find(q[1]), 0);
			ex = SpButton(ex, ey, 160, "Role: " + roles[roleIdx], 12, SpEnc(r, (roleIdx + 1) % 6), m_cCell, m_cText);
			int ev = q[3].ToInt();
			ex = SpButton(ex, ey, 170, DCO_CommanderSpawnerComponent.ExperienceName(ev), 13, SpEnc(r, (ev + 1) % 4), m_cCell, m_cText);
			array<string> morales = {"Low morale", "Normal morale", "High morale"};
			int mv = Math.ClampInt(q[4].ToInt(), 0, 2);
			ex = SpButton(ex, ey, 170, morales[mv], 14, SpEnc(r, (mv + 1) % 3), m_cCell, m_cText);
			array<string> shapes = {"Rested", "Reduced"};
			int sv = Math.ClampInt(q[5].ToInt(), 0, 1);
			ex = SpButton(ex, ey, 120, shapes[sv], 15, SpEnc(r, (sv + 1) % 2), m_cCell, m_cText);
			int av = q[6].ToInt();
			int next = 10;
			if (av >= 40)
				next = 0;
			else if (av >= 25)
				next = 40;
			else if (av >= 10)
				next = 25;
			string at = "Arrival now";
			if (av > 0)
				at = "Arrival " + av.ToString() + " min";
			ex = SpButton(ex, ey, 170, at, 16, SpEnc(r, next), m_cCell, m_cText);
			ex = SpButton(ex, ey, 110, "Remove", 18, SpEnc(r, 0), m_cDanger, m_cWhite);
			SpButton(ex, ey, 90, "Done", 30, r, m_cOrange, m_cDark);
			return;
		}
		if (line.m_iType == 8)
		{
			string label = "+ Add group";
			if (m_bSpPicker)
				label = "Close the list";
			SpButton(x0 + 16, y + 3, 170, label, 31, 0, m_cCell, m_cText);
			Cell(m_wDyn, x0 + 200, y + 3, w - 216, 34, line.m_sText, m_cClear, m_cDim, 0, 16, 0);
			return;
		}
		if (line.m_iType == 11)
		{
			RenderBuilder(line, y, s);
			return;
		}
		if (line.m_iType == 12)
		{
			PRenderPickRow(line, y);
			return;
		}
		if (line.m_iType == 13)
		{
			SpButton(x0 + 32, y + 3, 200, "+ New preset", 100, 0, m_cCell, m_cText);
			return;
		}
		if (line.m_iType == 9)
		{
			if (line.m_iRow < 0)
			{
				Cell(m_wDyn, x0 + 32, y, w - 48, 28, line.m_sText, m_cClear, m_cDim, 0, 16, 0);
				return;
			}
			Cell(m_wDyn, x0 + 32, y, w - 64, 28, line.m_sText, m_cRowBg, m_cText, 0, 16, Act(DCO_ESetAct.SPAWNER, 32, line.m_iRow, string.Empty));
		}
	}

	protected string SpQuality(int exp, int morale, int shape)
	{
		string s = DCO_CommanderSpawnerComponent.ExperienceName(exp);
		if (morale == 0)
			s += " · low morale";
		else if (morale == 2)
			s += " · high morale";
		if (shape == 1)
			s += " · reduced";
		return s;
	}

	protected void SpCols(float x0, float y, string a, string b, string c, string d, string e, string f, string g, Color fg, int id, Color cFg = null)
	{
		Color weightFg = fg;
		if (cFg)
			weightFg = cFg;
		Cell(m_wDyn, x0 + 12, y + 4, 290, 28, a, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 306, y + 4, 120, 28, b, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 430, y + 4, 70, 28, c, m_cClear, weightFg, 2, 16, id);
		Cell(m_wDyn, x0 + 504, y + 4, 230, 28, d, m_cClear, fg, 0, 16, id);
		Cell(m_wDyn, x0 + 738, y + 4, 90, 28, e, m_cClear, fg, 2, 16, id);
		Cell(m_wDyn, x0 + 832, y + 4, 70, 28, f, m_cClear, fg, 2, 16, id);
		Cell(m_wDyn, x0 + 906, y + 4, 120, 28, g, m_cClear, fg, 2, 16, id);
	}

	protected void RenderReinforcements(DCO_SetLine line, float y)
	{
		AICommander_BaseComponent cmd = SpCommander();
		if (!cmd)
			return;
		float x0 = m_fSheetX;
		float w = m_fSheetW;
		Cell(m_wDyn, x0, y, w, line.m_fH - 6, string.Empty, m_cRowBg, m_cText, 0, 10, 0);
		Cell(m_wDyn, x0 + 12, y + 8, w - 24, 26, "Reinforcements", m_cClear, m_cOrange, 1, 18, 0);
		float ky = y + 40;
		string men = "-";
		string pts = "-";
		int mi;
		if (m_mRowByClass.Find("DCO_CommanderStockAttribute", mi))
			men = SpStockText(m_aRows[mi].m_vCur[0], "men");
		int pi;
		if (m_mRowByClass.Find("DCO_CommanderPointsStockAttribute", pi))
			pts = SpStockText(m_aRows[pi].m_vCur[0], "points");
		string request = "No request (no spawner answers requests)";
		foreach (DCO_CommanderSpawnerComponent ls : DCO_CommanderSpawnerComponent.GetAll())
		{
			if (!ls || ls.StatusField(1) != cmd.GetCommanderUID())
				continue;
			string liveMen = ls.StatusField(7);
			string livePts = ls.StatusField(8);
			if (!liveMen.IsEmpty() && !men.Contains("(live)"))
				men = liveMen + " (live)";
			if (!livePts.IsEmpty() && !pts.Contains("(live)"))
				pts = livePts + " (live)";
			string rq = ls.StatusField(15);
			if (!rq.IsEmpty() && rq != "-")
			{
				request = rq;
				request.Replace("Request: ", "");
				break;
			}
		}
		ky = SpKV(x0, ky, "Stock · men", men, false);
		ky = SpKV(x0, ky, "Stock · points", pts, false);
		ky = SpKV(x0, ky, "Request", request, false);
		ky = SpKV(x0, ky, string.Empty, "Two separate counters, each -1 = unlimited. A spawner set to Men takes only men, set to Points takes only points.", true);
		string uid = cmd.GetCommanderUID();
		vector cpos = cmd.GetOwner().GetOrigin();
		foreach (DCO_CommanderSpawnerComponent cs : DCO_CommanderSpawnerComponent.GetAll())
		{
			if (!cs || !cs.GetOwner() || cs.StatusField(1) != uid)
				continue;
			string name = cs.Title();
			string text = string.Format("%1 · %2 · %3 · %4 m", name, SpStateName(cs.GetRplState()), cs.StatusField(2), Math.Round(vector.DistanceXZ(cpos, cs.GetOwner().GetOrigin())));
			Cell(m_wDyn, x0 + 16, ky, w - 32, 24, text, m_cClear, SpStateColor(cs.GetRplState()), 0, 16, 0);
			ky += 26;
		}
	}

	protected string SpStockText(float v, string unit)
	{
		if (v < 0)
			return "unlimited " + unit;
		return string.Format("%1 %2 left", Math.Round(v), unit);
	}

	protected void Rule(float x, float y, float w)
	{
		Cell(m_wDyn, x, y, w, 1, string.Empty, m_cRule, m_cText, 0, 10, 0);
	}

	protected Widget GradCell(float x, float y, float w, float h, int id)
	{
		return Cell(m_wDyn, x, y, w, h, string.Empty, m_cWarm, m_cText, 0, 10, id);
	}

	protected void SelBg(float x, float y, float w, float h, int id)
	{
		GradCell(x, y, w, h, id);
		Cell(m_wDyn, x, y, 3, h, string.Empty, m_cOrange, m_cText, 0, 10, id);
	}

	protected float TextW(string text, float perChar)
	{
		return text.Length() * perChar + 12;
	}

	protected float KeyBadge(float x, float y, string key, bool on, int id)
	{
		float w = Math.Max(26, key.Length() * 9 + 12);
		Color bg = m_cOrange;
		Color fg = m_cDark;
		if (!on)
		{
			bg = Color.FromSRGBA(68, 73, 77, 255);
			fg = Color.FromSRGBA(138, 143, 147, 255);
		}
		Cell(m_wDyn, x, y, w, 22, key, bg, fg, 3, 14, id);
		return w;
	}

	protected float KeyHint(float x, float y, string key, string label, bool on, int id, bool bold = false, Color labelColor = null)
	{
		float bw = KeyBadge(x, y, key, on, id);
		int size = 17;
		int style = 0;
		float perChar = 9.2;
		if (bold)
		{
			size = 19;
			style = 1;
			perChar = 10.4;
		}
		Color fg = m_cInk;
		if (!on)
			fg = Color.FromSRGBA(135, 138, 140, 255);
		if (labelColor)
			fg = labelColor;
		float lw = TextW(label, perChar);
		Cell(m_wDyn, x + bw + 6, y - 4, lw, 30, label, m_cClear, fg, style, size, id);
		return x + bw + 6 + lw;
	}

	protected Widget TagCell(float x, float y, string text, Color fg)
	{
		float w = TextW(text, 7.8);
		return Cell(m_wDyn, x, y, w, 20, text, m_cTagBg, fg, 2, 14, 0);
	}

	protected void CheckBox(float x, float y, bool on, int id)
	{
		if (on)
		{
			Cell(m_wDyn, x, y, 14, 14, string.Empty, m_cOrange, m_cText, 0, 10, id);
			return;
		}
		Cell(m_wDyn, x, y, 14, 14, string.Empty, m_cDim, m_cText, 0, 10, id);
		Cell(m_wDyn, x + 1, y + 1, 12, 12, string.Empty, m_cSurface, m_cText, 0, 10, id);
	}

	protected void ScrollBar(float x, float y, float h, int scroll, int maxScroll)
	{
		if (maxScroll <= 0)
			return;
		Cell(m_wDyn, x, y, 2, h, string.Empty, m_cTrack, m_cText, 0, 10, 0);
		float thumb = Math.Max(24, h / (maxScroll + 1));
		float ty = y + (h - thumb) * scroll / maxScroll;
		Cell(m_wDyn, x, ty, 2, thumb, string.Empty, m_cOrange, m_cText, 0, 10, 0);
	}

	protected float TotalW()
	{
		if (m_bHelpOpen)
			return m_fWinW + m_fHelpW;
		return m_fWinW;
	}

	protected void ResizeWindow()
	{
		float w = TotalW();
		FrameSlot.SetPos(m_wWindow, -w * 0.5, -m_fWinH * 0.5);
		FrameSlot.SetSize(m_wWindow, w, m_fWinH);
	}

	protected bool IsTyping()
	{
		if (m_wSearch && m_wSearch.IsInWriteMode())
			return true;
		foreach (EditBoxWidget rowBox : m_aTextBox)
		{
			if (rowBox && rowBox.IsInWriteMode())
				return true;
		}
		return m_wPresetBox && m_wPresetBox.IsInWriteMode();
	}

	protected void RenderChrome()
	{
		string title = T("set_editing") + "  ·  " + m_sTarget;
		if (m_iItems > 1)
			title += "  <color rgba=\"201,151,111,255\">×" + m_iItems.ToString() + "</color>";
		Cell(m_wDyn, m_fPad, 12, 740, 46, title, m_cClear, m_cInk, 1, 30, 0);
		RenderSpinner();

		float ty = m_fToolY + 8;
		IconCell(m_wDyn, m_fPad, ty + 5, 20, m_sIconSet, "search", m_cDim, 0);
		Rule(m_fPad, ty + 30, 320);

		float tx = m_fPad + 336;
		Color simpleBg = m_cTab;
		Color simpleFg = m_cDim;
		int simpleStyle = 2;
		Color advBg = m_cTab;
		Color advFg = m_cDim;
		int advStyle = 2;
		Color expBg = m_cTab;
		Color expFg = m_cDim;
		int expStyle = 2;
		if (m_bSimple)
		{
			simpleBg = m_cOrange;
			simpleFg = m_cWhite;
			simpleStyle = 3;
		}
		else if (m_bExpert)
		{
			expBg = m_cOrange;
			expFg = m_cWhite;
			expStyle = 3;
		}
		else
		{
			advBg = m_cOrange;
			advFg = m_cWhite;
			advStyle = 3;
		}
		string simpleTip = T("set_tip_simple");
		if (m_aSimpleIdx.IsEmpty())
		{
			simpleFg = Color.FromSRGBA(90, 95, 99, 255);
			simpleTip = T("set_tip_simple_none");
		}
		Cell(m_wDyn, tx, ty, 96, 30, T("set_mode_simple"), simpleBg, simpleFg, simpleStyle, 16, Act(DCO_ESetAct.MODE, 1, 0, simpleTip));
		Cell(m_wDyn, tx + 98, ty, 96, 30, T("set_mode_advanced"), advBg, advFg, advStyle, 16, Act(DCO_ESetAct.MODE, 2, 0, T("set_tip_advanced")));
		Cell(m_wDyn, tx + 196, ty, 96, 30, T("set_mode_expert"), expBg, expFg, expStyle, 16, Act(DCO_ESetAct.MODE, 3, 0, T("set_tip_expert")));

		float cx = tx + 306;
		int onlyId = Act(DCO_ESetAct.ONLYMOD, 0, 0, T("set_tip_only_mod"));
		CheckBox(cx, ty + 8, m_bOnlyMod, onlyId);
		Cell(m_wDyn, cx + 20, ty, 150, 30, T("set_only_mod"), m_cClear, m_cText, 0, 16, onlyId);
		if (m_bPad)
		{
			PadGlyph(cx + 172, ty + 1, "view", true, onlyId);
			Cell(m_wDyn, cx + 204, ty, 70, 30, T("set_pad_hold"), m_cClear, m_cDim, 0, 15, onlyId);
		}

		bool hasPresets = !PresetScope().IsEmpty();
		string presetTip = T2("set_tip_presets", "s", PresetScopeLabel());
		if (!hasPresets)
			presetTip = T("set_tip_no_presets");
		int presetId = Act(DCO_ESetAct.PRESET, 0, 0, presetTip);
		string presetName = m_sPresetName;
		if (presetName.IsEmpty())
			presetName = T("set_preset_none");
		float right = m_fWinW - m_fPad;
		if (m_bPad)
			PadGlyph(right - 28, ty + 1, "Y", hasPresets, presetId);
		else
			KeyBadge(right - 26, ty + 4, "P", hasPresets, presetId);
		Cell(m_wDyn, right - 380, ty, 346, 30, "<color rgba=\"143,149,154,255\">" + T("set_preset_label") + "</color>   " + presetName, m_cClear, m_cInk, 0, 17, presetId);

		Cell(m_wDyn, 0, m_fRuleY, TotalW(), 2, string.Empty, m_cOrange, m_cText, 0, 10, 0);
	}

	protected void RenderSpinner()
	{
		array<string> values = {};
		string label;
		int current;
		bool locked;
		if (m_bGlobalTarget && !m_aFactionKeys.IsEmpty())
		{
			label = T("set_scope_faction");
			values.Insert(T("set_faction_base"));
			values.InsertAll(m_aFactionNames);
			current = m_aFactionKeys.Find(m_sFaction) + 1;
		}
		else if (m_iTargetKind == 2 && m_bMembersReady && !m_aMemberIds.IsEmpty())
		{
			label = T("set_scope_members");
			values.Insert(T("set_members_all"));
			foreach (int mi, string memberName : m_aMemberNames)
			{
				string shown = memberName;
				if (MemberHasOwn(mi))
					shown += "  <color rgba=\"201,151,111,255\">*</color>";
				values.Insert(shown);
			}
			current = m_iMember;
			locked = m_iItems > 1;
		}
		else
		{
			return;
		}
		float right = m_fWinW - m_fPad;
		float valW = 230;
		float y = 18;
		int prevId = 0;
		int nextId = 0;
		Color arrow = m_cText;
		if (locked)
			arrow = Color.FromSRGBA(105, 110, 114, 255);
		else
		{
			prevId = Act(DCO_ESetAct.SPIN, -1, 0, string.Empty);
			nextId = Act(DCO_ESetAct.SPIN, 1, 0, string.Empty);
		}
		IconCell(m_wDyn, right - 20, y + 6, 18, m_sIconRight, string.Empty, arrow, nextId);
		Cell(m_wDyn, right - 24 - valW, y, valW, 30, values[current], m_cClear, m_cInk, 2, 19, nextId);
		IconCell(m_wDyn, right - 46 - valW, y + 6, 18, m_sIconLeft, string.Empty, arrow, prevId);
		Cell(m_wDyn, right - 46 - valW - 130, y, 124, 30, label, m_cClear, m_cDim, 0, 19, 0);
		int count = values.Count();
		float dashW = 12;
		float totalDash = 1.0 * count * dashW + (count - 1) * 3;
		float dx = right - 24 - valW * 0.5 - totalDash * 0.5;
		for (int i = 0; i < count; i++)
		{
			Color dash = Color.FromSRGBA(110, 116, 121, 255);
			if (i == current)
				dash = m_cOrange;
			Cell(m_wDyn, dx + i * (dashW + 3), y + 33, dashW, 2, string.Empty, dash, m_cText, 0, 10, 0);
		}
	}

	protected bool SubPending(DCO_SetSub sub)
	{
		foreach (int r : sub.m_aRows)
		{
			if (RowVisible(m_aRows[r]) && IsPending(m_aRows[r]))
				return true;
		}
		return false;
	}

	protected string FolderTitle(DCO_SetFolder folder)
	{
		string name = folder.m_sName;
		if (folder.m_bMember)
			name = T("set_members_block");
		name.ToUpper();
		return name;
	}

	protected void RenderTree()
	{
		float x = m_fTreeX;
		float w = m_fTreeW;
		float top = m_fBodyY;
		float bottom = m_fBodyY + m_fBodyH;
		if (SimpleView())
		{
			string heading = T("set_mode_simple");
			heading.ToUpper();
			Cell(m_wDyn, x + 2, top + 6, w - 4, 24, heading, m_cClear, m_cDim, 0, 15, 0);
			Rule(x, top + 30, w);
			Widget info = Cell(m_wDyn, x + 2, top + 38, w - 4, 260, T("set_simple_info"), m_cClear, m_cDim, 0, 16, 0);
			WrapCell(info);
			return;
		}
		if (m_bPad)
		{
			PadGlyph(x, top + 4, "LB", true, 0);
			Cell(m_wDyn, x + 32, top + 2, w - 64, 30, T("set_pad_section"), m_cClear, m_cDim, 2, 16, 0);
			PadGlyph(x + w - 28, top + 4, "RB", true, 0);
			top += 36;
		}
		float sideH = 0;
		if (SpSideOn())
		{
			sideH = 610;
			bottom -= sideH;
		}

		array<int> kinds = {};
		array<int> idx = {};
		array<float> heights = {};
		kinds.Insert(0);
		idx.Insert(-1);
		heights.Insert(44);
		foreach (int f, DCO_SetFolder folder : m_aFolders)
		{
			if (!FolderVisible(folder))
				continue;
			kinds.Insert(1);
			idx.Insert(f);
			heights.Insert(40);
			if (DCO_SettingsPrefs.IsClosed(folder.m_sKey))
				continue;
			foreach (int s : folder.m_aSubs)
			{
				kinds.Insert(2);
				idx.Insert(s);
				heights.Insert(m_fTreeLineH);
			}
		}

		float total = 0;
		int start = kinds.Count();
		for (int back = kinds.Count() - 1; back >= 0; back--)
		{
			if (total + heights[back] > bottom - top)
				break;
			total += heights[back];
			start = back;
		}
		m_iMaxTreeScroll = (int)Math.Max(0, start);
		if (m_bTreeFollow)
		{
			m_bTreeFollow = false;
			int selPos = -1;
			foreach (int pos, int kindAt : kinds)
			{
				if ((kindAt == 0 && m_iSel < 0) || (kindAt == 2 && idx[pos] == m_iSel))
					selPos = pos;
			}
			if (selPos >= 0 && selPos < m_iTreeScroll)
				m_iTreeScroll = selPos;
			else if (selPos >= 0)
			{
				float used = 0;
				for (int hp = m_iTreeScroll; hp <= selPos; hp++)
				{
					used += heights[hp];
				}
				while (used > bottom - top && m_iTreeScroll < selPos)
				{
					used -= heights[m_iTreeScroll];
					m_iTreeScroll++;
				}
			}
		}
		m_iTreeScroll = Math.ClampInt(m_iTreeScroll, 0, m_iMaxTreeScroll);

		float y = top;
		bool browse = !SearchMode() && !m_bOnlyMod;
		for (int i = m_iTreeScroll; i < kinds.Count(); i++)
		{
			if (y + heights[i] > bottom)
				break;
			int kind = kinds[i];
			int index = idx[i];
			if (kind == 0)
			{
				bool favSel = m_iSel < 0 && browse;
				int favId = Act(DCO_ESetAct.FAVS, 0, 0, string.Empty);
				if (favSel)
					SelBg(x, y, w, m_fTreeLineH, favId);
				else
					Cell(m_wDyn, x, y, w, m_fTreeLineH, string.Empty, m_cClear, m_cText, 0, 10, favId);
				IconCell(m_wDyn, x + 12, y + 9, 18, m_sIconSet, "favourite", m_cOrange, favId);
				Color favFg = m_cText;
				if (favSel)
					favFg = m_cInk;
				Cell(m_wDyn, x + 36, y, w - 46, m_fTreeLineH, T("set_favorites"), m_cClear, favFg, 0, 18, favId);
			}
			else if (kind == 1)
			{
				DCO_SetFolder folder = m_aFolders[index];
				bool folderPending = false;
				foreach (int fs : folder.m_aSubs)
				{
					if (SubPending(m_aSubs[fs]))
						folderPending = true;
				}
				int folderId = Act(DCO_ESetAct.FOLDER, index, 0, string.Empty);
				float folderNameW = w - 22;
				if (FolderAllBeta(folder))
				{
					string folderBeta = T("set_beta_tag");
					float folderTagW = TextW(folderBeta, 7.8);
					TagCell(x + w - 22 - folderTagW, y + 9, folderBeta, m_cBeta);
					folderNameW -= folderTagW + 6;
				}
				Cell(m_wDyn, x + 2, y + 6, folderNameW, 26, FolderTitle(folder), m_cClear, m_cDim, 0, 15, folderId);
				if (folderPending)
					IconCell(m_wDyn, x + w - 16, y + 14, 10, m_sIconDot, string.Empty, m_cOrange, folderId);
				Rule(x, y + 32, w);
			}
			else
			{
				DCO_SetSub sub = m_aSubs[index];
				bool sel = m_iSel == index && browse;
				int subId = Act(DCO_ESetAct.SUB, index, 0, string.Empty);
				if (sel)
					SelBg(x, y, w, m_fTreeLineH, subId);
				else
					Cell(m_wDyn, x, y, w, m_fTreeLineH, string.Empty, m_cClear, m_cText, 0, 10, subId);
				Color subFg = m_cText;
				if (sel)
					subFg = m_cInk;
				float subNameW = w - 32;
				if (SubAllBeta(sub))
				{
					string subBeta = T("set_beta_tag");
					float subTagW = TextW(subBeta, 7.8);
					TagCell(x + w - 22 - subTagW, y + m_fTreeLineH * 0.5 - 10, subBeta, m_cBeta);
					subNameW -= subTagW + 6;
				}
				Cell(m_wDyn, x + 12, y, subNameW, m_fTreeLineH, sub.m_sShort, m_cClear, subFg, 0, 18, subId);
				if (SubPending(sub))
					IconCell(m_wDyn, x + w - 16, y + 13, 10, m_sIconDot, string.Empty, m_cOrange, subId);
			}
			y += heights[i];
		}
		ScrollBar(x + w + 8, top, bottom - top, m_iTreeScroll, m_iMaxTreeScroll);
		if (sideH > 0)
			RenderSpawnerSide(x, bottom + 12, w);
	}

	protected float SideLine(float x, float y, float w, string text, Color fg, int lines)
	{
		Widget c = Cell(m_wDyn, x, y, w, 20 * lines, text, m_cClear, fg, 0, 16, 0);
		if (lines > 1)
			WrapCell(c);
		return y + 20 * lines + 2;
	}

	protected bool SpSideOn()
	{
		return SpSub("CommanderSpawner") && SpEdited();
	}

	protected void RenderSpawnerSide(float x, float y, float w)
	{
		DCO_CommanderSpawnerComponent s = SpEdited();
		if (!s)
			return;
		Rule(x, y - 4, w);
		int st = s.GetRplState();
		Cell(m_wDyn, x, y, w, 28, SpStateName(st), SpStateColor(st), m_cDark, 3, 17, 0);
		y += 32;
		y = SideLine(x, y, w, s.StatusField(0), m_cText, 3);
		y = SideLine(x, y, w, "Preset: " + s.StatusField(14), m_cDim, 1);
		string reinf = s.StatusField(15);
		if (!reinf.IsEmpty() && reinf != "-")
		{
			if (reinf.Contains("URGENT"))
			{
				Cell(m_wDyn, x, y, w, 22, "Reinforcement: " + reinf, Color.FromSRGBA(12, 12, 12, 255), m_cDanger, 0, 16, 0);
				y += 24;
			}
			else
			{
				Color reinfFg = m_cText;
				if (reinf.Contains("PRIORITY"))
					reinfFg = m_cDanger;
				else if (reinf.Contains("ROUTINE"))
					reinfFg = m_cAmber;
				y = SideLine(x, y, w, "Reinforcement: " + reinf, reinfFg, 1);
			}
		}
		y = SideLine(x, y, w, "Feeds: " + s.StatusField(1), m_cText, 1);
		y = SideLine(x, y, w, "Spawned: " + s.StatusField(2), m_cText, 1);
		y = SideLine(x, y, w, "Last: " + s.StatusField(3), m_cText, 1);
		y = SideLine(x, y, w, "Next allowed: " + s.StatusField(4), m_cText, 1);
		array<string> orders = {};
		string ostr = s.StatusField(5);
		if (!ostr.IsEmpty())
			ostr.Split(";", orders, true);
		y = SideLine(x, y, w, string.Format("On the way: %1", orders.Count()), m_cText, 1);
		foreach (int oi, string o : orders)
		{
			if (oi >= 3)
				break;
			y = SideLine(x + 8, y, w - 8, o, m_cDim, 1);
		}
		int mode = SpInt(s, 9);
		Color menFg = m_cText;
		if (mode != 1)
			menFg = m_cDim;
		Color ptsFg = m_cText;
		if (mode != 2)
			ptsFg = m_cDim;
		y = SideLine(x, y, w, "Stock: " + s.StatusField(7), menFg, 1);
		y = SideLine(x, y, w, "Stock: " + s.StatusField(8), ptsFg, 1);
		y = SideLine(x, y, w, s.StatusField(6), m_cText, 2);
		string note = "Takes nothing from the commander stock.";
		if (mode == 1)
			note = "Takes men from the commander stock.";
		else if (mode == 2)
			note = "Takes points from the commander stock.";
		int shared = SpInt(s, 11);
		if (shared > 0 && mode != 0)
			note += string.Format(" Shared with %1 other %2.", shared, DCO_CommanderSpawnerComponent.Plural(shared, "spawner", "spawners"));
		y = SideLine(x, y, w, note, m_cDim, 2);
		y += 4;
		Cell(m_wDyn, x, y, w, 30, "Spawn now", m_cOrange, m_cDark, 3, 16, Act(DCO_ESetAct.SPAWNER, 0, 0, string.Empty));
		y += 34;
		if (st == DCO_ESpawnerState.WAITING && s.StatusField(10) == "faction_cap")
		{
			y = SideLine(x, y, w, "The faction has reached its AI soldier cap. It spawns when there are fewer.", m_cAmber, 2);
			y += 4;
		}
		else if (st == DCO_ESpawnerState.WAITING)
		{
			Cell(m_wDyn, x, y, w, 30, "Spawn anyway", m_cAmber, m_cDark, 3, 16, Act(DCO_ESetAct.SPAWNER, 1, 0, string.Empty));
			y += 34;
		}
		string toggle = "Turn off";
		if (s.StatusField(12) == "gm")
			toggle = "Turn on";
		Cell(m_wDyn, x, y, w, 30, toggle, m_cCell, m_cText, 3, 16, Act(DCO_ESetAct.SPAWNER, 2, 0, string.Empty));
		y += 34;
		Cell(m_wDyn, x, y, w, 30, "Show on map", m_cCell, m_cText, 3, 16, Act(DCO_ESetAct.SPAWNER, 61, 0, string.Empty));
	}

	protected string SpRoleName(string r)
	{
		if (r == "FIRE_TEAM")
			return "Fire team";
		if (r == "AT")
			return "AT";
		if (r == "RECON")
			return "Recon";
		if (r == "MOTORIZED")
			return "Motorized";
		if (r == "ASSAULT")
			return "Assault";
		if (r == "AUTO")
			return "Auto";
		return r;
	}

	protected void RenderSheet()
	{
		BuildLines();
		float top = m_fBodyY;
		float bottom = m_fBodyY + m_fBodyH;
		float total = 0;
		int start = m_aLines.Count();
		for (int i = m_aLines.Count() - 1; i >= 0; i--)
		{
			if (total + m_aLines[i].m_fH > bottom - top)
				break;
			total += m_aLines[i].m_fH;
			start = i;
		}
		m_iMaxScroll = (int)Math.Max(0, start);
		m_iScroll = Math.ClampInt(m_iScroll, 0, m_iMaxScroll);

		float y = top;
		m_iShownFirst = m_iScroll;
		m_iShownLast = m_iScroll - 1;
		for (int li = m_iScroll; li < m_aLines.Count(); li++)
		{
			DCO_SetLine line = m_aLines[li];
			if (y + line.m_fH > bottom)
				break;
			RenderLine(line, y);
			m_iShownLast = li;
			y += line.m_fH;
		}
		ScrollBar(m_fSheetX + m_fSheetW + 8, top, bottom - top, m_iScroll, m_iMaxScroll);
	}

	protected int SectionPending()
	{
		int n = 0;
		foreach (DCO_SetLine line : m_aLines)
		{
			if (line.m_iType == 1 && line.m_iPart == 0 && IsPending(m_aRows[line.m_iRow]))
				n++;
			if (line.m_iType == 3)
			{
				DCO_SimpleSetting setting = SimpleAt(line.m_iRow);
				if (setting && SimplePending(setting))
					n++;
			}
		}
		return n;
	}

	protected void RenderLine(DCO_SetLine line, float y)
	{
		float x0 = m_fSheetX;
		if (line.m_iType == 0)
		{
			Cell(m_wDyn, x0, y + 2, m_fLabelW, 34, line.m_sText, m_cClear, m_cOrange, 1, 20, 0);
			Cell(m_wDyn, x0 + m_fCellX, y + 8, 7 * m_fCellStep - 3, 26, line.m_sHint, m_cClear, m_cDim, 2, 16, 0);
			Rule(x0 + m_fCellX, y + 35, 7 * m_fCellStep - 3);
			Color helpBg = m_cClear;
			Color helpFg = m_cDim;
			if (m_bHelpOpen)
			{
				helpBg = m_cOrange;
				helpFg = m_cDark;
			}
			Cell(m_wDyn, x0 + m_fResetX - 30, y + 7, 26, 24, "?", helpBg, helpFg, 3, 16, Act(DCO_ESetAct.HELP, 0, 0, T("set_tip_help")));
			int pendingRows = SectionPending();
			Color revertColor = Color.FromSRGBA(105, 110, 114, 255);
			int revertId = 0;
			if (pendingRows > 0)
			{
				revertColor = m_cText;
				revertId = Act(DCO_ESetAct.REVERT_SEC, 0, 0, T("set_tip_revert_sec"));
			}
			IconCell(m_wDyn, x0 + m_fResetX + 4, y + 8, 22, m_sIconSet, "restart", revertColor, revertId);
			return;
		}
		if (line.m_iType == 2)
		{
			Widget emptyCell = Cell(m_wDyn, x0 + 12, y + 24, m_fSheetW - 24, 56, line.m_sText, m_cClear, m_cDim, 0, 19, 0);
			WrapCell(emptyCell);
			if (SearchMode())
				Cell(m_wDyn, x0 + 12, y + 84, 240, 28, T("set_clear_search"), m_cClear, m_cOrange, 0, 17, Act(DCO_ESetAct.CLEAR_SEARCH, 0, 0, string.Empty));
			return;
		}
		if (line.m_iType == 3)
		{
			RenderSimpleLine(line, y);
			return;
		}
		if (line.m_iType == 4)
		{
			Cell(m_wDyn, x0 + 8, y + 8, m_fSheetW - 16, 24, line.m_sText, m_cClear, m_cOrange, 1, 17, 0);
			Rule(x0 + 8, y + 33, m_fSheetW - 16);
			return;
		}
		if (line.m_iType >= 5 && line.m_iType <= 28)
		{
			RenderSpawnerLine(line, y);
			return;
		}
		RenderRow(line, y);
	}

	protected bool RowInherits(DCO_SetRow row)
	{
		if (FactionMode())
			return FactionRow(row) && !IsOverridden(row);
		if (!MemberScope() && CommanderDocRow(row))
		{
			if (IsPending(row))
				return false;
			if (DCO_CmdDocPresetAttribute.Cast(row.m_Attr))
				return row.m_vCur[0] == 0;
			return row.m_vOrig[1] == 0;
		}
		if (MemberScope() && row.m_bMember)
		{
			vector groupValue;
			if (!GroupValue(row, groupValue))
				return false;
			return Same(ViewValue(row), groupValue);
		}
		return false;
	}

	protected string InheritTag(DCO_SetRow row)
	{
		if (FactionMode())
		{
			vector builtin;
			if (row && DCO_DoctrineGM.BuiltinRowValue(row.m_Attr, m_sFaction, builtin))
				return T("set_builtin_tag");
			return T("set_global_tag");
		}
		if (row && CommanderDocRow(row))
			return T("set_faction_tag");
		return T("set_group_tag");
	}

	protected void RegisterLine(Widget focusW, Widget starW, int rowKey, bool fav)
	{
		m_aLineFocusW.Insert(focusW);
		m_aLineStarW.Insert(starW);
		m_aLineRowIdx.Insert(rowKey);
		m_aLineFav.Insert(fav);
		bool focused = rowKey == m_iFocusRow;
		if (focusW)
			focusW.SetVisible(focused);
		if (starW)
			starW.SetVisible(fav || focused);
	}

	protected void RenderRow(DCO_SetLine line, float y)
	{
		DCO_SetRow row = m_aRows[line.m_iRow];
		float x0 = m_fSheetX;
		float h = line.m_fH - 3;
		bool first = line.m_iPart == 0;
		bool inherit = RowInherits(row);
		bool pending = IsPending(row);
		int rowId = Act(DCO_ESetAct.ROW, line.m_iRow, 0, string.Empty);
		Cell(m_wDyn, x0, y, m_fSheetW, h, string.Empty, m_cRowBg, m_cText, 0, 10, rowId);
		Widget focusW = GradCell(x0, y, m_fSheetW, h, rowId);
		Widget starW = null;
		bool fav = DCO_SettingsPrefs.IsFav(row.m_sClass);
		float cy = y + (h - m_fTileH) * 0.5;
		if (first)
		{
			if (pending)
				IconCell(m_wDyn, x0 + 8, y + h * 0.5 - 5, 10, m_sIconDot, string.Empty, m_cOrange, rowId);
			Color starColor = m_cDim;
			if (fav)
				starColor = m_cOrange;
			starW = IconCell(m_wDyn, x0 + m_fLabelW - 24, y + h * 0.5 - 9, 18, m_sIconSet, "favourite", starColor, Act(DCO_ESetAct.STAR, line.m_iRow, 0, string.Empty));

			float tagX = x0 + m_fLabelW - 30;
			if (row.m_bBeta)
			{
				string betaTag = T("set_beta_tag");
				tagX -= TextW(betaTag, 7.8) + 4;
				TagCell(tagX, y + h * 0.5 - 10, betaTag, m_cBeta);
			}
			string heavy;
			if (row.m_iKind != DCO_ESetKind.TEXT && !ShowsMixed(row))
			{
				vector current = ViewValue(row);
				heavy = DCO_SettingsTree.HeavyReason(row.m_sClass, current[0]);
			}
			if (!heavy.IsEmpty())
			{
				string heavyTag = T("set_heavy_tag");
				tagX -= TextW(heavyTag, 7.8) + 4;
				TagCell(tagX, y + h * 0.5 - 10, heavyTag, m_cAmber);
			}
			if (row.m_iKind == DCO_ESetKind.DROP && !ShowsMixed(row) && !DropMatches(row))
			{
				string customTag = T("set_custom_tag");
				tagX -= TextW(customTag, 7.8) + 4;
				TagCell(tagX, y + h * 0.5 - 10, customTag, m_cOrange);
			}
			if (inherit)
			{
				string inheritTag = InheritTag(row);
				tagX -= TextW(inheritTag, 7.8) + 4;
				TagCell(tagX, y + h * 0.5 - 10, inheritTag, m_cTagFg);
			}

			Color labelColor = m_cInk;
			if (row.m_bDisabled)
				labelColor = Color.FromSRGBA(135, 138, 140, 255);
			else if (inherit)
				labelColor = Color.FromSRGBA(241, 241, 239, 140);
			else if (m_bPad && line.m_iRow == m_iFocusRow)
				labelColor = m_cWhite;
			string label = row.m_sLabel;
			if (line.m_bPath && row.m_iSub >= 0)
				label += "  <color rgba=\"143,149,154,255\">·  " + m_aSubs[row.m_iSub].m_sShort + "</color>";
			float labelW = tagX - x0 - 26;
			int labelLen = row.m_sLabel.Length();
			if (line.m_bPath && row.m_iSub >= 0)
				labelLen += 4 + m_aSubs[row.m_iSub].m_sShort.Length();
			int labelSize = 19;
			if (labelLen * 8.6 > labelW - 6)
				labelSize = 17;
			Widget labelCell = Cell(m_wDyn, x0 + 22, y, labelW, h, label, m_cClear, labelColor, 0, labelSize, rowId);
			if (labelCell && labelSize != 19)
			{
				RichTextWidget labelText = RichTextWidget.Cast(labelCell.FindAnyWidget("Text"));
				if (labelText)
					labelText.SetTextWrapping(true);
			}
		}
		RegisterLine(focusW, starW, line.m_iRow, fav);

		switch (row.m_iKind)
		{
			case DCO_ESetKind.CHECK:
			{
				if (first)
				{
					vector checkValue = ViewValue(row);
					bool on = checkValue[0] != 0;
					Tile(row, line.m_iRow, 0, cy, T("set_on"), !ShowsMixed(row) && on, 1, inherit);
					Tile(row, line.m_iRow, 1, cy, T("set_off"), !ShowsMixed(row) && !on, 0, inherit);
				}
				break;
			}
			case DCO_ESetKind.DROP:
			case DCO_ESetKind.MULTI:
			{
				int perLine = PerLine(row);
				int firstOption = line.m_iPart * perLine;
				vector optionValue = ViewValue(row);
				int mask = optionValue[0];
				for (int c = 0; c < perLine && firstOption + c < row.m_aNames.Count(); c++)
				{
					int e = firstOption + c;
					bool lit;
					if (row.m_iKind == DCO_ESetKind.DROP)
						lit = Math.AbsFloat(row.m_aValues[e] - optionValue[0]) < 0.0001;
					else
						lit = (mask & Bit(row, e)) != 0;
					if (ShowsMixed(row))
						lit = false;
					Tile(row, line.m_iRow, c, cy, row.m_aNames[e], lit, e, inherit);
				}
				break;
			}
			case DCO_ESetKind.SLIDER:
			{
				RenderSlider(row, line.m_iRow, x0 + m_fCellX, cy, 7 * m_fCellStep - 3, inherit);
				break;
			}
			case DCO_ESetKind.TEXT:
			{
				RenderTextBox(row, line.m_iRow, x0 + m_fCellX, cy, 7 * m_fCellStep - 3, m_fTileH);
				break;
			}
		}

		if (first && pending && !row.m_bDisabled)
			IconCell(m_wDyn, x0 + m_fResetX + 4, y + h * 0.5 - 11, 22, m_sIconSet, "restart", m_cOrange, Act(DCO_ESetAct.REVERT_ROW, line.m_iRow, 0, T("set_tip_revert")));
	}

	protected void Tile(DCO_SetRow row, int rowIndex, int col, float y, string text, bool lit, int valueIndex, bool dim)
	{
		float step = m_fCellStep;
		if (row.m_iKind != DCO_ESetKind.CHECK && Parts(row) > 1)
			step = 7 * m_fCellStep / PerLine(row);
		float w = step - (m_fCellStep - m_fCellW);
		float x = m_fSheetX + m_fCellX + 1.0 * col * step;
		bool multi = row.m_iKind == DCO_ESetKind.MULTI;
		Color bg = m_cCell;
		Color fg = Color.FromSRGBA(218, 220, 222, 255);
		Color line = m_cOrange;
		if (lit)
		{
			bg = m_cTileOn;
			fg = m_cWhite;
		}
		if (dim)
		{
			bg = Color.FromSRGBA(95, 100, 104, 87);
			fg = Color.FromSRGBA(218, 220, 222, 140);
			line = Color.FromSRGBA(255, 255, 255, 140);
			if (lit)
				bg = Color.FromSRGBA(122, 90, 66, 130);
		}
		int id;
		if (row.m_bDisabled)
		{
			fg = Color.FromSRGBA(135, 138, 140, 255);
			id = Act(DCO_ESetAct.ROW, rowIndex, 0, string.Empty);
		}
		else
		{
			id = Act(DCO_ESetAct.CELL, rowIndex, valueIndex, string.Empty);
		}
		if (multi)
		{
			Cell(m_wDyn, x, y, w, m_fTileH, string.Empty, bg, fg, 0, 10, id);
			CheckBox(x + 8, y + (m_fTileH - 14) * 0.5, lit, id);
			int size = 16;
			if (text.Length() > 18)
				size = 14;
			Widget lc = Cell(m_wDyn, x + 28, y, w - 32, m_fTileH, text, m_cClear, fg, 0, size, id);
			RichTextWidget lt;
			if (lc)
				lt = RichTextWidget.Cast(lc.FindAnyWidget("Text"));
			if (lt && size != 16)
				lt.SetTextWrapping(true);
		}
		else
			TileCell(x, y, text, bg, fg, id, w);
		if (lit && !multi)
			Cell(m_wDyn, x, y + m_fTileH - 2, w, 2, string.Empty, line, m_cText, 0, 10, id);
		bool cursor = lit;
		if (multi)
			cursor = valueIndex == m_iFocusCell;
		if (m_bPad && cursor && rowIndex == m_iFocusRow && !row.m_bDisabled)
			Outline(x, y, w, m_fTileH, id);
	}

	protected void Outline(float x, float y, float w, float h, int id)
	{
		Cell(m_wDyn, x - 2, y - 2, w + 4, 2, string.Empty, m_cWhite, m_cText, 0, 10, id);
		Cell(m_wDyn, x - 2, y + h, w + 4, 2, string.Empty, m_cWhite, m_cText, 0, 10, id);
		Cell(m_wDyn, x - 2, y, 2, h, string.Empty, m_cWhite, m_cText, 0, 10, id);
		Cell(m_wDyn, x + w, y, 2, h, string.Empty, m_cWhite, m_cText, 0, 10, id);
	}

	protected float PadGlyph(float x, float y, string name, bool on, int id)
	{
		Color col = m_cWhite;
		if (!on)
			col = Color.FromSRGBA(255, 255, 255, 80);
		IconCell(m_wDyn, x, y, 28, DCO_SettingsChooserUI.PadSet(), DCO_SettingsChooserUI.PadGlyphName(name), col, id);
		return 28;
	}

	protected float PadHint(float x, float y, string glyph, string label, bool on, int id, bool bold = false)
	{
		float gw = PadGlyph(x, y - 4, glyph, on, id);
		int size = 17;
		int style = 0;
		float perChar = 9.2;
		if (bold)
		{
			size = 19;
			style = 1;
			perChar = 10.4;
		}
		Color fg = m_cInk;
		if (!on)
			fg = Color.FromSRGBA(135, 138, 140, 255);
		float lw = TextW(label, perChar);
		Cell(m_wDyn, x + gw + 6, y - 4, lw, 30, label, m_cClear, fg, style, size, id);
		return x + gw + 6 + lw;
	}

	protected void TileCell(float x, float y, string text, Color bg, Color fg, int id, float w = -1)
	{
		if (w <= 0)
			w = m_fCellW;
		int size = 17;
		float fit = w / 9.0;
		if (text.Length() > fit * 2)
			size = 14;
		else if (text.Length() > fit)
			size = 15;
		Widget c = Cell(m_wDyn, x, y, w, m_fTileH, text, bg, fg, 2, size, id);
		if (!c || size == 17)
			return;
		RichTextWidget t = RichTextWidget.Cast(c.FindAnyWidget("TextC"));
		if (t)
			t.SetTextWrapping(true);
	}

	protected float SliderBarW()
	{
		return 7 * m_fCellStep - 3 - 130;
	}

	protected void RenderSlider(DCO_SetRow row, int rowIndex, float x, float y, float w, bool dim)
	{
		float barW = SliderBarW();
		vector v = ViewValue(row);
		bool mixed = ShowsMixed(row);
		Color orange = m_cOrange;
		if (dim)
			orange = Color.FromSRGBA(201, 151, 111, 140);
		int id = Act(DCO_ESetAct.ROW, rowIndex, 0, string.Empty);
		if (row.m_bDisabled)
			orange = Color.FromSRGBA(135, 138, 140, 255);
		else
			id = Act(DCO_ESetAct.SLIDERBAR, rowIndex, 0, string.Empty);
		float by = y + (m_fTileH - 14) * 0.5;
		Cell(m_wDyn, x, y - 4, barW, m_fTileH + 8, string.Empty, m_cClear, m_cText, 0, 10, id);
		Cell(m_wDyn, x, by, barW, 1, string.Empty, orange, m_cText, 0, 10, id);
		Cell(m_wDyn, x, by + 13, barW, 1, string.Empty, orange, m_cText, 0, 10, id);
		Cell(m_wDyn, x, by, 1, 14, string.Empty, orange, m_cText, 0, 10, id);
		Cell(m_wDyn, x + barW - 1, by, 1, 14, string.Empty, orange, m_cText, 0, 10, id);
		float t = 0;
		if (!mixed && row.m_fMax > row.m_fMin)
			t = Math.Clamp((v[0] - row.m_fMin) / (row.m_fMax - row.m_fMin), 0, 1);
		Widget fill = Cell(m_wDyn, x, by, Math.Max(1, barW * t), 14, string.Empty, orange, m_cText, 0, 10, id);
		if (fill)
			fill.SetVisible(t > 0);
		if (row.m_bKnownDef && row.m_fMax > row.m_fMin)
		{
			float dt = Math.Clamp((row.m_vDef[0] - row.m_fMin) / (row.m_fMax - row.m_fMin), 0, 1);
			Cell(m_wDyn, x + barW * dt - 1, by - 5, 2, 24, string.Empty, m_cDim, m_cText, 0, 10, id);
		}
		Widget tick = Cell(m_wDyn, x + barW * t - 1, by - 3, 2, 20, string.Empty, m_cWhite, m_cText, 0, 10, id);
		if (tick)
			tick.SetVisible(!row.m_bDisabled && (rowIndex == m_iFocusRow || rowIndex == m_iDragRow));
		string text = SliderText(row, v[0]);
		if (mixed)
			text = T("set_mixed");
		Color fg = m_cInk;
		if (dim)
			fg = Color.FromSRGBA(241, 241, 239, 140);
		Widget valueW = Cell(m_wDyn, x + barW + 10, y, 120, m_fTileH, text, m_cClear, fg, 2, 19, Act(DCO_ESetAct.ROW, rowIndex, 0, string.Empty));
		m_aSliderRow.Insert(rowIndex);
		m_aSliderFill.Insert(fill);
		m_aSliderText.Insert(valueW);
		m_aSliderTick.Insert(tick);
		m_aSliderY.Insert(by);
	}

	protected void RenderTextBox(DCO_SetRow row, int rowIndex, float x, float y, float w, float h)
	{
		Widget edit = GetGame().GetWorkspace().CreateWidgets(m_sEditLayout, m_wDyn);
		if (!edit)
			return;
		FrameSlot.SetPos(edit, x, y);
		FrameSlot.SetSize(edit, w, h);
		ImageWidget editBg = ImageWidget.Cast(edit.FindAnyWidget("Bg"));
		if (editBg)
			editBg.SetColor(m_cCell);
		EditBoxWidget box = EditBoxWidget.Cast(edit.FindAnyWidget("Box"));
		if (box)
		{
			box.SetText(row.m_sText);
			box.SetVirtualKeyboardTitle(row.m_sLabel);
			m_aTextRow.Insert(rowIndex);
			m_aTextBox.Insert(box);
		}
		Tag(edit, Act(DCO_ESetAct.TEXTBOX, rowIndex, 0, string.Empty));
	}

	protected bool SimpleView()
	{
		return m_bSimple && !SearchMode() && !m_bOnlyMod;
	}

	protected DCO_SimpleSetting SimpleAt(int index)
	{
		array<ref DCO_SimpleSetting> all = DCO_SettingsHelp.Simple();
		if (!all || !all.IsIndexValid(index))
			return null;
		return all[index];
	}

	protected bool SimpleUsable(DCO_SimpleSetting setting)
	{
		if (!setting)
			return false;
		if (!setting.m_sMirror.IsEmpty())
		{
			int mirrorRow = RowByClass(setting.m_sMirror);
			return mirrorRow >= 0 && (!FactionMode() || FactionRow(m_aRows[mirrorRow]));
		}
		if (!setting.m_aLevels || setting.m_aLevels.IsEmpty() || !setting.m_aLevels[0].m_aValues)
			return false;
		foreach (DCO_SimpleValue value : setting.m_aLevels[0].m_aValues)
		{
			int r = RowByClass(value.m_sAttribute);
			if (r >= 0 && !m_aRows[r].m_bMember && (!FactionMode() || FactionRow(m_aRows[r])))
				return true;
		}
		return false;
	}

	protected void BuildSimple()
	{
		m_aSimpleIdx.Clear();
		array<ref DCO_SimpleSetting> all = DCO_SettingsHelp.Simple();
		if (!all)
			return;
		foreach (int i, DCO_SimpleSetting setting : all)
		{
			if (setting && setting.m_iTarget == m_iTargetKind && SimpleUsable(setting))
				m_aSimpleIdx.Insert(i);
		}
	}

	protected void BuildSimpleLines()
	{
		DCO_SetLine head = new DCO_SetLine();
		head.m_iType = 0;
		head.m_fH = 40;
		head.m_sText = T("set_mode_simple");
		head.m_sHint = T("set_simple_levels");
		m_aLines.Insert(head);
		foreach (int si : m_aSimpleIdx)
		{
			DCO_SimpleSetting setting = SimpleAt(si);
			if (!SimpleUsable(setting))
				continue;
			if (!setting.m_sMirror.IsEmpty())
			{
				int r = RowByClass(setting.m_sMirror);
				int parts = Parts(m_aRows[r]);
				for (int p = 0; p < parts; p++)
				{
					DCO_SetLine mirrorLine = new DCO_SetLine();
					mirrorLine.m_iType = 1;
					mirrorLine.m_iRow = r;
					mirrorLine.m_iPart = p;
					mirrorLine.m_fH = m_fLineH;
					m_aLines.Insert(mirrorLine);
				}
				continue;
			}
			DCO_SetLine levelLine = new DCO_SetLine();
			levelLine.m_iType = 3;
			levelLine.m_iRow = si;
			levelLine.m_fH = m_fLineH;
			m_aLines.Insert(levelLine);
		}
	}

	protected int SimpleLevel(DCO_SimpleSetting setting)
	{
		foreach (int li, DCO_SimpleLevel level : setting.m_aLevels)
		{
			if (!level || !level.m_aValues)
				continue;
			bool match = true;
			bool any = false;
			foreach (DCO_SimpleValue value : level.m_aValues)
			{
				int r = RowByClass(value.m_sAttribute);
				if (r < 0)
					continue;
				DCO_SetRow row = m_aRows[r];
				if (FactionMode() && !FactionRow(row))
					continue;
				any = true;
				vector v = ViewValue(row);
				if (ShowsMixed(row) || Math.AbsFloat(v[0] - value.m_fValue) > 0.001 + Math.AbsFloat(value.m_fValue) * 0.0001)
				{
					match = false;
					break;
				}
			}
			if (match && any)
				return li;
		}
		return -1;
	}

	protected void SimpleRows(DCO_SimpleSetting setting, notnull array<int> outRows)
	{
		if (!setting.m_aLevels || setting.m_aLevels.IsEmpty() || !setting.m_aLevels[0].m_aValues)
			return;
		foreach (DCO_SimpleValue value : setting.m_aLevels[0].m_aValues)
		{
			int r = RowByClass(value.m_sAttribute);
			if (r >= 0 && outRows.Find(r) < 0)
				outRows.Insert(r);
		}
	}

	protected bool SimplePending(DCO_SimpleSetting setting)
	{
		array<int> rows = {};
		SimpleRows(setting, rows);
		foreach (int r : rows)
		{
			if (IsPending(m_aRows[r]))
				return true;
		}
		return false;
	}

	protected void ApplySimpleLevel(int simpleIndex, int levelIndex)
	{
		DCO_SimpleSetting setting = SimpleAt(simpleIndex);
		if (!setting || !setting.m_aLevels.IsIndexValid(levelIndex) || !setting.m_aLevels[levelIndex].m_aValues)
			return;
		int changed = 0;
		foreach (DCO_SimpleValue value : setting.m_aLevels[levelIndex].m_aValues)
		{
			int r = RowByClass(value.m_sAttribute);
			if (r < 0)
				continue;
			DCO_SetRow row = m_aRows[r];
			if (FactionMode() && !FactionRow(row))
				continue;
			SetCur(row, value.m_fValue);
			changed++;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_simple setting=" + DCO_SettingsBaseline.LogText(setting.m_sName) + " level=" + DCO_SettingsBaseline.LogText(setting.m_aLevels[levelIndex].m_sName) + " values=" + changed.ToString() + " faction=" + m_sFaction);
		RequestRender();
	}

	protected void RenderSimpleLine(DCO_SetLine line, float y)
	{
		DCO_SimpleSetting setting = SimpleAt(line.m_iRow);
		if (!setting)
			return;
		float x0 = m_fSheetX;
		float h = line.m_fH - 3;
		float cy = y + (h - m_fTileH) * 0.5;
		int rowKey = -2 - line.m_iRow;
		int labelId = Act(DCO_ESetAct.SIMPLE, line.m_iRow, -1, string.Empty);
		Cell(m_wDyn, x0, y, m_fSheetW, h, string.Empty, m_cRowBg, m_cText, 0, 10, labelId);
		Widget focusW = GradCell(x0, y, m_fSheetW, h, labelId);
		RegisterLine(focusW, null, rowKey, false);
		bool pending = SimplePending(setting);
		if (pending)
			IconCell(m_wDyn, x0 + 8, y + h * 0.5 - 5, 10, m_sIconDot, string.Empty, m_cOrange, labelId);
		int current = SimpleLevel(setting);
		float tagX = x0 + m_fLabelW - 8;
		int simpleTotal;
		int simpleBeta = SimpleBeta(setting, simpleTotal);
		if (simpleBeta > 0 && simpleBeta == simpleTotal)
		{
			string betaTag = T("set_beta_tag");
			tagX -= TextW(betaTag, 7.8) + 4;
			TagCell(tagX, y + h * 0.5 - 10, betaTag, m_cBeta);
		}
		if (current < 0)
		{
			string customTag = T("set_simple_custom");
			tagX -= TextW(customTag, 7.8) + 4;
			TagCell(tagX, y + h * 0.5 - 10, customTag, m_cTagFg);
		}
		Cell(m_wDyn, x0 + 22, y, tagX - x0 - 26, h, setting.m_sName, m_cClear, m_cInk, 0, 19, labelId);
		int count = setting.m_aLevels.Count();
		for (int li = 0; li < count && li < 7; li++)
		{
			string name = setting.m_aLevels[li].m_sName;
			bool lit = li == current;
			Color bg = m_cCell;
			Color fg = Color.FromSRGBA(218, 220, 222, 255);
			if (lit)
			{
				bg = m_cTileOn;
				fg = m_cWhite;
			}
			float tx = x0 + m_fCellX + 1.0 * li * m_fCellStep;
			int levelId = Act(DCO_ESetAct.SIMPLE, line.m_iRow, li, string.Empty);
			TileCell(tx, cy, name, bg, fg, levelId);
			if (lit)
				Cell(m_wDyn, tx, cy + m_fTileH - 2, m_fCellW, 2, string.Empty, m_cWhite, m_cText, 0, 10, levelId);
			if (lit && m_bPad && rowKey == m_iFocusRow)
				Outline(tx, cy, m_fCellW, m_fTileH, levelId);
		}
		if (pending)
			IconCell(m_wDyn, x0 + m_fResetX + 4, y + h * 0.5 - 11, 22, m_sIconSet, "restart", m_cOrange, Act(DCO_ESetAct.REVERT_SIMPLE, line.m_iRow, 0, T("set_tip_revert")));
	}

	protected string SimpleDesc(int simpleIndex, int levelIndex)
	{
		DCO_SimpleSetting setting = SimpleAt(simpleIndex);
		if (!setting)
			return string.Empty;
		string text = Bold(setting.m_sName) + "   " + setting.m_sDescription;
		foreach (int li, DCO_SimpleLevel level : setting.m_aLevels)
		{
			if (!level || !level.m_aValues || (levelIndex >= 0 && li != levelIndex))
				continue;
			string parts;
			foreach (DCO_SimpleValue value : level.m_aValues)
			{
				int r = RowByClass(value.m_sAttribute);
				if (r < 0)
					continue;
				DCO_SetRow row = m_aRows[r];
				vector v = row.m_vCur;
				v[0] = value.m_fValue;
				if (!parts.IsEmpty())
					parts += ",  ";
				parts += row.m_sLabel + " " + ValueText(row, v);
			}
			if (levelIndex < 0 && parts.Length() > 140)
				parts = parts.Substring(0, 137) + "...";
			text += "\n" + Dim(level.m_sName + ":  " + parts);
		}
		if (SimpleLevel(setting) < 0)
			text += "\n" + Warn(T("set_simple_custom_tip"));
		int simpleTotal;
		int simpleBeta = SimpleBeta(setting, simpleTotal);
		if (simpleBeta > 0 && simpleBeta == simpleTotal)
			text += "\n" + Beta(T("set_beta_help"));
		else if (simpleBeta > 0)
			text += "\n" + Beta(T("set_beta_simple"));
		return text;
	}

	protected int SimpleBeta(DCO_SimpleSetting setting, out int total)
	{
		int n;
		total = 0;
		if (!setting || !setting.m_aLevels)
			return 0;
		foreach (DCO_SimpleLevel level : setting.m_aLevels)
		{
			if (!level || !level.m_aValues)
				continue;
			foreach (DCO_SimpleValue value : level.m_aValues)
			{
				int r = RowByClass(value.m_sAttribute);
				if (r < 0)
					continue;
				total++;
				if (m_aRows[r].m_bBeta)
					n++;
			}
		}
		return n;
	}

	protected float MeterPoints(DCO_SetRow row, DCO_MeterWeight weight, float value)
	{
		if (weight.m_iMode == 1)
		{
			array<string> pairs = {};
			weight.m_sPerOption.Split(" ", pairs, true);
			foreach (string pair : pairs)
			{
				array<string> kv = {};
				pair.Split(":", kv, false);
				if (kv.Count() == 2 && Math.AbsFloat(kv[0].ToFloat() - value) < 0.001)
					return kv[1].ToFloat();
			}
			return 0;
		}
		float lo = row.m_fMin;
		float hi = row.m_fMax;
		if (row.m_iKind != DCO_ESetKind.SLIDER && !row.m_aValues.IsEmpty())
		{
			lo = row.m_aValues[0];
			hi = row.m_aValues[0];
			foreach (float optionValue : row.m_aValues)
			{
				lo = Math.Min(lo, optionValue);
				hi = Math.Max(hi, optionValue);
			}
		}
		if (hi <= lo)
			return weight.m_fAtMin;
		float t = Math.Clamp((value - lo) / (hi - lo), 0, 1);
		return weight.m_fAtMin + (weight.m_fAtMax - weight.m_fAtMin) * t;
	}

	protected string MeterName(int meter)
	{
		if (meter == 1)
			return T("set_meter_perf");
		if (meter == 2)
			return T("set_meter_realism");
		return T("set_meter_difficulty");
	}

	protected string MeterWord(int meter, int score)
	{
		int band = Math.ClampInt(score / 25, 0, 3);
		string prefix = "set_meter_diff_";
		if (meter == 1)
			prefix = "set_meter_load_";
		else if (meter == 2)
			prefix = "set_meter_real_";
		return T(prefix + band.ToString());
	}

	protected string MeterKey(int meter)
	{
		if (meter == 1)
			return "load";
		if (meter == 2)
			return "real";
		return "diff";
	}

	protected float WeightMax(DCO_MeterWeight weight)
	{
		if (weight.m_iMode != 1)
			return Math.Max(0, Math.Max(weight.m_fAtMin, weight.m_fAtMax));
		float best = 0;
		array<string> pairs = {};
		weight.m_sPerOption.Split(" ", pairs, true);
		foreach (string pair : pairs)
		{
			array<string> kv = {};
			pair.Split(":", kv, false);
			if (kv.Count() == 2)
				best = Math.Max(best, kv[1].ToFloat());
		}
		return best;
	}

	protected float MeterMax(int meter)
	{
		float total = 0;
		array<ref DCO_MeterWeight> weights = DCO_SettingsHelp.Weights();
		if (!weights)
			return 0;
		foreach (DCO_MeterWeight weight : weights)
		{
			if (weight && weight.m_iMeter == meter)
				total += WeightMax(weight);
		}
		return total;
	}

	protected int MeterNorm(float total, int meter)
	{
		if (!m_aMeterMax || !m_aMeterMax.IsIndexValid(meter) || m_aMeterMax[meter] <= 0)
			return 0;
		return Math.ClampInt((int)Math.Round(total / m_aMeterMax[meter] * 100), 0, 100);
	}

	protected vector AppliedValue(DCO_SetRow row)
	{
		vector before = row.m_vOrig;
		if (FactionMode())
		{
			DCO_SetFactionEdit edit = Fac();
			if (edit && !edit.m_mOrig.Find(row.m_sClass, before))
				before = row.m_vOrig;
		}
		else if (MemberScope() && row.m_bMember)
		{
			DCO_SetFactionEdit memberEdit = MemEdit();
			if (memberEdit && !memberEdit.m_mOrig.Find(row.m_sClass, before))
				before = row.m_vOrig;
		}
		return before;
	}

	protected DCO_MeterState MeterState(int meter)
	{
		DCO_MeterState state = new DCO_MeterState();
		float applied = 0;
		float pending = 0;
		float def = 0;
		array<ref DCO_MeterWeight> weights = DCO_SettingsHelp.Weights();
		if (weights)
		{
			foreach (DCO_MeterWeight weight : weights)
			{
				if (!weight || weight.m_iMeter != meter)
					continue;
				int r = RowByClass(weight.m_sAttribute);
				if (r < 0)
					continue;
				DCO_SetRow row = m_aRows[r];
				if (ShowsMixed(row) || (FactionMode() && !FactionRow(row)))
				{
					state.m_bPartial = true;
					continue;
				}
				vector appliedValue = AppliedValue(row);
				vector pendingValue = ViewValue(row);
				vector defValue = appliedValue;
				if (row.m_bHasDef)
					defValue = row.m_vDef;
				float appliedPoints = MeterPoints(row, weight, appliedValue[0]);
				float pendingPoints = MeterPoints(row, weight, pendingValue[0]);
				applied += appliedPoints;
				pending += pendingPoints;
				def += MeterPoints(row, weight, defValue[0]);
				state.m_aLabels.Insert(row.m_sLabel);
				state.m_aPoints.Insert(pendingPoints);
				state.m_aDelta.Insert(pendingPoints - appliedPoints);
			}
		}
		state.m_iApplied = MeterNorm(applied, meter);
		state.m_iPending = MeterNorm(pending, meter);
		state.m_iDefault = MeterNorm(def, meter);
		return state;
	}

	protected string MeterTip(int meter, DCO_MeterState state)
	{
		string tip = Bold(MeterName(meter)) + "   " + state.m_iPending.ToString() + " / 100  ·  " + MeterWord(meter, state.m_iPending);
		if (state.m_bPartial)
			tip += "  ·  " + Warn(T("set_meter_partial"));
		int band = Math.ClampInt(state.m_iPending / 25, 0, 3);
		string scale;
		for (int b = 0; b < 4; b++)
		{
			if (b > 0)
				scale += "  ·  ";
			if (b == band)
				scale += Bold(MeterWord(meter, b * 25));
			else
				scale += MeterWord(meter, b * 25);
		}
		tip += "\n" + scale;
		tip += "\n" + Dim(T2("set_meter_default", "n", state.m_iDefault.ToString()));
		array<float> points = {};
		points.Copy(state.m_aPoints);
		for (int n = 0; n < 5; n++)
		{
			int best = -1;
			foreach (int i, float value : points)
			{
				if (value > 0 && (best < 0 || value > points[best]))
					best = i;
			}
			if (best < 0)
				break;
			int bestPoints = (int)Math.Round(points[best]);
			tip += "\n" + state.m_aLabels[best] + "   +" + bestPoints.ToString();
			points[best] = -1;
		}
		if (state.m_aLabels.IsEmpty())
			tip += "\n" + T("set_meter_none");
		return tip;
	}

	protected bool HasMeterRows()
	{
		array<ref DCO_MeterWeight> weights = DCO_SettingsHelp.Weights();
		if (!weights)
			return false;
		foreach (DCO_MeterWeight weight : weights)
		{
			if (weight && RowByClass(weight.m_sAttribute) >= 0)
				return true;
		}
		return false;
	}

	protected void RenderMeters()
	{
		if (!HasMeterRows())
			return;
		for (int m = 0; m < 3; m++)
		{
			DCO_MeterState state = MeterState(m);
			RenderMeter(m, state, m_fTreeX, m_fDescY + 8 + m * 32, m_fTreeW, Act(DCO_ESetAct.METER, m, 0, MeterTip(m, state)));
		}
	}

	protected void RenderMeter(int meter, DCO_MeterState state, float x, float y, float w, int id)
	{
		Cell(m_wDyn, x, y - 2, w, 32, string.Empty, m_cClear, m_cText, 0, 10, id);
		Cell(m_wDyn, x, y - 2, 104, 20, MeterName(meter), m_cClear, m_cText, 0, 14, id);
		string value = state.m_iPending.ToString();
		if (state.m_iPending != state.m_iApplied)
			value = state.m_iApplied.ToString() + " → " + state.m_iPending.ToString();
		float valueW = TextW(value, 8.2);
		Cell(m_wDyn, x + 104, y - 3, valueW, 20, value, m_cClear, m_cInk, 0, 15, id);
		string word = MeterWord(meter, state.m_iPending);
		Color wordColor = m_cDim;
		if (meter == 1 && state.m_iPending >= 75)
			wordColor = m_cAmber;
		if (state.m_bPartial)
		{
			word = T("set_meter_partial");
			wordColor = m_cDim;
		}
		Cell(m_wDyn, x + 104 + valueW, y - 1, w - 104 - valueW, 18, word, m_cClear, wordColor, 0, 13, id);

		float by = y + 21;
		Cell(m_wDyn, x, by, w, 6, string.Empty, m_cTrack, m_cText, 0, 10, id);
		float appliedW = w * state.m_iApplied / 100.0;
		float pendingW = w * state.m_iPending / 100.0;
		float solidW = Math.Min(appliedW, pendingW);
		if (solidW > 0)
			Cell(m_wDyn, x, by, solidW, 6, string.Empty, m_cOrange, m_cText, 0, 10, id);
		if (pendingW > appliedW)
			Cell(m_wDyn, x + appliedW, by, pendingW - appliedW, 6, string.Empty, m_cPendUp, m_cText, 0, 10, id);
		else if (appliedW > pendingW)
			Cell(m_wDyn, x + pendingW, by, appliedW - pendingW, 6, string.Empty, m_cPendDown, m_cText, 0, 10, id);
		for (int q = 1; q < 4; q++)
		{
			Cell(m_wDyn, x + w * q * 0.25 - 1, by, 2, 6, string.Empty, m_cSurface, m_cText, 0, 10, id);
		}
		float defX = x + w * state.m_iDefault / 100.0;
		Cell(m_wDyn, Math.Clamp(defX - 1, x, x + w - 2), by - 6, 2, 12, string.Empty, m_cWhite, m_cText, 0, 10, id);
	}

	protected string MeterReviewLine(int meter)
	{
		DCO_MeterState state = MeterState(meter);
		string text = Bold(MeterName(meter)) + "   ";
		if (state.m_iApplied == state.m_iPending)
			return text + state.m_iPending.ToString() + "  ·  " + Dim(T("set_meter_nochange"));
		text += state.m_iApplied.ToString() + " → " + state.m_iPending.ToString() + "  ·  " + MeterWord(meter, state.m_iApplied) + " → " + MeterWord(meter, state.m_iPending);
		array<float> deltas = {};
		deltas.Copy(state.m_aDelta);
		string parts;
		for (int n = 0; n < 3; n++)
		{
			int best = -1;
			foreach (int i, float delta : deltas)
			{
				if (Math.AbsFloat(delta) >= 0.5 && (best < 0 || Math.AbsFloat(delta) > Math.AbsFloat(deltas[best])))
					best = i;
			}
			if (best < 0)
				break;
			int change = (int)Math.Round(deltas[best]);
			string sign = "";
			if (change > 0)
				sign = "+";
			if (!parts.IsEmpty())
				parts += ", ";
			parts += state.m_aLabels[best] + " " + sign + change.ToString();
			deltas[best] = 0;
		}
		if (!parts.IsEmpty())
			text += "  ·  " + Dim(parts);
		return text;
	}

	protected void LogMeters()
	{
		if (!HasMeterRows())
			return;
		string scores;
		string defaults;
		string partial;
		string max;
		for (int m = 0; m < 3; m++)
		{
			DCO_MeterState state = MeterState(m);
			scores += " " + MeterKey(m) + "=" + state.m_iApplied.ToString();
			defaults += " def_" + MeterKey(m) + "=" + state.m_iDefault.ToString();
			if (state.m_bPartial)
			{
				if (!partial.IsEmpty())
					partial += ",";
				partial += MeterKey(m);
			}
			if (!max.IsEmpty())
				max += "/";
			max += ((int)Math.Round(m_aMeterMax[m])).ToString();
		}
		if (partial.IsEmpty())
			partial = "none";
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_meters target=" + DCO_SettingsBaseline.LogText(m_sTarget) + scores + defaults + " partial=" + partial + " max=" + max);
	}

	protected void LogMetersApply()
	{
		if (!HasMeterRows())
			return;
		string text;
		for (int m = 0; m < 3; m++)
		{
			DCO_MeterState state = MeterState(m);
			text += " " + MeterKey(m) + "=" + state.m_iApplied.ToString() + "->" + state.m_iPending.ToString();
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_meters_apply" + text);
	}

	protected float ParaHeight(string plain, float width)
	{
		float perLine = Math.Max(10, width / 8.0);
		array<string> parts = {};
		plain.Split("\n", parts, false);
		int lines = 0;
		foreach (string part : parts)
		{
			lines += (int)Math.Max(1, Math.Ceil(part.Length() / perLine));
		}
		return lines * 21 + 4;
	}

	protected void AddPara(notnull array<string> texts, notnull array<bool> heads, string text, bool head)
	{
		if (text.IsEmpty())
			return;
		texts.Insert(text);
		heads.Insert(head);
	}

	protected void RenderHelp()
	{
		float x0 = m_fWinW - 8;
		float w = m_fHelpW - 16;
		float top = m_fRuleY + 2;
		float bottom = m_fFootY;
		Cell(m_wDyn, x0, top, w, bottom - top, string.Empty, m_cPanelBg, m_cText, 0, 14, 0);
		Cell(m_wDyn, x0 + 10, top + 10, w - 100, 30, T("set_help_title"), m_cClear, m_cInk, 1, 20, 0);
		KeyHint(x0 + w - 90, top + 14, "?", T("set_close_btn"), true, Act(DCO_ESetAct.HELP, 0, 0, string.Empty));

		array<string> texts = {};
		array<bool> heads = {};
		if (SimpleView())
		{
			AddPara(texts, heads, T("set_mode_simple"), true);
			AddPara(texts, heads, T("set_simple_info"), false);
		}
		else if (m_aSubs.IsIndexValid(m_iSel) && !SearchMode())
		{
			DCO_SetSub sub = m_aSubs[m_iSel];
			DCO_SettingSubHelp help = DCO_SettingsHelp.GetSub(sub.m_Category);
			AddPara(texts, heads, sub.m_sName, true);
			if (help)
			{
				AddPara(texts, heads, T("set_help_purpose"), true);
				AddPara(texts, heads, help.m_sPurpose, false);
				AddPara(texts, heads, T("set_help_interact"), true);
				AddPara(texts, heads, help.m_sInteract, false);
				if (help.m_aSetups && !help.m_aSetups.IsEmpty())
				{
					AddPara(texts, heads, T("set_help_setups"), true);
					foreach (DCO_SettingSetup setup : help.m_aSetups)
					{
						AddPara(texts, heads, setup.m_sName + ": " + setup.m_sValues, false);
					}
				}
				AddPara(texts, heads, T("set_help_pitfalls"), true);
				AddPara(texts, heads, help.m_sPitfalls, false);
			}
			else
			{
				AddPara(texts, heads, sub.m_sDesc, false);
			}
		}
		else
		{
			AddPara(texts, heads, T("set_help_pick"), false);
		}
		array<ref DCO_SettingTerm> terms = DCO_SettingsHelp.Terms();
		if (terms && !terms.IsEmpty())
		{
			AddPara(texts, heads, T("set_help_glossary"), true);
			foreach (DCO_SettingTerm term : terms)
			{
				AddPara(texts, heads, term.m_sTerm + " - " + term.m_sDefinition, false);
			}
		}

		m_iHelpMax = (int)Math.Max(0, texts.Count() - 1);
		m_iHelpScroll = Math.ClampInt(m_iHelpScroll, 0, m_iHelpMax);
		float y = top + 46;
		for (int i = m_iHelpScroll; i < texts.Count(); i++)
		{
			float ph = ParaHeight(texts[i], w - 20);
			if (y + ph > bottom - 8)
				break;
			Color color = m_cText;
			int size = 16;
			if (heads[i])
			{
				color = m_cOrange;
				size = 17;
			}
			Widget para = Cell(m_wDyn, x0 + 10, y, w - 20, ph, texts[i], m_cClear, color, 0, size, 0);
			WrapCell(para);
			y += ph + 4;
		}
		ScrollBar(x0 + w - 4, top + 46, bottom - top - 54, m_iHelpScroll, m_iHelpMax);
	}

	protected string Dim(string text)
	{
		return "<color rgba=\"143,149,154,255\">" + text + "</color>";
	}

	protected string Warn(string text)
	{
		return "<color rgba=\"255,201,138,255\">" + text + "</color>";
	}

	protected string Beta(string text)
	{
		return "<color rgba=\"140,190,235,255\">" + text + "</color>";
	}

	protected bool IsDescAct(int kind)
	{
		return kind == DCO_ESetAct.ROW || kind == DCO_ESetAct.CELL || kind == DCO_ESetAct.STAR || kind == DCO_ESetAct.REVERT_ROW || kind == DCO_ESetAct.SLIDERBAR || kind == DCO_ESetAct.TEXTBOX || kind == DCO_ESetAct.SIMPLE || kind == DCO_ESetAct.REVERT_SIMPLE;
	}

	protected void SetFocusRow(int rowKey)
	{
		m_iFocusRow = rowKey;
		foreach (int i, Widget focusW : m_aLineFocusW)
		{
			bool focused = m_aLineRowIdx[i] == rowKey;
			if (focusW)
				focusW.SetVisible(focused);
			Widget starW = m_aLineStarW[i];
			if (starW)
				starW.SetVisible(m_aLineFav[i] || focused);
		}
		foreach (int si, int sliderRow : m_aSliderRow)
		{
			Widget tick = m_aSliderTick[si];
			if (tick)
				tick.SetVisible((sliderRow == rowKey || sliderRow == m_iDragRow) && !m_aRows[sliderRow].m_bDisabled);
		}
		UpdateDescAction();
	}

	protected void SetDesc(int kind, int a, int b)
	{
		if (kind != m_iDescKind || a != m_iDescA || b != m_iDescB)
			m_iDescSkip = 0;
		m_iDescKind = kind;
		m_iDescA = a;
		m_iDescB = b;
		if (m_wDescText)
			m_wDescText.SetText(SkipParas(DescText(kind, a, b)));
	}

	protected void RenderDesc()
	{
		float x = m_fSheetX;
		float w = m_fSheetW;
		float y = m_fDescY;
		Rule(x, y, w);
		Widget box = Cell(m_wDyn, x, y + 8, w - 210, m_fDescH - 12, string.Empty, m_cClear, m_cInk, 0, 19, 0);
		if (box)
		{
			m_wDescText = RichTextWidget.Cast(box.FindAnyWidget("Text"));
			if (m_wDescText)
			{
				m_wDescText.SetTextWrapping(true);
				m_wDescText.SetText(SkipParas(DescText(m_iDescKind, m_iDescA, m_iDescB)));
			}
		}
		m_iDescActId = Act(DCO_ESetAct.DESC_ACTION, -1, 0, string.Empty);
		m_wDescAct = Cell(m_wDyn, x + w - 200, y + 12, 200, 28, string.Empty, m_cClear, m_cOrange, 0, 16, m_iDescActId);
		UpdateDescAction();
	}

	protected int DescActionKind(DCO_SetRow row)
	{
		if (!row || row.m_bDisabled || row.m_iKind == DCO_ESetKind.TEXT)
			return 0;
		if (FactionMode())
		{
			if (FactionRow(row) && IsOverridden(row))
				return 1;
			return 0;
		}
		if (MemberScope() && row.m_bMember)
		{
			vector groupValue;
			if (GroupValue(row, groupValue) && !Same(ViewValue(row), groupValue))
				return 2;
			return 0;
		}
		if (row.m_bHasDef && !ShowsMixed(row) && !Same(row.m_vCur, row.m_vDef))
			return 3;
		return 0;
	}

	protected void UpdateDescAction()
	{
		if (!m_wDescAct || m_iDescActId <= 0 || m_iDescActId >= m_aActA.Count())
			return;
		int kind = 0;
		if (m_iPanel == DCO_ESetPanel.NONE && m_aRows.IsIndexValid(m_iFocusRow))
			kind = DescActionKind(m_aRows[m_iFocusRow]);
		m_aActA[m_iDescActId] = m_iFocusRow;
		m_aActB[m_iDescActId] = kind;
		RichTextWidget t = RichTextWidget.Cast(m_wDescAct.FindAnyWidget("Text"));
		if (t)
		{
			string label;
			if (kind == 1)
				label = T("set_use_global");
			else if (kind == 2)
				label = T("set_use_group");
			else if (kind == 3)
				label = T("set_reset_default");
			t.SetText(label);
		}
		m_wDescAct.SetVisible(kind > 0);
	}

	protected string DescText(int kind, int a, int b)
	{
		string text;
		if (!m_sStatus.IsEmpty())
			text = Warn(m_sStatus) + "\n";
		if (kind == DCO_ESetAct.SIMPLE)
			return text + SimpleDesc(a, b);
		if (kind == DCO_ESetAct.REVERT_SIMPLE)
			return text + SimpleDesc(a, -1);
		if (kind != DCO_ESetAct.NONE && m_aRows.IsIndexValid(a))
			return text + RowDesc(m_aRows[a], kind, b);
		if (m_aRows.IsIndexValid(m_iFocusRow))
			return text + RowDesc(m_aRows[m_iFocusRow], DCO_ESetAct.ROW, 0);
		if (m_iFocusRow <= -2)
			return text + SimpleDesc(-2 - m_iFocusRow, -1);
		if (SimpleView())
			return text + Bold(T("set_mode_simple")) + "   " + T("set_simple_info");
		if (SearchMode())
			return text + T("set_desc_search");
		if (m_aSubs.IsIndexValid(m_iSel))
			return text + SubDesc(m_iSel);
		return text + Bold(T("set_favorites")) + "   " + T("set_desc_favs");
	}

	protected string SubDesc(int index)
	{
		DCO_SetSub sub = m_aSubs[index];
		DCO_SettingSubHelp help = DCO_SettingsHelp.GetSub(sub.m_Category);
		string text = Bold(sub.m_sName) + "   ";
		if (help && !help.m_sPurpose.IsEmpty())
			text += help.m_sPurpose;
		else
			text += sub.m_sDesc;
		if (help && !help.m_sInteract.IsEmpty())
			text += "\n" + Dim(help.m_sInteract);
		int betaTotal;
		int betaN = BetaCount(sub, betaTotal);
		if (betaN > 0 && betaN == betaTotal)
			text += "\n" + Beta(T("set_beta_help"));
		else if (betaN > 0)
			text += "\n" + Beta(T2("set_beta_count", "n", betaN.ToString(), "m", betaTotal.ToString()));
		return text;
	}

	protected string EffectText(DCO_SettingHelp help, string valueText)
	{
		if (!help || help.m_sEffect.IsEmpty())
			return string.Empty;
		string effect = help.m_sEffect;
		effect.Replace("%1", valueText);
		return effect;
	}

	protected string OptionLine(DCO_SetRow row, DCO_SettingHelp help, int kind, int b)
	{
		if (kind == DCO_ESetAct.CELL)
		{
			string name;
			int effectIndex = b;
			if (row.m_iKind == DCO_ESetKind.CHECK)
			{
				effectIndex = 1 - b;
				name = T("set_off");
				if (b != 0)
					name = T("set_on");
			}
			else
			{
				if (!row.m_aNames.IsIndexValid(b))
					return string.Empty;
				name = row.m_aNames[b];
			}
			string optionEffect = DCO_SettingsHelp.OptionEffect(help, effectIndex);
			if (optionEffect.IsEmpty())
				return Bold(name);
			return Bold(name) + ":  " + optionEffect;
		}
		if (ShowsMixed(row) || row.m_iKind == DCO_ESetKind.TEXT || row.m_iKind == DCO_ESetKind.MULTI)
			return string.Empty;
		vector v = ViewValue(row);
		string now;
		if (row.m_iKind == DCO_ESetKind.CHECK)
		{
			int checkIndex = 1;
			if (v[0] != 0)
				checkIndex = 0;
			now = DCO_SettingsHelp.OptionEffect(help, checkIndex);
		}
		else if (row.m_iKind == DCO_ESetKind.DROP)
		{
			foreach (int e, float value : row.m_aValues)
			{
				if (Math.AbsFloat(value - v[0]) < 0.0001)
					now = DCO_SettingsHelp.OptionEffect(help, e);
			}
		}
		else
		{
			now = EffectText(help, ValueText(row, v));
		}
		if (now.IsEmpty())
			return string.Empty;
		return Dim(T("set_desc_now")) + " " + now;
	}

	protected string MemberOwnText(DCO_SetRow row)
	{
		if (!row.m_bMember || MemberScope())
			return string.Empty;
		vector groupValue;
		if (!GroupValue(row, groupValue))
			return string.Empty;
		int own = 0;
		string names;
		foreach (int mi, map<string, vector> vals : m_aMemberVals)
		{
			vector v;
			if (!vals || !vals.Find(row.m_sClass, v) || Same(v, groupValue))
				continue;
			own++;
			if (own <= 2)
			{
				if (!names.IsEmpty())
					names += ", ";
				names += m_aMemberNames[mi];
			}
		}
		if (own == 0)
			return string.Empty;
		return T2("set_members_own", "n", own.ToString(), "m", names);
	}

	protected string RowDesc(DCO_SetRow row, int kind, int b)
	{
		DCO_SettingHelp help = DCO_SettingsHelp.Get(row.m_sClass);
		string text = Bold(row.m_sLabel);
		if (MemberScope() && row.m_bMember && m_aMemberNames.IsIndexValid(m_iMember - 1))
			text += Dim("  ·  " + m_aMemberNames[m_iMember - 1]);
		string what = row.m_sDesc;
		if (help && !help.m_sWhat.IsEmpty())
			what = help.m_sWhat;
		if (row.m_bBeta)
			what += " " + Beta(T("set_beta_help"));
		if (!what.IsEmpty())
			text += "   " + what;
		string option = OptionLine(row, help, kind, b);
		if (!option.IsEmpty())
			text += "   " + option;

		string info;
		if (row.m_bHasDef && row.m_iKind != DCO_ESetKind.TEXT && !FactionMode() && !MemberScope())
			info = T2("set_tip_default", "v", ValueText(row, row.m_vDef));
		if (IsPending(row) && row.m_iKind != DCO_ESetKind.TEXT)
		{
			vector before = row.m_vOrig;
			if (FactionMode())
			{
				DCO_SetFactionEdit edit = Fac();
				if (edit && !edit.m_mOrig.Find(row.m_sClass, before))
					before = row.m_vOrig;
			}
			else if (MemberScope() && row.m_bMember)
			{
				DCO_SetFactionEdit memberEdit = MemEdit();
				if (memberEdit)
					memberEdit.m_mOrig.Find(row.m_sClass, before);
			}
			if (!info.IsEmpty())
				info += "   ·   ";
			string beforeText = ValueText(row, before);
			if (row.m_bConflict && !FactionMode() && !MemberScope())
				beforeText = T("set_mixed");
			info += T2("set_desc_before", "v", beforeText);
		}
		if (RowInherits(row))
		{
			if (!info.IsEmpty())
				info += "   ·   ";
			info += T2("set_desc_follows", "n", InheritTag(row));
		}
		string own = MemberOwnText(row);
		if (!own.IsEmpty())
		{
			if (!info.IsEmpty())
				info += "   ·   ";
			info += own;
		}
		if (ShowsMixed(row))
		{
			if (!info.IsEmpty())
				info += "   ·   ";
			info += T("set_tip_mixed");
		}
		if (row.m_bDisabled)
		{
			if (!info.IsEmpty())
				info += "   ·   ";
			info += T("set_tip_disabled");
		}
		if (help && !help.m_sRecommended.IsEmpty())
		{
			if (!info.IsEmpty())
				info += "   ·   ";
			info += T("set_desc_rec") + " " + help.m_sRecommended;
		}
		if (!info.IsEmpty())
			text += "\n" + Dim(info);

		string warn;
		if (kind == DCO_ESetAct.CELL && row.m_iKind == DCO_ESetKind.DROP && row.m_aValues.IsIndexValid(b))
			warn = DCO_SettingsTree.HeavyReason(row.m_sClass, row.m_aValues[b]);
		else if (row.m_iKind != DCO_ESetKind.TEXT && !ShowsMixed(row))
		{
			vector current = ViewValue(row);
			warn = DCO_SettingsTree.HeavyReason(row.m_sClass, current[0]);
		}
		string rule = RowWarning(row);
		if (!rule.IsEmpty())
		{
			if (!warn.IsEmpty())
				warn += "   ·   ";
			warn += rule;
		}
		if (!warn.IsEmpty())
			text += "\n" + Warn(warn);
		return text;
	}

	protected string RowWarning(DCO_SetRow row)
	{
		foreach (DCO_SettingsRuleDef rule : DCO_SettingsTree.Get().m_aRules)
		{
			if (rule.m_sA != row.m_sClass && rule.m_sB != row.m_sClass)
				continue;
			array<string> warnings = {};
			Warnings(warnings);
			if (warnings.Find(rule.m_sMessage) >= 0)
				return rule.m_sMessage;
		}
		return string.Empty;
	}

	protected void RenderPadFooter()
	{
		float y = m_fFootY;
		Rule(m_fPad, y, m_fWinW - 2 * m_fPad);
		float cy = y + 18;
		int pending = TotalPending();
		float x = PadHint(m_fPad, cy, "B", T("set_back"), true, Act(DCO_ESetAct.CLOSE, 0, 0, string.Empty));
		DCO_SetRow row;
		if (m_iPanel == DCO_ESetPanel.NONE && m_aRows.IsIndexValid(m_iFocusRow))
			row = m_aRows[m_iFocusRow];
		bool simpleFocus = m_iPanel == DCO_ESetPanel.NONE && m_iFocusRow <= -2;
		if (row && !row.m_bDisabled)
		{
			if (row.m_iKind == DCO_ESetKind.CHECK || row.m_iKind == DCO_ESetKind.MULTI)
				x = PadHint(x + 18, cy, "A", T("set_pad_toggle"), true, 0);
			else if (row.m_iKind == DCO_ESetKind.TEXT)
				x = PadHint(x + 18, cy, "A", T("set_pad_edit"), true, 0);
			else
				x = PadHint(x + 18, cy, "DPAD_horizontal", T("set_pad_change"), true, 0);
		}
		else if (simpleFocus)
		{
			x = PadHint(x + 18, cy, "DPAD_horizontal", T("set_pad_change"), true, 0);
		}
		bool rowPending = row && IsPending(row);
		if (simpleFocus)
		{
			DCO_SimpleSetting focusSetting = SimpleAt(-2 - m_iFocusRow);
			rowPending = focusSetting && SimplePending(focusSetting);
		}
		x = PadHint(x + 18, cy, "X", T("set_pad_revert_row"), rowPending, 0);
		x = PadHint(x + 18, cy, "Y", T("set_pad_preset"), !PresetScope().IsEmpty(), 0);
		if (row && row.m_iKind == DCO_ESetKind.SLIDER && !row.m_bDisabled)
		{
			PadGlyph(x + 18, cy - 4, "LT", true, 0);
			x = PadHint(x + 48, cy, "RT", T("set_pad_big"), true, 0);
		}
		int revertId = 0;
		if (pending > 0)
			revertId = Act(DCO_ESetAct.REVERT_ALL, 0, 0, T("set_tip_revert_all"));
		x = PadHint(x + 18, cy, "RS", T("set_pad_revert_all"), pending > 0, revertId);
		if (!m_aSimpleIdx.IsEmpty())
			x = PadHint(x + 18, cy, "view", T("set_pad_mode"), true, 0);

		string applyText = T("set_apply");
		if (pending > 0)
			applyText = T2("set_apply_n", "n", pending.ToString());
		float applyW = 28 + 6 + TextW(applyText, 10.4);
		int applyId = 0;
		if (pending > 0)
			applyId = Act(DCO_ESetAct.APPLY, 0, 0, T("set_tip_apply"));
		PadHint(m_fWinW - m_fPad - applyW, cy, "menu", applyText, pending > 0, applyId, true);
	}

	protected void RenderFooter()
	{
		if (m_bPad)
		{
			RenderPadFooter();
			return;
		}
		float y = m_fFootY;
		Rule(m_fPad, y, m_fWinW - 2 * m_fPad);
		float cy = y + 18;
		int pending = TotalPending();
		float x = KeyHint(m_fPad, cy, "ESC", T("set_close_btn"), true, Act(DCO_ESetAct.CLOSE, 0, 0, string.Empty));
		int revertId = 0;
		if (pending > 0)
			revertId = Act(DCO_ESetAct.REVERT_ALL, 0, 0, T("set_tip_revert_all"));
		x = KeyHint(x + 18, cy, "R", T("set_revert_btn"), pending > 0, revertId);
		x = KeyHint(x + 18, cy, "L", T("set_log"), true, Act(DCO_ESetAct.LOG, 0, 0, T("set_tip_log")));
		x = KeyHint(x + 18, cy, "F", T("set_find_btn"), true, Act(DCO_ESetAct.FIND, 0, 0, string.Empty));

		string applyText = T("set_apply");
		if (pending > 0)
			applyText = T2("set_apply_n", "n", pending.ToString());
		float applyW = 26 + 6 + TextW(applyText, 10.4);
		float applyX = m_fWinW - m_fPad - applyW;
		int applyId = 0;
		if (pending > 0)
			applyId = Act(DCO_ESetAct.APPLY, 0, 0, T("set_tip_apply"));
		KeyHint(applyX, cy, "Enter", applyText, pending > 0, applyId, true);
	}

	protected int TotalPending()
	{
		int n = 0;
		foreach (DCO_SetRow row : m_aRows)
		{
			if (IsBasePending(row) && !row.m_bDisabled)
				n++;
		}
		foreach (string key, DCO_SetFactionEdit edit : m_mFaction)
		{
			if (edit)
				n += FactionChanges(edit);
		}
		foreach (int memberIndex, DCO_SetFactionEdit memberEdit : m_mMemberEdits)
		{
			if (memberEdit)
				n += FactionChanges(memberEdit);
		}
		n += m_aMemberClasses.Count();
		return n;
	}

	protected void RenderDiscard()
	{
		float w = 540;
		float h = 204;
		float x = (m_fWinW - w) * 0.5;
		float y = (m_fWinH - h) * 0.5;
		Cell(m_wDyn, 0, 0, TotalW(), m_fWinH, string.Empty, m_cShade, m_cText, 0, 10, Act(DCO_ESetAct.DISCARD_BG, 0, 0, string.Empty));
		Cell(m_wDyn, x, y, w, h, string.Empty, m_cPopBg, m_cText, 0, 10, 0);
		Cell(m_wDyn, x, y, w, 2, string.Empty, m_cOrange, m_cText, 0, 10, 0);
		Cell(m_wDyn, x + 22, y + 14, w - 44, 34, T("set_discard_title"), m_cClear, m_cInk, 1, 24, 0);
		Widget body = Cell(m_wDyn, x + 22, y + 54, w - 44, 76, T2("set_discard_text", "n", TotalPending().ToString()), m_cClear, m_cText, 0, 17, 0);
		WrapCell(body);
		float by = y + h - 36;
		float bx = x + 22;
		if (m_bPad)
		{
			bx = PadHint(bx, by, "B", T("set_discard_back"), true, Act(DCO_ESetAct.DISCARD_BACK, 0, 0, string.Empty));
			bx = PadHint(bx + 16, by, "X", T("set_discard_drop"), true, Act(DCO_ESetAct.DISCARD, 0, 0, string.Empty));
			PadHint(bx + 16, by, "A", T("set_discard_apply"), true, Act(DCO_ESetAct.DISCARD_APPLY, 0, 0, string.Empty), true);
			return;
		}
		bx = KeyHint(bx, by, "ESC", T("set_discard_back"), true, Act(DCO_ESetAct.DISCARD_BACK, 0, 0, string.Empty));
		bx = KeyHint(bx + 16, by, "D", T("set_discard_drop"), false, Act(DCO_ESetAct.DISCARD, 0, 0, string.Empty), false, m_cDanger);
		KeyHint(bx + 16, by, "Enter", T("set_discard_apply"), true, Act(DCO_ESetAct.DISCARD_APPLY, 0, 0, string.Empty), true);
	}

	void OnAct(int id)
	{
		if (id <= 0 || id >= m_aActKind.Count())
			return;
		int kind = m_aActKind[id];
		int a = m_aActA[id];
		int b = m_aActB[id];
		if (kind == DCO_ESetAct.SPAWNER)
		{
			SpAct(a, b);
			return;
		}
		if (m_bDiscardOpen && kind != DCO_ESetAct.DISCARD && kind != DCO_ESetAct.DISCARD_BACK && kind != DCO_ESetAct.DISCARD_APPLY)
			return;
		if (kind != DCO_ESetAct.RESETALL)
			m_bConfirmReset = false;
		switch (kind)
		{
			case DCO_ESetAct.FOLDER:
			{
				DCO_SettingsPrefs.ToggleClosed(m_aFolders[a].m_sKey);
				RequestRender();
				break;
			}
			case DCO_ESetAct.SUB:
			{
				m_iSel = a;
				m_iScroll = 0;
				m_iFocusRow = -1;
				m_iDescKind = DCO_ESetAct.NONE;
				m_bOnlyMod = false;
				ClearSearch();
				RequestRender();
				break;
			}
			case DCO_ESetAct.FAVS:
			{
				m_iSel = -1;
				m_iScroll = 0;
				m_iFocusRow = -1;
				m_iDescKind = DCO_ESetAct.NONE;
				m_bOnlyMod = false;
				ClearSearch();
				RequestRender();
				break;
			}
			case DCO_ESetAct.ROW:
			{
				SetFocusRow(a);
				SetDesc(DCO_ESetAct.ROW, a, 0);
				break;
			}
			case DCO_ESetAct.TEXTBOX:
			{
				SetFocusRow(a);
				SetDesc(DCO_ESetAct.ROW, a, 0);
				EditText(a);
				break;
			}
			case DCO_ESetAct.STAR:
			{
				DCO_SettingsPrefs.ToggleFav(m_aRows[a].m_sClass);
				m_iFocusRow = a;
				RequestRender();
				break;
			}
			case DCO_ESetAct.CELL:
			{
				m_iFocusRow = a;
				m_iFocusCell = b;
				OnCell(m_aRows[a], b);
				break;
			}
			case DCO_ESetAct.SLIDERBAR:
			{
				if (m_bDragUsed)
				{
					m_bDragUsed = false;
					break;
				}
				DCO_SetRow sliderRow = m_aRows[a];
				m_iFocusRow = a;
				float barX = m_fSheetX + m_fCellX;
				float t = Math.Clamp((ClickLocalX() - barX) / SliderBarW(), 0, 1);
				SetCur(sliderRow, Quant(sliderRow, sliderRow.m_fMin + t * (sliderRow.m_fMax - sliderRow.m_fMin)));
				break;
			}
			case DCO_ESetAct.REVERT_ROW:
			{
				LogRevert("row", m_aRows[a].m_sClass);
				RevertRow(m_aRows[a]);
				UpdateDependencies();
				RequestRender();
				break;
			}
			case DCO_ESetAct.REVERT_SIMPLE:
			{
				DCO_SimpleSetting setting = SimpleAt(a);
				if (setting)
				{
					array<int> simpleRows = {};
					SimpleRows(setting, simpleRows);
					foreach (int sr : simpleRows)
					{
						RevertRow(m_aRows[sr]);
					}
				}
				UpdateDependencies();
				RequestRender();
				break;
			}
			case DCO_ESetAct.REVERT_SEC:
			{
				foreach (DCO_SetLine line : m_aLines)
				{
					if (line.m_iType == 1 && line.m_iPart == 0)
						RevertRow(m_aRows[line.m_iRow]);
					if (line.m_iType == 3)
					{
						DCO_SimpleSetting lineSetting = SimpleAt(line.m_iRow);
						if (!lineSetting)
							continue;
						array<int> lineRows = {};
						SimpleRows(lineSetting, lineRows);
						foreach (int lr : lineRows)
						{
							RevertRow(m_aRows[lr]);
						}
					}
				}
				UpdateDependencies();
				RequestRender();
				break;
			}
			case DCO_ESetAct.REVERT_ALL:
			{
				RevertAll();
				break;
			}
			case DCO_ESetAct.DESC_ACTION:
			{
				if (!m_aRows.IsIndexValid(a))
					break;
				DCO_SetRow actionRow = m_aRows[a];
				if (b == 1)
				{
					DCO_SetFactionEdit edit = Fac();
					if (edit)
						edit.m_mCur.Remove(actionRow.m_sClass);
				}
				else if (b == 2)
				{
					vector groupValue;
					DCO_SetFactionEdit memberEdit = MemEdit();
					if (memberEdit && GroupValue(actionRow, groupValue))
						memberEdit.m_mCur.Set(actionRow.m_sClass, groupValue);
				}
				else if (b == 3)
				{
					actionRow.m_vCur = actionRow.m_vDef;
					actionRow.m_bTouched = true;
				}
				UpdateDependencies();
				RequestRender();
				break;
			}
			case DCO_ESetAct.SPIN:
			{
				if (m_bGlobalTarget && !m_aFactionKeys.IsEmpty())
				{
					int factionCount = m_aFactionKeys.Count() + 1;
					int currentFaction = m_aFactionKeys.Find(m_sFaction) + 1;
					SelectFaction((currentFaction + a + factionCount) % factionCount);
				}
				else if (m_bMembersReady && !m_aMemberIds.IsEmpty())
				{
					int memberCount = m_aMemberIds.Count() + 1;
					SelectMember((m_iMember + a + memberCount) % memberCount);
				}
				break;
			}
			case DCO_ESetAct.ONLYMOD:
			{
				m_bOnlyMod = !m_bOnlyMod;
				m_iScroll = 0;
				m_iFocusRow = -1;
				m_iDescKind = DCO_ESetAct.NONE;
				RequestRender();
				break;
			}
			case DCO_ESetAct.CLEAR_SEARCH:
			{
				ClearSearch();
				m_iScroll = 0;
				RequestRender();
				break;
			}
			case DCO_ESetAct.FIND:
			{
				OnKeyFind();
				break;
			}
			case DCO_ESetAct.RESETALL:
			{
				ResetAllDefaults();
				break;
			}
			case DCO_ESetAct.CLOSE:
			case DCO_ESetAct.CANCEL:
			{
				if (m_iPanel != DCO_ESetPanel.NONE)
					ClosePanel();
				else
					RequestClose();
				break;
			}
			case DCO_ESetAct.DISCARD:
			{
				DiscardClose();
				break;
			}
			case DCO_ESetAct.DISCARD_BACK:
			{
				m_bDiscardOpen = false;
				RequestRender();
				break;
			}
			case DCO_ESetAct.DISCARD_APPLY:
			{
				DiscardApply();
				break;
			}
			case DCO_ESetAct.APPLY:
			{
				if (TotalPending() == 0)
					break;
				m_iPanel = DCO_ESetPanel.REVIEW;
				m_iPanelScroll = 0;
				RequestRender();
				break;
			}
			case DCO_ESetAct.CONFIRM:
			{
				DoApply();
				break;
			}
			case DCO_ESetAct.PRESET:
			{
				if (m_iPanel == DCO_ESetPanel.PRESETS)
					ClosePanel();
				else
					OpenPresets();
				break;
			}
			case DCO_ESetAct.EXPORT:
			{
				ExportText();
				break;
			}
			case DCO_ESetAct.IMPORT:
			{
				ImportText();
				break;
			}
			case DCO_ESetAct.LOG:
			{
				m_iPanel = DCO_ESetPanel.LOG;
				m_iPanelScroll = 0;
				RequestLog();
				RequestRender();
				break;
			}
			case DCO_ESetAct.PANEL_CLOSE:
			{
				ClosePanel();
				break;
			}
			case DCO_ESetAct.PRESET_PICK:
			{
				if (m_aPresetNames.IsIndexValid(a))
				{
					m_iPresetSel = a;
					m_sSaveName = m_aPresetNames[a];
					m_bConfirmOverwrite = false;
					m_bConfirmDelete = false;
				}
				RequestRender();
				break;
			}
			case DCO_ESetAct.PRESET_LOAD:
			{
				LoadSelectedPreset();
				break;
			}
			case DCO_ESetAct.PRESET_SAVE:
			{
				SavePreset();
				break;
			}
			case DCO_ESetAct.PRESET_SAVE_LOCAL:
			{
				SaveLocalPreset();
				break;
			}
			case DCO_ESetAct.CFG_SAVE:
			{
				SaveServerConfig();
				break;
			}
			case DCO_ESetAct.MODE:
			{
				SetMode(a);
				break;
			}
			case DCO_ESetAct.HELP:
			{
				m_bHelpOpen = !m_bHelpOpen;
				m_iHelpScroll = 0;
				DCO_SettingsPrefs.SetHelpOpen(m_bHelpOpen);
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_help open=" + m_bHelpOpen.ToString());
				RequestRender();
				break;
			}
			case DCO_ESetAct.SIMPLE:
			{
				m_iFocusRow = -2 - a;
				if (b >= 0)
				{
					ApplySimpleLevel(a, b);
				}
				else
				{
					SetFocusRow(-2 - a);
					SetDesc(DCO_ESetAct.SIMPLE, a, -1);
				}
				break;
			}
			case DCO_ESetAct.PRESET_DELETE:
			{
				DeletePreset();
				break;
			}
			case DCO_ESetAct.LOG_FILTER:
			{
				m_bLogTargetOnly = a == 1;
				RequestLog();
				RequestRender();
				break;
			}
			case DCO_ESetAct.LOG_REFRESH:
			{
				RequestLog();
				RequestRender();
				break;
			}
		}
	}

	protected void SelectFaction(int index)
	{
		string key;
		if (index > 0 && index <= m_aFactionKeys.Count())
			key = m_aFactionKeys[index - 1];
		if (key == m_sFaction)
			return;
		m_sFaction = key;
		m_iScroll = 0;
		m_iFocusRow = -1;
		m_iDescKind = DCO_ESetAct.NONE;
		m_sPresetName = string.Empty;
		if (FactionMode())
		{
			DCO_SetFactionEdit edit;
			if (!m_mFaction.Find(key, edit) || !edit)
			{
				edit = new DCO_SetFactionEdit();
				edit.m_sName = m_aFactionNames[index - 1];
				m_mFaction.Set(key, edit);
			}
			if (!edit.m_bLoaded)
			{
				m_Manager.DCO_FactionState(key);
				m_sStatus = T("set_faction_loading");
			}
		}
		FixSelection();
		UpdateDependencies();
		RequestRender();
	}

	protected string FactionName(string key)
	{
		int i = m_aFactionKeys.Find(key);
		if (i >= 0)
			return m_aFactionNames[i];
		return key;
	}

	protected string EncodeOverrides(DCO_SetFactionEdit edit)
	{
		string outText;
		foreach (string className, vector v : edit.m_mCur)
		{
			if (!outText.IsEmpty())
				outText += ";";
			outText += DCO_SettingsCodec.Entry(className, v);
		}
		return outText;
	}

	protected string PresetScope()
	{
		if (FactionMode())
			return "Faction_" + m_sFaction;
		if (m_iTargetKind == 0)
			return "Global";
		if (m_iTargetKind == 1)
			return "Commander";
		if (m_iTargetKind == 2)
			return "Group";
		if (m_iTargetKind == 3)
			return "Unit";
		return string.Empty;
	}

	protected string PresetScopeLabel()
	{
		if (FactionMode())
			return T2("set_faction_scope", "n", FactionName(m_sFaction));
		string scope = PresetScope();
		if (scope == "Group")
			return T("set_scope_group");
		if (scope == "Unit")
			return T("set_scope_unit");
		if (scope == "Commander")
			return T("set_scope_commander");
		if (scope == "Global")
			return T("set_scope_global");
		return "-";
	}

	protected string ExportScope()
	{
		string scope = PresetScope();
		if (scope.IsEmpty())
			return "Waypoint";
		return scope;
	}

	protected int RowByClass(string className)
	{
		int index;
		if (m_mRowByClass.Find(className, index))
			return index;
		return -1;
	}

	protected bool Exportable(DCO_SetRow row)
	{
		if (row.m_iKind == DCO_ESetKind.TEXT || ShowsMixed(row))
			return false;
		if (row.m_sClass == "DCO_CommanderPresetLoadAttribute")
			return false;
		return true;
	}

	protected string BuildContent()
	{
		if (FactionMode())
		{
			DCO_SetFactionEdit edit = Fac();
			if (!edit)
				return string.Empty;
			return EncodeOverrides(edit);
		}
		string outText;
		foreach (DCO_SetRow row : m_aRows)
		{
			if (!Exportable(row))
				continue;
			if (!outText.IsEmpty())
				outText += ";";
			if (row.m_bMember)
				outText += "m:";
			outText += DCO_SettingsCodec.Entry(row.m_sClass, row.m_vCur);
		}
		return outText;
	}

	protected int ApplyContent(string content)
	{
		array<string> keys = {};
		array<vector> values = {};
		DCO_SettingsCodec.Decode(content, keys, values);
		int applied = 0;
		if (FactionMode())
		{
			DCO_SetFactionEdit edit = Fac();
			if (!edit)
				return 0;
			edit.m_mCur.Clear();
			foreach (int fi, string factionClass : keys)
			{
				int fr = RowByClass(factionClass);
				if (fr < 0 || !FactionRow(m_aRows[fr]))
					continue;
				edit.m_mCur.Set(factionClass, values[fi]);
				applied++;
			}
			UpdateDependencies();
			return applied;
		}
		m_aMemberClasses.Clear();
		m_aMemberValues.Clear();
		foreach (int i, string key : keys)
		{
			bool member = key.StartsWith("m:");
			string className = key;
			if (member)
				className = key.Substring(2, key.Length() - 2);
			int r = RowByClass(className);
			if (r >= 0)
			{
				DCO_SetRow row = m_aRows[r];
				if (row.m_iKind == DCO_ESetKind.TEXT || row.m_sClass == "DCO_CommanderPresetLoadAttribute")
					continue;
				if (!Same(row.m_vCur, values[i]) || ShowsMixed(row))
				{
					row.m_vCur = values[i];
					row.m_bTouched = true;
				}
				applied++;
				continue;
			}
			if (member && m_iTargetKind == 2)
			{
				m_aMemberClasses.Insert(className);
				m_aMemberValues.Insert(values[i]);
				applied++;
			}
		}
		UpdateDependencies();
		return applied;
	}

	protected void OpenPresets()
	{
		string scope = PresetScope();
		if (scope.IsEmpty())
		{
			m_sStatus = T("set_tip_no_presets");
			RequestRender();
			return;
		}
		m_iPanel = DCO_ESetPanel.PRESETS;
		m_iPanelScroll = 0;
		m_iPresetSel = -1;
		m_bConfirmOverwrite = false;
		m_bConfirmDelete = false;
		m_aPresetNames.Clear();
		m_aPresetLocal.Clear();
		if (scope == "Commander")
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
				mgr.DCO_GetPresetNames(m_aPresetNames);
			m_bPresetListReady = true;
		}
		else
		{
			m_bPresetListReady = false;
			m_Manager.DCO_PresetList(scope);
		}
		RequestRender();
	}

	protected void LoadSelectedPreset()
	{
		if (!m_aPresetNames.IsIndexValid(m_iPresetSel))
		{
			m_sStatus = T("set_preset_pick");
			RequestRender();
			return;
		}
		string name = m_aPresetNames[m_iPresetSel];
		if (IsLocalPreset(m_iPresetSel))
		{
			LoadLocalPreset(name);
			return;
		}
		if (PresetScope() == "Commander")
		{
			int r = RowByClass("DCO_CommanderPresetLoadAttribute");
			if (r < 0)
				return;
			DCO_SetRow row = m_aRows[r];
			float value = m_iPresetSel + 1;
			foreach (int i, string entryName : row.m_aNames)
			{
				if (entryName == name)
					value = row.m_aValues[i];
			}
			vector v = row.m_vCur;
			v[0] = value;
			row.m_vCur = v;
			row.m_bTouched = true;
			m_sPresetName = name;
			m_sStatus = T2("set_preset_cmd_pending", "n", name);
			ClosePanel();
			return;
		}
		m_Manager.DCO_PresetLoad(PresetScope(), name);
		m_sStatus = T("set_preset_loading");
		RequestRender();
	}

	protected void SavePreset()
	{
		string name = DCO_CommanderPresets.Sanitize(m_sSaveName);
		if (name.IsEmpty())
		{
			m_sStatus = T("set_preset_bad_name");
			RequestRender();
			return;
		}
		bool exists = ServerPresetExists(name);
		if (exists && !m_bConfirmOverwrite)
		{
			m_bConfirmOverwrite = true;
			m_sStatus = T2("set_preset_exists", "n", name);
			RequestRender();
			return;
		}
		if (PresetScope() == "Commander")
		{
			int r = RowByClass("DCO_CommanderPresetSaveAttribute");
			if (r < 0)
				return;
			DCO_SetRow row = m_aRows[r];
			row.m_sText = name;
			if (exists)
				row.m_sText = name + "!";
			row.m_bTouched = true;
			m_sPresetName = name;
			m_sStatus = T2("set_preset_cmd_save_pending", "n", name);
			ClosePanel();
			return;
		}
		m_Manager.DCO_PresetSave(PresetScope(), name, BuildContent(), exists);
		m_bConfirmOverwrite = false;
		m_sStatus = T("set_preset_saving");
		RequestRender();
	}

	protected void DeletePreset()
	{
		if (PresetScope() == "Commander" || !m_aPresetNames.IsIndexValid(m_iPresetSel))
			return;
		string name = m_aPresetNames[m_iPresetSel];
		if (!m_bConfirmDelete)
		{
			m_bConfirmDelete = true;
			m_sStatus = T2("set_preset_delete_confirm", "n", name);
			RequestRender();
			return;
		}
		m_bConfirmDelete = false;
		if (IsLocalPreset(m_iPresetSel))
		{
			bool deleted = DCO_SettingsPresets.Delete(PresetScope(), name);
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_preset_delete_local scope=" + PresetScope() + " name=" + name + " result=" + deleted.ToString());
			if (deleted)
			{
				m_aPresetNames.RemoveOrdered(m_iPresetSel);
				m_aPresetLocal.RemoveOrdered(m_iPresetSel);
				m_sStatus = T2("set_preset_deleted_local", "n", name);
			}
			m_iPresetSel = -1;
			RequestRender();
			return;
		}
		m_iPresetSel = -1;
		m_Manager.DCO_PresetDelete(PresetScope(), name);
		RequestRender();
	}

	protected bool IsLocalPreset(int index)
	{
		return m_aPresetLocal.IsIndexValid(index) && m_aPresetLocal[index];
	}

	protected bool ServerPresetExists(string name)
	{
		foreach (int i, string presetName : m_aPresetNames)
		{
			if (presetName == name && !IsLocalPreset(i))
				return true;
		}
		return false;
	}

	protected bool LocalPresetsSeparate()
	{
		return !Replication.IsServer() && DCO_SettingsPresets.ValidScope(PresetScope());
	}

	protected void AddLocalPresets(string scope)
	{
		if (!LocalPresetsSeparate() || scope != PresetScope())
			return;
		array<string> locals = {};
		DCO_SettingsPresets.List(scope, locals);
		foreach (string localName : locals)
		{
			m_aPresetNames.Insert(localName);
			m_aPresetLocal.Insert(true);
		}
	}

	protected void LoadLocalPreset(string name)
	{
		string content;
		bool ok = DCO_SettingsPresets.Load(PresetScope(), name, content);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_preset_load_local scope=" + PresetScope() + " name=" + name + " result=" + ok.ToString());
		if (!ok)
		{
			m_sStatus = T2("set_preset_load_fail", "n", name);
			RequestRender();
			return;
		}
		int applied = ApplyContent(content);
		m_sPresetName = name;
		m_sStatus = T2("set_preset_loaded", "n", name, "c", applied.ToString());
		m_iPanel = DCO_ESetPanel.NONE;
		StartupFinish(name, applied);
		RequestRender();
	}

	protected void StartupFinish(string name, int applied)
	{
		if (!m_bStartup)
			return;
		m_bStartup = false;
		int pending = TotalPending();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "startup_preset_apply name=" + name + " read=" + applied.ToString() + " changes=" + pending.ToString());
		if (pending > 0)
			GetGame().GetCallqueue().CallLater(DoApply, 0);
		else
			GetGame().GetCallqueue().CallLater(RequestClose, 0);
	}

	protected void SaveServerConfig()
	{
		int pending = TotalPending();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_cfg_save_click pending=" + pending.ToString());
		if (pending > 0)
		{
			m_sStatus = T2("set_cfgsave_pending", "n", pending.ToString());
			RequestRender();
			return;
		}
		m_sStatus = T("set_cfgsave_wait");
		m_Manager.DCO_SaveServerConfig();
		RequestRender();
	}

	protected void SaveLocalPreset()
	{
		string name = DCO_CommanderPresets.Sanitize(m_sSaveName);
		if (name.IsEmpty() || !LocalPresetsSeparate())
		{
			m_sStatus = T("set_preset_bad_name");
			RequestRender();
			return;
		}
		bool ok = DCO_SettingsPresets.Save(PresetScope(), name, BuildContent());
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_preset_save_local scope=" + PresetScope() + " name=" + name + " result=" + ok.ToString());
		if (!ok)
		{
			m_sStatus = T2("set_preset_save_fail", "n", name);
			RequestRender();
			return;
		}
		bool listed;
		foreach (int i, string presetName : m_aPresetNames)
		{
			if (presetName == name && IsLocalPreset(i))
				listed = true;
		}
		if (!listed)
		{
			m_aPresetNames.Insert(name);
			m_aPresetLocal.Insert(true);
		}
		m_sStatus = T2("set_preset_saved_local", "n", name);
		RequestRender();
	}

	protected void ExportText()
	{
		string content = BuildContent();
		System.ExportToClipboard(DCO_SettingsCodec.ExportText(ExportScope(), content));
		array<string> entries = {};
		content.Split(";", entries, true);
		m_sStatus = T2("set_export_done", "n", entries.Count().ToString());
		RequestRender();
	}

	protected void ImportText()
	{
		string text = System.ImportFromClipboard();
		string scope, content, error;
		if (!DCO_SettingsCodec.ImportText(text, scope, content, error))
		{
			m_sStatus = T("set_import_err_" + error);
			m_Manager.DCO_ReportImport(false, ExportScope(), error, 0);
			RequestRender();
			return;
		}
		if (scope == "AI" && (ExportScope() == "Group" || ExportScope() == "Unit"))
			scope = ExportScope();
		if (scope != ExportScope())
		{
			m_sStatus = T2("set_import_err_scope", "a", scope, "b", ExportScope());
			m_Manager.DCO_ReportImport(false, scope, "scope", 0);
			RequestRender();
			return;
		}
		int applied = ApplyContent(content);
		m_Manager.DCO_ReportImport(true, scope, "ok", applied);
		m_sStatus = T2("set_import_done", "n", applied.ToString());
		m_iPanel = DCO_ESetPanel.REVIEW;
		m_iPanelScroll = 0;
		RequestRender();
	}

	protected void RequestLog()
	{
		m_bLogReady = false;
		m_aLogLines.Clear();
		string target;
		if (m_bLogTargetOnly)
			target = TargetLogName();
		m_Manager.DCO_RequestChangeLog(target);
	}

	protected string TargetLogName()
	{
		if (m_iItems > 1)
			return m_iItems.ToString() + "_entities";
		string name = m_sTarget;
		if (m_iItems == 1)
		{
			array<Managed> items = {};
			m_Manager.GetEditedItems(items);
			if (!items.IsEmpty())
				name = DCO_SettingsBaseline.ItemName(items[0]);
		}
		return DCO_SettingsBaseline.LogText(name);
	}

	protected void OnServerEvent(int kind, string a, string b)
	{
		switch (kind)
		{
			case 1:
			{
				if (a != PresetScope())
					return;
				m_aPresetNames.Clear();
				m_aPresetLocal.Clear();
				if (!b.IsEmpty())
					b.Split(";", m_aPresetNames, true);
				foreach (string serverName : m_aPresetNames)
				{
					m_aPresetLocal.Insert(false);
				}
				AddLocalPresets(a);
				m_bPresetListReady = true;
				if (!m_aPresetNames.IsIndexValid(m_iPresetSel))
					m_iPresetSel = -1;
				break;
			}
			case 2:
			{
				int applied = ApplyContent(b);
				m_sPresetName = a;
				m_sStatus = T2("set_preset_loaded", "n", a, "c", applied.ToString());
				m_iPanel = DCO_ESetPanel.NONE;
				StartupFinish(a, applied);
				break;
			}
			case 3:
			{
				if (a == "save_ok")
				{
					m_sPresetName = b;
					m_sStatus = T2("set_preset_saved", "n", b);
				}
				else if (a == "save_exists")
				{
					m_bConfirmOverwrite = true;
					m_sStatus = T2("set_preset_exists", "n", b);
				}
				else
				{
					m_sStatus = T2("set_preset_" + a, "n", b);
				}
				break;
			}
			case 4:
			{
				m_aLogLines.Clear();
				if (!b.IsEmpty())
					b.Split("\n", m_aLogLines, true);
				m_bLogReady = true;
				break;
			}
			case 5:
			{
				DCO_SetFactionEdit edit;
				if (!m_mFaction.Find(a, edit) || !edit)
					return;
				array<string> keys = {};
				array<vector> values = {};
				DCO_SettingsCodec.Decode(b, keys, values);
				edit.m_mOrig.Clear();
				edit.m_mCur.Clear();
				foreach (int i, string className : keys)
				{
					edit.m_mOrig.Set(className, values[i]);
					edit.m_mCur.Set(className, values[i]);
				}
				edit.m_bLoaded = true;
				if (a == m_sFaction)
					m_sStatus = T2("set_faction_loaded", "n", FactionName(a), "c", keys.Count().ToString());
				UpdateDependencies();
				break;
			}
			case 7:
			{
				OnMembers(a, b);
				break;
			}
			case 6:
			{
				if (b == "ok")
					m_sStatus = T2("set_faction_applied", "n", FactionName(a));
				else
					m_sStatus = T2("set_faction_apply_fail", "n", FactionName(a));
				break;
			}
		}
		RequestRender();
	}

	protected void Warnings(notnull array<string> outWarnings)
	{
		foreach (DCO_SettingsRuleDef rule : DCO_SettingsTree.Get().m_aRules)
		{
			int ra = RowByClass(rule.m_sA);
			if (ra < 0 || ShowsMixed(m_aRows[ra]))
				continue;
			float a = m_aRows[ra].m_vCur[0];
			float b = 0;
			if (rule.m_iKind == 0 || rule.m_iKind == 2)
			{
				int rb = RowByClass(rule.m_sB);
				if (rb < 0 || ShowsMixed(m_aRows[rb]))
					continue;
				b = m_aRows[rb].m_vCur[0];
			}
			bool hit = false;
			switch (rule.m_iKind)
			{
				case 0:
					hit = a > b * rule.m_fFactor;
					break;
				case 1:
					hit = Math.AbsFloat(a - rule.m_fA) < 0.0001;
					break;
				case 2:
					hit = a >= rule.m_fA && Math.AbsFloat(b - rule.m_fB) < 0.0001;
					break;
				case 3:
					hit = a > rule.m_fA;
					break;
			}
			if (hit)
				outWarnings.Insert(rule.m_sMessage);
		}
	}

	protected void RenderPanel()
	{
		float x0 = m_fSheetX;
		float y0 = m_fRuleY + 2;
		float w = m_fSheetW + 10;
		float h = m_fDescY - y0 - 4;
		m_wPresetBox = null;
		Cell(m_wDyn, x0, y0, w, h, string.Empty, m_cPanelBg, m_cText, 0, 10, 0);
		Cell(m_wDyn, x0, y0, w, 2, string.Empty, m_cOrange, m_cText, 0, 10, 0);
		switch (m_iPanel)
		{
			case DCO_ESetPanel.PRESETS:
				RenderPresetPanel(x0, y0, w, h);
				break;
			case DCO_ESetPanel.REVIEW:
				RenderReviewPanel(x0, y0, w, h);
				break;
			case DCO_ESetPanel.LOG:
				RenderLogPanel(x0, y0, w, h);
				break;
		}
	}

	protected void PanelTitle(float x0, float y0, float w, string title)
	{
		Cell(m_wDyn, x0 + 18, y0 + 10, w - 200, 34, title, m_cClear, m_cInk, 1, 22, 0);
		if (m_bPad)
			PadHint(x0 + w - 120, y0 + 18, "B", T("set_back"), true, Act(DCO_ESetAct.PANEL_CLOSE, 0, 0, string.Empty));
		else
			KeyHint(x0 + w - 120, y0 + 18, "ESC", T("set_back"), true, Act(DCO_ESetAct.PANEL_CLOSE, 0, 0, string.Empty));
	}

	protected void RenderPresetPanel(float x0, float y0, float w, float h)
	{
		if (m_bStartup)
			PanelTitle(x0, y0, w, T("set_startup_title"));
		else
			PanelTitle(x0, y0, w, T("set_presets_title") + "  ·  " + PresetScopeLabel());
		float listX = x0 + 18;
		float listW = 480;
		float top = y0 + 56;
		float bottom = y0 + h - 60;
		int visible = (int)((bottom - top) / 43);
		m_iPanelMax = (int)Math.Max(0, m_aPresetNames.Count() - visible);
		m_iPanelScroll = Math.ClampInt(m_iPanelScroll, 0, m_iPanelMax);
		float y = top;
		if (!m_bPresetListReady)
			Cell(m_wDyn, listX, y, listW, 40, T("set_loading"), m_cClear, m_cDim, 0, 16, 0);
		else if (m_aPresetNames.IsEmpty())
			Cell(m_wDyn, listX, y, listW, 40, T("set_presets_empty"), m_cClear, m_cDim, 0, 16, 0);
		for (int i = m_iPanelScroll; i < m_aPresetNames.Count() && i < m_iPanelScroll + visible; i++)
		{
			int pickId = Act(DCO_ESetAct.PRESET_PICK, i, 0, string.Empty);
			if (i == m_iPresetSel)
				SelBg(listX, y, listW, 40, pickId);
			else
				Cell(m_wDyn, listX, y, listW, 40, string.Empty, m_cRowBg, m_cText, 0, 10, pickId);
			Color nameFg = m_cText;
			if (i == m_iPresetSel)
				nameFg = m_cInk;
			Cell(m_wDyn, listX + 12, y, listW - 140, 40, m_aPresetNames[i], m_cClear, nameFg, 0, 18, pickId);
			string source = T("set_preset_src_server");
			if (IsLocalPreset(i))
				source = T("set_preset_src_local");
			else if (!LocalPresetsSeparate())
				source = string.Empty;
			if (!source.IsEmpty())
				Cell(m_wDyn, listX + listW - 128, y, 120, 40, source, m_cClear, m_cDim, 2, 16, pickId);
			y += 43;
		}
		ScrollBar(listX + listW + 6, top, bottom - top, m_iPanelScroll, m_iPanelMax);

		float rx = x0 + 530;
		float ry = top;
		bool commander = PresetScope() == "Commander";
		Cell(m_wDyn, rx, ry, 440, 24, T("set_preset_name"), m_cClear, m_cDim, 0, 16, 0);
		Widget edit = GetGame().GetWorkspace().CreateWidgets(m_sEditLayout, m_wDyn);
		if (edit)
		{
			FrameSlot.SetPos(edit, rx, ry + 26);
			FrameSlot.SetSize(edit, 440, 32);
			ImageWidget editBg = ImageWidget.Cast(edit.FindAnyWidget("Bg"));
			if (editBg)
				editBg.SetColor(m_cCell);
			m_wPresetBox = EditBoxWidget.Cast(edit.FindAnyWidget("Box"));
			if (m_wPresetBox)
			{
				m_wPresetBox.SetText(m_sSaveName);
				m_wPresetBox.SetVirtualKeyboardTitle(T("set_preset_name"));
			}
			Tag(edit, Act(DCO_ESetAct.PRESET_NAME, 0, 0, T("set_tip_preset_name")));
		}
		float ay = ry + 78;
		string saveLabel = T("set_preset_save_btn");
		if (m_bConfirmOverwrite)
			saveLabel = T("set_preset_overwrite_btn");
		if (m_bPad)
		{
			PadHint(rx, ay, "A", T("set_preset_load_btn"), m_iPresetSel >= 0, Act(DCO_ESetAct.PRESET_LOAD, 0, 0, T("set_tip_preset_load")));
			PadHint(rx + 220, ay, "X", saveLabel, true, Act(DCO_ESetAct.PRESET_SAVE, 0, 0, T("set_tip_preset_save")));
			PadHint(rx, ay + 40, "LS", T("set_pad_type_name"), true, 0);
			if (!commander)
			{
				string padDel = T("set_preset_delete_btn");
				if (m_bConfirmDelete)
					padDel = T("set_preset_delete_confirm_btn");
				PadHint(rx, ay + 80, "LT", padDel, m_iPresetSel >= 0, Act(DCO_ESetAct.PRESET_DELETE, 0, 0, T("set_tip_preset_delete")));
				if (LocalPresetsSeparate())
					PadHint(rx + 220, ay + 80, "RT", T("set_preset_save_local_btn"), true, Act(DCO_ESetAct.PRESET_SAVE_LOCAL, 0, 0, T("set_tip_preset_save_local")));
			}
		}
		else
			KeyHint(rx, ay, "Enter", T("set_preset_load_btn"), m_iPresetSel >= 0, Act(DCO_ESetAct.PRESET_LOAD, 0, 0, T("set_tip_preset_load")));
		if (!m_bPad)
			KeyHint(rx + 220, ay, "S", saveLabel, true, Act(DCO_ESetAct.PRESET_SAVE, 0, 0, T("set_tip_preset_save")));
		if (!commander && !m_bPad)
		{
			KeyHint(rx, ay + 40, "X", T("set_export"), true, Act(DCO_ESetAct.EXPORT, 0, 0, T("set_tip_export")));
			KeyHint(rx + 220, ay + 40, "I", T("set_import"), true, Act(DCO_ESetAct.IMPORT, 0, 0, T("set_tip_import")));
			string delLabel = T("set_preset_delete_btn");
			if (m_bConfirmDelete)
				delLabel = T("set_preset_delete_confirm_btn");
			KeyHint(rx, ay + 80, "Del", delLabel, m_iPresetSel >= 0, Act(DCO_ESetAct.PRESET_DELETE, 0, 0, T("set_tip_preset_delete")), false, m_cDanger);
			if (LocalPresetsSeparate())
				KeyHint(rx + 220, ay + 80, "L", T("set_preset_save_local_btn"), true, Act(DCO_ESetAct.PRESET_SAVE_LOCAL, 0, 0, T("set_tip_preset_save_local")));
		}
		string note = T("set_preset_note");
		if (LocalPresetsSeparate())
			note += "\n" + T("set_preset_note_local");
		if (commander)
			note = T("set_preset_note_cmd");
		else if (PresetScope() == "Group")
			note = T("set_preset_note_group");
		else if (PresetScope() == "Unit")
			note = T("set_preset_note_unit");
		Widget noteCell = Cell(m_wDyn, rx, ay + 124, w - 550, 130, note, m_cClear, m_cDim, 0, 16, 0);
		WrapCell(noteCell);
		if (!m_sStatus.IsEmpty())
		{
			Widget statusCell = Cell(m_wDyn, rx, ay + 258, w - 550, 56, m_sStatus, m_cClear, m_cAmber, 0, 16, 0);
			WrapCell(statusCell);
		}

		if (m_bStartup)
			return;
		if (PresetScope() == "Global")
		{
			string cfgLabel = T("set_preset_cfgsave_btn");
			Cell(m_wDyn, listX, y0 + h - 46, TextW(cfgLabel, 9.2) + 40, 34, cfgLabel, m_cCell, m_cInk, 3, 17, Act(DCO_ESetAct.CFG_SAVE, 0, 0, T("set_tip_cfgsave")));
		}
		string resetLabel = T("set_reset_all_default");
		if (m_bConfirmReset)
			resetLabel = T("set_reset_all_confirm");
		float resetW = TextW(resetLabel, 9.2) + 38;
		if (m_bPad)
			PadHint(x0 + w - 18 - resetW, y0 + h - 40, "RS", resetLabel, true, Act(DCO_ESetAct.RESETALL, 0, 0, T("set_tip_reset_all")));
		else
			KeyHint(x0 + w - 18 - resetW, y0 + h - 40, "Del", resetLabel, false, Act(DCO_ESetAct.RESETALL, 0, 0, T("set_tip_reset_all")), false, m_cDanger);
	}

	protected void WrapCell(Widget cell)
	{
		if (!cell)
			return;
		RichTextWidget t = RichTextWidget.Cast(cell.FindAnyWidget("Text"));
		if (t)
			t.SetTextWrapping(true);
	}

	protected string ScopeText()
	{
		if (FactionMode())
			return FactionName(m_sFaction);
		if (MemberScope() && m_aMemberNames.IsIndexValid(m_iMember - 1))
			return m_aMemberNames[m_iMember - 1];
		if (m_bGlobalTarget)
			return T("set_faction_base");
		if (m_iTargetKind == 2 && m_bMembersReady)
			return T("set_members_all");
		return string.Empty;
	}

	protected void RenderReviewPanel(float x0, float y0, float w, float h)
	{
		PanelTitle(x0, y0, w, T("set_review_title"));

		array<string> lines = {};
		array<int> kinds = {};
		for (int meterLine = 0; meterLine < 3 && HasMeterRows(); meterLine++)
		{
			lines.Insert(MeterReviewLine(meterLine));
			kinds.Insert(5);
		}
		foreach (DCO_SetSub sub : m_aSubs)
		{
			foreach (int r : sub.m_aRows)
			{
				DCO_SetRow row = m_aRows[r];
				if (row.m_bDisabled || !IsBasePending(row))
					continue;
				string oldText;
				if (row.m_iKind == DCO_ESetKind.TEXT)
					oldText = row.m_sTextOrig;
				else if (row.m_bConflict)
					oldText = T("set_mixed");
				else
					oldText = ValueText(row, row.m_vOrig);
				string newText;
				if (row.m_iKind == DCO_ESetKind.TEXT)
					newText = row.m_sText;
				else
					newText = ValueText(row, row.m_vCur);
				string prefix;
				if (row.m_bMember)
					prefix = Dim("[" + T("set_members_all") + "]  ");
				lines.Insert(prefix + row.m_sLabel + "      " + Dim(oldText) + "   →   " + newText);
				kinds.Insert(1);
				if (row.m_iKind != DCO_ESetKind.TEXT)
				{
					string heavy = DCO_SettingsTree.HeavyReason(row.m_sClass, row.m_vCur[0]);
					if (!heavy.IsEmpty())
					{
						lines.Insert(heavy);
						kinds.Insert(4);
					}
				}
			}
		}
		foreach (string factionKey, DCO_SetFactionEdit edit : m_mFaction)
		{
			if (!edit || FactionChanges(edit) == 0)
				continue;
			foreach (DCO_SetSub fsub : m_aSubs)
			{
				foreach (int fr : fsub.m_aRows)
				{
					DCO_SetRow frow = m_aRows[fr];
					if (!FactionRow(frow) || !FactionPending(edit, frow.m_sClass))
						continue;
					vector oldValue, newValue;
					bool hadOld = edit.m_mOrig.Find(frow.m_sClass, oldValue);
					bool hasNew = edit.m_mCur.Find(frow.m_sClass, newValue);
					string fromText = T("set_global_tag") + " (" + ValueText(frow, frow.m_vOrig) + ")";
					if (hadOld)
						fromText = ValueText(frow, oldValue);
					string toText = T("set_global_tag") + " (" + ValueText(frow, frow.m_vOrig) + ")";
					if (hasNew)
						toText = ValueText(frow, newValue);
					lines.Insert(Dim("[" + FactionName(factionKey) + "]  ") + frow.m_sLabel + "      " + Dim(fromText) + "   →   " + toText);
					kinds.Insert(1);
				}
			}
		}
		foreach (int memberIndex, DCO_SetFactionEdit memberEdit : m_mMemberEdits)
		{
			if (!memberEdit || FactionChanges(memberEdit) == 0 || !m_aMemberNames.IsIndexValid(memberIndex))
				continue;
			foreach (string className, vector value : memberEdit.m_mCur)
			{
				if (!FactionPending(memberEdit, className))
					continue;
				int mr = RowByClass(className);
				if (mr < 0)
					continue;
				DCO_SetRow mrow = m_aRows[mr];
				vector before;
				memberEdit.m_mOrig.Find(className, before);
				lines.Insert(Dim("[" + m_aMemberNames[memberIndex] + "]  ") + mrow.m_sLabel + "      " + Dim(ValueText(mrow, before)) + "   →   " + ValueText(mrow, value));
				kinds.Insert(1);
			}
		}
		if (!m_aMemberClasses.IsEmpty())
		{
			foreach (int mi, string memberClass : m_aMemberClasses)
			{
				vector mv = m_aMemberValues[mi];
				lines.Insert(Dim("[" + T("set_members_all") + "]  ") + m_Manager.DCO_LabelOf(memberClass) + "   →   " + DCO_SettingsCodec.Num(mv[0]));
				kinds.Insert(1);
			}
		}
		int changes = 0;
		foreach (int k : kinds)
		{
			if (k == 1)
				changes++;
		}
		if (changes == 0)
		{
			lines.Insert(T("set_review_none"));
			kinds.Insert(2);
		}
		array<string> warnings = {};
		Warnings(warnings);
		foreach (string warn : warnings)
		{
			lines.Insert(warn);
			kinds.Insert(4);
		}

		float top = y0 + 56;
		float bottom = y0 + h - 56;
		int visible = (int)((bottom - top) / 38);
		m_iPanelMax = (int)Math.Max(0, lines.Count() - visible);
		m_iPanelScroll = Math.ClampInt(m_iPanelScroll, 0, m_iPanelMax);
		float y = top;
		for (int i = m_iPanelScroll; i < lines.Count() && i < m_iPanelScroll + visible; i++)
		{
			int kind = kinds[i];
			if (kind == 1)
			{
				Cell(m_wDyn, x0 + 18, y, w - 36, 36, string.Empty, m_cRowBg, m_cText, 0, 10, 0);
				Cell(m_wDyn, x0 + 30, y, w - 60, 36, lines[i], m_cClear, m_cInk, 0, 16, 0);
				y += 38;
			}
			else if (kind == 4)
			{
				Cell(m_wDyn, x0 + 30, y - 2, w - 60, 28, lines[i], m_cClear, m_cAmber, 0, 16, 0);
				y += 28;
			}
			else if (kind == 5)
			{
				Cell(m_wDyn, x0 + 30, y, w - 60, 30, lines[i], m_cClear, m_cText, 0, 16, 0);
				y += 32;
			}
			else
			{
				Cell(m_wDyn, x0 + 30, y, w - 60, 36, lines[i], m_cClear, m_cDim, 0, 16, 0);
				y += 38;
			}
		}
		ScrollBar(x0 + w - 12, top, bottom - top, m_iPanelScroll, m_iPanelMax);

		string info = T2("set_review_count", "n", changes.ToString(), "t", m_sTarget);
		string scope = ScopeText();
		if (!scope.IsEmpty())
			info += "  ·  " + scope;
		Cell(m_wDyn, x0 + 18, y0 + h - 44, w - 300, 30, info, m_cClear, m_cDim, 0, 16, 0);
		float applyW = 32 + TextW(T("set_confirm_apply"), 10.4);
		if (m_bPad)
			PadHint(x0 + w - 18 - applyW - 24, y0 + h - 38, "A", T("set_confirm_apply"), changes > 0, Act(DCO_ESetAct.CONFIRM, 0, 0, T("set_tip_apply")), true);
		else
			KeyHint(x0 + w - 18 - applyW - 24, y0 + h - 38, "Enter", T("set_confirm_apply"), changes > 0, Act(DCO_ESetAct.CONFIRM, 0, 0, T("set_tip_apply")), true);
	}

	protected void RenderLogPanel(float x0, float y0, float w, float h)
	{
		PanelTitle(x0, y0, w, T("set_log_title"));
		float y = y0 + 56;
		Color allBg = m_cTab;
		Color allFg = m_cDim;
		Color tgtBg = m_cTab;
		Color tgtFg = m_cDim;
		if (m_bLogTargetOnly)
		{
			tgtBg = m_cOrange;
			tgtFg = m_cWhite;
		}
		else
		{
			allBg = m_cOrange;
			allFg = m_cWhite;
		}
		Cell(m_wDyn, x0 + 18, y, 170, 30, T("set_log_all"), allBg, allFg, 3, 16, Act(DCO_ESetAct.LOG_FILTER, 0, 0, string.Empty));
		Cell(m_wDyn, x0 + 190, y, 220, 30, T("set_log_target"), tgtBg, tgtFg, 3, 16, Act(DCO_ESetAct.LOG_FILTER, 1, 0, TargetLogName()));
		Cell(m_wDyn, x0 + 422, y, 150, 30, T("set_log_refresh"), m_cClear, m_cOrange, 0, 16, Act(DCO_ESetAct.LOG_REFRESH, 0, 0, string.Empty));
		y += 42;

		if (!m_bLogReady)
		{
			Cell(m_wDyn, x0 + 18, y, w - 36, 28, T("set_loading"), m_cClear, m_cDim, 0, 16, 0);
			return;
		}
		if (m_aLogLines.IsEmpty())
		{
			Cell(m_wDyn, x0 + 18, y, w - 36, 28, T("set_log_empty"), m_cClear, m_cDim, 0, 16, 0);
			return;
		}
		float bottom = y0 + h - 12;
		int visible = (int)((bottom - y) / 32);
		m_iPanelMax = (int)Math.Max(0, m_aLogLines.Count() - visible);
		m_iPanelScroll = Math.ClampInt(m_iPanelScroll, 0, m_iPanelMax);
		float top = y;
		for (int i = m_iPanelScroll; i < m_aLogLines.Count() && i < m_iPanelScroll + visible; i++)
		{
			array<string> f = {};
			m_aLogLines[i].Split("|", f, false);
			string text = m_aLogLines[i];
			if (f.Count() >= 6)
				text = Dim(f[0] + "   " + f[1] + "   " + f[2]) + "   " + f[3] + ":  " + Dim(f[4]) + "  →  " + f[5];
			Cell(m_wDyn, x0 + 18, y, w - 36, 30, string.Empty, m_cRowBg, m_cText, 0, 10, 0);
			Cell(m_wDyn, x0 + 28, y, w - 56, 30, text, m_cClear, m_cInk, 0, 16, 0);
			y += 32;
		}
		ScrollBar(x0 + w - 12, top, bottom - top, m_iPanelScroll, m_iPanelMax);
	}

	protected void OnCell(DCO_SetRow row, int valueIndex)
	{
		if (row.m_bDisabled)
			return;
		switch (row.m_iKind)
		{
			case DCO_ESetKind.CHECK:
				SetCur(row, valueIndex);
				break;
			case DCO_ESetKind.DROP:
			{
				if (row.m_aValues.IsIndexValid(valueIndex))
					SetCur(row, row.m_aValues[valueIndex]);
				break;
			}
			case DCO_ESetKind.MULTI:
			{
				if (!row.m_aValues.IsIndexValid(valueIndex))
					break;
				vector maskValue = ViewValue(row);
				int mask = maskValue[0];
				if (ShowsMixed(row))
					mask = 0;
				mask = mask ^ Bit(row, valueIndex);
				SetCur(row, mask);
				break;
			}
			case DCO_ESetKind.SLIDER:
				SetCur(row, Level(row, valueIndex));
				break;
		}
	}

	protected void ClearSearch()
	{
		m_sSearch = string.Empty;
		if (m_wSearch)
			m_wSearch.SetText(string.Empty);
	}

	void OnEditChange(Widget w, bool finished)
	{
		EditBoxWidget box = EditBoxWidget.Cast(w);
		if (!box)
			return;
		int id = w.GetUserID();
		if (id == 100001)
		{
			string text = box.GetText();
			text = text.Trim();
			if (text == m_sSearch)
				return;
			m_sSearch = text;
			m_iScroll = 0;
			RequestRender();
			return;
		}
		if (id <= 0 || id >= m_aActKind.Count())
			return;
		int kind = m_aActKind[id];
		int a = m_aActA[id];
		if (kind == DCO_ESetAct.PRESET_NAME)
		{
			m_sSaveName = box.GetText();
			m_bConfirmOverwrite = false;
			return;
		}
		if (kind == DCO_ESetAct.SPAWNER && a == 40)
		{
			m_sSpBName = box.GetText();
			m_bSpBConfirm = false;
			m_sSpMsg = string.Empty;
			return;
		}
		if (!m_aRows.IsIndexValid(a))
			return;
		DCO_SetRow row = m_aRows[a];
		if (kind == DCO_ESetAct.TEXTBOX)
		{
			int boxAt = m_aTextBox.Find(box);
			if (m_bRendering || boxAt < 0 || m_aTextRow[boxAt] != a)
				return;
			string value = box.GetText();
			if (value != row.m_sText)
			{
				row.m_sText = value;
				row.m_bTouched = true;
			}
			if (finished)
				RequestRender();
			return;
		}
		if (kind == DCO_ESetAct.NUMBOX && finished)
		{
			string raw = box.GetText();
			raw = raw.Trim();
			if (raw.IsEmpty() || raw == T("set_mixed"))
				return;
			float v = Quant(row, raw.ToFloat());
			vector viewNow = ViewValue(row);
			if (!ShowsMixed(row) && Near(row, v, viewNow[0]))
				return;
			SetCur(row, v);
		}
	}

	void OnWheel(int x, int wheel)
	{
		float wx, wy;
		m_wWindow.GetScreenPos(wx, wy);
		float local = GetGame().GetWorkspace().DPIUnscale(x - wx);
		int delta = 3;
		if (wheel > 0)
			delta = -3;
		if (m_bHelpOpen && local > m_fWinW - 8)
		{
			int helpStep = 1;
			if (wheel > 0)
				helpStep = -1;
			int help = Math.ClampInt(m_iHelpScroll + helpStep, 0, m_iHelpMax);
			if (help == m_iHelpScroll)
				return;
			m_iHelpScroll = help;
			RequestRender();
			return;
		}
		if (m_bDiscardOpen)
			return;
		if (m_iPanel != DCO_ESetPanel.NONE && local >= m_fSheetX)
		{
			int panel = Math.ClampInt(m_iPanelScroll + delta, 0, m_iPanelMax);
			if (panel == m_iPanelScroll)
				return;
			m_iPanelScroll = panel;
		}
		else if (local < m_fSheetX)
		{
			int tree = Math.ClampInt(m_iTreeScroll + delta, 0, m_iMaxTreeScroll);
			if (tree == m_iTreeScroll)
				return;
			m_iTreeScroll = tree;
		}
		else
		{
			int sheet = Math.ClampInt(m_iScroll + delta, 0, m_iMaxScroll);
			if (sheet == m_iScroll)
				return;
			m_iScroll = sheet;
		}
		RequestRender();
	}

	void OnHover(int id, bool enter)
	{
		if (enter)
			m_iHoverAny = id;
		else if (id == m_iHoverAny)
			m_iHoverAny = 0;
		if (m_bDiscardOpen)
			return;
		if (id > 0 && id < m_aActKind.Count() && m_iPanel == DCO_ESetPanel.NONE && IsDescAct(m_aActKind[id]))
		{
			HideTip();
			int descKind = m_aActKind[id];
			int descA = m_aActA[id];
			int rowKey = descA;
			if (descKind == DCO_ESetAct.SIMPLE || descKind == DCO_ESetAct.REVERT_SIMPLE)
				rowKey = -2 - descA;
			if (enter)
			{
				m_iHover = id;
				if (rowKey != m_iFocusRow)
					SetFocusRow(rowKey);
				if (descKind == DCO_ESetAct.CELL)
					m_iFocusCell = m_aActB[id];
				SetDesc(descKind, descA, m_aActB[id]);
			}
			else if (id == m_iHover)
			{
				m_iHover = 0;
				if (descKind == DCO_ESetAct.SIMPLE || descKind == DCO_ESetAct.REVERT_SIMPLE)
					SetDesc(DCO_ESetAct.SIMPLE, descA, -1);
				else
					SetDesc(DCO_ESetAct.ROW, descA, 0);
			}
			return;
		}
		if (!enter)
		{
			if (id == m_iHover)
			{
				m_iHover = 0;
				HideTip();
			}
			return;
		}
		if (id <= 0 || id >= m_aActTip.Count())
			return;
		m_iHover = id;
		ShowTip(m_aActTip[id]);
	}

	protected void ShowTip(string text)
	{
		if (!m_wTip || text.IsEmpty())
		{
			HideTip();
			return;
		}
		RichTextWidget t = RichTextWidget.Cast(m_wTip.FindAnyWidget("Text"));
		if (!t)
			return;
		t.SetText(text);

		array<string> parts = {};
		text.Split("\n", parts, false);
		int lines = 0;
		foreach (string p : parts)
		{
			lines += (int)Math.Max(1, Math.Ceil(p.Length() / 52.0));
		}
		float w = 420;
		float h = lines * 21 + 14;

		WorkspaceWidget ws = GetGame().GetWorkspace();
		int mx, my;
		WidgetManager.GetMousePos(mx, my);
		float cx = ws.DPIUnscale(mx);
		float cy = ws.DPIUnscale(my);
		float sw, sh;
		GetRootWidget().GetScreenSize(sw, sh);
		sw = ws.DPIUnscale(sw);
		sh = ws.DPIUnscale(sh);
		float x = cx + 18;
		float y = cy + 22;
		if (x + w > sw - 8)
			x = cx - w - 14;
		if (y + h > sh - 8)
			y = cy - h - 14;
		FrameSlot.SetPos(m_wTip, x, y);
		FrameSlot.SetSize(m_wTip, w, h);
		m_wTip.SetVisible(true);
	}

	protected void HideTip()
	{
		if (m_wTip)
			m_wTip.SetVisible(false);
	}

	protected void DoApply()
	{
		array<string> warnings = {};
		Warnings(warnings);
		string applyScope = "base";
		if (FactionMode())
			applyScope = "faction";
		else if (MemberScope())
			applyScope = "member";
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_review_confirm changes=" + TotalPending().ToString() + " warnings=" + warnings.Count().ToString() + " scope=" + applyScope + " members=" + m_aMemberIds.Count().ToString() + " edited_members=" + m_mMemberEdits.Count().ToString() + " input=" + InputTag());
		LogMetersApply();
		foreach (string factionKey, DCO_SetFactionEdit edit : m_mFaction)
		{
			if (edit && edit.m_bLoaded && FactionChanges(edit) > 0)
				m_Manager.DCO_FactionApply(factionKey, EncodeOverrides(edit));
		}
		m_sFaction = string.Empty;
		m_iMember = 0;
		UpdateDependencies();
		array<int> ids = {};
		array<string> memberClasses = {};
		array<float> memberValues = {};
		foreach (int mci, string presetClass : m_aMemberClasses)
		{
			if (RowByClass(presetClass) >= 0)
				continue;
			vector presetValue = m_aMemberValues[mci];
			memberClasses.Insert(presetClass);
			memberValues.Insert(presetValue[0]);
			memberValues.Insert(presetValue[1]);
			memberValues.Insert(presetValue[2]);
		}
		foreach (DCO_SetRow row : m_aRows)
		{
			if (row.m_bDisabled || !IsBasePending(row))
				continue;
			if (row.m_bMember)
			{
				memberClasses.Insert(row.m_sClass);
				memberValues.Insert(row.m_vCur[0]);
				memberValues.Insert(row.m_vCur[1]);
				memberValues.Insert(row.m_vCur[2]);
				continue;
			}
			if (row.m_iKind == DCO_ESetKind.TEXT)
			{
				DCO_NameEditorAttribute nameAttribute = DCO_NameEditorAttribute.Cast(row.m_Attr);
				if (!nameAttribute)
					continue;
				nameAttribute.SetPendingName(row.m_sText);
				SCR_BaseEditorAttributeVar textVar = nameAttribute.GetVariable(true);
				textVar.SetInt(1);
			}
			else
			{
				SCR_BaseEditorAttributeVar var = row.m_Attr.GetVariable(true);
				var.SetVector(row.m_vCur);
				if (row.m_bConflict)
					row.m_Attr.SetIsOverridingValues(true);
			}
			ids.Insert(m_Manager.DCO_IndexOf(row.m_Attr));
		}
		int resetAll = 0;
		if (m_bResetAll)
			resetAll = 1;
		if (!memberClasses.IsEmpty())
			m_Manager.DCO_ApplyMembers(memberClasses, memberValues);
		foreach (int memberIndex, DCO_SetFactionEdit memberEdit : m_mMemberEdits)
		{
			if (!memberEdit || !m_aMemberIds.IsIndexValid(memberIndex))
				continue;
			array<string> overrideClasses = {};
			array<float> overrideValues = {};
			foreach (string overrideClass, vector overrideValue : memberEdit.m_mCur)
			{
				if (!FactionPending(memberEdit, overrideClass))
					continue;
				overrideClasses.Insert(overrideClass);
				overrideValues.Insert(overrideValue[0]);
				overrideValues.Insert(overrideValue[1]);
				overrideValues.Insert(overrideValue[2]);
			}
			if (!overrideClasses.IsEmpty())
				m_Manager.DCO_ApplyMemberOverride(m_aMemberIds[memberIndex], overrideClasses, overrideValues);
		}
		m_Manager.DCO_BeginApply(ids, resetAll, warnings.Count());
		m_bEnded = true;
		m_Manager.ConfirmEditing();
		if (m_Manager.DCO_IsMode())
			CloseSelf();
	}
}

class DCO_MeterState
{
	int m_iApplied;
	int m_iPending;
	int m_iDefault;
	bool m_bPartial;
	ref array<string> m_aLabels;
	ref array<float> m_aPoints;
	ref array<float> m_aDelta;

	void DCO_MeterState()
	{
		m_aLabels = {};
		m_aPoints = {};
		m_aDelta = {};
	}
}
