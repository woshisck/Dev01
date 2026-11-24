// Scene drawing beneath Ninja's AfterDOF volumetric surface composition.
// This pass runs BeforeDOF: the real volume naturally attenuates its ink.
// Peripheral/far/shadow weights are art masks, not a fog-transmittance buffer.
// This is a screen-composited pass, but marks use one fixed world-anchored 2D
// basis. Camera motion never translates their phase across the scene.
// Diagnostic backing must also cover sky/invalid GBuffer pixels.
if (DebugMode>3.5) return float3(0.0,0.0,0.0);
float3 scene = max(SceneColor.rgb, 0.0);
bool surface = IsFinite(SceneDepth) && SceneDepth > 0.0 && SceneDepth < 1000000.0;
float normalLength = dot(SceneNormal.xyz, SceneNormal.xyz);
if (!surface || !all(IsFinite(SceneNormal.xyz)) || normalLength < 0.01) return scene;
if (!all(IsFinite(WorldPosition.xyz))) return scene;

float3 normal = SceneNormal.xyz * rsqrt(normalLength);
float physicalLuma = dot(scene * View.OneOverPreExposure, float3(0.2126,0.7152,0.0722));
float shadow = 1.0 - smoothstep(0.018, 0.12, physicalLuma);
float2 screenUV = GetViewportUV(Parameters);
float radial = length((screenUV-0.5)*2.0);
float peripheral = smoothstep(clamp(PeripheralStart,0.05,0.95), 1.15, radial);
float distant = smoothstep(max(FarStart,0.0), max(FarStart,0.0)+max(FarFeather,1.0), SceneDepth);
float region = saturate(max(ScreenBaseWeight,max(peripheral*0.9,distant*0.65)))
    * lerp(0.24,1.0,shadow);

float2 depthUV = GetDefaultSceneTextureUV(Parameters,1);
float2 normalUV = GetDefaultSceneTextureUV(Parameters,8);
float2 roughUV = GetDefaultSceneTextureUV(Parameters,11);
float2 depthStep = GetSceneTextureBufferSize(1).zw * clamp(EdgeWidth,0.5,3.0);
float2 normalStep = GetSceneTextureBufferSize(8).zw * clamp(EdgeWidth,0.5,3.0);
float2 roughStep = GetSceneTextureBufferSize(11).zw * clamp(EdgeWidth,0.5,3.0);
float2 taps[4] = {float2(1,0),float2(-1,0),float2(0,1),float2(0,-1)};
float structure = 0.0;
float detail = 0.0;
[unroll] for (int i=0;i<4;++i)
{
    float z = SceneTextureLookup(ClampSceneTextureUV(depthUV+taps[i]*depthStep,1),1,false).r;
    bool valid = IsFinite(z) && z>0.0 && z<1000000.0;
    structure = max(structure,valid ? smoothstep(0.0015,0.014,abs(z-SceneDepth)/max(SceneDepth,1.0)) : 1.0);
    if (valid)
    {
        float3 otherNormal = SceneTextureLookup(ClampSceneTextureUV(normalUV+taps[i]*normalStep,8),8,false).rgb;
        float rough = SceneTextureLookup(ClampSceneTextureUV(roughUV+taps[i]*roughStep,11),11,false).r;
        if (all(IsFinite(otherNormal)))
        {
            float difference = length(otherNormal-normal);
            structure = max(structure,smoothstep(0.35,0.9,difference)*0.45);
            detail = max(detail,smoothstep(0.18,0.55,difference)*0.35);
        }
        if (IsFinite(rough)) detail = max(detail,smoothstep(0.05,0.22,abs(rough-SceneRoughness)));
    }
}

// A fixed orthonormal reference-camera plane shared by the entire scene.
// These coordinates are independent of current view, surface UV, normal and
// time. WorldUnitsPerPixel calibrates the reference drawing's scale in cm.
float rightLength = dot(AnchorRight.xyz,AnchorRight.xyz);
float3 anchorRight = rightLength>0.0001 ? AnchorRight.xyz*rsqrt(rightLength) : float3(1.0,0.0,0.0);
float3 upCandidate = AnchorUp.xyz-anchorRight*dot(AnchorUp.xyz,anchorRight);
float3 fallbackUp = abs(anchorRight.z)<0.99 ? float3(0.0,0.0,1.0) : float3(0.0,1.0,0.0);
fallbackUp -= anchorRight*dot(fallbackUp,anchorRight);
float upLength = dot(upCandidate,upCandidate);
float3 anchorUp = upLength>0.0001 ? upCandidate*rsqrt(upLength) : normalize(fallbackUp);
float2 anchorPixel = float2(dot(WorldPosition.xyz,anchorRight),dot(WorldPosition.xyz,anchorUp))
    / max(WorldUnitsPerPixel,0.05);

struct FYogSceneHatching
{
    float PatchSeed(float2 cell)
    {
        float3 p = frac(float3(cell.x,cell.y,cell.x)*float3(0.1031,0.1030,0.0973));
        p += dot(p,p.yzx+33.33);
        return frac((p.x+p.y)*p.z);
    }
    float Stroke(float phase)
    {
        float aa = max(fwidth(phase),0.015);
        float strokeCoverage = 1.0-smoothstep(0.055,0.055+min(aa,0.45),abs(frac(phase)-0.5));
        // Fade unresolved lines instead of aliasing into moving moire.
        return strokeCoverage*saturate(1.0-aa*1.5);
    }
};
FYogSceneHatching hatching;
// Coarse stable patches give hand-laid directional changes. No texture noise
// modulates scene luminance or breaks strokes into fine grains.
float patchSize = clamp(ScreenPatchSize,32.0,1024.0);
float2 patchCell = floor(anchorPixel/patchSize);
float2 patchCenter = (patchCell+0.5)*patchSize;
float seed = hatching.PatchSeed(patchCell);
float angle = radians(ScreenHatchAngle+(seed-0.5)*32.0);
float2 pencilDirection = float2(cos(angle),sin(angle));
float2 crossDirection = float2(-pencilDirection.y,pencilDirection.x);
float2 patchPixel = anchorPixel-patchCenter;

// HatchWorldSize keeps its compatibility name: reference-plane pixel spacing.
// Long quiet gaps and a sparse second shadow band replace the old fine brush
// breakup. fwidth in Stroke adapts the edges to the actual current projection.
float spacing = max(HatchWorldSize,2.0);
float along = dot(patchPixel,pencilDirection);
float across = dot(patchPixel,crossDirection);
float pencilA = hatching.Stroke(across/spacing+seed*13.0);
float pencilB = hatching.Stroke(along/(spacing*1.24)+seed*7.0);
float segmentA = frac(along/max(spacing*9.0,48.0)+seed);
float segmentB = frac(across/max(spacing*11.0,64.0)+seed*0.73);
float gapA = smoothstep(0.08,0.18,segmentA)*(1.0-smoothstep(0.65,0.80,segmentA));
float gapB = smoothstep(0.10,0.22,segmentB)*(1.0-smoothstep(0.62,0.78,segmentB));
float2 patchInterior = min(frac(anchorPixel/patchSize),1.0-frac(anchorPixel/patchSize))*patchSize;
float patchFade = smoothstep(0.5,4.0,min(patchInterior.x,patchInterior.y));
float brokenPencil = saturate(pencilA*gapA*smoothstep(0.20,0.62,shadow)
    +pencilB*gapB*smoothstep(0.72,0.98,shadow)*0.35)*patchFade;
float ink = saturate((structure*max(OutlineStrength,0.0)+detail*max(DetailStrength,0.0))*region);

if (DebugMode>0.5 && DebugMode<1.5) return region.xxx*View.PreExposure;
if (DebugMode>1.5 && DebugMode<2.5) return ink.xxx*View.PreExposure;
if (DebugMode>2.5) return float3(0.0,brokenPencil,region)*View.PreExposure;

// BrushTexture/BrushStrength/BrushWorldSize are deliberately ignored. Old
// presets cannot reintroduce full-frame black/white grain, even at high values.
float3 color = scene * (1.0-saturate(brokenPencil*clamp(HatchStrength,0.0,2.0)*region*0.55));
float3 inkFloor = min(color*0.2,max(InkColor.rgb,0.0)*View.PreExposure);
return max(lerp(color,inkFloor,ink),0.0);
