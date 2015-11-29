#include "orb.h"

#define KEYPARTS 1
//KHASH...
KHASH_DEFINE(StrInt, 	kh_cstr_t, int, kh_str_hash_func, kh_str_hash_equal, 1)
KHASH_DEFINE(IntInt, 	khint32_t, int, kh_int_hash_func, kh_int_hash_equal, 1)
KHASH_DEFINE(IntFloat, 	khint32_t, float, kh_int_hash_func, kh_int_hash_equal, 1)
KHASH_DEFINE(Str_AttributeLocation, kh_cstr_t, AttributeLocation, kh_str_hash_func, kh_str_hash_equal, 1)

#ifndef ORB_KHASH_TYPES_OFF
KHASH_DEFINE(StrPtr, 	kh_cstr_t, uintptr_t, kh_str_hash_func, kh_str_hash_equal, 1)
#endif//ORB_KHASH_TYPES_OFF

static khiter_t k;
//...KHASH.

glUniformVectorFunction glUniformVectorFunctions[4][2]; 
glUniformMatrixFunction glUniformMatrixFunctions[4][4][2];

static khash_t(IntInt) * glSizesByName;

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

Device * Device_construct()
{
	Device * device = calloc(1, sizeof(Device));
	device->nameToIndex = kh_init(StrInt);
	return device;
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
			Engine_initialise(engine, this->width, this->height, NULL); //no window title!
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

#ifdef DESKTOP //glfw!
static short buttonOrbToGlfw[] =
{
	XX,
	YY,
	GLFW_MOUSE_BUTTON_1,
	GLFW_MOUSE_BUTTON_2,
	GLFW_MOUSE_BUTTON_3,
	GLFW_MOUSE_BUTTON_4,
	GLFW_MOUSE_BUTTON_5,
	GLFW_MOUSE_BUTTON_6,
	GLFW_MOUSE_BUTTON_7,
	GLFW_MOUSE_BUTTON_8
};
#define GLFW_MOUSE_BUTTON_LEFT   GLFW_MOUSE_BUTTON_1
#define GLFW_MOUSE_BUTTON_RIGHT   GLFW_MOUSE_BUTTON_2
#define GLFW_MOUSE_BUTTON_MIDDLE   GLFW_MOUSE_BUTTON_3
#define GLFW_MOUSE_BUTTON_LAST   GLFW_MOUSE_BUTTON_8

static short keyOrbToGlfw[] =
{
	GLFW_KEY_UNKNOWN,
	GLFW_KEY_0,
	GLFW_KEY_1,
	GLFW_KEY_2,
	GLFW_KEY_3,
	GLFW_KEY_4,
	GLFW_KEY_5,
	GLFW_KEY_6,
	GLFW_KEY_7,
	GLFW_KEY_8,
	GLFW_KEY_9,
	
	GLFW_KEY_KP_0,
	GLFW_KEY_KP_1,
	GLFW_KEY_KP_2,
	GLFW_KEY_KP_3,
	GLFW_KEY_KP_4,
	GLFW_KEY_KP_5,
	GLFW_KEY_KP_6,
	GLFW_KEY_KP_7,
	GLFW_KEY_KP_8,
	GLFW_KEY_KP_9,
	
	GLFW_KEY_A,
	GLFW_KEY_B,
	GLFW_KEY_C,
	GLFW_KEY_D,
	GLFW_KEY_E,
	GLFW_KEY_F,
	GLFW_KEY_G,
	GLFW_KEY_H,
	GLFW_KEY_I,
	GLFW_KEY_J,
	GLFW_KEY_K,
	GLFW_KEY_L,
	GLFW_KEY_M,
	GLFW_KEY_N,
	GLFW_KEY_O,
	GLFW_KEY_P,
	GLFW_KEY_Q,
	GLFW_KEY_R,
	GLFW_KEY_S,
	GLFW_KEY_T,
	GLFW_KEY_U,
	GLFW_KEY_V,
	GLFW_KEY_W,
	GLFW_KEY_X,
	GLFW_KEY_Y,
	GLFW_KEY_Z,
	
	GLFW_KEY_F1,
	GLFW_KEY_F2,
	GLFW_KEY_F3,
	GLFW_KEY_F4,
	GLFW_KEY_F5,
	GLFW_KEY_F6,
	GLFW_KEY_F7,
	GLFW_KEY_F8,
	GLFW_KEY_F9,
	GLFW_KEY_F10,
	GLFW_KEY_F11,
	GLFW_KEY_F12,
	GLFW_KEY_F13,
	GLFW_KEY_F14,
	GLFW_KEY_F15,
	GLFW_KEY_F16,
	GLFW_KEY_F17,
	GLFW_KEY_F18,
	GLFW_KEY_F19,
	GLFW_KEY_F20,
	GLFW_KEY_F21,
	GLFW_KEY_F22,
	GLFW_KEY_F23,
	GLFW_KEY_F24,
	GLFW_KEY_F25,
	
	GLFW_KEY_RIGHT,
	GLFW_KEY_LEFT,
	GLFW_KEY_DOWN,
	GLFW_KEY_UP,
	
	GLFW_KEY_LEFT_SHIFT,
	GLFW_KEY_LEFT_CONTROL,
	GLFW_KEY_LEFT_ALT,
	GLFW_KEY_LEFT_SUPER,
	GLFW_KEY_RIGHT_SHIFT,
	GLFW_KEY_RIGHT_CONTROL,
	GLFW_KEY_RIGHT_ALT,
	GLFW_KEY_RIGHT_SUPER,
	
	GLFW_KEY_SPACE,
	GLFW_KEY_APOSTROPHE,
	GLFW_KEY_COMMA,
	GLFW_KEY_MINUS,
	GLFW_KEY_PERIOD,
	GLFW_KEY_SLASH,
	GLFW_KEY_SEMICOLON,
	GLFW_KEY_EQUAL,
	GLFW_KEY_BACKSLASH,
	GLFW_KEY_LEFT_BRACKET,
	GLFW_KEY_RIGHT_BRACKET,
	GLFW_KEY_GRAVE_ACCENT,
	GLFW_KEY_WORLD_1, //non-US #1
	GLFW_KEY_WORLD_2, //non-US #2
	GLFW_KEY_ESCAPE,
	GLFW_KEY_ENTER,
	GLFW_KEY_TAB,
	GLFW_KEY_BACKSPACE,
	GLFW_KEY_INSERT,
	GLFW_KEY_DELETE,
	GLFW_KEY_PAGE_UP,
	GLFW_KEY_PAGE_DOWN,
	GLFW_KEY_HOME,
	GLFW_KEY_END,
	GLFW_KEY_CAPS_LOCK,
	GLFW_KEY_SCROLL_LOCK,
	GLFW_KEY_NUM_LOCK,
	GLFW_KEY_PRINT_SCREEN,
	GLFW_KEY_PAUSE,
	GLFW_KEY_KP_DECIMAL,
	GLFW_KEY_KP_DIVIDE,
	GLFW_KEY_KP_MULTIPLY,
	GLFW_KEY_KP_SUBTRACT,
	GLFW_KEY_KP_ADD,
	GLFW_KEY_KP_ENTER,
	GLFW_KEY_KP_EQUAL,
	GLFW_KEY_MENU
};


void GLFW_updateKey(int i, Device * keyboard)
{
	DeviceChannel * channel = &keyboard->channels.a[i]; //orb index as from enum Key
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = (float)glfwGetKey(window, keyOrbToGlfw[i]) == GLFW_PRESS;
	DeviceChannel_setCurrentDelta(channel);
}
#endif// DESKTOP //glfw!

void Loop_processInputs(Engine * engine)
{
	#ifdef DESKTOP //glfw!
	
	Device * device;
	DeviceChannel * channel;
	
	for (k = kh_begin(engine->devicesByName); k != kh_end(engine->devicesByName); ++k)
	{
		if (kh_exist(engine->devicesByName, k))
		{
			//char * key = kh_key(engine->devicesByName, k);
			Device * device = kh_val(engine->devicesByName, k);
			device->consumed = false; //reset this each global update
			for (int j = 0; j < kv_size(device->channels); ++j) //for every raw input that may trigger said mapping
			{
				DeviceChannel * channel = &kv_A(device->channels, j);
				channel->consumed = false;
			}
			if (device->update)
				device->update(device);
		}
	}

	#endif//DESKTOP
}
void Loop_initialise(Engine * engine, int windowWidth, int windowHeight, int windowTitle)
{
	#ifdef __ANDROID__
	//app_dummy();
	struct android_app * app = engine->app;
	app->userData = engine;
	app->onAppCmd = Android_onAppCmd;
	app->onInputEvent = Android_onInputEvent;
	


	#endif//__ANDROID__
	#ifdef DESKTOP
	Engine_initialise(engine, windowWidth, windowHeight, windowTitle); 
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
				//exit(EXIT_FAILURE);
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


//Updates uniforms for the current program.
void UniformGroup_update(khash_t(StrPtr) * uniformsByName, Program * programPtr)
{
	for (k = kh_begin(uniformsByName); k != kh_end(uniformsByName); ++k)
	{
        if (kh_exist(uniformsByName, k))
		{
			//const char * key = kh_key(uniformsByName,k);
			//LOGI("key=%s\n", key);
			Uniform * uniform = kh_value(uniformsByName, k);
			GLint location = glGetUniformLocation(programPtr->id, uniform->name);
			
			switch (uniform->type)
			{
				case UniformTexture:
				{
					Texture * texturePtr = uniform->values;
					//TODO optimise: if needs refresh!
					Texture_refresh(texturePtr);
					Texture_prepare(texturePtr, programPtr);
				}
				break;
				case UniformVector:
				{
					(*(glUniformVectorFunctions[uniform->componentsMajor-1][uniform->typeNumeric]))
						(location, uniform->elements, uniform->values);
				}
				break;
				case UniformMatrix:
				{
					(*(glUniformMatrixFunctions[uniform->componentsMajor-1][uniform->componentsMinor-1][uniform->typeNumeric]))
						(location, uniform->elements, uniform->matrixTranspose, uniform->values);
				}
				break;
				default: break;
			}
		}
	}
}

void Attribute_initialise(Attribute * attribute, GLuint index, GLint components, GLenum type, GLboolean normalized)
{
	attribute->index = index;
	attribute->components = components;
	attribute->type = type;
	attribute->normalized = normalized;
}

//must be called in order of vertex struct members representing each attribute!
Attribute * Mesh_addAttribute(Mesh * this, GLuint index, GLint components, GLenum type, GLboolean normalized)
{
	Attribute * attribute = &this->attribute[index];
	Attribute_initialise(attribute, index, components, type, normalized);
	kv_push(Attribute *, this->attributeActive, attribute);
	
	//this->_offsetIntoVertex = attribute->components * sizeof(GL_FLOAT);
	
	return attribute;
}

void Mesh_initialise(Mesh * this, 
	GLuint topology,
	GLenum usage,
	size_t stride)
{
	this->topology = topology;
	this->usage = usage;
	
	glGenBuffers(1, &this->id);
	kv_init(this->attributeActive);

	this->stride = stride;
	
	//tetrahedron: 4 verts, 4 tris (1:1) - most compact; discrete tris: 3 verts, 1 tri (3:1) - least compact
	//so we know vertex count max, and worst case for index count is 1 tri per vertex, so make it the same. 
	this->indexCount = 0;
	this->index = malloc(sizeof(GLushort) * MESH_VERTICES_SHORT_MAX);
	
	this->vertexCount = 0;
	this->vertexArray = malloc(stride * MESH_VERTICES_SHORT_MAX);
}

//TODO use size-doubling, and do this dynamically? - last step 2^16 will have to have a special case in it however to restrict.
//TODO then just use addVertex to modify? - as continuous allocations will not happen.
//TODO addFace?
void Mesh_resize(Mesh * this, size_t size)
{
	if (size > MESH_VERTICES_SHORT_MAX)
	{
		LOGI("[ORB] Error: Cannot exceeed mesh vertex maximum.\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		//for every attribute, resize
		
		this->vertexCount = size;
	}
}

void Mesh_appendFace(Mesh * mesh, GLushort a, GLushort b, GLushort c)
{
	//LOGI("MESH index count=%d a=%u b=%u c=%u\n", mesh->indexCount, a, b, c);
	mesh->index[mesh->indexCount + 0] = a;
	mesh->index[mesh->indexCount + 1] = b;
	mesh->index[mesh->indexCount + 2] = c;

	mesh->indexCount+=3;
	//TODO reallocate array if indexCount > size
}

Index Mesh_appendVertex(Mesh * mesh, void * vertex)
{
	//LOGI("MESH vertex count=%d\n", mesh->vertexCount);
	memcpy(((char *)mesh->vertexArray) + mesh->vertexCount * mesh->stride, vertex, mesh->stride);
	
	mesh->vertexBytes += mesh->stride;
	Index index = mesh->vertexCount;
	++mesh->vertexCount;
	return index;
}

void Mesh_submit(Mesh * mesh, Engine * engine)
{
	if (engine->capabilities.vao || engine->debugDesktopNoVAO)
	{
		//gen & bind
		glGenVertexArrays(1, &(mesh->vao)); //VAO de-necessitates glGetAttribLocation & glVertexAttribPointer on each modify op
		glBindVertexArray(mesh->vao);
	}
	
	//submit interleaved mesh data
	glBindBuffer(GL_ARRAY_BUFFER, mesh->id);
	glBufferData(GL_ARRAY_BUFFER, mesh->vertexBytes, NULL, mesh->usage); //orphan first
	glBufferData(GL_ARRAY_BUFFER, mesh->vertexBytes, mesh->vertexArray, mesh->usage);
	
	//set up attribute pointers / arrays
	if (engine->capabilities.vao)
	{
		Attribute * attribute;
		
		size_t offset = 0;
		//ORDER HERE *MUST* MATCH THAT OF VERTEX STRUCT MEMBERS! - set via Mesh_initialise()
		for (int i = 0; i < kv_size(mesh->attributeActive); ++i)
		{
			Attribute * attribute = kv_A(mesh->attributeActive, i);
			glVertexAttribPointer(attribute->index, attribute->components, attribute->type, attribute->normalized, mesh->stride, offset); 
			glEnableVertexAttribArray(attribute->index);
			
			offset += attribute->components * glSizeof(attribute->type);//sizeof(float);
		}
	}

	if (engine->capabilities.vao)
		glBindVertexArray(0); //unbind
}

void Mesh_clear(Mesh * this)
{
	this->indexCount = 0;
	this->vertexCount = 0;
	this->vertexBytes = 0;
	memset(this->vertexArray, 0, sizeof(this->vertexArray));
}

void Mesh_merge(Mesh * this, Mesh * other)
{
	int vertexCount = this->vertexCount;
	for (int v = 0; v < other->vertexCount; ++v)
	{
		Mesh_appendVertex(this, ((char *)other->vertexArray) + v * other->stride);		
	}
	for (int i = 0; i < other->indexCount; i+=3)
	{
		Mesh_appendFace(this, 	vertexCount + other->index[i+0],
								vertexCount + other->index[i+1],
								vertexCount + other->index[i+2]);
		//LOGI("index other: %d %d %d\n", other->index[i+0], other->index[i+1], other->index[i+2]);
	}
}

/*
void Mesh_calculateNormals(Mesh * this)
{
}
*/

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

/// Passes index to the next available Transform in Transforms, and increments the count.
size_t Transforms_assign(Transforms * this)
{
	if (this->count < this->capacity)
		return this->count++;
	else //DEV - til we have Transforms_reallocate()
	{
		
		LOGI("[ORB] Error: Transforms count cannot exceed capacity.\n");
		exit(EXIT_FAILURE);
	}
}

//TODO should in fact do the usual realloc-to-twice-current-size
void Transforms_allocate(Transforms * this, size_t capacity)
{
	this->count = 0;
	this->capacity = capacity;
	
	this->matrix 	= malloc(sizeof(mat4x4) 	* capacity);
	this->posLclPx 	= malloc(sizeof(vec3) 		* capacity);
	this->posWldPx 	= malloc(sizeof(vec3) 		* capacity);
	this->posNdc 	= malloc(sizeof(vec3) 		* capacity); //TODO remove - not operated on by orb
	this->parent	= malloc(sizeof(uint16_t)	* capacity);
}

void Transforms_clear(Transforms * this)
{
	memset(this->matrix, 0, this->capacity);
	memset(this->posLclPx, 0, this->capacity);
	memset(this->posWldPx, 0, this->capacity);
	memset(this->posNdc, 0, this->capacity); //TODO remove - not operated on by orb
	memset(this->parent, SIZE_MAX, this->capacity);
}

void Transforms_updateOne(Transforms * this, size_t index)
{
	//clear position world in order to build it up.
	for (int a = 0; a < 3; ++a) //TODO maybe i should iterate discrete axis list?
	{
		this->posWldPx[index][a] = this->posLclPx[index][a];
	}
	size_t ancestorIndex = this->parent[index];

	while (ancestorIndex != SIZE_MAX)
	{
		for (int a = 0; a < 3; ++a) //TODO maybe a should iterate discrete axis list?
		{
			this->posWldPx[index][a] += this->posLclPx[ancestorIndex][a];
		}
		ancestorIndex = this->parent[ancestorIndex];
	}
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
			exit(EXIT_FAILURE);
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

//------------------TextureAtlas/Entry------------------//
KHASH_DEFINE(Str_TextureAtlasEntry, kh_cstr_t, TextureAtlasEntry, kh_str_hash_func, kh_str_hash_equal, 1)

static khiter_t k;

TextureAtlas * TextureAtlas_construct()
{
	TextureAtlas * atlas = calloc(1, sizeof(TextureAtlas));
	atlas->entriesByName = kh_init(Str_TextureAtlasEntry);
	return atlas;
}

void TextureAtlas_load(TextureAtlas * atlas, const char * filename)
{
	ezxml_t atlasXml = ezxml_parse_file(filename);
	if (atlasXml)
		LOGI("ATLAS LOADED.\n");
	
	TextureAtlas_parse(atlas, atlasXml);
}

void TextureAtlas_parse(TextureAtlas * atlas, ezxml_t atlasXml)
{
	//LOGI("*********************************\n");
	
	atlas->w = (uint16_t)atoi(ezxml_attr(atlasXml, "width"));
	atlas->h = (uint16_t)atoi(ezxml_attr(atlasXml, "height"));
	
	for (ezxml_t xml = ezxml_child(atlasXml, "sprite"); xml; xml = xml->next)
	{
		TextureAtlasEntry entry = {0};
		//strcpy(entry.name, ezxml_attr(xml, "n"));
		entry.name = ezxml_attr(xml, "n");
		//LOGI("*********************************name=%s\n", entry.name);

		entry.x = (uint16_t)atoi(ezxml_attr(xml, "x"));
		entry.y = (uint16_t)atoi(ezxml_attr(xml, "y"));
		entry.w = (uint16_t)atoi(ezxml_attr(xml, "w"));
		entry.h = (uint16_t)atoi(ezxml_attr(xml, "h"));
		
		//optionals...
		const char * oX = ezxml_attr(xml, "oX");
		if (oX) entry.xTrim = atoi(oX);
		const char * oY = ezxml_attr(xml, "oY");
		if (oY) entry.yTrim = atoi(oY);
		const char * oW = ezxml_attr(xml, "oW");
		if (oW) entry.wPreTrim = atoi(oW);
		const char * oH = ezxml_attr(xml, "oH");
		if (oH) entry.hPreTrim = atoi(oH);
		const char * pX = ezxml_attr(xml, "pX");
		if (pX) entry.xPivot = atoi(pX);
		const char * pY = ezxml_attr(xml, "pY");
		if (pY) entry.yPivot = atoi(pY);
		
		TextureAtlas_put(atlas, entry);
	}
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
Shader * Shader_load(const char * path, const char * name, GLenum type)//, const char ** attributeLocations)
{
	Shader * shader = calloc(1, sizeof(Shader));
	strcpy(shader->name, name);//, STRLEN_MAX);
	size_t lengthName = strlen(name); 
	size_t lengthPath = strlen(path);
	char pathname[lengthPath+lengthName+1]; //1 = '\0'
	strcpy(pathname, path);
	strcat(pathname, name);
	LOGI("pathname %s\n", pathname);
	shader->source = Text_load(pathname);
	shader->type = type;
	shader->id = glCreateShader(shader->type);
	Shader_initialiseFromSource(shader);
	LOGI("name %s\n", shader->name);
	return shader;
}

void Shader_initialiseFromSource(Shader * this)//, const char* shader_str, GLenum shader_type)
{
	// Compile the shader
	GLuint id = this->id;
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
			exit(EXIT_FAILURE);
			
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

Program * Program_construct()
{
	Program * this = calloc(1, sizeof(Program));
	
	this->id = glCreateProgram();
	
	this->attributeLocationsByName = kh_init(StrPtr);
	//kv_init(this->attributeLocationsByIndex);
}

void Program_initialiseFromShaders(Program * this, GLuint vertex_shader, GLuint fragment_shader)
{
	GLuint id = this->id;
	
	// Attach the shaders to the program
	glAttachShader(id, vertex_shader);
	glAttachShader(id, fragment_shader);
	LOGI("gl error %i\n", glGetError());
	
	//note this must occur before link, see glBindAttribLocation docs	
	for (k = kh_begin(this->attributeLocationsByName); k != kh_end(this->attributeLocationsByName); ++k)	
	{
		if (kh_exist(this->attributeLocationsByName, k))
		{
			AttributeLocation * attributeLocation = &kh_value(this->attributeLocationsByName, k);
			glBindAttribLocation(id, attributeLocation->index, attributeLocation->name);
		}
	}
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
			exit(EXIT_FAILURE);
			
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
		glBindBuffer(GL_ARRAY_BUFFER, mesh->id);
		//glBufferData(GL_ARRAY_BUFFER, mesh->vertexBytes, mesh->vertexArray, mesh->usage);
		size_t offset = 0;
		for (int i = 0; i < kv_size(mesh->attributeActive); ++i)
		{
			Attribute * attribute = kv_A(mesh->attributeActive, i);
			glVertexAttribPointer(attribute->index, attribute->components, attribute->type, attribute->normalized, mesh->stride, offset); 
			glEnableVertexAttribArray(attribute->index); //enable attribute for use
			
			offset += attribute->components * glSizeof(attribute->type);//sizeof(float);
		}
	}
	
	UniformGroup_update(renderable->uniformsByName, this->program);

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

void Engine_initialise(Engine * this, int width, int height, const char * windowTitle)
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
	
	this->width = width;
	this->height = height;
	window = glfwCreateWindow(this->width, this->height, windowTitle, NULL, NULL);

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
	this->texturesByName 		= kh_init(StrPtr);
	
	glSizesByName			= kh_init(IntInt);
	kh_set(IntInt, glSizesByName, GL_BYTE, sizeof(char));
	kh_set(IntInt, glSizesByName, GL_UNSIGNED_BYTE, sizeof(GLubyte));
	kh_set(IntInt, glSizesByName, GL_FLOAT, sizeof(GLfloat));
	kh_set(IntInt, glSizesByName, GL_HALF_FLOAT, sizeof(GLfloat) / 2);
	kh_set(IntInt, glSizesByName, GL_INT, sizeof(GLint));
	kh_set(IntInt, glSizesByName, GL_UNSIGNED_INT, sizeof(GLuint));
	kh_set(IntInt, glSizesByName, GL_SHORT, sizeof(GLint));
	kh_set(IntInt, glSizesByName, GL_UNSIGNED_SHORT, sizeof(GLuint));
	
	//TODO include others under https://www.opengl.org/sdk/docs/man/html/glVertexAttribPointer.xhtml ->type
	
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

int _glSizeof(const int key)
{
	return kh_get_val(IntInt, glSizesByName, key, INT_MIN);	
}

Program * Engine_setCurrentProgram(Engine * this, char * name)
{
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
	
	//LOGI("@@ %s", path);
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


///////////// DEVICE TYPES //////////////

//KEYBOARD

void Keyboard_initialise(Device * device)
{
	device->indexToName = KeyString;
	device->nameToIndex = kh_init(StrInt);
	khiter_t k; int ret; int c = 0;
	FOREACH_KEY(GENERATE_KH)
	
	kv_init(device->channels);
	for (int i = 0; i < ORB_KEYS_COUNT; ++i)
	{
		DeviceChannel channel = {0};
		channel.active = true;	
		kv_push(DeviceChannel, device->channels, channel);		
	}
}

void Keyboard_update(Device * device)
{
	for (int i = 0; i < ORB_KEYS_COUNT; ++i)
	{
		GLFW_updateKey(i, device); //TODO should be some generic function pointer that has been preset to the GLFW function
	}
}

Device * Keyboard_construct()
{
	Device * device = Device_construct();
	device->initialise = Keyboard_initialise;
	device->update = Keyboard_update;
	return device;
}

//MOUSE

void Mouse_initialise(Device * device)
{
	device->indexToName = MouseButtonString;
	device->nameToIndex = kh_init(StrInt);
	khiter_t k; int ret; int c = 0;
	FOREACH_MOUSE_BUTTON(GENERATE_KH)
	
	kv_init(device->channels);
	for (int i = 0; i < ORB_MOUSE_BUTTONS_COUNT; ++i)
	{
		DeviceChannel channel = {0};
		channel.active = true;	
		kv_push(DeviceChannel, device->channels, channel);		
	}
}

void Mouse_update(Device * device)
{
	//LOGI("ORB_MOUSE_BUTTONS_COUNT=%d\n", ORB_MOUSE_BUTTONS_COUNT);
	for (int i = 0; i < ORB_MOUSE_BUTTONS_COUNT; ++i) //x & y
	{
		DeviceChannel * channel = &device->channels.a[i];
		
		if (i < 2) //first 2 are motion axes
		{
			//TODO if USE_GLFW
			//relative to mouse start point: For FPS
			double p[2];
			glfwGetCursorPos(window, &p[XX], &p[YY]);
			DeviceChannel_setPreviousState(channel);
			channel->state[CURRENT] = p[i];
			DeviceChannel_setCurrentDelta(channel);
		}
		else
		{
			DeviceChannel_setPreviousState(channel);
			//LOGI("buttonOrbToGlfw[i]=%d\n", buttonOrbToGlfw[i]);
			channel->state[CURRENT] = (float)glfwGetMouseButton(window, buttonOrbToGlfw[i]) == GLFW_PRESS;
			DeviceChannel_setCurrentDelta(channel);
		}
	}
}

Device * Mouse_construct()
{
	Device * device = Device_construct();
	device->initialise = Mouse_initialise;
	device->update = Mouse_update;
	return device;
}