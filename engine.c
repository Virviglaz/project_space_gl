#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include "project_space_gl.h"

static pthread_t thandle;
static struct object_list *objects = NULL;
pthread_mutex_t *mutex;

static int add_object(enum object_type type, void *object)
{
	int res = 0;
	static uint num = 0;
	struct object_list *new_object;
	//pthread_mutex_lock(mutex);

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

	while(new_object->next)
		new_object = new_object->next;

	new_object->next = malloc(sizeof(struct object_list));	
	if (!new_object->next) {
		res = -ENOMEM;
		goto err;
	}
	
	new_object = new_object == objects ? new_object : new_object->next;
	new_object->type = type;
	new_object->object = object;
	new_object->num = ++num;
	new_object->next = NULL;

err:
	//pthread_mutex_unlock(mutex);
	return res;
}

static int add_sphere(float x, float y, float z, float r,
	const char *name)
{
	struct sphere *sphere = malloc(sizeof(struct sphere));
	if (!sphere)
		return -ENOMEM;

	sphere->x = x;
	sphere->y = y;
	sphere->z = z;
	sphere->radius = r;
	sphere->name = name;
	return add_object(SPHERE, sphere);
}

static void *engine_thread(void *params)
{
	mutex = (pthread_mutex_t *)params;
	//printf("Started %d!\n", __LINE__);
	//pthread_mutex_lock(mutex);
	//pthread_mutex_unlock(mutex);
	//printf("Started %d!\n", __LINE__);
	add_sphere(0.5,0.5,0.5,0.5, "Lunar");
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