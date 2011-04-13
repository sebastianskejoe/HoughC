void draw_circle(int x, int y, int radius, float color[3]) {
	float theta;
	float x0, y0, r;
	x0 = x;
	y0 = y;
	r = radius;

	glColor3f(color[0], color[1], color[2]);
	glBegin(GL_LINE_LOOP);
		for (theta = 0.0f; theta < 2*M_PI; theta += 0.01f) {
			glVertex2f(x0+r*cos(theta), y0+r*sin(theta));
		}
	glEnd();
}

void bindTexture(GLuint *texture, int type, int x, int y, void *data) {
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,x,y,0,GL_LUMINANCE,type,data);
}


void draw_image_texture(unsigned char *map, int x, int y, float minx, float miny, float maxx, float maxy) {
	GLuint tex;
	// Draw edge
	bindTexture(&tex, GL_UNSIGNED_BYTE, x, y, map);
	glBegin (GL_QUADS);
		glTexCoord2f (0.0, 1.0);
		glVertex2f(minx,miny);

		glTexCoord2f (1.0, 1.0);
		glVertex2f (maxx,miny);

		glTexCoord2f (1.0, 0.0);
		glVertex2f (maxx, maxy);

		glTexCoord2f (0.0, 0.0);
		glVertex2f (minx, maxy);
	glEnd ();
}
