#ifndef COM_ARCANEINGENUITY_ORB_H
#define COM_ARCANEINGENUITY_ORB_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include "../log/log.h"
#include "linmath.h"
#include "../klib/kvec.h"

//KHASH...
#include "../klib/khash.h"
// setup khash for key/value types
// shorthand way to get the key from hashtable or defVal if not found
#define kh_get_val(kname, hash, key, defVal) ({k=kh_get(kname, hash, key);(k!=kh_end(hash)?kh_val(hash,k):defVal);})

// shorthand way to set value in hash with single line command.  Returns value
// returns 0=replaced existing item, 1=bucket empty (new key), 2-adding element previously deleted
#define kh_set(kname, hash, key, val) ({int ret; k = kh_put(kname, hash,key,&ret); kh_value(hash,k) = val; ret;})

#include "../ezxml/ezxml.h"

static const int StrInt = 33;
static const int IntInt = 34;
static const int IntFloat = 35;

typedef uint16_t Index;

//KHASH_DECLARE
KHASH_DECLARE(StrInt, kh_cstr_t, int)
KHASH_DECLARE(IntInt, khint32_t, int)
KHASH_DECLARE(IntFloat, khint32_t, float)
#ifndef KH_DECL_STRPTR
#define KH_DECL_STRPTR
static const int StrPtr = 36;
KHASH_DECLARE(StrPtr, kh_cstr_t, uintptr_t)
#endif//KH_DECL_STRPTR
#ifndef KH_DECL_STR_ATTRIBUTELOCATION
#define KH_DECL_STR_ATTRIBUTELOCATION
struct AttributeLocation;
static const int Str_AttributeLocation = 40;
KHASH_DECLARE(Str_AttributeLocation, kh_cstr_t, struct AttributeLocation)
#endif//KH_DECL_STR_ATTRIBUTELOCATION

#ifdef _WIN32
	//define something for Windows (32-bit and 64-bit, this part is common)
	#define DESKTOP 1
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef _WIN64
		//define something for Windows (64-bit only)
	#endif
#elif __APPLE__
	#include "TargetConditionals.h"
	#if TARGET_OS_IPHONE
		#define MOBILE 1
		#if TARGET_IPHONE_SIMULATOR
		#else //actual device
		#endif
		// iOS Simulator
	#elif TARGET_OS_MAC
		// Mac OS
		#define DESKTOP 1
	#else
		// Unsupported platform
	#endif
#elif __ANDROID__
	#include <android/sensor.h>
	//#include <android/log.h>
	#include "android_native_app_glue.h"

	#include <android/api-level.h>
	
	#define MAIN void android_main(struct android_app* app)
	
	#define MOBILE 1 //really? you don't know that this implies mobile. maybe we really should avoid these categories.
	#include <jni.h>
	#include <errno.h>
	#include <math.h>

	#include <dlfcn.h> //for dynamic linking of extensions	
	
	#include <EGL/egl.h>
	
	#define GL_GLEXT_PROTOTYPES 1
	//#include <GLES/glext.h>
	#include <GLES2/gl2ext.h>
	//#include <GLES/gl.h> //for e.g. GL_PERSPECTIVE_CORRECTION_HINT
	#include <GLES2/gl2.h>

	//#define glGenVertexArrays GenVertexArraysOES
	//#define glBindVertexArray BindVertexArrayOES
	

	//TODO API level checks via __ANDROID_API__
	// 1.0 	1
	// 1.1 	2
	// 1.5 	3
	// 1.6 	4
	// 2.0 	5
	// 2.0.1 	6
	// 2.1 	7
	// 2.2 	8
	// 2.3 	9
	// 2.3.3 	10
	// 3.0 	11
	
	//"polyfills"
	#define log2(value) (log(value) / log(2))
	
	//OpenGL ES Extensions
	PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays;
	PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray;
	PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArrays;
	PFNGLISVERTEXARRAYOESPROC glIsVertexArray;

#elif __linux
	#define DESKTOP 1 //not certain about this, but should be true...?
	// linux
#elif __unix // all unices not caught above
	// Unix
#elif __posix
	// POSIX
#endif

#ifdef DESKTOP
	#define MAIN int main(int argc, char *argv[])
	
	#define GLEW_STATIC
	#include "glew/glew.h"
	#include "glfw/glfw3.h"

	//#include "stb_image_aug.h"
	#define STB_IMAGE_STATIC 1
	#define STB_IMAGE_IMPLEMENTATION
	#include "stb_image.h"
	#undef STB_IMAGE_IMPLEMENTATION
	
#endif//DESKTOP

#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#define GL_RG 0x8227
#define GL_RED 0x1903
//#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
//#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF

//FIXED CONSTANTS
#define NEG 0
#define POS 1

#define XX 0
#define YY 1
#define ZZ 2
#define PRESSURE 2 //touch

//multitouch channels
#define XX0 0
#define YY0 1
#define PRESSURE0 2
#define XX1 3
#define YY1 4
#define PRESSURE1 5
#define XX2 6
#define YY2 7
#define PRESSURE2 8
#define XX3 9
#define YY3 10
#define PRESSURE3 11

//channel state/delta index
#define CURRENT 0
#define PREVIOUS 1

#define POSITION_COMPONENTS 3
#define TEXCOORD_COMPONENTS 2
#define VERTICES_PER_TRIANGLE 3
#define BITS_IN_BYTE 8

#ifndef MESH_VERTICES_SHORT_MAX
#define MESH_VERTICES_SHORT_MAX 65536 - 1 //-1 = compatibility for primitive restart, although only available in 3.0/ES 3.0
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

#define STRLEN_MAX 64

typedef struct Window
{
	#ifdef DESKTOP
	GLFWwindow * window;
	#else //DEV, ANDROID
		#ifdef __ANDROID__
	ANativeWindow * window;
		#endif //__ANDROID__
	#endif //DESKTOP
} Window;
//TODO set up other functions to wrap the different window types' provision of information e.g. resolution

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

typedef struct BMFontKerning
{
	unsigned short first; //id
	unsigned short second; //id
	unsigned short amount;
} BMFontKerning;

typedef struct BMFontPage
{
	unsigned char id;
	const char * file;
} BMFontPage;

typedef struct BMFont
{
	BMFontInfo info;
	BMFontCommon common;
	
	BMFontPage pages[16];
	BMFontCharacter characters[256]; //use BMFontCharacter.id as array index.
	BMFontKerning kerningsByFirst[256]; //index by first id, matches second array
	BMFontKerning kerningsBySecond[256]; //index by second id, matches first array
} BMFont;

//Mesh Attribute is just raw data. It is up to ShaderPrograms (ShaderAttributes) to know what to do with it.
typedef struct Attribute 
{
    GLuint index; //attribute location
	GLint components; //"size" in glVertexAttribPointer 
	GLenum type; //as glVertexAttribPointer
	GLboolean normalized; //as glVertexAttribPointer
} Attribute;

typedef void (*glUniformVectorFunction)(GLint location, GLsizei count, const GLint * value);
extern glUniformVectorFunction glUniformVectorFunctions[4][2];

typedef void (*glUniformMatrixFunction)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value);
extern glUniformMatrixFunction glUniformMatrixFunctions[4][4][2];

enum UniformTypeNumeric
{
	UniformFloat,
	UniformInt
	//UniformUInt, //not in ES 2.0
};

enum UniformType
{
	UniformVector,
	UniformMatrix,
	//UniformMatrixTransposed, //to place boolean matrixTranspose in Uniform - more compact/efficient
	UniformTexture
};

//TODO lists of Uniforms should stand alone, and should not necessarily run every frame:
//be able to run whenever calling code sees fit,
//whether that be once in a blue moon, once every frame for a group of renderables, or once
//every frame for each distinct renderable (based on its state).
typedef struct Uniform
{
	const char * name;
	enum UniformType type;
	enum UniformTypeNumeric typeNumeric; //float, int or whatever - applies only if type != texture
	GLsizei componentsMajor; //count major (for matrices and vectors)
	GLsizei componentsMinor; //count minor (for matrices only)
	GLsizei elements; //count of array size
	
	GLvoid * values; //"values" since many glUniform* are non-scalar
	
	//TODO should make UniformType contain 
	GLboolean matrixTranspose; //for glUniformMatrix calls - applies only if type == matrix
	//GLint location; //cached from glgetUniformLocation
} Uniform;

typedef struct Texture
{
	char name[64]; //or as key?
    
    /** The byte buffer representing the decoded image data. */
	uint8_t * data;
    
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
	
	//TODO rename to ...byPname (as https://www.khronos.org/opengles/sdk/docs/man/xhtml/glTexParameter.xml)
	khash_t(IntFloat) * floatParametersByName;
	khash_t(IntInt) * intParametersByName;

} Texture;


typedef struct TextureAtlasEntry
{
	//char name[STRLEN_MAX];
	char * name;
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	
	//if trimmed / cropped
	uint16_t xTrim; ///< The offset from x at which the sprite *actually* begins.
	uint16_t yTrim; ///< The offset from y at which the sprite *actually* begins.
	uint16_t wPreTrim; ///< The width before trim.
	uint16_t hPreTrim; ///< The height before trim.
	
	float xPivot; ///< x pivot as alpha value (0.0->1.0) across width
	float yPivot; ///< y pivot as alpha value (0.0->1.0) across height
	
} TextureAtlasEntry;

#ifndef KH_DECL_Str_TextureAtlasEntry
#define KH_DECL_Str_TextureAtlasEntry
static const int Str_TextureAtlasEntry = 1000;
KHASH_DECLARE(Str_TextureAtlasEntry, kh_cstr_t, TextureAtlasEntry)
#endif//KH_DECL_Str_TextureAtlasEntry

typedef struct TextureAtlas
{
	khash_t(Str_TextureAtlasEntry) * entriesByName;
	uint16_t w;
	uint16_t h;
	
} TextureAtlas;


typedef struct Triangle
{
	GLushort index[VERTICES_PER_TRIANGLE]; //into the owning Mesh's arrays: position, texcoord etc.
	GLfloat normal[VERTICES_PER_TRIANGLE];
} Triangle;

typedef struct Mesh
{	
	GLuint id; //buffer id
	GLuint topology; //GL_TRIANGLES or whatever (see also Program)
	GLenum usage; //as  used by glBufferData for all Attributes
	GLsizei stride; //as glVertexAttribPointer - unified for the interleaved array
	//TODO bounds only require a single attribute. Maybe we should make this a pointer to an array? But Attribute is really small, points to data elsewhere.
	Attribute attribute[HH_ATTRIBUTES_MAX]; //also contains positions needed for faces
	kvec_t(Attribute *) attributeActive; //we could of course als just have a null pointer for non-active attributes
	
	GLushort * index; //what we send to GPU.... TODO generate from face
	GLsizei indexCount;
	
	void * vertexArray; //ShaderAttribute will know how to read it, depending on which attribute it is.
	GLsizeiptr vertexBytes; //"size" in glBufferData
	GLsizei vertexCount; //per each attribute
	
	GLsizei faceCount;
	
	GLuint vao;
	GLuint sampler;
	
	//size_t _offsetIntoVertex;
	
	//Face * face;
	
	/** Did vertices get modified / need upload? */
	bool changed;
	
	
} Mesh;

//Renderable is a unique combination of some Mesh (vertex data) and some Material (shader + uniforms).
//It is optional for grouping these related render-time aspects together. It is best used where efficiency is not of the essence, e.g. UI (where efficiency is important, rather use multiple arrays with same index into each). 
//Materials are either treated explicitly or simply as the input interface + matching renderable information for a given ProgramPath
typedef struct Renderable
{	
	khash_t(StrPtr) * uniformsByName;
	
	//occasional upload i.e. not performance-critical, so these objects can be pointers to structs.
	Mesh * mesh;

	//a link back to the data this Renderable represents
	//void * userData;
} Renderable;

//TODO you will need to memcpy out the range of units used in a single instances draw call
//TODO make it use a particular Mesh, Material (with Texture) etc.
typedef struct RenderableInstances
{
	//we always need a contiguous buffer. So to keep things fast, client app *should*
	//memcpy out a contiguous block from a super-array: this means pre-sorting.
	GLsizei	sizeofElement;
	GLsizei count;
	GLuint buffer;
	GLvoid * data; //for now, model matrices.
} RenderableInstances;

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


typedef struct Shader
{
	char name[STRLEN_MAX];
	GLuint id;
	GLenum type; //(NOPE!) SHADER_VERTEX, _FRAGMENT, _GEOMETRY, _TESSELATION_CONTROL, _TESSELATION_EVALULATION
	const char * source;
	
	//inputs:
	khash_t(StrInt) attributesByName;
	khash_t(StrInt) uniformsByName;
	
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

typedef struct AttributeLocation
{
	char name[STRLEN_MAX];
	GLuint index;
} AttributeLocation;

typedef struct Program
{
	//contains information on a single shader pass
	//batching & instancing require *exactly* the same state per instance - this includes uniforms & attributes
	char name[STRLEN_MAX];
	//char * version //shader language version (copied from shaders' version, or later used to dictate this)
	
	GLuint id;
	
	Shader vertex;
	Shader geometry;
	Shader fragment;
	
	//inputs: as vertex shader's inputs?
	//output (frame or render) buffer
	
	//attribute locations are a specification by the program - to which vertex shaders must conform.
	khash_t(Str_AttributeLocation) * attributeLocationsByName;
	//khash_t(IntStr) attributeLocationsByIndex; //can't be a list, must be a map since indices are not compact
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
	
	//some uniforms are general - these are those - setting for the entire Material so to speak
	//others are specific to the Renderable(Set)
	khash_t(StrInt) uniforms; //for referencing uniforms by name
	khash_t(StrInt) attributes; //ref attribs by name
	
	//root of the graph from which we determine the assembled code for each class of shader.
	char * vertexShader;
	char * fragmentShader;
	char * geometryShader;

} Material;

typedef struct Color
{
	float r;
	float g;
	float b;
	float a;
} Color;

#define FOREACH_MOUSE_BUTTON(HANDLER) \
	HANDLER(ORB_MOUSE_X) \
	HANDLER(ORB_MOUSE_Y) \
	HANDLER(ORB_MOUSE_BUTTON_1) \
	HANDLER(ORB_MOUSE_BUTTON_2) \
	HANDLER(ORB_MOUSE_BUTTON_3) \
	HANDLER(ORB_MOUSE_BUTTON_4) \
	HANDLER(ORB_MOUSE_BUTTON_5) \
	HANDLER(ORB_MOUSE_BUTTON_6) \
	HANDLER(ORB_MOUSE_BUTTON_7) \
	HANDLER(ORB_MOUSE_BUTTON_8) \
	HANDLER(ORB_MOUSE_BUTTONS_COUNT)

#define FOREACH_KEY(HANDLER) \
	HANDLER(ORB_KEY_UNKNOWN) \
	HANDLER(ORB_KEY_0) \
	HANDLER(ORB_KEY_1) \
	HANDLER(ORB_KEY_2) \
	HANDLER(ORB_KEY_3) \
	HANDLER(ORB_KEY_4) \
	HANDLER(ORB_KEY_5) \
	HANDLER(ORB_KEY_6) \
	HANDLER(ORB_KEY_7) \
	HANDLER(ORB_KEY_8) \
	HANDLER(ORB_KEY_9) \
	HANDLER(ORB_KEY_KP_0) \
	HANDLER(ORB_KEY_KP_1) \
	HANDLER(ORB_KEY_KP_2) \
	HANDLER(ORB_KEY_KP_3) \
	HANDLER(ORB_KEY_KP_4) \
	HANDLER(ORB_KEY_KP_5) \
	HANDLER(ORB_KEY_KP_6) \
	HANDLER(ORB_KEY_KP_7) \
	HANDLER(ORB_KEY_KP_8) \
	HANDLER(ORB_KEY_KP_9) \
	HANDLER(ORB_KEY_A) \
	HANDLER(ORB_KEY_B) \
	HANDLER(ORB_KEY_C) \
	HANDLER(ORB_KEY_D) \
	HANDLER(ORB_KEY_E) \
	HANDLER(ORB_KEY_F) \
	HANDLER(ORB_KEY_G) \
	HANDLER(ORB_KEY_H) \
	HANDLER(ORB_KEY_I) \
	HANDLER(ORB_KEY_J) \
	HANDLER(ORB_KEY_K) \
	HANDLER(ORB_KEY_L) \
	HANDLER(ORB_KEY_M) \
	HANDLER(ORB_KEY_N) \
	HANDLER(ORB_KEY_O) \
	HANDLER(ORB_KEY_P) \
	HANDLER(ORB_KEY_Q) \
	HANDLER(ORB_KEY_R) \
	HANDLER(ORB_KEY_S) \
	HANDLER(ORB_KEY_T) \
	HANDLER(ORB_KEY_U) \
	HANDLER(ORB_KEY_V) \
	HANDLER(ORB_KEY_W) \
	HANDLER(ORB_KEY_X) \
	HANDLER(ORB_KEY_Y) \
	HANDLER(ORB_KEY_Z) \
	HANDLER(ORB_KEY_F1) \
	HANDLER(ORB_KEY_F2) \
	HANDLER(ORB_KEY_F3) \
	HANDLER(ORB_KEY_F4) \
	HANDLER(ORB_KEY_F5) \
	HANDLER(ORB_KEY_F6) \
	HANDLER(ORB_KEY_F7) \
	HANDLER(ORB_KEY_F8) \
	HANDLER(ORB_KEY_F9) \
	HANDLER(ORB_KEY_F10) \
	HANDLER(ORB_KEY_F11) \
	HANDLER(ORB_KEY_F12) \
	HANDLER(ORB_KEY_F13) \
	HANDLER(ORB_KEY_F14) \
	HANDLER(ORB_KEY_F15) \
	HANDLER(ORB_KEY_F16) \
	HANDLER(ORB_KEY_F17) \
	HANDLER(ORB_KEY_F18) \
	HANDLER(ORB_KEY_F19) \
	HANDLER(ORB_KEY_F20) \
	HANDLER(ORB_KEY_F21) \
	HANDLER(ORB_KEY_F22) \
	HANDLER(ORB_KEY_F23) \
	HANDLER(ORB_KEY_F24) \
	HANDLER(ORB_KEY_F25) \
	HANDLER(ORB_KEY_RIGHT) \
	HANDLER(ORB_KEY_LEFT) \
	HANDLER(ORB_KEY_DOWN) \
	HANDLER(ORB_KEY_UP) \
	HANDLER(ORB_KEY_LEFT_SHIFT) \
	HANDLER(ORB_KEY_LEFT_CONTROL) \
	HANDLER(ORB_KEY_LEFT_ALT) \
	HANDLER(ORB_KEY_LEFT_SUPER) \
	HANDLER(ORB_KEY_RIGHT_SHIFT) \
	HANDLER(ORB_KEY_RIGHT_CONTROL) \
	HANDLER(ORB_KEY_RIGHT_ALT) \
	HANDLER(ORB_KEY_RIGHT_SUPER) \
	HANDLER(ORB_KEY_SPACE) \
	HANDLER(ORB_KEY_APOSTROPHE) \
	HANDLER(ORB_KEY_COMMA) \
	HANDLER(ORB_KEY_MINUS) \
	HANDLER(ORB_KEY_PERIOD) \
	HANDLER(ORB_KEY_SLASH) \
	HANDLER(ORB_KEY_SEMICOLON) \
	HANDLER(ORB_KEY_EQUAL) \
	HANDLER(ORB_KEY_BACKSLASH) \
	HANDLER(ORB_KEY_LEFT_BRACKET) \
	HANDLER(ORB_KEY_RIGHT_BRACKET) \
	HANDLER(ORB_KEY_GRAVE_ACCENT) \
	HANDLER(ORB_KEY_WORLD_1) \
	HANDLER(ORB_KEY_WORLD_2) \
	HANDLER(ORB_KEY_ESCAPE) \
	HANDLER(ORB_KEY_ENTER) \
	HANDLER(ORB_KEY_TAB) \
	HANDLER(ORB_KEY_BACKSPACE) \
	HANDLER(ORB_KEY_INSERT) \
	HANDLER(ORB_KEY_DELETE) \
	HANDLER(ORB_KEY_PAGE_UP) \
	HANDLER(ORB_KEY_PAGE_DOWN) \
	HANDLER(ORB_KEY_HOME) \
	HANDLER(ORB_KEY_END) \
	HANDLER(ORB_KEY_CAPS_LOCK) \
	HANDLER(ORB_KEY_SCROLL_LOCK) \
	HANDLER(ORB_KEY_NUM_LOCK) \
	HANDLER(ORB_KEY_PRINT_SCREEN) \
	HANDLER(ORB_KEY_PAUSE) \
	HANDLER(ORB_KEY_KP_DECIMAL) \
	HANDLER(ORB_KEY_KP_DIVIDE) \
	HANDLER(ORB_KEY_KP_MULTIPLY) \
	HANDLER(ORB_KEY_KP_SUBTRACT) \
	HANDLER(ORB_KEY_KP_ADD) \
	HANDLER(ORB_KEY_KP_ENTER) \
	HANDLER(ORB_KEY_KP_EQUAL) \
	HANDLER(ORB_KEY_MENU) \
	HANDLER(ORB_KEYS_COUNT) \
	//ORB_KEY_WORLD_1 & _2 are non-US keys (differ depending on region?)

#define GENERATE_ENUM(value) value,
#define GENERATE_STRING(value) #value,
#define GENERATE_KH(value) k = kh_put(StrInt,device->nameToIndex,#value,&ret); kh_value(device->nameToIndex,k) = c; c++;

//unlike e.g. GLFW, these are ordered zero-based, compact so they may be used as indices into an packed array
typedef enum Key
{
    FOREACH_KEY(GENERATE_ENUM)
} Key;

typedef enum Button
{
    FOREACH_MOUSE_BUTTON(GENERATE_ENUM)
} Button;

static const char * KeyString[] =
{
	FOREACH_KEY(GENERATE_STRING)
};

static const char * MouseButtonString[] =
{
	FOREACH_MOUSE_BUTTON(GENERATE_STRING)
};

void Key_setupStringToKey();

typedef enum InputBasis
{
	STATE, //default
	DELTA
} InputBasis;

typedef struct DeviceChannel
{
	//2 states & 2 deltas is enough to calculate the most recent other (state for delta or delta for state)
	float state[2];
	float delta[2];
	InputBasis basis;
	//bool blocked;
	bool active; //framework-private, particularly useful for multi-touch or devices that have disconnected but are waiting on clean-up by OS etc.
	bool consumed; ///< Has input from this channel already been consumed during this Hub_update()? (prevents a device being used twice) TODO prefer Input.consumed!
} DeviceChannel;

typedef struct Device
{
	kvec_t(DeviceChannel) channels;
	//void * other; //special reference to other information, e.g. an array of fingers for a touch device.
	//TODO Finger fingers[]; //or rather, a pointer to an array elsewhere, if touchscreen device.
	uint64_t channelsActiveMask;
	khash_t(StrInt) * nameToIndex; ///< Maps string name to numeric index, e.g. "ORB_KEY_SPACE" to enum index of same name ORB_KEY_SPACE.
	char ** indexToName; ///< Maps numeric index to string name, e.g. "ORB_KEY_SPACE" to enum index of same name ORB_KEY_SPACE.
	void (*initialise)	(struct Device * const this);
	void (*update)		(struct Device * const this);
	bool consumed; ///< Has input from this device already been consumed during this Hub_update()? (prevents a device being used twice) TODO prefer Input.consumed!
} Device;

typedef void (*DeviceUpdate)	(Device * device);
typedef void (*DeviceInitialise)(Device * device);

/*
typedef struct Finger
{
	size_t index; //id?
	DeviceChannel channels[3]; //X, Y, pressure
} Finger;

//use as alternative to Device
typedef struct TouchDevice
{
	Finger fingers[10];
	size_t fingerCount; //fingers in actual use at this moment
}
*/
typedef float (*InputFunction) 		();
typedef void  (*ResponseFunction) 	(void * context, float value, float valueLast);
typedef bool  (*HasFocusFunction) 	(void * context);


//abstracts raw channel inputs into game logic inputs, provides a response for same, and stores recent values
typedef struct Input
{
	Device * device;
	uint16_t code; //index of channel in device
	bool negate; //if true, flip the sign on the incoming value
	//TODO optional custom function for combining raw channel values?
	bool consumed; ///< Has this input already been consumed during this Hub_update()? (prevents a device being used twice) - PREFER to channel.consumed! as this is game logic related
} Input;
const struct Input inputEmpty;

/// A mapping of some raw input (via a DeviceChannel) to an user-defined response.

/// One such mapping may have many contributing inputs, but their results are not cumulative;
/// instead, all "negated" values are combined and all "non-negated" values are combined,
/// and the results of these two are added. This means that even if five "positive" keys
/// and only one "negative" key for an axis are being held down, the result is still zero.
typedef struct InputMapping
{
	char name[STRLEN_MAX]; //same as key into khash holding all InputMappings
	ResponseFunction ctrlResponse;
	ResponseFunction viewResponse;
	HasFocusFunction hasFocus;
	InputBasis basis;
	kvec_t(Input) inputsList; //the inputs which can trigger a function call
	
	float state[2];
	float delta[2];
} InputMapping;

typedef kvec_t(Input) InputList;
typedef kvec_t(InputMapping) InputMappingList;

typedef struct Capabilities
{
	bool vao;
} Capabilities;

typedef struct Engine
{
	Capabilities capabilities;
	bool debugDesktopNoVAO;
	
	int inputEventCount;
	
	khash_t(StrPtr) * programsByName;
	khash_t(StrPtr) * shadersByName;
	khash_t(StrPtr) * texturesByName;
	khash_t(StrPtr) * materialsByName;
	khash_t(StrPtr) * meshesByName;
	khash_t(StrPtr) * renderPathsByName;
	khash_t(StrPtr) * renderTexturesByName;
	
	//until we create merged map and list that contain not only pointers to arrays but also the arrays themselves... use these as backing arrays
	Shader   	shaders[HH_SHADERS_MAX];
	Texture * 	textures[HH_TEXTURES_MAX];
	Texture * 	renderTextures[HH_RENDERTEXTURES_MAX];
	Material 	materials[HH_MATERIALS_MAX];
	Program  	programs[HH_PROGRAMS_MAX];
	Mesh *		meshes[HH_MESHES_MAX];
	
	uint64_t shaderKeys[HH_SHADERS_MAX];
	uint64_t textureKeys[HH_TEXTURES_MAX];
	uint64_t rendertextureKeys[HH_RENDERTEXTURES_MAX];
	uint64_t materialKeys[HH_MATERIALS_MAX];
	uint64_t programKeys[HH_PROGRAMS_MAX];
	uint64_t meshKeys[HH_MESHES_MAX];
	//TODO - these lists but not generic / pointer - int lists! then remove same fields from Renderables.
	//List changedMesh; //indices of Renderables that changed vertex data
	//List changedMaterials; //indices of Renderables that changed uniforms
	
	//map is best - if devices connect in different orders at runtime, we are not relying on compile-time indices into an array
	//DeviceHub deviceHub;
	//Device array[2];
	khash_t(StrPtr) * devicesByName;
	
	//khash_t(StrPtr) * inputMappingsByName;
	
	Program * program; //the current shader program TODO should be an index into array? then use a getter to access. 
	
	//Transforms may not all have associated renderables...
	
	//TODO incorporate as separate RTT renderables array?
	Renderable fullscreenQuad;
	mat4x4 fullscreenQuadMatrix;

	#ifdef __ANDROID__	
	struct android_app* app;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
	EGLint * attribsList;
	
	bool touches[10]; //DEV
	bool paused;
	#endif//__ANDROID__
	
	int32_t width;
	int32_t height;
	int32_t touchX;
	int32_t touchY;
	
	void (*userInitialiseFunc)();
	bool   userInitialised;
	
	void (*userUpdateFunc)(void * const);
	void * userUpdateArg;
	
	void (*userSuspendFunc)(void * const);
	void * userSuspendArg;
	
	float deltaSec;
} Engine;

typedef void (*IndexedRenderableFunction)(struct Renderable * renderable, uint16_t i, void * model);

typedef kvec_t(uint16_t) IndexList;

/// creates new Renderable%s and adds them to this View%'s list for rendering
void IndexedRenderableManager_create(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr,
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model);

/// updates a Renderable that has (at any point previously) been created
void IndexedRenderableManager_update(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr, //actually unused here, but keeps the arg lists uniform between create/update/render
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model);

/// renders a group of Renderables from an index list
void IndexedRenderableManager_render(
	Renderable * const renderables,
	IndexList * indexRenderListPtr,
	Engine * enginePtr);

Attribute * Mesh_activateAttributeAt(Mesh * this, size_t i);
void Mesh_submit(Mesh * mesh, Engine * engine);
void Mesh_calculateNormals(Mesh * this);
void Mesh_appendTri(Mesh * mesh, GLushort a, GLushort b, GLushort c);
void Mesh_appendLine(Mesh * mesh, GLushort a, GLushort b);
Index Mesh_appendVertex(Mesh * mesh, void * vertex);
void Mesh_initialise(Mesh * this, 
	GLuint topology,
	GLenum usage,
	size_t stride);
void Mesh_clear(Mesh * this);
Attribute * Mesh_addAttribute(Mesh * this, GLuint index, GLint components, GLenum type, GLboolean normalized);
void Mesh_merge(Mesh * this, Mesh * other);

void Attribute_submitData(Attribute * attribute, Mesh * mesh, Engine * engine);
void Attribute_prepare(Attribute * attribute, Mesh * mesh);

void UniformGroup_update(khash_t(StrPtr) * uniformsByName, Program * program);

Texture * Texture_create();
void Texture_loadData(Texture * texture, const char * filename);
void Texture_createData(Texture * texture);
void Texture_clearData(Texture * texture, bool alphaFull);
//Texture * Texture_loadFromMemory(const char * filename);

GLenum Texture_getTextureUnitConstant(Texture * this);
void Texture_fresh(Texture * this);
void Texture_refresh(Texture * this);
void Texture_prepare(Texture * this, Program * program);
void Texture_setTexelFormats(Texture * this, GLenum arranged, GLenum atomTypeExternal); //set both internal and external format
void Texture_setDimensionCount(Texture * this, GLenum dimensions);

int Texture_free(Texture * texture);

void * Texture_read2(int x, int y, void * texel);
void Texture_write2(int x, int y, void * texel);
void * Texture_read3(int x, int y, int z);
void Texture_write3(int x, int y, int z);

#define TextureAtlas_put(atlas, entry) kh_set(Str_TextureAtlasEntry, atlas->entriesByName, entry.name, entry) 
#define TextureAtlas_get(atlas, name)  kh_get_val(Str_TextureAtlasEntry, atlas->entriesByName, name, (TextureAtlasEntry){0})
//#define TextureAtlas_initialise(atlas) atlas = kh_init(Str_TextureAtlasEntry)
TextureAtlas * TextureAtlas_construct();
void TextureAtlas_load(TextureAtlas * atlas, const char * filename);
void TextureAtlas_parse(TextureAtlas * atlas, ezxml_t atlasXml);
void Texture_applyParameters(Texture * this);

void RenderTexture_createDepth(Texture * const this, GLuint i, uint16_t width, uint16_t height);
void RenderTexture_createColor(Texture * const this, GLuint i, uint16_t width, uint16_t height, GLenum format);

GLuint GLBuffer_create(
    GLenum target,
    GLsizei size,
    const void *data,
	GLenum usage
);


Shader * 	Shader_load(const char * path, const char * name, GLenum type);
void 		Shader_initialiseFromSource(Shader * this);

typedef kvec_t(char *) StrList;
Program * 	Program_construct();
void 		Program_initialiseFromShaders(Program * this, GLuint vertex_shader, GLuint fragment_shader);

void Engine_initialise(Engine * this, int width, int height, const char * windowTitle);
void Engine_dispose(Engine * engine);
void Engine_clear(); //TODO should be Render_clear?
Program * Engine_setCurrentProgram(Engine * this, char * name);
Program * Engine_getCurrentProgram(Engine * this);
void Engine_many(Engine * this, Renderable * renderable, RenderableInstances * instances);
void Engine_one(Engine * this, Renderable * renderable);
void Engine_getPath(Engine * engine, const char * path, int pathLength, const char * partial);
float Engine_smoothstep(float t);

char* Text_load(char* filename);

void Loop_initialise(Engine * engine,
	int windowWidth, //ignored for Android
	int windowHeight, //ignored for Android
	const char *  windowTitle); //ignored for Android
void Loop_run(Engine * engine);

void GLFW_errorCallback(int error, const char * description);
bool GLTool_isExtensionSupported(const char * extension); //redundant, see GLFW

///////////// DEVICE TYPES //////////////

typedef struct Keyboard
{
	Device base;
} Keyboard;
void Keyboard_initialise(Device * device);
void Keyboard_update(Device * device);
Device * Keyboard_construct();

typedef enum Mouse
{
    ORB_BUTTON_LEFT,
	ORB_BUTTON_RIGHT,
	ORB_BUTTON_MIDDLE
} Mouse;
void Mouse_initialise(Device * device);
void Mouse_update(Device * device);
Device * Mouse_construct();

int _glSizeof(const int key);
#define glSizeof(key) _glSizeof(key)

///////////// PLATFORM SPECIFIC FUNCTIONS //////////////

/*
//Private:
#ifdef __ANDROID__
void 	Android_frame(Engine * engine);
void 	Android_onAppCmd(struct android_app* app, int32_t cmd);
int32_t Android_onInputEvent(struct android_app* app, AInputEvent* event);
#endif//__ANDROID__
*/

//globals
//Engine engine; //allows every other file to ref as extern, and no requirement to include main from renderer etc.
#ifdef DESKTOP
GLFWwindow * window; //TODO include as void * window in Engine
#endif//DESKTOP
#ifdef __ANDROID__
#endif//__ANDROID__

#endif //COM_ARCANEINGENUITY_ORB_H