#version 130

uniform sampler2D trianglesTex;         // all triangles/meshes collected from the simulated scene
uniform sampler2D firstReflectionTex;   // first reflections by rasterization
uniform sampler2D originTex;            // incident vector of reflected surface
uniform sampler2D directionTex;         // direction vector of reflected surface

uniform vec2 rttTexSize;                // texture size of RTTs
uniform vec2 trianglesTexSize;          // texture size of triangles

const int INT_MIN = -2147483648;

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
        float d = dist(root.k, nd);
        float dx = 0;
        switch (i) {
            case 0: dx = root.k.x - nd.x; break;
            case 1: dx = root.k.y - nd.y; break;
            case 2: dx = root.k.z - nd.z; break;
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

// secondary reflections: ray-triangle intersection
vec4 secondaryReflections() {
    Ray ray;

    vec4 primaryRefl = texture2D(firstReflectionTex, gl_FragCoord.xy / rttTexSize);
    vec4 origin = texture2D(originTex, gl_FragCoord.xy / rttTexSize);
    vec4 direction = texture2D(directionTex, gl_FragCoord.xy / rttTexSize);

    vec4 output = vec4(0,0,0,1);

    // perform ray-triangle intersection only for pixels with valid normal values
    if (primaryRefl.z > 0) {
        ray.origin = origin.xyz;
        ray.direction = direction.xyz;

        // find the closest triangle to the origin point
        Triangle best;
        float best_dist;
        nearest(ray.origin, best, best_dist);

        // ray-triangle intersection
        vec3 intersectedPoint;
        bool intersected = rayTriangleIntersection(ray, best.a, best.b, best.c, intersectedPoint);

        // TODO: compute the correct depth/normal values
        if (intersected) {
            output.y = 1;    // depth
            output.z = 1;    // normal
        }
    }

    return output;
}

void main() {
    // primary reflections by rasterization
    vec4 primaryRefl = texture2D(firstReflectionTex, gl_FragCoord.xy / rttTexSize);

    // secondary reflections by ray-tracing
    vec4 secondaryRefl = secondaryReflections();

    // TODO: outData = primary and secondary reflections
    gl_FragData[0] = primaryRefl;
    // gl_FragData[0] = secondaryRefl;
}