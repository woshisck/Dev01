// Screen-space ink and camera-ray fog. World positions (cm) authorize sight and locate spirit volumes.
// CPU trace fans own gameplay visibility; this shader never authorizes additional sight.
float3 scene = max(SceneColor.rgb, 0);
bool hasSurface = SceneDepth < 1000000;
float3 wp = hasSurface ? WorldPosition : Observer.xyz;
float2 viewportUV = GetViewportUV(Parameters);
float aspect = View.ViewSizeAndInvSize.x * View.ViewSizeAndInvSize.w;
// Equal-sized strokes on floors, walls and sky; no world-XY / Z-axis projection.
float2 canvas = (viewportUV - .5) * float2(aspect, 1);
float4 sources[4] = { Source0, Source1, Source2, Source3 };
float strengths[4] = { SourceStrength0, SourceStrength1, SourceStrength2, SourceStrength3 };
float reveal = 0;
float boundary = 0;
float t = TimeSeconds * FlowSpeed;
float2 flowUV = canvas * max(StrokeScale, .25);
float2 flow = float2(sin(flowUV.y * .7 + t), cos(flowUV.x * .6 - t * .73));
float grain = Texture2DSample(StrokeTexture, StrokeTextureSampler, flowUV + flow * .08 + float2(t * .017, -t * .012)).r;
float grain2 = Texture2DSample(StrokeTexture, StrokeTextureSampler, flowUV.yx * .61 - flow * .07 + float2(-t * .011, t * .016)).r;
float turbulence = sin(wp.x * .009 + t * 1.8) * cos(wp.y * .007 - t * 1.3);
[unroll] for (int s = 0; s < 4; ++s)
{
    if (s >= SourceCount || sources[s].w < 1) continue;
    if (!hasSurface) break;
    float2 d = wp.xy - sources[s].xy;
    float angle = frac(atan2(d.y, d.x) / 6.28318530718 + 1);
    // Neighbour sectors are conservatively combined to avoid interpolating light through corners.
    float sector = angle * 64;
    float u0 = (floor(sector) + .5) / 64;
    float u1 = (fmod(floor(sector) + 1, 64) + .5) / 64;
    float cut0 = Texture2DSampleLevel(VisibilityTexture, VisibilityTextureSampler, float2(u0, (s + .5) / 4), 0).r;
    float cut1 = Texture2DSampleLevel(VisibilityTexture, VisibilityTextureSampler, float2(u1, (s + .5) / 4), 0).r;
    float cut = min(cut0, cut1);
    float radius = sources[s].w * cut;
    float dist = length(d);
    float width = max(BoundaryWidth, 1);
    // Noise contracts the visible region, never expands beyond CPU-authorized visibility.
    float retreat = width * BoundaryNoise * saturate(.5 + .25 * turbulence + .25 * (grain - grain2));
    float r = saturate((radius - dist - retreat) / width);
    r = r * r * (3 - 2 * r);
    float height = 1 - smoothstep(260, 600, abs(wp.z - sources[s].z));
    r *= height * saturate(strengths[s]);
    reveal = max(reveal, r);
    boundary = max(boundary, 4 * r * (1 - r));
}
float luma = dot(scene, float3(.2126, .7152, .0722));
float darkness = 1 - smoothstep(max(DarkThreshold * .2, .001), max(DarkThreshold, .002), luma);
float distanceAmount = smoothstep(DistanceFogStart, max(DistanceFogEnd, DistanceFogStart + 1), SceneDepth);
float distantFog = DistanceFogDensity * (1 - exp(-distanceAmount * 3.5)) * (1 - reveal * .95);
float fogMask = saturate(max((1 - reveal) * FogDensity + darkness * DarkFogStrength * reveal, distantFog));
// Far geometry loses contrast into a continuous cool mist, including empty background depth.
float contrast = FogContrast * lerp(1, .18, distanceAmount * DistanceFogDensity);
float3 lowContrast = FogColor.rgb * (FogBrightness + .42 + distanceAmount * .18)
    + (scene - luma) * contrast * .3 + luma * contrast;
float3 color = lerp(scene, lowContrast, fogMask);

float2 depthUV = GetDefaultSceneTextureUV(Parameters, 1);
float2 normalUV = GetDefaultSceneTextureUV(Parameters, 8);
float2 depthStep = GetSceneTextureBufferSize(1).zw;
float2 normalStep = GetSceneTextureBufferSize(8).zw;
float edge = 0;
float2 taps[4] = { float2(1,0), float2(-1,0), float2(0,1), float2(0,-1) };
[unroll] for (int k = 0; k < 4; ++k)
{
    float z = SceneTextureLookup(ClampSceneTextureUV(depthUV + taps[k] * depthStep * 1.5, 1), 1, false).r;
    float3 n = SceneTextureLookup(ClampSceneTextureUV(normalUV + taps[k] * normalStep * 1.5, 8), 8, false).rgb;
    float depthEdge = saturate(abs(z - SceneDepth) / max(SceneDepth * .015, 3));
    float normalEdge = saturate(length(n - SceneNormal.rgb) * 1.8);
    if (hasSurface) edge = max(edge, max(depthEdge, normalEdge));
}
color -= FogColor.rgb * edge * EdgeStrength * fogMask * .45 * (1 - distanceAmount * .7);
float strokes = (grain - .5) + (grain2 - .5) * .45;
color += strokes * StrokeStrength * fogMask * float3(.7, .82, .8) * (1 - distanceAmount * .5);
// A shallow, moving density rim suggests mist receding around firelight.
color += FogColor.rgb * boundary * BoundaryNoise * (.12 + .1 * turbulence);
float playerDist = length(wp.xy - Observer.xy);
float playerMask = (1 - smoothstep(65, 150, playerDist)) * reveal * hasSurface;
color += PlayerLift * playerMask * (1 - saturate(luma)) * float3(.7, .66, .57);

float4 echoes[8] = { Echo0, Echo1, Echo2, Echo3, Echo4, Echo5, Echo6, Echo7 };
float4 velocities[8] = { EchoVelocity0, EchoVelocity1, EchoVelocity2, EchoVelocity3, EchoVelocity4, EchoVelocity5, EchoVelocity6, EchoVelocity7 };
float3 ray = normalize(WorldPosition - CameraPosition + float3(0, 0, .001));
[unroll] for (int e = 0; e < 8; ++e)
{
    if (e >= EchoCount || echoes[e].w <= 0) continue;
    float2 vel = velocities[e].xy;
    float speed = saturate(velocities[e].w * 4);
    float radius = max(velocities[e].z, 25);
    float2 direction = length(vel) > 1 ? normalize(vel) : float2(1, 0);
    float3 along = float3(direction, 0);
    float3 across = float3(-direction.y, direction.x, 0);
    float trail = SpiritTrailLength * speed;
    float height = max(SpiritVolumeHeight, 10);
    float3 center = echoes[e].xyz + float3(0, 0, height * .35) - along * trail * .28;
    float3 axes = float3(radius * 1.3 + trail * .38, radius * 1.3, height * .85);
    float3 origin = CameraPosition - center;
    float3 o = float3(dot(origin, along), dot(origin, across), origin.z) / axes;
    float3 r = float3(dot(ray, along), dot(ray, across), ray.z) / axes;
    float nearest = max(-dot(o, r) / max(dot(r, r), .00000001), 0);
    float3 q = o + r * nearest;
    float d2 = dot(q, q);
    if (d2 >= 3.3333) continue;
    float churn = sin(q.x * 4.1 + TimeSeconds * (1 + speed) + e)
        * cos(q.z * 3.7 - TimeSeconds * .8) * sin(q.y * 3.3 + TimeSeconds * .55);
    float body = exp(-max(d2 + churn * .32 * speed, 0) * 1.8) * saturate(1 - d2 * .3);
    float drift = Texture2DSampleLevel(StrokeTexture, StrokeTextureSampler,
        q.yz * .45 + float2(TimeSeconds * -.025, TimeSeconds * .018) + e * .13, 0).r;
    float density = body * lerp(.65, 1.1, drift) * echoes[e].w * SpiritDisturbance * (1 - reveal);
    float alpha = saturate(density * (.38 + speed * .62));
    // Density gradients suggest a soft 3D body; movement displaces the mist and leaves a trailing volume.
    float3 densityNormal = normalize(q + float3(.001, .001, .001));
    float volumeLight = .55 + .45 * saturate(dot(densityNormal, normalize(float3(-.4, -.6, .8))));
    float2 displacement = float2(q.y, -q.z) * body * speed * SpiritDisturbance * .006;
    float2 warpedUV = ViewportUVToSceneTextureUV(saturate(viewportUV + displacement), 14);
    float3 warped = SceneTextureLookup(ClampSceneTextureUV(warpedUV, 14), 14, false).rgb;
    color += (warped - scene) * alpha * .2 * (1 - fogMask * .7);
    float3 mistBody = FogColor.rgb * (.5 + FogBrightness) + SpiritColor.rgb * volumeLight * (.24 + .32 * speed);
    color = lerp(color, mistBody + color * .42, alpha);
}
// PresentationFade comes from the owned local view and active fade volumes.
// Truly dark surfaces still receive fog and character readability assistance.
return lerp(scene, max(color, 0), saturate(PresentationFade));
