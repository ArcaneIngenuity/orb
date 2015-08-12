#include "orb.h"

#define KEYPARTS 1

void Window_terminate(Engine * engine)
{
	#ifdef DESKTOP
	glfwTerminate();//free()s any windows
	window = NULL; //JIC
	#elif MOBILE
	#ifdef __ANDROID__
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
	#endif//__ANDROID__
	#endif//DESKTOP/MOBILE
}

/*
void DeviceChannel_shift(DeviceChannel * this)
{
	int length = this->historyLength;
	
	//TODO one loop would work here if the two arrays were known to be of equal length... could this be made an option?
	
	//set all later values by shifting forward
	int states = this->states;
	for (int i = 0; i < states.length - 1; i++)
	{
		DeviceChannel_shiftState(this, i, i+1);
	}
	
	//set all later values by shifting forward
	var deltas = this.deltas;
	for (int i = 0; i < deltas.length - 1; i++)
	{
		DeviceChannel_shiftDelta(this, i, i+1);
	}
	
}
*/
void DeviceChannel_setCurrentDelta(DeviceChannel * this)
{
	this->delta[CURRENT] = this->state[CURRENT] - this->state[PREVIOUS];
}

void DeviceChannel_setPreviousDelta(DeviceChannel * this)
{
	this->delta[PREVIOUS] = this->delta[CURRENT];
}

void DeviceChannel_setCurrentState(DeviceChannel * this)
{
	this->state[CURRENT] = this->state[PREVIOUS] + this->delta[CURRENT];
}

void DeviceChannel_setPreviousState(DeviceChannel * this)
{
	this->state[PREVIOUS] = this->state[CURRENT];
}

#define CURT_SOURCE

#define CURT_ELEMENT_STRUCT
#define CURT_ELEMENT_TYPE InputResponse
#include "pod/list.h"
#undef  CURT_ELEMENT_TYPE
#undef  CURT_ELEMENT_STRUCT

#undef  CURT_SOURCE

void InputResponse_executeList(InputResponseList * list, void * target)
{
for (int i = 0; i < list->length; i++)
	{
		InputResponse * inputResponse = &list->entries[i];

		if (inputResponse->channelPos != NULL) //mandatory positive input contributor
		{
			if (inputResponse->basis == STATE)
			{
				//LOGI("STATE");
				float pos = inputResponse->channelPos->state[CURRENT];
				float neg = 0;
				if (inputResponse->channelNeg != NULL) //optional negative input contributor
					neg = inputResponse->channelNeg->state[CURRENT];

				inputResponse->state[PREVIOUS] = inputResponse->state[CURRENT];
				inputResponse->state[CURRENT]  = pos - neg; //assumes both are abs magnitudes
				
				inputResponse->response(target, inputResponse->state[CURRENT], inputResponse->state[PREVIOUS]);
			}
			else //(inputResponse->basis = DELTA) //only trigger on a change between the two values
			{
				//LOGI("DELTA");
				float pos = inputResponse->channelPos->delta[CURRENT];
				float neg = 0;
				if (inputResponse->channelNeg != NULL) //optional negative input contributor
					neg = inputResponse->channelNeg->delta[CURRENT];

				inputResponse->delta[PREVIOUS] = inputResponse->delta[CURRENT];
				inputResponse->delta[CURRENT]  = pos - neg; //assumes both are abs magnitudes
				
				if (inputResponse->delta[CURRENT] != 0)
					inputResponse->response(target, inputResponse->delta[CURRENT], inputResponse->delta[PREVIOUS]);
			}
		}
		else
		{
			if (inputResponse->channelNeg == NULL) //both NULL? always execute
			{
				inputResponse->response(target, 0, 0);
			}
		}
	}
}

bool InputResponse_equals(InputResponse a, InputResponse b) //TODO make equals a function pointer in list.h
{
	return false; //DEV no members yet
}

void Loop_processInputs(Engine * engine)
{
	#ifdef DESKTOP //glfw!
	//relative to mouse start point: For FPS
	double p[2];
	glfwGetCursorPos(window, &p[XX], &p[YY]);
	DeviceChannel * channel;
	Device * mouse = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
	for (int i = 0; i < 2; i++)
	{
		channel = &mouse->channels[i];
		channel->state[CURRENT] = p[i];
		DeviceChannel_setCurrentDelta(channel);
		DeviceChannel_setPreviousState(channel);
	}
	LOGI("x=%.3f y=%.3f\n", p[XX], p[YY]);
	
	Device * keyboard = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("keyboard"));
	
	//space
	channel = &keyboard->channels[0];
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	DeviceChannel_setPreviousState(channel);
	
	//S
	channel = &keyboard->channels[1];
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	DeviceChannel_setPreviousState(channel);
	
	//W
	channel = &keyboard->channels[2];
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	DeviceChannel_setPreviousState(channel);
	
	//D
	channel = &keyboard->channels[3];
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	DeviceChannel_setPreviousState(channel);
	
	//A
	channel = &keyboard->channels[4];
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	DeviceChannel_setPreviousState(channel);
	
	#endif//DESKTOP
}

void Loop_initialise(Engine * engine)
{
	#ifdef __ANDROID__
	//app_dummy();
	struct android_app * app = engine->app;
	app->userData = engine;
	app->onAppCmd = Android_onAppCmd;
	app->onInputEvent = Android_onInputEvent;

	#endif//__ANDROID__
	#ifdef DESKTOP
	Engine_initialise(engine); 
	engine->userInitialiseFunc(); //!!! For now, Ctrl_init must go after Engine_initialise(), because it still relies on glfw for input AND glReadPixels
	#endif//DESKTOP
}

void Loop_run(Engine * engine)
{
	#ifdef DESKTOP
	double t1, t2 = 0;\
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		Loop_processInputs(engine);
		
		t2 = t1;\
		t1 = glfwGetTime();
		engine->deltaSec = t1 - t2;
		//printf("deltaSec %.10f\n", deltaSec);

		//TODO
		engine->userUpdateFunc(engine->userUpdateArg);
		
		glfwSwapBuffers(window);
	}
	#elif MOBILE
	#ifdef __ANDROID__
	while (1)
	{
		int ident;
		int events;
		struct android_poll_source* source;
		struct android_app* state = engine->app;
		

		if (engine->initialisedWindow) //TODO make this a function pointer set when cmd init occurs to avoid branch
		{

		//we must flush input to get rid of old deltas / states or they will persist
		//do so BEFORE event loop to ensure event values don't get overridden
		Device * device = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
		
		for (int i = 0; i < 2; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			//DeviceChannel_setCurrentDelta(channel);
			DeviceChannel_setPreviousState(channel);
			channel->state[CURRENT] = 0;
			//channel->delta[CURRENT] = 0;
		}
		}
		
		while ((ident=ALooper_pollAll(0, NULL, &events,(void**)&source)) >= 0)
		{
			if (source != NULL)
			{
				source->process(state, source);
			}
			if (state->destroyRequested != 0)
			{
				Window_terminate(engine);
				exit(0);
			}
		}
		
		if (engine->initialisedWindow) //TODO make this a function pointer set when cmd init occurs to avoid branch
		{

		//we must flush input to get rid of old deltas / states or they will persist
		//do so BEFORE event loop to ensure event values don't get overridden
		Device * device = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
		
		for (int i = 0; i < 2; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			DeviceChannel_setCurrentDelta(channel);
		}
		}
		
		//TODO if accumulated sufficient time
		Android_frame(engine);
	}
	#endif//__ANDROID__
	#endif//DESKTOP/MOBILE
}

#ifdef __ANDROID__
int32_t Android_onInputEvent(struct android_app* app, AInputEvent* event)
{
	Engine * engine = (Engine *)app->userData;
	Device * device = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
	
	switch (AInputEvent_getType(event))
	{
	case AINPUT_EVENT_TYPE_MOTION:
		float p[2];
		p[XX] = AMotionEvent_getX(event, 0);
		p[YY] = AMotionEvent_getY(event, 0);
	
		for (int i = 0; i < 2; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			channel->state[CURRENT] = p[i];
			//DeviceChannel_setCurrentDelta(channel);
			//DeviceChannel_setPreviousState(channel);
		}

		LOGI("x %f\ty %f\n",p[XX], p[YY]);
		return 1;
		break;
	case ACTION_DOWN:
		//set last state to current, so first delta on touch will be 0
		for (int i = 0; i < 2; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			channel->_active = true;
		}
		break;
	case ACTION_UP:
		for (int i = 0; i < 2; i++)
		{
			//set  delta to zero
			DeviceChannel * channel = &device->channels[i];
			channel->_active = false;
		}
		break;
	case ACTION_POINTER_DOWN:
		//set last state to current, so first delta on touch will be 0
		for (int i = 0; i < 2; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			//channel->_active = true;
		}
		break;
	case ACTION_POINTER_UP:
		for (int i = 0; i < 2; i++)
		{
			//set  delta to zero
			DeviceChannel * channel = &device->channels[i];
			//channel->_active = false;
		}
		break;
	}
	return 0;
}

void Android_frame(Engine * engine)
{
	// No display.
	if (engine->display == NULL) return;
	
	engine->userUpdateFunc(engine->userUpdateArg);
	
	eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Process the next main command.
 */
void Android_onAppCmd(struct android_app* app, int32_t cmd)
{
	Engine * engine = (struct engine*)app->userData;
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (engine->app->window != NULL)
		{
			Engine_initialise(engine);

			engine->userInitialiseFunc(); //TODO - call from within Engine_initialise()?
			
			Android_frame(engine);
			
			engine->initialisedWindow = true;
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		Window_terminate(engine);
		break;
	case APP_CMD_LOST_FOCUS:
		Android_frame(engine);
		break;
	}
}
#endif//__ANDROID__
#ifdef DESKTOP
//TODO see "I/O callbacks" in stbi_image.h for loading images out of a data file
void GLFW_errorCallback(int error, const char * description)
{
    //fputs(description, stderr);
	LOGI ("GLFW ERROR: code %i msg: %s\n", error, description);
}
#endif//DESKTOP

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
	#ifdef DESKTOP
	texture->data = stbi_load(filename, &(texture->width), &(texture->height), &(texture->components), 0);
	#endif//DESKTOP
	return texture;
}

void RenderTexture_createDepth(Texture * const this, GLuint i, uint16_t width, uint16_t height)
{
	this->unit = i;
	this->width = width;
	this->height = height;
	
	Texture_setDimensionCount(this, GL_TEXTURE_2D);
	Texture_setTexelFormats(this, GL_DEPTH_COMPONENT16, GL_FLOAT);
	
	intMap_put(&this->intParametersByName, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	intMap_put(&this->intParametersByName, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	intMap_put(&this->intParametersByName, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	intMap_put(&this->intParametersByName, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	Texture_applyParameters(this);
	
	glActiveTexture(GL_TEXTURE0 + this->unit); //"which texture unit a texture object is bound to when glBindTexture is called."
	glBindTexture(GL_TEXTURE_2D, this->id); //binds the texture with id specified, to the 2D target
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16 , this->width, this->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
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
		//case GL_DEPTH_COMPONENT24:  this->components = 1; break;
		//case GL_DEPTH_COMPONENT32F: this->components = 1; break;
		case GL_RED:  				this->components = 1; break;
		case GL_RG:   				this->components = 2; break;
		case GL_RGB: 
		case GL_BGR:  				this->components = 3; break;
		case GL_RGBA:
		case GL_BGRA: 				this->components = 4; break;
		default:
			LOGI("Error: can accept only GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA or GL_BGRA.\n");
			exit(0);
	}
}

void Texture_setDimensionCount(Texture * this, GLenum dimensions)
{
	LOGI("set dimensiosn to %d\n", dimensions);
	this->dimensions = dimensions;
}

void Texture_applyParameters(Texture * this)
{
	LOGI("unit %u\n",this->unit);
	glActiveTexture(GL_TEXTURE0 + this->unit);
	LOGI("e0 %s\n",glGetError());
	glBindTexture(GL_TEXTURE_2D, this->id);
	LOGI("e1 %s\n",glGetError());
	LOGI("b.2\n");
	for (uint8_t p = 0; p < this->intParametersByName.count; p++)
	{
		GLenum key = this->intParametersByName.keys[p];
		GLint value = this->intParametersByName.entries[p];
		LOGI("GL_TEXTURE_2D %i\n", GL_TEXTURE_2D);
		LOGI("GL_TEXTURE_MIN_FILTER GL_NEAREST %u %i\n", GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		LOGI("GL_TEXTURE_MAG_FILTER GL_NEAREST %u %i\n", GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		LOGI("GL_TEXTURE_WRAP_S 	  GL_REPEAT  %u %i\n", GL_TEXTURE_WRAP_S, GL_REPEAT);
		LOGI("GL_TEXTURE_WRAP_T 	  GL_REPEAT  %u %i\n", GL_TEXTURE_WRAP_T, GL_REPEAT);
		LOGI("ber %u %u %i\n", this->dimensions, key, value);
		LOGI("glTexParameteri == NULL? %u\n", glTexParameteri == NULL);
		glTexParameteri(this->dimensions, key, value);
		//LOGI("berrror %s\n",glGetError());
	}
	LOGI("b.3\n");
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
	LOGI("buffer id %i\n", buffer);
    return buffer;
}

//------------------Shader------------------//
void Shader_load(Engine * this, const char * path, const char * name)
{
	Shader * vert;
	Shader * frag;
	Program * program;
	LOGI("this NULL?=%d", this==NULL);
	if (strlen(name) > KEYPARTS * 8 - 1)
	{
		LOGI("[Shader_load] Length of shader name must be less than or equal to KEYPARTS * 8 - 1.");
		exit(0);
	}
	vert = malloc(sizeof(Shader));
	frag = malloc(sizeof(Shader));
	vert->type = GL_VERTEX_SHADER;
	frag->type = GL_FRAGMENT_SHADER;
	
	size_t lengthName = strlen(name); 
	size_t lengthPath = strlen(path);
	
	char vertFilepath[lengthPath+lengthName+1+4]; //5 = .vert
	strcpy(vertFilepath, path);
	strcat(vertFilepath, name);
	strcat(vertFilepath, ".vert");
	LOGI("vertFilepath %s\n", vertFilepath);
	vert->source = Text_load(vertFilepath);
	LOGI("vert %s\n\n", vert->source);
	Shader_construct(vert);
	
	char fragFilepath[lengthPath+lengthName+1+4]; //5 = .frag
	strcpy(fragFilepath, path);
	strcat(fragFilepath, name);
	strcat(fragFilepath, ".frag");
	LOGI("fragFilepath %s\n", fragFilepath);
	frag->source = Text_load(fragFilepath);
	LOGI("frag %s\n\n", frag->source);
	Shader_construct(frag);
	
	int final = lengthName < 8 ? lengthName : 8 - 1;
	
	char vertKey[8 + 1] = {0};
	strncpy(vertKey, name, 8);
	vertKey[final] = 'v'; //set last character to distinguish. TODO should be either first null or last character.
	LOGI("v=%s\n", vertKey);
	//TODO check whether key exists
	put(&this->shadersByName, *(uint64_t *) vertKey, vert); //diffusev
	
	char fragKey[8 + 1] = {0};
	strncpy(fragKey, name, 8);
	fragKey[final] = 'f'; //set last character to distinguish. TODO should be either first null or last character.
	LOGI("f=%s\n", fragKey);
	//TODO check whether key exists
	put(&this->shadersByName, *(uint64_t *) fragKey, frag); //diffusef
	
	program = malloc(sizeof(Program));
	Program_construct(program, vert->id, frag->id);
	program->topology = GL_TRIANGLES;
	LOGI("program %d", program);
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
	GLint compiled;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);

	if(!compiled)
	{
		
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		LOGI("glCompileShader() failed:\n");
		if(infoLogLength > 1)
		{
			//GLchar infoLog[sizeof(char) * infoLogLength + 1];
			char* infoLog = malloc(sizeof(char) * infoLogLength);
			glGetShaderInfoLog(id, infoLogLength, NULL, infoLog);
			LOGI("error: %s\n", infoLog);
			LOGI("source: \n%s\n", this->source);
	
			//TODO encapsulate the below in a exitOnFatalError() that can be used anywhere.
			//TODO set up error codes for untimely exit.
			#ifdef DESKTOP
			glfwTerminate();
			#endif//DESKTOP
			exit(-1);
			
			glDeleteShader(id);
			
			free(infoLog);
		}
		else
		{
			LOGI("<no GL info log available>\n");
		}
	}
	else
	{
		LOGI("glCompileShader() success.\n");
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		if(infoLogLength > 1)
		{
			GLchar infoLog[sizeof(char) * infoLogLength + 1];
			glGetShaderInfoLog(id, infoLogLength + 1, NULL, infoLog);
			LOGI("%s\n", infoLog, this->source);
		}
		else
		{
			LOGI("<no GL info log available>\n");
		}
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
	LOGI("gl error %i\n", glGetError());
	
	glBindAttribLocation(id, 0, "position");
	glBindAttribLocation(id, 1, "texcoord");
	
	// Link program and check success
	glLinkProgram(id);
	
	GLint linked;
	glGetProgramiv(id, GL_LINK_STATUS, &linked);
	
	if (!linked)
	{
		LOGI("glLinkProgram() failed:\n");
		
		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		if (infoLogLength > 1)
		{
			LOGI("infoLogLength=%i\n", infoLogLength);
			//GLchar infoLog[sizeof(char) * infoLogLength + 1];
			char* infoLog = malloc(sizeof(char) * infoLogLength);
			glGetProgramInfoLog(id, infoLogLength, NULL, infoLog);
			LOGI("%s\n", infoLog);
			
			#ifdef DESKTOP
			glfwTerminate();
			#endif//DESKTOP
			exit(-1);
			
			//TODO do these atexit
			//Delete shaders; won't take effect unless they are detached first.
			glDetachShader(id, vertex_shader);
			glDetachShader(id, fragment_shader);
			
			if (vertex_shader)
				glDeleteShader(vertex_shader);
			if (fragment_shader)
				glDeleteShader(fragment_shader);
			
			glDeleteProgram(id);
			
			free(infoLog);
		}
		else
		{
			LOGI("<no GL info log available>\n");
		}
	}
	else
		LOGI("glLinkProgram() success.\n");
}
/*
bool linkProgramSuccess(int program)
{
	int status;
	
	
	if (!status)
	{
		GLint infoLogLength;
		glGetProgramiv(program,GL_INFO_LOG_LENGTH,&infoLogLength);
		GLchar infoLog[infoLogLength + 1];
		//LOGI("info log length:%i\n", infoLogLength);
		glGetProgramInfoLog(program, infoLogLength + 1, NULL, infoLog);
		LOGI("glLinkProgram() failed: %s\n", infoLog);
	}
	LOGI("status %i %i %i\n", status, GL_TRUE, GL_FALSE);
	return status == GL_TRUE;
}
*/
//--------------Render-------------------//


void Engine_clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Engine_clearColor()
{
	 glClear(GL_COLOR_BUFFER_BIT);
}

void Engine_clearDepth()
{
	 glClear(GL_DEPTH_BUFFER_BIT);
}

void Engine_createFullscreenQuad(Engine * this, GLuint positionVertexAttributeIndex, GLuint texcoordVertexAttributeIndex)
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
	
	LOGI("index %d %d\n", sizeof(_index), mesh->indexCount * 2);
	LOGI("pos   %d %d\n", sizeof(_position), mesh->vertexCount * 4);
	LOGI("texco %d %d\n", sizeof(_texcoord), mesh->vertexCount * 4);
	
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

void Engine_createScreenQuad(Mesh * mesh, GLuint positionVertexAttributeIndex, GLuint texcoordVertexAttributeIndex,
	int w, int h,
	int rcx, int rcy
)
{
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
	
	float wabs = (float)w/1024;
	float habs = (float)h/768;
	
	GLfloat _position[8] = {
		-wabs, -habs,
		+wabs, -habs,
		+wabs, +habs,
		-wabs, +habs
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
	
	LOGI("0index %d %d\n", sizeof(_index), mesh->indexCount * 2);
	LOGI("0pos   %d %d\n", sizeof(_position), mesh->vertexCount * 4);
	LOGI("0texco %d %d\n", sizeof(_texcoord), mesh->vertexCount * 4);
	
	//gen & bind
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
void Engine_many(Program * program, RenderableSet * renderableSet, const GLfloat * matVP)
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
void Engine_one(Engine * this, Renderable * renderable, const GLfloat * matM, const GLfloat * matVP)
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

void Engine_oneUI(Engine * this, Renderable * renderable, const GLfloat * matM)
{
	Mesh * mesh = renderable->mesh;
	//LOGI("mesh == NULL? %i", mesh==NULL);
	//LOGI("glBindVertexArray == NULL? %i", glBindVertexArray==NULL);
	//LOGI("??? %i", mesh->vao);

	glBindVertexArray(mesh->vao);

	//prep uniforms...
	//...model matrix
	GLint mLoc = glGetUniformLocation(this->program->id, "m");
	glUniformMatrix4fv(mLoc, 1, GL_FALSE, (GLfloat *)matM);
	
	if (mesh->index == NULL)
		glDrawArrays(mesh->topology, 0, mesh->vertexCount);
	else
		glDrawElements(mesh->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index);
	
	glBindVertexArray(0);
}

void Engine_initialise(Engine * this)
{
	LOGI("Engine initialising...\n");
	#ifdef DESKTOP
	//WINDOW, CONTEXT & INPUT
	//glfwSetErrorCallback(GLFW_errorCallback);
	
	if (!glfwInit())
		exit(EXIT_FAILURE);
	else
		printf("GLFW initialised.\n");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_SAMPLES, 4); //HW AA
	//glfwWindowHint(GLFW_REFRESH_RATE, 10);
	
	window = glfwCreateWindow(1024, 768, "War & Adventure Pre-Alpha", NULL, NULL);

	if (!window)
	{
		printf("GLFW could not create window.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	printf("OPENGL ERROR %i\n", glGetError());
	printf("OpenGL ver. %i.%i\n", glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR), glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR ));
	
	glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
	
	//set callback for later window resize events
	//glfwSetWindowSizeCallback(window, onWindowResize);
	
	//EXTENSIONS
	glewExperimental = GL_TRUE; 
	
	printf("OPENGL ERROR %i\n", glGetError());

	if (glewInit())
	{	
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("GLEW initialised.\n");
	}
	printf("OPENGL ERROR %i\n", glGetError());
	
	//some debug info on texture capabilities
	GLint maxSize, maxUnits, maxAttachments;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttachments);
	printf("GL_MAX_TEXTURE_SIZE=%d\n", maxSize);
	printf("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS=%d\n", maxUnits);
	printf("GL_MAX_COLOR_ATTACHMENTS=%d\n", maxAttachments);
	
	#elif MOBILE
	#if __ANDROID__

	ANativeActivity* activity = this->app->activity;
    JNIEnv* env = activity->env;

	// Setup OpenGL ES 2
	// http://stackoverflow.com/questions/11478957/how-do-i-create-an-opengl-es-2-context-in-a-native-activity

	const EGLint attribs[] = {
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, //important
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_NONE
	};

	EGLint attribList[] =
	{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
	};

	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(this->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, this->app->window, NULL);

	context = eglCreateContext(display, config, NULL, attribList);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}
	else
		LOGI("Success eglMakeCurrent");

	// Grab the width and height of the surface
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	this->display = display;
	this->context = context;
	this->surface = surface;
	this->width = w;
	this->height = h;

	// Initialize GL state.
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, w, h);
	
	//EXTENSIONS
	char * extensions = glGetString(GL_EXTENSIONS);
	char * vaoExt = "GL_OES_vertex_array_object";
	
	if (strstr(extensions, vaoExt) != NULL)
	{
		LOGI("GL_OES_vertex_array_object FOUND!");
		
		void * libhandle = dlopen("libGLESv2.so", RTLD_LAZY);
		if (libhandle)
		{
			LOGI("libGLESv2 dlopen()ed; retrieving extension.");
		
			glBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC) 		dlsym(libhandle, "glBindVertexArrayOES");
			glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSOESPROC) dlsym(libhandle, "glDeleteVertexArraysOES");
			glGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC)		dlsym(libhandle, "glGenVertexArraysOES");
			glIsVertexArray = (PFNGLISVERTEXARRAYOESPROC)			dlsym(libhandle, "glIsVertexArrayOES");
			
			LOGI("BindVertexArrayOES? %s", glBindVertexArray ? "YES" : "NO");										
			LOGI("DeleteVertexArraysOES? %s", glDeleteVertexArrays ? "YES" : "NO");										
			LOGI("GenVertexArraysOES? %s", glGenVertexArrays ? "YES" : "NO");										
			LOGI("IsVertexArrayOES? %s", glIsVertexArray ? "YES" : "NO");	
		}
		else
		{
			LOGI("libGLESv2 did not dlopen(); could not retrieve extension.");
			
			//TODO quit
		}
		
	}
	else
	{
		//TODO check all rendering options - GL3, GLES3, GL2, GLES2 without VAOs 
		
		LOGI("GL_OES_vertex_array_object MISSING!");
		
		//TODO quit
	}
	
	#endif//__ANDROID__
	#endif//DESKTOP/MOBILE
	
	//MISCELLANEOUS
	
	//initialise collection objects
	//TODO "this" should be "orb" instance
	//TODO rename HH_ constants to ORB_
	voidPtrMap_create(&this->programsByName,	HH_PROGRAMS_MAX, 	&this->programKeys, 	(void *)&this->programs, NULL);
	voidPtrMap_create(&this->shadersByName, 	HH_SHADERS_MAX, 	&this->shaderKeys, 		(void *)&this->shaders, NULL);
	voidPtrMap_create(&this->texturesByName, 	HH_TEXTURES_MAX, 	&this->textureKeys, 	(void *)&this->textures, NULL);
	voidPtrMap_create(&this->materialsByName, 	HH_MATERIALS_MAX, 	&this->materialKeys, 	(void *)&this->materials, NULL);
	voidPtrMap_create(&this->meshesByName, 		HH_MESHES_MAX, 		&this->meshKeys,	 	(void *)&this->meshes, NULL);
	voidPtrMap_create(&this->devicesByName, 	2, 					&this->deviceKeys,	 	(void *)&this->devices, NULL);
	LOGI("map count=%i", this->programsByName.count);

	//reintroduce if we bring transform list back into this library.
	//Renderable * renderable = &this->renderable;
	//for (int i = 0; i < transformsCount; i++)
	//	mat4x4_identity(renderable->matrix[i]);

	LOGI("Engine initialised.\n");
}

void Engine_dispose(Engine * engine)
{
	//TODO free engine collections.
	
	#ifdef DESKTOP
	Window_terminate(&engine);
	#endif//DESKTOP
}

Program * Engine_setCurrentProgram(Engine * this, char * name)
{
	//LOGI("name=%s\n", name);
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

Program * Engine_getCurrentProgram(Engine * this)
{
	return this->program;
}
	
//------------------TOOLS------------------//
/*
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
			//LOGI("EXT: %s\n", extensions[i]); 
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
*/
char* Text_load(char* filename)
{
	//should be portable - http://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer
	
	char * str;
	
	
			

	//#ifdef DESKTOP
	FILE *file = fopen(filename, "rb"); //open for reading
	if (file)
	{

		
		fseek(file, 0, SEEK_END); //seek to end
		long fileSize = ftell(file); //get current position in stream
		fseek(file, 0, SEEK_SET); //seek to start
		LOGI(filename);
		LOGI("? %ld", fileSize);
		
		
		str = malloc(fileSize + 1); //allocate enough room for file + null terminator (\0)

		if (str != NULL) //if allocation succeeded
		{
			
			LOGI("fileSize...%ld\n", fileSize);
			size_t freadResult;
			freadResult = fread(str, 1, fileSize, file); //read elements as one byte each, into string, from file. 
			//LOGI("freadResult...%d\n", freadResult);
			
			if (freadResult != fileSize)
			{
				fputs ("Reading error", stderr);
				//exit (3);
				
				str = NULL;
				return str;
			}
			
			fclose(file);

			str[fileSize] = 0; //'\0';
		}
	}
	else
		LOGI("File not found: %s", filename);
	
	
	
	//#elif __ANDROID__

	//#endif //platforms
	
	return str;
}

float Engine_smoothstep(float t)
{
	return 3 * t * t - 2 * t * t * t;
}