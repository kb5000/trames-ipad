// SPDX-License-Identifier: GPL-3.0-or-later
#define _POSIX_C_SOURCE 200809L
#include <alsa/asoundlib.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double clock_seconds(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9; }
static int record(void) {
    snd_pcm_t *pcm;int error=snd_pcm_open(&pcm,"default",SND_PCM_STREAM_CAPTURE,0);
    if(error<0){fprintf(stderr,"capture open: %s\n",snd_strerror(error));return 77;}
    error=snd_pcm_set_params(pcm,SND_PCM_FORMAT_FLOAT_LE,SND_PCM_ACCESS_RW_INTERLEAVED,1,48000,1,80000);
    if(error<0){fprintf(stderr,"capture params: %s\n",snd_strerror(error));snd_pcm_close(pcm);return 77;}
    double start=clock_seconds(),squares=0,peak=0;unsigned total=0;float samples[480];
    while(total<48000){
        snd_pcm_sframes_t n=snd_pcm_readi(pcm,samples,480);
        if(n<0){fprintf(stderr,"capture read: %s\n",snd_strerror(n));snd_pcm_close(pcm);return 1;}
        for(int i=0;i<n;++i){double x=samples[i];if(!isfinite(x)){snd_pcm_close(pcm);return 1;}squares+=x*x;if(fabs(x)>peak)peak=fabs(x);}
        total+=n;
    }
    printf("{\"capture_frames\":%u,\"elapsed\":%.6f,\"rms\":%.8f,\"peak\":%.8f}\n",total,clock_seconds()-start,sqrt(squares/total),peak);
    snd_pcm_close(pcm);return 0;
}
static void *capture_thread(void *ignored) {(void)ignored;return (void *)(intptr_t)record();}
int main(int argc,char **argv) {
    if(argc>1 && !strcmp(argv[1],"--capture"))return record();
    int clock_test=argc>1 && !strcmp(argv[1],"--clock");
    int duplex=argc>1 && !strcmp(argv[1],"--duplex");pthread_t thread;
    if(duplex && pthread_create(&thread,NULL,capture_thread,NULL))return 1;
    fprintf(stderr,"output opening\n");
    snd_pcm_t *pcm;int error=snd_pcm_open(&pcm,"default",SND_PCM_STREAM_PLAYBACK,0);
    fprintf(stderr,"output open result %d\n",error);
    if(error<0){fprintf(stderr,"output open: %s\n",snd_strerror(error));return 1;}
    error=snd_pcm_set_params(pcm,SND_PCM_FORMAT_FLOAT_LE,SND_PCM_ACCESS_RW_INTERLEAVED,2,48000,1,clock_test?400000:80000);
    if(error<0){fprintf(stderr,"output params: %s\n",snd_strerror(error));snd_pcm_close(pcm);return 1;}
    fprintf(stderr,"output configured\n");
    double start=clock_seconds();unsigned sent=0;float samples[960];
    while(sent<144000){
        for(unsigned i=0;i<480;++i)samples[i*2]=samples[i*2+1]=0.05f*sinf((sent+i)*440.0*6.283185307179586/48000);
        unsigned offset=0;
        while(offset<480){snd_pcm_sframes_t n=snd_pcm_writei(pcm,samples+offset*2,480-offset);if(n<0){fprintf(stderr,"write: %s\n",snd_strerror(n));return 1;}offset+=n;}
        sent+=480;
        if(clock_test && sent==19200){
            snd_pcm_sframes_t previous=-1,maximum=0,delay;double before=clock_seconds(),max_gap=0;
            for(int tick=0;tick<25;++tick){
                struct timespec pause={0,10000000};nanosleep(&pause,NULL);
                if(snd_pcm_delay(pcm,&delay)<0)return 1;
                double now=clock_seconds(),gap=now-before;before=now;
                snd_pcm_sframes_t position=(snd_pcm_sframes_t)sent-delay;
                if(tick>=5 && previous>=0){
                    if(position<previous || position>(snd_pcm_sframes_t)sent)return 1;
                    if(position-previous>maximum)maximum=position-previous;
                    if(gap>max_gap)max_gap=gap;
                }
                previous=position;
            }
            int smooth=maximum<=48000*(max_gap+.005)*1.5;
            printf("{\"clock_max_step_frames\":%ld,\"clock_max_poll_gap\":%.6f,\"continuous_clock\":%s}\n",(long)maximum,max_gap,smooth?"true":"false");
            if(!smooth)return 1;
        }
        if(sent%24000==0)fprintf(stderr,"output submitted %u frames\n",sent);
    }
    error=snd_pcm_drain(pcm);double elapsed=clock_seconds()-start;snd_pcm_close(pcm);
    printf("{\"playback_frames\":%u,\"rate\":48000,\"channels\":2,\"elapsed\":%.6f,\"drain\":%d}\n",sent,elapsed,error);
    if(duplex){void *status;pthread_join(thread,&status);if(status)return (int)(intptr_t)status;}
    return error<0 || elapsed<2.8 || elapsed>5.0;
}
