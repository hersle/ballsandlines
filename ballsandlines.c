#include <stdio.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <float.h>
#include <assert.h>

/* TODO: make bounce coeficcient affect only velocity normal to line */

#define BOUNCECOEFF 1.0
#define G 3.0

struct point {
	double x, y;
};

struct ball {
	struct point pos;
	double velx, vely;
};

struct line {
	struct point points[2];
};

GLFWwindow *win;
int nlines;
struct line lines[1000];
struct ball ball;

int npointsselected = 0;
struct line newline;
struct point worldpos;

double distsq(struct point p1, struct point p2) {
	return (p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y);
}

struct point get_cursor_world_pos() {
	struct point worldpos;
	glfwGetCursorPos(win, &worldpos.x, &worldpos.y);
	return worldpos;
}

struct point get_closest_point(struct point p) {
	int i, j;
	double mindistsq;
	struct point closest;

	assert(nlines > 0);
	mindistsq = DBL_MAX;
	for (i = 0; i < nlines; i++) {
		for (j = 0; j < 2; j++) {
			if (distsq(p, lines[i].points[j]) < mindistsq) {
				mindistsq = distsq(p, lines[i].points[j]);
				closest = lines[i].points[j];
			}
		}
	}
	return closest;
}

void key_cb(GLFWwindow *win, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
		if (action == GLFW_PRESS) {
			newline.points[npointsselected] = get_closest_point(worldpos);
		} else if (action == GLFW_RELEASE) {
			newline.points[npointsselected] = worldpos;
		}
	}
}

void mouse_button_cb(GLFWwindow *win, int button, int action, int mods) {
	if (action != GLFW_PRESS)
		return;

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		npointsselected++;
		if (npointsselected == 2) {
			lines[nlines++] = newline;
			npointsselected = 0;
		}

		/* force update cursor position */
		action = glfwGetKey(win, GLFW_KEY_LEFT_SHIFT);
		key_cb(win, GLFW_KEY_LEFT_SHIFT, 0, action, 0);
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		ball.pos = worldpos;
		ball.velx = ball.vely = 0.0;
	}
}

void cursor_pos_cb(GLFWwindow *win, double cursx, double cursy) {
	worldpos.x = cursx;
	worldpos.y = cursy;

	if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		newline.points[npointsselected] = get_closest_point(worldpos);
	} else {
		newline.points[npointsselected] = worldpos;
	}
}

void display() {
	int i;

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glLineWidth(1.0);
	glBegin(GL_LINES);
	glColor3d(1.0, 1.0, 1.0);
	for (i = 0; i < nlines; i++) {
		glVertex2d(lines[i].points[0].x, lines[i].points[0].y);
		glVertex2d(lines[i].points[1].x, lines[i].points[1].y);
	}
	glEnd();

	glPointSize(5.0);
	glColor3d(1.0, 0.0, 0.0);
	glBegin(GL_POINTS);
	glVertex2d(ball.pos.x, ball.pos.y);
	glEnd();

	glColor3d(0.0, 0.0, 1.0);
	glBegin(GL_POINTS);
	for (i = 0; i <= npointsselected; i++) {
		glVertex2d(newline.points[i].x, newline.points[i].y);
	}
	glEnd();

	if (npointsselected == 1) {
		glColor3d(0.0, 1.0, 0.0);
		glBegin(GL_LINES);
		glVertex2d(newline.points[0].x, newline.points[0].y);
		glVertex2d(newline.points[1].x, newline.points[1].y);
		glEnd();
	}

	glfwSwapBuffers(win);
}

void resize(GLFWwindow *win, int width, int height)
{
	/* make window and world coordinates equal */
    glLoadIdentity();
    glOrtho(0.0, (double) width, (double) height, 0.0, -1.0, 1.0);
    glViewport(0, 0, width, height);

	/* place one line on each side of the window */
	lines[0].points[0].x = lines[3].points[1].x = 0.0;
	lines[0].points[0].y = lines[3].points[1].y = 0.0;
	lines[0].points[1].x = lines[1].points[0].x = 0.0;
	lines[0].points[1].y = lines[1].points[0].y = (double) height;
	lines[1].points[1].x = lines[2].points[0].x = (double) width;
	lines[1].points[1].y = lines[2].points[0].y = (double) height;
	lines[2].points[1].x = lines[3].points[0].x = (double) width;
	lines[2].points[1].y = lines[3].points[0].y = 0.0;
}

double wedge(struct point p1, struct point p2, struct point p3) {
	/* calculates p1p2 x p1p3 */
	return (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);
}

double scalar3(struct point p1, struct point p2, struct point p3) {
	/* calculates p1p2 * p1p3 */
	return (p2.x - p1.x) * (p3.x - p1.x) + (p2.y - p1.y) * (p3.y - p1.y);
}

int will_collide(struct line l, double t) {
	/* returns whether the ball collides with the line within the given time */
	struct point newpos;
	double w1, w2, sp1, sp2;
	newpos.x = ball.pos.x + ball.velx * t;
	newpos.y = ball.pos.y + ball.vely * t;
	w1 = wedge(l.points[0], l.points[1], ball.pos);
	w2 = wedge(l.points[0], l.points[1], newpos);
	sp1 = scalar3(l.points[0], l.points[1], ball.pos);
	sp2 = scalar3(l.points[1], l.points[0], ball.pos);
	return (w1 * w2 <= 0.0) && sp1 >= 0.0 && sp2 >= 0.0;
}

void collide(struct line l) {
	double veltanx, veltany;
	double ldx, ldy;
	double sp;
	double linedistsq;

	ldx = l.points[1].x - l.points[0].x;
	ldy = l.points[1].y - l.points[0].y;
	sp = ball.velx * ldx + ball.vely * ldy;
	linedistsq = distsq(l.points[0], l.points[1]);
	veltanx = sp * ldx / linedistsq;
	veltany = sp * ldy / linedistsq;
	ball.velx = BOUNCECOEFF * (2.0 * veltanx - ball.velx);
	ball.vely = BOUNCECOEFF * (2.0 * veltany - ball.vely);
}

void update_state(double t) {
	int i, collided;

	ball.vely += G;

	// collided = 0;
	for (i = 0; i < nlines; i++) {
		if (will_collide(lines[i], t)) {
			collide(lines[i]);
			// collided = 1;
		}
	}

	ball.pos.x += ball.velx * t;
	ball.pos.y += ball.vely * t;
}

void init() {
	nlines = 4; /* one line on each side of the window */
	ball.pos.x = ball.pos.y = ball.velx = ball.vely = 0.0;
}

void play() {
	double time_prev, time_now;
	init();
	time_prev = glfwGetTime();
	while (!glfwWindowShouldClose(win)) {
		display();
		glfwPollEvents();
		time_now = glfwGetTime();
		update_state(time_now - time_prev);
		time_prev = time_now;
	}
}

int main() {
	glfwInit();
	win = glfwCreateWindow(400, 400, "ballsandlines", NULL, NULL);
	glfwMakeContextCurrent(win);
	glfwSetFramebufferSizeCallback(win, resize);
	glfwSetMouseButtonCallback(win, mouse_button_cb);
	glfwSetCursorPosCallback(win, cursor_pos_cb);
	glfwSetKeyCallback(win, key_cb);
	play();
	glfwDestroyWindow(win);
	glfwTerminate();
	return 0;
}
