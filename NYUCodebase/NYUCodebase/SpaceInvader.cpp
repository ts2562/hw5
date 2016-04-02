#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include "SDL_opengles2.h"


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *image_path) {
	SDL_Surface *surface = IMG_Load(image_path);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, surface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(surface);
	return textureID;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);


	SDL_Event event;
	bool done = false;
#ifdef _WINDOWS
	glewInit();
#endif

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");



	GLuint dogTexture = LoadTexture("dog.png");
	GLuint ballTexture = LoadTexture("ball.png");
	GLuint boneTexture = LoadTexture("bone.png");

	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;

	double movement = 0.0;

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}


		glClear(GL_COLOR_BUFFER_BIT);
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		glBindTexture(GL_TEXTURE_2D, dogTexture);
		float ticks = (float)SDL_GetTicks() / 1000.0f;

		float elapsed = ticks - lastFrameTicks;

		lastFrameTicks = ticks;

		movement = movement + elapsed;

		float vertices[] = { -1.5 + (movement * 0.1), -0.5, -0.5 + (movement* 0.1), -0.5, -0.5 + (movement * 0.1), 0.5,
			-1.5 + (movement * 0.1), -0.5, -0.5 + (movement * 0.1), 0.5, -1.5 + (movement * 0.1), 0.5 };


		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);

		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);

		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		glDisableVertexAttribArray(program.texCoordAttribute);




		glBindTexture(GL_TEXTURE_2D, boneTexture);

		float vertices2[] = { -2.5, -0.5, -1.5, -0.5, -1.5, 0.5, -2.5, -0.5, -1.5, 0.5, -2.5, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);

		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);

		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		glDisableVertexAttribArray(program.texCoordAttribute);


		glBindTexture(GL_TEXTURE_2D, ballTexture);

		float vertices3[] = { 1.5, -0.5, 2.5, -0.5, 2.5, 0.5, 1.5, -0.5, 2.5, 0.5, 1.5, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);

		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords3[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords3);

		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
