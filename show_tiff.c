// simple tiff file vewer based on x4.c simple xwindows example using pixmap
// Usage: show_tiff [-q] [-d] [-s start] [-f file_pattern]
// -q = do not restart at end of images
// -e = debug output
// -s start = starting number
// -m minimum = minimux index
// -x maximum = maximum index
// -f file_pattern = pattern used to find image
// -d dir_path = directory path added to file pattern
// -h = print help information
//
// compile: gcc show_tiff.c -o show_tiff -lX11 -ltiff

#include <X11/Xlib.h> // must precede most other headers!
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <tiffio.h>
#include <limits.h>
#include <time.h> 

void update_screen(tdata_t raster, uint32 width, uint32 height);
int update_tiff(tdata_t *raster, uint32 *width, uint32 *height, char *dir_path, char *img_file, int img_min, int img_max, int *img_no, int qflag);
char *get_fullpath(char *dir_path, char *img_file, int img_no);
char *conv_template_to_char(char *temp, int img_no);
int read_tiff(tdata_t *raster, uint32 *width, uint32 *height, char *img_file_name);

// default image file pattern
char *img_file = "FRONT%04i.tif";
//char img_file[] = "FRONT0000.tif";

// global variables for image fetch
int img_min = 0; // minimum image number
int img_max = 0; // maximum index (0 = no maximum)
int img_no = 0; // starting image number
tdata_t raster = 0; // pointer to image mallox space
uint32 width, height; // feteched image width and height
char *dir_path = ""; // directory path is not used (includes null char as string def)

// global variables needed for xwindows display
int XRES, YRES;
Display *dsp;
Window win;
int screen;
Pixmap pxm;
GC gc;
unsigned int white;
unsigned int black;

// debug flag global so visible in functions
int dflag = 0;

int main(int argc, char **argv){

  int qflag = 0; // flag for no restart
  int c;

  opterr = 0;

  while ((c = getopt (argc, argv, "qehf:d:s:m:x:")) != -1) // use short form and no long options
    switch (c)
      {
      case 'q':
        qflag = 1;
        break;
      case 'e':
        dflag = 1;
        break;
      case 'f':
        img_file = optarg;
        break;
      case 'd':
        dir_path = optarg;
        break;
      case 's':
        // https://stackoverflow.com/questions/4796662/how-to-take-integers-as-command-line-arguments
        img_no = atoi( optarg );
        break;
      case 'm':
        img_min = atoi( optarg );
        break;
      case 'x':
        img_max = atoi( optarg );
        break;
      case 'h':
        printf("Usage: %s [-q] [-e] [-s start] [-f file_pattern] [-d directory_path]\n", argv[0]);
        // forget removing the repeated \t - https://stackoverflow.com/questions/14678948/how-to-repeat-a-char-using-printf
        printf("\t-q \t\t\tdo not restart at end of images\n");
        printf("\t-e \t\t\tdebug output\n");
        printf("\t-s start\t\tstarting number (default: %d)\n",img_no);
        printf("\t-m minimum\t\tminimum number (default: %d)\n",img_min);
        printf("\t-x maximum\t\tmaximum number (default: no maximum)\n");
        printf("\t-f file_pattern\t\t(default: '%s')\n",img_file);
        printf("\t-d directory_path\t\t(default: '%s')\n",dir_path);
        return EXIT_SUCCESS; // https://stackoverflow.com/questions/9944785/what-is-the-difference-between-exit0-and-exit1-in-c
      case '?':
        if (optopt == 'f' || optopt == 'd' || optopt == 's')
          fprintf (stderr, "Error: Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Error: Unknown option `-%c'.\n", optopt);
        else {
          fprintf (stderr, "Error: Unknown option character `\\x%x'.\n", optopt);
          return EXIT_FAILURE;
        }
      default:
        abort ();
      }
 
  if (dflag) printf ("qflag = %d, minimum = %d, maximum = %d, start = %d, img_file = %s, dir_path = %s\n", qflag, img_min, img_max, img_no, img_file, dir_path);

  if (img_no<img_min) {
    fprintf (stderr, "img_no<img_min therefore setting starting image to %d\n",img_min);
    img_no=img_min;
  }

  if (img_max && img_no>img_max) { // only do max comparison if img_max!=0
    fprintf (stderr, "img_no>img_max therefore setting starting image to %d\n",img_max);
    img_no=img_max;
  }

  dsp = XOpenDisplay( NULL ); // initialise display
  if ( !dsp ) { return EXIT_FAILURE; } // exit with error

  screen = DefaultScreen(dsp);

  if (DefaultDepth(dsp,screen) != 24) {
    fprintf(stderr, "Not a 24bit (truecolour) display");
    XCloseDisplay(dsp);
    return EXIT_FAILURE; // exit with error
  }

  // read in first so we know the image size
  update_tiff(&raster, &width, &height, dir_path, img_file, img_min, img_max, &img_no, qflag);

  XRES = width; //DisplayWidth(dsp,screen)/2;
  YRES = height; //DisplayHeight(dsp,screen)/2;

  white = WhitePixel(dsp,screen);
  black = BlackPixel(dsp,screen);

  win = XCreateSimpleWindow(dsp,
                            DefaultRootWindow(dsp),
                            0, 0,   // origin
                            XRES, YRES, // size
                            0, black, // border width/clr
                            white);   // backgrd clr

  // respond to window manager delete event e.g. user click
  Atom wmDelete=XInternAtom(dsp, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(dsp, win, &wmDelete, 1);

  pxm = XCreatePixmap(dsp, win, XRES, YRES, 24); // image created before copy to screen

  gc = XCreateGC(dsp, win,
                 0,       // mask of values
                 NULL);   // array of values

  XSetForeground(dsp, gc, black);

  // setup xwindows event handling
  XEvent evt;
  long eventMask = StructureNotifyMask;
  eventMask |= ButtonPressMask|ButtonReleaseMask|KeyPressMask|KeyReleaseMask;
  XSelectInput(dsp, win, eventMask);
  
  // find keycode for exit key
  KeyCode keyQ;
  keyQ = XKeysymToKeycode(dsp, XStringToKeysym("Q"));

  XMapWindow(dsp, win);

  // wait until window appears
  do { XNextEvent(dsp,&evt); } while (evt.type != MapNotify);
  
  // display first tiff image
  update_screen(raster, width, height);

  int loop = 1;

  // use high resolution clock to limit build up of keypress events, first read initial time
  // https://stackoverflow.com/questions/14682824/measuring-elapsed-time-in-linux-for-a-c-program
  struct timespec prev, curr;
  clock_gettime(CLOCK_MONOTONIC, &prev);

  while (loop) {

    // http://www-h.eng.cam.ac.uk/help/tpl/graphics/X/X11R5/node25.html
    XNextEvent(dsp, &evt);

    switch (evt.type) {

      case (ButtonRelease) : // on mouse button release go to next image
        update_tiff(&raster, &width, &height, dir_path, img_file, img_min, img_max, &img_no, qflag);
        update_screen(raster, width, height);
        break;

      case (KeyRelease) : // on key press and release
        if (evt.xkey.keycode == keyQ) { loop = 0; break; } // on quit key break loop and exit program
        clock_gettime(CLOCK_MONOTONIC, &curr); // get current time
        double diff = (curr.tv_sec - prev.tv_sec) * 1e6 + (curr.tv_nsec - prev.tv_nsec) / 1e3; // time difference in microseconds
        if (dflag) printf("time (uS): %g\n",diff);
        if (diff>20e3) { // limit key presses to 50Hz image update
          if (update_tiff(&raster, &width, &height, dir_path, img_file, img_min, img_max, &img_no, qflag)) { // get next image but if no restart then quit
            update_screen(raster, width, height); // new image so update screen
            clock_gettime(CLOCK_MONOTONIC, &prev); // reset timestamp
          } else {
            loop = 0; break; // quit program
          }
        }
        break;

      case (ConfigureNotify) : // wm notification of change of parameters
        // check for window resize
        if (evt.xconfigure.width != XRES || evt.xconfigure.height != YRES)
        {
          XRES = evt.xconfigure.width; // NEEDS UPDATE AS IGNORED AT THE MOMENT
          YRES = evt.xconfigure.height;
          update_screen(raster, width, height);
        }
        break;

      case (ClientMessage) : // wm notification of close application message
        if (evt.xclient.data.l[0] == wmDelete) loop = 0;
        break;

      default :
        break;
    }
  } 

  // clean up xwindows resources
  XFreePixmap(dsp,pxm); 
  XDestroyWindow(dsp, win);
  XCloseDisplay(dsp);

  // make sure we have freed any memory allocated
  _TIFFfree(raster);

  return EXIT_SUCCESS;
}

void update_screen(tdata_t raster, uint32 width, uint32 height)
{
  //XClearWindow(dsp, win);
  XSetForeground(dsp,gc,white);

  // simple copy of pixel values from tiff to xwindows image buffer, one pixel at a time (NOT OPTIMUM!)
  int x, y;
  for (x=0;x<width;x++) {
    for (y=0;y<height;y++) {
      int i=y*width+x;
      int r=(((uint32 *)raster)[i]>>16)&255, g=(((uint32 *)raster)[i]>>8)&255, b=(((uint32 *)raster)[i]>>0)&255;
      XSetForeground(dsp,gc, (r<<16)|(g<<8)|b);
      XDrawPoint(dsp, pxm, gc, x, height-y-1);
    }
  }
  // copy pixmap to window (use hardware blit in future);
  XCopyArea(dsp,pxm,win,gc,0,0,XRES,YRES,0,0);

  // update screen with image from buffer
  XFlush(dsp);
}

int update_tiff(tdata_t *raster, uint32 *width, uint32 *height, char *dir_path, char *img_file, int img_min, int img_max, int *img_no, int qflag) {
  // read in first so we know the image size
  if (!read_tiff(raster, width, height, get_fullpath(dir_path, img_file, *img_no))) {
    // invalid file fetch
    if (qflag) { // do not restart
      return 0; // no update i.e. quit
    } else { // try and fetch first image
      *img_no = img_min;
      if (!read_tiff(raster, width, height, get_fullpath(dir_path, img_file, *img_no))) {
        fprintf( stderr, "Error: Cannot read first image file: %s\n",get_fullpath(dir_path, img_file, *img_no));
        exit(EXIT_FAILURE);
      }
    }
  }
  (*img_no)++; // if successful setup for next image
  if (img_max && *img_no>img_max) *img_no=img_min; // only do max comparison if img_max!=0
  return 1;
}

char *get_fullpath(char *dir_path, char *img_file, int img_no) {
  char *buf = conv_template_to_char(img_file, img_no); // get image name
  char *fullpath;
  if (strlen(dir_path)>0) { // avoid adding '/' at start of fullpath if img_dir not given
    // https://stackoverflow.com/questions/2153715/concatenating-file-with-path-to-get-full-path-in-c?rq=1
    /* + 2 because of the '/' and the terminating 0 */
    fullpath = malloc(strlen(dir_path) + strlen(buf) + 2);
    if (fullpath == NULL) {
      /* deal with error and exit */
      fprintf( stderr, "Error: Memory error, cannot create full path");
      exit(EXIT_FAILURE);
    }
    sprintf(fullpath, "%s/%s", dir_path, buf);
    free(buf); // partial name not needed any more
  } else {
    fullpath = buf;
  }
  return fullpath;
}

char *conv_template_to_char(char *temp, int img_no) {
  // https://stackoverflow.com/questions/29087129/how-to-calculate-the-length-of-output-that-sprintf-will-generate
  ssize_t bufsz = snprintf(NULL, 0, temp, img_no); // variable length of pattern match so need to get size of char string
  if (bufsz>0 && bufsz<INT_MAX) { // check for sensible values
    char* buf = malloc(bufsz + 1); // get memory (plus \0 end of string)
    snprintf(buf, bufsz + 1, temp, img_no); // put filename in memory
    return buf; // return filename
  } else {
    return 0; // fail
  }
}

int read_tiff(tdata_t *raster, uint32 *width, uint32 *height, char *img_file_name) {
    int retval = 0; // assume failure
    if (dflag) printf("File name of tiff file: %s\n", img_file_name);
    TIFF* tif; // file handle
    if ((tif = TIFFOpen(img_file_name, "r"))) { // if not able to open then exit
      uint16 samples, bits; // get image properties
      TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, width);          // uint32 width;
      TIFFGetField(tif, TIFFTAG_IMAGELENGTH, height);        // uint32 height;
      TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);   // uint16 samples;
      TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits);        // uint16 bits;
      if (dflag) printf("Width: %u, Height: %u, Samples_per_pixel: %u, Bits_per_sample: %u\n", *width, *height, samples, bits);
      int32 npixels=*width*(*height);
      if (*raster) { // remalloc as image size may have changed
        *raster=(uint32 *)_TIFFrealloc(*raster, npixels *sizeof(uint32));
      } else { // first time so malloc fresh memory
        *raster=(uint32 *)_TIFFmalloc(npixels *sizeof(uint32));
      }
      if (TIFFReadRGBAImage(tif, *width, *height, *raster, 0)) {
          if (dflag) printf("Read success\n");
          retval = 1;
      } else {
          if (dflag) printf("Read fail!\n");
      }
      TIFFClose(tif);
    }
    return retval;
}
