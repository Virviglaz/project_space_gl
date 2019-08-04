#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include "project_space_gl.h"

static pthread_t thandle;
static struct object_list *objects = NULL;
extern pthread_mutex_t mutex;

static int add_object(enum object_type type, void *object)
{
	int res = 0;
	static uint num = 0;
	struct object_list *new_object;
	pthread_mutex_lock(&mutex);

	if (!objects) { /* Create first element */
		objects = malloc(sizeof(struct object_list));
		if (!object) {
			res = -ENOMEM;
			goto err;
		}
		objects->object = NULL;
		objects->num = 0;
		objects->next = NULL;
	}
	new_object = objects;

	/* Go to last object in list */
	while(new_object->next)
		new_object = new_object->next;

	new_object->type = type;
	new_object->object = object;
	new_object->num = ++num;
	new_object->next = NULL;

	new_object->next = malloc(sizeof(struct object_list));	
	if (!new_object->next) {
		res = -ENOMEM;
		goto err;
	}
	new_object = new_object->next;
	new_object->next = NULL;
err:
	pthread_mutex_unlock(&mutex);
	return res;
}

static int add_sphere(float x, float y, float z, float r,
	const char *name, uint32_t color)
{
	struct sphere *sphere = malloc(sizeof(struct sphere));
	if (!sphere)
		return -ENOMEM;

	sphere->x = x;
	sphere->y = y;
	sphere->z = z;
	sphere->color = color;
	sphere->radius = r;
	sphere->name = name;
	return add_object(SPHERE, sphere);
}

static int remove_object(uint num)
{
	struct object_list *del_object = objects;
	struct object_list *pre_object;

	if (!del_object)
		return -EINVAL;
	
	while(del_object) {
		if (del_object->num == num)
			break;
		del_object = del_object->next;
	}

	if (!del_object)
		return -ENODATA;
}

static void *engine_thread(void *params)
{
	add_sphere(0.5,0.5,0.5,0.7, "Lunar1", RED);
	add_sphere(-0.5,-0.5,0,0.3, "Lunar2", GREEN);
}

pthread_t engine_start(pthread_mutex_t *mutex)
{
	if (pthread_create(&thandle, NULL, &engine_thread, mutex))
		printf("Error creating thread!\n");
	return thandle;
}

struct object_list *get_object_list(void)
{
	return objects;
}