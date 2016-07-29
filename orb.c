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

//The window we'll be rendering to


//OpenGL context
SDL_GLContext gContext;

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
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
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

/*
void Window_terminate(Engine * engine)
{
	LOGI("WINDOW_TERMINATE\n");
	
	#ifdef DESKTOP
	glfwDestroyWindow(window);
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
*/

Device * Device_construct()
{
	Device * device = calloc(1, sizeof(Device));
	device->nameToIndex = kh_init(StrInt);
	return device;
}
void Device_dispose(Device * this)
{
	kv_destroy(this->channels);
	kh_destroy(StrInt,  this->nameToIndex);
	//kvec_t(DeviceChannel) ;
	//khash_t(StrInt) * nameToIndex
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
 /*
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
	
		// app->savedState = malloc(sizeof(struct saved_state));
        // *((struct saved_state*)engine->app->savedState) = engine->state;
        // app->savedStateSize = sizeof(struct saved_state);

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
		
		// if (!engine->context)
		// {
		// engine->context = eglCreateContext(engine->display, engine->config, NULL, engine->attribsList);

		// if (eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context) == EGL_FALSE)
		// {
			// LOGW("Unable to eglMakeCurrent");
			// // return -1;
		// }
		// else
			// LOGI("Success eglMakeCurrent");
		
		// }
		
		break;
	}
}
*/
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
//must be same order as in corresponding header
static int buttonOrbToSDL[] =
{
	XX,
	YY/*,
	GLFW_MOUSE_BUTTON_1,
	GLFW_MOUSE_BUTTON_2,
	GLFW_MOUSE_BUTTON_3,
	GLFW_MOUSE_BUTTON_4,
	GLFW_MOUSE_BUTTON_5,
	GLFW_MOUSE_BUTTON_6,
	GLFW_MOUSE_BUTTON_7,
	GLFW_MOUSE_BUTTON_8
	*/
};
/*
#define GLFW_MOUSE_BUTTON_LEFT   GLFW_MOUSE_BUTTON_1
#define GLFW_MOUSE_BUTTON_RIGHT   GLFW_MOUSE_BUTTON_2
#define GLFW_MOUSE_BUTTON_MIDDLE   GLFW_MOUSE_BUTTON_3
#define GLFW_MOUSE_BUTTON_LAST   GLFW_MOUSE_BUTTON_8
*/

//must be same order as in corresponding header
static int keyOrbToSDL[] =
{
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_0,
	SDL_SCANCODE_1,
	SDL_SCANCODE_2,
	SDL_SCANCODE_3,
	SDL_SCANCODE_4,
	SDL_SCANCODE_5,
	SDL_SCANCODE_6,
	SDL_SCANCODE_7,
	SDL_SCANCODE_8,
	SDL_SCANCODE_9,
	SDL_SCANCODE_KP_0,
	SDL_SCANCODE_KP_1,
	SDL_SCANCODE_KP_2,
	SDL_SCANCODE_KP_3,
	SDL_SCANCODE_KP_4,
	SDL_SCANCODE_KP_5,
	SDL_SCANCODE_KP_6,
	SDL_SCANCODE_KP_7,
	SDL_SCANCODE_KP_8,
	SDL_SCANCODE_KP_9,
	SDL_SCANCODE_A,
	SDL_SCANCODE_B,
	SDL_SCANCODE_C,
	SDL_SCANCODE_D,
	SDL_SCANCODE_E,
	SDL_SCANCODE_F,
	SDL_SCANCODE_G,
	SDL_SCANCODE_H,
	SDL_SCANCODE_I,
	SDL_SCANCODE_J,
	SDL_SCANCODE_K,
	SDL_SCANCODE_L,
	SDL_SCANCODE_M,
	SDL_SCANCODE_N,
	SDL_SCANCODE_O,
	SDL_SCANCODE_P,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_R,
	SDL_SCANCODE_S,
	SDL_SCANCODE_T,
	SDL_SCANCODE_U,
	SDL_SCANCODE_V,
	SDL_SCANCODE_W,
	SDL_SCANCODE_X,
	SDL_SCANCODE_Y,
	SDL_SCANCODE_Z,
	SDL_SCANCODE_F1,
	SDL_SCANCODE_F2,
	SDL_SCANCODE_F3,
	SDL_SCANCODE_F4,
	SDL_SCANCODE_F5,
	SDL_SCANCODE_F6,
	SDL_SCANCODE_F7,
	SDL_SCANCODE_F8,
	SDL_SCANCODE_F9,
	SDL_SCANCODE_F10,
	SDL_SCANCODE_F11,
	SDL_SCANCODE_F12,
	SDL_SCANCODE_F13,
	SDL_SCANCODE_F14,
	SDL_SCANCODE_F15,
	SDL_SCANCODE_F16,
	SDL_SCANCODE_F17,
	SDL_SCANCODE_F18,
	SDL_SCANCODE_F19,
	SDL_SCANCODE_F20,
	SDL_SCANCODE_F21,
	SDL_SCANCODE_F22,
	SDL_SCANCODE_F23,
	SDL_SCANCODE_F24,
	SDL_SCANCODE_RIGHT,
	SDL_SCANCODE_LEFT,
	SDL_SCANCODE_DOWN,
	SDL_SCANCODE_UP,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_LCTRL,
	SDL_SCANCODE_LALT,
	SDL_SCANCODE_LGUI,
	SDL_SCANCODE_RSHIFT,
	SDL_SCANCODE_RCTRL,
	SDL_SCANCODE_RALT,
	SDL_SCANCODE_RGUI,
	SDL_SCANCODE_SPACE,
	SDL_SCANCODE_APOSTROPHE,
	SDL_SCANCODE_COMMA,
	SDL_SCANCODE_MINUS,
	SDL_SCANCODE_PERIOD,
	SDL_SCANCODE_SLASH,
	SDL_SCANCODE_SEMICOLON,
	SDL_SCANCODE_EQUALS,
	SDL_SCANCODE_BACKSLASH,
	SDL_SCANCODE_LEFTBRACKET,
	SDL_SCANCODE_RIGHTBRACKET,
	SDL_SCANCODE_GRAVE,
	SDL_SCANCODE_ESCAPE,
	SDL_SCANCODE_RETURN,
	SDL_SCANCODE_TAB,
	SDL_SCANCODE_BACKSPACE,
	SDL_SCANCODE_INSERT,
	SDL_SCANCODE_DELETE,
	SDL_SCANCODE_PAGEUP,
	SDL_SCANCODE_PAGEDOWN,
	SDL_SCANCODE_HOME,
	SDL_SCANCODE_END,
	SDL_SCANCODE_CAPSLOCK,
	SDL_SCANCODE_SCROLLLOCK,
	SDL_SCANCODE_NUMLOCKCLEAR,
	SDL_SCANCODE_PRINTSCREEN,
	SDL_SCANCODE_PAUSE,
	SDL_SCANCODE_KP_PERIOD,
	SDL_SCANCODE_KP_DIVIDE,
	SDL_SCANCODE_KP_MULTIPLY,
	SDL_SCANCODE_KP_MINUS,
	SDL_SCANCODE_KP_PLUS,
	SDL_SCANCODE_KP_ENTER,
	SDL_SCANCODE_KP_EQUALS,
	SDL_SCANCODE_MENU
	
	//TODO need to include the full list from https://wiki.libsdl.org/SDL_Keycode
};


#endif// DESKTOP //glfw!

void Loop_processInputs(Engine * engine)
{
	#ifdef DESKTOP //glfw!
	
	//update low-level input (raw)
	for (k = kh_begin(engine->devicesByName); k != kh_end(engine->devicesByName); ++k)
	{
		if (kh_exist(engine->devicesByName, k))
		{
			//char * key = kh_key(engine->devicesByName, k);
			Device * device = (Device *) kh_val(engine->devicesByName, k);
			device->consumed = false; //reset this each global update
			for (uint32_t j = 0; j < kv_size(device->channels); ++j) //for every raw input that may trigger said mapping
			{
				DeviceChannel * channel = &kv_A(device->channels, j);
				channel->consumed = false;
			}
			if (device->update)
				device->update(device);
		}
	}
	//TODO update high-level input (mappings)

	#endif//DESKTOP
}

bool Engine_initialiseWindowAndContext(Engine * engine, int width, int height, char * title)
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		SDL_version compiled;
		SDL_version linked;
		SDL_VERSION(&compiled);
		SDL_GetVersion(&linked);
		LOGI("SDL version compiled against: %d.%d.%d\n",
			   compiled.major, compiled.minor, compiled.patch);
		LOGI("SDL version linked   against: %d.%d.%d\n",
			   linked.major, linked.minor, linked.patch);
		if (compiled.major != compiled.major ||
			compiled.minor != compiled.minor ||
			compiled.patch != compiled.patch)
			success = false;
		else
		{
			//Create window
			engine->window = NULL; //or was this already done in engine = {0}?
			engine->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 400, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
			if( engine->window == NULL )
			{
				LOGI( "Window could not be created. SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				LOGI( "Window created.\n");
				//Create context
				gContext = SDL_GL_CreateContext( engine->window );
				if( gContext == NULL )
				{
					LOGI( "OpenGL context could not be created. SDL Error: %s\n", SDL_GetError() );
					success = false;
				}
				else
				{
					SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, GL_ES );
					SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, GL_VER_MAJOR );
					SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, GL_VER_MINOR );
					LOGI ("OpenGL %s %i.%i context created.\n", GL_ES ? "ES" : "", GL_VER_MAJOR, GL_VER_MINOR);
					
					//Initialize GLEW
					glewExperimental = GL_TRUE; 
					GLenum glewError = glewInit();
					if( glewError != GLEW_OK )
					{
						LOGI( "Error initializing GL Extension Wrangler. %s\n", glewGetErrorString( glewError ) );
					}
					else
					{
						LOGI( "GL Extension Wrangler initialised.\n");
					}

					//Use Vsync
					if( SDL_GL_SetSwapInterval( 1 ) < 0 )
					{
						LOGI( "Warning: Could not enable VSync. SDL Error: %s\n", SDL_GetError() );
					}
					else
					{
						LOGI( "VSync enabled.\n");
						
					}
				}
			}
		}
	}

	return success;
}

static float deltaSecFixed = (float) 1.0f / 60.f;


void Loop_initialise(Engine * engine, int windowWidth, int windowHeight, const char * windowTitle)
{
	/*
	#ifdef __ANDROID__
	//app_dummy();
	struct android_app * app = engine->app;
	app->userData = engine;
	app->onAppCmd = Android_onAppCmd;
	app->onInputEvent = Android_onInputEvent;
	


	#endif//__ANDROID__
	*/
	
	//TODO #if ORB_FIXED_REFRESH_RATE
	engine->deltaSec = deltaSecFixed;
	
	#ifdef DESKTOP
	Engine_initialise(engine, windowWidth, windowHeight, windowTitle); 
	engine->userInitialiseFunc(); //!!! For now, Ctrl_init must go after Engine_initialise(), because it still relies on glfw for input AND glReadPixels
	engine->userInitialised = true;
	#endif//DESKTOP
}
static int nbFrames = 0;

void Loop_run(Engine * engine)
{
	#ifdef DESKTOP
	double t1, t2 = 0;
	//else
	if (true) //TODO remove - see crazy foo SDL tut
	{
		bool quit = false;
		SDL_Event e;
		SDL_StartTextInput();
		
		while( !quit )
		{
			//Handle events on queue
			while( SDL_PollEvent( &e ) != 0 )
			{
				//User requests quit
				if( e.type == SDL_QUIT )
				{
					quit = true;
				}
				//Handle keypress with current mouse position
				else if( e.type == SDL_TEXTINPUT )
				{
					if(  e.text.text[ 0 ] == 'q' )
					{
						quit = true;
						break;
					}
				}
				else if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
				{
					//LOGI("KEY\n");
				}
			}

			Loop_processInputs(engine);
		
			engine->userUpdateFunc(engine->userUpdateArg);
			
			//Update screen
			SDL_GL_SwapWindow( engine->window );
		}
		
		//Disable text input
		SDL_StopTextInput();
	}

	//Free resources and close SDL
	/*
	while (!glfwWindowShouldClose(window))
	{

		
		//t2 = t1;
		t1 = glfwGetTime();
		engine->deltaSec = t1 - t2;
		//printf("deltaSec %.10f\n", engine->deltaSec);
		
		nbFrames++;

		
		// if ( engine->deltaSec >= 1.0 ) // If last cout was more than 1 sec ago
		// {
			// char title [256];
			// title [255] = '\0';

			// snprintf ( title, 255,
					 // "%s v.%s - [FPS: %3.2f]",
					   // "War & Adventure", "0.00", (float)nbFrames / engine->deltaSec);

			// glfwSetWindowTitle (window, title);

			// nbFrames = 0;
			// // t2 += 1.0;
			// // t2 = t1;
		// }
		
		
		//if (deltaSecFixed >= engine->deltaSec)
		//{
		glfwPollEvents();
		
		Loop_processInputs(engine);
		
		engine->userUpdateFunc(engine->userUpdateArg);
		
		glfwSwapBuffers(window);
		
		t2 = t1;
	}
	*/
	#elif MOBILE
	/*
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
	
		for (uint32_t i = 0; i < 8; i++)
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
	*/
	#endif//DESKTOP/MOBILE
}

//Updates uniforms for the current program.
void UniformGroup_update(khash_t(StrPtr) * uniformsByName, Program * programPtr)
{
	for (k = kh_begin(uniformsByName); k != kh_end(uniformsByName); ++k)
	{
        if (kh_exist(uniformsByName, k))
		{
			//const char * key = kh_key(uniformsByName,k);
			//LOGI("key=%s\n", key);
			Uniform * uniform = (Uniform *) kh_value(uniformsByName, k);
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

void Mesh_removeAttributes(Mesh * this)
{
	/*
		for (uint32_t i = 0; i < kv_size(this->attribute); ++i)
		{
			Attribute * attribute = kv_A(this->attribute, i);
*/
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

void Mesh_dispose(Mesh * this)
{
	free(this->index);
	free(this->vertexArray);
	kv_destroy(this->attributeActive);
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

void Mesh_appendTri(Mesh * mesh, GLushort a, GLushort b, GLushort c)
{
	//LOGI("MESH index count=%d a=%u b=%u c=%u\n", mesh->indexCount, a, b, c);
	mesh->index[mesh->indexCount + 0] = a;
	mesh->index[mesh->indexCount + 1] = b;
	mesh->index[mesh->indexCount + 2] = c;

	mesh->indexCount+=3;
	//TODO reallocate array if indexCount > size
}

void Mesh_appendLine(Mesh * mesh, GLushort a, GLushort b)
{
	//LOGI("MESH index count=%d a=%u b=%u c=%u\n", mesh->indexCount, a, b, c);
	mesh->index[mesh->indexCount + 0] = a;
	mesh->index[mesh->indexCount + 1] = b;

	mesh->indexCount+=2;
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
		void * offset = NULL;
		//ORDER HERE *MUST* MATCH THAT OF VERTEX STRUCT MEMBERS! - set via Mesh_initialise()
		for (uint32_t i = 0; i < kv_size(mesh->attributeActive); ++i)
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

//ASSUMES TRIS!
void Mesh_merge(Mesh * this, Mesh * other)
{
	int vertexCount = this->vertexCount;
	for (int v = 0; v < other->vertexCount; ++v)
	{
		Mesh_appendVertex(this, ((char *)other->vertexArray) + v * other->stride);		
	}
	for (int i = 0; i < other->indexCount; i+=3)
	{
		Mesh_appendTri(this, 	vertexCount + other->index[i+0],
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
		//int j = a * 14; //index into triangle index array
		
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


//------------------Renderable-----------------//

void IndexedRenderableManager_create(
	Renderable * const renderables,
	IndexList * const indexRenderListPtr,
	IndexList * const indexListPtr,
	IndexedRenderableFunction fnc,
	void * model)
{
	for (uint32_t ii = 0; ii < kv_size(*indexListPtr); ii++)
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
	for (uint32_t ii = 0; ii < kv_size(*indexListPtr); ii++)
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
	for (uint32_t ii = 0; ii < kv_size(*indexRenderListPtr); ++ii)
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

//ASSUMES RGB!
void Texture_clearData(Texture * texture, bool alphaFull)
{
	memset(texture->data, 0, texture->width*texture->height*texture->components*sizeof(GLubyte));
	if (alphaFull)
	{
		for (uint32_t i = 3; i < texture->width*texture->height*texture->components*sizeof(GLubyte); i+=4)
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
	
	//return texture;
}

void RenderTexture_createDepth(Texture * const this, GLuint i, uint16_t width, uint16_t height)
{
	this->unit = i;
	this->width = width;
	this->height = height;
	
	Texture_setDimensionCount(this, GL_TEXTURE_2D);
	Texture_setTexelFormats(this, GL_DEPTH_COMPONENT16, GL_FLOAT);
	
	//int ret, is_missing;
	
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
	
	//int ret, is_missing;
	
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
	glBindTexture(this->dimensions, this->id); //ensure the correct texture is bound to the texture unit that the shader will use (?)
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
	glBindTexture(this->dimensions, this->id); //binds the texture with id specified, to the 2D target
	glTexImage2D (GL_TEXTURE_2D, 0, this->arrangedInternal, this->width, this->height, 0, this->arrangedExternal, this->atomTypeExternal, 0);
}

void Texture_refresh(Texture * this)
{
	glActiveTexture(GL_TEXTURE0 + this->unit); //"glActiveTexture specifies which texture unit a texture object is bound to when glBindTexture is called."
	glBindTexture(this->dimensions, this->id); //binds the texture with id specified, to the 2D target
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
	glBindTexture(this->dimensions, this->id);

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

//------------------Atlas/Entry------------------//
KHASH_DEFINE(Str_AtlasEntry, kh_cstr_t, AtlasEntry, kh_str_hash_func, kh_str_hash_equal, 1)

static khiter_t k;

Atlas * Atlas_construct()
{
	Atlas * atlas = calloc(1, sizeof(Atlas));
	atlas->entriesByName = kh_init(Str_AtlasEntry);
	return atlas;
}

void Atlas_load(Atlas * atlas, const char * filename)
{
	atlas->config = ezxml_parse_file(filename);
	if (atlas->config)
		LOGI("ATLAS LOADED.\n");
	
	Atlas_parse(atlas, (ezxml_t)atlas->config);
}

void Atlas_parse(Atlas * atlas, ezxml_t atlasXml) //don't need ezxml-typed parameter? take from atlas->config? but then how can generic function know what to do?
{
	//LOGI("*********************************\n");
	
	atlas->w = (uint16_t)atoi(ezxml_attr(atlasXml, "width"));
	atlas->h = (uint16_t)atoi(ezxml_attr(atlasXml, "height"));
	
	for (ezxml_t xml = ezxml_child(atlasXml, "sprite"); xml; xml = xml->next)
	{
		AtlasEntry entry = {0};
		//strcpy(entry.name, ezxml_attr(xml, "n"));
		entry.name = (char *) ezxml_attr(xml, "n");
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
		
		Atlas_put(atlas, entry);
	}
}

void Atlas_dispose(Atlas * atlas)
{
	ezxml_free((ezxml_t)atlas->config);
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
	#ifdef   ORB_DEBUG_SHADER_PROGRAMS
	LOGI("pathname %s name %s\n", pathname, shader->name);
	#endif //ORB_DEBUG_SHADER_PROGRAMS
	shader->source = Text_load(pathname);
	shader->type = type;
	shader->id = glCreateShader(shader->type);
	Shader_initialiseFromSource(shader);
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
		#ifdef   ORB_DEBUG_SHADER_PROGRAMS
		LOGI("glCompileShader() failed:\n");
		#endif //ORB_DEBUG_SHADER_PROGRAMS
		if(infoLogLength > 1)
		{
			//GLchar infoLog[sizeof(char) * infoLogLength + 1];
			char* infoLog = malloc(sizeof(char) * infoLogLength);
			glGetShaderInfoLog(id, infoLogLength, NULL, infoLog);
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("error: %s\n", infoLog);
			LOGI("source: \n%s\n", this->source);
			#endif //ORB_DEBUG_SHADER_PROGRAMS
			//TODO encapsulate the below in a exitOnFatalError() that can be used anywhere.
			//TODO set up error codes for untimely exit.
			#ifdef DESKTOP
			//glfwTerminate();
			#endif//DESKTOP
			exit(EXIT_FAILURE);
			
			glDeleteShader(id);
			
			free(infoLog);
		}
		else
		{
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("<no GL info log available>\n");
			#endif //ORB_DEBUG_SHADER_PROGRAMS
		}
	}
	else
	{
		#ifdef   ORB_DEBUG_SHADER_PROGRAMS
		LOGI("glCompileShader() success.\n");
		#endif //ORB_DEBUG_SHADER_PROGRAMS
		GLint infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		if(infoLogLength > 1)
		{
			GLchar infoLog[sizeof(char) * infoLogLength + 1];
			glGetShaderInfoLog(id, infoLogLength + 1, NULL, infoLog);
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("%s\n", infoLog);//, this->source);
			#endif //ORB_DEBUG_SHADER_PROGRAMS
		}
		else
		{
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("<no GL info log available>\n");
			#endif //ORB_DEBUG_SHADER_PROGRAMS
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
	
	this->attributeLocationsByName = kh_init(Str_AttributeLocation);
	//kv_init(this->attributeLocationsByIndex);
	return this;
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
		#ifdef   ORB_DEBUG_SHADER_PROGRAMS
		LOGI("glLinkProgram() failed:\n");
		#endif //ORB_DEBUG_SHADER_PROGRAMS
		
		GLint infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		
		if (infoLogLength > 1)
		{
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("infoLogLength=%i\n", infoLogLength);
			#endif //ORB_DEBUG_SHADER_PROGRAMS
			//GLchar infoLog[sizeof(char) * infoLogLength + 1];
			char* infoLog = malloc(sizeof(char) * infoLogLength);
			glGetProgramInfoLog(id, infoLogLength, NULL, infoLog);
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("%s\n", infoLog);
			#endif //ORB_DEBUG_SHADER_PROGRAMS
			
			#ifdef DESKTOP
			//glfwTerminate();
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
			#ifdef   ORB_DEBUG_SHADER_PROGRAMS
			LOGI("<no GL info log available>\n");
			#endif //ORB_DEBUG_SHADER_PROGRAMS
		}
	}
	else
	{
		#ifdef   ORB_DEBUG_SHADER_PROGRAMS
		LOGI("glLinkProgram() success.\n");
		#endif //ORB_DEBUG_SHADER_PROGRAMS
	}
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

//TODO glBufferSubData(GL_ARRAY_BUFFER,
//TODO consider storing 2 buffers for each dynamic object - from https://www.opengl.org/sdk/docs/man3/xhtml/glBufferSubData.xml
//"Consider using multiple buffer objects to avoid stalling the rendering pipeline during data store updates.
	//If any rendering in the pipeline makes reference to data in the buffer object being updated by glBufferSubData, especially
	//from the specific region being updated, that rendering must drain from the pipeline before the data store can be updated."
void Engine_many(Engine * this, Renderable * renderable, RenderableInstances * instances)
{
	Mesh * mesh = renderable->mesh;
	
	//bind attributes
	if (this->capabilities.vao)
		glBindVertexArray(mesh->vao);
	else
	{				
		glBindBuffer(GL_ARRAY_BUFFER, mesh->id);
		//glBufferData(GL_ARRAY_BUFFER, mesh->vertexBytes, mesh->vertexArray, mesh->usage);
		void * offset = NULL;
		for (uint32_t i = 0; i < kv_size(mesh->attributeActive); ++i)
		{
			Attribute * attribute = kv_A(mesh->attributeActive, i);
			glVertexAttribPointer(attribute->index, attribute->components, attribute->type, attribute->normalized, mesh->stride, offset); 
			glEnableVertexAttribArray(attribute->index); //enable attribute for use
			
			offset += attribute->components * glSizeof(attribute->type);//sizeof(float);
		}
	}
	
	//bind instanced attributes
	glBindBuffer(GL_ARRAY_BUFFER, instances->buffer); //TODO use const instead of literal buffer name
	glBufferData(GL_ARRAY_BUFFER, instances->sizeofElement * instances->count, instances->data, GL_DYNAMIC_DRAW); 
		
	//potentially split over multiple attributes to accomodate greater attribute in question (matrices)
	//HERE BE HACKS...
	GLuint firstLocation = 3;
	const int MAT4X4_COMPONENTS = 16;
	const int VEC4_COMPONENTS = 4; //the max line size that an attribute can accomodate
	int attributeCount = MAT4X4_COMPONENTS / VEC4_COMPONENTS; //number of attributes needed to accomodate the variable in question
	
	for (int i = 0; i < attributeCount; ++i)
	{
		int j = firstLocation + i;
		glEnableVertexAttribArray(j); 
		glVertexAttribPointer(j, VEC4_COMPONENTS, GL_FLOAT, GL_FALSE, attributeCount * sizeof(vec4), (GLvoid*)(i * sizeof(vec4)));
		glVertexAttribDivisor(j, 1);
	}
	//...HERE BE HACKS.

	//uniforms
	UniformGroup_update(renderable->uniformsByName, this->program);

	//render
	if (mesh->index == NULL)
	{
		//TODO
		//glDrawArrays(mesh->topology, 0, mesh->vertexCount);
		//glDrawArraysInstanced(mesh->topology, 0, mesh->vertexCount, GL_UNSIGNED_SHORT, mesh->index, renderableSet->count);
	}
	else
		//glDrawElements(mesh->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index);
		glDrawElementsInstanced(mesh->topology, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->index, instances->count);
		
	//unbind attributes
	if (this->capabilities.vao)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0); //vertex attribute array target no longer bound
		glBindVertexArray(0);
	}
}

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
		void * offset = NULL;
		for (uint32_t i = 0; i < kv_size(mesh->attributeActive); ++i)
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
void glUniform1fv_wrapper(int a, int b, const int * c)
{
	glUniform1fv(a, b, (const float *) c);
}
void glUniform2fv_wrapper(int a, int b, const int * c)
{
	glUniform2fv(a, b, (const float *) c);
}
void glUniform3fv_wrapper(int a, int b, const int * c)
{
	glUniform3fv(a, b, (const float *) c);
}
void glUniform4fv_wrapper(int a, int b, const int * c)
{
	glUniform4fv(a, b, (const float *) c);
}

void Engine_initialise(Engine * this, int width, int height, const char * windowTitle)
{
	LOGI("[ORB] Engine initialise...\n");
	bool success = Engine_initialiseWindowAndContext(this, this->width, this->height, windowTitle);
	//LOGI("[ORB] %s\n",  success ? "...Engine initialise" : "Engine failed to initialise.");
	if (success)
	{
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	
	/*
	int maj, min, rev;
	glfwGetVersion(&maj, &min, &rev);
	LOGI("GLFW version %i.%i.%i\n", maj, min, rev);
	engine = this; //DEV, for glfwSetCursorPosCallback
	#ifdef DESKTOP
	//WINDOW, CONTEXT & INPUT
	//glfwSetErrorCallback(GLFW_errorCallback);
	
	if (!glfwInit())
		exit(EXIT_FAILURE);
	else
		printf("GLFW initialised.\n");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	//glfwWindowHint(GLFW_SAMPLES, 4); //HW AA
	glfwWindowHint(GLFW_REFRESH_RATE, 60);
	
	this->width = width;
	this->height = height;
	window = glfwCreateWindow(this->width, this->height, windowTitle, //glfwGetPrimaryMonitor()
		NULL, NULL);

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
	*/
	
	
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
	
	/*
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
	*/
	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	/*
	eglChooseConfig(this->display, configAttribs, &this->config, 1, &numConfigs);

	//LOGI("numConfigs=%d",numConfigs);
	*/
	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	/*
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
	*/
	
	//MISCELLANEOUS
	
	//initialise collection objects
	//TODO "this" should be "orb" instance

	this->programsByName 	= kh_init(StrPtr);
	this->shadersByName 		= kh_init(StrPtr);
	this->devicesByName 		= kh_init(StrPtr);
	this->texturesByName 		= kh_init(StrPtr);
	this->atlasesByName 		= kh_init(StrPtr);
	this->meshesByName 		= kh_init(StrPtr);
	
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
	
	glUniformVectorFunctions[1-1][1] = glUniform1iv;
	glUniformVectorFunctions[2-1][1] = glUniform2iv;
	glUniformVectorFunctions[3-1][1] = glUniform3iv;
	glUniformVectorFunctions[4-1][1] = glUniform4iv;
	
	//wrapper functions allow us to use a void (*) (int, int, const * int for all without compiler warning about incompatible signatures)
	//bearing in mind - C cannot cast function signature types easily - try with the non-wrapped versions to see the warnings.
	glUniformVectorFunctions[1-1][0] = glUniform1fv_wrapper;
	glUniformVectorFunctions[2-1][0] = glUniform2fv_wrapper;
	glUniformVectorFunctions[3-1][0] = glUniform3fv_wrapper;
	glUniformVectorFunctions[4-1][0] = glUniform4fv_wrapper;

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
	LOGI("[ORB] ...Engine initialise\n");
	}
	else
		exit(EXIT_FAILURE);
}

void Engine_dispose(Engine * engine)
{	
	//TODO free engine collections.
	kh_cstr_t name;
	
	Mesh * mesh;
	kh_foreach(engine->meshesByName, name, mesh, 
		LOGI("removed %s.\n", name);
		Engine_removeMesh(engine, name);
	)
	kh_destroy(StrPtr, engine->meshesByName);
	
	Device * device;
	kh_foreach(engine->devicesByName, name, device, 
		LOGI("removed %s.\n", name);
		//Engine_removeDevice(engine, name);
		device->dispose(device);
		free (device);
	)
	kh_destroy(StrPtr, engine->devicesByName);
	
	Atlas * atlas;
	kh_foreach(engine->atlasesByName, name, atlas, 
		LOGI("removed %s.\n", name);
		Engine_removeAtlas(engine, name);

	)
	kh_destroy(StrPtr, engine->atlasesByName);
	
	//Atlas_dispose(atlas); //TODO actually, all atlases should be tracked and removed in Orb
	
	/*
	#ifdef DESKTOP
	Window_terminate(engine);
	#endif//DESKTOP
	*/
	//Deallocate program
	//glDeleteProgram( gProgramID );

	//Destroy window	
	SDL_DestroyWindow( engine->window );
	engine->window = NULL;

	//Quit SDL subsystems
	SDL_Quit();
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
		this->program = (Program *) kh_val(this->programsByName, k);
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
void Engine_getPath(Engine * engine, const char * path, int pathLength, const char * partial)
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
	strcpy((char *)path, prefix); 
	strcat((char *)path, partial); 
	///path = pathTemp;
	#else //all other supported OS?
	//relative path?
	//chdir("/sdcard/");
	//char * path = "./shd/";
	#endif //OS
	
	LOGI("@@ %s", path);
}

Mesh * Engine_addMesh(Engine * this, const char * name)
{
	Mesh * mesh = calloc(1, sizeof(Mesh));
	strncpy(mesh->name, name, STRLEN_MAX); //since we just calloc'ed, we can assume mesh->name is all zero bytes
	//..also, strncpy will silently truncate excessively long names
	kh_set(StrPtr, this->meshesByName, mesh->name, (uintptr_t) mesh);	
	return mesh;
}

//TODO use an arg "poolOnRemove" which sends it to a pool for reuse by _addMesh(), which should be modified to use pooled instances.
void Engine_removeMesh(Engine * this, const char * name)
{
	Mesh * mesh = (Mesh *) kh_get_val(StrPtr, this->meshesByName, name, NULL);	
	//it makes sense to dispose here, but shouldn't we pool the members that were disposed...?
	Mesh_dispose(mesh);
	free(mesh);
}

Atlas *  Engine_addAtlas(Engine * this, const char * name)
{
		Atlas * atlas = Atlas_construct();
		return atlas;
}

//TODO use an arg "poolOnRemove" which sends it to a pool for reuse by _addMesh(), which should be modified to use pooled instances.
void Engine_removeAtlas(Engine * this, const char * name)
{
	Atlas * atlas = (Atlas *) kh_get_val(StrPtr, this->atlasesByName, name, NULL);	
	//it makes sense to dispose here, but shouldn't we pool the members that were disposed...?
	Atlas_dispose(atlas);
	free(atlas);
}

char* Text_load(char* filename)
{
	//should be portable - http://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer
	
	char * str = NULL;

	FILE *file = fopen(filename, "rb"); //open for reading
	if (file)
	{
		fseek(file, 0, SEEK_END); //seek to end
		long fileSize = ftell(file); //get current position in stream
		fseek(file, 0, SEEK_SET); //seek to start
		
		#ifdef   DEBUG_TEXT_LOADING
		LOGI("[ORB](loadTextFile) filename=%s\n", filename);
		#endif //DEBUG_TEXT_LOADING
		
		if (fileSize > -1)
		{
			str = malloc(fileSize + 1); //allocate enough room for file + null terminator (\0)

			if (str != NULL) //if allocation succeeded
			{
				#ifdef   DEBUG_TEXT_LOADING
				LOGI("[ORB](loadTextFile) fileSize...%ld\n", fileSize);
				#endif //DEBUG_TEXT_LOADING

				size_t freadResult;
				freadResult = fread(str, 1, fileSize, file); //read elements as one byte each, into string, from file. 
				//LOGI("freadResult...%d\n", freadResult);
				
				if (freadResult != (uint64_t) fileSize)
				{
					fputs ("Reading error", stderr);//TODO fix to use LOGE()
					//exit (3);
					
					str = NULL;
					return str;
				}
				
				

				str[fileSize] = 0; //'\0';
			}
		}
		#ifdef   DEBUG_TEXT_LOADING
		else
			LOGI("[ORB](loadTextFile) fileSize invalid: %ld, errno=%i\n", fileSize, errno);
		#endif //DEBUG_TEXT_LOADING

		fclose(file);
	}
	#ifdef   DEBUG_TEXT_LOADING
	else
		LOGI("File not found: %s\n", filename);
	#endif //DEBUG_TEXT_LOADING
	
	return str;
}

float Engine_smoothstep(float t)
{
	return 3 * t * t - 2 * t * t * t;
}


///////////// INPUT MAPPING //////////////

///////////// DEVICE TYPES //////////////

//KEYBOARD

void Keyboard_initialise(Device * device)
{
	device->indexToName = (char**)KeyString;
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

void Key_update(int i, Device * keyboard, uint8_t * state)
{
	DeviceChannel * channel = &keyboard->channels.a[i]; //orb index as from enum Key
	DeviceChannel_setPreviousState(channel);
	channel->state[CURRENT] = 
		(float)state[keyOrbToSDL[i]];
	DeviceChannel_setCurrentDelta(channel);
}

void Keyboard_update(Device * device)
{
	uint8_t * state = SDL_GetKeyboardState(NULL);
	for (int i = 0; i < ORB_KEYS_COUNT; ++i)
	{
		Key_update(i, device, state);
	}
}

Device * Keyboard_construct()
{
	Device * device = Device_construct();
	device->initialise = Keyboard_initialise;
	device->update = Keyboard_update;
	device->dispose = Device_dispose;
	return device;
}

void Mouse_initialise(Device * device)
{
	device->indexToName = (char**)MouseButtonString;
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
	int state[2] = {0, 0};
	int delta[2] = {0, 0};
	//TODO SDL_GetMouseState()
	//if (channel->basis == DELTA)
	//else //channel->basis == STATE
	SDL_GetRelativeMouseState(&delta[XX], &delta[YY]); //do not move into loop - it clears state on read!
	
	for (int i = 0; i < ORB_MOUSE_BUTTONS_COUNT; ++i) //x & y
	{
		DeviceChannel * channel = &device->channels.a[i];
		
		if (i < 2) //first 2 are motion axes
		{
			DeviceChannel_setPreviousState(channel);
			channel->delta[CURRENT] = delta[i];
			DeviceChannel_setCurrentState(channel);
		
			/*	
			DeviceChannel_setPreviousState(channel);
			channel->state[CURRENT] = state[i];
			DeviceChannel_setCurrentDelta(channel);
			*/			
		}
		else //rest are buttons
		{
			// DeviceChannel_setPreviousState(channel);
			uint32_t buttonFlags;
			
			//if (channel->basis == STATE)
			//SDL_GetRelativeMouseState
			// channel->state[CURRENT] = (float)glfwGetMouseButton(window, buttonOrbToSDL[i]) == GLFW_PRESS;
			// DeviceChannel_setCurrentDelta(channel);
		}
	}
	
	LOGI("mx=d%f, s%f p%f\n", device->channels.a[XX].delta[CURRENT], device->channels.a[XX].state[CURRENT], device->channels.a[XX].state[PREVIOUS]);
	LOGI("my=d%f, s%f p%f\n", device->channels.a[YY].delta[CURRENT], device->channels.a[YY].state[CURRENT], device->channels.a[YY].state[PREVIOUS]);
}

Device * Mouse_construct()
{
	Device * device = Device_construct();
	device->initialise = Mouse_initialise;
	device->update = Mouse_update;
	device->dispose = Device_dispose;
	return device;
}