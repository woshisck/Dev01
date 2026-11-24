// Native medium only. Brush mapping consumes the rendered scattering later;
// brush settings never change extinction, albedo, or emission here.
struct FogShape
{
    float Height(float z,float d,float center,float halfSize,float feather)
    {
        float h=max(halfSize,1)*lerp(.78,1.12,saturate(d));
        return 1-smoothstep(max(h-max(feather,1),0),h,abs(z-center));
    }
    float Clear(float3 p,float4 observer,float radius,float feather,float residual)
    {
        if(observer.w<.5)return 1;
        float3 d=p-observer.xyz;d.z*=.4;
        float c=1-smoothstep(radius,radius+max(feather,1),length(d));
        return lerp(1,saturate(residual),c);
    }
};
FogShape shape;
float3 w=WorldPosition.xyz;
float3 delta=w-VolumeCenter.xyz;
float3 box=float3(dot(delta,VolumeAxisX.xyz),dot(delta,VolumeAxisY.xyz),dot(delta,VolumeAxisZ.xyz))/max(VolumeExtent.xyz,1);
float bounds=1-smoothstep(.78,1,max(abs(box.x),max(abs(box.y),abs(box.z))));
float2 uv=(w.xy-SimulationCenter.xy)/max(SimulationExtent.xy*2,1)+.5;
float coverage=(1-smoothstep(.8,1,max(abs(uv.x*2-1),abs(uv.y*2-1))))*saturate(SimulationReady);
float4 field=Texture2DSampleLevel(PlanarField,PlanarFieldSampler,saturate(uv),0);
float authored=Texture2DSampleLevel(DensityPattern,DensityPatternSampler,frac(w.xy/max(PatternWorldSize,100)),0).r;
float d=pow(max(lerp(authored,field.b,coverage),.001),max(DensityPower,.1));
float2 velocity=field.rg*coverage;
// Two phases prevent UV reset popping; simulation is slow and stays pinned in world space.
float phase=frac(FlowTime*.045);
float blend=abs(phase*2-1);
// Field RG is signed world XY cm/s. Preserve its direction and bound displacement in cm.
float2 flow=velocity/max(25,length(velocity))*max(FlowDisplacement,0);
float3 n0=(w-float3(flow*(phase-.5),0))/max(NoiseWorldSize,10);
float3 n1=(w-float3(flow*(frac(phase+.5)-.5),0))/max(NoiseWorldSize,10);
float3 noise=lerp(Texture3DSampleLevel(VolumeNoise,VolumeNoiseSampler,n0,0).rgb,Texture3DSampleLevel(VolumeNoise,VolumeNoiseSampler,n1,0).rgb,blend);
float volumeDetail=lerp(1,saturate((noise.r-.15)*1.65),saturate(NoiseStrength));
float height=shape.Height(w.z,d,HeightCenter,HeightHalfSize,HeightSoftness);
float clearing=shape.Clear(w,ObserverPosition,ClearRadius,ClearFeather,ClearResidual);

// Reconstructed density probes. XY adds four 2D samples, Z reuses the central
// planar density. Six 3D samples below include the visible erosion in the normal.
float epsilon=clamp(NoiseWorldSize*.25,60,140);
float q=d*height*clearing;
float2 qXY=0;
[unroll]for(int axis=0;axis<2;axis++)
{
    float2 offset=axis==0?float2(epsilon,0):float2(0,epsilon);
    float3 p=w+float3(offset,0);
    float2 puv=(p.xy-SimulationCenter.xy)/max(SimulationExtent.xy*2,1)+.5;
    float pc=(1-smoothstep(.8,1,max(abs(puv.x*2-1),abs(puv.y*2-1))))*saturate(SimulationReady);
    float pd=Texture2DSampleLevel(PlanarField,PlanarFieldSampler,saturate(puv),0).b;
    float pa=Texture2DSampleLevel(DensityPattern,DensityPatternSampler,frac(p.xy/max(PatternWorldSize,100)),0).r;
    pd=pow(max(lerp(pa,pd,pc),.001),max(DensityPower,.1));
    qXY[axis]=pd*shape.Height(p.z,pd,HeightCenter,HeightHalfSize,HeightSoftness)
        *shape.Clear(p,ObserverPosition,ClearRadius,ClearFeather,ClearResidual);
}
float3 pz=w+float3(0,0,epsilon);
float qZ=d*shape.Height(pz.z,d,HeightCenter,HeightHalfSize,HeightSoftness)
    *shape.Clear(pz,ObserverPosition,ClearRadius,ClearFeather,ClearResidual);
float3 cloudNeighbors;
[unroll]for(int axis3=0;axis3<3;axis3++)
{
    float3 offset3=0;offset3[axis3]=epsilon/max(NoiseWorldSize,10);
    float nr=lerp(Texture3DSampleLevel(VolumeNoise,VolumeNoiseSampler,n0+offset3,0).r,
                  Texture3DSampleLevel(VolumeNoise,VolumeNoiseSampler,n1+offset3,0).r,blend);
    float detail=lerp(1,saturate((nr-.15)*1.65),saturate(NoiseStrength));
    cloudNeighbors[axis3]=(axis3==0?qXY.x:(axis3==1?qXY.y:qZ))*detail;
}
float3 difference=cloudNeighbors-q*volumeDetail;
float gradientLength=length(difference);
float3 normal=-difference/max(gradientLength,.00001);
float gradientGate=smoothstep(.008,.055,gradientLength);
float3 lightDir=normalize(FallbackLightDirection.xyz+float3(.0001,0,0));
float facing=dot(normal,lightDir);
float densityGate=smoothstep(.10,.22,q)*(1-smoothstep(.58,.78,q));
float baseVeil=max(DensityFloor,0)*height*clearing;
float cloud=d*volumeDetail*height*clearing;
float rho=max(Density,0)*bounds*(baseVeil+cloud);
float farMix=saturate(PaletteMix+(1-PaletteMix)*distance(w,CameraPosition.xyz)/max(PaletteDistance,1));
float3 tint=max(lerp(NearColour.rgb,FarColour.rgb,farMix),0);
// Dense cores keep a smooth air mass instead of revealing every noise normal.
float shapeLight=lerp(1,.62+.38*saturate(facing),gradientGate*densityGate);
FogAlbedoOut=saturate(FogAlbedo.rgb);
float3 emission=tint*max(AmbientStrength,0)*rho*100*shapeLight;
return float4(emission,rho*100);
