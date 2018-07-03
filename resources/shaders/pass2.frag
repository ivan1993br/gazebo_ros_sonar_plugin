#version 130

uniform sampler2D trianglesTex;     // all triangles/meshes collected from the simulated scene
uniform sampler2D firstReflectionTex;     // first reflections by rasterization
uniform sampler2D originTex;        // incident vector of reflected surface
uniform sampler2D directionTex;     // direction vector of reflected surface
uniform vec2 textureSize;

// ray definition
struct Ray {
    vec3 origin;
    vec3 direction;
};

// triangle definition
struct Triangle {
    vec3 a;     // vertex A
    vec3 b;     // vertex B
    vec3 c;     // vertex C
    vec3 k;     // centroid
    vec3 n;     // normal
};

float getTexData(int i, int j) {
    return texelFetch(trianglesTex, ivec2(i,j), 0).r;
}

Triangle getTriangleData(int idx) {
    Triangle triangle;
    triangle.a = vec3(getTexData(idx,0), getTexData(idx,1), getTexData(idx,2));
    triangle.b = vec3(getTexData(idx,3), getTexData(idx,4), getTexData(idx,5));
    triangle.c = vec3(getTexData(idx,6), getTexData(idx,7), getTexData(idx,8));
    triangle.k = vec3(getTexData(idx,9), getTexData(idx,10), getTexData(idx,11));
    triangle.n = vec3(getTexData(idx,12), getTexData(idx,13), getTexData(idx,14));
    return triangle;
}

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

    // check ray intersection with plane, which is spanned by the triangle
    return rayPlaneIntersection(ray, buildPlane(a, b, c), intersectedPoint);
}

// secondary reflections: ray-triangle intersection
vec4 secondaryReflections(vec4 primaryRefl, vec4 origin, vec4 direction) {
    Ray ray;
    // ray.origin = directionEyeSpace;
    // ray.direction = reflectedDir;

    // test ray-triangle intersection only for pixels with valid normal values
    // from primary reflections
    vec4 output = vec4(0,0,0,0);
    // for(int i = 0; i < textureSize.x; i++) {
    //     for (int j = 0; j < textureSize.y; j++) {
    //         // check if normal value is positive
    //         float normal = fetchTexel(primaryRefl, ivec2(resolution.x, resolution.y), ivec2(i,j)).x;
    //         if (normal > 0) {
    //             if (test == false) output.z = 0.5;
    //             else output.z = 1.0;
    //             test = !test;
    //         }

    //     }
    // }



    // for (int idx = 0; idx < resolution.x; idx++) {
    //     for (int idy = 0; idy < resolution.y; idy++) {
    //         if ()
    //     }
    //    }



    // TODO: test ray-triangle intersection only for pixels with valid normal values
    // for primary reflections
    // for (int idx = 0; idx < texSize.x; idx++) {
    //     Triangle triangle = getTriangleData(idx);
    //
    //     // TODO: test triangle intersection
    //     // hasIntersection = rayTriangleIntersection(ray, A, B, C, intersectedPoint);
    // }

    return output;
}

void main() {
    // primary reflections by rasterization
    vec4 primaryRefl = texture2D(firstReflectionTex, gl_FragCoord.xy / textureSize);

    // origin/incident and direction vectors of reflected surfaces
    vec4 origin = texture2D(originTex, gl_FragCoord.xy / textureSize);
    vec4 direction = texture2D(directionTex, gl_FragCoord.xy / textureSize);

    // secondary reflections by ray-tracing
    vec4 secondaryRefl = secondaryReflections(primaryRefl, origin, direction);

    // TODO: outData = primary and secondary reflections
    gl_FragData[0] = primaryRefl;
    // gl_FragData[0] = secondaryRefl;
}