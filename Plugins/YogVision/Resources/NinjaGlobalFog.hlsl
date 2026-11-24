// Global world-space fog for the owned Ninja density pipeline.
// This post-process integrates a horizontally unbounded atmosphere whose
// density decays continuously above the ground, without a finite fog ceiling.
// It uses no mesh/proxy bounds or primitive transform; scene depth terminates
// each view ray. The accepted XYZ billow / signed-RG input / B density logic
// remains the shape authority. The Actor's local cache is only an accelerator.
struct FYogNinjaGlobalField
{
    float groundHeight, bodyHeight, groundAmount, upperAmount;
    float playerStrength, playerFeather, playerHeight, lightStrength, lightFeather;
    float environmentEnabled, heightEnabled, nearHeight, farHeight, heightFeather;
    float4 observer, lights[4], directions[4], maskMin, maskInvSize, maskLayout;
    float3 inputOrigin;
    float inputSize, inputRotation, fogHeight;
    float densityPower, noiseSize, noiseFlow, noiseSpeed;
    float atmosphereAmount, atmosphereVariation, boundaryDetail, boundaryMotion;

    float Smoother(float a, float b, float x)
    {
        float t=saturate((x-a)/max(b-a,0.001));
        return t*t*t*(t*(t*6.0-15.0)+10.0);
    }

    float Spot(float3 relative,float4 direction)
    {
        if(direction.w<=-0.99) return 1.0;
        return Smoother(direction.w,min(direction.w+0.12,1.0),
            dot(relative/max(length(relative),0.001),direction.xyz));
    }

    float2 Environment(float3 world,Texture3D cache,SamplerState cacheSampler)
    {
        if(environmentEnabled<0.5) return float2(1.0,0.0);
        float2 cached=0.0;
        float cacheWeight=0.0;
        if(maskLayout.w>=0.5 && all(maskInvSize.xyz>0.0))
        {
            float3 uvw=(world-maskMin.xyz)*maskInvSize.xyz;
            if(all(uvw>=0.0) && all(uvw<=1.0))
            {
                float resolution=max(floor(maskLayout.x+0.5),2.0);
                float slices=max(floor(maskLayout.y+0.5),2.0);
                float3 uv=float3((0.5+saturate(uvw.xy)*(resolution-1.0))/resolution,
                    (0.5+sqrt(saturate(uvw.z))*(slices-1.0))/slices);
                float4 data=Texture3DSampleLevel(cache,cacheSampler,uv,0);
                cached=float2(max(data.r,0.0),saturate(data.b-1.0));
                float xyEdge=min(min(uvw.x,1.0-uvw.x),min(uvw.y,1.0-uvw.y));
                float zEdge=min(uvw.z,1.0-uvw.z);
                cacheWeight=smoothstep(0.0,0.04,xyEdge)*smoothstep(0.0,0.02,zEdge);
                // Keep the common path as one volume lookup. Analytic player
                // and four-light evaluation is only needed at/outside edges.
                if(cacheWeight>=1.0) return cached;
            }
        }
        // Same analytic ground/player/light factors as the low-rate producer.
        // GDF-only contact and wall effects exist inside the cache and fade to
        // this baseline continuously; the edge is never a boundary of the fog.
        float ground=exp2(-max(world.z-groundHeight,0.0)/max(bodyHeight,10.0));
        float factor=lerp(max(upperAmount,0.0),max(groundAmount,0.0),ground);
        float playerClear=0.0, playerNear=0.0;
        if(observer.w>0.0)
        {
            float3 relative=world-observer.xyz;
            float planar=1.0-Smoother(observer.w,observer.w+max(playerFeather,1.0),length(relative.xy));
            playerClear=planar*(1.0-Smoother(max(playerHeight,1.0)*0.2,
                max(playerHeight,1.0),abs(relative.z)));
            if(playerStrength>0.0) playerNear=planar;
        }
        factor*=1.0-saturate(playerStrength)*playerClear;
        float lightClear=0.0,lightNear=0.0;
        [unroll] for(int i=0;i<4;i++)
        {
            float4 light=lights[i];
            if(light.w>0.0)
            {
                float3 relative=world-light.xyz;
                float influence=(1.0-Smoother(light.w,light.w+max(lightFeather,1.0),length(relative)))
                    *Spot(relative,directions[i]);
                lightClear=max(lightClear,influence);
                float3 lowRelative=float3(world.xy,groundHeight+max(nearHeight,1.0))-light.xyz;
                float planar=1.0-Smoother(light.w,light.w+max(lightFeather,1.0),length(lowRelative.xy));
                lightNear=max(lightNear,planar*Spot(lowRelative,directions[i]));
            }
        }
        factor*=1.0-saturate(lightStrength)*lightClear;
        if(heightEnabled>0.5)
        {
            float nearInfluence=saturate(max(saturate(playerStrength)*playerNear,
                saturate(lightStrength)*lightNear));
            float localHeight=lerp(max(farHeight,nearHeight),max(nearHeight,1.0),nearInfluence);
            float feather=lerp(max(heightFeather,1.0),
                min(max(heightFeather,1.0),max(nearHeight,1.0)*0.7+20.0),nearInfluence);
            factor*=1.0-Smoother(localHeight-feather,localHeight+feather,world.z-groundHeight);
        }
        float rim=4.0*playerClear*(1.0-playerClear)*saturate(playerStrength);
        float2 baseline=float2(max(factor,0.0),rim);
        return lerp(baseline,cached,cacheWeight);
    }

    // x density, y palette coordinate, zw signed local input velocity.
    float4 Sample(float3 world,float clock,
        Texture2D inputMap,SamplerState inputSampler,
        Texture3D noiseMap,SamplerState noiseSampler,
        Texture3D environmentCache,SamplerState environmentSampler)
    {
        float height=world.z-groundHeight;
        // fogHeight is the half-density height, not the top of a volume.
        // The same continuous profile is used by view and shadow samples.
        // Only the below-ground guard (or floating-point underflow) is zero;
        // negligible contributions are culled by optical depth per view cell.
        float vertical=Smoother(-25.0,0.0,height)
            *exp2(-max(height,0.0)/max(fogHeight,50.0));
        if(vertical<=0.0) return 0.0;
        float2 delta=world.xy-inputOrigin.xy;
        float angle=inputRotation*0.01745329252;
        float sn=sin(angle),cs=cos(angle);
        float2 uv=float2(delta.x*cs+delta.y*sn,-delta.x*sn+delta.y*cs)
            /max(inputSize,50.0)+0.5;
        float3 data=Texture2DSampleLevel(inputMap,inputSampler,frac(uv),0).rgb;
        float palette=pow(max(data.b,0.0),max(densityPower,0.001));
        float2 velocity=data.rg; // Ninja input is already signed; no second decode.
        float2 environment=Environment(world,environmentCache,environmentSampler);
        float rim=environment.y,motion=saturate(boundaryMotion),detail=0.0;
        if(atmosphereVariation>0.001 || boundaryDetail>0.001)
        {
            float3 q=world/max(noiseSize,50.0);
            float3 a=q*6.2831853;
            float t=clock*max(noiseSpeed,0.0);
            float3 stillBend=float3(sin(a.y)+cos(a.z),sin(a.z)+cos(a.x),sin(a.x)+cos(a.y));
            float3 rollingBend=float3(sin(a.y+t)+cos(a.z-t*0.7),
                sin(a.z+t*0.8)+cos(a.x-t),sin(a.x+t*0.6)+cos(a.y-t*0.5));
            float warp=clamp(noiseFlow,0.0,2.0)*0.16;
            q+=warp*lerp(stillBend,rollingBend,rim*motion);
            q.xy-=velocity*(warp*0.9*rim*motion);
            float n=Texture3DSampleLevel(noiseMap,noiseSampler,q,0).r;
            detail=saturate((n-0.64)/0.22);
            detail=detail*detail*(3.0-2.0*detail);
        }
        float body=detail*detail;
        float shape=lerp(1.0,0.28+1.8*body,saturate(atmosphereVariation));
        float animatedDetail=clamp((palette-0.4)*2.0,-0.65,0.65);
        shape*=max(1.0+saturate(boundaryDetail)*rim*motion*animatedDetail*0.4,0.5);
        float density=max(atmosphereAmount,0.0)*shape*environment.x*vertical;
        float colorPalette=lerp(saturate(0.45+detail*0.2),palette,
            saturate(boundaryDetail)*rim*motion*0.35);
        return float4(density,colorPalette,velocity*rim*motion);
    }
};

FYogNinjaGlobalField field;
field.groundHeight=EnvGroundHeight;
field.bodyHeight=EnvGroundBodyHeight;field.groundAmount=EnvGroundAmount;field.upperAmount=EnvUpperAmount;
field.playerStrength=EnvPlayerStrength;field.playerFeather=EnvPlayerFeather;field.playerHeight=EnvPlayerHeight;
field.lightStrength=EnvLightStrength;field.lightFeather=EnvLightFeather;
field.environmentEnabled=SpatialEnvironmentEnabled;
field.heightEnabled=EnvSpatialHeightEnabled;field.nearHeight=EnvNearFogHeight;
field.farHeight=EnvFarFogHeight;field.heightFeather=EnvHeightFeather;
field.observer=EnvObserver;
field.lights[0]=EnvLight0;field.lights[1]=EnvLight1;field.lights[2]=EnvLight2;field.lights[3]=EnvLight3;
field.directions[0]=EnvLightDirection0;field.directions[1]=EnvLightDirection1;
field.directions[2]=EnvLightDirection2;field.directions[3]=EnvLightDirection3;
field.maskMin=HybridMaskMin;field.maskInvSize=HybridMaskInvSize;field.maskLayout=HybridMaskLayout;
field.inputOrigin=GlobalInputOrigin.xyz;field.inputSize=GlobalInputWorldSize;field.inputRotation=GlobalInputRotation;
field.fogHeight=max(GlobalFogHeight,50.0);
field.densityPower=DensityPow;field.noiseSize=HybridNoiseSize;
field.noiseFlow=HybridNoiseFlow;field.noiseSpeed=HybridNoiseSpeed;
field.atmosphereAmount=HybridAtmosphereAmount;field.atmosphereVariation=HybridAtmosphereVariation;
field.boundaryDetail=HybridBoundaryDetail;field.boundaryMotion=HybridBoundaryMotion;

float3 scene=max(SceneColor.rgb,0.0);
if(GlobalFogEnabled<0.5 || Density<=0.0001 || HybridAtmosphereAmount<=0.0001) return scene;
float3 camera=CameraPosition.xyz;
float3 ray=-CameraVector.xyz;
float rayLength=length(ray);
if(!all(IsFinite(ray)) || rayLength<0.0001) return scene;
ray/=rayLength;
float start=max(GlobalStartDistance,0.0),end=max(GlobalMaxDistance,start+1.0);
if(IsFinite(SceneDepth) && SceneDepth>0.0 && SceneDepth<1000000.0 && all(IsFinite(WorldPosition.xyz)))
    end=min(end,max(dot(WorldPosition.xyz-camera,ray),0.0));
// Clip only the below-ground half-space. There is no upper or X/Y plane.
// Endpoint heights avoid division by an almost-horizontal ray direction.
if(end<=start+0.001) return scene;
float lower=EnvGroundHeight-25.0;
float startHeight=camera.z+ray.z*start-lower;
float endHeight=camera.z+ray.z*end-lower;
if(startHeight<0.0 && endHeight<0.0) return scene;
if(startHeight<0.0 || endHeight<0.0)
{
    float groundT=lerp(start,end,saturate(startHeight/(startHeight-endHeight)));
    if(startHeight<0.0) start=groundT;
    else end=groundT;
}
start=max(start,0.0);
if(end<=start+0.001) return scene;
int stepBudget=(int)clamp(floor(MaxViewSteps+0.5),8.0,128.0);
int shadowSteps=(int)clamp(floor(HybridShadowSamples+0.5),1.0,20.0);
float reference=max(DensityReferenceSize,50.0);
// Native Ninja uses 1.5 / MaxViewSteps in its reference-scale coordinates.
// Preserve that world-space density sampling target for short segments while
// retaining the quality control as a hard cap. Redistribute across the WHOLE
// visible segment so a long view never loses its far fog to a budget cutoff.
float targetStepCm=1.5*reference/float(stepBudget);
int steps=(int)clamp(ceil((end-start)/targetStepCm),8.0,float(stepBudget));
float pathLength=end-start;
float invSteps=1.0/float(steps);
// Concentrate bounded cells toward the lower end of the world-height ray.
// Horizontal views remain uniform. The signed bias is continuous as ray.z
// crosses zero; each cell still integrates its own real world-space length.
float heightBias=1.0-exp2(-abs(ray.z)*pathLength/max(field.fogHeight,50.0));
float signedHeightBias=(ray.z<0.0?1.0:-1.0)*heightBias;
float sigmaScale=max(Density,0.0)/reference;
float shadowStepCm=max(ShadowStepSize,0.0001)*reference*20.0/float(shadowSteps)*max(HybridShadowReach,0.05);
float clock=GlobalClock*HybridTimeScale+HybridTimeOffset;
int3 randomPosition=int3(Parameters.SvPosition.xy,View.StateFrameIndexMod8);
float jitter=float(Rand3DPCG16(randomPosition).x)/65535.0;
float transmittance=1.0;
float3 scatter=0.0,weightedPosition=0.0,weightedFlow=0.0;
float weightedLight=0.0,weight=0.0;
float3 directional=GlobalLightDirection.xyz;
directional=dot(directional,directional)>0.0001?normalize(directional):float3(0.0,0.0,1.0);
float angle=GlobalInputRotation*0.01745329252;
float sn=sin(angle),cs=cos(angle);
[loop] for(int i=0;i<steps;i++)
{
    float u0=float(i)*invSteps;
    float u1=(i+1==steps)?1.0:float(i+1)*invSteps;
    float f0=u0+signedHeightBias*u0*(1.0-u0);
    float f1=u1+signedHeightBias*u1*(1.0-u1);
    float cellLength=pathLength*(f1-f0);
    float distanceCm=start+pathLength*lerp(f0,f1,jitter);
    float3 world=camera+ray*distanceCm;
    float4 fieldSample=field.Sample(world,clock,GlobalInput,GlobalInputSampler,
        HybridNoise,HybridNoiseSampler,HybridEnvironmentVolume,HybridEnvironmentVolumeSampler);
    float nearWeight=smoothstep(max(GlobalStartDistance,0.0),
        max(GlobalStartDistance,0.0)+max(GlobalNearFade,1.0),distanceCm);
    float extinction=max(fieldSample.x,0.0)*sigmaScale*nearWeight;
    float opticalDepth=extinction*cellLength;
    // A skipped cell changes opacity by at most 1e-6, irrespective of world
    // height or ray length (at the default 32 cells: at most 3.2e-5 total).
    if(opticalDepth<=0.000001) continue;
    float previous=transmittance;
    transmittance*=exp(-opticalDepth);
    float absorption=previous-transmittance;
    float3 towardLight=directional;
    float attenuation=1.0,lightDistance=1000000.0;
    if(GlobalLightPosition.w>0.0)
    {
        float3 relative=GlobalLightPosition.xyz-world;
        lightDistance=max(length(relative),0.001);towardLight=relative/lightDistance;
        float fade=saturate(1.0-lightDistance*lightDistance/max(LightAttenuationRadius*LightAttenuationRadius,1.0));
        attenuation=fade*fade;
    }
    float lightVisibility=1.0;
    [loop] for(int l=0;l<shadowSteps;l++)
    {
        float lightT=(float(l)+0.5)*shadowStepCm;
        if(lightT>=lightDistance) break;
        float3 lightWorld=world+towardLight*lightT;
        float4 lightSample=field.Sample(lightWorld,clock,GlobalInput,GlobalInputSampler,
            HybridNoise,HybridNoiseSampler,HybridEnvironmentVolume,HybridEnvironmentVolumeSampler);
        lightVisibility*=exp(-max(lightSample.x,0.0)*sigmaScale*max(Shadow,0.0)*shadowStepCm);
        if(lightVisibility<0.001) break;
    }
    lightVisibility*=attenuation;
    float palette=saturate(pow(max(fieldSample.y,0.0),max(ColorPow,0.001))*ColorMul+ColorAdd);
    float3 paletteColor=lerp(Color1.rgb,Color2.rgb,palette);
    scatter+=absorption*paletteColor*max(PointLightColor.rgb,0.0)*max(LightIntensity,0.0)*lightVisibility;
    weightedPosition+=absorption*world;
    float2 velocity=fieldSample.zw;
    weightedFlow+=absorption*float3(velocity.x*cs-velocity.y*sn,
        velocity.x*sn+velocity.y*cs,0.0)*max(GlobalInputWorldSize,50.0);
    weightedLight+=absorption*saturate(lightVisibility);
    weight+=absorption;
    if(transmittance<0.001) break;
}
float safeWeight=max(weight,0.00001);
if(FogHatchDebugMode>0.5 && FogHatchDebugMode<1.5) return (1.0-transmittance).xxx*View.PreExposure;
if(FogHatchDebugMode>1.5 && FogHatchDebugMode<2.5)
    return float3(saturate(weightedFlow.xy/safeWeight/1000.0*0.5+0.5),1.0-transmittance)*View.PreExposure;
if(FogHatchDebugMode>2.5) return (weightedLight/safeWeight).xxx*View.PreExposure;
float4 fog=FogHatching.Apply(Parameters,scatter,transmittance,
    weightedPosition/safeWeight,weightedFlow/safeWeight,weightedLight/safeWeight,
    clock,FogHatchStrength,FogHatchSpacing,FogHatchFlowStrength,1.0,0.0,
    WorldUnitsPerPixel,AnchorRight.xyz,AnchorUp.xyz,FogHatchRegularity);
return scene*fog.a+max(fog.rgb,0.0)*View.PreExposure;
