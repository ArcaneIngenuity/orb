#ifndef HEDGEHOG_H
#define HEDGEHOG_H

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/glfw3.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../src/stb/stb_image_aug.h"
#include "linmath.h"
#include "../../curt/list_generic.h"
#include "../../curt/map_generic.h"
#include "../../curt/intMap.h"
#include "../../curt/floatMap.h"

//FIXED CONSTANTS
#define X 0
#define Y 1
#define Z 2
#define POSITION_COMPONENTS 3
#define TEXCOORD_COMPONENTS 2
#define VERTICES_PER_TRIANGLE 3

#ifndef MESH_VERTICES_SHORT_MAX
#define MESH_VERTICES_SHORT_MAX 65536
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//USER CONSTANTS (define before inclusion of this header to override)

#ifndef	HH_SHADERS_MAX
#define HH_SHADERS_MAX 64
#endif

#ifndef	HH_PROGRAMS_MAX
#define HH_PROGRAMS_MAX 64
#endif

#ifndef	HH_RENDERTEXTURES_MAX
#define HH_RENDERTEXTURES_MAX 8
#endif

#ifndef	HH_TEXTURES_MAX
#define HH_TEXTURES_MAX 128
#endif

#ifndef	HH_TEXTURE_PARAMETERS_MAX
#define HH_TEXTURE_PARAMETERS_MAX 16
#endif

#ifndef	HH_MATERIALS_MAX
#define HH_MATERIALS_MAX 64
#endif

#ifndef	HH_RENDERABLES_MAX
#define HH_RENDERABLES_MAX 16384
#endif

#ifndef	HH_TEXTURES_RENDERABLE_MAX
#define HH_TEXTURES_RENDERABLE_MAX 16
#endif

#ifndef	HH_MESHES_MAX
#define HH_MESHES_MAX 256
#endif

#ifndef	HH_ATTRIBUTES_MAX
#define HH_ATTRIBUTES_MAX 8
#endif

#ifndef	HH_TRANSFORMS_MAX
#define HH_TRANSFORMS_MAX 1024
#endif

typedef struct BMFontInfo
{
	char * face;
	unsigned short size;
	bool bold;
	bool italic;
	char * charset;
	bool unicode;
	unsigned short stretchH;
	bool smooth;
	unsigned char aa;
	unsigned short paddingUp;
	unsigned short paddingRight;
	unsigned short paddingDown;
	unsigned short paddingLeft;
	unsigned short spacingH;
	unsigned short spacingV;
	unsigned short outline;
	
} BMFontInfo;
const struct BMFontInfo bmFontInfoEmpty;

typedef struct BMFontCommon
{
	unsigned short lineHeight;
	unsigned short base;
	unsigned short scaleW;
	unsigned short scaleH;
	unsigned char pages;
	bool packed;
	unsigned char alphaChnl;
	unsigned char redChnl;
	unsigned char greenChnl;
	unsigned char blueChnl;
} BMFontCommon;
const struct BMFontCommon bmFontCommonEmpty;

typedef struct BMFontCharacter
{
	unsigned short id; //used as index into bmFont.characters array.
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	unsigned short xoffset;
	unsigned short yoffset;
	unsigned short xadvance;
	unsigned char page;
	unsigned char chnl;
} BMFontCharacter;
const struct BMFontCharacter bmFontCharacterEmpty;


typedef struct BMFontKerning
{
	unsigned short first; //id
	unsigned short second; //id
	unsigned short amount;
} BMFontKerning;
const struct BMFontKerning bmFontKerningEmpty;

typedef struct BMFontPage
{
	unsigned char id;
	const char * file;
} BMFontPage;
const struct BMFontPage bmFontPageEmpty;

typedef struct BMFont
{
	BMFontInfo info;
	BMFontCommon common;
	
	BMFontPage pages[16];
	BMFontCharacter characters[256]; //use BMFontCharacter.id as array index.
	BMFontKerning kerningsByFirst[256]; //index by first id, matches second array
	BMFontKerning kerningsBySecond[256]; //index by second id, matches first array
} BMFont;
const struct BMFont bmFontEmpty;

//instance of the abstract, non-mesh-specific concept of a vertex attribute.
//there could be many ShaderAttributes which would work with a given Mesh Attribute, and vice versa, so no point linking these directly.
//the shader
typedef struct ShaderAttribute 
{
	char name[8]; //or instead, as key or numeric constant index?
	char type[8]; //vec2, mat4 etc.
	char typeCompound[8]; //vec, mat etc.
	char typeNumeric[8]; //float, int etc.
	int components; //2, 3, 16 etc.void * 

	//qualifiers
	bool polarity; //in/out/inout
	GLuint location; //layout, e.g. layout(location = 3)
	GLuint index; //layout, e.g. layout(location = 3, index = 1) UNUSED
} ShaderAttribute;

//Mesh Attribute is just raw data. It is up to ShaderPrograms (ShaderAttributes) to know what to do with it.
typedef struct Attribute 
{
	//count is in the Mesh holding the Attribute.
	void * vertex; //ShaderAttribute will know how to read it, depending on which attribute it is.
	GLuint id; //buffer id
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
	
	void * value;
	
	bool isTexture; //some special calls for textures: tex and sampler. TODO actually, this comes from reading the type
} Uniform;

typedef struct Texture
{
	char * name; //or as key?
    
    /** The byte buffer representing the decoded image data. */
	unsigned char * data;
    
    /** Texture width. */
    GLsizei width;
    
    /** Texture height. */
    GLsizei height;
	
	/** Texture components / channels. */
	GLsizei components;
	
	 /** OpenGL's texture ID as gotten from glGenTextures(). */
    GLuint id;
	
    /** The texture unit ordinal (NOT internal OpenGL code e.g. GL_TEXTURE0) currently in use for this texture. */
    GLuint unit;
	
	/** Did texels get modified / need upload? */
	bool changed;
	
	//atom="type", arrangedExternal="format", arrangedInternal="internalFormat" all in glTexImage2D()
	GLenum atomTypeExternal;
	GLenum arrangedExternal; //32 bit unsigned = same as GLuint
	GLint  arrangedInternal; //32 bit unsigned = same as GLenum
	
	GLenum dimensions; //glTexImage2D & glBindTexture "target"
	
	floatMap floatParametersByName;
	intMap intParametersByName;
	int 		intParameterValues[HH_TEXTURE_PARAMETERS_MAX];
	float 		floatParameterValues[HH_TEXTURE_PARAMETERS_MAX];
	uint64_t 	intParameterKeys[HH_TEXTURE_PARAMETERS_MAX];
	uint64_t 	floatParameterKeys[HH_TEXTURE_PARAMETERS_MAX];
} Texture;

typedef struct Face
{
	GLushort index[VERTICES_PER_TRIANGLE]; //into the owning Mesh's arrays: position, texcoord etc.
	GLfloat normal[VERTICES_PER_TRIANGLE];
} Face;

typedef struct Mesh
{
	GLuint topology; //GL_TRIANGLES or whatever (see also Program)

	//TODO bounds only require a single attribute. Maybe we should make this a pointer to an array? But Attribute is really small, points to data elsewhere.
	Attribute attribute[HH_ATTRIBUTES_MAX]; //also contains positions needed for faces
	Face * face;
	
	GLushort * index; //what we send to GPU.... TODO generate from face
	
	GLsizei indexCount;
	GLsizei vertexCount; //to be applied to each attribute
	GLsizei faceCount;
	
	GLuint vao;
	GLuint sampler;
	
	/** Did vertices get modified / need upload? */
	bool changed;
} Mesh;

//Renderable is a combination (typically unique) of some Mesh (vertex data) and some Material (shader + uniforms).
//Materials are either treated explicitly or simply as the input interface + matching renderable information for a given ProgramPath
typedef struct Renderable
{
	//occasional upload i.e. not performance-critical, so these objects can be pointers to structs.
	Mesh * mesh;
	Texture * textures[HH_TEXTURES_RENDERABLE_MAX]; 
	//TODO Material (with Texture)
	//Material materials[];
	//A Material consists of:
	//-a ShaderPath/Pipe, which consists of multiple shader Programs running in sequence
	//-the parameters needed to populate that pipe at each stage
} Renderable;
const struct Renderable renderableEmpty;

//TODO you will need to memcpy out the range of units used in a single instances draw call
//TODO make it use a particular Mesh, Material (with Texture) etc.
typedef struct RenderableSet
{
	Renderable * renderable;

	//we always need a contiguous buffer. So to keep things fast, client app *should*
	//memcpy out a contiguous block from a super-array: this means pre-sorting.
	GLsizei count;
	GLuint buffer;
	const GLvoid * data; //for now, model matrices.
} RenderableSet;

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
	Map attributesByName;
	Map uniformsByName;
	
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
	Texture * texture;
	//Map uniforms; //for referencing uniforms by name
	//Map attributes; //ref attribs by name
	
	//root of the graph from which we determine the assembled code for each class of shader.
	char * vertexShader;
	char * fragmentShader;
	char * geometryShader;

} Material;

typedef struct Camera
{
	vec3* lookingAt; //ongoing
	vec3* movingWith; //ongoing
	//TODO options that allow cinematic effects
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
	Map programsByName;
	Map shadersByName;
	Map texturesByName;
	Map materialsByName;
	//Map renderPathsByName;
	
	Map renderTexturesByName;
	
	//until we create merged map and list that contain not only pointers to arrays but also the arrays themselves... use these as backing arrays
	Shader   shaders[HH_SHADERS_MAX];
	Texture * textures[HH_TEXTURES_MAX];
	Texture * renderTextures[HH_RENDERTEXTURES_MAX];
	Material materials[HH_MATERIALS_MAX];
	Program  programs[HH_PROGRAMS_MAX];
	
	uint64_t shaderKeys[HH_SHADERS_MAX];
	uint64_t textureKeys[HH_TEXTURES_MAX];
	uint64_t rendertextureKeys[HH_RENDERTEXTURES_MAX];
	uint64_t materialKeys[HH_MATERIALS_MAX];
	uint64_t programKeys[HH_PROGRAMS_MAX];
	//TODO - these lists but not generic / pointer - int lists! then remove same fields from Renderables.
	//List changedMesh; //indices of Renderables that changed vertex data
	//List changedMaterials; //indices of Renderables that changed uniforms
	
	//should be in the order they are to be rendered in
	Renderable renderables[HH_RENDERABLES_MAX];
	
	Program * program; //the current shader program TODO should be an index into array? then use a getter to access. 
	
	//Transforms may not all have associated renderables...
	
	//TODO incorporate as separate RTT renderables array?
	Renderable fullscreenQuad;
	mat4x4 fullscreenQuadMatrix;
} Hedgehog;
const struct Hedgehog hedgehogEmpty;

void Hedgehog_initialise(Hedgehog * this);
Program * Hedgehog_setCurrentProgram(Hedgehog * this, char * name);
Program * Hedgehog_getCurrentProgram(Hedgehog * this);

void Mesh_calculateNormals(Mesh * this);

Texture * Texture_create();
Texture * Texture_load(const char * filename);
Texture * Texture_loadFromMemory(const char * filename);

GLenum Texture_getTextureUnitConstant(Texture * this);
void Texture_fresh(Texture * this);
void Texture_refresh(Texture * this);
void Texture_setTexelFormats(Texture * this, GLenum arranged, GLenum atomTypeExternal); //set both internal and external format
void Texture_setDimensionCount(Texture * this, GLenum dimensions);

int Texture_free(Texture * texture);

void * Texture_read2(int x, int y, void * texel);
void Texture_write2(int x, int y, void * texel);
void * Texture_read3(int x, int y, int z);
void Texture_write3(int x, int y, int z);

void RenderTexture_createDepth(Texture * const this, uint16_t width, uint16_t height);
void RenderTexture_createColor(Texture * this, GLuint i, uint16_t width, uint16_t height, GLenum format);

GLuint GLBuffer_create(
    GLenum target,
    GLsizei size,
    const void *data,
	GLenum usage
);

void Shader_load(Hedgehog * this, char * name);
void Shader_construct(Shader * this);
void Program_construct(Program * this, GLuint vertex_shader, GLuint fragment_shader); //we pass in a reference to a position in an already-defined array. This lets us keep our structures local.

void Hedgehog_clear();
void Hedgehog_renderSet(Program * program, RenderableSet * renderableSet, const GLfloat * matVP);
void Hedgehog_renderOne(Hedgehog * this, Renderable * renderable, const GLfloat * matM, const GLfloat * matVP);
void Hedgehog_createFullscreenQuad(Hedgehog * this, GLuint positionVertexAttributeIndex, GLuint texcoordVertexAttributeIndex);

//void Matrix_setProjectionPerspective(mat4x4 matrix, float near, float far, float top, float right);

char* Text_load(char* filename);

void GLFW_errorCallback(int error, const char * description);
bool GLTool_isExtensionSupported(const char * extension); //redundant, see GLFW

float Hedgehog_smoothstep(float t);

#endif //HEDGEHOG_H