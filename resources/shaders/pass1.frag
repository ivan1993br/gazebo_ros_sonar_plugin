#version 130

in vec3 positionEyeSpace;
in vec3 normalEyeSpace;
in vec3 positionWorldSpace;
in vec3 normalWorldSpace;
in mat3 TBN;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;
uniform float reflectance;
uniform float attenuationCoeff;

uniform sampler2D normalTex;
uniform bool useNormalTex;

uniform sampler2D trianglesTex;         // all triangles/meshes collected from the simulated scene
uniform vec2 trianglesTexSize;          // texture size of triangles

const int INT_MIN = -2147483648;
const float EPSILON = 0.001;

// ray definition
struct Ray {
    vec3 origin;        // starting point
    vec3 direction;     // ray direction
};

// triangle definition
struct Triangle {
    vec3 v0;            // vertex A
    vec3 v1;            // vertex B
    vec3 v2;            // vertex C
    vec3 center;        // centroid
    vec3 normal;        // normal
};

// queue definition
struct Queue {
    int front, rear, size;
    int array[64];
};

// function to create a queue of given capacity. It initializes size of queue as 0
Queue createQueue() {
    Queue queue;
    queue.size = 0;
    queue.front = 0;
    queue.rear = queue.array.length() - 1; // This is important, see the enqueue
    return queue;
}

// Queue is full when size becomes equal to the capacity
bool isFull(Queue queue) {
    return (queue.size == queue.array.length());
}

// Queue is empty when size is 0
bool isEmpty(Queue queue) {
    return (queue.size == 0);
}

// Function to add an item to the queue.  It changes rear and size
void enqueue(Queue queue, int item) {
    if (isFull(queue))
        return;
    queue.rear = (queue.rear + 1) % queue.array.length();
    queue.array[queue.rear] = item;
    queue.size = queue.size + 1;
}

// Function to remove an item from queue.  It changes front and size
int dequeue(Queue queue) {
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue.array[queue.front];
    queue.front = (queue.front + 1) % queue.array.length();
    queue.size = queue.size - 1;
    return item;
}

// Function to get front of queue
int front(Queue queue) {
    if (isEmpty(queue))
        return INT_MIN;
    return queue.array[queue.front];
}

// Function to get rear of queue
int rear(Queue queue) {
    if (isEmpty(queue))
        return INT_MIN;
    return queue.array[queue.rear];
}

float getTexData(int i, int j) {
    return texelFetch(trianglesTex, ivec2(i,j), 0).r;
}

Triangle getTriangleData(int idx) {
    Triangle triangle;
    triangle.v0     = vec3(getTexData(idx,0), getTexData(idx,1), getTexData(idx,2));
    triangle.v1     = vec3(getTexData(idx,3), getTexData(idx,4), getTexData(idx,5));
    triangle.v2     = vec3(getTexData(idx,6), getTexData(idx,7), getTexData(idx,8));
    triangle.center = vec3(getTexData(idx,9), getTexData(idx,10), getTexData(idx,11));
    triangle.normal = vec3(getTexData(idx,12), getTexData(idx,13), getTexData(idx,14));
    return triangle;
}

// Calculate the distance between two points
float dist(vec3 a, vec3 b) {
    vec3 c = a - b;
    return (c.x * c.x +
            c.y * c.y +
            c.z * c.z);
}

void nearest(vec3 nd, out Triangle best, out float best_dist) {

    Queue queue = createQueue();
    enqueue(queue, 0);
    int i = 0;

    bool first = true;

    while (!isEmpty(queue)) {
        int idx = dequeue(queue);

        if (idx > (trianglesTexSize.x - 1))
            continue;

        Triangle root = getTriangleData(idx);
        float d = dist(root.center, nd);
        float dx = 0;
        switch (i) {
            case 0: dx = root.center.x - nd.x; break;
            case 1: dx = root.center.y - nd.y; break;
            case 2: dx = root.center.z - nd.z; break;
        }

        if (first || d < best_dist) {
            best_dist = d;
            best = root;
            first = false;
        }

        // If chance of exact match is high
        if (best_dist == 0)
            return;

        i = (i + 1) % 3;

        int idx_left  = 2 * idx + 1;
        int idx_right = 2 * idx + 2;
        if (dx > 0)
            enqueue(queue, idx_left);
        else
            enqueue(queue, idx_right);
    }
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
    float linearDepth = length(positionEyeSpace);

    // Attenuation effect of sound in the water
    nNormalEyeSpace = nNormalEyeSpace * exp(-2 * attenuationCoeff * linearDepth);

    // Normalize depth using range value (farPlane)
    linearDepth = linearDepth / farPlane;
    gl_FragDepth = linearDepth;

    // presents the normal and depth data as matrix
    vec4 output = vec4(0, 0, 0, 1);
    if (linearDepth <= 1) {
        if (drawDepth)  output.y = linearDepth;
        if (drawNormal) output.z = abs(dot(nPositionEyeSpace, nNormalEyeSpace));
    }

    return output;
}

// // The Moller-Trumbore Algorithm
// vec3 triIntersect(Ray ray, Triangle tri, out Collision col) {
// 	vec3 org = ray.origin;
// 	vec3 dir = ray.direction;

// 	vec3 p0 = tri.v0;
// 	vec3 p1 = tri.v1;
// 	vec3 p2 = tri.v2;

// 	vec3 s0 = p1 - p0;
// 	vec3 s1 = p2 - p0;

// 	vec3 p = cross(dir, s1);
// 	float det = dot(s0, p);

// 	if (det <= 0.0) {
// 		return vec3(0, 0, 0);       // ray parallel to triangle
// 	}

// 	vec3 top0 = org - p0;
// 	float u = dot(top0, p) / det;

// 	if (u < 0.0 || u > 1.0) {       // outside the triangle
// 		return vec3(0, 0, 0);
// 	}

// 	vec3 q = cross(top0, s0);
// 	float v = dot(dir, q) / det;

// 	if (v < 0.0 || u + v > 1.0) {   // outside the triangle
// 		return vec3(0, 0, 0);
// 	}

//     // store data: distance
//     col.dist = length(ray.origin - tri.center) / farPlane;

//     // store data: normal
//     col.normal = tri.normal;

// 	return org + dir * dot(s1, q) / det;
// }

// // computes the intersection between the ray and triangle
// bool rayTriangleIntersection (Ray ray, Triangle tri) {
//     // get ray-triangle edges
//     vec3 pa = tri.v0 - ray.origin;
//     vec3 pb = tri.v1 - ray.origin;
//     vec3 pc = tri.v2 - ray.origin;

//     // check if ray direction cross the triangle ABC
//     intersectedPoint.x = dot(pb, cross(ray.direction, pc));
//     if (intersectedPoint.x < 0.0) return false;

//     intersectedPoint.y = dot(pc, cross(ray.direction, pa));
//     if (intersectedPoint.y < 0.0) return false;

//     intersectedPoint.z = dot(pa, cross(ray.direction, pb));
//     if (intersectedPoint.z < 0.0) return false;

//     return true;
// }

int compare(float a, float b) {
    if (a < b - EPSILON)
        return -1;
    else
        if (a > b + EPSILON)
            return 1;
        else
            return 0;
}

bool rayTriangleIntersection (Ray ray, Triangle tri) {
    float a = dot(tri.normal, tri.v0 - ray.origin);
    float b = dot(tri.normal, ray.direction);
    if (compare(b, 0.0) == 0)
        return false;

    float r = a / b;
    if (compare(r, 0.0) < 0)
        return false;

    return true;
}

// secondary reflections: ray-triangle intersection
vec4 secondaryReflections(vec4 firstR) {

    // calculate the reflection direction for an incident vector
    vec3 nNormalWorldSpace = normalize(normalWorldSpace);
    vec3 reflectedDir = reflect(positionWorldSpace, nNormalWorldSpace);

    // set the ray's origin and direction
    Ray ray;
    ray.origin = positionWorldSpace;
    ray.direction = reflectedDir;

    // perform ray-triangle intersection only for pixels with valid normal values
    vec4 output = vec4(0,0,0,1);

    if (firstR.z > 0) {
        // find the closest triangle to the origin point
        Triangle tri;
        float best_dist;
        nearest(ray.origin, tri, best_dist);

        // output.yz = vec2(1,1);

        // perform ray-triangle intersection
        if (rayTriangleIntersection(ray, tri)) {
        //     float linearDepth = length(ray.origin - tri.center) / farPlane;
        //     if (linearDepth <= 1) {
        //         if (drawDepth)  output.y = linearDepth;
        //         if (drawNormal) output.z = 1;
        //     }
        }
    }

    return output;
}

void main() {
    // output: primary reflections by rasterization
    vec4 firstR = primaryReflections();
    vec4 secndR = secondaryReflections(firstR);

    gl_FragData[0] = firstR;
    // gl_FragData[0] = secndR;
    // gl_FragData[0] = vec4(1,0,0,1);
}