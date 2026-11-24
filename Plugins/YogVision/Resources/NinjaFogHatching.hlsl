// World-anchored 2D marks composited inside the owned Ninja fog pass.
// No second raymarch or density/brush texture samples are added.
// The caller accumulates a representative point and light with the existing
// view-ray absorption weights, then evaluates ONE approximate direction there.
// Nothing here reads a scene/carrier normal or changes transmittance.
struct FYogNinjaFogHatching
{
    float3 AnimationVelocityApprox(float3 world, float clock,
        float sizeCm, float warp, float clockRate)
    {
        // First-order motion of the existing animated density coordinates:
        // v ~= -d(warpedCoordinates)/dt. This is NOT NS fluid velocity, nor
        // the old Ninja 2D atlas RG field (which no longer drives this shape).
        float3 a = world / max(sizeCm,50.0) * 6.2831853;
        float3 derivative = float3(
            cos(a.y+clock) + 0.7*sin(a.z-0.7*clock),
            0.8*cos(a.z+0.8*clock) + sin(a.x-clock),
            0.6*cos(a.x+0.6*clock) + 0.5*sin(a.y-0.5*clock));
        return -derivative * (max(sizeCm,50.0)*saturate(warp)*0.22*clockRate);
    }

    float Hash(float2 p)
    {
        return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);
    }

    float Stroke(float phase)
    {
        float aa=max(fwidth(phase),0.015);
        float strokeCoverage=1.0-smoothstep(0.055,0.055+min(aa,0.40),
            abs(frac(phase)-0.5));
        return strokeCoverage*saturate(1.0-aa*1.4);
    }

    float4 Apply(FMaterialPixelParameters MP, float3 fogColor, float T,
        float3 representativeWorld, float3 representativeWorldVelocity,
        float averageLight, float clock, float strength, float spacing,
        float flowStrength, float distanceWeight, float debugMode,
        float worldUnitsPerPixel, float3 anchorRightInput, float3 anchorUpInput,
        float regularity)
    {
        float opacity=saturate(1.0-T);
        // Exactly the same fixed reference-camera basis as the scene pass.
        // No current-view matrix, viewport position, UV or surface normal is
        // used to place the marks or orient their transport direction.
        float rightLength=dot(anchorRightInput,anchorRightInput);
        float3 anchorRight=rightLength>0.0001 ? anchorRightInput*rsqrt(rightLength) : float3(1.0,0.0,0.0);
        float3 upCandidate=anchorUpInput-anchorRight*dot(anchorUpInput,anchorRight);
        float3 fallbackUp=abs(anchorRight.z)<0.99 ? float3(0.0,0.0,1.0) : float3(0.0,1.0,0.0);
        fallbackUp-=anchorRight*dot(fallbackUp,anchorRight);
        float upLength=dot(upCandidate,upCandidate);
        float3 anchorUp=upLength>0.0001 ? upCandidate*rsqrt(upLength) : normalize(fallbackUp);
        float units=max(worldUnitsPerPixel,0.05);
        float2 velocity=float2(dot(representativeWorldVelocity,anchorRight),
            dot(representativeWorldVelocity,anchorUp))/units;
        velocity=clamp(velocity,float2(-120.0,-120.0),float2(120.0,120.0));
        float speed=length(velocity);
        float2 fallback=float2(0.819152,0.573576);
        float2 axis=lerp(fallback,velocity/max(speed,0.001),
            smoothstep(0.5,3.0,speed)*saturate(flowStrength));
        axis=dot(axis,axis)>0.001 ? normalize(axis) : fallback;
        float ordered=saturate(regularity);
        [branch] if(ordered>0.0001)
        {
            // Pencil orientation is invariant under a 180deg reversal. Align
            // that sign before interpolation to avoid opposing vectors passing
            // through zero. Zero regularity retains the exact legacy path.
            float2 alignedAxis=dot(axis,fallback)<0.0 ? -axis : axis;
            axis=normalize(lerp(alignedAxis,fallback,ordered*0.90));
        }
        float light=saturate(averageLight);

        // Premultiplied debug RGB, with the ORIGINAL T retained. The native
        // material's outer DistanceFade multiplies RGB/opacity once more.
        // Against a black scene, mode 1 therefore displays final 1-T, while
        // modes 2/3 display opacity-weighted motion/light (not straight data).
        if(debugMode>0.5 && debugMode<1.5)
            return float4(opacity.xxx,T);
        if(debugMode>1.5 && debugMode<2.5)
            return float4(float3(axis*0.5+0.5,saturate(speed/80.0))*opacity,T);
        if(debugMode>2.5)
            return float4(light.xxx*opacity,T);
        if(strength<=0.001 || opacity<=0.001)
            return float4(fogColor,T);

        // The representative point is accumulated in the original march.
        // Seventy-two reference pixels describe a fixed patch in world space,
        // so moving the camera cannot sweep a viewport grid across the cloud.
        float2 pixel=float2(dot(representativeWorld,anchorRight),
            dot(representativeWorld,anchorUp))/units;
        float2 cell=floor(pixel/72.0);
        float seed=Hash(cell);
        float2 localPixel=pixel-(cell+0.5)*72.0;
        float2 tangent=axis;
        float2 normal=float2(-tangent.y,tangent.x);
        // Bounded continuous deformation, driven by the same clock/direction
        // as the XYZ density. Pausing that clock also pauses the line motion.
        float flowAmount=clamp(flowStrength,0.0,2.0);
        float along=dot(localPixel,tangent);
        float across=dot(localPixel,normal);
        float bend=sin(along*0.025+clock*0.65+seed*6.2831853)
            * min(speed*0.055,4.0)*flowAmount*lerp(1.0,0.20,ordered);
        float phase=(across+bend)/max(spacing,2.5)+seed;
        // Broad quiet gaps replace the old fine sinusoidal breakup. Only the
        // cloud-clock bend animates the strokes; there is no granular noise.
        float segment=frac(along/max(spacing*10.0,64.0)+seed);
        float broken=smoothstep(0.08,0.20,segment)
            *(1.0-smoothstep(0.66,0.82,segment));
        float pencil=Stroke(phase)*broken;
        float crossStroke=Stroke((across*0.65+along*0.76-bend*0.4)
            /max(spacing*1.24,3.0)+seed*1.7);
        float crossSegment=frac(across/max(spacing*12.0,72.0)+seed*0.73);
        float crossGap=smoothstep(0.10,0.22,crossSegment)
            *(1.0-smoothstep(0.62,0.78,crossSegment));
        pencil=saturate(pencil+crossStroke*crossGap*(1.0-light)*0.25*lerp(1.0,0.25,ordered));

        // Use the actual native DistanceFade-adjusted fog coverage. Keep the
        // strongest drawing near the fog/clear transition; solid fog is quiet.
        float finalOpacity=opacity*saturate(distanceWeight);
        float region=smoothstep(0.025,0.20,finalOpacity)
            * (1.0-0.72*smoothstep(0.68,1.0,finalOpacity));
        float darkness=0.30+0.70*(1.0-light);
        float ink=saturate(pencil*clamp(strength,0.0,2.0)*region*darkness*0.70);
        return float4(max(fogColor,0.0)*(1.0-ink),T);
    }
};
FYogNinjaFogHatching FogHatching;
