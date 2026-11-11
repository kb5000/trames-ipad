// SPDX-License-Identifier: GPL-3.0-or-later
// Independent original libwayland-client peer: crop, scale, transform and errors.
#define _GNU_SOURCE
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "viewporter-client-protocol.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *shell;
static struct wp_viewporter *viewporter;
static struct wl_surface *surface;
static struct wp_viewport *viewport;
static struct wl_buffer *buffer;
static const char *mode;
static int stage,finished;
static void commit(void);
static void frame_done(void *data,struct wl_callback *callback,uint32_t time){
    (void)data;(void)time;wl_callback_destroy(callback);
    if(++stage==3){finished=1;return;}
    if(stage==1){
        wl_surface_set_buffer_transform(surface,WL_OUTPUT_TRANSFORM_90);
        wp_viewport_set_source(viewport,0,0,wl_fixed_from_int(4),wl_fixed_from_int(8));
        wp_viewport_set_destination(viewport,3,5);
    }else{
        wp_viewport_destroy(viewport);viewport=NULL;
        wl_surface_set_buffer_transform(surface,WL_OUTPUT_TRANSFORM_NORMAL);
        wl_surface_set_buffer_scale(surface,2);
    }
    commit();
}
static const struct wl_callback_listener frame_listener={frame_done};
static void commit(void){
    struct wl_callback *callback=wl_surface_frame(surface);wl_callback_add_listener(callback,&frame_listener,NULL);
    wl_surface_attach(surface,buffer,0,0);wl_surface_damage(surface,0,0,12,8);wl_surface_commit(surface);
}
static void configured(void *data,struct xdg_surface *xdg,uint32_t serial){
    (void)data;xdg_surface_ack_configure(xdg,serial);
    if(!strcmp(mode,"bad-size"))wp_viewport_set_source(viewport,0,0,wl_fixed_from_double(3.5),wl_fixed_from_int(4));
    else{
        wp_viewport_set_source(viewport,wl_fixed_from_int(!strcmp(mode,"out-of-buffer")?7:2),0,wl_fixed_from_int(4),wl_fixed_from_int(4));
        wp_viewport_set_destination(viewport,12,6);
    }
    commit();
}
static const struct xdg_surface_listener xdg_listener={configured};
static void top_configure(void *data,struct xdg_toplevel *top,int32_t w,int32_t h,struct wl_array *states){(void)data;(void)top;(void)w;(void)h;(void)states;}
static void top_close(void *data,struct xdg_toplevel *top){(void)data;(void)top;finished=1;}
static const struct xdg_toplevel_listener top_listener={.configure=top_configure,.close=top_close};
static void ping(void *data,struct xdg_wm_base *base,uint32_t serial){(void)data;xdg_wm_base_pong(base,serial);}
static const struct xdg_wm_base_listener shell_listener={ping};
static void global(void *data,struct wl_registry *registry,uint32_t id,const char *name,uint32_t version){
    (void)data;(void)version;
    if(!strcmp(name,"wl_compositor"))compositor=wl_registry_bind(registry,id,&wl_compositor_interface,4);
    if(!strcmp(name,"wl_shm"))shm=wl_registry_bind(registry,id,&wl_shm_interface,1);
    if(!strcmp(name,"xdg_wm_base")){shell=wl_registry_bind(registry,id,&xdg_wm_base_interface,1);xdg_wm_base_add_listener(shell,&shell_listener,NULL);}
    if(!strcmp(name,"wp_viewporter"))viewporter=wl_registry_bind(registry,id,&wp_viewporter_interface,1);
}
static void global_remove(void *data,struct wl_registry *registry,uint32_t id){(void)data;(void)registry;(void)id;}
static const struct wl_registry_listener registry_listener={global,global_remove};
int main(int argc,char **argv){
    mode=argc>1?argv[1]:"valid";display=wl_display_connect(NULL);assert(display);
    struct wl_registry *registry=wl_display_get_registry(display);wl_registry_add_listener(registry,&registry_listener,NULL);
    assert(wl_display_roundtrip(display)>=0);assert(compositor&&shm&&shell&&viewporter);
    surface=wl_compositor_create_surface(compositor);viewport=wp_viewporter_get_viewport(viewporter,surface);
    if(!strcmp(mode,"duplicate"))(void)wp_viewporter_get_viewport(viewporter,surface);
    else if(!strcmp(mode,"no-surface")){wl_surface_destroy(surface);wp_viewport_set_destination(viewport,2,2);}
    else if(!strcmp(mode,"bad-value"))wp_viewport_set_destination(viewport,0,2);
    else{
        char path[]="/tmp/trames-viewport-XXXXXX";int fd=mkstemp(path);assert(fd>=0);unlink(path);assert(ftruncate(fd,128)==0);
        uint32_t *pixels=mmap(NULL,128,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);assert(pixels!=MAP_FAILED);
        for(int y=0;y<4;y++)for(int x=0;x<8;x++)pixels[y*8+x]=0xff000000u|(uint32_t)(y*8+x);
        struct wl_shm_pool *pool=wl_shm_create_pool(shm,fd,128);buffer=wl_shm_pool_create_buffer(pool,0,8,4,32,WL_SHM_FORMAT_ARGB8888);wl_shm_pool_destroy(pool);munmap(pixels,128);close(fd);
        struct xdg_surface *xdg=xdg_wm_base_get_xdg_surface(shell,surface);xdg_surface_add_listener(xdg,&xdg_listener,NULL);
        struct xdg_toplevel *top=xdg_surface_get_toplevel(xdg);xdg_toplevel_add_listener(top,&top_listener,NULL);xdg_toplevel_set_title(top,"Viewport ABI");wl_surface_commit(surface);
    }
    while(!finished&&wl_display_dispatch(display)>=0){}
    if(strcmp(mode,"valid")){
        const struct wl_interface *interface=NULL;uint32_t id=0,code=wl_display_get_protocol_error(display,&interface,&id);(void)id;
        uint32_t expected=!strcmp(mode,"bad-size")?1:!strcmp(mode,"out-of-buffer")?2:!strcmp(mode,"no-surface")?3:0;
        assert(interface&&code==expected);assert(!strcmp(interface->name,!strcmp(mode,"duplicate")?"wp_viewporter":"wp_viewport"));
        printf("viewport-error-ok %s code=%u\n",mode,code);
    }else{assert(finished&&stage==3);puts("viewport-client-ok crop-transform-reset");}
    wl_display_disconnect(display);return 0;
}
