#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <X11/extensions/shape.h>
#include <X11/xpm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <jansson.h>

#include "out01.xpm"
#include "out02.xpm"
#include "out03.xpm"
#include "waiting.xpm"

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */

#define MAX_PROP_WORDS 100000

typedef struct _MSG {
  char* method;  
  char* content;  
  char* link;  
  struct _MSG* next;  
} MSG;

static void
x11_set_property(Display *dpy, Window win, char *atom, int state) {
  XEvent xev;
  int set = _NET_WM_STATE_ADD;
  Atom type, property;

  if (state == 0) set = _NET_WM_STATE_REMOVE;
  type = XInternAtom(dpy, "_NET_WM_STATE", 0);
  property = XInternAtom(dpy, atom, 0);
  xev.type = ClientMessage;
  xev.xclient.type = ClientMessage;
  xev.xclient.window = win;
  xev.xclient.message_type = type;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = set;
  xev.xclient.data.l[1] = property;
  xev.xclient.data.l[2] = 0;
  XSendEvent(dpy, DefaultRootWindow(dpy), 0, SubstructureNotifyMask, &xev);
}

static void
x11_set_opacity(Display *dpy, Window win, unsigned long opacity) {
  Atom _NET_WM_WINDOW_OPACITY = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", 0);
  XChangeProperty(dpy, win, _NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1L);
}

static void
x11_set_window_type(Display *dpy, Window win, char* type) {
  Atom _NET_WM_WINDOW_TYPE = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", 0);
  Atom _NET_WM_WINDOW_TYPE_DOCK = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0);
  XChangeProperty(dpy, win, _NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace,
      (unsigned char *)&_NET_WM_WINDOW_TYPE_DOCK, 1);
}

static int
x11_moveresize_window(Display *dpy, Window win, int x, int y, int width, int height) {
  XEvent xevent;
  static Atom moveresize = 0;
  if (!moveresize) {
    moveresize = XInternAtom(dpy, "_NET_MOVERESIZE_WINDOW", 0);
    if (!moveresize) return -1;
  }

  xevent.type = ClientMessage;
  xevent.xclient.window = win;
  xevent.xclient.message_type = moveresize;
  xevent.xclient.format = 32;
  xevent.xclient.data.l[0] = StaticGravity | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11);
  xevent.xclient.data.l[1] = x;
  xevent.xclient.data.l[2] = y;
  xevent.xclient.data.l[3] = width;
  xevent.xclient.data.l[4] = height;
  return XSendEvent(dpy, DefaultRootWindow(dpy), 0, SubstructureRedirectMask, &xevent);
}

MSG* free_msg(MSG* top) {
  MSG* next;
  if (!top) return NULL;
  next = top->next;
  if (top->method) free(top->method);
  if (top->content) free(top->content);
  if (top->link) free(top->link);
  free(top);
  return next;
}

int
main(int argc, char* argv[]) {
  Display *dpy;
  Drawable root;
  Window win;
  Pixmap shape[10];
  XImage *image[10];
  int step = 0, waittime = 0;
  char **xpms[] = {
    out01, out02, out03, out02, waiting
  };
  int screen, width, height;
  GC  gc;
  XFontSet fs;
  XEvent event;
  Atom gopherNotify;
  int i, mode = 0;
  int x, y;
  int dx = 0, dy = 0, da = 5;
  int alpha = 0;
  char **miss;
  char *def;
  int n_miss;
  MSG *msg = NULL;
  int foreground = 0;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-f") == 0)
      foreground = 1;
  }

  if (foreground == 0) {
    pid_t pid = fork();
    if (pid < 0)
      exit(EXIT_FAILURE);
    if (pid > 0)
      exit(EXIT_SUCCESS);
  }

  srand(time(NULL));
  dx = rand() % 10 + 5;

  setlocale(LC_CTYPE,"");

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "error: cannot connect to X server\n");
    return 1;
  }

  screen = DefaultScreen(dpy);
  root = DefaultRootWindow(dpy);

  width = DisplayWidth(dpy, screen);
  height = DisplayHeight(dpy, screen);
  win = XCreateSimpleWindow(dpy, root,
      0, 0, 200, 200, 0, BlackPixel(dpy, 0), WhitePixel(dpy, 0));
  gc = XCreateGC(dpy, win, 0, 0);
  fs = XCreateFontSet(dpy, "-*-*-*-R-Normal--14-130-75-75-*-*", &miss, &n_miss, &def);

  for (i = 0; i < 5; i++) {
    XGCValues values;
    XImage *mask = NULL, *maskr = NULL;
    int ix, iy;
    XpmCreateImageFromData(dpy, xpms[i], &image[i], &mask, NULL);
    shape[i] = XCreatePixmap(dpy, root, 200, 200, 1);
    GC maskgc = XCreateGC(dpy, shape[i], 0, &values);
    XPutImage(dpy, shape[i], maskgc, mask, 0, 0, 0, 0, 200, 200);

    image[i+5] = XSubImage(image[i], 0, 0, 200, 200);
    maskr = XSubImage(mask, 0, 0, 200, 200);
    for (ix = 0; ix < 200; ix++) {
      for (iy = 0; iy < 200; iy++) {
        XPutPixel(image[i+5], 200 - ix, iy, XGetPixel(image[i], ix, iy));
        XPutPixel(maskr, 200 - ix, iy, XGetPixel(mask, ix, iy));
      }
    }
    shape[i+5] = XCreatePixmap(dpy, root, 200, 200, 1);
    XPutImage(dpy, shape[i+5], maskgc, maskr, 0, 0, 0, 0, 200, 200);

    XFreeGC(dpy, maskgc);
  }

  x11_set_property(dpy, win, "_NET_WM_STATE_STAYS_ON_TOP", 1);
  x11_set_property(dpy, win, "_NET_WM_STATE_ABOVE", 1);
  x11_set_property(dpy, win, "_NET_WM_STATE_SKIP_TASKBAR", 1);
  x11_set_property(dpy, win, "_NET_WM_STATE_SKIP_PAGER", 1);
  x11_set_property(dpy, win, "_NET_WM_STATE_STICKY", 1);
  x11_set_property(dpy, win, "_NET_WM_WINDOW_OPACITY", 200);
  x11_set_window_type(dpy, win, "_NET_WM_WINDOW_TYPE_DOCK");

  x = -200;
  y = height - 200;
  x11_moveresize_window(dpy, win, x, y, 200, 200);
  XSelectInput(dpy, win, ExposureMask | PropertyChangeMask);
  XStoreName(dpy, win, "Gopher");
  XMapWindow(dpy, win);

  srand(time(NULL));

  gopherNotify = XInternAtom(dpy, "GopherNotify", 0);

  int curr = 0;
  while(1) {
    if (mode == 0 && msg && msg->method) {
      if (strcmp(msg->method, "message") == 0) {
        mode = 2;
        waittime = 60;
      } else if (strcmp(msg->method, "jump") == 0) {
        mode = 1;
        dy = -20;
      }
    }
    if (msg && strcmp(msg->method, "exit") == 0) {
      waittime = 30;
      da = -5;
    }
    switch (mode) {
      case 0:
        step++;
        x += dx;
        y += dy;
        if (rand() % 40 == 0) {
          mode = 1;
          dy = -20;
        }
        break;
      case 1:
        step = 0;
        x += dx / 2;
        y += dy;
        dy += 2;
        if (y > height - 200) {
          y = height - 200;
          dy = 0;
          mode = 0;
          msg = free_msg(msg);
        }
        break;
      case 2:
        waittime--;
        if (waittime <= 0) {
          mode = 0;
          msg = free_msg(msg);
        }
        break;
    }

    if (da != 0) {
      alpha += da;
      if (alpha > 99) {
        alpha = 99;
        da = 0;
      } else if (alpha < 0) {
        alpha = 0;
        da = 0;
        break;
      }
    }

    if (mode == 3 && waittime < 0) {
      break;
    }

    if ((dx < 0 && x < 0) || (dx > 0 && x > width - 200)) dx = -dx;
    curr = (mode==2?4:(step%4))+(dx>0?0:5);
    XShapeCombineMask(dpy, win, ShapeBounding, 0, 0, shape[curr], ShapeSet);
    if (da != 0) x11_set_opacity(dpy, win, (unsigned long)((alpha - 100) * (0xffffffff / 100)));

    XPutImage(dpy, win, gc, image[curr], 0, 0, 0, 0, 200, 200);
    if (mode == 2 && msg->content) {
      XSetForeground(dpy, gc, BlackPixel(dpy, 0));
      Xutf8DrawString(dpy, win, fs, gc, 20, 150, msg->content, strlen(msg->content));
    } else {
      x11_moveresize_window(dpy, win, x, y, 200, 200);
    }

    while(XPending(dpy) > 0) {
      XNextEvent(dpy, &event);
      switch(event.type) {
        case PropertyNotify:
          {
            unsigned char *prop = NULL;
            int	result, actualFormat;
            unsigned long numItems, bytesAfter;
            Atom actualType;

            result = XGetWindowProperty(dpy, win, gopherNotify, 0L,
                (long)MAX_PROP_WORDS, 1,
                XA_STRING, &actualType,
                &actualFormat, &numItems, &bytesAfter,
                &prop);
            if (result == 0 && prop) {
              json_error_t error;
              json_t *body = json_loads((char*) prop, 0, &error);
              if (body != NULL) {
                MSG* newmsg = malloc(sizeof(MSG));;
                memset(newmsg, 0, sizeof(MSG));
                json_t *v;
                v = json_object_get(body, "method");
                if (v != NULL) newmsg->method = strdup(json_string_value(v));
                v = json_object_get(body, "content");
                if (v != NULL) newmsg->content = strdup(json_string_value(v));
                v = json_object_get(body, "link");
                if (v != NULL) newmsg->link = strdup(json_string_value(v));
                json_decref(body);
                if (msg) {
                  MSG* q = msg;
                  while (q->next) q = q->next;
                  q->next = newmsg;
                } else msg = newmsg;
              }
            }
          }
          break;
        default:
          break;
      }
    }
    XFlush(dpy);
    usleep(30000);
  }
  XCloseDisplay(dpy);
}

// vim: set sw=2 cino=J2 et:
