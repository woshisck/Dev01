// Project-owned hybrid density extension for the licensed Ninja raymarch.
// The live Ninja intermediate RT stores signed XY velocity in RG and density
// in B. M_FlowPlay_VolumeSmoke already decodes velocity from its source atlas.
// Environment coefficients are PRECOMPUTED; no GDF query occurs in this file.
struct FYogNinjaHybridDensity
{
    // x density coefficient; y animated clear-zone rim. B retains GDF-ready
    // information as 0 unavailable / 1 ready / 1..2 ready plus rim weight.
    float2 Mask(FMaterialPixelParameters MP, float3 world,
        Texture3D environmentVolume, SamplerState environmentVolumeSampler,
        float4 maskMin, float4 maskInvSize, float4 maskLayout)
    {
        // Layout remains (square XY resolution, Z slices, atlas columns,
        // ready) for existing diagnostics. The volume has identical texels.
        [branch] if (maskLayout.w < 0.5 || min(maskInvSize.x,
            min(maskInvSize.y,maskInvSize.z)) <= 0.0) return float2(1.0,0.0);
        float3 uvw = (world-maskMin.xyz)*maskInvSize.xyz;
        [branch] if (any(uvw < 0.0) || any(uvw > 1.0)) return float2(1.0,0.0);
        float resolution = max(floor(maskLayout.x+0.5),2.0);
        float slices = max(floor(maskLayout.y+0.5),2.0);
        // Producer places slice k at minZ+sizeZ*(k/(N-1))^2.
        float slice = sqrt(saturate(uvw.z))*(slices-1.0);
        // Map all three axes between first/last texel centres. Hardware
        // linear filtering performs the same two XY bilinear + Z lerp
        // interpolation, with implementation-dependent filter-weight rounding.
        float2 within = 0.5+saturate(uvw.xy)*(resolution-1.0);
        float3 volumeUV = float3(within/resolution,(0.5+slice)/slices);
        float4 data = Texture3DSampleLevel(environmentVolume,environmentVolumeSampler,volumeUV,0);
        // Contact/ground coefficients can exceed 1. Never saturate this mask.
        return float2(max(data.r,0.0),saturate(data.b-1.0));
    }

    // x actual density; y native palette position; zw signed input RT velocity.
    float4 Sample(FMaterialPixelParameters MP, float3 p, float3 origin,
        Texture2D velocityDensity, SamplerState velocityDensitySampler,
        Texture3D volumeNoise, SamplerState volumeNoiseSampler,
        Texture2D environmentMask, SamplerState environmentMaskSampler,
        Texture3D environmentVolume, SamplerState environmentVolumeSampler,
        float clock, float densityPower, float height, float heightFeather,
        float densityOffset, float baseDensity, float fadeFalloff, float zBias,
        float noiseAmount, float noiseSize, float noiseFlow, float noiseSpeed,
        float atmosphereAmount, float atmosphereVariation,
        float boundaryDetail, float boundaryMotion,
        float4 maskMin, float4 maskInvSize, float4 maskLayout)
    {
        // The native proxy has oversized raster bounds. View and light rays
        // must share the same analytic unit box even when Extrude is raised.
        [branch] if (any(p < 0.0) || any(p > 1.0)) return 0.0;
        // Exactly one live input RT read. Its B includes Ninja flipbook/flow
        // interpolation and owned input-material brush modulation already.
        float3 data = Texture2DSampleLevel(velocityDensity,velocityDensitySampler,p.xy,0).rgb;
        float palette = pow(max(data.b,0.0),max(densityPower,0.001));
        float2 velocity = data.rg; // Signed already: do NOT apply *2-1 again.
        float3 world = origin+TransformLocalVectorToWorld(MP,(p-0.5)*100.0).xyz;
        float2 environment = Mask(MP,world,environmentVolume,environmentVolumeSampler,
            maskMin,maskInvSize,maskLayout);
        [branch] if (atmosphereAmount > 0.0001)
        {
            // Continuous atmospheric support and rounded XYZ billows share
            // one density function for view and light. Keep a residual medium
            // between billows, instead of cutting the world into empty clouds.
            float rim = environment.y;
            float motion = saturate(boundaryMotion);
            float detail = 0.0;
            [branch] if (atmosphereVariation > 0.001 || boundaryDetail > 0.001)
            {
                float3 q = world/max(noiseSize,50.0);
                // Bounded, smooth 3D domain bending. The distant shape stays
                // world-anchored; only the player's clear transition rolls.
                // Blend displacements rather than multiplying time by rim:
                // this avoids a growing phase discontinuity at the boundary.
                float3 a = q*6.2831853;
                float t = clock*max(noiseSpeed,0.0);
                float3 stillBend = float3(sin(a.y)+cos(a.z),
                    sin(a.z)+cos(a.x),sin(a.x)+cos(a.y));
                float3 rollingBend = float3(sin(a.y+t)+cos(a.z-t*0.7),
                    sin(a.z+t*0.8)+cos(a.x-t),sin(a.x+t*0.6)+cos(a.y-t*0.5));
                float warp = clamp(noiseFlow,0.0,2.0)*0.16;
                q += warp*lerp(stillBend,rollingBend,rim*motion);
                q.xy -= velocity*(warp*0.9*rim*motion);
                float n = Texture3DSampleLevel(volumeNoise,volumeNoiseSampler,q,0).r;
                detail = saturate((n-0.64)/0.22);
                detail = detail*detail*(3.0-2.0*detail);
            }
            // The former +/-10% modulation flattened the medium into a veil.
            // A broad soft density range exposes rounded, self-shadowed forms
            // without any height cap, additional carrier or extra noise read.
            float body = detail*detail;
            float shape = lerp(1.0,0.28+1.8*body,saturate(atmosphereVariation));
            float animatedDetail = clamp((palette-0.4)*2.0,-0.65,0.65);
            shape *= max(1.0+saturate(boundaryDetail)*rim*motion*animatedDetail*0.4,0.5);
            float density = max(atmosphereAmount,0.0)*shape*environment.x;
            // Avoid global palette breathing from the animated input image.
            float stillPalette = saturate(0.45+detail*0.2);
            float colorPalette = lerp(stillPalette,palette,
                saturate(boundaryDetail)*rim*motion*0.35);
            return float4(density,colorPalette,velocity*rim*motion);
        }
        float feather = max(heightFeather,0.0001);
        float plane = abs(p.z-0.5)-zBias;
        // Keep Ninja's extrusion interval, but flatten its endpoint slopes.
        // Linear saturation combined with high extinction made cloud edges
        // look cut out. View AND light use the same C2 transition.
        float edge=saturate((plane-densityOffset-palette*height
            +feather*0.5)/feather);
        float extrusion=1.0-edge*edge*edge*(edge*(edge*6.0-15.0)+10.0);
        float fade = pow(1.0-saturate(abs(p.z-0.5)/max(height+feather,0.0001)),
            max(fadeFalloff,0.001));
        float density = saturate(extrusion+max(baseDensity,0.0)*fade*palette);
        [branch] if (density <= 0.001) return float4(density,palette,velocity);
        [branch] if (noiseAmount > 0.001)
        {
            // The 2D RT remains the shape and motion authority. A single weak
            // XYZ sample only adds depth to its extrusion, never a second cloud.
            float3 q = world/max(noiseSize,50.0);
            q.xy -= velocity*clamp(noiseFlow,0.0,2.0);
            q += clock*noiseSpeed*float3(0.037,-0.023,0.041);
            float n = Texture3DSampleLevel(volumeNoise,volumeNoiseSampler,q,0).r;
            // VolumeNoise2 is biased high; centre the small modulation near .75.
            float detail = clamp((n-0.75)*2.0,-0.5,0.5);
            density *= max(1.0+clamp(noiseAmount,0.0,1.0)*detail,0.0);
        }
        density *= environment.x;
        return float4(density,palette,velocity);
    }
};
FYogNinjaHybridDensity HybridField;
