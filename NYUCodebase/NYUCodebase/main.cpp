
#ifdef _WINDOWS
	#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include "SDL_opengles2.h"
#include "vector"
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

Matrix projM;
Matrix modM;
Matrix viewM;

Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;
ShaderProgram* prog;

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

class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int textureID1, float u1, float v1, float width1, float height1, float
		size1) : textureID(textureID1), u(u1), v(v1), width(width1), height(height1), size(size1){
	};
	void Draw(ShaderProgram *p);
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
	Matrix matrix;
};

void SheetSprite::Draw(ShaderProgram *program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };
	// draw our arrays
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

class Tile
{
public:
	vector<float> vecCord;
	vector<float> txtCord;
	GLuint textureID;
	Tile(GLuint txtID, vector<float>& vec, vector<float>& text) :textureID(txtID), vecCord(vec), txtCord(text){}
	void Draw(ShaderProgram* program, vector<float>& vec, vector<float>& text)
	{
		program->setModelMatrix(modM);
		program->setProjectionMatrix(projM);
		program->setViewMatrix(viewM);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vec.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, text.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}
};

class Entity {
public:
	Entity();
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	bool collidesWith(Entity *entity);
	SheetSprite sprite;
	float x;
	float y;
	float width;
	float height;
	float velocity_x;
	float velocity_y;
	float acceleration_x;
	float acceleration_y;
	bool isStatic;
	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;
	Matrix entityMove;
};

int mapWidth;
int mapHeight;

#define LEVEL_HEIGHT 32
#define LEVEL_WIDTH 100
#define TILE_SIZE 0.5f
#define SPRITE_COUNT_X 28
#define SPRITE_COUNT_Y 42
unsigned int** levelData;

bool readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height"){
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned int[mapWidth];
		}
		return true;
	}
}

bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned int val = (unsigned int)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	
	return true;
}

GLuint* tex;
float movePLS = 0.0f;
Matrix entityMove;
void placeEntity(string type, float x, float y)
{
	int index = 74;
	float u = (float)(((int)index) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	float v = (float)(((int)index) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
	float spriteWidth = 1.0 / (float)SPRITE_COUNT_X;
	float spriteHeight = 1.0 / (float)SPRITE_COUNT_Y;
	
	GLfloat texCoords[] = {
		u, v + spriteHeight + y,
		u + spriteWidth, v + y,
		u , v + y,
		u + spriteWidth, v + y,
		u, v + spriteHeight + y,
		u + spriteWidth, v + spriteHeight + y
	};
	
	Matrix* entityMove2 = &entityMove;
	//entityMove.Translate(-movePLS, 0, 0);


	float vertices[] = { -0.5f + movePLS + 2.0f, -0.5f - 12.0f, 0.5f + movePLS + 2.0f,
		0.5f - 12.0f, -0.5f + movePLS + 2.0f, 0.5f - 12.0f, 0.5f + movePLS + 2.0f, 0.5f - 12.0f, -0.5f + movePLS + 2.0f,
		-0.5f - 12.0f, 0.5f + movePLS + 2.0f, -0.5f - 12.0f };
	prog->setModelMatrix(modelMatrix);
	prog->setProjectionMatrix(projectionMatrix);
	prog->setViewMatrix(viewMatrix);
	glUseProgram(prog->programID);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glVertexAttribPointer(prog->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(prog->positionAttribute);
	glVertexAttribPointer(prog->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(prog->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6); //12 == # of elements in vertices(12/2)
	glDisableVertexAttribArray(prog->positionAttribute);
	glDisableVertexAttribArray(prog->texCoordAttribute);

	//cout << "it works" << endl;
	//cout << u << "            " << v  << endl;
}


bool readEntityData(std::ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
				float placeX = atoi(xPosition.c_str()) / 16 * TILE_SIZE;
			float placeY = atoi(yPosition.c_str()) / 16 * -TILE_SIZE;
			placeEntity(type, placeX, placeY); //draws entity
		}
	}
	return true;
}

void Draw(GLuint texture, ShaderProgram* program, vector<float>& vec, vector<float>& text)
{
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
			float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
			float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
			vec.insert(vec.end(), {
				TILE_SIZE * x, -TILE_SIZE * y,
				TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				TILE_SIZE * x, -TILE_SIZE * y,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
			});
			//cout << u << "            " << v << endl;  //0.5,0.14
			text.insert(text.end(), {
				u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),
				u, v,
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
			});
		}
	}
	
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);
	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, texture);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vec.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, text.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, vec.size() / 2);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void renderGame(){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	SDL_Event event;
	bool done = false;

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	prog = &program;

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	GLuint spriteSheetTexture = LoadTexture("spritesheet_rgba.png");
	tex = &spriteSheetTexture;

	glViewport(0, 0, 640, 360);
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);
	float lastFrameTicks = 0.0f;
	double movement = 0.0;
	double move = 0.0;
	double moveY = 0.0;
	ifstream infile("test3.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				return;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		
		else if (line == "[Object Layer 1]") {
		readEntityData(infile);
		}
		
	}
	
	vector<float> vertexData;
	vector<float> texCoordData;
	
	int texture = LoadTexture("sheet.png");
	//Tile world = Tile(spriteSheetTexture, vertexData, texCoordData);
	
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
		glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		
		modelMatrix.identity();
		modelMatrix.Translate(-3.75f, 12.0f, 0);
		viewMatrix.identity();
		viewMatrix.Translate(-move, -moveY, 0);
		Draw(spriteSheetTexture, &program, vertexData, texCoordData);
		placeEntity("hi", 0, 0);
		program.setModelMatrix(entityMove);
		modelMatrix.identity();
		modelMatrix.Translate(-move, 0, 0);

		if (keys[SDL_SCANCODE_A]) {
			move -= elapsed; //opposite when you move the camera
		}
		else if (keys[SDL_SCANCODE_D]) {
			move += elapsed;
		}

		else if (keys[SDL_SCANCODE_W]) {
			moveY += elapsed; //opposite when you move the camera
		}
		else if (keys[SDL_SCANCODE_S]) {
			moveY -= elapsed;
		}
		
		else if (keys[SDL_SCANCODE_I]) {
			movePLS -= elapsed; 
			move -= elapsed;
		}
		else if (keys[SDL_SCANCODE_P]) {
			movePLS += elapsed;
			move += elapsed;
		}
		
		SDL_GL_SwapWindow(displayWindow);
		}
		
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	
#ifdef _WIN32
	AllocConsole();
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
#endif
	cout << "hi" << endl;
		
	renderGame(); 
	return 0;
}
