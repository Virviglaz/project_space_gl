#include <GL/glut.h>
#include "project_space_gl.h"
 
struct {
	 int width, height;
	 uint fefresh_mills;
} settings = { .fefresh_mills = 10, };

struct color_t {
	uint8_t r,g,b;
};

pthread_mutex_t mutex;
pthread_t engine_thread_handle;
double cx, cy, cz;
bool is_active = true;

static void update_screen_size(int width, int height)
{
	settings.width = width;
	settings.height = height;	 
}

static struct color_t *convert_color(uint32_t color)
{
	static struct color_t c;
	c.r = color >> 16;
	c.g = color >> 8;
	c.b = color;
	return &c;
}

static int draw_sphere(void *draw_object)
{
	struct object_list *object = draw_object;
	struct sphere *sphere = object->object;
	struct color_t *c = convert_color(sphere->color);

	glColor3d(c->r, c->g, c->b);
	glPushMatrix();
	//pthread_mutex_lock(&mutex);
	glTranslated(
		object->physic->pos.x - cx,
		object->physic->pos.y - cy,
		object->physic->pos.z - cz);

	glutSolidSphere(sphere->radius, sphere->slices, sphere->staks);
	//pthread_mutex_unlock(&mutex);
	glPopMatrix();
	return 0;
}

static int draw_object(enum object_type type, void *object)
{
	if (!object)
		return -EINVAL;
	switch (type)
	{
	case SPHERE:
		return draw_sphere(object);
	}
	return -EINVAL;
}
 
/* Initialize OpenGL Graphics */
static void initGL()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color to black and opaque
	glClearDepth(1.0f);                   // Set background depth to farthest
	glEnable(GL_DEPTH_TEST);   // Enable depth testing for z-culling
	glDepthFunc(GL_LEQUAL);    // Set the type of depth-test
	glShadeModel(GL_SMOOTH);   // Enable smooth shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Nice perspective corrections
}

static void display()
{
	struct object_list *object = get_object_list();
	int res = 0;

	if (!object)
		return;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	
	pthread_mutex_lock(&mutex);
	while (object->next)
	{
		res = draw_object(object->type, object);
		if (res)
			printf("Error: %d\n", res);
	
		object = object->next;
	}
	pthread_mutex_unlock(&mutex);
	
	glutSwapBuffers();
}
 
static void timer(int value)
{
	glutPostRedisplay();
	glutTimerFunc(settings.fefresh_mills, timer, 0);
}
 
static void reshape(GLsizei width, GLsizei height)
{
	// Compute aspect ratio of the new window
	if (height == 0) height = 1;                // To prevent divide by 0
	GLdouble aspect = (GLdouble)width / (GLdouble)height;
 
	// Set the viewport to cover the new window
	glViewport(0, 0, width, height);
 
	// Set the aspect ratio of the clipping volume to match the viewport
	glMatrixMode(GL_PROJECTION);  // To operate on the Projection matrix
	glLoadIdentity();             // Reset
	// Enable perspective projection with fovy, aspect, zNear and zFar
	gluPerspective(45.0f, aspect, 0.1f, 100.0f);
}
 
/* Main function: GLUT runs as a console application starting at main() */
int main(int argc, char** argv)
{
	if (pthread_mutex_init(&mutex, NULL)) {
		printf("Mutex create error!\n");
		goto err_mutex;
	}

	engine_thread_handle = engine_start(NULL);
	glutInit(&argc, argv);            // Initialize GLUT
	glutInitDisplayMode(GLUT_DOUBLE); // Enable double buffered mode
	glutInitWindowSize(1280, 960);   // Set the window's initial width & height
	glutInitWindowPosition(500, 250); // Position the window's initial top-left corner
	glutCreateWindow("Space");          // Create window with the given title
	glutDisplayFunc(display);       // Register callback handler for window re-paint event
	glutReshapeFunc(reshape);       // Register callback handler for window re-size event
	initGL();                       // Our own OpenGL initialization
	glutTimerFunc(0, timer, 0);     // First timer call immediately [NEW]
	glutMainLoop();                 // Enter the infinite event-processing loop
err_mutex:
	is_active = false;
	pthread_cancel(engine_thread_handle);
	//pthread_mutex_destroy(&mutex);
	pthread_join(engine_thread_handle, NULL);
	return 0;
}
