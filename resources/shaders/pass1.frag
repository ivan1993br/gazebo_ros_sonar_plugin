#version 130

in vec3 worldPos;
in vec3 worldNormal;
in vec3 cameraPos;
in mat3 TBN;

uniform float farPlane;
uniform bool drawNormal;
uniform bool drawDepth;                 // TODO: rename this uniform -> drawDistance
uniform float reflectance;
uniform float attenuationCoeff;

uniform sampler2D normalTex;
uniform bool useNormalTex;

uniform sampler2D trianglesTex;         // all triangles/meshes collected from the simulated scene
uniform vec2 trianglesTexSize;          // texture size of triangles

const int INT_MIN = -2147483648;

// ray definition
struct Ray {
    vec3 origin, direction;
};

// triangle definition
struct Triangle {
    vec3 v0;        // vertex A
    vec3 v1;        // vertex B
    vec3 v2;        // vertex C
    vec3 center;    // centroid
    vec3 normal;    // normal
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

// Calculate the closest triangle to the ray
Triangle nearest(vec3 nd) {

    Queue queue = createQueue();
    enqueue(queue, 0);
    int i = 0;

    bool first = true;
    Triangle best;
    float best_dist;

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
            return best;

        i = (i + 1) % 3;

        int idx_left  = 2 * idx + 1;
        int idx_right = 2 * idx + 2;
        if (dx > 0)
            enqueue(queue, idx_left);
        else
            enqueue(queue, idx_right);
    }
    return best;
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

    // Normal for textured scenes (by normal mapping)
    if (useNormalTex) {
        vec3 normalRGB = texture2D(normalTex, gl_TexCoord[0].xy).rgb;
        vec3 normalMap = (normalRGB * 2.0 - 1.0) * TBN;
        nWorldNormal = normalize(normalMap);
    }

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

// ============================================================================================================================

// secondary reflections: ray-triangle intersection
vec4 secondaryReflections(vec4 firstR) {

    // calculate the reflection direction for an incident vector
    // TODO: use the nWorldNormal after first reflection process: normal mapping, reflectivity and attenuation.
    vec3 worldIncident = cameraPos - worldPos;
    vec3 nWorldNormal = normalize(worldNormal);
    vec3 reflectedDir = reflect(-worldIncident, nWorldNormal);

    // set current ray
    Ray ray = Ray(worldPos, reflectedDir);

    // perform ray-triangle intersection only for pixels with valid normal values
    vec4 output = vec4(0,0,0,1);
    if (firstR.z > 0) {
        // find the closest triangle to the origin point
        Triangle triangle = nearest(ray.origin);

        // ray-triangle intersection
        bool intersected = rayIntersectsTriangle(ray, triangle);
        if (intersected) {
            output = vec4(1,1,1,1);
        }
   }

    return output;
}

// ============================================================================================================================

void main() {
    // output: primary reflections by rasterization
    vec4 firstR = primaryReflections();

    // output: secondary reflections by ray-tracing
    vec4 secndR = secondaryReflections(firstR);

    // gl_FragData[0] = firstR;
    gl_FragData[0] = secndR;
}