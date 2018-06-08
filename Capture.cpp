/* -LICENSE-START-
** Copyright (c) 2013 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
**
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>

#include <mpi.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <vector>
#include <math.h>
#include <string.h>

#include "DeckLinkAPI.h"
#include "Capture.h"
#include "Config.h"
#include "Display.h"

using namespace std;
using namespace videocave;

int WIDTH=1920; //1280;
int HEIGHT=1080; //720;
int SCREEN_WIDTH=1366;
int SCREEN_HEIGHT=3072;

// MPI buffer
char buff[1024], buff_r[1024];  
int numprocs;  
MPI_Status stat; 
unsigned char* buff_data = NULL; // to store frame data
int buff_data_length;
unsigned char *rgbImageData = NULL;
bool first_frame = true;
static BMDTimeValue startoftime;
BMDTimeValue frameRateDuration, frameRateScale;

static pthread_mutex_t	frame_finished_mutex_t;
static bool frame_finished = true;


static pthread_mutex_t	g_sleepMutex;
static pthread_cond_t	g_sleepCond;
static int				g_videoOutputFile = -1;
static int				g_audioOutputFile = -1;
static bool				g_do_exit = false;

static BMDConfig		g_config;

static IDeckLinkInput*	g_deckLinkInput = NULL;

static unsigned long	g_frameCount = 0;


unsigned int startTime;
uint getTime()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (tp.tv_sec * 1000 + tp.tv_usec / 1000);
}

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() : m_refCount(1)
{
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
	return __sync_add_and_fetch(&m_refCount, 1);
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
	int32_t newRefValue = __sync_sub_and_fetch(&m_refCount, 1);
	if (newRefValue == 0)
	{
		delete this;
		return 0;
	}
	return newRefValue;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
	IDeckLinkVideoFrame*				rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions*	threeDExtensions = NULL;
	void*								frameBytes;
	void*								audioFrameBytes;

	// Handle Video Frame
	//if (videoFrame && frame_finished)
	if (videoFrame)
	{
		//pthread_mutex_lock(&frame_finished_mutex_t);
		//frame_finished = false;
		//pthread_mutex_unlock(&frame_finished_mutex_t);

		// If 3D mode is enabled we retreive the 3D extensions interface which gives.
		// us access to the right eye frame by calling GetFrameForRightEye() .
		if ( (videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) != S_OK) ||
			(threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK))
		{
			rightEyeFrame = NULL;
		}

		if (threeDExtensions)
			threeDExtensions->Release();

		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
		{
			printf("Frame received (#%lu) - No input signal detected\n", g_frameCount);
		}
		else
		{
			const char *timecodeString = NULL;
			if (g_config.m_timecodeFormat != 0)
			{
				IDeckLinkTimecode *timecode;
				if (videoFrame->GetTimecode(g_config.m_timecodeFormat, &timecode) == S_OK)
				{
					timecode->GetString(&timecodeString);
				}
			}
			if (timecodeString)
				free((void*)timecodeString);

			if(first_frame) {
				BMDTimeValue hframedur;
				HRESULT h = videoFrame->GetHardwareReferenceTimestamp(frameRateScale, &startoftime, &hframedur);
				assert(h == S_OK);
				//BMDTimeValue hframedur;
				//videoFrame->GetStreamTime(&startoftime, &hframedur, frameRateScale);

				startTime = getTime();
			}

			//BMDTimeValue frameTime, frameDuration;
			//videoFrame->GetStreamTime(&frameTime, &frameDuration, frameRateScale);
			//cout << frameTime << " " << frameDuration << endl;
			//bool skip = false;

			BMDTimeScale htimeScale = frameRateScale;
			BMDTimeValue hframeTime;
			BMDTimeValue hframeDuration;
			bool skip = false;
			HRESULT h = videoFrame->GetHardwareReferenceTimestamp(htimeScale, &hframeTime, &hframeDuration);
			if (h == S_OK)  {
				//cout << "startoftime " << startoftime << " ";
				double frametime = (double)(hframeTime-startoftime) / (double)hframeDuration;
				//cout << "hframeDuration: " << hframeDuration <<  " hframeTime: " << hframeTime << flush << endl;
				if ( ((int)frametime) > g_frameCount) { skip = true; }

				//cout << "frametime: " << frametime << " g_frameCount:" << g_frameCount << flush << endl;

				unsigned int timePass = getTime() - startTime;
				if(timePass > g_frameCount * (hframeDuration/30.0))
					skip = 1;
				//cout << "timePass: " << timePass << " time:" << g_frameCount * (hframeDuration/30.0) << flush << endl;
			}

			if(skip)
				cout << "#" << flush;
			
			if(g_frameCount % 30 == 0) {
				/*
				printf("Frame received (#%lu) [%s] - %s - Size: %li bytes\n",
					g_frameCount,
					timecodeString != NULL ? timecodeString : "No timecode",
					rightEyeFrame != NULL ? "Valid Frame (3D left/right)" : "Valid Frame",
					videoFrame->GetRowBytes() * videoFrame->GetHeight());
				*/
				cout << "." << flush;
			}

			

			// process frame data
			if(first_frame) {
				strcpy(buff, "length");
				for(int i=1;i<numprocs;i++)
					MPI_Send(buff, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD);

				for(int i=1;i<numprocs;i++)  
					MPI_Recv(buff_r, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);

				int length = videoFrame->GetRowBytes() * videoFrame->GetHeight();
				for(int i=1;i<numprocs;i++)
					MPI_Send(&length, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

				for(int i=1;i<numprocs;i++)  
					MPI_Recv(buff_r, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);

				first_frame = false;
			}

			if(!skip) {
				strcpy(buff, "update");
				for(int i=1;i<numprocs;i++)
					MPI_Send(buff, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD);

				for(int i=1;i<numprocs;i++)  
					MPI_Recv(buff_r, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);
				
				videoFrame->GetBytes(&frameBytes);
				assert(frameBytes);

				for(int i=1;i<numprocs;i++)
					MPI_Send((char*)frameBytes, videoFrame->GetRowBytes() * videoFrame->GetHeight(), MPI_CHAR, i, 0, MPI_COMM_WORLD);

				for(int i=1;i<numprocs;i++)  
					MPI_Recv(buff_r, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);
			}
			

			//cout << "End of frame " << g_frameCount << flush << endl;
		}

		if (rightEyeFrame)
			rightEyeFrame->Release();

		g_frameCount++;

		//pthread_mutex_lock(&frame_finished_mutex_t);
		//frame_finished = true;
		//pthread_mutex_unlock(&frame_finished_mutex_t);
	}

	if (g_config.m_maxFrames > 0 && videoFrame && g_frameCount >= g_config.m_maxFrames)
	{
		g_do_exit = true;
		pthread_cond_signal(&g_sleepCond);
	}

	return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags formatFlags)
{
	// This only gets called if bmdVideoInputEnableFormatDetection was set
	// when enabling video input
	HRESULT	result;
	char*	displayModeName = NULL;
	BMDPixelFormat	pixelFormat = bmdFormat10BitYUV;

	if (formatFlags & bmdDetectedVideoInputRGB444)
		pixelFormat = bmdFormat10BitRGB;

	mode->GetName((const char**)&displayModeName);
	printf("Video format changed to %s %s\n", displayModeName, formatFlags & bmdDetectedVideoInputRGB444 ? "RGB" : "YUV");

	if (displayModeName)
		free(displayModeName);

	if (g_deckLinkInput)
	{
		g_deckLinkInput->StopStreams();

		result = g_deckLinkInput->EnableVideoInput(mode->GetDisplayMode(), pixelFormat, g_config.m_inputFlags);
		if (result != S_OK)
		{
			fprintf(stderr, "Failed to switch video mode\n");
			goto bail;
		}

		g_deckLinkInput->StartStreams();
	}

bail:
	return S_OK;
}

static void sigfunc(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
		g_do_exit = true;

	pthread_cond_signal(&g_sleepCond);
}

int runCapture(int argc, char *argv[])
{
	HRESULT							result;
	int								exitStatus = 1;
	int								idx;

	IDeckLinkIterator*				deckLinkIterator = NULL;
	IDeckLink*						deckLink = NULL;

	IDeckLinkAttributes*			deckLinkAttributes = NULL;
	bool							formatDetectionSupported;

	IDeckLinkDisplayModeIterator*	displayModeIterator = NULL;
	IDeckLinkDisplayMode*			displayMode = NULL;
	char*							displayModeName = NULL;
	BMDDisplayModeSupport			displayModeSupported;

	DeckLinkCaptureDelegate*		delegate = NULL;

	pthread_mutex_init(&g_sleepMutex, NULL);
	pthread_cond_init(&g_sleepCond, NULL);

	pthread_mutex_init(&frame_finished_mutex_t, NULL);

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

	// Process the command line arguments
	if (!g_config.ParseArguments(argc, argv))
	{
		g_config.DisplayUsage(exitStatus);
		goto bail;
	}

	// Get the DeckLink device
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (!deckLinkIterator)
	{
		fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
		goto bail;
	}

	idx = g_config.m_deckLinkIndex;

	while ((result = deckLinkIterator->Next(&deckLink)) == S_OK)
	{
		if (idx == 0)
			break;
		--idx;

		deckLink->Release();
	}

	if (result != S_OK || deckLink == NULL)
	{
		fprintf(stderr, "Unable to get DeckLink device %u\n", g_config.m_deckLinkIndex);
		goto bail;
	}

	// Get the input (capture) interface of the DeckLink device
	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&g_deckLinkInput);
	if (result != S_OK)
		goto bail;

	// Get the display mode
	if (g_config.m_displayModeIndex == -1)
	{
		// Check the card supports format detection
		result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
		if (result == S_OK)
		{
			result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &formatDetectionSupported);
			if (result != S_OK || !formatDetectionSupported)
			{
				fprintf(stderr, "Format detection is not supported on this device\n");
				goto bail;
			}
		}

		g_config.m_inputFlags |= bmdVideoInputEnableFormatDetection;

		// Format detection still needs a valid mode to start with
		idx = 0;
	}
	else
	{
		idx = g_config.m_displayModeIndex;
	}

	result = g_deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK)
		goto bail;

	while ((result = displayModeIterator->Next(&displayMode)) == S_OK)
	{
		if (idx == 0)
			break;
		--idx;

		displayMode->Release();
	}

	if (result != S_OK || displayMode == NULL)
	{
		fprintf(stderr, "Unable to get display mode %d\n", g_config.m_displayModeIndex);
		goto bail;
	}

	// Get display mode name
	result = displayMode->GetName((const char**)&displayModeName);
	if (result != S_OK)
	{
		displayModeName = (char *)malloc(32);
		snprintf(displayModeName, 32, "[index %d]", g_config.m_displayModeIndex);
	}

	// Check display mode is supported with given options
	result = g_deckLinkInput->DoesSupportVideoMode(displayMode->GetDisplayMode(), g_config.m_pixelFormat, bmdVideoInputFlagDefault, &displayModeSupported, NULL);
	if (result != S_OK)
		goto bail;

	if (displayModeSupported == bmdDisplayModeNotSupported)
	{
		fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
		goto bail;
	}

	if (g_config.m_inputFlags & bmdVideoInputDualStream3D)
	{
		if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D))
		{
			fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
			goto bail;
		}
	}

	displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);

	// Print the selected configuration
	g_config.DisplayConfiguration();

	// Configure the capture callback
	delegate = new DeckLinkCaptureDelegate();
	g_deckLinkInput->SetCallback(delegate);

	// Open output files
	if (g_config.m_videoOutputFile != NULL)
	{
		g_videoOutputFile = open(g_config.m_videoOutputFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (g_videoOutputFile < 0)
		{
			fprintf(stderr, "Could not open video output file \"%s\"\n", g_config.m_videoOutputFile);
			goto bail;
		}
	}

	if (g_config.m_audioOutputFile != NULL)
	{
		g_audioOutputFile = open(g_config.m_audioOutputFile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (g_audioOutputFile < 0)
		{
			fprintf(stderr, "Could not open audio output file \"%s\"\n", g_config.m_audioOutputFile);
			goto bail;
		}
	}

	// Block main thread until signal occurs
	while (!g_do_exit)
	{
		// Start capturing
		result = g_deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(), g_config.m_pixelFormat, g_config.m_inputFlags);
		if (result != S_OK)
		{
			fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
			goto bail;
		}

		result = g_deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_config.m_audioSampleDepth, g_config.m_audioChannels);
		if (result != S_OK)
			goto bail;

		result = g_deckLinkInput->StartStreams();
		if (result != S_OK)
			goto bail;

		// All Okay.
		exitStatus = 0;

		pthread_mutex_lock(&g_sleepMutex);
		pthread_cond_wait(&g_sleepCond, &g_sleepMutex);
		pthread_mutex_unlock(&g_sleepMutex);

		fprintf(stderr, "Stopping Capture\n");
		g_deckLinkInput->StopStreams();
		g_deckLinkInput->DisableAudioInput();
		g_deckLinkInput->DisableVideoInput();
	}

bail:
	if (g_videoOutputFile != 0)
		close(g_videoOutputFile);

	if (g_audioOutputFile != 0)
		close(g_audioOutputFile);

	if (displayModeName != NULL)
		free(displayModeName);

	if (displayMode != NULL)
		displayMode->Release();

	if (displayModeIterator != NULL)
		displayModeIterator->Release();

	if (delegate != NULL)
		delegate->Release();

	if (g_deckLinkInput != NULL)
	{
		g_deckLinkInput->Release();
		g_deckLinkInput = NULL;
	}

	if (deckLinkAttributes != NULL)
		deckLinkAttributes->Release();

	if (deckLink != NULL)
		deckLink->Release();

	if (deckLinkIterator != NULL)
		deckLinkIterator->Release();

	return exitStatus;
}


void YUV422UYVY_ToRGB24(unsigned char* pbYUV, unsigned char* pbRGB, int yuv_len)
{
	// yuv_len: number of bytes
	int B_Cb128,R_Cr128,G_CrCb128;
    int Y0,U,Y1,V;
    int R,G,B;

	unsigned char *yuv_index;
	unsigned char *rgb_index;

	yuv_index = pbYUV;
	rgb_index = pbRGB;

    for (int i = 0, j = 0; i < yuv_len; )
    {
		U  = (int)((float)yuv_index[i++]-128.0f);
		Y0 = (int)(1.164f * ((float)pbYUV[i++]-16.0f));
		V  = (int)((float)yuv_index[i++]-128.0f);
		Y1 = (int)(1.164f * ((float)yuv_index[i++]-16.0f));

		R_Cr128   =  (int)(1.596f*V);
		G_CrCb128 = (int)(-0.813f*V - 0.391f*U);
		B_Cb128   =  (int)(2.018f*U);

		R= Y0 + R_Cr128;
		G = Y0 + G_CrCb128;
		B = Y0 + B_Cb128;

		rgb_index[j++] = max(0, min(R, 255));
		rgb_index[j++] = max(0, min(G, 255));
		rgb_index[j++] = max(0, min(B, 255));

		R= Y1 + R_Cr128;
		G = Y1 + G_CrCb128;
		B = Y1 + B_Cb128;

		rgb_index[j++] = max(0, min(R, 255));
		rgb_index[j++] = max(0, min(G, 255));
		rgb_index[j++] = max(0, min(B, 255));
	}
}


int main( int argc, char* argv[] ){

    char idstr[1024]; 
	char processor_name[MPI_MAX_PROCESSOR_NAME];  
	int myid; int i; int namelen;  
	MPI_Status stat;  

	MPI_Init(&argc,&argv);  
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);  
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);  
	MPI_Get_processor_name(processor_name, &namelen);  

    if(myid == 0) {  // server
		for(i=1;i<numprocs;i++)  {  
      		MPI_Recv(buff, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);
      		printf("Client %d: %s\n", i, buff); 
      		if(strcmp(buff, "ready") != 0) {
				cout << "node: " << i << " failed!!" << endl;
				return 1;
			}
    	} 

		runCapture(argc, argv);
	}
	else {
		cout << "(" << myid << ") numdisplay: " << numprocs-1 << endl;
        Display* display = new Display(myid, WIDTH, HEIGHT, numprocs-1, false);
        int ret = display->initWindow(SCREEN_WIDTH, SCREEN_HEIGHT);
        display->setup();

		strcpy(buff, "ready");
		MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

		bool running = true;
		while(running) {
    		MPI_Recv(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &stat); 
    		//cout << "(" << myid << ") Rev: " << buff << flush << endl;
			if (strcmp(buff, "stop") == 0) {
				running = false;
			}
			else if (strcmp(buff, "length") == 0) {
				strcpy(buff, "ok");
		        MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

				MPI_Recv(&buff_data_length, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
				cout << "(" << myid << ") data length: " << buff_data_length << endl;
				buff_data = new unsigned char[buff_data_length];
				rgbImageData = new unsigned char [WIDTH*HEIGHT*3];
				
				strcpy(buff, "ok");
		        MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
			else if (strcmp(buff, "update") == 0) {
				strcpy(buff, "ok");
		        MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

				// now receive buffer and update display
				MPI_Recv(buff_data, buff_data_length, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &stat);
				YUV422UYVY_ToRGB24((unsigned char*)buff_data, rgbImageData, buff_data_length);
				display->update(rgbImageData);
				display->render();
				
				strcpy(buff, "ok");
		        MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
        }

		if (buff_data)
			delete  []buff_data;

		if (rgbImageData)
			delete []rgbImageData;
	}

	return 0;
}