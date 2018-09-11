#version 130

in vec3 worldPos;                   // xxxxxxxx
in vec3 worldNormal;                // xxxxxxxx
in vec3 cameraPos;                  // xxxxxxxx
in mat3 TBN;                        // xxxxxxxx

uniform bool drawNormal;            // xxxxxxxx
uniform bool drawDistance;          // xxxxxxxx
uniform float farPlane;             // xxxxxxxx
uniform float reflectance;          // xxxxxxxx
uniform float attenuationCoeff;     // xxxxxxxx

uniform sampler2D normalTex;        // xxxxxxxx
uniform bool useNormalTex;          // xxxxxxxx

uniform sampler2D trianglesTex;     // all triangles/meshes collected from the simulated scene
uniform vec3 trianglesTexSize;      // texture size of triangles

// ray definition
struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 inv_direction;
    int sign[3];
};

Ray makeRay(vec3 origin, vec3 direction) {
    vec3 inv_direction = vec3(1.0) / direction;
    return Ray(
        origin,
        direction,
        inv_direction,
        int[3] (int(inv_direction.x < 0.0),
                int(inv_direction.y < 0.0),
                int(inv_direction.z < 0.0)));
}

// triangle definition
struct Triangle {
    vec3 v0, v1, v2;    // vertices A, B and C
    vec3 center;        // centroid
    vec3 normal;        // normal
};

// box definition
struct Box {
    vec3 aabb[2];    // bottom left (vmin) and top right (vmax) vertices
};

// get data element from texture
float getTexData(sampler2D tex, int i, int j) {
    return texelFetch(tex, ivec2(i,j), 0).r;
}

// get triangle data from texture
Triangle getTriangleData(sampler2D tex, int idx) {
    Triangle triangle;
    triangle.v0     = vec3(getTexData(tex, idx, 0),     getTexData(tex, idx, 1),    getTexData(tex, idx, 2));
    triangle.v1     = vec3(getTexData(tex, idx, 3),     getTexData(tex, idx, 4),    getTexData(tex, idx, 5));
    triangle.v2     = vec3(getTexData(tex, idx, 6),     getTexData(tex, idx, 7),    getTexData(tex, idx, 8));
    triangle.center = vec3(getTexData(tex, idx, 9),     getTexData(tex, idx, 10),   getTexData(tex, idx, 11));
    triangle.normal = vec3(getTexData(tex, idx, 12),    getTexData(tex, idx, 13),   getTexData(tex, idx, 14));
    return triangle;
}

// Ray-AABB (axis-aligned bounding box) intersection algorithm
// source: https://bit.ly/2oCdyEP
bool rayIntersectsBox(Ray ray, Box box)
{
    float tmin  = (box.aabb[ray.sign[0]    ].x - ray.origin.x) * ray.inv_direction.x;
    float tmax  = (box.aabb[1 - ray.sign[0]].x - ray.origin.x) * ray.inv_direction.x;
    float tymin = (box.aabb[ray.sign[1]    ].y - ray.origin.y) * ray.inv_direction.y;
    float tymax = (box.aabb[1 - ray.sign[1]].y - ray.origin.y) * ray.inv_direction.y;
    float tzmin = (box.aabb[ray.sign[2]    ].z - ray.origin.z) * ray.inv_direction.z;
    float tzmax = (box.aabb[1 - ray.sign[2]].z - ray.origin.z) * ray.inv_direction.z;

    tmin = max(max(tmin, tymin), tzmin);
    tmax = min(min(tmax, tymax), tzmax);

    return (tmin < tmax);
}

// Möller–Trumbore ray-triangle intersection algorithm
// source: http://bit.ly/2MxnPMG
bool rayIntersectsTriangle(Ray ray, Triangle triangle)
{
    float EPSILON = 0.0000001;

    vec3 edge1, edge2, h, s, q;
    float a, f, u, v;
    edge1 = triangle.v1 - triangle.v0;
    edge2 = triangle.v2 - triangle.v0;

    h = cross(ray.direction, edge2);
    a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;

    f = 1 / a;
    s = ray.origin - triangle.v0;
    u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;

    q = cross(s, edge1);
    v = f * dot(ray.direction, q);
    if (v < 0.0 || (u + v) > 1.0)
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * dot(edge2, q);
    if (t <= EPSILON)   // this means that there is a line intersection but not a ray intersection.
        return false;

    return true;        // ray intersection
}

// ============================================================================================================================

// primary reflections: rasterization
vec4 primaryReflections() {
    vec3 worldIncident = cameraPos - worldPos;
    vec3 nWorldPos = normalize(worldIncident);
    vec3 nWorldNormal = normalize(worldNormal);

    // normal for textured scenes (by normal mapping)
    if (useNormalTex) {
        vec3 normalRGB = texture2D(normalTex, gl_TexCoord[0].xy).rgb;
        vec3 normalMap = (normalRGB * 2.0 - 1.0) * TBN;
        nWorldNormal = normalize(normalMap);
    }

    // material's reflectivity property
    if (reflectance > 0)
        nWorldNormal = min(nWorldNormal * reflectance, 1.0);

    // distance calculation
    float viewDistance = length(worldIncident);

    // attenuation effect of sound in the water
    nWorldNormal = nWorldNormal * exp(-2 * attenuationCoeff * viewDistance);

    // normalize distance using range value (farPlane)
    float nViewDistance = viewDistance / farPlane;

    // presents the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (nViewDistance <= 1) {
        if (drawDistance)   output.y = nViewDistance;
        if (drawNormal)     output.z = abs(dot(nWorldPos, nWorldNormal));
    }

    return output;
}

// ============================================================================================================================

// secondary reflections: ray-triangle intersection
vec4 secondaryReflections1(vec4 firstR) {

    // calculate the reflection direction for an incident vector
    // TODO: use the nWorldNormal after first reflection process: normal mapping, reflectivity and attenuation.
    vec3 worldIncident = cameraPos - worldPos;
    vec3 nWorldNormal = normalize(worldNormal);
    vec3 reflectedDir = reflect(-worldIncident, nWorldNormal);

    // set current ray
    Ray ray = makeRay(worldPos, reflectedDir);

    // perform ray-triangle intersection only for pixels with valid normal values
    vec4 output = vec4(0,0,0,1);
    if (firstR.z > 0) {

        // test ray-triangle intersection
        Triangle tri;
        bool intersected = false;
        for (int idx = 0; intersected == false, idx < trianglesTexSize.x; idx++) {
            tri = getTriangleData(trianglesTex, idx);
            intersected = rayIntersectsTriangle(ray, tri);
        }

        // if intersected, calculates the distance and normal values
        if (intersected) {
            // distance calculation
            float reverbDistance = length(ray.origin - tri.center);
            float nReverbDistance = reverbDistance / farPlane;

            // normal calculation
            vec3 nTrianglePos = normalize(cameraPos - tri.center);
            vec3 nTriangleNormal = normalize(tri.normal);

            // presents the normal and distance data as matrix
            if (nReverbDistance <= 1) {
                if (drawDistance)   output.y = nReverbDistance;
                if (drawNormal)     output.z = abs(dot(nTrianglePos, nTriangleNormal));
            }
        }
   }

    return output;
}

// secondary reflections: ray-box and ray-triangle intersection
// vec4 secondaryReflections2(vec4 firstR) {

//     // calculate the reflection direction for an incident vector
//     // TODO: use the nWorldNormal after first reflection process: normal mapping, reflectivity and attenuation.
//     vec3 worldIncident = cameraPos - worldPos;
//     vec3 nWorldNormal = normalize(worldNormal);
//     vec3 reflectedDir = reflect(-worldIncident, nWorldNormal);

//     // set current ray
//     Ray ray = makeRay(worldPos, reflectedDir);

//     // perform ray-triangle intersection only for pixels with valid normal values
//     vec4 output = vec4(0,0,0,1);
//     Box box;
//     if (firstR.z > 0) {

//         // test ray-box intersection
//         bool triangleIntersected = false;
//         for (int idxB = int(trianglesTexSize.x); triangleIntersected == false, idxB < int(trianglesTexSize.y - 1); idxB++) {
//             box.aabb[0] = getTriangleData(trianglesTex, idxB + 0).center;
//             box.aabb[1] = getTriangleData(trianglesTex, idxB + 1).center;

//             bool boxIntersected = rayIntersectsBox(ray, box);
//             if (boxIntersected) {
//                 // test ray-triangle intersection
//                 output = vec4(1,1,1,1);
//                 triangleIntersected = true;
//             }
//         }
//    }

//     return output;
// }
// ============================================================================================================================

void main() {
    // output: primary reflections by rasterization
    vec4 firstR = primaryReflections();

    // output: secondary reflections by ray-tracing
    // vec4 secndR = secondaryReflections1(firstR);
    // vec4 secndR = secondaryReflections1(firstR);

    gl_FragData[0] = firstR;
    // gl_FragData[0] = secndR;
}