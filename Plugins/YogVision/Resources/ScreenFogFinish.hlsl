float3 scene=max(SceneColor.rgb,0);
float2 uv=GetViewportUV(Parameters);
float4 integrated=Texture2DSampleLevel(AirFogTransmittance,AirFogTransmittanceSampler,uv,0);
if(!all(IsFinite(integrated))) integrated=float4(0,0,0,1);
float transmission=saturate(integrated.a),air=1-transmission;
if(AirFogDebug>.5 && AirFogDebug<1.5) return air.xxx;
if(Density<=0) return scene;
float3 scatter=max(integrated.rgb,0)*View.OneOverPreExposure;
float3 fogColour=scatter/max(air,.0001);
FYogScreenFogRegion region;
float3 forward=normalize(ResolvedView.ViewForward);
float3 right=normalize(ResolvedView.ViewRight),up=normalize(ResolvedView.ViewUp);
float3 observer=VisionSource.a>.5 ? VisionSource.xyz : CameraPosition+normalize(ResolvedView.ViewForward)*NearOffset;
float mass=region.Cluster(WorldPosition,NoiseScale);
float4 masks=region.Volume(WorldPosition,observer,WorldVisionRadius,WorldVisionFeather,
    (mass-.5)*BoundaryDetail*WorldVisionFeather,UseScreenVision,LightClearStrength,
    LightPosition0,LightDirection0,LightColour0,LightPosition1,LightDirection1,LightColour1,
    LightPosition2,LightDirection2,LightColour2,LightPosition3,LightDirection3,LightColour3);
// Debug 2 is the shared screen region; debug 3 is its local-light data.
// Debug 1 above remains the actual native integration readback.
if(AirFogDebug>2.5) return masks.z.xxx;
if(AirFogDebug>1.5) return float3(masks.x,masks.y,masks.z);
// Only surface form and outlines remain here. All brush and fog relief
// live in the Volume material, before UE's lighting and integration.
float3 key=float3(cos(radians(FormLightPitch))*cos(radians(FormLightYaw)),
    cos(radians(FormLightPitch))*sin(radians(FormLightYaw)),sin(radians(FormLightPitch)));
// SceneColor already contains the native volume. Additional flattening is
// restrained to retain architectural planes instead of applying fog twice.
float3 color=scene;

// Keep the source MI's near/far hue relationship, but soften saturation in
// dense unlit air to the approved slate-grey direction. Lit pockets retain it.
// A restrained, late-stage artistic body keeps dense air readable
// in this very dark corridor. It uses the SAME MI near/far palette and actual
// native coverage; native integrated light still supplies the lit variation.
float3 bodyPalette=lerp(ArtistNearColor.rgb,ArtistFarColor.rgb,saturate(SceneDepth/max(PaletteDistance,1)))
    *saturate(ArtistFogMix)*PaletteStrength;
float3 bodyTint=lerp(dot(bodyPalette,float3(.2126,.7152,.0722)).xxx,bodyPalette,.25);
// No flat additive body tint. Native volume radiance owns the air appearance.
bool surface=IsFinite(SceneDepth)&&SceneDepth>0&&SceneDepth<1000000;
float normalLength=dot(SceneNormal.rgb,SceneNormal.rgb);
bool valid=surface&&all(IsFinite(SceneNormal.rgb))&&normalLength>.01;
if(valid)
{
    float f=dot(SceneNormal.rgb*rsqrt(normalLength),key);
    float visibility=.28+.72*transmission;
    float3 base=all(IsFinite(SceneBaseColor.rgb))?saturate(SceneBaseColor.rgb):.18;
    color*=1-FormShadowStrength*(1-smoothstep(-.8,.1,f))*visibility;
    color+=FormLightColor.rgb*(.008+base*.04)*FormLightStrength*smoothstep(-.1,.8,f)*visibility;
    color+=base*.018*saturate(1-masks.w)*transmission*FormLightStrength;
}
if(!surface) return max(color,0);
float2 duv=GetDefaultSceneTextureUV(Parameters,1),nuv=GetDefaultSceneTextureUV(Parameters,8),ruv=GetDefaultSceneTextureUV(Parameters,11);
float2 ds=GetSceneTextureBufferSize(1).zw*EdgeWidth,ns=GetSceneTextureBufferSize(8).zw*EdgeWidth,rs=GetSceneTextureBufferSize(11).zw*EdgeWidth;
float2 taps[4]={float2(1,0),float2(-1,0),float2(0,1),float2(0,-1)};
float structure=0,detail=0;
[unroll]for(int k=0;k<4;++k)
{
    float z=SceneTextureLookup(ClampSceneTextureUV(duv+taps[k]*ds,1),1,false).r;
    bool neighbour=IsFinite(z)&&z>0&&z<1000000;
    structure=max(structure,neighbour?smoothstep(.002,.025,abs(z-SceneDepth)/max(SceneDepth,1)):1);
    if(neighbour&&valid)
    {
        float3 n=SceneTextureLookup(ClampSceneTextureUV(nuv+taps[k]*ns,8),8,false).rgb;
        float rough=SceneTextureLookup(ClampSceneTextureUV(ruv+taps[k]*rs,11),11,false).r;
        if(all(IsFinite(n))&&IsFinite(rough)) detail=max(detail,max(smoothstep(.16,.65,length(n-SceneNormal.rgb)),smoothstep(.04,.22,abs(rough-SceneRoughness))));
    }
}
return max(lerp(color,EdgeColor.rgb,saturate(structure*StructureStrength+detail*DetailStrength)*air),0);
