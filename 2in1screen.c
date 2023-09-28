/*
mkdir -p ./bin \
&& gcc -O2 -o ./bin/2in1screen 2in1screen.c \
&& chown root:root ./bin/2in1screen \
&& chmod 0755 ./bin/2in1screen \
&& sudo mv ./bin/2in1screen /usr/bin/
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DATA_SIZE 256
#define N_STATE 4
char basedir[DATA_SIZE];
char *basedir_end = NULL;
char content[DATA_SIZE];
char command[DATA_SIZE*4];

char output[DATA_SIZE] = "DSI1";

char *ROT[] =
	{
		"normal",
		"inverted",
		"right",
		"left"
	};
// 0
// 1
// 2
// 3
char *COOR[]  =
	{
		"1 0 0 0 1 0 0 0 1",
		"-1 0 1 0 -1 1 0 0 1",
		"0 1 0 -1 0 1 0 0 1",
		"0 -1 1 1 0 0 0 0 1"
	};

char *ROT_WACOM[]  =
	{
		"none",
		"half",
		"cw",
		"ccw"
	};

char *SCROLL[]  =
	{
		"4 5 6 7",
		"5 4 7 6",
		"6 7 5 4",
		"7 6 4 5"
	};

// 3
// 2
// 0
// 1

char *COOR_TOUCHPAD[]  =
	{
		"0 -1 1 1 0 0 0 0 1",
		"0 1 0 -1 0 1 0 0 1",
		"1 0 0 0 1 0 0 0 1",
		"-1 0 1 0 -1 1 0 0 1"
	};

//char *SCROLL_TOUCHPAD[]  =
//	{
//		"7 6 4 5"
//		"6 7 5 4",
//		"4 5 6 7",
//		"5 4 7 6",
//	};

double accel_y = 0.0;
//
#if N_STATE == 4
double accel_x = 0.0;
#endif
double accel_gx = 2.0;
double accel_gy = 6.0;

double thrshld = 8.0;
int current_state = 0;

int rotation_changed(){
	int state = 0;

	fprintf(stdout, "x %f ", accel_x);
	fprintf(stdout, "gx %f ", accel_gx);
	fprintf(stdout, "y %f ", accel_y);
	fprintf(stdout, "gy %f ", accel_gy);

	if(-1*thrshld < accel_x && accel_x < thrshld && -1*thrshld < accel_y && accel_y < thrshld)
	{
		// change direction only if i want to
		fprintf(stdout, "not changed\n");
		return 0;
	}

	if(accel_y < -accel_gy)
	{
		state = 0;
	}
	else if(accel_y > accel_gy)
	{
		state = 1;
	}
#if N_STATE == 4
	else if(accel_x > accel_gx)
	{
		state = 2;
	}
	else if(accel_x < -accel_gx)
	{
		state = 3;
	}

	fprintf(stdout, "state %i\n", state);
#endif

	if(current_state!=state)
	{
		current_state = state;
		return 1;
	}
	else return 0;
}

FILE* bdopen(char const *fname, char leave_open){
	*basedir_end = '/';
	strcpy(basedir_end+1, fname);
	FILE *fin = fopen(basedir, "r");
	setvbuf(fin, NULL, _IONBF, 0);
	fgets(content, DATA_SIZE, fin);
	*basedir_end = '\0';

	if(leave_open==0)
	{
		fclose(fin);
		return NULL;
	}
	else return fin;
}

void rotate_screen(){
    // ROTATE DISPLAY
	sprintf(command, "xrandr --output %s --rotate %s", output, ROT[current_state]);
	system(command);

	// ADJUST SCROLL TOUCH SCREEN
	// for libinput and mtrack
    //	sprintf(command, "xinput set-prop \"%s\" --type=int \"Trackpad Scroll Buttons\" %s", "GXTP7380:00 27C6:0113", SCROLL[current_state]);
    //	system(command);

    // ADJUST TRANSFORM MATRICES/ROTATIONS
    // // libinput
	// sprintf(command, "xinput set-prop \"%s\" --type=float \"Coordinate Transformation Matrix\" %s", "GXTP7380:00 27C6:0113 touch", COOR[current_state]);
	// sprintf(command, "xinput set-prop \"%s\" --type=float \"Coordinate Transformation Matrix\" %s", "GXTP7380:00 27C6:0113 Stylus stylus", COOR[current_state]);
	// sprintf(command, "xinput set-prop \"%s\" --type=float \"Coordinate Transformation Matrix\" %s", "GXTP7380:00 27C6:0113 Stylus eraser", COOR[current_state]);
	// libwacom
	sprintf(command, "xsetwacom set \"%s\" \"Rotate\" %s", "GXTP7380:00 27C6:0113 touch", ROT_WACOM[current_state]);
	system(command);
	sprintf(command, "xsetwacom set \"%s\" \"Rotate\" %s", "GXTP7380:00 27C6:0113 Stylus stylus", ROT_WACOM[current_state]);
	system(command);
	sprintf(command, "xsetwacom set \"%s\" \"Rotate\" %s", "GXTP7380:00 27C6:0113 Stylus eraser", ROT_WACOM[current_state]);
	system(command);
	sleep(1.0);  // needed, otherwise the mapping happens too early, resulting in a wrong one
	sprintf(command, "xsetwacom set \"%s\" \"MapToOutput\" %s", "GXTP7380:00 27C6:0113 touch", output);
	system(command);
	sprintf(command, "xsetwacom set \"%s\" \"MapToOutput\" %s", "GXTP7380:00 27C6:0113 Stylus stylus", output);
	system(command);
	sprintf(command, "xsetwacom set \"%s\" \"MapToOutput\" %s", "GXTP7380:00 27C6:0113 Stylus eraser", output);
	system(command);


    // ADJUST TRANSFORM MATRIX TOUCHPAD
    // libinput (only?)
	sprintf(command, "xinput set-prop \"%s\" --type=float \"Coordinate Transformation Matrix\" %s", "HAILUCK CO.,LTD USB KEYBOARD Mouse", COOR_TOUCHPAD[current_state]);
	system(command);
}

int main(int argc, char const *argv[]) {
	FILE *pf = popen("ls /sys/bus/iio/devices/iio:device*/in_accel*", "r");

	if(!pf)
	{
		fprintf(stderr, "IO Error.\n");
		return 2;
	}

	if(fgets(basedir, DATA_SIZE , pf)!=NULL)
	{
		basedir_end = strrchr(basedir, '/');
		if(basedir_end) *basedir_end = '\0';
		fprintf(stderr, "Accelerometer: %s\n", basedir);
	}
	else
	{
		fprintf(stderr, "Unable to find any accelerometer.\n");
		return 1;
	}
	pclose(pf);

	bdopen("in_accel_scale", 0);
	double scale = atof(content);

	FILE *dev_accel_y = bdopen("in_accel_y_raw", 1);
#if N_STATE == 4
	FILE *dev_accel_x = bdopen("in_accel_x_raw", 1);
#endif

	while(1)
	{
		fseek(dev_accel_y, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_y);
		accel_y = atof(content) * scale;
#if N_STATE == 4
		fseek(dev_accel_x, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_x);
		accel_x = atof(content) * scale;
#endif
		if(rotation_changed())
			rotate_screen();
		sleep(2);
	}
	
	return 0;
}
