static bool IsInOpenArea(IEntity aiEntity, float checkRadius = 15.0)
{
    if (!aiEntity) return false;

    vector center = aiEntity.GetOrigin();
    center[1] = center[1] + 1.0; 

    BaseWorld world = GetGame().GetWorld();
    int openDirections = 0;
    int totalDirections = 8;

    for (int i = 0; i < totalDirections; i++)
    {
        float angle = (i * (360.0 / totalDirections)) * Math.DEG2RAD;
        
        vector endPos = center;
        endPos[0] = center[0] + (Math.Sin(angle) * checkRadius);
        endPos[2] = center[2] + (Math.Cos(angle) * checkRadius);

        TraceParam param = new TraceParam();
        param.Start = center;
        param.End = endPos;
        param.Exclude = aiEntity;
        param.LayerMask = EPhysicsLayerDefs.Projectile;

        float hitFraction = world.TraceMove(param, null);

        if (hitFraction == 1.0)
        {
            openDirections++;
        }
    }

    if (openDirections >= 6)
    {
        return true;
    }

    return false;
}