#ifndef HEDGEHOG_H
#define HEDGEHOG_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/glfw3.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/stb_image_aug.h"
#include "linmath.h"
#include "../../curtmap/src/curtmap.h"

#define HH_SHADERS_MAX 64
#define HH_PROGRAMS_MAX 64
#define HH_TEXTURES_MAX 1024
#define HH_MATERIALS_MAX 64

typedef struct Texture
{
	 /** OpenGL's texture ID as gotten from glGenTextures(). */
    GLuint id;
	
    /** The texture unit ordinal (NOT internal OpenGL code e.g. GL_TEXTURE0) currently in use for this texture. */
    GLuint unit;
    
    /** The byte buffer representing the decoded image data. */
	unsigned char * data;
    
    /** Texture width. */
    int width;
    
    /** Texture height. */
    int height;
	
	/** Texture components / channels. */
	int components;
} Texture;

typedef struct Mesh
{
	GLuint topology; //GL_TRIANGLES or whatever (see also Program)

	GLushort * index;
	GLfloat  * position;
	GLfloat  * texcoord;
	
	GLsizei indexCount;
	GLsizei positionCount;
	
	GLuint vao;
	GLuint sampler;
} Mesh;


typedef struct MeshInstances
{
	Mesh * mesh;
	GLsizei count;
	GLuint buffer;
	const GLvoid * data;
} MeshInstances;

//Materials are either treated explicitly or simply as the input interface + matching renderable information for a given ProgramPath
typedef struct Renderable
{
	Mesh * mesh;
	Texture * texture; //TODO later this should go on Material, which goes on in here?
	
	//Material materials[];
	//A Material consists of:
	//-a ShaderPath/Pipe, which consists of multiple shader Programs running in sequence
	//-the parameters needed to populate that pipe at each stage

	
	GLuint positionBuffer; //HACK, TODO should be in DynamicModel
	GLuint texcoordBuffer; //HACK, TODO should be in DynamicModel
} Renderable;

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
	
	//inputs:
	CurtMap attributesByName;
	CurtMap uniformsByName;
	
	//shader version
} Shader;

//NB this is only needed once we get to the stage where we determine if we can compact the pipeline more, since it will be built of functions
typedef struct ShaderFunction
{
	//arguments
	
	//return value

} ShaderFunction;

/*
//joins one shader's input to another's output... if appropriate
typedef struct ShaderPipe
{
	ShaderVariable from;
	ShaderVariable to;
	unsigned char status; //fixed / broken / warning
} ShaderPipe;
*/

typedef struct Attribute
{
	char name[8];
	char type[8];
	char typeBase[8]; //vec, mat or whatever
	char typeNumeric[8]; //float, int or whatever
	int components; //count
	int elements; //count of array size
	
	//qualifiers
	bool polarity; //in/out/inout
	GLuint location; //layout, e.g. layout(location = 3)
	GLuint index; //layout, e.g. layout(location = 3, index = 1) UNUSED
} Attribute;

typedef struct Uniform
{
	char name[8];
	char type[8];
	char typeBase[8]; //vec, mat or whatever
	char typeNumeric[8]; //float, int or whatever
	int components; //count
	int elements; //count of array size
	//qualifiers
	
	bool isTexture; //some special calls for textures: tex and sampler. TODO actually, this comes from reading the type
} Uniform;

typedef struct Program
{
	//contains information on a single shader pass
	//batching & instancing require *exactly* the same state per instance - this includes uniforms & attributes
	char * name;
	//char * version (copied from shaders' version, or later used to dictate this)
	
	GLuint id;
	GLuint topology; //GL_TRIANGLES or whatever (see also Mesh)
	
	Shader vertex;
	Shader geometry;
	Shader fragment;
	
	//inputs: as vertex shader's inputs?

	//output (frame or render) buffer
} Program;


typedef struct RenderPath
{
	//Tree * rootProgram; //we work backwards from the root, fulfilling each set of input dependencies till we reach the root and can conclude.
	
	//for multipass rendering.
	//a tree of programs, each with a defined input and output interface, that lead to a single program producing a final result
	//interfaces must be fulfilled on input end.
	//as for output end, we must put that out to some render target.

	//some inputs come from previous Programs; others from elsewhere. We need to figure out how that should be done.

} RenderPath;

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
	
	CurtMap renderPathsByName;
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
//void Renderer_instance(Program * program, GLuint vao, const GLfloat * matVP, const GLvoid * indices, int elementCount, int instanceCount, const GLvoid * instanceData);
void Renderer_instance(Program * program, MeshInstances * meshInstances, const GLfloat * matVP);
void Renderer_single(Program * program, GLuint vao, const GLfloat * matVP, const GLvoid * indices, int elementCount, const GLfloat * matM);


//void Matrix_setProjectionPerspective(mat4x4 matrix, float near, float far, float top, float right);

char* Text_load(char* filename);

void GLFW_errorCallback(int error, const char * description);
bool GLTool_isExtensionSupported(const char * extension); //redundant, see GLFW

#endif //HEDGEHOG_H