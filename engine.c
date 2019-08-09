#include "project_space_gl.h"
#include <math.h>
#include <unistd.h>

#define DBG	printf("Done: %u\n", __LINE__);
#define MAX_NAME_SIZE	50

static pthread_t thandle;
static struct object_list *objects = NULL;
extern pthread_mutex_t mutex;
extern bool is_active;
extern double cx, cy, cz;
struct
{
	double G;
	double M;
	uint32_t D;
	uint32_t N;
} fconst = { .G = 0.0001f, .M = 1000, .D = 20, .N = 0, };

struct sphere default_sphere = { .radius = 30, .slices = 50, .staks = 20, };

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

static int add_sphere(double x, double y, double z,
	double vx, double vy, double vz, double radius,
	double weight, const char *name, uint32_t color)
{
	struct physic *phy = malloc(sizeof(struct physic));
	struct sphere *sphere = malloc(sizeof(struct sphere));

	if (!sphere)
		return -ENOMEM;

	phy->pos.x = x / fconst.M;
	phy->pos.y = y / fconst.M;
	phy->pos.z = z / fconst.M;
	phy->speed.x = vx / fconst.M;
	phy->speed.y = vy / fconst.M;
	phy->speed.z = vz / fconst.M;
	sphere->color = color;
	sphere->radius = radius / fconst.M;
	sphere->name = malloc(MAX_NAME_SIZE);
	strcpy(sphere->name, name);
	sphere->slices = default_sphere.slices;
	sphere->staks = default_sphere.staks;
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

static int get_object_num(struct object_list *object)
{
	int num = 0;
	struct object_list *listobject = objects;

	if (!objects || !object)
		return -EINVAL;

	while(listobject->next) {
		if (listobject == object)
			return num;
		num++;
		listobject = listobject->next;
	}

	return -ENODATA;	
}

static int remove_object(struct object_list *object)
{
	struct object_list *del_object = objects;
	struct object_list *pre_object;
	struct sphere *sphere;

	/* Delete first one */
	if (objects == object) {
		objects = objects->next;

	/* Find object */
	} else {
		while (del_object->next) {
			pre_object = del_object;
			del_object = del_object->next;
			if (del_object == object)
				break;
		}
		pre_object->next = del_object->next;
	}

	switch (del_object->type) {
	case SPHERE:
		sphere = del_object->object;
		free(sphere->name);
		break;
	}

	free(del_object->physic);
	free(del_object->object);

	return 0;
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

static double distance2(struct physic *src, struct physic *dst)
{
	double dx = dst->pos.x - src->pos.x;
	double dy = dst->pos.y - src->pos.y;
	double dz = dst->pos.z - src->pos.z;

	return dx * dx + dy * dy + dz * dz;
}

static int do_impact_spheres(struct object_list *object1, struct object_list *object2, double distance)
{
	struct sphere *sp1 = object1->object;
	struct sphere *sp2 = object2->object;
	struct object_list *new_object;

	struct physic *phy;
	struct sphere *sphere;

	if (sp1->radius + sp2->radius < distance)
		return 0; /* No impact, objects far */

	phy = malloc(sizeof(struct physic));
	sphere = malloc(sizeof(struct sphere));
	sphere->name = malloc(MAX_NAME_SIZE);
	sphere->color = WHITE;
	snprintf(sphere->name, MAX_NAME_SIZE - 1, "%s+%s", sp1->name, sp2->name);
	printf("Impact %s and %s\n", sp1->name, sp2->name);
	sphere->slices = default_sphere.slices;
	sphere->staks = default_sphere.staks;

	/* Just take the largest one */
	sphere->radius = sp1->radius > sp2->radius ? sp1->radius : sp2->radius;

	/* Calculate new positions */
	phy->pos.x = (object1->physic->pos.x + object2->physic->pos.x) / 2;
	phy->pos.y = (object1->physic->pos.y + object2->physic->pos.y) / 2;
	phy->pos.z = (object1->physic->pos.z + object2->physic->pos.z) / 2;

	/* Save pulses summary law */
	phy->weight = object1->physic->weight + object2->physic->weight;
	phy->speed.x = (object1->physic->weight * object1->physic->speed.x +
		object2->physic->weight * object2->physic->speed.x) / phy->weight;
	phy->speed.y = (object1->physic->weight * object1->physic->speed.y +
		object2->physic->weight * object2->physic->speed.y) / phy->weight;
	phy->speed.z = (object1->physic->weight * object1->physic->speed.z +
		object2->physic->weight * object2->physic->speed.z) / phy->weight;

	add_object_to_list(SPHERE, sphere, phy);

	remove_object(object1);
	remove_object(object2);

	return 1;
}

static int check_impact(struct object_list *object1, struct object_list *object2, double distance)
{
	if (object1->type == SPHERE && object2->type == SPHERE)
		return do_impact_spheres(object1, object2, distance);
	
	return 0;
}

static int do_gravity(struct object_list *object, struct object_list *reference)
{
	int res = 0;
	double acc, distance;
	if (object == reference)
		return 0;

	/* Calculate square distance */
	distance = distance2(object->physic, reference->physic);

	/* Gravity law */
	acc = -fconst.G * reference->physic->weight / distance;

	/* We had a distance in power of 2, but now we need sqrt */
	distance = sqrtf(distance);

	/* Check impact condition */
	if (check_impact(object, reference, distance))
		return res;

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

static struct physic *mass_center(void)
{
	static struct physic center;
	struct object_list *object = objects;

	center.pos.x = 0;
	center.pos.y = 0;
	center.pos.z = 0;
	center.weight = 0;

	while (object->next) {
		center.pos.x += object->physic->pos.x * object->physic->weight;
		center.pos.y += object->physic->pos.y * object->physic->weight;
		center.pos.z += object->physic->pos.z * object->physic->weight;
		center.weight += object->physic->weight;
		object = object->next;
	}

	center.pos.x /= center.weight;
	center.pos.y /= center.weight;
	center.pos.z /= center.weight;

	cx = center.pos.x;
	cy = center.pos.y;
	cz = center.pos.z;

	return &center;
}

static void *engine_thread(void *params)
{
	int res = 0;

	add_sphere(0,0,0,	0,0,0,		50, 1000, "Sun", RED);
	add_sphere(0,800,100,	10,0,0,		30, 100, "Earth", GREEN);
	add_sphere(0,880,100,	22,0,0,	10, 20, "Lunar 1", YELLOW);
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

		mass_center();

		pthread_mutex_unlock(&mutex);
	}
	pthread_exit(NULL);
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