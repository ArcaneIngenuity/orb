#ifndef HEDGEHOG_H
#define HEDGEHOG_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/glfw3.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "../src/stb_image_aug.h"
#include "linmath.h"
#include "loader.h"
#include "../../curtmap/src/curtmap.h"

#define HH_SHADERS_MAX 64
#define HH_PROGRAMS_MAX 64
#define HH_TEXTURES_MAX 1024
#define HH_MATERIALS_MAX 64

typedef struct Texture
{
	 /** OpenGL's texture ID as gotten from glGenTextures(). */
    GLuint id;// = -1;
    
    /** The texture unit ordinal (NOT internal OpenGL code e.g. GL_TEXTURE0) currently in use for this texture. */
    //TODO may remove this as it may be better kept out at the view level?
    int unit;// = -1;
    
    /** The byte buffer representing the decoded image data. */
	unsigned char * data;
    
    /** Texture width. */
    int width;// = -1;
    
    /** Texture height. */
    int height;// = -1;
	
	/** Texture components / channels. */
	int components;
} Texture;

typedef struct Mesh
{
	GLushort index[180 * 180 * 4 * 3]; // DEV for now, make it the max size of a chunk! [TILES_ACROSS * TILES_ACROSS * TRIANGLES_PER_TILE * 3];
 	//GLushort id;
	GLfloat position[181*181*2*3]; //values per short = 2^16
	GLfloat textureCoordinate[181*181*2*3]; //values per short = 2^16
	
	GLsizei indexCount;
	GLsizei positionCount;
	
	GLuint vao;
	GLuint sampler_state;
} Mesh;

//both ShaderComponents will have these (duplicated) -- however we will check in against out and type against type
typedef struct ShaderVariable
{
	char * type; //for now, just use a string and check equality
	unsigned char inoutQualifier;
	unsigned char layoutQualifier;
} ShaderVariable;

typedef struct ShaderComponent
{
	ShaderVariable * inputs;
	ShaderVariable * outputs;//one shader's output is another's input - this is how they're linked
} ShaderComponent;

typedef struct Shader
{
	GLuint id;
	GLenum type; //(NOPE!) SHADER_VERTEX, _FRAGMENT, _GEOMETRY, _TESSELATION_CONTROL, _TESSELATION_EVALULATION
	const char * source;
} Shader;

//joins one shader's input to another's output... if appropriate
typedef struct ShaderPipe
{
	ShaderVariable from;
	ShaderVariable to;
	unsigned char status; //fixed / broken / warning
} ShaderPipe;

typedef struct Program
{
	//contains information on a single shader pass
	//batching & instancing require *exactly* the same state per instance - this includes uniforms & attributes
	
	GLuint id;
	
	//support all possible attributes and use only those necessary (composition).
	char * name;

} Program;

//for multipass rendering.
typedef struct ProgramSeries
{

} ProgramSeries;

typedef struct Material
{
	Program program;
	Texture texture;
	//Map uniforms; //for referencing uniforms by name
	//Map attributes; //ref attribs by name
	
	//root of the graph from which we determine the assembled code for each class of shader.
	char * vertexShader;
	char * fragmentShader;
	char * geometryShader;

} Material;

typedef struct Transform
{
	mat4x4 matrix;
	vec3 position;
	vec3 rotation;
	//quat rotation;
} Transform;
const struct Transform transformEmpty;

typedef struct Camera
{
	Transform transform;
	
	//quat quaternion;
	vec3* lookingAt; //ongoing
	vec3* movingWith; //ongoing
	//TODO other options that allow cinematic effects
	//Entity entity;
} Camera;
const struct Camera cameraEmpty;

typedef struct Color
{
	float r;
	float g;
	float b;
	float a;
} Color;

typedef struct Hedgehog
{
	int shaderCount;
	int textureCount;
	int materialsCount;
	int programCount;
	Shader shaders[HH_SHADERS_MAX];
	Texture textures[HH_TEXTURES_MAX];
	Material materials[HH_MATERIALS_MAX];
	Program programs[HH_PROGRAMS_MAX];
	CurtMap programsByName;
	CurtMap shadersByName;
	CurtMap texturesByName;
	CurtMap materialsByName;

	//current - necessary?
	//maybe if we wrap OpenGL call to say, Program_makeCurrent()
	//Program program;
	
} Hedgehog;
const struct Hedgehog hedgehogEmpty;

Texture * Texture_construct();
Texture * Texture_load(const char * filename);
Texture * Texture_loadFromMemory(const char * filename);

GLenum Texture_getTextureUnitConstant(Texture * this);
bool Texture_setParametersStandard(Texture * texture, int unit, bool repeat);
int Texture_free(Texture * texture);

void * Texture_read2(int x, int y, void * texel);
void Texture_write2(int x, int y, void * texel);
void * Texture_read3(int x, int y, int z);
void Texture_write3(int x, int y, int z);

GLuint GLBuffer_create(
    GLenum target,
    GLsizei size,
    const void *data,
	GLenum usage
);

void Shader_construct(Shader * this);
void Program_construct(Program * this, GLuint vertex_shader, GLuint fragment_shader); //we pass in a reference to a position in an already-defined array. This lets us keep our structures local.

void Renderer_clear();
void Renderer_instance(GLuint program, GLuint vao, const GLfloat * matVP, const GLvoid * instanceData, const GLvoid * indices, int elementCount, int instanceCount);


//void Matrix_setProjectionPerspective(mat4x4 matrix, float near, float far, float top, float right);

void GLFW_errorCallback(int error, const char * description);
bool GLTool_isExtensionSupported(const char * extension); //redundant, see GLFW

#endif //HEDGEHOG_H