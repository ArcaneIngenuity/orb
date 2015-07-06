#include "hedgehog.h"

#define KEYPARTS 1

#include <assert.h>

//TODO see "I/O callbacks" in stbi_image.h for loading images out of a data file
void GLFW_errorCallback(int error, const char * description)
{
    //fputs(description, stderr);
	printf ("GLFW ERROR: code %i msg: %s\n", error, description);
}

void Mesh_calculateNormals(Mesh * this)
{
	
	//this->normals

}

void Axes_create(float radius, float bodyLength, float headLength, GLushort * index, GLfloat * position) {
/*
	GLushort indices[6] = {
		//0, 1, 2,
		//2, 3, 0
		2, 1, 0,
		0, 3, 2
	};
	*/
	//GLfloat position[9*3*3];
	
	
	for (int a = 0; a < 1; a++) //for each axis
	{
		int b = (a + 1) % 3;
		int c = (a + 2) % 3;
	
		int i = a * 3 * 3; //index into vertex attribute array
		int j = a * 14; //index into triangle index array
		//create 9 vertex positions per axis:
		//arrow base near origin (4 vertices * 3 components = 12 floats)
		position[i+a] = radius;
		position[i+b] = -radius;
		position[i+c] = -radius;
		i+=3;
		position[i+a] = radius;
		position[i+b] = radius;
		position[i+c] = -radius;
		i+=3;
		position[i+a] = radius;
		position[i+b] = -radius;
		position[i+c] = -radius;
		i+=3;
		position[i+a] = radius;
		position[i+b] = -radius;
		position[i+c] = radius;
		i+=3;
		
		/*
		//arrow end away from origin (4 vertices * 3 components = 12 floats)
		position[i+a] = bodyLength;
		position[i+b] = -radius;
		position[i+c] = -radius;
		i+=3;
		position[i+a] = bodyLength;
		position[i+b] = radius;
		position[i+c] = -radius;
		i+=3;
		position[i+a] = bodyLength;
		position[i+b] = -radius;
		position[i+c] = -radius;
		i+=3;
		position[i+a] = bodyLength;
		position[i+b] = -radius;
		position[i+c] = radius;
		i+=3;
		*/
		
		//arrow tip
		position[i+a] = bodyLength + headLength;
		position[i+b] = 0;
		position[i+c] = 0;
		/*
		//create indices (4 tris cap, 8 tris body, 2 tris base)
		//base
		index[j] = {0, 1, 2}; j+=3;
		index[j] = {0, 2, 3}; j+=3;
	
		//cap
		index[j] = {0, 1, 4}; j+=3;
		index[j] = {1, 2, 4}; j+=3;
		index[j] = {2, 3, 4}; j+=3;
		index[j] = {0, 3, 4}; j+=3;
		*/
	}
	
/*	= {
		-0.5, -0.5, 0.,
		0.5, -0.5, 0.,
		0.5, 0.5, 0.,
		-0.5, 0.5, 0.,

		-0.5, -0.5, 0.,
		0.5, -0.5, 0.,
		0.5, 0.5, 0.,
		-0.5, 0.5, 0.,
		
		
		
		
		
		-0.5, 0.5, 0.
	};
	*/
	/*
	GLfloat color[9*3] = {
		-0.5, -0.5, 0., 1.,
		0.5, -0.5, 0., 1.,
		0.5, 0.5, 0., 1.,
		-0.5, 0.5, 0., 1.
	};
	*/
}
/*
void Transform_update(Transform* this)
{
	
	//Do matrix calculation for this transform.
	if (this->parent)
		mat4x4_dup(this->matrix , this->parent->matrix);
	else
		mat4x4_identity(this->matrix);

	... more to port ...
	//equations of motion

	//first find out what direction "forward" is based on local-to-parent (or world) orientation
	Matrix4 rotation = new Matrix4(); //WILL BE FULL HIERARCHICAL MATRIX
	rotation.rotate(0, 1, 0, yaw);
	rotation.rotate(1, 0, 0, pitch);
	rotation.rotate(0, 0, 1, roll);

	//------------
			
	//WRAP (must come after eqns / motion)
//2014: Beware of wrap + nested transforms
	if (wrap == true)
	{
		wrap();
	}
	
	
	
	Matrix4 translation = new Matrix4();
	translation.translate(position);
	
	
	//SET MATRIX
	matrix.mul(translation.cpy()).mul(rotation.cpy());         
	

}
*/

void Transform_translate(mat4x4 * matPos, vec3 * position, vec3 delta)
{
	mat4x4 matTemp;
	mat4x4_identity(&matTemp);
	*position[XX] += delta[XX];
	*position[YY] += delta[YY];
	*position[ZZ] += delta[ZZ];
	mat4x4_translate(*matPos, *position[XX], *position[YY], *position[ZZ]);
}

void Transform_rotate(mat4x4 * matRot, vec3 * rotation, vec3 delta)
{
	mat4x4 matTemp;
	mat4x4_identity(&matTemp);
	*rotation[XX] += delta[XX];
	*rotation[YY] += delta[YY];
	*rotation[ZZ] += delta[ZZ];
	mat4x4_identity(matTemp);
	mat4x4_rotate_Y(*matRot, matTemp, *rotation[YY]); mat4x4_dup(matTemp, *matRot);
	mat4x4_rotate_X(*matRot, matTemp, *rotation[XX]);
}

//TODO kill in favour of same in Transform
void Transform_updateRotationMatrixYX(vec3 rotation, mat4x4 * matRot)
{
	mat4x4 m;
	mat4x4_identity(m);
	mat4x4_rotate_Y(*matRot, m, rotation[YY]); mat4x4_dup(m, *matRot);
	mat4x4_rotate_X(*matRot, m, rotation[XX]);
}

//TODO kill in favour of same in Transform
void Transform_updatePositionMatrix(vec3 position, mat4x4 * matPos)
{
	mat4x4_translate(*matPos, position[XX], position[YY], position[ZZ]);
}

//don't call until we have done all translations AND rotations
void Transform_finalise(mat4x4 * matTrans, mat4x4 * matPos, mat4x4 * matRot)
{
	mat4x4_mul(*matTrans, *matPos, *matRot);
}

void Camera_lookAt()
{
	//lookingAt;
}
/*
//http://www.songho.ca/opengl/gl_projectionmatrix.html
void Matrix_setProjectionPerspective(mat4x4 matrix, float near, float far, float top, float right)
{
	mat4x4_identity(matrix);
	matrix[0][0] = near / right;
	matrix[1][1] = near / top;
	matrix[2][2] = -(far + near) / (far - near);
	matrix[3][2] = (-2.f * far * near) / (far - near);
	matrix[2][3] = -1.f;
}
*/

//------------------Material-----------------//


//------------------InstanceGroup------------//


//------------------Batch--------------------//

//used when we need a lot of different geometries that use same material to be tightly packed for upload to / rendering on GPU.




//------------------Texture------------------//

Texture * Texture_create()
{
	Texture * texture = malloc(sizeof(Texture));
	glGenTextures(1, &texture->id);
	intMap_create  (&texture->intParametersByName, 		16, &texture->intParameterKeys,   &texture->intParameterValues,   -1);
	floatMap_create(&texture->floatParametersByName, 	16, &texture->floatParameterKeys, &texture->floatParameterValues, -1.f);
	return texture;
}

Texture * Texture_load(const char * filename)
{
	Texture * texture = Texture_create();
	//texture->name = (char *) filename;
	texture->data = stbi_load(filename, &(texture->width), &(texture->height), &(texture->components), 0);
	
	return texture;
}

void RenderTexture_createDepth(Texture * const this, GLuint i, uint16_t width, uint16_t height)
{
	this->unit = i;
	this->width = width;
	this->height = height;
	
	Texture_setDimensionCount(this, GL_TEXTURE_2D);
	Texture_setTexelFormats(this, GL_DEPTH_COMPONENT24, GL_FLOAT);
	
	intMap_put(&this->intParametersByName, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	intMap_put(&this->intParametersByName, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	intMap_put(&this->intParametersByName, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	intMap_put(&this->intParametersByName, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	intMap_put(&this->intParametersByName, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	intMap_put(&this->intParametersByName, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	Texture_applyParameters(this);
	
	glActiveTexture(GL_TEXTURE0 + this->unit); //"which texture unit a texture object is bound to when glBindTexture is called."
	glBindTexture(GL_TEXTURE_2D, this->id); //binds the texture with id specified, to the 2D target
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24 , this->width, this->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
}

void RenderTexture_createColor(Texture * const this, GLuint i, uint16_t width, uint16_t height, GLenum format)
{
	this->unit = i;
	this->width = width;
	this->height = height;
	
	Texture_setDimensionCount(this, GL_TEXTURE_2D);
	Texture_setTexelFormats(this, format, GL_UNSIGNED_BYTE);
	
	intMap_put(&this->intParametersByName, GL_TEXTURE_WRAP_S, GL_REPEAT);
	intMap_put(&this->intParametersByName, GL_TEXTURE_WRAP_T, GL_REPEAT);
	intMap_put(&this->intParametersByName, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	intMap_put(&this->intParametersByName, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Texture_applyParameters(this);
	
	this->data = malloc(sizeof(unsigned char) * this->width * this->height * this->components);
	memset(this->data, 0, sizeof(unsigned char) * this->width * this->height * this->components);
	
	Texture_refresh(this); //fresh()?
}

//prepares the RenderTexture to be rendered from.
void Texture_prepare(Texture * this, Program * program)
{
	glActiveTexture(GL_TEXTURE0 + this->unit); //"glActiveTexture specifies which texture unit a texture object is bound to when glBindTexture is called."
	glBindTexture(GL_TEXTURE_2D, this->id); //ensure the correct texture is bound to the texture unit that the shader will use (?)
	GLint uniformTexture = glGetUniformLocation(program->id, this->name);
	glUniform1i(uniformTexture, this->unit); //let shader's sampler bind to appropriate texture unit
}

/** Returns the OpenGL internally-used constant for a given zero-based texture unit ordinal. */
GLuint Texture_getTextureUnitConstant(Texture * this)
{
	return this->unit + GL_TEXTURE0;
}

void Texture_fresh(Texture * this)
{
	glActiveTexture(GL_TEXTURE0 + this->unit); //"which texture unit a texture object is bound to when glBindTexture is called."
	glBindTexture(GL_TEXTURE_2D, this->id); //binds the texture with id specified, to the 2D target
	glTexImage2D (GL_TEXTURE_2D, 0, this->arrangedInternal, this->width, this->height, 0, this->arrangedExternal, this->atomTypeExternal, 0);
}

void Texture_refresh(Texture * this)
{
	glActiveTexture(GL_TEXTURE0 + this->unit); //"glActiveTexture specifies which texture unit a texture object is bound to when glBindTexture is called."
	glBindTexture(GL_TEXTURE_2D, this->id); //binds the texture with id specified, to the 2D target
	glTexImage2D (GL_TEXTURE_2D, 0, this->arrangedInternal, this->width, this->height, 0, this->arrangedExternal, this->atomTypeExternal, this->data); //uploads specified image data to the 2D target
	//glTexSubImage2D (GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data); //uploads specified image data to the 2D target
}

void Texture_setTexelFormats(Texture * this, GLenum arranged, GLenum atomTypeExternal)
{
	this->arrangedInternal = this->arrangedExternal = arranged;
	this->atomTypeExternal = atomTypeExternal;
	switch (arranged)
	{
		//TODO (see glTexImage2D) GL_DEPTH_STENCIL
		case GL_DEPTH_COMPONENT:  	this->components = 1; break;
		case GL_DEPTH_COMPONENT16:  this->components = 1; break;
		case GL_DEPTH_COMPONENT24:  this->components = 1; break;
		case GL_DEPTH_COMPONENT32F: this->components = 1; break;
		case GL_RED:  				this->components = 1; break;
		case GL_RG:   				this->components = 2; break;
		case GL_RGB: 
		case GL_BGR:  				this->components = 3; break;
		case GL_RGBA:
		case GL_BGRA: 				this->components = 4; break;
		default:
			printf("Error: hedgehog can accept only GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA or GL_BGRA.\n");
			exit(0);
	}
}

void Texture_setDimensionCount(Texture * this, GLenum dimensions)
{
	this->dimensions = dimensions;
}

void Texture_applyParameters(Texture * this)
{
	glActiveTexture(GL_TEXTURE0 + this->unit);
	glBindTexture(GL_TEXTURE_2D, this->id);
	
	for (uint8_t p = 0; p < this->intParametersByName.count; p++)
	{
		GLenum key = this->intParametersByName.keys[p];
		GLint value = this->intParametersByName.entries[p];
		glTexParameteri(this->dimensions, key, value);
	}
	
	//TODO loop over float params
}
//------------------GLBuffer------------------//

GLuint GLBuffer_create(
    GLenum target,
    GLsizei size,
    const void *data,
	GLenum usage
) {
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, size, data, usage);
	printf("buffer id %i\n", buffer);
    return buffer;
}


//--------------Program-------------------//

bool linkProgramSuccess(int program)
{
	int status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	
	if (!status)
	{
		GLint infoLogLength;
		glGetProgramiv(program,GL_INFO_LOG_LENGTH,&infoLogLength);
		GLchar infoLog[infoLogLength + 1];
		//printf("info log length:%i\n", infoLogLength);
		glGetProgramInfoLog(program, infoLogLength + 1, NULL, infoLog);
		printf("glLinkProgram() failed: %s\n", infoLog);
	}
	printf("status %i\n", status);
	return status == GL_TRUE;
}

//------------------Shader------------------//
void Shader_load(Hedgehog * this, char * name)
{
	Shader * vert;
	Shader * frag;
	Program * program;
	
	if (strlen(name) > KEYPARTS * 8 - 1)
	{
		printf("[Shader_load] Length of shader name must be less than or equal to KEYPARTS * 8 - 1.");
		exit(0);
	}
	vert = malloc(sizeof(Shader));
	frag = malloc(sizeof(Shader));
	vert->type = GL_VERTEX_SHADER;
	frag->type = GL_FRAGMENT_SHADER;
	
	#if _WIN32
	char * path = ".\\shaders\\";
	#else //all other supported OS?
	char * path = "./shaders/";
	#endif //linux
	
	size_t lengthName = strlen(name); //5 = .vert
	size_t lengthPath = strlen(name); //5 = .vert
	//printf("l=%d\n", lengthName);
	char vertFilepath[lengthPath+lengthName+1+4];
	strcpy(vertFilepath, path);
	strcat(vertFilepath, name);
	strcat(vertFilepath, ".vert");
	printf("vertFilepath %s\n", vertFilepath);
	vert->source = Text_load(vertFilepath);
	
	char fragFilepath[lengthPath+lengthName+1+4]; //5 = .frag
	strcpy(fragFilepath, path);
	strcat(fragFilepath, name);
	strcat(fragFilepath, ".frag");
	printf("fragFilepath %s\n", fragFilepath);
	frag->source = Text_load(fragFilepath);
	
	//printf("vert %s\n\n", vert->source);
	//printf("frag %s\n\n", frag->source);
	
	Shader_construct(vert);
	Shader_construct(frag);
	
	int final = lengthName < 8 ? lengthName : 8 - 1;
	
	char vertKey[8 + 1] = {0};
	strncpy(vertKey, name, 8);
	vertKey[final] = 'v'; //set last character to distinguish. TODO should be either first null or last character.
	printf("v=%s\n", vertKey);
	//TODO check whether key exists
	put(&this->shadersByName, *(uint64_t *) vertKey, vert); //diffusev
	
	char fragKey[8 + 1] = {0};
	strncpy(fragKey, name, 8);
	fragKey[final] = 'f'; //set last character to distinguish. TODO should be either first null or last character.
	
	printf("f=%s\n", fragKey);
	//TODO check whether key exists
	put(&this->shadersByName, *(uint64_t *) fragKey, frag); //diffusef
	
	program = malloc(sizeof(Program));
	Program_construct(program, vert->id, frag->id);
	program->topology = GL_TRIANGLES;
	
	//TODO check whether key exists
	put(&this->programsByName, *(uint64_t *) pad(name), program);
}

void Shader_construct(Shader * this)//, const char* shader_str, GLenum shader_type)
{
	// Compile the shader
	GLuint id = this->id = glCreateShader(this->type);
	//TODO check for errors
	glShaderSource(id, 1, &this->source, NULL);
	//TODO check for errors
	glCompileShader(id);
	
	// Check the result of the compilation
	GLint result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	//printf("result %i", result);
	if(!result)
	{
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		GLchar infoLog[infoLogLength + 1];
		//printf("info log length:%i\n", infoLogLength);
		glGetShaderInfoLog(id, infoLogLength + 1, NULL, infoLog);
		printf("glCompileShader() failed: %s\n%s\n", infoLog, this->source);
		
		//TODO encapsulate the below in a exitOnFatalError() that can be used anywhere.
		//TODO set up error codes for untimely exit.
		glfwTerminate();
		exit(-1);
	}
	else
	{
		printf("glCompileShader() success.\n");
	}
}
/*
//search on "layout qualifiers" here: https://www.opengl.org/registry/doc/GLSLangSpec.3.30.6.pdf
void Shader_validateAttributeInterface(Shader * out, Shader * in)
{
	Attribute * attributeIn, attributeOut;
	char * nameIn;

	//check in array order of the in Shader's attributes map, since it's the one that must get the right inputs
	for (int i = 0; i < in->attributesByName.count; i++)
	{
		nameIn = in->attributesByName.keys[i];
		attributeIn = in->attributesByName.values[i];
		
		if (attributeIn->polarity == ATTRIBUTE_QUALIFIER_IN ||
			attributeIn->polarity == ATTRIBUTE_QUALIFIER_INOUT) //and in has the expected polarity
		{
			attributeOut = get(out->attributesByName, nameIn);	
			if (attributeOut != NULL) //same name exists in the out Shader
			{
				if (attributeOut->polarity == ATTRIBUTE_QUALIFIER_OUT ||
					attributeOut->polarity == ATTRIBUTE_QUALIFIER_INOUT) //and out has the expected polarity
				{
					if (attributeOut->typeBase == attributeIn->typeBase) //VEC, MAT, etc.
				}
				
			}
		
		}

		
	}
}
*/
//to deal with dependency fullfilment: sometimes the result of some step a will not only input to b, but also to c.
//since b is earlier, we must resolve c early enough to input to b.
//so, since all we get in is a dependency graph, we must break this into phases a,b,c etc. with everything happening early enough to influence dependent phases.
//the only logical way to do this is to walk back from root, adding requisite shaders to preceding phases, and if thereafter we find they are requisite to an earlier phase, change the phase they occur in.
//Phases as discrete objects will help visualisation once we get the shader builder going. we can hit a button to order into phases and see what this looks like. 


//Phases are basically "parallel" runs of Program.
//But consider this: If a group of functionality, none of which is dependent on any of the rest, is run together, can't we do this all as one program?
//a dynamically built shader program. Combine everything needed for that phase into one vs, one gs, one fs and be done.

//then we can simply keep a map of shader function strings? with tokens that get replaced as appropriate?

//however, some results can be immediately calculated, i.e. in the current phase.
//while other results have to be produced by a full shader pass.

//what is the logical distinction between these?... to require a full pass, one or more of these cases must apply:
//-must be rendered / seen (final result)
//-must be interacted with on CPU e.g. id buffer (need render buffer for user feedback)
//-must be interpolated in terms of fragments / surfaces e.g. normal buffer
//-need depth or other internal info (can only be gotten from GL internals by a fullscreen rasterisation) PRE-CACHED rather than accessed here and there


//------------------Program------------------//

void Program_construct(Program * this, GLuint vertex_shader, GLuint fragment_shader)
{
	GLuint id = this->id = glCreateProgram();
	
	// Attach the shaders to the program
	glAttachShader(id, vertex_shader);
	glAttachShader(id, fragment_shader);
	printf("gl error %i\n", glGetError());
	
	// Link program and check success
	glLinkProgram(id);
	if (!linkProgramSuccess(id))
	{
		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		GLchar* infoLog[infoLogLength + 1];
		glGetProgramInfoLog(id, infoLogLength + 1, NULL, *infoLog);
		printf("glLinkProgram() failed: %s\n", infoLog);
		printf("gl error %i\n", glGetError());
		//glfwTerminate();
		//exit(-1);
		
		//TODO do these atexit
		//Delete shaders; won't take effect unless they are detached first.
		glDetachShader(id, vertex_shader);
		glDetachShader(id, fragment_shader);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}
	else
		printf("glLinkProgram() success.\n");
}

void Hedgehog_clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Hedgehog_clearColor()
{
	 glClear(GL_COLOR_BUFFER_BIT);
}

void Hedgehog_clearDepth()
{
	 glClear(GL_DEPTH_BUFFER_BIT);
}

void Hedgehog_createFullscreenQuad(Hedgehog * this, GLuint positionVertexAttributeIndex, GLuint texcoordVertexAttributeIndex)
{
	mat4x4_identity(this->fullscreenQuadMatrix);
	
	//mesh + associated VAO
	Mesh * mesh = this->fullscreenQuad.mesh = malloc(sizeof(Mesh));;
	mesh->topology = GL_TRIANGLES;
	mesh->indexCount = 6;
	mesh->vertexCount = 4;
	mesh->index = malloc(sizeof(GLushort) * mesh->indexCount);
	mesh->attribute[positionVertexAttributeIndex].vertex = malloc(sizeof(GLfloat) * mesh->vertexCount * TEXCOORD_COMPONENTS);
	mesh->attribute[texcoordVertexAttributeIndex].vertex = malloc(sizeof(GLfloat) * mesh->vertexCount * TEXCOORD_COMPONENTS);
	
	Attribute * position = &mesh->attribute[positionVertexAttributeIndex];
	Attribute * texcoord = &mesh->attribute[texcoordVertexAttributeIndex];

	//create vertex attributes using static initialisers, then copy these into the Mesh
	GLushort _index[6] = {
		0, 1, 2,
		2, 3, 0
	};
	
	GLfloat _position[8] = {
		-1.0, -1.0,
		+1.0, -1.0,
		+1.0, +1.0,
		-1.0, +1.0
	};
	
	GLfloat _texcoord[8] = {
		//y-inverted due to texture space flip
		0.0, 0.0,
		1.0, 0.0,
		1.0, 1.0,
		0.0, 1.0
	};

	memcpy(mesh->index, _index, sizeof(_index));
	memcpy(position->vertex, _position, sizeof(_position));
	memcpy(texcoord->vertex, _texcoord, sizeof(_texcoord));
	
	printf("index %d %d\n", sizeof(_index), mesh->indexCount * 2);
	printf("pos   %d %d\n", sizeof(_position), mesh->vertexCount * 4);
	printf("texco %d %d\n", sizeof(_texcoord), mesh->vertexCount * 4);
	
	glGenVertexArrays(1, &(mesh->vao)); //VAO frees us from having to call glGetAttribLocation & glVertexAttribPointer on each modify op
	glBindVertexArray(mesh->vao);
	
	//positions
	glGenBuffers(1, &position->id);
	glBindBuffer(GL_ARRAY_BUFFER, position->id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh->vertexCount * TEXCOORD_COMPONENTS, position->vertex, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(positionVertexAttributeIndex, TEXCOORD_COMPONENTS, GL_FLOAT, GL_FALSE, 0, 0); //provide data / layout info; "take buffer that is bound at the time called and associates that buffer with the current VAO"
	glEnableVertexAttribArray(positionVertexAttributeIndex); //enable attribute for use
	
	//texcoords
	glGenBuffers(1, &texcoord->id);
	glBindBuffer(GL_ARRAY_BUFFER, texcoord->id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh->vertexCount * TEXCOORD_COMPONENTS, texcoord->vertex, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(texcoordVertexAttributeIndex, TEXCOORD_COMPONENTS, GL_FLOAT, GL_FALSE, 0, 0); //provide data / layout info; "take buffer that is bound at the time called and associates that buffer with the current VAO"
	glEnableVertexAttribArray(texcoordVertexAttributeIndex); //enable attribute for use
	
	//unbind
	glBindVertexArray(0);
}
/*
void Hedgehog_renderSet(Program * program, RenderableSet * renderableSet, const GLfloat * matVP)
{
	Mesh * mesh = renderableSet->mesh;

	glUseProgram(program->id);
	glBindVertexArray(mesh->vao);
	
	//prep uniforms...(general to all instances)
	//...view-projection matrix (general to all instancing approaches)
	GLint vpLoc = glGetUniformLocation(program->id, "vp");
	glUniformMatrix4fv(vpLoc, 1, GL_FALSE, (GLfloat *)matVP);
	
	//upload instance data
	glBindBuffer(GL_ARRAY_BUFFER, renderableSet->buffer); //TODO use const instead of literal buffer name
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * renderableSet->count, renderableSet->data, GL_DYNAMIC_DRAW); 
	//TODO specify usage (e.g. GL_DYNAMIC_DRAW) on the Mesh? Or better yet, on the particular Renderable using Mesh.
	//TODO glBufferSubData(GL_ARRAY_BUFFER,
	//TODO consider storing 2 buffers for each dynamic object - from https://www.opengl.org/sdk/docs/man3/xhtml/glBufferSubData.xml
	//"Consider using multiple buffer objects to avoid stalling the rendering pipeline during data store updates.
	//If any rendering in the pipeline makes reference to data in the buffer object being updated by glBufferSubData, especially
	//from the specific region being updated, that rendering must drain from the pipeline before the data store can be updated."
	//TODO the above is per-instance model matrix. We additionally need per-instance ID so we don't have to repeat ID ad nauseam as a vertex attribute.
	
	//bind vertex array & draw
	glDrawElementsInstanced(program->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index, renderableSet->count);
	//TODO optimise draw call by reducing index type for character parts(!) to only use GL_UNSIGNED_BYTE if possible,
	//i.e. 0-255 vertices - could be faster, see http://www.songho.ca/opengl/gl_vertexarray.html, search on "maximum".

	glBindVertexArray(0);	
	glUseProgram(0);
}
*/
//TODO should pass Mesh instead of RenderableSet, though in same arg position.
//TODO instead of matM, a void * arg pointing to wherever all the uniforms for this object lie. same for attributes?
void Hedgehog_renderOne(Hedgehog * this, Renderable * renderable, const GLfloat * matM, const GLfloat * matVP)
{
	Mesh * mesh = renderable->mesh;

	glBindVertexArray(mesh->vao);
	
	//prep uniforms...
	//...view-projection matrix
	GLint vpLoc = glGetUniformLocation(this->program->id, "vp");
	glUniformMatrix4fv(vpLoc, 1, GL_FALSE, (GLfloat *)matVP);
	//...model matrix
	GLint mLoc = glGetUniformLocation(this->program->id, "m");
	glUniformMatrix4fv(mLoc, 1, GL_FALSE, (GLfloat *)matM);
	
	if (mesh->index == NULL)
		glDrawArrays(mesh->topology, 0, mesh->vertexCount);
	else
		glDrawElements(mesh->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index);
	
	glBindVertexArray(0);
}


void Hedgehog_initialise(Hedgehog * this)
{
	voidPtrMap_create(&this->programsByName,	HH_PROGRAMS_MAX, 	&this->programKeys, 	(void *)&this->programs, NULL);
	voidPtrMap_create(&this->shadersByName, 	HH_SHADERS_MAX, 	&this->shaderKeys, 		(void *)&this->shaders, NULL);
	voidPtrMap_create(&this->texturesByName, 	HH_TEXTURES_MAX, 	&this->textureKeys, 	(void *)&this->textures, NULL);
	voidPtrMap_create(&this->materialsByName, 	HH_MATERIALS_MAX, 	&this->materialKeys, 	(void *)&this->materials, NULL);
	
	//reintroduce if we bring transform list back into hedgehog.
	//Renderable * renderable = &this->renderable;
	//for (int i = 0; i < transformsCount; i++)
	//	mat4x4_identity(renderable->matrix[i]);
}

Program * Hedgehog_setCurrentProgram(Hedgehog * this, char * name)
{
	//printf("name=%s\n", name);
	if (name == NULL)
	{
		this->program = NULL;
		glUseProgram(0);
	}
	else
	{
		this->program = (Program *)get(&this->programsByName, *(uint64_t *) pad(name));
		assert (this->program != NULL);
		glUseProgram(this->program->id);
	}
	return this->program;
}

Program * Hedgehog_getCurrentProgram(Hedgehog * this)
{
	return this->program;
}
	
//------------------TOOLS------------------//

bool isExtensionSupported(const char * extension)
{
    if ( glGetStringi != NULL ) // Use the post-3.0 core profile method for querying extensions.
    {
        GLint numExtensions = 0;
        glGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		const char * extensions[numExtensions];
        for( int i = 0; i < numExtensions; ++i )
        {
            extensions[i] = (const char *)glGetStringi( GL_EXTENSIONS, i );
			//printf("EXT: %s\n", extensions[i]); 
			if (strstr(extensions[i], extension))
				return true;
        }
    }
    else // Fall-back to the pre-3.0 or 3.0+ compatibility profile method for querying extensions.
    {
		return NULL != strstr( (char const*)glGetString( GL_EXTENSIONS ), extension );
    }
     
	return false;
}

char* Text_load(char* filename)
{
	//should be portable - http://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer
	FILE *file = fopen(filename, "rb"); //open for reading
	fseek(file, 0, SEEK_END); //seek to end
	long fileSize = ftell(file); //get current position in stream
	fseek(file, 0, SEEK_SET); //seek to start
	printf(filename);
	printf("? %ld", fileSize);

	char *str = malloc(fileSize + 1); //allocate enough room for file + null terminator (\0)

	if (str != NULL) //if allocation succeeded
	{
		printf("fileSize...%ld\n", fileSize);
		size_t freadResult;
		freadResult = fread(str, 1, fileSize, file); //read elements as one byte each, into string, from file. 
		//printf("freadResult...%d\n", freadResult);
		
		if (freadResult != fileSize)
		{
			fputs ("Reading error", stderr);
			//exit (3);
			
			str = NULL;
			return str;
		}
		
		fclose(file);

		str[fileSize] = 0;//'\0'; //last element is null termination.
	}
	
	return str;
}

float Hedgehog_smoothstep(float t)
{
	return 3 * t * t - 2 * t * t * t;
}