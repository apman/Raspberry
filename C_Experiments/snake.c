#define _GNU_SOURCE
#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"
#define DEV_FB "/dev"
#define FB_DEV_NAME "fb"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>   // needed for system calls close(), read(), usleep()
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <poll.h>
#include <dirent.h>
#include <string.h>

#include <linux/input.h>


// LED matrix row & col numbering starts in opposite corner of the joy-stick with 0

enum direction_t{
	UP,
	RIGHT,
	DOWN,
	LEFT,
	NONE,
};
struct segment_t {
	struct segment_t *next;
	struct segment_t *prev;
	int x;
	int y;
};

// trailEnd is actually the head of the LL, the path grows by adding to the front
struct segment_t *trailEnd = NULL;    
struct segment_t MsRedRidingHood;    

struct Point_t {
	int x;
	int y;
};

struct fb_t {
	uint16_t pixel[8][8];
};


int running = 1;

struct fb_t *fb;

static int is_event_device(const struct dirent *dir)
{
	return strncmp(EVENT_DEV_NAME, dir->d_name,
		       strlen(EVENT_DEV_NAME)-1) == 0;
}
static int is_framebuffer_device(const struct dirent *dir)
{
	return strncmp(FB_DEV_NAME, dir->d_name,
		       strlen(FB_DEV_NAME)-1) == 0;
}

/* Looks for input devices in /dev/input 
   (not all input is managed in /dev/input/ (e.g. microphones), so I guess sensors aren't either) */
static int open_evdev(const char *dev_name)
{
	struct dirent **namelist;
	int i, ndev;
	int fd = -1;

	ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
	if (ndev <= 0)
		return ndev;

	for (i = 0; i < ndev; i++)
	{
		fprintf(stderr, "\n\nLook for device: %d", ndev);
		char fname[64];
		char name[256];

		snprintf(fname, sizeof(fname),
			 "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);
		fprintf(stderr, "\ndevice file: %s", fname);
		fd = open(fname, O_RDONLY);
		if (fd < 0)
			continue;
		ioctl(fd, EVIOCGNAME(sizeof(name)), name);
		fprintf(stderr, "\ndevice found: %s", name);
		if (strcmp(dev_name, name) == 0)   // if device found is joystick stop looking
		break;
		close(fd);
	}

	for (i = 0; i < ndev; i++)
		free(namelist[i]);

	return fd;
}

static int open_fbdev(const char *dev_name)
{
	struct dirent **namelist;
	int i, ndev;
	int fd = -1;
	struct fb_fix_screeninfo fix_info;

	ndev = scandir(DEV_FB, &namelist, is_framebuffer_device, versionsort);
	if (ndev <= 0)
		return ndev;

	for (i = 0; i < ndev; i++)
	{
		fprintf(stderr, "\n\nLook for fb device: %d", ndev);
		char fname[64];
		char name[256];

		snprintf(fname, sizeof(fname),
			 "%s/%s", DEV_FB, namelist[i]->d_name);
		fprintf(stderr, "\nfb device file: %s", fname);
		fd = open(fname, O_RDWR);
		if (fd < 0)
			continue;
		ioctl(fd, FBIOGET_FSCREENINFO, &fix_info);
		fprintf(stderr, "\ndevice found: %s\n\n", fix_info.id);
		if (strcmp(dev_name, fix_info.id) == 0)
			break;
		close(fd);
		fd = -1;
	}
	for (i = 0; i < ndev; i++)
		free(namelist[i]);

	return fd;
}

void render()
{
	// Set the file buffer contents (and thereby the whole 8 x 8 LED grid) to green.
	memset(fb, 0x0F00, 128);

	struct segment_t *seg_i;
	for(seg_i = trailEnd; seg_i->next; seg_i=seg_i->next) {
		fb->pixel[seg_i->x][seg_i->y] = 0xFFF0;
	}
	fb->pixel[seg_i->x][seg_i->y] = 0xF000;
}

int edgeReached()
{
  // CHECK IF PATH GOES OFF SCREEN
	if (trailEnd->x < 0 || trailEnd->x > 7 ||
	    trailEnd->y < 0 || trailEnd->y > 7) {
		fprintf(stderr, "Off the edge.\n");
		return 1;
	}
	return 0;
}

void addToPath() {
		struct segment_t *nextStep;

		nextStep = malloc(sizeof(struct segment_t));
		if (!nextStep) {
			fprintf(stderr, "Sorry, ran out of memory.\n");
			running = 0;
			return;
		}
		nextStep->x=trailEnd->x;
		nextStep->y=trailEnd->y;
		nextStep->next=trailEnd;
		trailEnd->prev=nextStep;
		trailEnd = nextStep;	    
}

void reset(void)
{
	memset(fb, 0, 128); // turn all the lights off

	if (trailEnd != NULL) {
		// return all allocated memory
		struct segment_t *seg_i;
		struct segment_t *nextBrick;
		seg_i=trailEnd;
		while (seg_i->next) {
			nextBrick=seg_i->next;
			fprintf(stderr, "Freeing memory for one segment.\n");
			free(seg_i);
			seg_i=nextBrick;
		}
		seg_i = NULL;
	}
	// reset for next game
	MsRedRidingHood.next = NULL;
	MsRedRidingHood.prev = NULL;
	MsRedRidingHood.x = rand() % 8;
	MsRedRidingHood.y = 0;
	trailEnd = &MsRedRidingHood;
}



void runAlongThePath() {
	struct segment_t seg_i;
	struct segment_t *nextStep;
	seg_i=MsRedRidingHood;
	while (seg_i.prev) {
		fprintf(stderr, "path point: %d, %d\n", seg_i.x, seg_i.y);
		nextStep=seg_i.prev;
		fb->pixel[seg_i.x][seg_i.y] = 0xF000;
		usleep(200000);  // .2 secs between steps
		fb->pixel[seg_i.x][seg_i.y] = 0xFFF0;
		seg_i=*nextStep;
	}
	usleep(3000000);   // wait 3 secs before next round
	reset();
}



void change_dir(unsigned int code)
{
	switch (code) {
		case KEY_UP:
			trailEnd->x--;
			break;
		case KEY_RIGHT:
			trailEnd->y++;
			break;
		case KEY_DOWN:
			trailEnd->x++;
			break;
		case KEY_LEFT:
			trailEnd->y--;
			break;
	}
}

void handle_events(int evfd)
{
	struct input_event ev[64];
	int i, rd;

	rd = read(evfd, ev, sizeof(struct input_event) * 64);
	if (rd < (int) sizeof(struct input_event)) {
		fprintf(stderr, "expected %d bytes, got %d\n",
			(int) sizeof(struct input_event), rd);
		return;
	}
	if (ev->type != EV_KEY)  // event type not a key or button
		return;    // jump to next round in the for-loop
	if (ev->value != 1)  // 1 = key pressed, 0 = released
		return;   
	switch (ev->code) {
		case KEY_ENTER:  // i.e. press the joystick down
			running = 0;  // stop the game 
			break;
		default:
			addToPath();
			change_dir(ev->code);
			fprintf(stderr, "code = %d\n", ev->code);  
	}
}

int main(int argc, char* args[])
{
	int ret = 0;   // return value
	int fbfd = 0;   // file handle for framebuffer (i.e. LED matrix)
	struct pollfd evpoll = {
		.events = POLLIN,
	};
	
	srand (time(NULL));  // Seeds the pseudo-random number generator used by rand() with the value seed

	// connect to the joystick
	evpoll.fd = open_evdev("Raspberry Pi Sense HAT Joystick");
	if (evpoll.fd < 0) {
		fprintf(stderr, "Event device 'Joystick' not found.\n");
		return evpoll.fd;
	}

	// open frame buffer (LED matrix)
	fbfd = open_fbdev("RPi-Sense FB");
	if (fbfd <= 0) {
		ret = fbfd;
		printf("Error: cannot open framebuffer device.\n");
		goto err_ev; 
	}
	
	/* On modern operating systems, it is possible to mmap (pronounced “em-map”) a file to a region of memory. 
		 When this is done, the file can be accessed just like an array in the program. This is more efficient 
		 than read or write, as only the regions of the file that a program actually accesses are loaded */
	fb = mmap(0, 128, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if (!fb) {
		ret = EXIT_FAILURE;
		printf("Failed to mmap.\n");
		goto err_fb;
	}

	// MsRedRidingHood.next = NULL;
	// MsRedRidingHood.prev = NULL;
	// MsRedRidingHood.x = rand() % 8;
	// MsRedRidingHood.y = 0;
	// trailEnd = &MsRedRidingHood;
	reset();
	while (running) {
		while (poll(&evpoll, 1, 0) > 0)
			handle_events(evpoll.fd);
		if (edgeReached()) {
			// pop off the last added brick (it's off-screen)
			trailEnd = trailEnd->next;
			free(trailEnd->prev);
			trailEnd->prev = NULL;

			fprintf(stderr, "calling runAlongThePath.\n");
			runAlongThePath();
		}
		render();
		usleep (200000);
	}
	reset();
	munmap(fb, 128);
err_fb:
	close(fbfd);
err_ev:
	close(evpoll.fd);
	return ret;
}
