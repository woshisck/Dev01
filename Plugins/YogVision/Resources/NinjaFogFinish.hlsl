// Final structural drawing over the already composited scene and Ninja fog.
// UE 5.8 PostProcessing.cpp runs BeforeBloom after AfterDOF translucency,
// including its deferred composition inside TSR. SceneTextures still bind the
// original opaque SceneDepth/GBuffer, independent of the composited fog RGB.
// This pass reads no roughness or object UV. The peripheral hatch atlas is a
// project-authored visual reference, not Dota 2's shader or its implementation.
float3 color=max(SceneColor.rgb,0.0);
bool validDepth=IsFinite(SceneDepth) && SceneDepth>0.0 && SceneDepth<1000000.0;
if(!validDepth) return color;

float2 depthUV=GetDefaultSceneTextureUV(Parameters,1);
float2 normalUV=GetDefaultSceneTextureUV(Parameters,8);
// BeforeBloom is post-TSR. Input0 may be output resolution while the original
// GBuffer remains input resolution: convert one OUTPUT pixel to each buffer.
float2 outputSize=max(GetSceneTextureViewSize(14).xy,float2(1.0,1.0));
float2 depthScale=GetSceneTextureViewSize(1).xy/outputSize;
float2 normalScale=GetSceneTextureViewSize(8).xy/outputSize;
float width=clamp(EdgeWidth,0.5,4.0);
float2 depthStep=GetSceneTextureBufferSize(1).zw*depthScale*width;
float2 normalStep=GetSceneTextureBufferSize(8).zw*normalScale*width;
float2 taps[4]={float2(1,0),float2(-1,0),float2(0,1),float2(0,-1)};
float normalLength=dot(SceneNormal.xyz,SceneNormal.xyz);
bool validNormal=all(IsFinite(SceneNormal.xyz)) && normalLength>0.01;
float3 centerNormal=validNormal ? SceneNormal.xyz*rsqrt(normalLength) : float3(0,0,1);
float outline=0.0;
[unroll] for(int i=0;i<4;++i)
{
    float depth=SceneTextureLookup(ClampSceneTextureUV(depthUV+taps[i]*depthStep,1),1,false).r;
    bool otherValid=IsFinite(depth) && depth>0.0 && depth<1000000.0;
    // Relative depth suppresses small bumps and texture-scale surface detail.
    float depthEdge=otherValid
        ? smoothstep(0.0045,0.025,abs(depth-SceneDepth)/max(SceneDepth,1.0)) : 1.0;
    outline=max(outline,depthEdge);
    if(validNormal && otherValid && NormalStrength>0.001)
    {
        float3 other=SceneTextureLookup(ClampSceneTextureUV(normalUV+taps[i]*normalStep,8),8,false).rgb;
        float otherLength=dot(other,other);
        if(all(IsFinite(other)) && otherLength>0.01)
        {
            float turn=1.0-saturate(dot(centerNormal,other*rsqrt(otherLength)));
            // About 50..76 degrees: emphasize large plane turns, not the
            // normal-map grain that the previous detail-outline emphasized.
            outline=max(outline,smoothstep(0.35,0.75,turn)*clamp(NormalStrength,0.0,1.0));
        }
    }
}

if(DebugMode>0.5 && DebugMode<1.5) return outline.xxx*View.PreExposure;

// A second drawing weight within THIS existing finish pass. Two overlapping
// layers of real pencil marks use the accepted, fixed reference-camera world
// plane. Viewport position and luminance reveal those fixed marks; they never
// translate/rotate their UVs. There is no time, per-pixel noise or extra pass.
float outerInk=0.0;
if(all(IsFinite(WorldPosition.xyz)) && OuterBrushStrength>0.001)
{
    float rightLength=dot(AnchorRight.xyz,AnchorRight.xyz);
    float3 anchorRight=rightLength>0.0001
        ? AnchorRight.xyz*rsqrt(rightLength) : float3(1.0,0.0,0.0);
    float3 upCandidate=AnchorUp.xyz-anchorRight*dot(AnchorUp.xyz,anchorRight);
    float3 fallbackUp=abs(anchorRight.z)<0.99 ? float3(0.0,0.0,1.0) : float3(0.0,1.0,0.0);
    fallbackUp-=anchorRight*dot(fallbackUp,anchorRight);
    float upLength=dot(upCandidate,upCandidate);
    float3 anchorUp=upLength>0.0001 ? upCandidate*rsqrt(upLength) : normalize(fallbackUp);
    float2 anchorPixel=float2(dot(WorldPosition.xyz,anchorRight),dot(WorldPosition.xyz,anchorUp))
        /max(WorldUnitsPerPixel,0.05);

    struct FYogOuterBrush
    {
        float Hash(float2 cell)
        {
            float3 p=frac(float3(cell.x,cell.y,cell.x)*float3(0.1031,0.1030,0.0973));
            p+=dot(p,p.yzx+33.33);
            return frac((p.x+p.y)*p.z);
        }
        float2 Rotate(float2 p,float2 axis)
        {
            return float2(dot(p,axis),dot(p,float2(-axis.y,axis.x)));
        }
        float Layer(Texture2D atlas,SamplerState atlasSampler,float2 planePixel,
            float2 planeDx,float2 planeDy,float pitch,float2 offset,float layerSeed,
            float angleDegrees,float density,float strokeWidth,float reveal)
        {
            float2 grid=planePixel/pitch+offset;
            float2 baseCell=floor(grid-0.5);
            // Radius <= .90 cells and jitter <= .08 guarantee that omitted
            // cells are outside support: 2x2 neighbors suffice even rotated.
            // Density expands actual stamp coverage, not just its opacity.
            float radius=lerp(0.70,0.90,density);
            float diameter=radius*2.0;
            float widthControl=saturate((strokeWidth-0.5)/7.5);
            float cutoff=lerp(0.16,0.015,widthControl)+(1.0-reveal)*0.20;
            float paper=1.0;
            [unroll] for(int y=0;y<2;++y)
            {
                [unroll] for(int x=0;x<2;++x)
                {
                    float2 cell=baseCell+float2(x,y);
                    float2 key=cell+float2(layerSeed*13.7,layerSeed*31.9);
                    float2 jitter=(float2(Hash(key+float2(3.1,7.9)),
                        Hash(key+float2(19.3,2.7)))-0.5)*0.16;
                    float2 delta=grid-(cell+0.5+jitter);
                    float support=1.0-smoothstep(radius*0.88,radius,length(delta));
                    // A zero-support stamp contributes exactly paper *= 1.
                    // Keep every positive feather value; no epsilon or radius
                    // approximation may remove shallow graphite coverage.
                    [branch] if(support!=0.0)
                    {
                        float seed=Hash(key);
                        float angle=radians(angleDegrees+(seed-0.5)*32.0);
                        float2 axis=float2(cos(angle),sin(angle));
                        float2 uv=Rotate(delta,axis)/diameter+0.5;
                        // Explicit gradients come from continuous planeDx/Dy
                        // computed before either new divergent guard.
                        float2 gradX=Rotate(planeDx/pitch,axis)/diameter;
                        float2 gradY=Rotate(planeDy/pitch,axis)/diameter;
                        float mirror=Hash(key+float2(41.2,9.1))>0.5 ? -1.0 : 1.0;
                        uv.x=(uv.x-0.5)*mirror+0.5;
                        gradX.x*=mirror;gradY.x*=mirror;
                        float tileIndex=floor(Hash(key+float2(11.4,53.2))*4.0);
                        float2 tile=float2(fmod(tileIndex,2.0),floor(tileIndex*0.5));
                        // Each quadrant has white source padding. Clamping stays
                        // inside that padding, avoiding neighboring atlas stamps.
                        float2 atlasUV=(tile+clamp(uv,0.02,0.98))*0.5;
                        float gray=Texture2DSampleGrad(atlas,atlasSampler,atlasUV,
                            gradX*0.5,gradY*0.5).r;
                        // White is paper. Reject only weak background paper gray;
                        // preserve real graphite pressure and derivative mip AA.
                        float ink=saturate((1.0-gray-0.035)/0.965);
                        ink=pow(saturate((ink-cutoff)/max(1.0-cutoff,0.001)),0.72);
                        float stamp=ink*support*lerp(0.78,1.0,Hash(key+float2(2.4,37.6)));
                        // Dark ink union retains the original accumulation order.
                        paper*=1.0-saturate(stamp);
                    }
                }
            }
            return 1.0-paper;
        }
    };
    FYogOuterBrush brush;
    float2 planeDx=ddx(anchorPixel);
    float2 planeDy=ddy(anchorPixel);
    // Default spacing 11 gives ~116px stamp diameter in the reference plane;
    // the actual hand-drawn line length/spacing comes from the selected atlas.
    float pitch=clamp(OuterBrushSpacing,8.0,96.0)*6.5;
    float strokeWidth=clamp(OuterBrushWidth,0.5,8.0);
    float density=saturate(OuterBrushDensity);
    float3 physicalColor=max(color*View.OneOverPreExposure,0.0);
    float physicalLuma=dot(physicalColor,float3(0.2126,0.7152,0.0722));
    float darkThreshold=max(OuterBrushDarkThreshold,0.001);
    float dark=1.0-smoothstep(darkThreshold*0.25,darkThreshold,max(physicalLuma,0.0));
    // A saturated cyan lamp can have low luminance despite a bright channel.
    // Protect it independently of the artist's shadow threshold.
    float maxChannel=max(physicalColor.r,max(physicalColor.g,physicalColor.b));
    float highlightProtection=1.0-smoothstep(0.10,0.24,maxChannel);
    float radial=length((GetViewportUV(Parameters)-0.5)*2.0);
    float peripheral=smoothstep(clamp(OuterBrushStart,0.05,0.95),1.15,radial);
    float tone=saturate(dark*(0.65+0.35*peripheral)+outline*0.10);
    float secondReveal=saturate(density*lerp(0.20,1.0,tone));
    // These literal zero factors cannot reveal a mark. Evaluate derivatives
    // above this guard, so explicit SampleGrad filtering/AA remains unchanged.
    [branch] if(dark!=0.0 && peripheral!=0.0 && highlightProtection!=0.0)
    {
        // At most 2 layers x 2x2 neighbors. Positive coverage uses the original
        // atlas, rotation, cutoff, gradients and dark-ink union without changes.
        float primaryWeight=smoothstep(0.0,0.30,density);
        float secondaryWeight=smoothstep(0.08,0.82,secondReveal)*0.80;
        float primary=0.0;
        float secondary=0.0;
        [branch] if(primaryWeight!=0.0)
            primary=brush.Layer(OuterHatchAtlas,OuterHatchAtlasSampler,anchorPixel,
                planeDx,planeDy,pitch,float2(0.0,0.0),0.73,-12.0,density,strokeWidth,
                lerp(0.68,1.0,density));
        [branch] if(secondaryWeight!=0.0)
            secondary=brush.Layer(OuterHatchAtlas,OuterHatchAtlasSampler,anchorPixel,
                planeDx,planeDy,pitch*0.86,float2(13.73,-7.19),2.41,42.0,density,
                strokeWidth,secondReveal);
        float drawn=1.0-(1.0-primary*primaryWeight)*(1.0-secondary*secondaryWeight);
        outerInk=saturate(drawn*dark*peripheral*highlightProtection
            *clamp(OuterBrushStrength,0.0,2.0)*0.85);
    }
}
if(DebugMode>1.5) return outerInk.xxx*View.PreExposure;

// A restrained artist palette veil: this supplements the existing volume
// rather than replacing its optical density, shadows, motion or transmittance.
float farFactor=smoothstep(max(FarStart,0.0),max(FarStart,0.0)+max(FarFeather,1.0),SceneDepth);
float veil=saturate(FarFogBlend)*farFactor;
float3 palette=max(FarFogColor.rgb,0.0)*View.PreExposure;
color=lerp(color,palette,veil);
// The accepted palette and structural mask are untouched. Coarse drawing only
// darkens selected peripheral shadows; it adds no white specks or new medium.
color*=1.0-outerInk;
// Draw AFTER the palette blend and actual Ninja composition, so environmental
// structure remains legible through fog without restoring hidden scene color.
return color*(1.0-saturate(outline*clamp(OutlineStrength,0.0,1.0)));
