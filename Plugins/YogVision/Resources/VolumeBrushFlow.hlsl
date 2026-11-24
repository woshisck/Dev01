// Shared thick-paint flow kernel for air, smoke and future fire materials.
// Two advected phases crossfade before either UV phase resets. Mirrored sampler
// addressing removes tile value discontinuities in the generated source image.
struct FYogVolumeBrushFlow
{
    float Sample(Texture2D Brush, SamplerState BrushSampler, float2 Canvas,
        float Time, float Scale, float Warp, float PixelFootprint)
    {
        uint width,height;Brush.GetDimensions(width,height);
        float Lod=max(0,log2(max(width,height)*max(Scale,.25)*max(PixelFootprint,.000001)));
        float2 uv = Canvas * max(Scale,.25);
        float2 velocity = float2(sin(uv.y*.83+.4*sin(uv.x*.31)),
            -sin(uv.x*.71+.35*cos(uv.y*.43))) * Warp * 2;
        float p0 = frac(Time*.2), p1 = frac(Time*.2+.5);
        float2 drift = float2(Time*.013,-Time*.019);
        float a = Texture2DSampleLevel(Brush,BrushSampler,uv-velocity*(p0-.5)+drift,Lod).r;
        float b = Texture2DSampleLevel(Brush,BrushSampler,uv-velocity*(p1-.5)+drift,Lod).r;
        return lerp(a,b,abs(p0*2-1));
    }
};
