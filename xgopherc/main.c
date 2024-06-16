#include <X11/Xlib.h>
#include <X11/Xatom.h>
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


typedef struct {
  Window w[100];
  int n;
} WINLIST;

typedef void (*enum_callback)(Window, void*);

static void
list_windows(Window w, void* data) {
  WINLIST *wl = (WINLIST*) data;
  wl->w[wl->n++] = w;
}

static void
enum_windows(Display *dpy, Window win, enum_callback f, void* data) {
  Window parent, *children = NULL;
  unsigned int count = 0;
  char *name = NULL;

  XFetchName(dpy, win, &name);
  if (name) {
    if (strcmp("Gopher", name) == 0) {
      if (f) f(win, data);
      XFree(name);
      return;
    }
    XFree(name);
  }

  if (XQueryTree(dpy, win, &win, &parent, &children, &count) == 0) {
    return;
  }

  if (count > 0) {
    int n;
    for (n = 0; n < count; n++) {
      enum_windows(dpy, children[n], f, data);
    }
  }
  XFree(children);
}

static void
send_message(Display *dpy, Window win, const char* method, const char* content, const char* link) {
  char* p;
  json_t *object = json_object();
  json_object_set(object, "method", json_string("exit"));
  json_object_set(object, "content", json_string(content));
  json_object_set(object, "link", json_string(link));
  p = json_dumps(object, 0);
  XChangeProperty(dpy, win, XInternAtom(dpy, "GopherNotify", 0), XA_STRING, 8,
      PropModeAppend, (unsigned char*) p, (int) strlen(p));
  json_decref(object);
}

int
main(int argc, char *const argv[]) {
  Display *dpy;
  int result, r = 0;
  char* method = NULL;
  char* content = NULL;
  char* link = NULL;

  while((result = getopt(argc,argv,"jxXm:l:")) != -1){
    switch(result){
      case 'j':
        method = "jump";
        break;
      case 'm':
        method = "message";
        content = optarg;
        break;
      case 'l':
        link = optarg;
        break;
      case 'x':
        method = "exit";
        break;
      case 'X':
        method = "exit-all";
        break;
      case ':':
      case '?':
        fprintf(stdout, "usage of %s [-j] [-m text] [-l link]\n", argv[0]);
        return 1;
    }
  }

  srand(time(NULL));

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "error: cannot connect to X server\n");
    return 1;
  }
  WINLIST wl = {0};
  enum_windows(dpy, DefaultRootWindow(dpy), list_windows, &wl);
  if (wl.n == 0) {
    fprintf(stderr, "gopher not found\n");
    r = 1;
  } else if (method) {
    if (strcmp(method, "exit-all") == 0) {
      int n;
      for (n = 0; n < wl.n; n++) send_message(dpy, wl.w[n], "exit", content, link);
    } else {
      Window win = wl.w[rand() % wl.n];
      send_message(dpy, win, "exit", content, link);
    }
  }
  XCloseDisplay(dpy);
  return r;
}

// vim: set sw=2 cino=J2 et:
