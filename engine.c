#include "project_space_gl.h"
#include <math.h>
#include <unistd.h>

#define DBG	printf("Done: %u\n", __LINE__);

static pthread_t thandle;
static struct object_list *objects = NULL;
extern pthread_mutex_t mutex;
extern bool is_active;
struct
{
	float G;
} fconst = { .G = 0.0001f, };


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
	float weight, const char *name, uint32_t color)
{
	struct sphere *sphere = malloc(sizeof(struct sphere));
	if (!sphere)
		return -ENOMEM;

	sphere->physic.pos.x = x;
	sphere->physic.pos.y = y;
	sphere->physic.pos.z = z;
	sphere->color = color;
	sphere->radius = r;
	sphere->name = name;
	sphere->physic.weight = weight;
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

static float distance2(struct physic *src, struct physic *dst)
{
	float dx = dst->pos.x - src->pos.x;
	float dy = dst->pos.y - src->pos.y;
	float dz = dst->pos.z - src->pos.z;

	return dx * dx + dy * dy + dz * dz;
}

static int do_impact_spheres(struct sphere *spere, struct sphere *reference)
{
	printf("Impact! %s and %s\n", spere->name, reference->name);
	return 0;
}

static int do_gravity_spheres(struct sphere *spere, struct sphere *reference)
{
	int res = 0;
	float acc, distance;
	if (spere == reference)
		return 0;

	distance = distance2(&spere->physic, &reference->physic);

	acc = -fconst.G * spere->physic.weight / distance;
	distance = sqrtf(distance);

	/* Check impact condition */
	if (distance < (spere->radius + reference->radius))
		res = do_impact_spheres(spere, reference);

	/* Gravity law */
	spere->physic.accel.x += acc * 
		(spere->physic.pos.x - reference->physic.pos.x) / distance;
	spere->physic.accel.y += acc * 
		(spere->physic.pos.y - reference->physic.pos.y) / distance;
	spere->physic.accel.z += acc * 
		(spere->physic.pos.z - reference->physic.pos.z) / distance;

	/* Calculate velocity */
	spere->physic.speed.x += spere->physic.accel.x;
	spere->physic.speed.y += spere->physic.accel.y;
	spere->physic.speed.z += spere->physic.accel.z;

	/* Move object */
	spere->physic.pos.x += spere->physic.speed.x;
	spere->physic.pos.y += spere->physic.speed.y;
	spere->physic.pos.z += spere->physic.speed.z;

	return res;
}

static int do_gravity(struct object_list *o1, struct object_list *o2)
{
	if (o1->type == SPHERE && o2->type == SPHERE)
		return do_gravity_spheres(o1->object, o2->object);

	return -EINVAL;
}

static int do_gravity_by_list(void)
{
	struct object_list *o1 = objects;
	int res = -ENODATA;

	while(o1->next) {
		struct object_list *o2 = objects;
		while(o2->next) {
			res = do_gravity(o1, o2);
			if (res)
				return res;
			o2 = o2->next;
		}
		o1 = o1->next;
	}
	return res;
}

static void *engine_thread(void *params)
{
	int res;
	add_sphere(0.5,0.5,0.5,0.1, 0.10, "Lunar1", RED);
	add_sphere(-0.5,-0.5,0,0.1, 0.10, "Lunar2", GREEN);
	add_sphere(0.0,0.5,0,0.1, 0.10, "Lunar3", BLUE);
	add_sphere(0.0,0.0,0,0.1, 0.10, "Lunar4", YELLOW);

	while(is_active) {
		res = do_gravity_by_list();
		usleep(100000);
	}
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