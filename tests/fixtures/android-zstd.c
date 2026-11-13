// SPDX-License-Identifier: GPL-3.0-or-later
// Diagnostic only: takes a user-installed library exposing the public Zstd API.
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {const void *src;size_t size,pos;} In;
typedef struct {void *dst;size_t size,pos;} Out;
static void *(*create_ctx)(void);static size_t(*free_ctx)(void*);static size_t(*param)(void*,int,int);
static size_t(*stream)(void*,Out*,In*,int);static unsigned(*is_error)(size_t);static const char*(*error_name)(size_t);
static int failures;static const unsigned char *input;
static void *job(void *arg) {
 int workers=(int)(intptr_t)arg;void *ctx=create_ctx();if(!ctx){printf("create ctx failed\n");__atomic_add_fetch(&failures,1,__ATOMIC_RELAXED);return NULL;}
 size_t r=param(ctx,100,3);if(is_error(r))printf("level %s\n",error_name(r));
 if(workers) {r=param(ctx,400,workers);if(is_error(r))printf("workers %d: %s\n",workers,error_name(r));}
 unsigned char *out=malloc(65536);In in={input,8<<20,0};size_t total=0;
 for(size_t loops=0;loops<10000;loops++) {Out o={out,65536,0};r=stream(ctx,&o,&in,2);total+=o.pos;
  if(is_error(r)){printf("workers=%d error=%s value=%zd in=%zu out=%zu total=%zu\n",workers,error_name(r),r,in.pos,o.pos,total);__atomic_add_fetch(&failures,1,__ATOMIC_RELAXED);break;}
  if(!r){printf("workers=%d compressed=%zu input=%zu\n",workers,total,in.pos);break;}
 }
 free(out);free_ctx(ctx);return NULL;
}
int main(int argc,char **argv) {
 setvbuf(stdout,NULL,_IONBF,0);printf("zstd-probe entered argc=%d\n",argc);
 if(argc!=2)return 2;void *lib=dlopen(argv[1],RTLD_NOW);if(!lib){printf("dlopen: %s\n",dlerror());return 1;}
 printf("library loaded\n");
 #define LOAD(var,name) do{*(void**)(&var)=dlsym(lib,name);if(!var){printf("missing %s\n",name);return 1;}}while(0)
 LOAD(create_ctx,"ZSTD_createCCtx");LOAD(free_ctx,"ZSTD_freeCCtx");LOAD(param,"ZSTD_CCtx_setParameter");LOAD(stream,"ZSTD_compressStream2");LOAD(is_error,"ZSTD_isError");LOAD(error_name,"ZSTD_getErrorName");
 unsigned char *data=malloc(8<<20);uint32_t seed=1;for(size_t i=0;i<(8<<20);i++){seed=seed*1664525+1013904223;data[i]=seed>>24;}input=data;
 job(0);job((void*)12);pthread_t t[12];int count=0;
 for(int i=0;i<12;i++){int r=pthread_create(&t[count],NULL,job,0);if(r){printf("pthread_create=%d\n",r);failures++;}else count++;}
 for(int i=0;i<count;i++)pthread_join(t[i],NULL);free(data);_Exit(failures?1:0);
}
