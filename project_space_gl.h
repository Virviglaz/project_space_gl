#ifndef PROJECT_SPACE_GL
#define PROJECT_SPACE_GL

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

enum object_type {
	ALL,
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
	} pos;
	struct {
		float x,y,z;
	} speed;
	struct {
		float x,y,z;
	} accel;
	float weight;
};

struct sphere {	
	float radius;
	uint32_t slices, staks;
	char *name;
	uint32_t color;
};

struct object_list {
	enum object_type type;
	uint num;
	void *object;
	struct physic *physic;
	struct object_list *next;
};

pthread_t engine_start(pthread_mutex_t *ext_mutex);
struct object_list *get_object_list(void);
struct physic *get_screen_center(void);

#endif