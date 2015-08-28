#include "orb.h"

#define KEYPARTS 1

void Window_terminate(Engine * engine)
{
	#ifdef DESKTOP
	glfwTerminate();//free()s any windows
	window = NULL; //JIC
	#elif MOBILE
	#ifdef __ANDROID__
	if (engine->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT)
		{
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE)
		{
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
#define CURT_ELEMENT_TYPE Input
#include "pod/list.h"
#undef  CURT_ELEMENT_TYPE
#undef  CURT_ELEMENT_STRUCT

#undef  CURT_SOURCE

void Input_executeList(InputList * list, void * target, bool debug)
{
	float pos = 0, neg = 0;

	for (int i = 0; i < list->length; i++)
	{
		Input * input = &list->entries[i];

		if (!input->channelPos && !input->channelNeg) //"always-run" response
			input->response(target, 0, 0);
		else
		{
			switch (input->basis)
			{
			case STATE: //only call response when channel state [CURRENT] is non-zero
				
				if (input->channelPos)
					pos = input->channelPos->state[CURRENT] * input->channelPos->active;
				
				if (input->channelNeg) //optional negative input contributor
					neg = input->channelNeg->state[CURRENT] * input->channelNeg->active;

				input->state[PREVIOUS] = input->state[CURRENT];
				input->state[CURRENT]  = pos - neg; //assumes both are abs magnitudes
				
				if (input->state[CURRENT] != 0)
					input->response(target, input->state[CURRENT], input->state[PREVIOUS]);
				break;
				
			case DELTA: //only call response when channel delta [CURRENT] is non-zero
				
				if (input->channelPos)
					pos = input->channelPos->delta[CURRENT] * input->channelPos->active;
				
				if (input->channelNeg) //optional negative input contributor
					neg = input->channelNeg->delta[CURRENT] * input->channelNeg->active;
				
				if (debug) LOGI("p=%f c=%f act=%d d=%f\n", input->delta[PREVIOUS], input->delta[CURRENT], input->channelPos->active, pos - neg);
				input->delta[PREVIOUS] = input->delta[CURRENT];
				input->delta[CURRENT]  = pos - neg; //assumes both are abs magnitudes
				
				if (input->delta[CURRENT] != 0)
					input->response(target, input->delta[CURRENT], input->delta[PREVIOUS]);
				break;
			}
		}
	}
}

bool Input_equals(Input a, Input b) //TODO make equals a function pointer in list.h
{
	return false; //DEV no members yet
}

#ifdef __ANDROID__
//PRIVATE
int32_t Android_onInputEvent(struct android_app* app, AInputEvent* event)
{
	Engine * engine = (Engine *)app->userData;
	Device * device = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
	
	int32_t count, eventAction, pid;
	uint32_t touchAction;
	size_t index;

	float p[2];
	
	switch (AInputEvent_getType(event))
	{
	case AINPUT_EVENT_TYPE_KEY:
		if (AKeyEvent_getKeyCode(event) == AKEYCODE_BACK)
		{
			return 1;
		}
		return 0; //allows back button to work
		break;
	
	case AINPUT_EVENT_TYPE_MOTION:
	
		eventAction = AMotionEvent_getAction(event);
		touchAction = eventAction & AMOTION_EVENT_ACTION_MASK;//eventAction & ;
		index  = (eventAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		count  = AMotionEvent_getPointerCount(event);
		
		//LOGI("AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT %d\n", AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
		//flags  =  action & AMOTION_EVENT_ACTION_MASK;
		
		//LOGI("POINTER... index=%d count=%d\n", index, count);
		
		p[XX] = AMotionEvent_getX(event, index);
		p[YY] = AMotionEvent_getY(event, index);
		switch(touchAction)
		{
		case AMOTION_EVENT_ACTION_DOWN:
			//LOGI("DOWN... %.3f %.3f\n", p[XX], p[YY]);
			for (int i = 0; i < 2; i++)
			{
				DeviceChannel * channel = &device->channels[i+index*3];
				channel->active = true;
				channel->state[CURRENT] = p[i]; //must set it to the start point
				channel->state[PREVIOUS] = p[i]; //must set it to the start point
				
				engine->touches[index] = true;
			}
			return true;
			break;
		
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			//LOGI("POINTER DOWN %d at %.3f, %.3f\n", index, p[XX], p[YY]);
			for (int i = 0; i < 2; i++)
			{
				DeviceChannel * channel = &device->channels[i+index*3];
				channel->active = true;
				channel->state[CURRENT] = p[i]; //must set it to the start point
				channel->state[PREVIOUS] = p[i]; //must set it to the start point
				
				engine->touches[index] = true;
			}
			return true;
			break;
		case AMOTION_EVENT_ACTION_UP:
			//LOGI("UP\n", index);
			for (int i = 0; i < 2; i++)
			{
				DeviceChannel * channel = &device->channels[i+index*3];
				channel->active = false;
				
				engine->touches[index] = false;
			}
			return true;
			break;
		case AMOTION_EVENT_ACTION_POINTER_UP:
			//LOGI("POINTER UP %d\n", index);
			for (int i = 0; i < 2; i++)
			{
				DeviceChannel * channel = &device->channels[i+index*3];
				channel->active = false;
				
				engine->touches[index] = false;
			}
			return true;
			break;
		case AMOTION_EVENT_ACTION_MOVE:
			//LOGI("MOVE... %.3f %.3f\n", p[XX], p[YY]);
			//LOGI("LAST... %.3f %.3f\n", device->channels[XX].state[PREVIOUS], device->channels[YY].state[PREVIOUS]);
			
			for (int j = 0; j < count; j++)
			{
				//pids are the indices of touches which persist and may not be zero-based
				pid = AMotionEvent_getPointerId(event, j);
				p[XX] = AMotionEvent_getX(event, pid);
				p[YY] = AMotionEvent_getY(event, pid);
				for (int i = 0; i < 2; i++)
				{
					DeviceChannel * channel = &device->channels[pid*3 + i];
					channel->state[CURRENT] = p[i];
				}
				//LOGI("POINTER ID=%d x=%.3f y=%.3f\n", pid, p[XX], p[YY]);
			}
		/*
			for (int t = 0; t < 10; t++) //< sizeof(engine->touches) / sizeof(bool)
			{
				if (engine->touches[t])
				{
					
						
						//DeviceChannel_setCurrentDelta(channel);
						//DeviceChannel_setPreviousState(channel);
					
					LOGI("TOUCH %d", t);
				}
			}
*/
			//LOGI("x %f\ty %f\n",p[XX], p[YY]);
			return true;
			
			break;
		case AMOTION_EVENT_ACTION_CANCEL:
			LOGI("CANCEL");
			break;
		default: LOGI("Unknown input event type.");
		}
		break;
	}
	return true;
}

//PRIVATE
void Android_frame(Engine * engine)
{
	// No display.
	if (engine->display == NULL)
	{
		LOGI("no engine->display");
		return;
	}
	engine->userUpdateFunc(engine->userUpdateArg);
	
	bool result = eglSwapBuffers(engine->display, engine->surface);
	//LOGI("Swap? %d", result);
}

/**
 * Process the next main command.
 */
 //PRIVATE
void Android_onAppCmd(struct android_app* app, int32_t cmd)
{
	Engine * engine = (Engine *)app->userData;
	switch (cmd)
	{
	case APP_CMD_CONFIG_CHANGED:
		LOGI("CONFIG_CHANGED");

		break;
	case APP_CMD_INPUT_CHANGED:
		LOGI("INPUT_CHANGED");
		break;
	case APP_CMD_SAVE_STATE:
	/*
		app->savedState = malloc(sizeof(struct saved_state));
        *((struct saved_state*)engine->app->savedState) = engine->state;
        app->savedStateSize = sizeof(struct saved_state);
*/
		LOGI("SAVE_STATE");
		break;
	case APP_CMD_INIT_WINDOW:
		LOGI("INIT_WINDOW");
		// The window is being shown, get it ready.
		
		if (engine->app->window != NULL)
		{
			LOGI("engine->app->window is NULL!");
			Engine_initialise(engine);
			//if (!engine->userInitialised)
			{
				engine->userInitialiseFunc(); //TODO - call from within Engine_initialise()?
				engine->userInitialised = true;
			}
			
			LOGI("...INIT_WINDOW!");
		}
		
		break;
	
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		LOGI("TERM_WINDOW");
		Window_terminate(engine);
		engine->userSuspendFunc(engine->userSuspendArg);
		break;
	case APP_CMD_GAINED_FOCUS:
		LOGI("GAINED_FOCUS");
		break;
	case APP_CMD_LOST_FOCUS:
		LOGI("LOST_FOCUS");
		break;
		
	case APP_CMD_PAUSE:
		LOGI("PAUSE");
		engine->paused = true;
		break;
		
	case APP_CMD_RESUME:
		LOGI("RESUME");
		engine->paused = false; //TODO actually should be done when eglContextMakeCurrent() succeeds
		/*
		if (!engine->context)
		{
		engine->context = eglCreateContext(engine->display, engine->config, NULL, engine->attribsList);

		if (eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context) == EGL_FALSE)
		{
			LOGW("Unable to eglMakeCurrent");
			//return -1;
		}
		else
			LOGI("Success eglMakeCurrent");
		
		}
		*/
		break;
	}
}
#endif //__ANDROID__

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
		DeviceChannel_setPreviousState(channel);
		channel->state[CURRENT] = p[i];
		DeviceChannel_setCurrentDelta(channel);
	}
	//LOGI("x=%.3f y=%.3f\n", p[XX], p[YY]);
	
	Device * keyboard = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("keyboard"));
	
	//space
	channel = &keyboard->channels[0];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = (float)glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//LOGI("sc=%.3f sp=%.3f sd=%.3f\n", channel->state[CURRENT], channel->state[PREVIOUS], channel->delta[CURRENT]);
	//S
	channel = &keyboard->channels[1];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//W
	channel = &keyboard->channels[2];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//D
	channel = &keyboard->channels[3];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//A
	channel = &keyboard->channels[4];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	
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
	engine->userInitialised = true;
	#endif//DESKTOP
}

void Loop_run(Engine * engine)
{
	#ifdef DESKTOP
	double t1, t2 = 0;
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
		engine->deltaSec = 0.0333333f;
		
		int ident;
		int events;
		struct android_poll_source* source;
		struct android_app* state = engine->app;
		

		//TODO make this a function pointer set when cmd init occurs to avoid branch
		if (engine->app->window)
		{

		//we must flush input to get rid of old deltas / states or they will persist
		//do so BEFORE event loop to ensure event values don't get overridden
		Device * device = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
		
		for (int i = 0; i < 8; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			//DeviceChannel_setCurrentDelta(channel);
			DeviceChannel_setPreviousState(channel);
			//channel->state[CURRENT] = 0;
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
				LOGI("state destroy has been req");
				//exit(0);
			}
		}
		
		if (engine->app->window)
		{

		//we must flush input to get rid of old deltas / states or they will persist
		//do so BEFORE event loop to ensure event values don't get overridden
		Device * device = (Device *) get(&engine->devicesByName, *(uint64_t *) pad("cursor"));
		
		for (int i = 0; i < 8; i++)
		{
			DeviceChannel * channel = &device->channels[i];
			DeviceChannel_setCurrentDelta(channel);
		}
		}
		
		//TODO if accumulated sufficient time
		if (!engine->paused)
			Android_frame(engine);
	}
	#endif//__ANDROID__
	#endif//DESKTOP/MOBILE
}

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
void Shader_load(Engine * this, const char * path, const char * name)//, const char ** attributeLocations)
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

void Program_construct(Program * this, GLuint vertex_shader, GLuint fragment_shader, const char * attributeLocations[], size_t attributeLocationsCount)
{
	GLuint id = this->id = glCreateProgram();
	
	// Attach the shaders to the program
	glAttachShader(id, vertex_shader);
	glAttachShader(id, fragment_shader);
	LOGI("gl error %i\n", glGetError());
	
	//TODO should these be stored in a list or something, then applied from there? this allows flexibility if re-specifying (unlikely)
	for (int i = 0; i < attributeLocationsCount; i++)
		glBindAttribLocation(id, i, attributeLocations[i]);
	
	//this->attributeLocationsList.entries = this->attributeLocations;
	//this->attributeLocationsList.capacity = 16; //TODO query GL for how many max? or use sizeof(program->attributeLocations)
	//this->attributeLocationsList.fail = NULL;
	
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

void Engine_createScreenQuad(Engine * this, Mesh * mesh, GLuint positionVertexAttributeIndex, GLuint texcoordVertexAttributeIndex,
	int w, int h,
	int rcx, int rcy
)
{
	mesh->topology = GL_TRIANGLES;
	mesh->indexCount = 6;
	mesh->vertexCount = 4;
	mesh->index = malloc(sizeof(GLushort) * mesh->indexCount);
	
	int32_t positionByteCount = sizeof(GLfloat) * mesh->vertexCount * TEXCOORD_COMPONENTS;
	int32_t texcoordByteCount = sizeof(GLfloat) * mesh->vertexCount * TEXCOORD_COMPONENTS;
	
	mesh->attribute[positionVertexAttributeIndex].vertex = malloc(positionByteCount);
	mesh->attribute[texcoordVertexAttributeIndex].vertex = malloc(texcoordByteCount);
	mesh->attributeCount = 2; //TODO should add the attributes through a method that tracks count.
	
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
	
    position->index = positionVertexAttributeIndex;
    texcoord->index = texcoordVertexAttributeIndex;
    position->components = TEXCOORD_COMPONENTS;
    texcoord->components = TEXCOORD_COMPONENTS;
    position->type = GL_FLOAT; //actually could be int byte?
    texcoord->type = GL_FLOAT; //actually could be int byte?
    position->normalized = GL_FALSE;
    texcoord->normalized = GL_FALSE;
    position->stride = 0;
    texcoord->stride = 0;
    position->pointer = 0;
    texcoord->pointer = 0;
    position->vertexBytes = positionByteCount;
    texcoord->vertexBytes = texcoordByteCount;
    position->usage = GL_DYNAMIC_DRAW;
    texcoord->usage = GL_DYNAMIC_DRAW;
	
	LOGI("0index %d %d\n", sizeof(_index), mesh->indexCount * 2);
	LOGI("0pos   %d %d\n", sizeof(_position), mesh->vertexCount * 4);
	LOGI("0texco %d %d\n", sizeof(_texcoord), mesh->vertexCount * 4);
	
	if (this->capabilities.vao || this->debugDesktopNoVAO)
	{
		//gen & bind
		glGenVertexArrays(1, &(mesh->vao)); //VAO de-necessitates glGetAttribLocation & glVertexAttribPointer on each modify op
		glBindVertexArray(mesh->vao);
	}

	//each attribute...
	for (int i = 0; i < mesh->attributeCount; i++)
	{
		Attribute * attribute = &mesh->attribute[i];
		
		glGenBuffers(1, &attribute->id);
		Attribute_submitData(attribute, this);
	}

	if (this->capabilities.vao)
		glBindVertexArray(0); //unbind
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
	
	if (this->capabilities.vao)
		glBindVertexArray(mesh->vao);
	else
	{
		for (int i = 0; i < mesh->attributeCount; i++)
		{
			Attribute * attribute = &mesh->attribute[i];
			
			glBindBuffer(GL_ARRAY_BUFFER, attribute->id);
			
			Attribute_prepare(attribute);
		}
	}
	
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
	
	if (this->capabilities.vao)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0); //vertex attribute array target no longer bound
		glBindVertexArray(0);
	}
}

void Engine_oneUI(Engine * this, Renderable * renderable, const GLfloat * matM)
{
	Mesh * mesh = renderable->mesh;
	
	if (this->capabilities.vao)
		glBindVertexArray(mesh->vao);
	else
	{
		for (int i = 0; i < mesh->attributeCount; i++)
		{
			Attribute * attribute = &mesh->attribute[i];
			
			glBindBuffer(GL_ARRAY_BUFFER, attribute->id);
			
			Attribute_prepare(attribute);
		}
	}

	//prep uniforms...
	//...model matrix
	GLint mLoc = glGetUniformLocation(this->program->id, "m");
	glUniformMatrix4fv(mLoc, 1, GL_FALSE, (GLfloat *)matM);
	
	if (mesh->index == NULL)
		glDrawArrays(mesh->topology, 0, mesh->vertexCount);
	else
		glDrawElements(mesh->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index);

	if (this->capabilities.vao)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0); //vertex attribute array target no longer bound
		glBindVertexArray(0);
	}
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
	
	//DEBUG as though no VAOs on desktop
	this->debugDesktopNoVAO = true;
	if (this->debugDesktopNoVAO)
		this->capabilities.vao = false;
	
	#elif MOBILE
	#if __ANDROID__

	ANativeActivity* activity = this->app->activity;
    JNIEnv* env = activity->env;

	// Setup OpenGL ES 2
	// http://stackoverflow.com/questions/11478957/how-do-i-create-an-opengl-es-2-context-in-a-native-activity

	const EGLint configAttribs[] = {
		//EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, //important
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 16,
		EGL_NONE
	};

	//this->attribsList = attribs;
	//memcpy(this->attribsList, attribs, sizeof(attribs));

	EGLint w, h, dummy, format;
	EGLint numConfigs;
	
	//EGLSurface surface;
	//EGLContext context;

	this->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	bool r = eglInitialize(this->display, 0, 0);
	//LOGI("r=%d", r);
	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglChooseConfig(this->display, configAttribs, &this->config, 1, &numConfigs);

	//LOGI("numConfigs=%d",numConfigs);
	
	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	bool re = eglGetConfigAttrib(this->display, this->config, EGL_NATIVE_VISUAL_ID, &format);
	//LOGI("re=%d", re);
	ANativeWindow_setBuffersGeometry(this->app->window, 0, 0, format);

	this->surface = eglCreateWindowSurface(this->display, this->config, this->app->window, NULL);

	const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	this->context = eglCreateContext(this->display, this->config, NULL, contextAttribs);

	if (eglMakeCurrent(this->display, this->surface, this->surface, this->context) == EGL_FALSE)
	{
		//LOGW("Unable to eglMakeCurrent");
		EGLint err = eglGetError();
		LOGW( "Unable to eglMakeCurrent %d", err );
			
			
			
		return -1;
	}
	else
		LOGI("Success eglMakeCurrent");

	// Grab the width and height of the surface
	eglQuerySurface(this->display, this->surface, EGL_WIDTH, &w);
	eglQuerySurface(this->display, this->surface, EGL_HEIGHT, &h);

	this->width = w;
	this->height = h;

	// Initialize GL state.
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	//glEnable(GL_CULL_FACE);
	//glDisable(GL_DEPTH_TEST);
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
			
			this->capabilities.vao =
				glBindVertexArray &&
				glDeleteVertexArrays &&
				glGenVertexArrays &&
				glIsVertexArray;
				
			LOGI("VAO supported? %s", this->capabilities.vao ? "YES" : "NO");
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

	FILE *file = fopen(filename, "rb"); //open for reading
	if (file)
	{
		fseek(file, 0, SEEK_END); //seek to end
		long fileSize = ftell(file); //get current position in stream
		fseek(file, 0, SEEK_SET); //seek to start
		LOGI(filename);
		
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
	
	return str;
}

float Engine_smoothstep(float t)
{
	return 3 * t * t - 2 * t * t * t;
}

void Attribute_prepare(Attribute * attribute)
{
	//provide data / layout info; "take buffer that is bound at the time called and associates that buffer with the current VAO"
	glVertexAttribPointer(attribute->index, attribute->components, attribute->type, attribute->normalized, attribute->stride, attribute->pointer); 
	glEnableVertexAttribArray(attribute->index); //enable attribute for use
}

void Attribute_tryPrepare(Attribute * attribute, Engine * engine)
{
	if (engine->capabilities.vao)
		Attribute_prepare(attribute);
}

void Attribute_submitData(Attribute * attribute, Engine * engine)
{
	glBindBuffer(GL_ARRAY_BUFFER, attribute->id);
    glBufferData(GL_ARRAY_BUFFER, attribute->vertexBytes, attribute->vertex, attribute->usage);
	Attribute_tryPrepare(attribute, &engine);
}