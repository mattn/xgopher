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

Atom gopherNotify;

static int
enum_windows(Display *dpy, Window win) {
  Window parent, *children;
  unsigned int count = 0;
  int n = 0;
  Window w = 0;
  char *name = NULL;

  XFetchName(dpy, win, &name);
  if (name && strcmp("Gopher", name) == 0) {
    return win;
  }
  if (name) XFree(name);

  if (XQueryTree(dpy, win, &win, &parent, &children, &count) == 0) {
    fprintf(stderr, "error: XQueryTree error\n");
    return 0;
  }
  for (n = 0; w == 0 && n < count; ++n) {
    w = enum_windows(dpy, children[n]);
  }
  XFree(children);
  return w;
}

int
main(int argc, char *const argv[]) {
  Display *dpy;
  Window win;
  int result, r = 0;
  char* method = NULL;
  char* content = NULL;
  char* link = NULL;

  while((result = getopt(argc,argv,"jm:l:")) != -1){
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
      case ':':
      case '?':
        fprintf(stdout, "usage of %s [-j] [-m text] [-l link]\n", argv[0]);
        return 1;
    }
  }

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "error: cannot connect to X server\n");
    return 1;
  }
  gopherNotify = XInternAtom(dpy, "GopherNotify", 0);
  win = enum_windows(dpy, DefaultRootWindow(dpy));
  if (win == 0) {
    fprintf(stderr, "gopher not found\n");
    r = 1;
  } else {
    char* p;
    json_t *object = json_object();
    json_object_set(object, "method", json_string(method));
    json_object_set(object, "content", json_string(content));
    json_object_set(object, "link", json_string(link));
    p = json_dumps(object, 0);
    XChangeProperty(dpy, win, gopherNotify, XA_STRING, 8,
        PropModeAppend, (unsigned char*) p, (int) strlen(p));
    json_decref(object);
  }
  XCloseDisplay(dpy);
  return r;
}

// vim: set sw=2 cino=J2 et:
