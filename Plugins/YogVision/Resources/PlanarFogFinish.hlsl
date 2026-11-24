// Architectural direction only. Fog brush mapping is composed from native
// scattering before artist post-processing; this finish never draws brush marks.
float3 inputScene=max(SceneColor.rgb,0);
if(!all(IsFinite(SceneNormal.xyz))||dot(SceneNormal.xyz,SceneNormal.xyz)<.01)return inputScene;
float2 screenUV=GetViewportUV(Parameters);
float4 fog=Texture2DSampleLevel(AirFogTransmittance,AirFogTransmittanceSampler,screenUV,0);
if(!all(IsFinite(fog)))return inputScene;
// AfterDOF still carries pre-exposed HDR. Work in physical linear colour for
// stable shadow thresholds and pigment, then restore exposure exactly once.
float3 scene=inputScene*View.OneOverPreExposure;
float3 n=normalize(SceneNormal.xyz);
float3 key=normalize(FallbackLightDirection.xyz+float3(.0001,0,0));
float plane=smoothstep(-.3,.9,dot(n,key));
scene+=FormStrength*(.004+max(SceneBaseColor.rgb,0)*.018)*plane*saturate(fog.a);
return max(scene,0)*View.PreExposure;
