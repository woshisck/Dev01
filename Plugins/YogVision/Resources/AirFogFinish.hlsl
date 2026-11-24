// Surface-only finish. All brush animation lives in the native fog medium.
// This view's original integrated scattering/transmittance defines its coverage.
float3 scene = max(SceneColor.rgb,0);
bool surface = IsFinite(SceneDepth) && SceneDepth>0 && SceneDepth<1000000;
float2 viewportUV = GetViewportUV(Parameters);
float4 integrated = Texture2DSampleLevel(AirFogTransmittance,AirFogTransmittanceSampler,viewportUV,0);
if (!all(IsFinite(integrated))) integrated=float4(0,0,0,1);
float transmittance = saturate(integrated.a);
float airMask = 1-transmittance;
if (AirFogDebug>.5) return airMask.xxx;
if (Density<=0) return scene;
// Native L is pre-exposed; PostProcessInput0's material wrapper removed exposure.
float3 inScattering = max(integrated.rgb,0)*View.OneOverPreExposure;
float3 fogColor = inScattering/max(airMask,.0001);
float3 color = scene;
float normalLength = dot(SceneNormal.rgb,SceneNormal.rgb);
bool validNormal = surface && all(IsFinite(SceneNormal.rgb)) && normalLength>.01;
if (validNormal)
{
    float3 n = SceneNormal.rgb*rsqrt(normalLength);
    float yaw = radians(FormLightYaw), pitch = radians(FormLightPitch);
    float3 key = float3(cos(pitch)*cos(yaw),cos(pitch)*sin(yaw),sin(pitch));
    float facing = dot(n,key);
    float lit = smoothstep(-.1,.8,facing);
    float shade = 1-smoothstep(-.8,.1,facing);
    float3 base = all(IsFinite(SceneBaseColor.rgb)) ? saturate(SceneBaseColor.rgb) : .18;
    // Broad normal-facing planes stay readable in unlit architecture.
    // Artistic form fill: neither a scene light nor cast shadows.
    float visibility = .28+.72*transmittance;
    color *= 1-FormShadowStrength*shade*visibility;
    color += FormLightColor.rgb*(.008+base*.04)*FormLightStrength*lit*visibility;
}
// Artistic contrast follows actual native coverage and its actual light/color.
// No second depth range, hand-made fog region or independently painted overlay.
color = lerp(color,fogColor,saturate(FarFlatten)*airMask);
if (!surface) return max(color,0); // Sky never reads invalid GBuffer neighbours.
float2 duv = GetDefaultSceneTextureUV(Parameters,1);
float2 nuv = GetDefaultSceneTextureUV(Parameters,8);
float2 ruv = GetDefaultSceneTextureUV(Parameters,11);
float2 ds = GetSceneTextureBufferSize(1).zw*EdgeWidth;
float2 ns = GetSceneTextureBufferSize(8).zw*EdgeWidth;
float2 rs = GetSceneTextureBufferSize(11).zw*EdgeWidth;
float2 taps[4] = {float2(1,0),float2(-1,0),float2(0,1),float2(0,-1)};
float structure = 0, detail = 0;
[unroll] for (int k=0;k<4;++k)
{
    float z = SceneTextureLookup(ClampSceneTextureUV(duv+taps[k]*ds,1),1,false).r;
    bool neighbour = IsFinite(z) && z>0 && z<1000000;
    structure=max(structure,neighbour ? smoothstep(.002,.025,abs(z-SceneDepth)/max(SceneDepth,1)) : 1);
    if (neighbour && validNormal)
    {
        float3 n = SceneTextureLookup(ClampSceneTextureUV(nuv+taps[k]*ns,8),8,false).rgb;
        float rough = SceneTextureLookup(ClampSceneTextureUV(ruv+taps[k]*rs,11),11,false).r;
        if (all(IsFinite(n)) && dot(n,n)>.01 && IsFinite(rough) && IsFinite(SceneRoughness))
            detail=max(detail,max(smoothstep(.16,.65,length(n-SceneNormal.rgb)),smoothstep(.04,.22,abs(rough-SceneRoughness))));
    }
}
float edge = saturate(structure*StructureStrength+detail*DetailStrength)*airMask;
return max(lerp(color,EdgeColor.rgb,edge),0);
