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
	int x;
	int y;
};
struct path_t {
	struct segment_t head;
	struct segment_t *tail;
	enum direction_t heading;
};
struct Point_t {
	int x;
	int y;
};

struct fb_t {
	uint16_t pixel[8][8];
};


int running = 1;

struct path_t path = {
	{NULL, 4, 4},
	NULL,
	NONE,
};
struct Point_t MsRedRidingHood = {
	4, 4,
};

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
		fprintf(stderr, "\ndevice found: %s", fix_info.id);
		if (strcmp(dev_name, fix_info.id) == 0)
			break;
		close(fd);
		fd = -1;
	}
	for (i = 0; i < ndev; i++)
		free(namelist[i]);

	return fd;
}

// 0xFFFF rgb white 
// 0xFF0F rgb light yellow 
// 0x00F0 r_b fuchsia
// 0xF00F r_b fuchsia
// 0x0F0F _gb
// 0x0FFF _gb light 
// 0x0F00 _g_ green
// 0xF800 r__ 
// 0x8800 r__
// 0xFF00 yellow
void render()
{
	struct segment_t *seg_i;
	// memset(fb, 0x00F0, 128);		// fill bg colour
	for(seg_i = path.tail; seg_i->next; seg_i=seg_i->next) {
		fb->pixel[seg_i->x][seg_i->y] = 0x0FFF;
	}
	fb->pixel[seg_i->x][seg_i->y] = 0xFF0F;
	fb->pixel[7][0] = 0x6666;
	fb->pixel[7][1] = 0x6660;
	fb->pixel[7][2] = 0x6606;
	fb->pixel[7][3] = 0x6066;
	fb->pixel[7][4] = 0x6600;
	fb->pixel[7][5] = 0x6006;
	fb->pixel[7][6] = 0x6060;
	fb->pixel[7][7] = 0x6000;

	fb->pixel[6][0] = 0x0666;
	fb->pixel[6][1] = 0x0660;
	fb->pixel[6][2] = 0x0606;
	fb->pixel[6][3] = 0x0066;
	fb->pixel[6][4] = 0x0600;
	fb->pixel[6][5] = 0x0006;
	fb->pixel[6][6] = 0x0060;
	fb->pixel[6][7] = 0x0000;

	fb->pixel[5][0] = 0x6666;
	fb->pixel[5][1] = 0x6660;
	fb->pixel[5][2] = 0x6606;
	fb->pixel[5][3] = 0x0666;
	fb->pixel[5][4] = 0x6600;
	fb->pixel[5][5] = 0x0606;
	fb->pixel[5][6] = 0x0660;
	fb->pixel[5][7] = 0x0600;

	fb->pixel[4][0] = 0x6066;
	fb->pixel[4][1] = 0x6060;
	fb->pixel[4][2] = 0x6006;
	fb->pixel[4][3] = 0x0066;
	fb->pixel[4][4] = 0x6000;
	fb->pixel[4][5] = 0x0006;
	fb->pixel[4][6] = 0x0060;
	fb->pixel[4][7] = 0x0000;

	fb->pixel[3][0] = 0x6666;
	fb->pixel[3][1] = 0x6660;
	fb->pixel[3][2] = 0x6066;
	fb->pixel[3][3] = 0x0666;
	fb->pixel[3][4] = 0x6060;
	fb->pixel[3][5] = 0x0066;
	fb->pixel[3][6] = 0x0660;
	fb->pixel[3][7] = 0x0060;

	fb->pixel[2][0] = 0x6606;
	fb->pixel[2][1] = 0x6600;
	fb->pixel[2][2] = 0x6006;
	fb->pixel[2][3] = 0x0606;
	fb->pixel[2][4] = 0x6000;
	fb->pixel[2][5] = 0x0006;
	fb->pixel[2][6] = 0x0600;
	fb->pixel[2][7] = 0x0000;

	fb->pixel[1][0] = 0x6666;
	fb->pixel[1][1] = 0x6606;
	fb->pixel[1][2] = 0x6066;
	fb->pixel[1][3] = 0x0666;
	fb->pixel[1][4] = 0x6006;
	fb->pixel[1][5] = 0x0066;
	fb->pixel[1][6] = 0x0606;
	fb->pixel[1][7] = 0x0006;

	fb->pixel[0][0] = 0x6660;
	fb->pixel[0][1] = 0x6600;
	fb->pixel[0][2] = 0x6060;
	fb->pixel[0][3] = 0x0660;
	fb->pixel[0][4] = 0x6000;
	fb->pixel[0][5] = 0x0060;
	fb->pixel[0][6] = 0x0600;
	fb->pixel[0][7] = 0x0000;


}

int check_collision()
{
	struct segment_t *seg_i;

  // CHECK IF PATH GOES OFF SCREEN
	if (path.head.x < 0 || path.head.x > 7 ||
	    path.head.y < 0 || path.head.y > 7) {
		return 1;
	}
	return 0;
}

void addToPath() {
		struct segment_t * new_tail;

		new_tail = malloc(sizeof(struct segment_t));
		if (!new_tail) {
			fprintf(stderr, "Ran out of memory.\n");
			running = 0;
			return;
		}
		new_tail->x=path.tail->x;
		new_tail->y=path.tail->y;
		new_tail->next=path.tail;
		path.tail = new_tail;	
		
}



void reset(void)
{
	struct segment_t *seg_i;
	struct segment_t *next_tail;
	seg_i=path.tail;
	while (seg_i->next) {
		next_tail=seg_i->next;
		free(seg_i);
		seg_i=next_tail;
	}
	path.tail=seg_i;
	path.tail->next=NULL;
	path.tail->x=rand() % 8;
	path.tail->y=0;
	// path.heading = NONE;
}

void change_dir(unsigned int code)
{
	switch (code) {
		case KEY_UP:
		  addToPath();
			path.tail->x--;
			break;
		case KEY_RIGHT:
		  addToPath();
			path.tail->y++;
			break;
		case KEY_DOWN:
		  addToPath();
			path.tail->x++;
			break;
		case KEY_LEFT:
		  addToPath();
			path.tail->y--;
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

	// Sets the file buffer contents (and thereby the 8 x 8 LED grid) to 0.
	memset(fb, 0, 128);  // (128 is 8 * 8 * size_of char)

	path.tail = &path.head;
	reset();
	while (running) {
		while (poll(&evpoll, 1, 0) > 0)
			handle_events(evpoll.fd);
		if (check_collision(0)) {
			reset();
		}
		render();
		usleep (300000);
	}
	memset(fb, 0, 128);
	reset();
	munmap(fb, 128);
err_fb:
	close(fbfd);
err_ev:
	close(evpoll.fd);
	return ret;
}
