#include "Hedgehog.h"

//TODO see "I/O callbacks" in stbi_image.h for loading images out of a data file
void GLFW_errorCallback(int error, const char * description)
{
    //fputs(description, stderr);
	printf ("GLFW ERROR: code %i msg: %s\n", error, description);
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

void Transform_update(Transform* this)
{
	/*
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
	*/

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

Texture * Texture_construct()
{
	Texture * texture = malloc(sizeof(Texture));
	return texture;
}

Texture * Texture_load(const char * filename)
{
	Texture * texture = Texture_construct();
	
	texture->data = stbi_load(filename, &(texture->width), &(texture->height), &(texture->components), 0);
	
	return texture;
}

/** Returns the OpenGL internally-used constant for a given zero-based texture unit ordinal. */
GLuint Texture_getTextureUnitConstant(Texture * this)
{
	return this->unit + GL_TEXTURE0;
}

bool Texture_setParametersStandard(Texture * texture, int unit, bool repeat)
{
	if (unit > 31 || unit < 0)
		return false; //Texture unit ordinal must be in range 0-31.

	texture->unit = unit; //or must we access the original?
	
	glActiveTexture(GL_TEXTURE0 + unit); //eg. 0 + 33984 = GL_TEXTURE0, while 31 + 33984 = GL_TEXTURE31.
	glBindTexture(GL_TEXTURE_2D, texture->id);
	//glPixelStorei(GL_PACK_ALIGNMENT, 4);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//http://stackoverflow.com/questions/5885223/android-opengl-es-texture-mapping-drawing-problem-skewed-images
	//TODO check for GL error using glew/glfw
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data); //after glTexParameteri? https://www.opengl.org/discussion_boards/showthread.php/133040-glTexParameteri-before-glTexImage2D
	//TODO check for GL error using glew/glfw

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//TODO check for GL error using glew/glfw
	
	glBindTexture(GL_TEXTURE_2D, 0); //unbind
	
	return true;
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

void Renderer_clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer_clearColor()
{
	 glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer_clearDepth()
{
	 glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer_instance(Program * program, MeshInstances * meshInstances, const GLfloat * matVP)
{
	Mesh * mesh = meshInstances->mesh;

	glUseProgram(program->id);
	glBindVertexArray(mesh->vao);
	
	//prep uniforms...(general to all instances)
	//...view-projection matrix (general to all instancing approaches)
	GLint vpLoc = glGetUniformLocation(program->id, "vp");
	glUniformMatrix4fv(vpLoc, 1, GL_FALSE, (GLfloat *)matVP);
	
	//upload instance data
	glBindBuffer(GL_ARRAY_BUFFER, meshInstances->buffer); //TODO use const instead of literal buffer name
	glBufferData(GL_ARRAY_BUFFER, sizeof(mat4x4) * meshInstances->count, meshInstances->data, GL_DYNAMIC_DRAW); 
	//TODO specify usage (e.g. GL_DYNAMIC_DRAW) on the Mesh? Or better yet, on the particular Renderable using Mesh.
	//TODO glBufferSubData(GL_ARRAY_BUFFER,
	//TODO consider storing 2 buffers for each dynamic object - from https://www.opengl.org/sdk/docs/man3/xhtml/glBufferSubData.xml
	//"Consider using multiple buffer objects to avoid stalling the rendering pipeline during data store updates.
	//If any rendering in the pipeline makes reference to data in the buffer object being updated by glBufferSubData, especially
	//from the specific region being updated, that rendering must drain from the pipeline before the data store can be updated."
	//TODO the above is per-instance model matrix. We additionally need per-instance ID so we don't have to repeat ID ad nauseam as a vertex attribute.
	
	//bind vertex array & draw
	glDrawElementsInstanced(program->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index, meshInstances->count);
	//TODO optimise draw call by reducing index type for character parts(!) to only use GL_UNSIGNED_BYTE if possible,
	//i.e. 0-255 vertices - could be faster, see http://www.songho.ca/opengl/gl_vertexarray.html, search on "maximum".

	glBindVertexArray(0);	
	glUseProgram(0);
}

//TODO should pass Mesh instead of MeshInstances, though in same arg position.
//TODO instead of matM, a void * arg pointing to wherever all the uniforms for this object lie. same for attributes?
void Renderer_single(Program * program, GLuint vao, const GLfloat * matVP, const GLvoid * indices, int elementCount, const GLfloat * matM)
{
	glUseProgram(program->id);
	glBindVertexArray(vao);
	
	//prep uniforms...
	//...view-projection matrix
	GLint vpLoc = glGetUniformLocation(program->id, "vp");
	glUniformMatrix4fv(vpLoc, 1, GL_FALSE, (GLfloat *)matVP);
	//...model matrix
	GLint mLoc = glGetUniformLocation(program->id, "m");
	glUniformMatrix4fv(mLoc, 1, GL_FALSE, (GLfloat *)matM);
	
	glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_SHORT, indices);
	
	glBindVertexArray(0);
	glUseProgram(0);
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
	printf("abc");
	printf("? %ld", fileSize);

	char *str = malloc(fileSize + 1); //allocate enough room for file + null terminator (\0)

	if (str != NULL) //if allocation succeeded
	{
		printf("fileSize...%ld\n", fileSize);
		size_t freadResult;
		freadResult = fread(str, 1, fileSize, file); //read elements as one byte each, into string, from file. 
		printf("freadResult...%d\n", freadResult);
		
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

