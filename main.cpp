#include "Display.h"

#include <mpi.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <vector>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
using namespace videocave;

char keyOnce[GLFW_KEY_LAST + 1];
#define glfwGetKeyOnce(WINDOW, KEY)             \
    (glfwGetKey(WINDOW, KEY) ?              \
     (keyOnce[KEY] ? false : (keyOnce[KEY] = true)) :   \
     (keyOnce[KEY] = false))


int main( int argc, char* argv[] ){

    char idstr[1024]; char buff[1024], buff_r[1024];  
	char processor_name[MPI_MAX_PROCESSOR_NAME];  
	int numprocs; int myid; int i; int namelen;  
	MPI_Status stat;  

	MPI_Init(&argc,&argv);  
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs);  
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);  
	MPI_Get_processor_name(processor_name, &namelen);  

    // load some testing image
    int x,y,n;
    string filename = "sample.jpg";
    unsigned char* data = stbi_load(filename.c_str(), &x, &y, &n, 0);
    if(!data) {
        cout << "Failed to load texture " << filename << endl;
        exit(0);
    }
    //cout << "Image info: " << x << " " << y << " " << n << endl;

    if(myid == 0) {  // server
        Display* display = new Display(0, x, y, 1, false);
        int ret = display->initWindow(300, 300);

        for(i=1;i<numprocs;i++)  {  
      		MPI_Recv(buff, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);
      		printf("Client %d: %s\n", i, buff); 
      		if(strcmp(buff, "ready") != 0) {
				cout << "node: " << i << " failed!!" << endl;
				return 1;
			}
    	} 

        bool running = true;
        while(running) {
            glfwPollEvents();
            if( glfwGetKey(display->window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ||
           		glfwWindowShouldClose(display->window) ) {
				running = false;
          		strcpy(buff, "stop");
				for(i=1;i<numprocs;i++)  
					MPI_Send(buff, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD);
				break;
			}

            strcpy(buff, "draw");
            for(i=1;i<numprocs;i++)  
				MPI_Send(buff, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD);
			for(i=1;i<numprocs;i++)  
				MPI_Recv(buff_r, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, &stat);
        }
    }
    else {
        strcpy(buff, "ready");
		MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        Display* display = new Display(myid, x, y, numprocs-1, false);
        int ret = display->initWindow(300, 600);
        display->setup();
        display->update(data);

        bool running = true;
		while(running) {
    		MPI_Recv(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &stat); 
    		//cout << "(" << myid << ") Rev: " << buff << endl;
			if (strcmp(buff, "stop") == 0) {
				running = false;
			}
            else if (strcmp(buff, "draw") == 0) {
                display->render();
                strcpy(buff, "ok");
		        MPI_Send(buff, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }
    }

    stbi_image_free(data);

    return 0;
}