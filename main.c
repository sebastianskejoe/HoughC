#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glu.h>

#include <pthread.h>
#include <sys/time.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stb_image.h"
#include "helper.c"

unsigned char *data, *map, *edge, *hough, *angle;
int x, y, z, n;
int cx, cy, radius, max_hits;
int done;
char do_hough;

int thread_count = 1;
char threads_done = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_arg_struct {
	unsigned char *data;
	int start, rows, opt;
	int *x0, *y0, *r0, *max;
};

/*
Converts a color map of n components to a binary map
*/
void convert_to_bin(unsigned char *in, unsigned char *out) {
	int i, j;
	// Convert to binary map
	for (j = 0; j < y; j++) {
		for (i = 0; i < x; i++) {
			if (in[j*x*n+i*n] != 0) {
				out[j*x+i] = 255;
			}
		}
	}
}

/*
Detects edges using the sobel operator
*/
void edge_sobel(unsigned char *in, unsigned char *out) {
	int i, j, Gx, Gy, G;
	// Edge detect - sobel operator
	for (j = 0; j < y-3; j++) {
		for (i = 0; i < x-3; i++) {
			// Calculate Gy - convolution mask: [[1,2,1][0,0,0][-1,-2,-1]]
			Gy = in[j*x+i]+2*in[j*x+i+1]+in[j*x+i+2]-(in[(j+2)*x+i]+2*in[(j+2)*x+i+1]+in[(j+2)*x+i+2]);
			// Calculate absolute Gx - convolution mask: [[-1,0,1][-2,0,2][-1,0,1]]
			Gx = -in[j*x+i]+in[j*x+i+2]-2*in[(j+1)*x+i]+2*in[(j+1)*x+i+2]-in[(j+2)*x+i]+in[(j+2)*x+i+2];

			G = abs(Gy)+abs(Gx);
			
			// Clamp to [0,255]
			if (G >= 255) {
				out[j*x+i] = 255;
			} else {
				out[j*x+i] = 0;
			}
		}
	}
}

/*
The thread routine to calculate the circles centre
*/
void *thread_hough(void *arg) {
	struct thread_arg_struct *args = (struct thread_arg_struct*)arg;
	printf("Starting thread %lu - looping through %d rows, starting at %d\n", pthread_self(), args->rows, args->start);
	int *hough = malloc(sizeof(int)*x*y*z);
	memset(hough, 0, sizeof(int)*x*y*z);
	int max, j, i, ux, uy, xh, yh, r;

	max = 0;

	for (j = args->start; j < args->start+args->rows; j++) {
		for (i = 0; i < x; i++) {

		if (args->data[j*x+i]) {
	
			// This is a simple(read stupid) optimization - assumes that radii of circles is no bigger than the half of
			// the image size.
//			if (args->opt) {
				ux = x < i+x/2 ? x : i+x/2;
				uy = y < j+y/2 ? y : j+y/2;
//			} else {
//				ux = x;
//				uy = y;
//			}

			for (xh = 0; xh < ux; xh++) {
				for (yh = 0; yh < uy; yh++) {
					r = sqrt((i-xh)*(i-xh)+(j-yh)*(j-yh));

					// Maximum radius is z
					if (r < 0 || r >= z) {
						continue;
					}

					hough[yh*x*z+xh*z+r] += 1;

					// New max
					if (hough[yh*x*z+xh*z+r] > max) {
						max = hough[yh*x*z+xh*z+r];
						*(args->x0) = xh;
						*(args->y0) = yh;
						*(args->r0) = r;
						*(args->max) = max;
					}
				}
			}

		}
		}
	}

	free(hough);
	hough = 0;

	printf("Quiting thread %lu\n", pthread_self());
	pthread_exit(0);
}

/*
Key func
Spawns 'key' number of threads
*/
void key(unsigned char key, int kx, int ky) {
	if (key > '9' || key <= '0') {
		return;
	}

	int num = key-'0';
	int i;
	pthread_t threads[num];
	struct thread_arg_struct thread_args[num];
	time_t start, end;
	div_t div_s;

	start = time(0);
	div_s = div(y,num);

	for (i = 0; i < num; i++) {
		thread_args[i].data = edge;
		thread_args[i].x0 = malloc(sizeof(int));
		thread_args[i].y0 = malloc(sizeof(int));
		thread_args[i].r0 = malloc(sizeof(int));
		thread_args[i].max = malloc(sizeof(int));
		thread_args[i].start = i*div_s.quot;
		thread_args[i].rows = div_s.quot;

		pthread_create(&threads[i], 0, thread_hough, (void *)&thread_args[i]);
	}

	for (i = 0; i < num; i++) {
		pthread_join(threads[i], 0);
	}

	// Find max
	for (i = 0; i < num; i++) {
		if (*thread_args[i].max > max_hits) {
			max_hits = *(thread_args[i].max);
			cx = *(thread_args[i].x0);
			cy = *(thread_args[i].y0);
			radius = *(thread_args[i].r0);
		}
	}

	end = time(0);
	printf("Start: %d - End: %d - Diff: %d\n", (int)start, (int)end, (int)end-(int)start);
	glutPostRedisplay();
}

void draw() {

	if (!done) {
		convert_to_bin(data, map);
		edge_sobel(map, edge);
	}
	glLoadIdentity();

	// Draw map and hough
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);

	draw_image_texture(map, x, y, -1.0f, -1.0f, 0.0f, 1.0f);
	draw_image_texture(edge, x, y, 0.0f, -1.0f, 1.0f, 1.0f);
	

	glDisable(GL_TEXTURE_2D);
	
	glOrtho(0,x*2,-y,0,0,1);
	float color[3] = {1.0f,0.0f,0.0f};
	draw_circle(cx,-cy,radius, color);
	glColor3f(1.0f,1.0f,1.0f);

	glFlush();

}


int main(int argc, char *argv[]) {
	// Load image
	data = stbi_load("img2.png", &x, &y, &n, 0);
	if (!data) {
		return 0;
	}
	unsigned char *dat = data;

	max_hits = 0;

	z = x; // probably too much
	map = malloc(sizeof(unsigned char)*x*y);
	edge = malloc(sizeof(unsigned char)*x*y);
	angle = malloc(sizeof(float)*x*y);
	hough = malloc(sizeof(short)*x*y*z);

	memset(map, 0, sizeof(unsigned char)*x*y);
	memset(edge, 0, sizeof(unsigned char)*x*y);
	memset(angle, 0, sizeof(float)*x*y);
	memset(hough, 0, sizeof(short)*x*y*z);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowPosition(0,0);
	glutInitWindowSize(x*2, y);
	glutCreateWindow("Hough transform");

	glutDisplayFunc(draw);
	glutKeyboardFunc(key);

	glutMainLoop();

	pthread_exit(0);

	free(map);
	free(hough);
	free(edge);
	map = 0;
	hough = 0;
	edge = 0;
	data = dat;
	stbi_image_free(data);
	return 0;
}
