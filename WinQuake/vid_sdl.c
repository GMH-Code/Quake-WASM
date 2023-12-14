/*
SDL2 Video Driver
=================

Updates and SDL2/WASM/32-bit renderer conversions by Gregory Maynard-Hoare
Contains repurposed code Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <SDL2/SDL.h>
#include "quakedef.h"
#ifndef GLQUAKE
#include "d_local.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

unsigned short  d_8to16table[256];

// The original defaults
//#define    BASEWIDTH    320
//#define    BASEHEIGHT   200
// Much better for high resolution displays
#ifdef GLQUAKE
#define    BASEWIDTH    1024
#define    BASEHEIGHT   768
#else
#define    BASEWIDTH    800
#define    BASEHEIGHT   600
#endif

int    VGA_width, VGA_height, VGA_rowbytes, VGA_bufferrowbytes = 0;
byte    *VGA_pagebase;

static SDL_Window *window = NULL;

static qboolean mouse_avail;
static float   mouse_x, mouse_y;
static int mouse_oldbuttonstate = 0;
#ifdef __EMSCRIPTEN__
static qboolean mouse_ready = false;
#endif

// No support for option menus
void (*vid_menudrawfn)(void) = NULL;
void (*vid_menukeyfn)(int key) = NULL;
SDL_Color colors[256];

#ifdef GLQUAKE
viddef_t vid;  // Global video state
static float vid_gamma = 0.6;
unsigned char d_15to8table[65536];
unsigned d_8to24table[256];
int texture_extension_number = 1;
qboolean gl_mtexable = false;
qboolean isPermedia = false;
float gldepthmin, gldepthmax;
const char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;
int texture_mode = GL_LINEAR;
cvar_t gl_ztrick = {"gl_ztrick", "0"};
static SDL_GLContext *gl_context = NULL;
#else
extern viddef_t vid;  // Global video state
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static SDL_PixelFormat *format = NULL;
Uint8 *pixels = NULL;  // Background 8-bit framebuffer
byte force_entire_redraw = 0;
#endif

#ifndef GLQUAKE
void render_rgb(uint32_t* rgb_values, int x, int y, int width, int height)
{
    int xp, yp, offset, loc;
    int x2 = x + width;
    int y2 = y + height;

    for (yp = y; yp < y2; yp++)
    {
        offset = yp * VGA_width;

        for (xp = x; xp < x2; xp++)
        {
            loc = offset + xp;
            SDL_Color col = colors[pixels[loc]];
            rgb_values[loc] = SDL_MapRGB(format, col.r, col.g, col.b);
        }
    }
}
#endif

void    VID_SetPalette (unsigned char *palette)
{
#ifdef GLQUAKE
    byte *pal;
    unsigned r,g,b;
    unsigned v;
    int r1,g1,b1;
    int k;
    unsigned short i;
    unsigned *table;
    int dist, bestdist;

//
// 8 8 8 encoding
//
    pal = palette;
    table = d_8to24table;
    for (i=0 ; i<256 ; i++)
    {
        r = pal[0];
        g = pal[1];
        b = pal[2];
        pal += 3;

        v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
        *table++ = v;
    }
    d_8to24table[255] &= 0xffffff;  // 255 is transparent

    for (i=0; i < (1<<15); i++) {
        /* Maps
        000000000000000
        000000000011111 = Red  = 0x1F
        000001111100000 = Blue = 0x03E0
        111110000000000 = Grn  = 0x7C00
        */
        r = ((i & 0x1F) << 3)+4;
        g = ((i & 0x03E0) >> 2)+4;
        b = ((i & 0x7C00) >> 7)+4;
        pal = (unsigned char *)d_8to24table;
        for (v=0,k=0,bestdist=10000*10000; v<256; v++,pal+=4) {
            r1 = (int)r - (int)pal[0];
            g1 = (int)g - (int)pal[1];
            b1 = (int)b - (int)pal[2];
            dist = (r1*r1)+(g1*g1)+(b1*b1);
            if (dist < bestdist) {
                k=v;
                bestdist = dist;
            }
        }
        d_15to8table[i]=k;
    }
#else
    int i;

    for ( i=0; i<256; ++i )
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
    }

    // Palette changes can affect pixels outside the update regions (such as
    // the status area), so we have to force an update to the entire texture.
    // We can, however, wait for the texture to actually be rendered.
    force_entire_redraw = 1;
#endif
}

void    VID_ShiftPalette (unsigned char *palette)
{
#ifndef GLQUAKE
    VID_SetPalette(palette);
#endif
}

#ifdef GLQUAKE
void GL_Init (void) {
    gl_vendor = glGetString (GL_VENDOR);
    Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
    gl_renderer = glGetString (GL_RENDERER);
    Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

    gl_version = glGetString (GL_VERSION);
    Con_Printf ("GL_VERSION: %s\n", gl_version);
    gl_extensions = glGetString (GL_EXTENSIONS);
    Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

    // Con_Printf ("%s %s\n", gl_renderer, gl_version);

    // CheckMultiTextureExtensions ();

    glClearColor (1,0,0,0);
    glCullFace(GL_FRONT);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.666);

    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel (GL_FLAT);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
    extern cvar_t gl_clear;

    *x = *y = 0;
    *width = vid.width;
    *height = vid.height;
}

void GL_EndRendering (void) {
    glFlush();
    SDL_GL_SwapWindow(window);
    Sbar_Changed();
}

void VID_Init8bitPalette(void)  {
}

static void Check_Gamma (unsigned char *pal) {
    float f, inf;
    unsigned char palette[768];
    int i;

    if ((i = COM_CheckParm("-gamma")) == 0) {
        if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
            (gl_vendor && strstr(gl_vendor, "3Dfx")))
            vid_gamma = 1;
        else
            vid_gamma = 0.6; // default to 0.6 on non-3dfx hardware
    } else
        vid_gamma = Q_atof(com_argv[i+1]);

    for (i=0 ; i<768 ; i++)
    {
        f = pow ( (pal[i]+1)/256.0 , vid_gamma );
        inf = f*255 + 0.5;
        if (inf < 0)
            inf = 0;
        if (inf > 255)
            inf = 255;
        palette[i] = inf;
    }

    memcpy (pal, palette, sizeof(palette));
}

qboolean VID_Is8bit(void)
{
    return false;
}
#endif

void    VID_Init (unsigned char *palette)
{
    int pnum;
    Uint32 flags;
#ifndef GLQUAKE
    int cachesize, chunk;
    byte *cache;
#else
    char gldir[MAX_OSPATH];
#endif

    // Load the SDL library
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0)
        Sys_Error("VID: Couldn't load SDL: %s", SDL_GetError());

    // Set up display mode (width and height)
    vid.width = BASEWIDTH;
    vid.height = BASEHEIGHT;
#ifndef GLQUAKE
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
#endif
    if ((pnum=COM_CheckParm("-winsize")))
    {
        if (pnum >= com_argc-2)
            Sys_Error("VID: -winsize <width> <height>\n");
        vid.width = Q_atoi(com_argv[pnum+1]);
        vid.height = Q_atoi(com_argv[pnum+2]);
        if (!vid.width || !vid.height)
            Sys_Error("VID: Bad window width/height\n");
    }

    // Set video width, height and flags
#ifdef GLQUAKE
    flags = (SDL_WINDOW_OPENGL);
#else
    flags = 0;
#endif

    if ( COM_CheckParm ("-fullscreen") )
        flags |= SDL_WINDOW_FULLSCREEN;

#if defined(__EMSCRIPTEN__) && defined(GLQUAKE)
    initialize_gl4es();
#endif

    window = SDL_CreateWindow(
        "Quake", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, vid.width, vid.height, flags
    );

    if (window == NULL)
        Sys_Error("VID: Couldn't create window: %s\n", SDL_GetError());

#ifndef GLQUAKE
    renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == NULL)
        Sys_Error("VID: Couldn't create renderer: %s\n", SDL_GetError());

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");  // There shouldn't be any non-integer scaling
    SDL_RenderSetLogicalSize(renderer, vid.width, vid.height);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, vid.width, vid.height);

    if (texture == NULL)
        Sys_Error("VID: Couldn't create render texture: %s\n", SDL_GetError());

    VID_SetPalette(palette);
    format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);
    SDL_SetWindowTitle(window, "Quake");
    // now know everything we need to know about the buffer
    VGA_width = vid.conwidth = vid.width;
    VGA_height = vid.conheight = vid.height;

    // Allocate memory for 8-bit background framebuffer
    pixels = malloc(VGA_width * VGA_height);

    if (pixels == NULL)
        Sys_Error("VID: Not enough memory for 8-bit buffer\n");

    VGA_pagebase = vid.buffer = pixels;
    VGA_rowbytes = vid.rowbytes = VGA_width;
    vid.conbuffer = vid.buffer;
    vid.conrowbytes = vid.rowbytes;
    vid.direct = 0;
#endif

    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

#ifdef GLQUAKE
    gl_context = SDL_GL_CreateContext(window);

    if (gl_context == NULL)
        printf("VID: OpenGL context could not be created: %s\n", SDL_GetError());

    Cvar_RegisterVariable (&gl_ztrick);

    GL_Init();

    sprintf (gldir, "%s/glquake", com_gamedir);
    Sys_mkdir (gldir);

    Check_Gamma(palette);
    VID_SetPalette(palette);
#else
    // allocate z buffer and surface cache
    chunk = vid.width * vid.height * sizeof (*d_pzbuffer);
    cachesize = D_SurfaceCacheForRes (vid.width, vid.height);
    chunk += cachesize;
    d_pzbuffer = Hunk_HighAllocName(chunk, "video");

    if (d_pzbuffer == NULL)
        Sys_Error("VID: Not enough memory for video mode\n");

    // initialize the cache memory 
    cache = (byte *) d_pzbuffer
            + vid.width * vid.height * sizeof (*d_pzbuffer);
    D_InitCaches (cache, cachesize);
#endif

    // initialize the mouse
    SDL_ShowCursor(0);
#ifndef __EMSCRIPTEN__
    // In WASM there appears to be a timing issue with this, so if we are using
    // WASM, we will postpone the switch until the first mouse move is detected
    SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
}

void    VID_Shutdown (void)
{
#ifdef GLQUAKE
    if (gl_context != NULL) {
        SDL_GL_DeleteContext(gl_context);
        gl_context = NULL;
    }
#else
    if (pixels != NULL) {
        free(pixels);
        pixels = NULL;
    }

    if (format != NULL) {
        SDL_FreeFormat(format);
        format = NULL;
    }

    if (texture != NULL) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
#endif

    if (window != NULL) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit();
}

#ifndef GLQUAKE
void    VID_Update (vrect_t *rects)
{
    vrect_t *rect;
    void* rgb_pixels;
    int rgb_pitch;

    if (SDL_LockTexture(texture, NULL, &rgb_pixels, &rgb_pitch) < 0)
        Sys_Error("VID: Couldn't lock texture: %s\n", SDL_GetError());

    uint32_t* rbg_pixels_i = (uint32_t*)rgb_pixels;

    if (force_entire_redraw)
    {
        // Render everything
        render_rgb(rbg_pixels_i, 0, 0, VGA_width, VGA_height);
        force_entire_redraw = 0;
    }
    else
    {
        // Render delta regions
        for (rect = rects; rect; rect = rect->pnext)
            render_rgb(rbg_pixels_i, rect->x, rect->y, rect->width, rect->height);
    }

    SDL_UnlockTexture(texture);

    // Running SDL_RenderCopy with the rects here appears to result in the status area being erased
    if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0)
        Sys_Error("VID: Couldn't render texture: %s\n", SDL_GetError());

    SDL_RenderPresent(renderer);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
    Uint8 *offset;


    if (pixels == NULL) return;
    if (x < 0) x = VGA_width + x - 1;
    offset = pixels + y * VGA_width + x;
    while ( height-- )
    {
        memcpy(offset, pbitmap, width);
        offset += VGA_width;
        pbitmap += width;
    }
}

/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
    // Rendering the 'loading' icon here results in slowdown when reading
    // multiple files in sequence, so wait for a screen update before
    // redrawing.
    force_entire_redraw = 1;
}
#endif

/*
================
Sys_SendKeyEvents
================
*/

void Sys_SendKeyEvents(void)
{
    SDL_Event event;
    int sym, state;
     int modstate;

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                sym = event.key.keysym.sym;
                state = event.key.state;
                modstate = SDL_GetModState();
                switch(sym)
                {
                   case SDLK_DELETE: sym = K_DEL; break;
                   case SDLK_BACKSPACE: sym = K_BACKSPACE; break;
                   case SDLK_F1: sym = K_F1; break;
                   case SDLK_F2: sym = K_F2; break;
                   case SDLK_F3: sym = K_F3; break;
                   case SDLK_F4: sym = K_F4; break;
                   case SDLK_F5: sym = K_F5; break;
                   case SDLK_F6: sym = K_F6; break;
                   case SDLK_F7: sym = K_F7; break;
                   case SDLK_F8: sym = K_F8; break;
                   case SDLK_F9: sym = K_F9; break;
                   case SDLK_F10: sym = K_F10; break;
                   case SDLK_F11: sym = K_F11; break;
                   case SDLK_F12: sym = K_F12; break;
                   //case SDLK_BREAK:
                   case SDLK_PAUSE: sym = K_PAUSE; break;
                   case SDLK_UP: sym = K_UPARROW; break;
                   case SDLK_DOWN: sym = K_DOWNARROW; break;
                   case SDLK_RIGHT: sym = K_RIGHTARROW; break;
                   case SDLK_LEFT: sym = K_LEFTARROW; break;
                   case SDLK_INSERT: sym = K_INS; break;
                   case SDLK_HOME: sym = K_HOME; break;
                   case SDLK_END: sym = K_END; break;
                   case SDLK_PAGEUP: sym = K_PGUP; break;
                   case SDLK_PAGEDOWN: sym = K_PGDN; break;
                   case SDLK_RSHIFT:
                   case SDLK_LSHIFT: sym = K_SHIFT; break;
                   case SDLK_RCTRL:
                   case SDLK_LCTRL: sym = K_CTRL; break;
                   case SDLK_RALT:
                   case SDLK_LALT: sym = K_ALT; break;
                   case SDLK_KP_0:
                       if(modstate & KMOD_NUM) sym = K_INS; 
                       else sym = SDLK_0;
                       break;
                   case SDLK_KP_1:
                       if(modstate & KMOD_NUM) sym = K_END;
                       else sym = SDLK_1;
                       break;
                   case SDLK_KP_2:
                       if(modstate & KMOD_NUM) sym = K_DOWNARROW;
                       else sym = SDLK_2;
                       break;
                   case SDLK_KP_3:
                       if(modstate & KMOD_NUM) sym = K_PGDN;
                       else sym = SDLK_3;
                       break;
                   case SDLK_KP_4:
                       if(modstate & KMOD_NUM) sym = K_LEFTARROW;
                       else sym = SDLK_4;
                       break;
                   case SDLK_KP_5: sym = SDLK_5; break;
                   case SDLK_KP_6:
                       if(modstate & KMOD_NUM) sym = K_RIGHTARROW;
                       else sym = SDLK_6;
                       break;
                   case SDLK_KP_7:
                       if(modstate & KMOD_NUM) sym = K_HOME;
                       else sym = SDLK_7;
                       break;
                   case SDLK_KP_8:
                       if(modstate & KMOD_NUM) sym = K_UPARROW;
                       else sym = SDLK_8;
                       break;
                   case SDLK_KP_9:
                       if(modstate & KMOD_NUM) sym = K_PGUP;
                       else sym = SDLK_9;
                       break;
                   case SDLK_KP_PERIOD:
                       if(modstate & KMOD_NUM) sym = K_DEL;
                       else sym = SDLK_PERIOD;
                       break;
                   case SDLK_KP_DIVIDE: sym = SDLK_SLASH; break;
                   case SDLK_KP_MULTIPLY: sym = SDLK_ASTERISK; break;
                   case SDLK_KP_MINUS: sym = SDLK_MINUS; break;
                   case SDLK_KP_PLUS: sym = SDLK_PLUS; break;
                   case SDLK_KP_ENTER: sym = SDLK_RETURN; break;
                   case SDLK_KP_EQUALS: sym = SDLK_EQUALS; break;
                   case SDLK_APPLICATION: sym = 0x60; break;  // 0x60 (tilde) or 0x7E (SDLK_BACKQUOTE) open the Console
                }
                // If we're not directly handled and still above 255
                // just force it to 0
                if(sym > 255) sym = 0;
                Key_Event(sym, state);
                break;

            case SDL_MOUSEMOTION:
#ifdef __EMSCRIPTEN__
                // The pointer does not get warped to the centre of the canvas
                // when captured in WebAssembly -- it is constrained to the
                // bounding box, but relative moves can still travel outside
                if (mouse_ready) {
                    mouse_x = event.motion.xrel*2;
                    mouse_y = event.motion.yrel*2;
                } else {
                    SDL_SetRelativeMouseMode(SDL_TRUE);  // Delayed switch to relative mode
                    mouse_ready = true;  // Drop the first (usually large) movement
                }
#else
                if ( (event.motion.x != (vid.width/2)) ||
                     (event.motion.y != (vid.height/2)) ) {
                    mouse_x = event.motion.xrel*10;
                    mouse_y = event.motion.yrel*10;
                    if ( (event.motion.x < ((vid.width/2)-(vid.width/4))) ||
                         (event.motion.x > ((vid.width/2)+(vid.width/4))) ||
                         (event.motion.y < ((vid.height/2)-(vid.height/4))) ||
                         (event.motion.y > ((vid.height/2)+(vid.height/4))) ) {
                        SDL_WarpMouse(vid.width/2, vid.height/2);
                    }
                }
#endif
                break;

            case SDL_QUIT:
                CL_Disconnect ();
                Host_ShutdownServer(false);        
                Sys_Quit ();
                break;
            default:
                break;
        }
    }
}

void IN_Init (void)
{
    if ( COM_CheckParm ("-nomouse") )
        return;
    mouse_x = mouse_y = 0.0;
    mouse_avail = 1;
}

void IN_Shutdown (void)
{
    mouse_avail = 0;
}

void IN_Commands (void)
{
    int i;
    int mouse_buttonstate;
   
    if (!mouse_avail) return;
   
    i = SDL_GetMouseState(NULL, NULL);
    /* Quake swaps the second and third buttons */
    mouse_buttonstate = (i & ~0x06) | ((i & 0x02)<<1) | ((i & 0x04)>>1);
    for (i=0 ; i<3 ; i++) {
        if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
            Key_Event (K_MOUSE1 + i, true);

        if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
            Key_Event (K_MOUSE1 + i, false);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move (usercmd_t *cmd)
{
    if (!mouse_avail)
        return;
   
    mouse_x *= sensitivity.value;
    mouse_y *= sensitivity.value;
   
    if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
        cmd->sidemove += m_side.value * mouse_x;
    else
        cl.viewangles[YAW] -= m_yaw.value * mouse_x;
    if (in_mlook.state & 1)
        V_StopPitchDrift ();
   
    if ( (in_mlook.state & 1) && !(in_strafe.state & 1)) {
        cl.viewangles[PITCH] += m_pitch.value * mouse_y;
        if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
        if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
    } else {
        if ((in_strafe.state & 1) && noclip_anglehack)
            cmd->upmove -= m_forward.value * mouse_y;
        else
            cmd->forwardmove -= m_forward.value * mouse_y;
    }
    mouse_x = mouse_y = 0.0;
}
