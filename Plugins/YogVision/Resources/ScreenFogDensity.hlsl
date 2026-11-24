// Volume-domain extinction/emission. No viewport UV or screen brush.
FYogScreenFogRegion region;
FYogScreenBrushFlow flow;
// Editor preview clears around the world-space point in front of the camera;
// gameplay supplies the real observer position. Only this clearing is mobile.
float3 observer=VisionSource.a>.5 ? VisionSource.xyz : CameraPosition+normalize(ResolvedView.ViewForward)*NearOffset;
float distance=length(WorldPosition-CameraPosition);
float mass=region.Cluster(WorldPosition,NoiseScale);
float4 masks=region.Volume(WorldPosition,observer,WorldVisionRadius,WorldVisionFeather,
    (mass-.5)*BoundaryDetail*WorldVisionFeather,UseScreenVision,LightClearStrength,
    LightPosition0,LightDirection0,LightColour0,LightPosition1,LightDirection1,LightColour1,
    LightPosition2,LightDirection2,LightColour2,LightPosition3,LightDirection3,LightColour3);
float x=saturate((distance-NearOffset)/max(DepthRamp,1));
float ramp=pow(x*x*(3-2*x),max(DepthPower,.1));
float nearDensity=clamp(NearDensity,0,max(Density,0));
float minimum=clamp(ClearMinDensity,0,nearDensity);
float cameraClear=smoothstep(ClearRadius,ClearRadius+max(ClearFeather,1),distance);
float medium=lerp(minimum,lerp(nearDensity,max(Density,0),ramp),masks.x*cameraClear);
// A world-fixed bounded fog bank supplies an actual front, back and height.
// Camera-distance tint is allowed; bank density never depends on that tint.
float3 bankP=(WorldPosition-FogBankCenter.xyz)/max(FogBankExtent.xyz,50);
float bank=1-smoothstep(.62,1.0,length(bankP)+(mass-.5)*NoiseAmount*.65);
float bounded=lerp(nearDensity,max(Density,0),bank);
bounded=lerp(minimum,bounded,masks.x*cameraClear);
medium=lerp(medium,bounded,saturate(FogBankStrength));
// Height datum is fixed in world space, not relative to camera height.
medium*=exp2(-max(WorldPosition.z-CameraHeightOffset,0)/max(HeightFalloff,10));
float paint=flow.VolumePaint(StrokeTexture,StrokeTextureSampler,FlowTexture,FlowTextureSampler,
    WorldPosition,TimeSeconds*StrokeFlowSpeed,StrokeScale,StrokeWarp,FlowMapScale);
float ink=clamp((paint-.4017)*8,-1,1);
// Brush changes actual optical depth; UE lights and shadows are integrated after it.
float masses=lerp(1,smoothstep(.28,.65,mass)*2.6,NoiseAmount);
medium*=max(.08,masses*(1+ink*StrokeStrength*1.8));
float3 key=float3(cos(radians(FormLightPitch))*cos(radians(FormLightYaw)),
    cos(radians(FormLightPitch))*sin(radians(FormLightYaw)),sin(radians(FormLightPitch)));
float relief=(region.Cluster(WorldPosition+key*120,NoiseScale)-mass)*4;
float form=VolumeFormStrength*masks.y+OuterVolumeStrength;
float3 tint=1+form*(max(relief,0)*FormLightStrength*FormLightColor.rgb
    -max(-relief,0)*FormShadowStrength);
float3 palette=lerp(ArtistNearColor.rgb,ArtistFarColor.rgb,saturate(distance/max(PaletteDistance,1)))
    *saturate(ArtistFogMix)*PaletteStrength;
float3 airColor=lerp(AmbientColor.rgb,palette,saturate(UseArtistPalette));
return float4(max(medium*airColor*AmbientIntensity*tint,0),max(medium,0));
