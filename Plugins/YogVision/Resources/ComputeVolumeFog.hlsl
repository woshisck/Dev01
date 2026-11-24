// The evolving local density is a persistent GPU Texture3D, never a projected atlas.
// A stationary three-dimensional background covers the environment outside that grid.
float3 delta=WorldPosition.xyz-VolumeCenter.xyz;
float3 p=float3(dot(delta,VolumeAxisX.xyz),dot(delta,VolumeAxisY.xyz),dot(delta,VolumeAxisZ.xyz));
float3 q=p/max(VolumeExtent.xyz,1);
float edge=1-smoothstep(.8,1,max(abs(q.x),max(abs(q.y),abs(q.z))));
float3 backgroundUV=q*float3(1.8,1.6,1.3)/max(ShapeScaleXYZ.xyz,.1)+float3(.31,.47,.19);
float3 background=Texture3DSampleLevel(BackgroundShape,BackgroundShapeSampler,backgroundUV,0).rgb;
float backgroundDensity=pow(saturate((background.r-.24)*2),1.35)*lerp(.65,1.2,background.g);
float3 sq=(WorldPosition.xyz-SimulationCenter.xyz)/max(SimulationExtent.xyz,1);
float3 uv=sq*.5+.5;
float coverage=(1-smoothstep(.8,1,max(abs(sq.x),max(abs(sq.y),abs(sq.z)))))*saturate(SimulationReady);
float field=Texture3DSampleLevel(DensityField,DensityFieldSampler,saturate(uv),0).r;
float shape=lerp(backgroundDensity,field,coverage);
float clear=ObserverPosition.w>.5 ? 1-smoothstep(ClearRadius,ClearRadius+max(ClearFeather,1),distance(WorldPosition.xyz,ObserverPosition.xyz)) : 0;
// Density never changes with camera distance. Native ray integration supplies distance fog.
float rho=max(Density,0)*edge*(.035+shape)*lerp(1,saturate(ClearResidual),clear);
float3 texel=1/max(GridResolution.xyz,1);
float dx=Texture3DSampleLevel(DensityField,DensityFieldSampler,saturate(uv+float3(texel.x,0,0)),0).r-field;
float dy=Texture3DSampleLevel(DensityField,DensityFieldSampler,saturate(uv+float3(0,texel.y,0)),0).r-field;
float dz=Texture3DSampleLevel(DensityField,DensityFieldSampler,saturate(uv+float3(0,0,texel.z)),0).r-field;
float3 normal=normalize(-float3(dx,dy,dz)/max(2*SimulationExtent.xyz*texel,.01)+float3(0,0,.00001));
float shade=lerp(.65,.3+.7*saturate(dot(normal,normalize(FallbackLightDirection.xyz+float3(.0001,0,0)))),coverage);
float farMix=saturate(PaletteMix+(1-PaletteMix)*distance(WorldPosition.xyz,CameraPosition.xyz)/max(PaletteDistance,1));
float3 colour=lerp(NearColour.rgb,FarColour.rgb,farMix);
colour=colour/max(dot(colour,float3(.2126,.7152,.0722)),.03);
// Material extinction/emission is per metre; native voxelization converts to per cm.
return float4(max(colour,0)*rho*100*max(FallbackLightStrength,0)*shade,rho*100);
