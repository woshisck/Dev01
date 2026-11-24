// Native world-space froxel medium. Camera view-Z owns every distance mask.
// xyz = artistic emission; w = optical density per metre.
float3 viewForward = normalize(ResolvedView.ViewForward);
float viewDepth = max(dot(WorldPosition-CameraPosition,viewForward),0);
float3 p = WorldPosition*NoiseScale;
float t = TimeSeconds*WindSpeed*NoiseScale;
float broad = sin(p.x*.71+t)*sin(p.y*.83-t*.61)*cos(p.z*.59+t*.31);
float fine = sin(p.x*1.63+p.z*.47-t*.53)*cos(p.y*1.37-p.z*.71+t*.43);
float noise = broad*.68+fine*.32;
// Metre-scale rolling dust instead of tiny froxel speckles.
float3 q = WorldPosition*BoundaryDetailScale+float3(t*.19,-t*.13,t*.09);
float sx = sin(q.x+.45*sin(q.y*.61));
float sy = sin(q.y*.79+.38*sin(q.z*.73));
float cz = cos(q.z*.67+.31*sin(q.x*.57));
float cloud = sx*sy*cz;
// Project each froxel onto the current camera, with filtered brush density.
// This modifies the MEDIUM before light integration, not only final scene color.
float3 cameraDelta=WorldPosition-CameraPosition;
float2 projected=float2(dot(cameraDelta,ResolvedView.ViewRight),-dot(cameraDelta,ResolvedView.ViewUp))
    / (IsOrthoProjection(ResolvedView) ? 1 : max(viewDepth,10));
float aspect=View.ViewSizeAndInvSize.x*View.ViewSizeAndInvSize.w;
float2 canvas=projected*float2(ResolvedView.ViewToClip[0][0],ResolvedView.ViewToClip[1][1])*.5*float2(aspect,1);
float footprint=GridPixelSize/max(View.ViewSizeAndInvSize.y,1);
FYogVolumeBrushFlow paintFlow;
float paint=paintFlow.Sample(StrokeTexture,StrokeTextureSampler,canvas,TimeSeconds*StrokeFlowSpeed,StrokeScale,StrokeWarp,footprint);
// Analytic density gradient. Surface GBuffer normals never shade the air.
float dx = cos(q.x+.45*sin(q.y*.61))*sy*cz
    - sx*sy*sin(q.z*.67+.31*sin(q.x*.57))*.31*.57*cos(q.x*.57);
float dy = cos(q.x+.45*sin(q.y*.61))*.45*.61*cos(q.y*.61)*sy*cz
    + sx*.79*cos(q.y*.79+.38*sin(q.z*.73))*cz;
float dz = sx*cos(q.y*.79+.38*sin(q.z*.73))*.38*.73*cos(q.z*.73)*cz
    - sx*sy*.67*sin(q.z*.67+.31*sin(q.x*.57));
float edgeOffset = BoundaryNoise*(noise*.5+cloud*BoundaryDetail+(paint-.5)*StrokeStrength*2);
float ramp = max(DepthRamp,1);
float x = saturate((viewDepth-NearOffset-edgeOffset)/ramp);
float smoothDepth = x*x*(3-2*x);
float depthWeight = pow(smoothDepth,max(DepthPower,.1));
float clearWeight = smoothstep(ClearRadius+edgeOffset*.2,
    ClearRadius+max(ClearFeather,1)+edgeOffset*.2,viewDepth);
float heightBase = CameraPosition.z-CameraHeightOffset;
float heightWeight = exp2(-max(WorldPosition.z-heightBase,0)/max(HeightFalloff,10));
float nearDensity = clamp(NearDensity,0,max(Density,0));
float clearDensity = clamp(ClearMinDensity,0,nearDensity);
float depthDensity = lerp(nearDensity,max(Density,0),depthWeight);
float rim = 4*smoothDepth*(1-smoothDepth);
float medium = lerp(clearDensity,depthDensity,clearWeight)*heightWeight
    * max(1+noise*NoiseAmount+rim*cloud*BoundaryDetail*.7+(paint-.5)*StrokeStrength*(.4+rim*2),.05);
// Single MI palette shared with Finish. Preserve SceneDepth/3500 gradient.
// lerp_fog's existing blend budget becomes the medium's radiance multiplier.
float3 palette = lerp(ArtistNearColor.rgb,ArtistFarColor.rgb,
    saturate(viewDepth/max(PaletteDistance,1)))*saturate(ArtistFogMix)*PaletteStrength;
float3 airColor = lerp(AmbientColor.rgb,palette,saturate(UseArtistPalette));
float yaw = radians(FormLightYaw), pitch = radians(FormLightPitch);
float3 key = float3(cos(pitch)*cos(yaw),cos(pitch)*sin(yaw),sin(pitch));
float3 gradient = float3(dx,dy,dz)*BoundaryDetailScale*max(BoundaryDetail,.001)
    - viewForward*(6*x*(1-x)/ramp);
float3 cloudNormal = gradient*rsqrt(max(dot(gradient,gradient),.000001));
float facing = dot(cloudNormal,key);
float bright = smoothstep(-.15,.8,facing);
float shade = 1-smoothstep(-.8,.15,facing);
float3 toCamera = normalize(CameraPosition-WorldPosition+float3(0,0,.001));
float phase = pow(saturate(dot(toCamera,-key)*.5+.5),4);
float shape = VolumeFormStrength*(rim+.18)*(.55+.45*BoundaryDetail);
float3 directionTint = 1-shape*FormShadowStrength*shade
    + shape*FormLightStrength*(bright+phase*.35)*FormLightColor.rgb;
// Actual scene lights still scatter through native fog with shadows.
// This additional art direction creates no shadow map.
float3 emission = medium*airColor*AmbientIntensity*max(directionTint,.05);
return float4(max(emission,0),max(medium,0));
