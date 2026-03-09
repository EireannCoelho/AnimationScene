#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragUV;

out vec4 fragColor;

uniform sampler2D diffuseTex;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform vec3 wallColor;
uniform int isTree;

uniform int useOverrideColor;   
uniform vec3 overrideColor;

uniform int isEmissive;
uniform float timeVal;
uniform float emissiveStrength;

uniform int presentType;
uniform vec3 solidColor;
uniform vec3 stripe1;
uniform vec3 stripe2;

uniform int isPresent;
uniform int isCurtain;

uniform int isWindow;
uniform vec3 windowColor;

uniform int isPainting;
uniform int paintingType;  // 0=flower, 1=cactus

uniform int isSnowSurface;  // 1 = brighter ambient for white floor/ceiling
uniform int isSnowRoomWall;  // 1 = flat light blue, no lighting variation (all walls same)

uniform int isForestTree;   // 1 = foliage: dark green base, white snow tips (uses fragUV.y)
uniform int isForestTreeWall;  // 1 = wall trees: white at base (snow), dark green at tip; 0 = ground: white at tip

void main()
{
    vec3 baseColor;

    // =====================================================
    // COLOR SELECTION
    // =====================================================

    // 1. override (ornaments, star, bulbs)
    if (useOverrideColor == 1 && isPresent == 0 && isCurtain == 0) {
        baseColor = overrideColor;
        // Forest tree foliage
        if (isForestTree == 1) {
            if (isForestTreeWall == 1) {
                // Wall: just a little white at base (snow), keep the fade
                float snowBlend = smoothstep(0.5, 0.15, fragUV.y);
                baseColor = mix(baseColor, vec3(0.92, 0.95, 0.98), snowBlend * 0.35);
            } else {
                // Ground: flipped - white at base, green at tips (experiment)
                float greenBlend = smoothstep(0.0, 0.8, fragUV.y);
                baseColor = mix(vec3(0.96, 0.98, 1.0), baseColor, greenBlend);
            }
        }
    }

    // 2. Tree
    else if (isTree == 1) {
        vec3 base = vec3(0.07, 0.25, 0.07);

        float n = fract(sin(dot(fragUV * 200.0,
                                vec2(12.9898, 78.233))) * 43758.5453);

        float streak = smoothstep(0.45, 0.55, fract(fragUV.y * 40.0));

        baseColor = base + 0.2 * n + 0.15 * streak;
    }

    else if (isCurtain == 1) {
        baseColor = overrideColor;

        // --------------------------------------------------
        // CURTAIN SHADOW BOOST — makes folds darker
        // --------------------------------------------------
        // dot(normal, lightDir) gives us how front-facing it is
        float NdotL = dot(normalize(fragNormal), normalize(lightPos - fragPos));

        // Darken areas facing away from light
        float foldShadow = clamp(0.4 + 0.6 * NdotL, 0.0, 1.0);
        baseColor *= foldShadow;

        // --------------------------------------------------
        // EXTRA AMBIENT OCCLUSION — darken inner folds
        // Use UV coordinate to simulate fold depth
        float ao = smoothstep(0.2, 0.5, abs(sin(fragUV.x * 20.0)));
        baseColor *= (0.75 + 0.25 * ao);
    }


    // 4. Presents
    else if (isPresent == 1) {

        if (presentType == 0) {
            baseColor = solidColor;
        }
        else if (presentType == 1) {
            float s = sin(fragPos.x * 20.0);
            float mask = s > 0.0 ? 1.0 : 0.0;
            baseColor = mix(stripe1, stripe2, mask);
        }
        else if (presentType == 2) {
            vec3 box = solidColor;

            float crossX = step(abs(fragPos.x), 0.05);
            float crossZ = step(abs(fragPos.z), 0.05);
            float bowMask = max(crossX, crossZ);

            vec3 bowColor = vec3(1.0, 0.1, 0.1);
            baseColor = mix(box, bowColor, bowMask);
        }
    }

    else if (isWindow == 1) {
        // base color: soft sky blue
        baseColor = windowColor;

        // brighten center (fake glow)
        float glow = smoothstep(0.0, 1.0, fragUV.y);
        baseColor *= (0.7 + 0.3 * glow);

        // weak emissive contribution
        baseColor += vec3(0.15, 0.15, 0.20);
    }

    else if (isPainting == 1) {
        // Procedural paintings: flower or cactus
        vec2 uv = fragUV;
        vec2 center = vec2(0.5, 0.5);

        if (paintingType == 0) {
            // Flower: red/pink petals around center
            float dist = length(uv - center);
            float angle = atan(uv.y - center.y, uv.x - center.x);
            float petals = 5.0;
            float petal = sin(angle * petals) * 0.15 + 0.35;
            float inPetal = smoothstep(petal, petal - 0.08, dist);
            vec3 petalColor = vec3(1.0, 0.4, 0.5);
            vec3 centerColor = vec3(1.0, 0.9, 0.3);
            float inCenter = smoothstep(0.12, 0.08, dist);
            baseColor = mix(vec3(0.95, 0.9, 0.85), petalColor, inPetal);
            baseColor = mix(baseColor, centerColor, inCenter);
        }
        else {
            // Cactus: green body with arms
            float dist = length(uv - center);
            float angle = atan(uv.y - center.y, uv.x - center.x);
            // Main body - elongated oval
            vec2 scaled = (uv - center) * vec2(0.8, 1.2);
            float body = 1.0 - smoothstep(0.25, 0.35, length(scaled));
            // Arms - simple rectangles
            float arm1 = smoothstep(0.15, 0.2, abs(uv.x - center.x - 0.2)) *
                        smoothstep(0.4, 0.35, abs(uv.y - center.y));
            float arm2 = smoothstep(0.15, 0.2, abs(uv.x - center.x + 0.15)) *
                        smoothstep(0.35, 0.3, abs(uv.y - center.y + 0.1));
            vec3 cactusGreen = vec3(0.2, 0.55, 0.25);
            vec3 darkGreen = vec3(0.15, 0.4, 0.18);
            baseColor = vec3(0.92, 0.88, 0.8);  // canvas
            baseColor = mix(baseColor, cactusGreen, body);
            baseColor = mix(baseColor, darkGreen, arm1);
            baseColor = mix(baseColor, darkGreen, arm2);
        }
    }

    // 5. Walls / floor / ceiling
    else {
        baseColor = wallColor;
    }

    // =======================================================
    // LIGHTING
    // =======================================================

    vec3 norm = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(viewPos - fragPos);

    float ambientFactor = (isSnowSurface == 1) ? 0.85 : 0.18;
    vec3 ambient  = ambientFactor * baseColor;
    float diff    = max(dot(norm, L), 0.0);
    vec3 diffuse  = diff * baseColor;

    vec3 reflectDir = reflect(-L, norm);
    float spec = pow(max(dot(V, reflectDir), 0.0), 32.0);
    vec3 specular = spec * lightColor * 0.5;

    // emissive lights
    vec3 emis = vec3(0.0);
    if (isEmissive == 1) {
        float pulse = 0.75 + 0.25 * sin(timeVal * 0.6 + fragPos.x * 10.0);
        emis = baseColor * emissiveStrength * pulse;
    }

    // Snow room walls: flat color, fade top to white (blend into ceiling)
    if (isSnowRoomWall == 1) {
        float fadeBlend = smoothstep(2.5, 5.0, fragPos.y);
        vec3 wallColorFaded = mix(baseColor, vec3(1.0, 1.0, 1.0), fadeBlend);
        fragColor = vec4(wallColorFaded, 1.0);
    } else {
        fragColor = vec4(ambient + diffuse + specular + emis, 1.0);
    }
}
