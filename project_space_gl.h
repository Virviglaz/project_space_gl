#ifndef PROJECT_SPACE_GL
#define PROJECT_SPACE_GL

enum object_type {
    SPHERE,
};

struct object {
    enum object_type type;
    float x,y,z;
    float weight;
    struct {
        float r,g,b;
    } color;
};

struct object_list {
    struct object object;
    struct object_list *next;
};

#endif