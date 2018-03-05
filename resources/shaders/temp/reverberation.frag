#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;

uniform sampler2D vertexMap;

out vec4 out_data;

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

void main() {
    vec3 nNormalEyeSpace = normalize(normalEyeSpace);

    vec3 nPositionEyeSpace = normalize(-positionEyeSpace);

    float linearDepth = sqrt(positionEyeSpace.x * positionEyeSpace.x +
                             positionEyeSpace.y * positionEyeSpace.y +
                             positionEyeSpace.z * positionEyeSpace.z);

    linearDepth = linearDepth / farPlane;

    // output the normal and depth data as matrix
    out_data = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawNormal) out_data.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
        if (drawDepth)  out_data.y = linearDepth;
    }

    gl_FragDepth = linearDepth;
}
