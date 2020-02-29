#include "driver.h"
#include "osdepend.h"
#include "window.h"
#include "video.h"
#include <pthread.h>
#import <AudioToolbox/AudioQueue.h>
#import <AudioToolbox/AudioServices.h>

#define MAX_AUDIO_BUFFERS 3

typedef struct AQCallbackStruct {
	void* dataPtr;
	AudioQueueRef queue;
	AudioQueueBufferRef buffer[MAX_AUDIO_BUFFERS];
	AudioStreamBasicDescription dataFormat;
    UInt32 frameCount;
} AQCallbackStruct;

#define TAM (1470 * 2 * 2 * 3)
unsigned char ptr_buf[TAM];
unsigned head = 0;
unsigned tail = 0;

pthread_mutex_t mut;

inline int fullQueue(unsigned short size){
    
    if(head < tail)
	{
		return head + size >= tail;
	}
	else if(head > tail)
	{
		return (head + size) >= TAM ? (head + size)- TAM >= tail : false;
	}
	else return false;
}

void enqueue(unsigned char *p,unsigned size){
    unsigned newhead;
    if(head + size < TAM)
    {
        memcpy(ptr_buf+head,p,size);
        newhead = head + size;
    }
    else
    {
        memcpy(ptr_buf+head,p, TAM -head);
        memcpy(ptr_buf,p + (TAM-head), size - (TAM-head));
        newhead = (head + size) - TAM;
    }
    pthread_mutex_lock(&mut);
    
    head = newhead;
    
    pthread_mutex_unlock(&mut);
}

unsigned short dequeue(unsigned char *p,unsigned size){
    
    unsigned real;
    
    if(head == tail)
    {
        memset(p,0,size);//TODO ver si quito para que no petardee
        return size;
    }
    
    pthread_mutex_lock(&mut);
    
    unsigned datasize = head > tail ? head - tail : (TAM - tail) + head ;
    real = datasize > size ? size : datasize;
    
    if(tail + real < TAM)
    {
        memcpy(p,ptr_buf+tail,real);
        tail+=real;
    }
    else
    {
        memcpy(p,ptr_buf + tail, TAM - tail);
        memcpy(p+ (TAM-tail),ptr_buf , real - (TAM-tail));
        tail = (tail + real) - TAM;
    }
    
    pthread_mutex_unlock(&mut);
    
    return real;
}
/**
 * AQBufferCallback()
 */

void AQBufferCallback(void* inAqc, AudioQueueRef inQ, AudioQueueBufferRef outQB) {
	AQCallbackStruct* aqc =  (AQCallbackStruct*) inAqc;

    unsigned char *coreAudioBuffer;
	coreAudioBuffer = (unsigned char*) outQB->mAudioData;

    int res = dequeue(coreAudioBuffer, aqc->dataFormat.mBytesPerFrame * aqc->frameCount);
	outQB->mAudioDataByteSize = res;

	AudioQueueEnqueueBuffer(inQ, outQB, 0, NULL);
}

AQCallbackStruct aqc;

static double				samples_per_frame;
int							attenuation = 0;

int osd_start_audio_stream(int stereo) {
    
    memset(ptr_buf, 0, TAM);
    pthread_mutex_init (&mut,NULL);
    
    if (Machine->sample_rate != 0) {
        memset(&aqc, 0, sizeof(AQCallbackStruct));
	
        aqc.dataFormat.mSampleRate = (Float64) Machine->sample_rate;
        aqc.dataFormat.mFormatID = kAudioFormatLinearPCM;
        aqc.dataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
        aqc.dataFormat.mChannelsPerFrame = (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO) ? 2 : 1;
        aqc.dataFormat.mFramesPerPacket = 1;
        aqc.dataFormat.mBitsPerChannel = 16;
		aqc.dataFormat.mBytesPerPacket = 2 * aqc.dataFormat.mChannelsPerFrame;
		aqc.dataFormat.mBytesPerFrame = 2 * aqc.dataFormat.mChannelsPerFrame;
        aqc.frameCount = (double)Machine->sample_rate / (double)Machine->drv->frames_per_second;
        
        AudioQueueNewOutput(&aqc.dataFormat,
							AQBufferCallback,
							&aqc,
							NULL,
							kCFRunLoopCommonModes,
							0,
							&aqc.queue);
        
        for (int loop = 0; loop < MAX_AUDIO_BUFFERS; loop++) {
            AudioQueueAllocateBuffer(aqc.queue, aqc.frameCount * aqc.dataFormat.mBytesPerFrame, &aqc.buffer[loop]);
            aqc.buffer[loop]->mAudioDataByteSize = aqc.frameCount * aqc.dataFormat.mBytesPerFrame;
			AudioQueueEnqueueBuffer(aqc.queue, aqc.buffer[loop], 0, NULL);
		}
        
		AudioQueueStart(aqc.queue, NULL);
     
        osd_set_mastervolume(attenuation);
    }
    
    samples_per_frame = (double)Machine->sample_rate / (double)Machine->drv->frames_per_second;
    
    return samples_per_frame;
}


void osd_stop_audio_stream(void) {
    AudioQueueStop(aqc.queue, true);
    AudioQueueDispose(aqc.queue, true);
}


int osd_update_audio_stream(INT16* buffer) {
    enqueue(buffer, samples_per_frame * aqc.dataFormat.mBytesPerPacket);
      
	return samples_per_frame;
}

void osd_sound_enable(int x) {
}

//============================================================
//	osd_set_mastervolume
//============================================================

void osd_set_mastervolume(int _attenuation)
{
	// clamp the attenuation to 0-32 range
	if (_attenuation > 0)
		_attenuation = 0;
	if (_attenuation < -32)
		_attenuation = -32;
	attenuation = _attenuation;
    
    
    
    AudioQueueSetParameter(aqc.queue, kAudioQueueParam_Volume, 1.0);
    
	// set the master volume
	//if (stream_buffer && is_enabled)
	//	IDirectSoundBuffer_SetVolume(stream_buffer, attenuation * 100);
}



//============================================================
//	osd_get_mastervolume
//============================================================

int osd_get_mastervolume(void)
{
	return attenuation;
}
