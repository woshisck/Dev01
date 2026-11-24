// World-space volume density: texture-shaped banks with slow coherent advection.
// Bounds only cull ray integration. No screen UV noise or fluid simulation.
struct SpatialFog
{
    float3 center, extent, ax, ay, az, observer;
    float density, variation, scale, feather, vision, radius, visionFeather, residual;
    float useShape, shapeStrength, shapeScale, detailStrength, detailScale, edgeSoftness, coreDensity, roundness;
    float3 stretch, velocity;
    float warpStrength, period, time, externalStrength;
    float hash(float3 p) { p=frac(p*.1031); p+=dot(p,p.yzx+33.33); return frac((p.x+p.y)*p.z); }
    float noise(float3 p)
    {
        float3 i=floor(p), f=frac(p); f=f*f*(3-2*f);
        return lerp(lerp(lerp(hash(i),hash(i+float3(1,0,0)),f.x),lerp(hash(i+float3(0,1,0)),hash(i+float3(1,1,0)),f.x),f.y),
                    lerp(lerp(hash(i+float3(0,0,1)),hash(i+float3(1,0,1)),f.x),lerp(hash(i+float3(0,1,1)),hash(i+float3(1,1,1)),f.x),f.y),f.z);
    }
    float3 local(float3 p) { float3 v=p-center; return float3(dot(v,ax),dot(v,ay),dot(v,az)); }
    float ellipsoid(float3 q,float3 c,float3 r) { return (length((q-c)/r)-1)*min(r.x,min(r.y,r.z)); }
    float unionSoft(float a,float b,float k) { float h=saturate(.5+.5*(b-a)/k);return lerp(b,a,h)-k*h*(1-h); }
    float field(float3 p,Texture3D shapeTex,SamplerState shapeSampler,Texture3D extTex,SamplerState extSampler,float footprint)
    {
        float3 lp=local(p);
        float3 advected=p-velocity*time;
        uint texX,texY,texZ;shapeTex.GetDimensions(texX,texY,texZ);
        float texSize=max(texX,max(texY,texZ));
        // Low-frequency body/warp stay fixed as camera and integration step lengths change.
        float4 shape=Texture3DSampleLevel(shapeTex,shapeSampler,advected*shapeScale,0);
        float phase=time*6.2831853/max(period,.1);
        float3 warp=float3(shape.b*2-1,shape.a*2-1,(shape.b-shape.a)) * warpStrength;
        // Oscillation bends detail locally; the body never follows the camera or jumps on a loop.
        warp*=float3(cos(phase),sin(phase),cos(phase*.5));
        float detailMip=clamp(log2(max(footprint*detailScale*texSize,1))-.5,0,log2(max(texSize,1)));
        float detail=Texture3DSampleLevel(shapeTex,shapeSampler,(advected+warp)*detailScale,detailMip).g;
        float n=noise(advected*scale)*.72+noise(advected*scale*.41+17.3)*.28;
        float3 q=abs(lp)-extent;
        float sd=length(max(q,0))+min(max(q.x,max(q.y,q.z)),0);
        // Only erode inward: density reaches zero before the finite integration box.
        float mask=1-smoothstep(-max(feather,1),0,sd+n*feather*.35);
        float3 bodyQ=lp/(extent*max(stretch,.05));
        float body=unionSoft(ellipsoid(bodyQ,float3(-.25,0,0),float3(.68,.82,.83)),ellipsoid(bodyQ,float3(.34,.22,-.03),float3(.59,.63,.78)),.28);
        body=unionSoft(body,ellipsoid(bodyQ,float3(0,-.30,-.08),float3(.76,.60,.72)),.25);
        body+=(shape.r-.5)*edgeSoftness*.55;
        float bodyMask=1-smoothstep(-max(edgeSoftness,.01),0,body);
        mask*=lerp(1,bodyMask,saturate(roundness)*saturate(useShape));
        float broad=lerp(saturate(coreDensity),1,smoothstep(.34,.66,shape.r));
        float textured=max(.02,broad*(1+(detail-.5)*2*detailStrength));
        float oldDensity=max(.1,1+(n-.5)*2*variation);
        float densityMod=lerp(oldDensity,textured,saturate(useShape)*saturate(shapeStrength));
        float clear=1-smoothstep(radius,radius+max(visionFeather,1),distance(p,observer));
        float d=max(density,0)*mask*lerp(1,lerp(1,residual,clear),vision)*densityMod;
        // The same field, including future interaction deltas, also attenuates lighting.
        if(abs(externalStrength)>.00001 && all(abs(lp)<extent))
            d+=Texture3DSampleLevel(extTex,extSampler,saturate(lp/(2*extent)+.5),0).a*externalStrength*mask*lerp(1,lerp(1,residual,clear),vision);
        return max(d,0);
    }
    float covered(float3 translated)
    {
        for(uint c=0;c<NumGlobalSDFClipmaps;c++)
        {
            float4 b=GlobalVolumeTranslatedCenterAndExtent[c];
            if(all(abs(translated-b.xyz)<b.www-2*b.w*GlobalVolumeTexelSize)) return 1;
        }
        return 0;
    }
    // 0 unavailable; 1 outside GDF coverage; 2 budget exhausted;
    // 3 confirmed blocked; 4 confirmed visible; 5 deliberately bypassed.
    float shadow(float3 p,float3 dir,float len,int steps,float bias,float useShadow,out float state)
    {
        state=0;
        if(useShadow<.5) { state=5; return 1; }
        if(NumGlobalSDFClipmaps==0) return 0; // unavailable is NOT an unoccluded light
        float t=max(bias*2,8);
        for(int j=0;j<steps;j++)
        {
            if(t>=len) { state=4; return 1; }
            float3 tp=DFFastToTranslatedWorld(p+dir*t,ResolvedView.PreViewTranslation);
            if(covered(tp)<.5) { state=1; return 0; } // deliberately conservative outside coverage
            float d=GetDistanceToNearestSurfaceGlobal(tp);
            if(d<bias) { state=3; return 0; }
            t+=max(d*.85,bias);
        }
        // Budget exhausted before reaching light: conservative, never fabricate visibility.
        state=t>=len ? 4 : 2;
        return state==4 ? 1 : 0;
    }
    float selfShadow(float3 p,float3 dir,float len,Texture3D shapeTex,SamplerState shapeSampler,Texture3D extTex,SamplerState extSampler)
    {
        float optical=0;
        float stepLen=min(len,1500)/4;
        for(int i=0;i<4;i++) optical+=field(p+dir*((i+.5)*stepLen),shapeTex,shapeSampler,extTex,extSampler,stepLen)*stepLen*.01;
        return exp(-optical);
    }
};
SpatialFog f;
f.center=VolumeCenter.xyz; f.extent=max(VolumeExtent.xyz,1);
f.ax=normalize(VolumeAxisX.xyz); f.ay=normalize(VolumeAxisY.xyz); f.az=normalize(VolumeAxisZ.xyz);
f.density=Density; f.variation=DensityVariation; f.scale=max(NoiseScale,.00001); f.feather=EdgeWidth;
f.vision=saturate(VisionEnabled); f.radius=ObserverClearRadius; f.visionFeather=ObserverFeather; f.residual=saturate(ClearDensityFraction);
f.useShape=UseDensityShape; f.shapeStrength=ShapeStrength; f.shapeScale=max(ShapeScale,.00001);
f.detailStrength=DetailStrength; f.detailScale=max(DetailScale,.00001); f.edgeSoftness=ShapeEdgeSoftness;
f.coreDensity=BodyCoreDensity; f.roundness=BodyRoundness; f.stretch=ShapeStretch.xyz;
f.velocity=FlowVelocity.xyz; f.warpStrength=FlowWarpStrength; f.period=FlowPeriod; f.time=FogTimeSeconds;
f.externalStrength=ExternalFieldStrength;
float3 camera=CameraPosition.xyz;
float3 rd=normalize(WorldPosition.xyz-camera);
float maxRay=min(length(WorldPosition.xyz-camera),MaxDistance);
if(SceneDepth<=0 || SceneDepth>1000000) maxRay=MaxDistance;
float3 origin=camera;
if(IsOrthoProjection(ResolvedView))
{
    rd=normalize(ResolvedView.ViewForward);
    maxRay=min(SceneDepth,MaxDistance);
    origin=WorldPosition.xyz-rd*SceneDepth;
}
// Editor focus uses the central camera aim. Gameplay gets the actual pawn position.
float3 forward=normalize(ResolvedView.ViewForward);
float focusT=abs(forward.z)>.01 ? (PreviewFocusHeight-camera.z)/forward.z : PreviewFocusMaxDistance;
focusT=clamp(focusT,100,PreviewFocusMaxDistance);
f.observer=GameplayObserver.w>.5 ? GameplayObserver.xyz : (ExplicitPreviewFocus.w>.5 ? ExplicitPreviewFocus.xyz : camera+forward*focusT);
float3 ro=f.local(origin), dl=float3(dot(rd,f.ax),dot(rd,f.ay),dot(rd,f.az));
float3 inv=1/(sign(dl)*max(abs(dl),.000001)+float3(dl.x==0,dl.y==0,dl.z==0)*.000001);
float3 a=(-f.extent-ro)*inv, b=(f.extent-ro)*inv;
float3 lo=min(a,b), hi=max(a,b);
float begin=max(0,max(lo.x,max(lo.y,lo.z))), end=min(maxRay,min(hi.x,min(hi.y,hi.z)));
float3 source=SceneColour.rgb;
if(end<=begin) return DebugMode>5.5 ? float3(0,0,0) : source;
int count=clamp((int)StepCount,8,128), shadowCount=clamp((int)ShadowSteps,2,48);
float ds=(end-begin)/count, T=1, optical=0;
float3 radiance=0;
float4 LP[4]={LightPosition0,LightPosition1,LightPosition2,LightPosition3};
float4 LD[4]={LightDirection0,LightDirection1,LightDirection2,LightDirection3};
float4 LC[4]={LightColour0,LightColour1,LightColour2,LightColour3};
float LI[4]={LightInnerCos0,LightInnerCos1,LightInnerCos2,LightInnerCos3};
float3 sun=normalize(DirectionalDirection.xyz+float3(.00001,0,0));
float shadowMean=0, sampleWeight=0;
float3 unknownShadowWeight=0;
float shadowQueryWeight=0, effectiveDepthSum=0, effectiveDepthWeight=0;
for(int i=0;i<count;i++)
{
    float t=begin+(i+.5)*ds;
    float3 p=origin+rd*t;
    float d=f.field(p,DensityShapeTexture,DensityShapeTextureSampler,FieldTexture,FieldTextureSampler,ds);
    if(d<.00001) continue;
    float3 light=AmbientRadiance.rgb*AmbientStrength;
    if(DirectionalStrength>0 && max(DirectionalColour.r,max(DirectionalColour.g,DirectionalColour.b))>0)
    {
        float sunState;
        float sunShadow=f.shadow(p,sun,3000,shadowCount,GeometryShadowBias,UseGeometryShadows,sunState);
        light+=DirectionalColour.rgb*DirectionalStrength*sunShadow*f.selfShadow(p,sun,1800,DensityShapeTexture,DensityShapeTextureSampler,FieldTexture,FieldTextureSampler)*(.65+.35*pow(saturate(dot(rd,sun)),2));
        if(sunState<4.5)
        {
            unknownShadowWeight+=d*float3(sunState==0 ? 1 : 0,sunState==1 ? 1 : 0,sunState==2 ? 1 : 0);
            shadowQueryWeight+=d;
        }
    }
    float localClear=0;
    for(int l=0;l<4;l++)
    {
        if(LC[l].w<.5 || LP[l].w<1) continue;
        float3 delta=LP[l].xyz-p; float dist=length(delta);
        if(dist>=LP[l].w) continue;
        float3 ldir=delta/max(dist,.001);
        float cone=LD[l].w < -1 ? 1 : smoothstep(LD[l].w,max(LD[l].w+.001,LI[l]),dot(-ldir,normalize(LD[l].xyz)));
        float atten=pow(saturate(1-dist/LP[l].w),2)*cone;
        if(atten<.0001) continue;
        float localState;
        float visibility=f.shadow(p,ldir,dist,shadowCount,GeometryShadowBias,UseGeometryShadows,localState);
        if(localState<4.5)
        {
            unknownShadowWeight+=d*float3(localState==0 ? 1 : 0,localState==1 ? 1 : 0,localState==2 ? 1 : 0);
            shadowQueryWeight+=d;
        }
        float fogShadow=f.selfShadow(p,ldir,dist,DensityShapeTexture,DensityShapeTextureSampler,FieldTexture,FieldTextureSampler);
        float phase=.7+.9*pow(saturate(dot(rd,ldir)),4);
        light+=LC[l].rgb*atten*visibility*fogShadow*phase;
        localClear=max(localClear,atten*visibility*LightClearStrength);
        shadowMean+=visibility*d;sampleWeight+=d;
    }
    d*=1-saturate(localClear);
    float alpha=1-exp(-d*ds*.01);
    float depthWeight=T*alpha;
    effectiveDepthSum+=depthWeight*t;
    effectiveDepthWeight+=depthWeight;
    float3 palette=lerp(NearColour.rgb,FarColour.rgb,saturate(PaletteMix+(1-PaletteMix)*t/max(PaletteDistance,1)));
    palette=lerp(float3(1,1,1),palette/max(dot(palette,float3(.2126,.7152,.0722)),.02),saturate(PaletteStrength));
    radiance+=T*alpha*light*FogAlbedo.rgb*palette;
    T*=1-alpha;optical+=d*ds*.01;
    if(T<.003) break;
}
// Debug 6: density-weighted query fractions; R unavailable, G outside coverage, B budget exhausted.
// Confirmed blocked/visible queries contribute to the denominator; bypassed queries do not.
// Debug 7: fog contribution centroid along this ray, normalized by MaxDistance.
if(DebugMode>6.5)
{
    float depth=effectiveDepthWeight>.000001 ? saturate(effectiveDepthSum/effectiveDepthWeight/max(MaxDistance,1)) : 0;
    return float3(depth,depth,depth);
}
if(DebugMode>5.5) return shadowQueryWeight>.000001 ? unknownShadowWeight/shadowQueryWeight : float3(0,0,0);
if(DebugMode>4.5) return NumGlobalSDFClipmaps==0 ? float3(1,0,1) : float3(0,saturate(NumGlobalSDFClipmaps/8.0),0);
if(DebugMode>3.5) return float3(sampleWeight>.0001?shadowMean/sampleWeight:0,0,0);
if(DebugMode>2.5) return radiance;
if(DebugMode>1.5) return float3(T,T,T);
if(DebugMode>.5) return float3(1-T,1-T,1-T);
return max(source*T+radiance,0);
