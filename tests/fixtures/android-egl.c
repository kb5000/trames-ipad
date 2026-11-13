// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <android/hardware_buffer.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
static void fault(int signal,siginfo_t *info,void *context){
    ucontext_t *u=context;Dl_info module={0},caller={0};dladdr((void*)u->uc_mcontext.pc,&module);dladdr((void*)u->uc_mcontext.regs[30],&caller);
    dprintf(2,"symbol=%s caller=%s+%#llx\n",module.dli_sname?module.dli_sname:"?",caller.dli_fname?caller.dli_fname:"?",(unsigned long long)(u->uc_mcontext.regs[30]-(uintptr_t)caller.dli_fbase));
    dprintf(2,"EGL fault: signal=%d address=%p pc=%#llx module=%s offset=%#llx x0=%#llx x1=%#llx x2=%#llx\n",signal,info->si_addr,(unsigned long long)u->uc_mcontext.pc,module.dli_fname?module.dli_fname:"?",(unsigned long long)(u->uc_mcontext.pc-(uintptr_t)module.dli_fbase),(unsigned long long)u->uc_mcontext.regs[0],(unsigned long long)u->uc_mcontext.regs[1],(unsigned long long)u->uc_mcontext.regs[2]);_exit(128+signal);
}
static void check(int value,const char *name){if(!value){fprintf(stderr,"%s EGL=%#x GL=%#x\n",name,eglGetError(),glGetError());exit(1);}}
int main(void){
    setbuf(stdout,NULL);struct sigaction action={.sa_sigaction=fault,.sa_flags=SA_SIGINFO};sigemptyset(&action.sa_mask);sigaction(SIGSEGV,&action,NULL);sigaction(SIGTRAP,&action,NULL);
    printf("HAL diagnostic environment: %s\n",getenv("TRAMES_ANDROID_TRACE"));
    void *vulkan=dlopen("libvulkan.so",RTLD_NOW|RTLD_LOCAL);check(vulkan!=NULL,"load original Vulkan");
    PFN_vkEnumerateInstanceExtensionProperties enumerate=dlsym(vulkan,"vkEnumerateInstanceExtensionProperties");
    Dl_info vi={0};dladdr((void*)enumerate,&vi);printf("Vulkan enum %p %s+%#llx\n",(void*)enumerate,vi.dli_fname,(unsigned long long)((uintptr_t)enumerate-(uintptr_t)vi.dli_fbase));
    uint32_t extensions=0;printf("Vulkan enumerate result %d\n",enumerate(NULL,&extensions,NULL));printf("Vulkan extensions %u\n",extensions);
    puts("eglGetDisplay");EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);check(display!=EGL_NO_DISPLAY,"get display");
    puts("eglInitialize");EGLint major,minor;check(eglInitialize(display,&major,&minor),"initialize");printf("EGL %d.%d: %s\n",major,minor,eglQueryString(display,EGL_VENDOR));
    EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig config;EGLint count;check(eglChooseConfig(display,attributes,&config,1,&count)&&count,"choose config");
    EGLint pbuffer[]={EGL_WIDTH,32,EGL_HEIGHT,32,EGL_NONE};EGLSurface surface=eglCreatePbufferSurface(display,config,pbuffer);check(surface!=EGL_NO_SURFACE,"pbuffer");
    EGLint version[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};EGLContext context=eglCreateContext(display,config,EGL_NO_CONTEXT,version);check(context!=EGL_NO_CONTEXT,"context");check(eglMakeCurrent(display,surface,surface,context),"make current");
    printf("GL renderer: %s\n",glGetString(GL_RENDERER));glClearColor(0.25f,0.5f,0.75f,1.f);glClear(GL_COLOR_BUFFER_BIT);unsigned char pixels[4];glReadPixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels);check(glGetError()==GL_NO_ERROR,"read pixels");check(abs((int)pixels[0]-64)<=1&&abs((int)pixels[1]-128)<=1&&abs((int)pixels[2]-191)<=1&&pixels[3]==255,"GPU pixel result");
    PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC clientBuffer=(void*)eglGetProcAddress("eglGetNativeClientBufferANDROID");
    PFNEGLCREATEIMAGEKHRPROC createImage=(void*)eglGetProcAddress("eglCreateImageKHR");
    PFNEGLDESTROYIMAGEKHRPROC destroyImage=(void*)eglGetProcAddress("eglDestroyImageKHR");
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTarget=(void*)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    check(clientBuffer && createImage && destroyImage && imageTarget,"AHB EGL functions");
    AHardwareBuffer_Desc desc={.width=65,.height=37,.layers=1,.format=AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,.usage=AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT|AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE|AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN};
    AHardwareBuffer *buffer=NULL;check(!AHardwareBuffer_allocate(&desc,&buffer),"original AHardwareBuffer allocation");
    EGLint imageAttrs[]={EGL_IMAGE_PRESERVED_KHR,EGL_TRUE,EGL_NONE};
    EGLImageKHR image=createImage(display,EGL_NO_CONTEXT,EGL_NATIVE_BUFFER_ANDROID,clientBuffer(buffer),imageAttrs);check(image!=EGL_NO_IMAGE_KHR,"native EGL image");
    GLuint texture,frame;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);imageTarget(GL_TEXTURE_2D,image);check(glGetError()==GL_NO_ERROR,"native EGL texture");
    glGenFramebuffers(1,&frame);glBindFramebuffer(GL_FRAMEBUFFER,frame);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"native framebuffer");
    const char *vertexSource="attribute vec2 position;void main(){gl_Position=vec4(position,0.,1.);}";
    const char *fragmentSource="precision mediump float;void main(){gl_FragColor=vec4(.25,.5,.75,1.);}";
    GLuint vertex=glCreateShader(GL_VERTEX_SHADER),fragment=glCreateShader(GL_FRAGMENT_SHADER),program=glCreateProgram();
    glShaderSource(vertex,1,&vertexSource,NULL);glCompileShader(vertex);glShaderSource(fragment,1,&fragmentSource,NULL);glCompileShader(fragment);
    glAttachShader(program,vertex);glAttachShader(program,fragment);glBindAttribLocation(program,0,"position");glLinkProgram(program);GLint linked=0;glGetProgramiv(program,GL_LINK_STATUS,&linked);check(linked,"native draw shader");glUseProgram(program);
    const GLfloat triangle[]={-1,-1,3,-1,-1,3};GLuint vertices;glGenBuffers(1,&vertices);glBindBuffer(GL_ARRAY_BUFFER,vertices);glBufferData(GL_ARRAY_BUFFER,sizeof(triangle),triangle,GL_STATIC_DRAW);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);glEnableVertexAttribArray(0);glViewport(0,0,65,37);glDrawArrays(GL_TRIANGLES,0,3);glFinish();check(glGetError()==GL_NO_ERROR,"native draw completion");
    void *nativePixels=NULL;check(!AHardwareBuffer_lock(buffer,AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,-1,NULL,&nativePixels),"native CPU consumer");
    unsigned char *color=nativePixels;printf("AHB CPU pixel=%u,%u,%u,%u\n",color[0],color[1],color[2],color[3]);check(abs((int)color[0]-64)<=1&&abs((int)color[1]-128)<=1&&abs((int)color[2]-191)<=1&&color[3]==255,"AHB native GPU storage pixels");
    check(!AHardwareBuffer_unlock(buffer,NULL),"native CPU release");glDeleteBuffers(1,&vertices);glDeleteProgram(program);glDeleteShader(vertex);glDeleteShader(fragment);glDeleteFramebuffers(1,&frame);glDeleteTextures(1,&texture);destroyImage(display,image);AHardwareBuffer_release(buffer);
    puts("android-egl: PASS original AHardwareBuffer/EGL native image GPU-to-CPU pixels");
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);puts("android-egl: PASS original EGL/ANGLE pbuffer pixels");return 0;
}
