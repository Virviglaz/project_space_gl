#ifndef PROJECT_SPACE_GL
#define PROJECT_SPACE_GL

enum object_type {
	SPHERE,
};

struct sphere {
	float x,y,z;
	float radius;
	const char *name;
	struct {
		float r,g,b;
	} color;
};

struct object_list {
	enum object_type type;
	void *object;
	uint num;
	struct object_list *next;
};

pthread_t engine_start(pthread_mutex_t *mutex);
struct object_list *get_object_list(void);

#endif