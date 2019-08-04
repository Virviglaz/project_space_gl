#ifndef PROJECT_SPACE_GL
#define PROJECT_SPACE_GL

enum object_type {
	SPHERE,
};

#define BLACK	0x000000
#define WHITE	0xFFFFFF
#define RED	0xFF0000
#define LIME	0x00FF00
#define BLUE	0x0000FF
#define YELLOW	0xFFFF00
#define CYAN 	0x00FFFF
#define MAGENTA 0xFF00FF
#define SILVER	0xC0C0C0
#define GRAY	0x808080
#define MAROON	0x800000
#define OLIVE	0x808000
#define GREEN	0x008000
#define PURPLE	0x800080
#define TEAL	0x008080
#define NAVY	0x000080

struct physic {
	struct {
		float x,y,z;
	} speed;
	struct {
		float x,y,z;
	} accel;
	float weight;
};

struct sphere {
	float x,y,z;
	float radius;
	const char *name;
	uint32_t color;
	struct physic physic;
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