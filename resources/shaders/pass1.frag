#version 130

in vec3 worldPos;
in vec3 worldNormal;
in vec3 cameraPos;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;     // TODO: rename this uniform -> drawDistance
uniform float reflectance;
uniform float attenuationCoeff;

// ray definition
struct Ray {
    vec3 origin, direction;     // starting point and direction
};

// primary reflections: rasterization
vec4 primaryReflections() {
    vec3 worldIncident = cameraPos - worldPos;
    vec3 nWorldPos = normalize(worldIncident);
    vec3 nWorldNormal = normalize(worldNormal);

   // Material's reflectivity property
    if (reflectance > 0)
        nWorldNormal = min(nWorldNormal * reflectance, 1.0);

    // Distance calculation
    float viewDistance = length(worldIncident);

    // Attenuation effect of sound in the water
    nWorldNormal = nWorldNormal * exp(-2 * attenuationCoeff * viewDistance);

    // Normalize distance using range value (farPlane)
    float nViewDistance = viewDistance / farPlane;

    // presents the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (nViewDistance <= 1) {
        if (drawDepth)  output.y = nViewDistance;
        if (drawNormal) output.z = abs(dot(nWorldPos, nWorldNormal));
    }

    return output;
}

// secondary reflections: ray-triangle intersection
vec4 secondaryReflections(vec4 firstR) {

    // calculate the reflection direction for an incident vector
    // TODO: use the nNormalWorldSpace after first reflection process:
    //       normal mapping, reflectivity and attenuation.
    vec3 worldIncident = cameraPos - worldPos;
    vec3 nWorldNormal = normalize(worldNormal);
    vec3 reflectedDir = reflect(-worldIncident, nWorldNormal);

    // set current ray
    Ray ray = Ray(worldPos, reflectedDir);

    // TODO: perform ray-triangle intersection
    vec4 output = vec4(0,0,0,1);

    // // perform ray-triangle intersection only for pixels with valid normal values
    // if (firstR.z > 0) {
    //     // find the closest triangle to the origin point
    //     Triangle best;
    //     float best_dist;
    //     nearest(ray.origin, best, best_dist);

    //     // ray-triangle intersection
    //     vec3 intersectedPoint;
    //     bool intersected = rayTriangleIntersection(ray, best.a, best.b, best.c, intersectedPoint);

    //     // TODO: compute the correct depth/normal values
    //     if (intersected) {
    //         output.y = 1;    // depth
    //         output.z = 1;    // normal
    //     }
    // }

    return output;
}

void main() {
    // output: primary reflections by rasterization
    vec4 firstR = primaryReflections();

    // output: secondary reflections by ray-tracing
    vec4 secndR = secondaryReflections(firstR);

    gl_FragData[0] = firstR;
    // gl_FragData[0] = secndR;
}