// Project-owned density extension. Embedded into a copy of Ninja M_VolumeSmoke.
// Ninja ray-box/depth integration and original lighting controls remain intact.
// p is the native raymarch position: local coordinates / 100 + 0.5.
struct FYogNinjaSpatialDensity
{
    // World-space medium modulation shared by the view and the light march.
    // No scene-normal/screen-UV projection or density is invented in empty cells.
    float Environment(FMaterialPixelParameters MP, float3 world, float density,
        float enabled, float groundHeight, float bodyHeight, float groundAmount,
        float upperAmount, float surfaceFade, float surfaceBand, float contactBoost,
        float4 observer, float playerStrength, float playerFeather, float playerHeight,
        float lightStrength, float lightFeather,
        float4 light0, float4 light1, float4 light2, float4 light3,
        float4 direction0, float4 direction1, float4 direction2, float4 direction3)
    {
        [branch] if (enabled < 0.5 || density < 0.001) return density;
        float ground = exp2(-max(world.z-groundHeight,0.0)/max(bodyHeight,10.0));
        float factor = lerp(max(upperAmount,0.0),max(groundAmount,0.0),ground);
        // GDF is a narrow-band field in translated-world centimetres. An empty
        // view binding returns zero, so never interpret an absent field as solid.
        #if PLATFORM_SUPPORTS_DISTANCE_FIELDS
        [branch] if (NumGlobalSDFClipmaps > 0)
        {
            float3 translated = WSAddDemote(WSPromote(world),MP.LWCData.PreViewTranslation);
            float distanceCm = GetDistanceToNearestSurfaceGlobal(translated);
            float fade = max(surfaceFade,1.0);
            float outside = smoothstep(-fade*0.25,fade,distanceCm);
            float contact = 1.0-smoothstep(fade,fade+max(surfaceBand,1.0),max(distanceCm,0.0));
            factor *= outside*(1.0+max(contactBoost,0.0)*contact);
        }
        #endif
        float playerClear = 0.0;
        [branch] if (observer.w > 0.0)
        {
            float3 relative = world-observer.xyz;
            playerClear = (1.0-smoothstep(observer.w,observer.w+max(playerFeather,1.0),length(relative.xy)))
                * (1.0-smoothstep(max(playerHeight,1.0)*0.65,max(playerHeight,1.0),abs(relative.z)));
        }
        factor *= 1.0-saturate(playerStrength)*playerClear;
        float4 lights[4] = {light0,light1,light2,light3};
        float4 directions[4] = {direction0,direction1,direction2,direction3};
        float clear = 0.0;
        [unroll] for (int index=0;index<4;index++)
        {
            float4 light = lights[index];
            [branch] if (light.w > 0.0)
            {
                float3 relative = world-light.xyz;
                float distanceCm = length(relative);
                float influence = 1.0-smoothstep(light.w,light.w+max(lightFeather,1.0),distanceCm);
                float4 direction = directions[index];
                [branch] if (direction.w > -0.99)
                    influence *= smoothstep(direction.w,min(direction.w+0.12,1.0),dot(relative/max(distanceCm,0.001),direction.xyz));
                clear = max(clear,influence);
            }
        }
        return density*factor*(1.0-saturate(lightStrength)*clear);
    }
    // x density, y palette position, z optional directional form-light factor.
    float3 Sample(FMaterialPixelParameters MP, float3 p, float3 origin,
        Texture3D volume, SamplerState volumeSampler,
        Texture2D brush, SamplerState brushSampler,
        float clock, float sizeCm, float coverage, float warp, float detail,
        float height, float heightFeather, float residual,
        float profileEnabled, float groundHeight, float groundThickness,
        float upperAmount, float volumeContrast, float nearDistance,
        float nearFeather, float centerStrength, float centerRadius,
        float centerFeather, float centerResidual, float billowStrength,
        float verticalScale, float albedoUniformity, float3 incidentWorld,
        float evaluateForm, float formStrength,
        float environmentEnabled, float envGroundHeight, float envBodyHeight,
        float envGroundAmount, float envUpperAmount, float envSurfaceFade,
        float envSurfaceBand, float envContactBoost, float4 envObserver,
        float envPlayerStrength, float envPlayerFeather, float envPlayerHeight,
        float envLightStrength, float envLightFeather,
        float4 envLight0, float4 envLight1, float4 envLight2, float4 envLight3,
        float4 envDirection0, float4 envDirection1, float4 envDirection2, float4 envDirection3)
    {
        float halfHeight = max(height + heightFeather, 0.001);
        float z = abs(p.z - 0.5);
        float edge = min(min(p.x, 1.0-p.x), min(p.y, 1.0-p.y));
        float envelope = smoothstep(0.0, 0.045, edge)
            * (1.0-smoothstep(halfHeight-max(heightFeather,0.025), halfHeight, z));
        if (envelope <= 0.0001) return float3(0.0, 0.0, 1.0);

        // Physical XYZ coordinates: non-uniform actor scale does not flatten cells.
        float3 world = origin + TransformLocalVectorToWorld(MP, (p-0.5)*100.0).xyz;
        float3 q = world / max(sizeCm, 50.0);
        // The opt-in profile can vary vertical cloud frequency independently.
        // 1 is unchanged; values below 1 fit more XYZ cloud structure in the
        // shallow ground layer. Legacy density ignores this control entirely.
        [branch] if (profileEnabled > 0.5 && verticalScale != 1.0)
            q.z /= clamp(verticalScale,0.35,2.0);
        float3 a = q * 6.2831853;
        // Smooth divergence-free analytic velocity pattern, including Z motion.
        // This is coordinate animation, not a Navier-Stokes simulation.
        float3 bend = float3(
            sin(a.y+clock) + cos(a.z-0.7*clock),
            sin(a.z+0.8*clock) + cos(a.x-clock),
            sin(a.x+0.6*clock) + cos(a.y-0.5*clock));
        float3 advected = q + saturate(warp)*0.22*bend;
        float n = Texture3DSampleLevel(volume, volumeSampler, advected, 0).r;
        // Native VolumeNoise2 is biased high (~0.75 mean, GPU slice probe).
        // Keep coverage in that useful range instead of filling the whole box.
        float threshold = lerp(0.88, 0.60, saturate(coverage));
        float cloud = smoothstep(threshold-0.055, threshold+0.055, n);

        // Triaxial brush density detail is evaluated INSIDE the volume, in the
        // same animated coordinates, never with carrier/scene/camera normals.
        float paint = 1.0;
        [branch] if (detail > 0.001 && cloud > 0.025 && cloud < 0.975)
        {
            float2 uvXY = advected.xy * 1.7;
            float2 uvYZ = advected.yz * 1.7 + float2(0.31,0.67);
            float2 uvZX = advected.zx * 1.7 + float2(0.73,0.19);
            float b = (Texture2DSampleLevel(brush,brushSampler,uvXY,0).r
                + Texture2DSampleLevel(brush,brushSampler,uvYZ,0).r
                + Texture2DSampleLevel(brush,brushSampler,uvZX,0).r) / 3.0;
            paint = lerp(1.0, saturate(0.30+1.5*b), saturate(detail));
        }
        // Compatibility: the original owned recipe retains its exact density
        // and palette field until the single-volume profile is explicitly on.
        [branch] if (profileEnabled <= 0.5)
        {
            float legacyDensity = envelope*(cloud*paint+max(residual,0.0)*0.35);
            legacyDensity = Environment(MP,world,legacyDensity,environmentEnabled,
                envGroundHeight,envBodyHeight,envGroundAmount,envUpperAmount,
                envSurfaceFade,envSurfaceBand,envContactBoost,envObserver,
                envPlayerStrength,envPlayerFeather,envPlayerHeight,envLightStrength,envLightFeather,
                envLight0,envLight1,envLight2,envLight3,envDirection0,envDirection1,envDirection2,envDirection3);
            return float3(legacyDensity,cloud,1.0);
        }

        // One world-space medium: ground body plus sparse upper clouds. The
        // optional lifted cloud cores below use at most ONE extra macro read.
        // Both the native view and light march evaluate this exact function.
        float thickness = max(groundThickness,10.0);
        float altitude = world.z-groundHeight;
        float aboveGround = smoothstep(-max(thickness*0.12,10.0),0.0,altitude);
        float positiveHeight = max(altitude,0.0);
        float groundEnvelope = exp2(-pow(positiveHeight/thickness,2.0))*aboveGround;
        float contrast = clamp(volumeContrast,0.25,4.0);
        float bodyCloud = pow(saturate(cloud),contrast);
        float upperCloud = pow(smoothstep(threshold+0.025,threshold+0.105,n),contrast);
        float upperEnvelope = (1.0-groundEnvelope)*aboveGround
            * exp2(-positiveHeight/max(thickness*3.0,30.0));
        float groundBody = groundEnvelope*(0.10+0.90*bodyCloud)*paint;
        float formFactor = 1.0;
        [branch] if (billowStrength > 0.001 && bodyCloud > 0.02)
        {
            // A separate, slowly moving XYZ domain avoids making the same
            // noise threshold set both cloud density and a striped top cap.
            // Reuse the volume/sampler; no atlas, extra ray or carrier is used.
            float slowClock = clock*0.20;
            float3 macroUV = q*0.43 + float3(0.173,0.619,0.347)
                + slowClock*float3(0.045,-0.031,0.024);
            float macroNoise = Texture3DSampleLevel(volume,volumeSampler,macroUV,0).r;
            float macroCore = smoothstep(0.68,0.86,macroNoise);
            float lift = thickness*clamp(billowStrength,0.0,1.5)*1.10*macroCore;
            float liftedEnvelope = exp2(-pow(max(positiveHeight-lift,0.0)/thickness,2.0))
                * aboveGround;
            float corePresence = smoothstep(0.02,0.12,bodyCloud);
            // Lift only actual cloud cores: the .10 uniform floor and residual
            // keep their original height, rather than becoming another sheet.
            groundBody += max(liftedEnvelope-groundEnvelope,0.0)
                * bodyCloud*macroCore*corePresence*paint;
            [branch] if (evaluateForm > 0.5 && formStrength > 0.001
                && dot(incidentWorld,incidentWorld) > 0.0001)
            {
                // One directional finite difference of the actual 3D macro
                // density, evaluated only by a lit VIEW sample. A world step
                // of CloudSize*.15 maps through the same .43 domain and Z scale.
                // This local form cue is neither a shadow map nor an NS solver;
                // it leaves density/T and the native self-shadow ray unchanged.
                float3 aheadQ = incidentWorld*0.15;
                aheadQ.z /= clamp(verticalScale,0.35,2.0);
                float aheadNoise = Texture3DSampleLevel(volume,volumeSampler,
                    macroUV+aheadQ*0.43,0).r;
                float formFace = saturate(0.55+(macroNoise-aheadNoise)*6.0);
                formFactor = lerp(1.0,lerp(0.48,1.25,formFace),saturate(formStrength));
            }
        }
        float upperBody = upperEnvelope*upperCloud*max(upperAmount,0.0)*paint;
        // BaseDensity remains an optional thin ground residual, rather than a
        // uniform tall wall that accumulates into flat opaque distant fog.
        float medium = groundBody+upperBody+groundEnvelope*max(residual,0.0)*0.35;

        // Query the SAME camera-relative medium at each candidate world point
        // in both marches. Using the current fragment's viewport UV here would
        // incorrectly give every light-ray sample the view ray's clear mask.
        float3 cameraRelative = world-WSDemote(GetWorldViewOrigin(MP));
        float nearKeep = smoothstep(max(nearDistance,0.0),
            max(nearDistance,0.0)+max(nearFeather,1.0),length(cameraRelative));
        float centerKeep = 1.0;
        [branch] if (centerStrength>0.001)
        {
            float3 viewPosition = mul(cameraRelative,(float3x3)ResolvedView.TranslatedWorldToView);
            float4 clip = mul(float4(viewPosition,1.0),ResolvedView.ViewToClip);
            if (clip.w>0.0001 && viewPosition.z>0.0)
            {
                // NDC radius is independent of viewport resolution and follows
                // the current FOV. The result is a soft camera-centred cone.
                float radial = length(clip.xy/clip.w);
                float clear = 1.0-smoothstep(max(centerRadius,0.0),
                    max(centerRadius,0.0)+max(centerFeather,0.001),radial);
                centerKeep = max(clamp(centerResidual,0.0,1.0),
                    1.0-saturate(centerStrength)*clear);
            }
        }
        // Palette only: reduce dense-cloud automatic brightening while leaving
        // the density, Beer-Lambert transmittance and native light inputs intact.
        float paletteT = lerp(bodyCloud,0.55,saturate(albedoUniformity));
        float finalDensity = Environment(MP,world,envelope*medium*nearKeep*centerKeep,environmentEnabled,
            envGroundHeight,envBodyHeight,envGroundAmount,envUpperAmount,
            envSurfaceFade,envSurfaceBand,envContactBoost,envObserver,
            envPlayerStrength,envPlayerFeather,envPlayerHeight,envLightStrength,envLightFeather,
            envLight0,envLight1,envLight2,envLight3,envDirection0,envDirection1,envDirection2,envDirection3);
        return float3(finalDensity,paletteT,formFactor);
    }
};
FYogNinjaSpatialDensity SpatialField;
