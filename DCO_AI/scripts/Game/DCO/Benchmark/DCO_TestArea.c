[EntityEditorProps(category: "DCO/Test", description: "Workbench-only test area overlay", sizeMin: "-2 -2 -2", sizeMax: "2 2 2", visible: false)]
class DCO_TestAreaClass : GenericEntityClass
{
}

class DCO_TestArea : GenericEntity
{
	[Attribute("Test", UIWidgets.EditBoxMultiline, "Label shown above the area")]
	string m_sLabel;

	[Attribute("50", UIWidgets.EditBox, "Area radius in meters")]
	float m_fRadius;

	[Attribute("1 0.8 0", UIWidgets.ColorPicker, "Overlay color")]
	vector m_vColor;

	[Attribute("0 0 0", UIWidgets.EditBox, "Optional arrow target in world coords, 0 0 0 = none")]
	vector m_vArrowTo;

#ifdef WORKBENCH
	override int _WB_GetAfterWorldUpdateSpecs(IEntitySource src)
	{
		return EEntityFrameUpdateSpecs.CALL_WHEN_ENTITY_VISIBLE;
	}

	override void _WB_AfterWorldUpdate(float timeSlice)
	{
		BaseWorld w = GetWorld();
		if (!w)
			return;
		vector c = GetOrigin();
		int col = ARGBF(1, m_vColor[0], m_vColor[1], m_vColor[2]);
		ShapeFlags f = ShapeFlags.ONCE | ShapeFlags.NOZBUFFER;

		vector ring[49];
		for (int i = 0; i < 49; i++)
		{
			float a = i * Math.PI2 / 48;
			float x = c[0] + Math.Cos(a) * m_fRadius;
			float z = c[2] + Math.Sin(a) * m_fRadius;
			ring[i] = Vector(x, w.GetSurfaceY(x, z) + 1, z);
		}
		Shape.CreateLines(col, f, ring, 49);

		float gy = w.GetSurfaceY(c[0], c[2]);
		vector pole[2];
		pole[0] = Vector(c[0], gy, c[2]);
		pole[1] = Vector(c[0], gy + 25, c[2]);
		Shape.CreateLines(col, f, pole, 2);

		if (m_vArrowTo != vector.Zero)
		{
			vector to = Vector(m_vArrowTo[0], w.GetSurfaceY(m_vArrowTo[0], m_vArrowTo[2]) + 2, m_vArrowTo[2]);
			Shape.CreateArrow(Vector(c[0], gy + 2, c[2]), to, 6, col, f);
		}

		DebugTextWorldSpace.Create(w, m_sLabel, DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA | DebugTextFlags.ONCE, c[0], gy + 27, c[2], 16, col, ARGBF(0.6, 0, 0, 0));
	}
#endif
}
