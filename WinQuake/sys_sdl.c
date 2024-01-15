#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifndef __WIN32__
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#endif

#include "quakedef.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

qboolean			isDedicated;

int noconinput = 0;

char *basedir = ".";
char *cachedir = "/tmp";

cvar_t  sys_linerefresh = {"sys_linerefresh","0"};// set for entity display
cvar_t  sys_nostdout = {"sys_nostdout","0"};

// Shared variables used in main / main_loop for Emscripten
static double time, oldtime, newtime;
extern int vcrFile;
extern int recording;
static int frame;
#ifdef __EMSCRIPTEN__
static qboolean restore_busy;
#endif

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val)
{
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	
	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);
	fprintf(stdout, "%s", text);
	
	//Con_Print (text);
}

void Sys_Quit (void)
{
    Host_Shutdown();
#ifdef __EMSCRIPTEN__
    EM_ASM(
        if (typeof Module.showConsole === 'function')
            Module.showConsole();
    );
    // Don't come back to the main loop after exiting, but do finish any async
    // saves
    emscripten_pause_main_loop();
#endif
    exit(0);
}

void Sys_Init(void)
{
#if id386
	Sys_SetFPCW();
#endif
}

#if !id386

/*
================
Sys_LowFPPrecision
================
*/
void Sys_LowFPPrecision (void)
{
// causes weird problems on Nextstep
}


/*
================
Sys_HighFPPrecision
================
*/
void Sys_HighFPPrecision (void)
{
// causes weird problems on Nextstep
}

#endif	// !id386


void Sys_Error (char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];

    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
	fprintf(stdout, "Error: %s\n", string);

	Host_Shutdown ();
	exit (1);

} 

void Sys_Warn (char *warning, ...)
{ 
    va_list     argptr;
    char        string[1024];
    
    va_start (argptr,warning);
    vsprintf (string,warning,argptr);
    va_end (argptr);
	fprintf(stdout, "Warning: %s", string);
} 

/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
Qfilelength
================
*/
static int Qfilelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;
	
	return Qfilelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	if ( handle >= 0 ) {
		fclose (sys_handles[handle]);
		sys_handles[handle] = NULL;
	}
}

void Sys_FileSeek (int handle, int position)
{
	if ( handle >= 0 ) {
		fseek (sys_handles[handle], position, SEEK_SET);
	}
}

int Sys_FileRead (int handle, void *dst, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = dst;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle]);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
		
}

int Sys_FileWrite (int handle, void *src, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = src;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle]);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
#ifdef __WIN32__
    mkdir (path);
#else
    mkdir (path, 0777);
#endif
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr; 
    static char data[1024];
    FILE *fp;
    
    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
    fp = fopen(file, "a");
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
}

double Sys_FloatTime (void)
{
#ifdef __WIN32__

	static int starttime = 0;

	if ( ! starttime )
		starttime = clock();

	return (clock()-starttime)*1.0/1024;

#else

    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;

#endif
}

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
    return 0;
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x)
{
	oktogo=1;
}

byte *Sys_ZoneBase (int *size)
{

	char *QUAKEOPT = getenv("QUAKEOPT");

	*size = 0xc00000;
	if (QUAKEOPT)
	{
		while (*QUAKEOPT)
			if (tolower(*QUAKEOPT++) == 'm')
			{
				*size = atof(QUAKEOPT) * 1024*1024;
				break;
			}
	}
	return malloc (*size);

}

void Sys_LineRefresh(void)
{
}

void Sys_Sleep(void)
{
	// Sleeping is now done outside the main loop with Emscripten
#ifndef __EMSCRIPTEN__
	SDL_Delay(1);
#endif
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}

void moncontrol(int x)
{
}

#ifdef __EMSCRIPTEN__
EM_JS(int, wasm_restore_busy, (), {
	return Module.restore_busy;
});

void wasm_init_fs(void)
{
	// Sync from IDBFS in the background
	EM_ASM(
		Module.restore_busy = 1;
#ifdef WASM_SAVE_PAKS
		FS.mkdir("/id1");
		FS.mount(IDBFS, {}, "/id1");
#endif
		FS.mkdir("/quake-wasm");
		FS.mount(IDBFS, {}, "/quake-wasm");
		console.info("Loading data...");
		FS.syncfs(true, function (err) {
			if (err)
				console.warn("Failed to load data: " + err);
			else
				console.info("Data loaded.");

			Module.restore_busy = 0;
		});
	);
}

void wasm_sync_fs(void)
{
	// Sync to IDBFS in the background
	EM_ASM(
		console.info("Saving data...");
		FS.syncfs(function (err) {
			if (err)
				console.warn("Failed to save data: " + err);
			else
				console.info("Data saved.");
		});
	);
}
#endif

int main (int c, char **v)
{
	quakeparms_t parms;
	int pnum;

#ifdef __EMSCRIPTEN__
	wasm_init_fs();
#endif

	moncontrol(0);

//	signal(SIGFPE, floating_point_exception_handler);
	signal(SIGFPE, SIG_IGN);

    COM_InitArgv(c, v);
    parms.argc = com_argc;
    parms.argv = com_argv;
    parms.memsize = 16*1024*1024;

    // Support for -mem and -heapsize parameters
    if ((pnum = COM_CheckParm("-mem")))
        parms.memsize = Q_atoi(com_argv[pnum + 1]) * 1024 * 1024;

    if ((pnum = COM_CheckParm("-heapsize")))
        parms.memsize = Q_atoi(com_argv[pnum + 1]) * 1024;

    parms.membase = malloc (parms.memsize);
    parms.basedir = basedir;
    parms.cachedir = NULL;  // Using 'cachedir' stops standalone .cfg files from execing

    Sys_Init();

    Host_Init(&parms);

    Con_Printf("\nQuake version 1.09 by id Software\n\n");
    Con_Printf("Based on the SDL patch from\n");
    Con_Printf("libsdl.org\n\n");
    Con_Printf("SDL2 & WASM conversion by\n");
    Con_Printf("Gregory Maynard-Hoare\n\n");

    if (com_argc > 1)
    {
        Con_Printf("Startup args:");

        for (pnum=1; pnum<com_argc; pnum++)
            Con_Printf(" %s", com_argv[pnum]);

        Con_Printf("\n\n");
    }

    Cvar_RegisterVariable (&sys_nostdout);

    oldtime = Sys_FloatTime () - 0.1;

#ifdef __EMSCRIPTEN__
    restore_busy = true;
#ifdef WASM_BENCHMARK
    emscripten_set_main_loop_timing(EM_TIMING_SETIMMEDIATE, 0);
#endif  // WASM_BENCHMARK
    emscripten_set_main_loop(main_loop, 0, 0);
#else
    while (1)
        main_loop();
#endif  // __EMSCRIPTEN__
}

void main_loop(void)
{
#ifdef __EMSCRIPTEN__
        if (restore_busy) {
            // Call JavaScript code to check restore status
            if (wasm_restore_busy())
                return;

            EM_ASM(
                if (typeof Module.hideConsole === 'function')
                    Module.hideConsole();
            );

            restore_busy = false;
        }
#endif

        // find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
                Sys_Sleep();
                return;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        if (++frame > 10)
            moncontrol(1);      // profile only while we do each Quake frame
        Host_Frame (time);
        moncontrol(0);

        // graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();
}


/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

	int r;
	unsigned long addr;
	int psize = getpagesize();

	fprintf(stdout, "writable code %lx-%lx\n", startaddr, startaddr+length);

	addr = startaddr & ~(psize-1);

	r = mprotect((char*)addr, length + startaddr - addr, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");

}

