#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#	define M_PI 3.14159265359
#endif

typedef unsigned short Index;

typedef struct Vertex {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
} Vertex;

typedef struct Model {
	unsigned int indexCount;
	Index* indices;

	unsigned int frameCount;
	Vertex** frames;
} Model;

static GLFWwindow* window = NULL;
static Model model = { 0, NULL, 0, NULL };
static GLuint texture = 0;
static float blend = 0.0f;

void glPerspective(float fovyInDegrees, float aspectRatio, float znear, float zfar) {
	float ymax, xmax;
	ymax = znear * tanf(fovyInDegrees * (float)M_PI / 360.0f);
	xmax = ymax * aspectRatio;
	glFrustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

int LoadTexture(void) {
	float aniso;
	int w, h, c;
	stbi_uc* data = stbi_load("Anduin.png", &w, &h, &c, 4);
	if (!data) {
		return 0;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

	return 1;
}

int LoadModel(void) {
	unsigned int frame;
	FILE* file = fopen("Anduin.mdl", "rb");
	if (!file) {
		return 0;
	}

	fread(&model.indexCount, sizeof(model.indexCount), 1, file);
	model.indices = (Index*)malloc(model.indexCount * sizeof(Index));
	fread(model.indices, sizeof(Index), model.indexCount, file);

	fread(&model.frameCount, sizeof(model.frameCount), 1, file);
	model.frames = (Vertex**)malloc(sizeof(Vertex*) * model.frameCount);

	for (frame = 0; frame < model.frameCount; ++frame) {
		unsigned int vertexCount;
		fread(&vertexCount, sizeof(vertexCount), 1, file);
		model.frames[frame] = (Vertex*)malloc(vertexCount * sizeof(Vertex));
		fread(model.frames[frame], sizeof(Vertex), vertexCount, file);
	}

	return 1;
}

int Initialize(void) {
	if (!glfwInit()) {
		return 0;
	}

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(1280, 720, "AnimDemo", NULL, NULL);
	if (!window) {
		return 0;
	}

	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) {
		return 0;
	}

	if (!LoadTexture()) {
		return 0;
	}

	if (!LoadModel()) {
		return 0;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPerspective(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, -1.0f, -2.0f);

	return 1;
}

int Release(void) {
	if (model.indexCount > 0) {
		free(model.indices);
	}

	if (model.frameCount > 0) {
		unsigned int frame;
		for (frame = 0; frame < model.frameCount; ++frame) {
			free(model.frames[frame]);
		}
		free(model.frames);
	}

	if (texture > 0) {
		glDeleteTextures(1, &texture);
	}

	if (window) {
		glfwDestroyWindow(window);
	}

	glfwTerminate();
	return 0;
}

void Update(float deltaTime) {
	blend += deltaTime * 0.75f;
	while (blend >= 1.0f) {
		blend -= 1.0f;
	}

}

void Render(void) {
	unsigned int index;
	unsigned int frame0, frame1;
	float t;
	Vertex* vertex0, *vertex1;

	glLoadIdentity();
	glTranslatef(0.0f, -0.5, -2.0f);
	glScalef(0.01f, 0.01f, 0.01f);
	glRotatef(-45.0f, 0.0f, 1.0f, 0.0f);

	glBegin(GL_TRIANGLES);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	t = blend * model.frameCount;

	frame0 = (unsigned int)floorf(blend * model.frameCount);
	frame1 = (unsigned int)ceilf(blend * model.frameCount);

	t -= floorf(t);


	if (frame1 >= model.frameCount) {
		frame1 = 0;
	}


	for (index = 0; index < model.indexCount; ++index) {
		vertex0 = &model.frames[frame0][model.indices[index]];
		vertex1 = &model.frames[frame1][model.indices[index]];

		float x = vertex0->x + (vertex1->x - vertex0->x) * t;
		float y = vertex0->y + (vertex1->y - vertex0->y) * t;
		float z = vertex0->z + (vertex1->z - vertex0->z) * t;

		glTexCoord2f(vertex0->u, 1.0f - vertex0->v);
		glVertex3f(x, y, z);
	}

	glEnd();
}

void Run() {
	double lastTime, currTime, acc;
	
	lastTime = glfwGetTime();
	acc = 0.0;

	while (!glfwWindowShouldClose(window)) {
		currTime = glfwGetTime();
		acc += (currTime - lastTime);
		lastTime = currTime;

		while (acc >= 1.0 / 60.0) {
			Update(1.0f / 60.0f);

			acc -= 1.0 / 60.0;
		}

		glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Render();

		glfwSwapBuffers(window);

		glfwPollEvents();
	}
}

int main(int argc, char** argv) {
	if (Initialize()) {
		Run();
	}
	return Release();
}

