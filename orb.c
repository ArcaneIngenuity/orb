#include "orb.h"

#define KEYPARTS 1
//KHASH...
KHASH_DEFINE(StrInt, 	kh_cstr_t, int, kh_str_hash_func, kh_str_hash_equal, 1)
KHASH_DEFINE(IntInt, 	khint32_t, int, kh_int_hash_func, kh_int_hash_equal, 1)
KHASH_DEFINE(IntFloat, 	khint32_t, float, kh_int_hash_func, kh_int_hash_equal, 1)

#ifndef ORB_KHASH_TYPES_OFF
KHASH_DEFINE(StrPtr, 	kh_cstr_t, uintptr_t, kh_str_hash_func, kh_str_hash_equal, 1)
#endif//ORB_KHASH_TYPES_OFF

static khiter_t k;

glUniformVectorFunction glUniformVectorFunctions[4][2]; 
glUniformMatrixFunction glUniformMatrixFunctions[4][4][2]; 

//...KHASH.

//helpers...
// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with)
{
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
	
    return result;
	// You must free the result if result is non-NULL.
}
//...helpers.

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

void Input_executeList(InputList * list, void * target, bool debug)
{
	float pos = 0, neg = 0;

	for (int i = 0; i < kv_size(*list); i++)
	{
		Input * input = &kv_A(*list, i);

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
				
				//if (debug) LOGI("p=%f c=%f act=%d d=%f\n", input->delta[PREVIOUS], input->delta[CURRENT], input->channelPos->active, pos - neg);
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
	
	k = kh_get(StrPtr, engine->devicesByName, "cursor");
	Device * device = kh_val(engine->devicesByName, k);
	
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
			LOGI("engine->app->window is valid.");
			Engine_initialise(engine);
			//if (!engine->userInitialised)
			{
				engine->userInitialiseFunc(); //TODO - call from within Engine_initialise()?
				engine->userInitialised = true;
			}
			
			LOGI("...INIT_WINDOW!");
		}
		else
		{
			LOGI("engine->app->window is NULL.");
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

void Android_ensureValidDataPath(struct android_app * app)
{
	if (app->activity->internalDataPath == NULL)
		app->activity->internalDataPath = "/data/data/com.arcaneingenuity.waradventure/files"; //HACK, TODO use JNI instead! (getExternalFilesDir(null))
	//...is it safe to set the member itself on the android activity class? Guess so.
}

void Android_extractAssetsFromAPKDirectory(struct android_app * app, const char * apkDir)
{
	LOGI("Android_extractAssetsFromAPKDirectory %s\n", apkDir);
    const char * filename = (const char*)NULL;
	char * fsDir = app->activity->internalDataPath;
	LOGI("--- %s\n", fsDir);
	
	char dirInFS[strlen(fsDir)+strlen(apkDir)+1+1]; //+1 = slash, +1 = null terminator
	strcpy(dirInFS, fsDir);
	strcat(dirInFS, "/");
	strcat(dirInFS, apkDir);
	
	LOGI("=== %s\n", dirInFS);
	
	int result = mkdir(dirInFS, 0770);
	if (result != 0)
		LOGI("failed to create directory: %s", dirInFS);
	//TODOcheck result and act thereon
    	
	AAssetManager* mgr = app->activity->assetManager;
    AAssetDir* assetDir = AAssetManager_openDir(mgr, apkDir);
	
	LOGI("??? %p\n", assetDir);
	
	//browse all files and copy them on disk one by one
	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL)
	{
		char pathInAPK[strlen(apkDir)+strlen(filename)+1+1]; //+1 = slash, +1 = null terminator
		strcpy(pathInAPK, apkDir);
		strcat(pathInAPK, "/");
		strcat(pathInAPK, filename);
		
		char pathInFS[strlen(fsDir)+strlen(pathInAPK)+1+1]; //+1 = slash, +1 = null terminator
		strcpy(pathInFS, fsDir);
		strcat(pathInFS, "/");
		strcat(pathInFS, pathInAPK);

		LOGI("filename =%s", filename);
		LOGI("pathInApk=%s", pathInAPK);
		LOGI("pathInFS =%s", pathInFS);
		
		const int BUFFERSIZE = 10;
		char buffer[BUFFERSIZE];
		int readCount = 0;
		
		AAsset* asset = AAssetManager_open(mgr, pathInAPK, AASSET_MODE_STREAMING);
		FILE* out = fopen(pathInFS, "w+");

		if (out)
		{
			while ((readCount = AAsset_read(asset, buffer, BUFFERSIZE)) > 0)
			{
				fwrite(&buffer, readCount, 1, out);
			}
			fflush(out);
			
			fclose(out);
			LOGI("Created / wrote %s.", pathInFS);
		}
		else
		{
			LOGI("Could not open %s for write.", pathInFS);
		}
		
		AAsset_close(asset);
		
	}
	
	AAssetDir_close(assetDir);
}

void Android_extractAssetsFromAPK(struct android_app * app, const char * apkDirs[], int apkDirsCount)
{
	LOGI("Android_extractAssetsFromAPK\n");
	for (int i = 0; i < apkDirsCount; ++i)
	{
		LOGI("i=%d\n", i);
		Android_extractAssetsFromAPKDirectory(app, apkDirs[i]);
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
	
	k = kh_get(StrPtr, engine->devicesByName, "cursor");
	Device * mouse = kh_val(engine->devicesByName, k);
	
	for (int i = 0; i < 2; i++) //x & y
	{
		channel = &mouse->channels[i];
		DeviceChannel_setPreviousState(channel);
		channel->state[CURRENT] = p[i];
		DeviceChannel_setCurrentDelta(channel);
	}
	//LOGI("x=%.3f y=%.3f\n", p[XX], p[YY]);

	k = kh_get(StrPtr, engine->devicesByName, "keyboard");
	Device * keyboard = kh_val(engine->devicesByName, k);	
	
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
	
	//K
	channel = &keyboard->channels[5];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//I
	channel = &keyboard->channels[6];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//L
	channel = &keyboard->channels[7];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//J
	channel = &keyboard->channels[8];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);

	//-
	channel = &keyboard->channels[9];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
	
	//=
	channel = &keyboard->channels[10];
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS;
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
		k = kh_get(StrPtr, engine->devicesByName, "cursor");
		Device * device = kh_val(engine->devicesByName, k);
	
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
		k = kh_get(StrPtr, engine->devicesByName, "cursor");
		Device * device = kh_val(engine->devicesByName, k);
		
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

//------------------Renderable-----------------//

void IndexedRenderableManager_create(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr,
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model)
{
	for (int ii = 0; ii < kv_size(*indexListPtr); ii++)
	{
		int i = kv_A(*indexListPtr, ii);
		
		Renderable * renderable = &renderables[i]; //get our renderable.. TODO a more flexible mapping of data index to renderable index?
		fnc(renderable, i, model); //..create it
		
		//TODO only add if a custom checkAdd func returns true
		//(allows e.g. AoI or other selectiveness about what to render)
		kv_push(uint16_t, *indexRenderListPtr, i);
	}
}

/// updates a Renderable that has (at any point previously) been created
void IndexedRenderableManager_update(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr, //actually unused here, but keeps the arg lists uniform between create/update/render
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model)
{
	for (int ii = 0; ii < kv_size(*indexListPtr); ii++)
	{
		int i = kv_A(*indexListPtr, ii);
		
		Renderable * renderable = &renderables[i]; //get our renderable.. TODO a more flexible mapping of data index to renderable index?
		fnc(renderable, i, model); //..update it
		
		//exactly as createRenderables, but no list add... could we make these the same? RenderableManager_applyFunction(bool withAdd)
	}
}

/// renders a group of Renderables from an index list
void IndexedRenderableManager_render(
	Renderable * const renderables,
	IndexList * indexRenderListPtr,
	Engine * enginePtr)
{	
	for (int ii = 0; ii < kv_size(*indexRenderListPtr); ++ii)
	{
		int i = kv_A(*indexRenderListPtr, ii);
		
		Renderable * renderable = &renderables[i];
		
		Engine_one(enginePtr, renderable);
	}
}

//------------------Material-----------------//


//------------------InstanceGroup------------//


//------------------Batch--------------------//

//used when we need a lot of different geometries that use same material to be tightly packed for upload to / rendering on GPU.




//------------------Texture------------------//

Texture * Texture_create()
{
	Texture * texture = malloc(sizeof(Texture));
	glGenTextures(1, &texture->id);
	
	texture->intParametersByName = kh_init(IntInt);
	texture->floatParametersByName = kh_init(IntFloat);
	
	return texture;
}

void Texture_createData(Texture * texture)
{
	texture->data = calloc(1, texture->width*texture->height*texture->components*sizeof(GLubyte));
}

void Texture_clearData(Texture * texture, bool alphaFull)
{
	memset(texture->data, 0, texture->width*texture->height*texture->components*sizeof(GLubyte));
	if (alphaFull)
	{
		for (int i = 3; i < texture->width*texture->height*texture->components*sizeof(GLubyte); i+=4)
		{
			texture->data[i] = 255;
		}
	}	
}

void Texture_setRGBA(Texture * texture, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	int i = x + y * texture->width;
	texture->data[0+4*i] = r;
	texture->data[1+4*i] = g;
	texture->data[2+4*i] = b;
	texture->data[3+4*i] = a;
}
void Texture_loadData(Texture * texture, const char * filename)
{
	//Texture * texture = Texture_create();
	//texture->name = (char *) filename;
	#ifdef DESKTOP
	texture->data = stbi_load(filename, &(texture->width), &(texture->height), &(texture->components), 0);
	if (texture->data == NULL)
		LOGI("WARNING: texture %s not loaded.\n", filename); 
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
	
	int ret, is_missing;
	
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
	
	int ret, is_missing;
	
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_WRAP_S, GL_REPEAT);
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_WRAP_T, GL_REPEAT);
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	kh_set(IntInt, this->intParametersByName, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
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
	//LOGI("tex name=%s loc=%d unit=%d\n", this->name, uniformTexture, this->unit);
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
	this->dimensions = dimensions;
}

void Texture_applyParameters(Texture * this)
{
	glActiveTexture(GL_TEXTURE0 + this->unit);
	glBindTexture(GL_TEXTURE_2D, this->id);

	for (k = kh_begin(this->intParametersByName); k != kh_end(this->intParametersByName); ++k)
	{
		if (kh_exist(this->intParametersByName, k))
		{
			int key = kh_key(this->intParametersByName, k);
			int tval = kh_value(this->intParametersByName, k);
			
			glTexParameteri(this->dimensions, key, tval);
		}
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
void Engine_loadShader(Engine * this, Shader ** shader, const char * path, const char * name, GLenum type)//, const char ** attributeLocations)
{
	(*shader) = calloc(1, sizeof(Shader));
	
	size_t lengthName = strlen(name); 
	size_t lengthPath = strlen(path);
	char pathname[lengthPath+lengthName+1]; //1 = '\0'
	strcpy(pathname, path);
	strcat(pathname, name);
	LOGI("pathname %s\n", pathname);
	(*shader)->source = Text_load(pathname);
	(*shader)->type = type;
	Shader_construct(*shader);

	kh_set(StrPtr, this->shadersByName, name, shader);
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

//--------------Render-------------------//


void Engine_clear() //TODO should be Render_clear?
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
void Engine_one(Engine * this, Renderable * renderable)
{
	Mesh * mesh = renderable->mesh;
	
	if (this->capabilities.vao)
		glBindVertexArray(mesh->vao);
	else
	{
		for (int i = 0; i < mesh->attributeCount; ++i)
		{
			Attribute * attribute = &mesh->attribute[i];
			
			glBindBuffer(GL_ARRAY_BUFFER, attribute->id);
			
			Attribute_prepare(attribute);
		}
	}
	
	UniformGroup_update(renderable->uniformPtrsByName, this->program);

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

	this->programsByName 		= kh_init(StrPtr);
	this->shadersByName 		= kh_init(StrPtr);
	this->devicesByName 		= kh_init(StrPtr);

	//reintroduce if we bring transform list back into this library.
	//Renderable * renderable = &this->renderable;
	//for (int i = 0; i < transformsCount; i++)
	//	mat4x4_identity(renderable->matrix[i]);

	//these get set here; brace initialisation not allowed due to glew not having function pointers ready yet(?)
	glUniformVectorFunctions[1-1][0] = glUniform1fv;
	glUniformVectorFunctions[1-1][1] = glUniform1iv;
	glUniformVectorFunctions[2-1][0] = glUniform2fv;
	glUniformVectorFunctions[2-1][1] = glUniform2iv;
	glUniformVectorFunctions[3-1][0] = glUniform3fv;
	glUniformVectorFunctions[3-1][1] = glUniform3iv;
	glUniformVectorFunctions[4-1][0] = glUniform4fv;
	glUniformVectorFunctions[4-1][1] = glUniform4iv;
	

	//TODO once this is encapsulated, it can easily just be a 3x3(x1) array (there are no 1's here, and only floats!)
	glUniformMatrixFunctions[2-1][2-1][0] = glUniformMatrix2fv;
	glUniformMatrixFunctions[3-1][3-1][0] = glUniformMatrix3fv;
	glUniformMatrixFunctions[4-1][4-1][0] = glUniformMatrix4fv;
	#ifdef DESKTOP
	glUniformMatrixFunctions[2-1][3-1][0] = glUniformMatrix2x3fv;
	glUniformMatrixFunctions[3-1][2-1][0] = glUniformMatrix3x2fv;
	glUniformMatrixFunctions[2-1][4-1][0] = glUniformMatrix2x4fv;
	glUniformMatrixFunctions[4-1][2-1][0] = glUniformMatrix4x2fv;
	glUniformMatrixFunctions[3-1][4-1][0] = glUniformMatrix3x4fv;
	glUniformMatrixFunctions[4-1][3-1][0] = glUniformMatrix4x3fv;
	#endif//DESKTOP
	LOGI("Engine initialised.\n");
}

void Engine_dispose(Engine * engine)
{
	//TODO free engine collections.
	
	#ifdef DESKTOP
	Window_terminate(&engine);
	#endif//DESKTOP
}

void Engine_loadProgramFromConfig(Engine * engine, ProgramConfig programConfig, const char * path)
{
	//TODO check first if a given shader exists already, before loading it!
	Shader * vert;
	Shader * frag;
	Engine_loadShader(engine, &vert, path, programConfig.vertexName, GL_VERTEX_SHADER);
	Engine_loadShader(engine, &frag, path, programConfig.fragmentName, GL_FRAGMENT_SHADER);

	Program * program = calloc(1, sizeof(Program));
	Program_construct(program, 
		vert->id,
		frag->id,
		programConfig.attributeLocations,
		programConfig.attributeLocationsCount
		);
	program->topology = GL_TRIANGLES; //TODO place in config?
	//TODO check whether key exists
	kh_set(StrPtr, engine->programsByName, programConfig.programName, program);	
}

void Engine_loadProgramsFromConfig(Engine * engine, ProgramConfig programConfigs[], uint8_t programConfigsCount, const char * path)
{	
	for (int i = 0; i < programConfigsCount; ++i)
		Engine_loadProgramFromConfig(engine, programConfigs[i], path);
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
		k = kh_get(StrPtr, this->programsByName, name);
		this->program = kh_val(this->programsByName, k);
		assert (this->program != NULL);
		glUseProgram(this->program->id);
	}
	return this->program;
}

Program * Engine_getCurrentProgram(Engine * this)
{
	return this->program;
}

/// Takes Linux-canonical (single forward slash) paths.
const char * Engine_getPath(Engine * engine, const char * path, int pathLength, const char * partial)
{
	//get paths ready for loading shaders
	#if _WIN32
	//relative path
	strcpy(path, ".\\");
	char * replaced = str_replace(partial, "/", "\\");
	if (replaced)
	{
		strcat(path, replaced);
		free(replaced);
	}
	#elif __ANDROID__
	//absolute path
	char * fsDir = engine->app->activity->internalDataPath;
	strcpy(path, fsDir);
	strcat(path, "/");
	strcat(path, partial);
	#elif __linux__
	char * prefix = "./";
	//char * pathTemp = malloc(sizeof(char) * (strlen(prefix) + strlen(partial)));
	strcpy(path, prefix); 
	strcat(path, partial); 
	///path = pathTemp;
	#else //all other supported OS?
	//relative path?
	//chdir("/sdcard/");
	//char * path = "./shd/";
	#endif //OS
	
	LOGI("@@ %s", path);
}

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

//Updates uniforms for the current program.
void UniformGroup_update(khash_t(StrPtr) * uniformPtrsByName, Program * programPtr)
{
	for (k = kh_begin(uniformPtrsByName); k != kh_end(uniformPtrsByName); ++k)
	{
        if (kh_exist(uniformPtrsByName, k))
		{
			//const char * key = kh_key(uniformPtrsByName,k);
			//LOGI("key=%s\n", key);
			Uniform * uniform = kh_value(uniformPtrsByName, k);
			GLint location = glGetUniformLocation(programPtr->id, uniform->name);
			
			switch (uniform->type)
			{
				case UniformTexture:
				{
					Texture * texturePtr = uniform->valuesPtr;
					//TODO optimise: if needs refresh!
					Texture_refresh(texturePtr);
					Texture_prepare(texturePtr, programPtr);
				}
				break;
				case UniformVector:
				{
					(*(glUniformVectorFunctions[uniform->componentsMajor-1][uniform->typeNumeric]))
						(location, uniform->elements, uniform->valuesPtr);
				}
				break;
				case UniformMatrix:
				{
					(*(glUniformMatrixFunctions[uniform->componentsMajor-1][uniform->componentsMinor-1][uniform->typeNumeric]))
						(location, uniform->elements, uniform->matrixTranspose, uniform->valuesPtr);
				}
				break;
				default: break;
			}
		}
	}
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