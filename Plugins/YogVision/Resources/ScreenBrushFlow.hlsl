// Texture RG is a velocity, never an opacity or a normal. Continuous local
// advection keeps each hatch intact instead of averaging two displaced copies.
struct FYogScreenBrushFlow
{
    float VolumePaint(Texture2D Brush,SamplerState BrushSampler,Texture2D Map,
        SamplerState MapSampler,float3 Position,float Time,float Scale,float Warp,float FlowScale)
    {
        // One tile = 12 metres at scale 1. Never uses the moving view origin.
        float3 p=Position*(max(Scale,.25)/1200.0);
        float2 a=p.xy,b=p.yz+float2(.37,.19),c=p.zx+float2(.61,.43);
        float phase=sin(Time*.35)*Warp*.07;
        a+=Velocity(Map,MapSampler,a,FlowScale,2)*phase;
        b+=Velocity(Map,MapSampler,b,FlowScale,2)*phase;
        c+=Velocity(Map,MapSampler,c,FlowScale,2)*phase;
        // Filter sub-froxel grain rather than putting it back in screen space.
        float pa=Texture2DSampleLevel(Brush,BrushSampler,a,3).r;
        float pb=Texture2DSampleLevel(Brush,BrushSampler,b,3).r;
        float pc=Texture2DSampleLevel(Brush,BrushSampler,c,3).r;
        return pa*.65+pb*.20+pc*.15;
    }
    float2 Velocity(Texture2D Map, SamplerState MapSampler, float2 Canvas, float Scale, float Lod)
    {
        float2 uv = Canvas * Scale + .5;
        float2 v = Texture2DSampleLevel(Map,MapSampler,uv,Lod).rg*2-1;
        // Mirrored tiling needs mirrored vectors, not just mirrored colors.
        float2 parity = 1-2*fmod(abs(floor(uv)),2);
        v *= parity * sin(frac(uv)*3.14159265);
        return v / max(length(v),1);
    }
    float Paint(Texture2D Brush, SamplerState BrushSampler, Texture2D Map,
        SamplerState MapSampler, float2 Canvas, float Time, float Scale,
        float Warp, float FlowScale, float PixelFootprint)
    {
        uint w,h; Brush.GetDimensions(w,h);
        float lod=max(0,log2(max(w,h)*max(Scale,.25)*max(PixelFootprint,.000001)));
        float2 uv=Canvas*Scale+.5;
        // Keep advection smaller than a hatch cluster. Large phase separation
        // averages unrelated lines and turns the pen into cloudy stippling.
        float2 velocity=Velocity(Map,MapSampler,Canvas,FlowScale,0)*Warp*.2;
        float2 displacement=velocity*sin(Time*.35)*.5;
        float a=Texture2DSampleLevel(Brush,BrushSampler,uv-displacement,lod).r;
        float2 broadUV=uv.yx*.61+float2(.17,.31);
        float2 broadDisplacement=displacement.yx*.61;
        float broadLod=max(0,lod+log2(.61));
        float c=Texture2DSampleLevel(Brush,BrushSampler,broadUV-broadDisplacement,broadLod).r;
        // Mid-grey is neutral pigment. Keep the new charcoal mask's dark
        // scumbles rather than normalizing every patch into white highlights.
        float ink=(a-.5)+.45*(c-.5);
        return .5+clamp(ink*1.7,-.55,.55);
    }
};
