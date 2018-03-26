#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in vec3 directionEyeSpace;
in vec3 reflectedDir;
in mat3 TBN;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;
uniform float reflectance;
uniform float attenuationCoeff;

uniform sampler2D normalTex;
uniform bool useNormalTex;

uniform sampler2D trianglesTex;
uniform int trianglesTexSize;

out vec4 outData;

// ray definition
struct Ray {
    vec3 origin;
    vec3 direction;
};

// builds a plane with the specified three coordinates
vec4 buildPlane(vec3 a, vec3 b, vec3 c) {
    vec4 plane;
    plane.xyz = normalize(cross(b - a, c - a));
    plane.w = dot(plane.xyz, a);
    return plane;
}

// computes the intersection between the ray and plane
bool rayPlaneIntersection(Ray ray, vec4 plane, out vec3 intersectedPoint) {
    // computes distance between ray-plane
    float t = (plane.w - dot(plane.xyz, ray.origin)) / dot(plane.xyz, ray.direction);
    if (t < 0.0) return false;

    intersectedPoint = ray.origin + ray.direction * t;

    return true;
}

// computes the intersection between the ray and triangle
bool rayTriangleIntersection (Ray ray, vec3 a, vec3 b, vec3 c, out vec3 intersectedPoint) {
    // get ray-triangle edges
    vec3 pa = a - ray.origin;
    vec3 pb = b - ray.origin;
    vec3 pc = c - ray.origin;

    // check if ray direction cross the triangle ABC
    intersectedPoint.x = dot(pb, cross(ray.direction, pc));
    if (intersectedPoint.x < 0.0) return false;

    intersectedPoint.y = dot(pc, cross(ray.direction, pa));
    if (intersectedPoint.y < 0.0) return false;

    intersectedPoint.z = dot(pa, cross(ray.direction, pb));
    if (intersectedPoint.z < 0.0) return false;

    // triangle's plane
    vec4 plane = buildPlane(a, b, c);

    // triangle's normal
    vec3 normal = plane.xyz;

    // check ray intersection with plane, which is spanned by the triangle
    return rayPlaneIntersection(ray, plane, intersectedPoint);
}

// primary reflections: rasterization
vec4 primaryReflections() {
    vec3 nNormalEyeSpace;

    // Normal for textured scenes (by normal mapping)
    if (useNormalTex) {
        vec3 normalRGB = texture2D(normalTex, gl_TexCoord[0].xy).rgb;
        vec3 normalMap = (normalRGB * 2.0 - 1.0) * TBN;
        nNormalEyeSpace = normalize(normalMap);
    }
    else {
        nNormalEyeSpace = normalize(normalEyeSpace);
    }

    // Material's reflectivity property
    if (reflectance > 0) {
        nNormalEyeSpace = min(nNormalEyeSpace * reflectance, 1.0);
    }

    // Depth calculation
    vec3 nPositionEyeSpace = normalize(-positionEyeSpace);
    float linearDepth = sqrt(positionEyeSpace.x * positionEyeSpace.x +
                             positionEyeSpace.y * positionEyeSpace.y +
                             positionEyeSpace.z * positionEyeSpace.z);
    linearDepth = linearDepth / farPlane;
    gl_FragDepth = linearDepth;

    // Attenuation effect of sound in the water
    nNormalEyeSpace = nNormalEyeSpace * exp(-2 * attenuationCoeff * linearDepth);

    // output the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawNormal) output.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
        if (drawDepth)  output.y = linearDepth;
    }
    return output;
}

// secondary reflections: rasterization using triangle intersection
vec4 secondaryReflections(in vec4 primaryRefl) {
    vec3 data = texelFetch(trianglesTex, ivec2(gl_FragCoord.xy), 0).xyz;

    Ray ray;
    ray.origin = directionEyeSpace;
    ray.direction = reflectedDir;

    // ray-triangle intersection process
    bool hasIntersection = false;
    vec3 intersectedPoint;

    int maxSteps = trianglesTexSize / 5;
    for(int i = 0; i < maxSteps; i++) {
        // TODO: test ray-triangle intersection only for pixels with positive normal values
        // in primary reflection.

        // triangle's vertices
        vec3 A = vec3(data[15 * i + 0], data[15 * i + 1], data[15 * i + 2]);
        vec3 B = vec3(data[15 * i + 3], data[15 * i + 4], data[15 * i + 5]);
        vec3 C = vec3(data[15 * i + 6], data[15 * i + 7], data[15 * i + 8]);

        // centroid
        vec3 K = vec3(data[15 * i + 9], data[15 * i + 10], data[15 * i + 11]);

        // normal
        vec3 N = vec3(data[15 * i + 12], data[15 * i + 13], data[15 * i + 14]);

        // TODO: test triangle intersection
        // hasIntersection = rayTriangleIntersection(ray, A, B, C, intersectedPoint);
    }

    vec4 myOut;
    return myOut;
}

void main() {
    // primary reflections by rasterization
    vec4 primaryRefl = primaryReflections();

    // secondary reflections by ray-tracing
    vec4 secondaryRefl = secondaryReflections(primaryRefl);

    // output data
    outData = primaryRefl;
}
