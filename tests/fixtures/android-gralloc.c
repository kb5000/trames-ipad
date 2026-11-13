// SPDX-License-Identifier: GPL-3.0-or-later
#include <hardware/gralloc.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
static void check(int okay,const char *step){if(!okay){fprintf(stderr,"android-gralloc: FAIL %s\n",step);exit(1);}}
int main(void){
    setbuf(stdout,NULL);
    int apex=open("/apex/com.android.art/javalib/core-oj.jar",O_RDONLY|O_CLOEXEC);if(apex>=0){char path[80],target[512];snprintf(path,sizeof(path),"/proc/self/fd/%d",apex);ssize_t n=readlink(path,target,sizeof(target)-1);if(n>=0){target[n]=0;printf("APEX FD path: %s\n",target);}else perror("APEX readlink");close(apex);}
    void *support=dlopen("libvndksupport.so",RTLD_NOW|RTLD_LOCAL);check(support!=NULL,"VNDK support");
    void *(*load)(const char*,int)=dlsym(support,"android_load_sphal_library");check(load!=NULL,"SPHAL loader");
    void *library=load("gralloc.trames.so",RTLD_NOW|RTLD_LOCAL);check(library!=NULL,"original SPHAL load");
    gralloc_module_t *module=dlsym(library,HAL_MODULE_INFO_SYM_AS_STR);check(module!=NULL,"HAL module");
    alloc_device_t *device=NULL;check(!module->common.methods->open(&module->common,GRALLOC_HARDWARE_GPU0,(hw_device_t**)&device),"HAL open");
    buffer_handle_t handle=NULL;int stride=0;
    check(!device->alloc(device,65,37,HAL_PIXEL_FORMAT_RGBA_8888,GRALLOC_USAGE_SW_WRITE_OFTEN|GRALLOC_USAGE_HW_RENDER,&handle,&stride),"native allocation");
    printf("native image 65x37 stride=%d fd=%d\n",stride,handle->data[0]);
    unsigned char *pixels=NULL;check(!module->lock(module,handle,GRALLOC_USAGE_SW_WRITE_OFTEN,0,0,65,37,(void**)&pixels),"canonical CPU mapping");
    for(int y=0;y<37;y++)for(int x=0;x<65;x++){unsigned char *p=pixels+(y*stride+x)*4;p[0]=x;p[1]=y;p[2]=123;p[3]=255;}
    check(!module->unlock(module,handle),"producer unlock");
    size_t size=sizeof(native_handle_t)+(handle->numFds+handle->numInts)*sizeof(int);native_handle_t *copy=malloc(size);check(copy!=NULL,"handle clone");memcpy(copy,handle,size);copy->data[0]=fcntl(handle->data[0],F_DUPFD_CLOEXEC,0);check(copy->data[0]>=0,"retained FD alias");
    check(!module->registerBuffer(module,copy),"import handle");
    check(!device->free(device,handle),"release original handle");
    check(!module->lock(module,copy,GRALLOC_USAGE_SW_READ_OFTEN,0,0,65,37,(void**)&pixels),"import mapping after original close");
    for(int y=0;y<37;y++)for(int x=0;x<65;x++){unsigned char *p=pixels+(y*stride+x)*4;check(p[0]==x&&p[1]==y&&p[2]==123&&p[3]==255,"shared native pixels");}
    check(!module->unlock(module,copy),"consumer unlock");
    check(module->lockAsync&&module->unlockAsync&&module->lockAsync_ycbcr,"advertised Gralloc 0.3 entrypoints");
    int fence=eventfd(1,EFD_CLOEXEC);check(fence>=0,"ready acquire fence");
    check(!module->lockAsync(module,copy,GRALLOC_USAGE_SW_READ_OFTEN,0,0,65,37,(void**)&pixels,fence),"async CPU mapping");
    errno=0;check(fcntl(fence,F_GETFD)==-1&&errno==EBADF,"acquire fence consumed");
    check(pixels[2]==123,"async canonical pixels");int release=99;
    check(!module->unlockAsync(module,copy,&release)&&release==-1,"completed CPU release");
    check(!module->unregisterBuffer(module,copy),"unregister");close(copy->data[0]);free(copy);device->common.close(&device->common);
    puts("android-gralloc: PASS original SPHAL/native storage, CPU aliases, async fence ownership and FD lifetime");return 0;
}
