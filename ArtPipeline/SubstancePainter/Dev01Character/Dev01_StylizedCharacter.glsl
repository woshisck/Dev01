// Dev01 stylized-character authoring preview for Substance 3D Painter 7.4+.
//
// The preview uses neutral-white calibration lighting so it does not recolor
// authored textures.  Artists may rotate only the key-light direction; light
// color, energy, exposure, contrast and Unreal lighting profiles remain fixed.
// Painter's environment map is deliberately not sampled.

import lib-pbr.glsl
import lib-bent-normal.glsl
import lib-utils.glsl

//: param auto channel_basecolor
uniform SamplerSparse basecolor_tex;
//: param auto channel_roughness
uniform SamplerSparse roughness_tex;
//: param auto channel_metallic
uniform SamplerSparse metallic_tex;
//: param auto channel_specularlevel
uniform SamplerSparse specular_control_tex;
//: param auto channel_user0
uniform SamplerSparse diffuse_bias_tex;

// These are the only artist-facing light controls. They rotate a fixed-energy
// neutral key and never change its color, intensity or the exported textures.
//: param custom { "default": -35.0, "label": "Key azimuth", "min": -180.0, "max": 180.0, "group": "Preview Light Direction" }
uniform float key_azimuth_degrees;
//: param custom { "default": 35.0, "label": "Key elevation", "min": -10.0, "max": 85.0, "group": "Preview Light Direction" }
uniform float key_elevation_degrees;

// Inspection output changes only the viewport visualization.
//: param custom {
//:   "default": 0,
//:   "label": "Viewport output",
//:   "group": "Authoring Checks",
//:   "widget": "combobox",
//:   "values": {
//:     "Final - Neutral two-tone PBR": 0,
//:     "Base Color": 1,
//:     "Diffuse Bias": 2,
//:     "Specular Control": 3,
//:     "Roughness": 4,
//:     "Metallic": 5,
//:     "Key Lit Mask": 6,
//:     "World Normal": 7,
//:     "Linear Highlight Headroom": 8
//:   }
//: }
uniform int viewport_output;

const float DEV01_PI = 3.14159265358979323846;

float sampleScalarOrDefault(SamplerSparse sparse_sampler, SparseCoord coord, float default_value)
{
    // Painter returns value in R and authored coverage in G.  Missing paint is
    // neutral for both Dev01 packed controls.
    vec2 sampled = textureSparse(sparse_sampler, coord).rg;
    return clamp(sampled.r + default_value * (1.0 - sampled.g), 0.0, 1.0);
}

vec3 previewKeyDirection()
{
    float azimuth = radians(key_azimuth_degrees);
    float elevation = radians(key_elevation_degrees);
    float horizontal = cos(elevation);
    return normalize(vec3(
        horizontal * cos(azimuth),
        sin(elevation),
        horizontal * sin(azimuth)));
}

float ueDGGX(float alpha_squared, float no_h)
{
    float d = (no_h * alpha_squared - no_h) * no_h + 1.0;
    return alpha_squared / max(DEV01_PI * d * d, 1.0e-6);
}

float ueVisSmithJointApprox(float alpha_squared, float no_v, float no_l)
{
    float alpha = sqrt(alpha_squared);
    float vis_v = no_l * (no_v * (1.0 - alpha) + alpha);
    float vis_l = no_v * (no_l * (1.0 - alpha) + alpha);
    return 0.5 / max(vis_v + vis_l, 1.0e-5);
}

vec3 ueFresnelSchlick(vec3 f0, float vo_h)
{
    float one_minus = 1.0 - vo_h;
    float fc = one_minus * one_minus;
    fc = fc * fc * one_minus;
    return clamp(clamp(50.0 * f0.g, 0.0, 1.0) * fc + (1.0 - fc) * f0, 0.0, 1.0);
}

vec3 previewAreaGGX(vec3 normal, vec3 view, vec3 light, vec3 f0, float roughness)
{
    float no_l = max(dot(normal, light), 0.0);
    float no_v = max(abs(dot(normal, view)), 1.0e-5);
    vec3 half_vector = normalize(view + light);
    float no_h = max(dot(normal, half_vector), 0.0);
    float vo_h = max(dot(view, half_vector), 0.0);

    // A finite-source floor keeps the normalized GGX lobe readable without
    // creating a point-light pin highlight in an authoring preview.
    float area_roughness = sqrt(roughness * roughness + 0.22 * 0.22);
    float alpha_squared = pow(max(area_roughness, 0.02), 4.0);
    return no_l
        * ueDGGX(alpha_squared, no_h)
        * ueVisSmithJointApprox(alpha_squared, no_v, no_l)
        * ueFresnelSchlick(f0, vo_h);
}

vec3 neutralAmbientIrradiance(vec3 normal)
{
    // Neutral-white hemispherical fill. The upper/lower difference reveals
    // form but carries no color-temperature shift.  The floor is deliberately
    // non-zero so the two-tone dark side retains material information.
    float sky = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    return vec3(mix(0.55, 0.85, sky));
}

vec3 neutralReflectionRadiance(vec3 reflection, vec3 key_light, float roughness)
{
    // Neutral low-frequency reflection approximation. Painter environment
    // rotation cannot tint or relight it; only the shared key direction moves.
    float sky = clamp(reflection.y * 0.5 + 0.5, 0.0, 1.0);
    float ambient = mix(0.08, 0.22, sky);
    float opening = pow(max(dot(reflection, key_light), 0.0),
                        mix(24.0, 2.0, roughness));
    return vec3(ambient + 0.20 * opening);
}

void outputDebug(vec3 value)
{
    albedoOutput(vec3(1.0));
    diffuseShadingOutput(value);
    specularShadingOutput(vec3(0.0));
}

void shade(V2F inputs)
{
    vec3 base_color = getBaseColor(basecolor_tex, inputs.sparse_coord);
    float roughness = getRoughness(roughness_tex, inputs.sparse_coord);
    float metallic = getMetallic(metallic_tex, inputs.sparse_coord);
    float specular_control = sampleScalarOrDefault(specular_control_tex, inputs.sparse_coord, 0.5);
    float packed_diffuse_bias = sampleScalarOrDefault(diffuse_bias_tex, inputs.sparse_coord, 0.5);

    LocalVectors vectors = computeLocalFrame(inputs);
    computeBentNormal(vectors, inputs);
    vec3 normal = normalize(vectors.normal);
    vec3 view = normalize(vectors.eye);
    vec3 key_light = previewKeyDirection();

    // Exact Dev01 two-tone partition: packed bias [0,1] -> [-1,1], then
    // half-Lambert and the current hard threshold.
    float diffuse_bias = packed_diffuse_bias * 2.0 - 1.0;
    float curve_time = clamp((dot(normal, key_light) + diffuse_bias + 1.0) * 0.5, 0.0, 1.0);
    float lit_mask = step(0.5, curve_time);

    if (viewport_output == 1) { outputDebug(base_color); return; }
    if (viewport_output == 2) { outputDebug(vec3(packed_diffuse_bias)); return; }
    if (viewport_output == 3) { outputDebug(vec3(specular_control)); return; }
    if (viewport_output == 4) { outputDebug(vec3(roughness)); return; }
    if (viewport_output == 5) { outputDebug(vec3(metallic)); return; }
    if (viewport_output == 6) { outputDebug(vec3(lit_mask)); return; }
    if (viewport_output == 7) { outputDebug(normal * 0.5 + 0.5); return; }

    float occlusion = clamp(getAO(inputs.sparse_coord, true, use_bent_normal), 0.0, 1.0);

    // Current engine behavior for MixMap.R: 0.5 neutral, lower suppresses the
    // GGX lobe, higher advances/broadens it while retaining normalized GGX.
    float signed_highlight_control = specular_control * 2.0 - 1.0;
    float highlight_advance = clamp(signed_highlight_control, 0.0, 1.0);
    float highlight_suppression = clamp(signed_highlight_control + 1.0, 0.0, 1.0);
    float direct_roughness = mix(roughness, sqrt(clamp(roughness, 0.0, 1.0)), highlight_advance);
    vec3 specular_color = generateSpecularColor(specular_control, base_color, metallic);

    // Physically based material response: dielectric/metal F0 controls how
    // much incident energy remains for diffuse. Stylization is limited to the
    // two-tone direct-light partition; it does not create extra light energy.
    vec3 diffuse_energy = (1.0 - metallic)
        * clamp(vec3(1.0) - specular_color, 0.0, 1.0);
    vec3 diffuse_brdf = base_color * diffuse_energy / DEV01_PI;

    const vec3 neutral_white = vec3(1.0);
    const float key_irradiance = 2.30;

    // AO remains a contact/detail cue.  It cannot collapse an entire unlit
    // hemisphere to black in the neutral authoring preview.
    const float ambient_occlusion_strength = 0.35;
    vec3 ambient_irradiance = neutralAmbientIrradiance(normal)
        * mix(1.0, occlusion, ambient_occlusion_strength);
    vec3 direct_irradiance = neutral_white * key_irradiance * lit_mask;
    vec3 diffuse = diffuse_brdf * (ambient_irradiance + direct_irradiance);

    vec3 direct_specular = previewAreaGGX(
        normal, view, key_light, specular_color, direct_roughness)
        * neutral_white
        * key_irradiance
        * highlight_suppression
        * lit_mask;

    float specular_occlusion = specularOcclusionCorrection(occlusion, metallic, roughness);
    vec3 reflection = reflect(-view, normal);
    vec3 environment_specular = specular_color
        * neutralReflectionRadiance(reflection, key_light, roughness)
        * mix(1.0, specular_occlusion, ambient_occlusion_strength)
        * highlight_suppression;

    vec3 linear_lighting = diffuse + direct_specular + environment_specular;
    if (viewport_output == 8)
    {
        float peak = max(linear_lighting.r, max(linear_lighting.g, linear_lighting.b));
        outputDebug(peak > 1.0
            ? mix(vec3(0.15, 0.0, 0.0), vec3(1.0, 0.0, 0.0), clamp(peak - 1.0, 0.0, 1.0))
            : vec3(0.0, clamp(peak, 0.0, 1.0), 0.0));
        return;
    }

    // Output linear light once.  Painter's ACES display transform handles the
    // monitor mapping; Unreal exposure/profile/tone are intentionally absent.
    outputDebug(linear_lighting);
}
