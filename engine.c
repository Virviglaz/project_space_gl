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
	float M;
	uint32_t D;
} fconst = { .G = 0.0001f, .M = 1000, .D = 20, };


static int add_object_to_list(enum object_type type, void *object,
	struct physic *physic)
{
	int res = 0;
	static uint num = 0;
	struct object_list *new_object;

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
	new_object->physic = physic;

	new_object->next = malloc(sizeof(struct object_list));	
	if (!new_object->next) {
		res = -ENOMEM;
		goto err;
	}
	new_object = new_object->next;
	new_object->next = NULL;
err:
	return res;
}

static int add_sphere(float x, float y, float z,
	float vx, float vy, float vz, float radius,
	float weight, const char *name, uint32_t color)
{
	struct physic *phy = malloc(sizeof(struct physic));
	struct sphere *sphere = malloc(sizeof(struct sphere));
	if (!sphere)
		return -ENOMEM;

	//printf("Adding object %s x=%.3f, y=%.3f, z=%.3f, vx=%.3f, vy=%.3f, vz=%.3f\n",
	//name, x, y, z, vx, vy, vz);

	phy->pos.x = x / fconst.M;
	phy->pos.y = y / fconst.M;
	phy->pos.z = z / fconst.M;
	phy->speed.x = vx / fconst.M;
	phy->speed.y = vy / fconst.M;
	phy->speed.z = vz / fconst.M;
	sphere->color = color;
	sphere->radius = radius / fconst.M;
	sphere->name = name;
	sphere->slices = 50;
	sphere->staks = 20;
	phy->weight = weight / fconst.M;
	return add_object_to_list(SPHERE, sphere, phy);
}

static struct object_list *find_object(void *object)
{
	struct object_list *listobject = objects;
	while(listobject->next)
		if (listobject->object == object)
			return listobject;

	return NULL;
}

static int remove_object(struct object_list *object)
{
	struct object_list *del_object = objects;
	struct object_list *pre_object;

	if (!object)
		return -EINVAL;
	
	while(del_object->next) {
		pre_object = del_object;
		del_object = del_object->next;
		if (del_object == object) {
			free(del_object->object);
			pre_object->next = del_object->next;
			return 0;
		}
	}
	return -ENODATA;
}

static int get_nof_objects(enum object_type type)
{
	int i = 0;
	struct object_list *object = objects;

	while(object->next)
		if (object->type == type || type == ALL)
			i++;
	return i;
}

static float distance2(struct physic *src, struct physic *dst)
{
	float dx = dst->pos.x - src->pos.x;
	float dy = dst->pos.y - src->pos.y;
	float dz = dst->pos.z - src->pos.z;

	return dx * dx + dy * dy + dz * dz;
}

static int do_impact_spheres(struct sphere *sphere, struct sphere *reference)
{
	int res;
	struct object_list *o_sphere = find_object(sphere);
	struct object_list *o_reference = find_object(sphere);
	printf("Impact! %s and %s\n", sphere->name, reference->name);
	res = remove_object(o_sphere);
	if (res)
		return res;
	res = remove_object(o_reference);

	return res;
}

static int do_gravity(struct object_list *object, struct object_list *reference)
{
	int res = 0;
	float acc, distance;
	if (object == reference)
		return 0;

	/* Calculate square distance */
	distance = distance2(object->physic, reference->physic);

	/* Gravity law */
	acc = -fconst.G * reference->physic->weight / distance;

	/* We had a distance in power of 2, but now we need sqrt */
	distance = sqrtf(distance);

	/* Check impact condition */
	//if (distance < (object->radius + reference->radius))
		//res = do_impact_spheres(object, reference);
	/* Calculate acceleration */
	object->physic->accel.x += acc *
		(object->physic->pos.x - reference->physic->pos.x) / distance;
	object->physic->accel.y += acc *
		(object->physic->pos.y - reference->physic->pos.y) / distance;
	object->physic->accel.z += acc *
		(object->physic->pos.z - reference->physic->pos.z) / distance;


	return res;
}

static int do_movement(struct physic *object)
{
	/* Calculate velocity */
	object->speed.x += object->accel.x;
	object->speed.y += object->accel.y;
	object->speed.z += object->accel.z;

	/* Move object */
	object->pos.x += object->speed.x;
	object->pos.y += object->speed.y;
	object->pos.z += object->speed.z;

	return 0;
}

static int do_gravity_by_list(void)
{
	struct object_list *o1 = objects;
	int res = -ENODATA;

	/* Reset all accelerations */
	while(o1->next) {
		struct sphere *p = o1->object;
		o1->physic->accel.x = 0;
		o1->physic->accel.y = 0;
		o1->physic->accel.z = 0;
		o1 = o1->next;
	}
	o1 = objects;

	/* Apply gravity for all */
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

static int do_movement_by_list(void)
{
	int res = -ENODATA;
	struct object_list *object = objects;
	if (!object)
		return res;

	while(object->next) {
		res = do_movement(object->physic);
		if (res)
			return res;
		object = object->next;
	}

	return res;
}

static void *engine_thread(void *params)
{
	int res = 0;
	add_sphere(0,300,0,	-10,0,0,	50, 1000, "Lunar1", RED);
	add_sphere(0,-300,0,	10,0,0,		50, 1000, "Lunar2", GREEN);
	//add_sphere(300,0,0,	0,-10,0,	30, 1000, "Lunar3", YELLOW);
	//add_sphere(-300,0,0,	0,0,0,		30, 1000, "Lunar4", BLUE);
	//add_sphere(0.3,0, -0.005,0,0, 0,0.01, 0.1, "Lunar3", BLUE);
	//add_sphere(-0.3,0,0, 0.005,0,0, 0.01, 0.1, "Lunar4", YELLOW);

	while(is_active) {
		usleep(fconst.D * 1000);

		pthread_mutex_lock(&mutex);
		res = do_gravity_by_list();
		if (res) {
			printf("Error do_gravity_by_list: %d\n", res);
			is_active = false;
		}

		res = do_movement_by_list();
		if (res) {
			printf("Error do_movement_by_list: %d\n", res);
			is_active = false;
		}
		pthread_mutex_unlock(&mutex);
	}
}

pthread_t engine_start(pthread_mutex_t *ext_mutex)
{
	if (pthread_create(&thandle, NULL, &engine_thread, ext_mutex))
		printf("Error creating thread!\n");
	return thandle;
}

struct object_list *get_object_list(void)
{
	return objects;
}