// Dev01 stylized-character authoring preview for Substance 3D Painter 7.4+.
//
// Deliberately limited to the stable 7.4 GLSL interface. This preview mirrors
// the Dev01 packing, Diffuse Bias two-tone split and MixMap.R highlight intent.
// UE scene shadows, Lumen, Lighting Profile and final post processing remain
// Unreal-only acceptance items.

import lib-utils.glsl

//: param auto channel_basecolor
uniform SamplerSparse basecolor_tex;
//: param auto channel_normal
uniform SamplerSparse normal_tex;
//: param auto channel_roughness
uniform SamplerSparse roughness_tex;
//: param auto channel_metallic
uniform SamplerSparse metallic_tex;
//: param auto channel_specularlevel
uniform SamplerSparse specular_control_tex;
//: param auto channel_user0
uniform SamplerSparse diffuse_bias_tex;

//: param custom { "default": -35.0, "label": "Key azimuth", "min": -180.0, "max": 180.0, "group": "Preview Light Direction" }
uniform float key_azimuth_degrees;
//: param custom { "default": 35.0, "label": "Key elevation", "min": -10.0, "max": 85.0, "group": "Preview Light Direction" }
uniform float key_elevation_degrees;

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

float sampleScalarOrDefault(SamplerSparse sampler_sparse, SparseCoord coord, float default_value)
{
    // Painter's sparse sampler stores authored coverage in G.
    vec2 sampled = textureSparse(sampler_sparse, coord).rg;
    return clamp(sampled.r + default_value * (1.0 - sampled.g), 0.0, 1.0);
}

vec3 sampleBaseColor(SamplerSparse sampler_sparse, SparseCoord coord)
{
    return clamp(textureSparse(sampler_sparse, coord).rgb, 0.0, 1.0);
}

vec3 sampleWorldNormal(SamplerSparse sampler_sparse, SparseCoord coord, V2F inputs)
{
    // Painter's Normal channel is tangent-space OpenGL normal data. Rebuild Z
    // from RG so the preview does not depend on newer lib-normal helpers.
    vec2 xy = textureSparse(sampler_sparse, coord).rg * 2.0 - 1.0;
    float z = sqrt(max(1.0 - dot(xy, xy), 0.0));
    vec3 tangent_normal = normalize(vec3(xy, z));
    vec3 tangent = normalize(inputs.tangent);
    vec3 bitangent = normalize(inputs.bitangent);
    return normalize(tangent * tangent_normal.x + bitangent * tangent_normal.y + inputs.normal * tangent_normal.z);
}

vec3 previewKeyDirection()
{
    float azimuth = radians(key_azimuth_degrees);
    float elevation = radians(key_elevation_degrees);
    float horizontal = cos(elevation);
    return normalize(vec3(horizontal * cos(azimuth), sin(elevation), horizontal * sin(azimuth)));
}

vec3 fresnelSchlick(vec3 f0, float voh)
{
    float f = pow(1.0 - clamp(voh, 0.0, 1.0), 5.0);
    return f0 + (vec3(1.0) - f0) * f;
}

float ggxDistribution(float alpha_squared, float noh)
{
    float d = (noh * alpha_squared - noh) * noh + 1.0;
    return alpha_squared / max(DEV01_PI * d * d, 1.0e-6);
}

float smithVisibility(float alpha_squared, float nov, float nol)
{
    float alpha = sqrt(alpha_squared);
    float vis_v = nol * (nov * (1.0 - alpha) + alpha);
    float vis_l = nov * (nol * (1.0 - alpha) + alpha);
    return 0.5 / max(vis_v + vis_l, 1.0e-5);
}

void outputDebug(vec3 value)
{
    albedoOutput(vec3(1.0));
    diffuseShadingOutput(value);
    specularShadingOutput(vec3(0.0));
}

void shade(V2F inputs)
{
    vec3 base_color = sampleBaseColor(basecolor_tex, inputs.sparse_coord);
    float roughness = sampleScalarOrDefault(roughness_tex, inputs.sparse_coord, 0.5);
    float metallic = sampleScalarOrDefault(metallic_tex, inputs.sparse_coord, 0.0);
    float specular_control = sampleScalarOrDefault(specular_control_tex, inputs.sparse_coord, 0.5);
    float packed_diffuse_bias = sampleScalarOrDefault(diffuse_bias_tex, inputs.sparse_coord, 0.5);
    vec3 normal = sampleWorldNormal(normal_tex, inputs.sparse_coord, inputs);
    // Painter 7.4's minimal custom-shader context does not expose camera_pos.
    // Use the stable object-space view approximation so the authoring preview
    // remains portable; this affects only viewport highlight placement.
    vec3 view = normalize(-inputs.position);
    vec3 key_light = previewKeyDirection();

    // Exact project partition: Packed Bias [0,1] -> [-1,1], then hard split.
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

    // MixMap.R behavior: 0.5 neutral; lower suppresses highlight, higher
    // broadens it. Use direct GLSL GGX to avoid Painter-version library drift.
    float signed_control = specular_control * 2.0 - 1.0;
    float highlight_advance = clamp(signed_control, 0.0, 1.0);
    float highlight_suppression = clamp(signed_control + 1.0, 0.0, 1.0);
    float direct_roughness = mix(roughness, sqrt(roughness), highlight_advance);
    float area_roughness = sqrt(direct_roughness * direct_roughness + 0.22 * 0.22);
    float alpha_squared = pow(max(area_roughness, 0.02), 4.0);

    // Painter/UE use 0.5 as the neutral dielectric Specular value, mapping to
    // F0=0.04. Keep MixMap.R in that same 0..0.08 dielectric range while
    // metallic pixels continue to take their F0 from Base Color.
    vec3 f0 = mix(vec3(0.08 * specular_control), base_color, metallic);
    vec3 half_vector = normalize(view + key_light);
    float nol = max(dot(normal, key_light), 0.0);
    float nov = max(abs(dot(normal, view)), 1.0e-5);
    float noh = max(dot(normal, half_vector), 0.0);
    float voh = max(dot(view, half_vector), 0.0);
    vec3 direct_specular = nol * ggxDistribution(alpha_squared, noh)
        * smithVisibility(alpha_squared, nov, nol)
        * fresnelSchlick(f0, voh)
        * 2.30 * highlight_suppression * lit_mask;

    // Keep the diffuse lobe in BRDF units and reserve the Fresnel/metal energy
    // used by the GGX lobe. The stylized hard split only selects fixed neutral
    // irradiance; it does not add a second unnormalized Base Color term.
    vec3 diffuse_energy = (1.0 - metallic)
        * clamp(vec3(1.0) - f0, 0.0, 1.0);
    vec3 diffuse_brdf = base_color * diffuse_energy / DEV01_PI;

    // Neutral fixed fill keeps the dark side readable without using Painter's
    // environment map. 0.55..0.85 and 2.30 are fixed irradiance calibration,
    // not exposure multipliers or artist controls.
    float sky = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient_irradiance = vec3(mix(0.55, 0.85, sky));
    vec3 direct_irradiance = vec3(2.30 * lit_mask);
    vec3 diffuse = diffuse_brdf * (ambient_irradiance + direct_irradiance);
    vec3 reflection = reflect(-view, normal);
    vec3 environment_specular = f0 * mix(0.08, 0.22, clamp(reflection.y * 0.5 + 0.5, 0.0, 1.0))
        * highlight_suppression;
    vec3 linear_lighting = diffuse + direct_specular + environment_specular;

    if (viewport_output == 8)
    {
        float peak = max(linear_lighting.r, max(linear_lighting.g, linear_lighting.b));
        outputDebug(peak > 1.0 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, peak, 0.0));
        return;
    }

    outputDebug(linear_lighting);
}
