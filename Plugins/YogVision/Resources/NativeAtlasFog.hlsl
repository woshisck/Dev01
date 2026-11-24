// Evaluated once per native fog voxel, not a per-screen-pixel raymarch.
// Baked density/velocity frames drive shape. UE integrates real scene lighting.
struct AtlasVolume
{
    float2 tile(float2 uv,float frame)
    {
        frame=fmod(frame,25);
        return (float2(fmod(frame,5),floor(frame/5))+clamp(uv,.003,.997))/5;
    }
    float flowDensity(Texture2D d,SamplerState ds,Texture2D v,SamplerState vs,float2 uv,float clock,float strength)
    {
        float frame=fmod(clock,25), f=frac(frame), a=floor(frame), b=fmod(a+1,25);
        float2 v0=Texture2DSampleLevel(v,vs,tile(uv,a),0).rg*2-1;
        float2 v1=Texture2DSampleLevel(v,vs,tile(uv,b),0).rg*2-1;
        float d0=Texture2DSampleLevel(d,ds,tile(uv-v0*f*strength,a),0).r;
        float d1=Texture2DSampleLevel(d,ds,tile(uv+v1*(1-f)*strength,b),0).r;
        return lerp(d0,d1,f);
    }
};
AtlasVolume atlas;
float3 delta=WorldPosition.xyz-VolumeCenter.xyz;
float3 p=float3(dot(delta,VolumeAxisX.xyz),dot(delta,VolumeAxisY.xyz),dot(delta,VolumeAxisZ.xyz));
float3 q=p/max(VolumeExtent.xyz,1);
float edge=1-smoothstep(.72,1,max(abs(q.x),max(abs(q.y),abs(q.z))));
float clock=FogTimeSeconds*max(AtlasFPS,0);
// World-anchored transport, independent of screen UVs. Retain the baked vortex
// animation; slow translation keeps a volume's silhouette moving as well.
float2 transport=FogTimeSeconds*float2(.006,-.004)*saturate(FlowStrength/.05);
float2 uv=frac((p.xy+float2(p.z*.23,p.z*-.17))*max(ShapeScale,.00001)+transport);
float d=atlas.flowDensity(DensityAtlas,DensityAtlasSampler,VelocityAtlas,VelocityAtlasSampler,uv,clock,FlowStrength);
float2 uvB=frac(float2(-p.y,p.x)*max(ShapeScale,.00001)*.73+float2(.37,.61)+p.z*.00013);
float db=atlas.flowDensity(DensityAtlas,DensityAtlasSampler,VelocityAtlas,VelocityAtlasSampler,uvB,clock+11.5,FlowStrength);
// Do not max two full fog layers or lift them with a sub-unit power: that fills
// both layers' gaps and averages away the animated silhouette along the ray.
float cloud=pow(saturate(d),1.8)*lerp(.75,1.25,db);
// Advected three-dimensional shape breaks the tall XY extrusion into actual
// spatial banks. This is baked-field transport, not an NS fluid simulation.
float3 volumeUV=WorldPosition.xyz*.0015-FogTimeSeconds*float3(.009,-.006,.002)*saturate(FlowStrength/.05);
float detail=Texture3DSampleLevel(DetailTexture,DetailTextureSampler,volumeUV,0).r;
cloud*=lerp(1,smoothstep(.22,.7,detail)*1.6,saturate(DetailStrength));
float vertical=1-smoothstep(.28+cloud*.3,.95,abs(q.z));
float clear=ObserverPosition.w>.5 ? 1-smoothstep(ClearRadius,ClearRadius+max(ClearFeather,1),distance(WorldPosition.xyz,ObserverPosition.xyz)) : 0;
float distanceToCamera=length(WorldPosition.xyz-CameraPosition.xyz);
float depthDensity=lerp(.4,1,smoothstep(300,2600,distanceToCamera));
float rho=max(Density,0)*edge*vertical*(.025+cloud*1.75)*depthDensity*lerp(1,saturate(ClearResidual),clear);
float farMix=saturate(PaletteMix+(1-PaletteMix)*distanceToCamera/max(PaletteDistance,1));
float3 colour=lerp(NearColour.rgb,FarColour.rgb,farMix);
colour=colour/max(dot(colour,float3(.2126,.7152,.0722)),.03);
// A cheap art-directed volume fill when the dungeon has no directional light.
// Existing point/spot/directional lights and their shadows stay in UE's fog lighting path.
float3 pseudoNormal=normalize(float3((d-db)*3, (detail-.5)*2,.6));
float shade=.4+.6*saturate(dot(pseudoNormal,normalize(FallbackLightDirection.xyz+float3(.0001,0,0))));
// Native voxelization multiplies material extinction/emission by 1/100 (per metre -> per cm).
// Actor exposes per-cm density, so explicitly convert exactly once here.
return float4(max(colour,0)*rho*100*max(FallbackLightStrength,0)*shade,rho*100);
