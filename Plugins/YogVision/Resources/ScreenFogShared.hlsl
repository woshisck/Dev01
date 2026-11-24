// One shared screen-space region definition for native density AND finish.
// UV distances use viewport height, so aspect/resolution do not stretch circles.
struct FYogScreenFogRegion
{
    // World-space clearing: only this observer bubble follows its owner.
    float4 Volume(float3 Position,float3 Observer,float Radius,float Feather,
        float Disturbance,float Enabled,float LightClear,
        float4 P0,float4 D0,float4 C0,float4 P1,float4 D1,float4 C1,
        float4 P2,float4 D2,float4 C2,float4 P3,float4 D3,float4 C3)
    {
        float vision=lerp(1,smoothstep(max(Radius+Disturbance,0),
            max(Radius+Disturbance,0)+max(Feather,1),length(Position-Observer)),saturate(Enabled));
        float light=max(max(Light(Position,P0,D0,C0),Light(Position,P1,D1,C1)),
            max(Light(Position,P2,D2,C2),Light(Position,P3,D3,C3)));
        return float4(saturate(vision*(1-light*saturate(LightClear))),
            max(4*vision*(1-vision),4*light*(1-light)*saturate(LightClear)),light,vision);
    }
    float Hash(float3 p)
    {
        p=frac(p*.1031);
        p+=dot(p,p.yzx+33.33);
        return frac((p.x+p.y)*p.z);
    }
    float Noise(float3 p)
    {
        float3 i=floor(p),f=frac(p);
        f=f*f*(3-2*f);
        return lerp(lerp(lerp(Hash(i),Hash(i+float3(1,0,0)),f.x),
                         lerp(Hash(i+float3(0,1,0)),Hash(i+float3(1,1,0)),f.x),f.y),
                    lerp(lerp(Hash(i+float3(0,0,1)),Hash(i+float3(1,0,1)),f.x),
                         lerp(Hash(i+float3(0,1,1)),Hash(i+float3(1,1,1)),f.x),f.y),f.z);
    }
    // Broad stationary density masses, shared by volume and finish. They do
    // not follow pixel grain, animation time, the camera, or texture phase.
    float Cluster(float3 Position,float Scale)
    {
        float3 p=Position*max(Scale,.00001)*.2;
        return .75*Noise(p)+.25*Noise(p*2.03+float3(17,31,7));
    }
    float2 Project(float3 Position, float3 Camera, float3 Forward, float3 Right,
        float3 Up, float2 ProjectionScale, bool Ortho)
    {
        float3 d=Position-Camera;
        return float2(dot(d,Right),-dot(d,Up))*ProjectionScale
            /(Ortho ? 1 : max(dot(d,Forward),10))*.5+.5;
    }
    float Light(float3 Position, float4 PositionRadius, float4 DirectionCone, float4 Colour)
    {
        if(PositionRadius.w<=0 || Colour.a<=0) return 0;
        float3 delta=Position-PositionRadius.xyz;
        float distance=length(delta);
        float radial=1-smoothstep(.25,1,distance/max(PositionRadius.w,1));
        float cone=DirectionCone.w < -1 ? 1 : smoothstep(DirectionCone.w,
            lerp(DirectionCone.w,1,.35),dot(delta/max(distance,.001),DirectionCone.xyz));
        return saturate(radial*cone*Colour.a);
    }
    // x=remaining fog, y=transition, z=local-light influence, w=vision fog.
    float4 Evaluate(float2 UV, float2 Centre, float Aspect, float Radius,
        float Feather, float Disturbance, float Enabled, float LightClear,
        float3 Position, float4 P0,float4 D0,float4 C0,
        float4 P1,float4 D1,float4 C1,float4 P2,float4 D2,float4 C2,
        float4 P3,float4 D3,float4 C3)
    {
        float radius=length((UV-Centre)*float2(Aspect,1));
        float vision=smoothstep(max(Radius+Disturbance,0),max(Radius+Disturbance,0)+max(Feather,.01),radius);
        vision=lerp(1,vision,saturate(Enabled));
        float light=max(max(Light(Position,P0,D0,C0),Light(Position,P1,D1,C1)),
            max(Light(Position,P2,D2,C2),Light(Position,P3,D3,C3)));
        float fog=vision*(1-light*saturate(LightClear));
        float transition=max(4*vision*(1-vision),4*light*(1-light)*saturate(LightClear));
        return float4(saturate(fog),saturate(transition),light,vision);
    }
};
