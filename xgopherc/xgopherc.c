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

Atom gopherNotify;

static int
enum_windows(Display *dpy, Window win, int depth) {
  Window parent, *children;
  unsigned int count = 0;
  int r = 1, n = 0;
  char *name = NULL;

  XFetchName(dpy, win, &name);
  if (name && strcmp("Gopher", name) == 0) {
    // TODO
    char* p = "{\"method\": \"message\", \"content\": \"ハローワールド\"}";
    XChangeProperty(dpy, win, gopherNotify, XA_STRING, 8,
        PropModeAppend, (unsigned char*) p, (int) strlen(p));
  }
  if (name) XFree(name);

  if (XQueryTree(dpy, win, &win, &parent, &children, &count) == 0) {
    fprintf(stderr, "error: XQueryTree error\n");
    return 0;
  }
  for (n = 0; r && n < count; ++n) {
    r = enum_windows(dpy, children[n], depth+1);
  }
  XFree(children);
  return r;
}

int
main(int argc, char *const argv[]) {
  Display *dpy = NULL;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "error: cannot connect to X server\n");
    return 1;
  }
  gopherNotify = XInternAtom(dpy, "GopherNotify", 0);
  enum_windows(dpy, DefaultRootWindow(dpy), 0);
  XCloseDisplay(dpy);
  return 0;
}
// vim: set sw=2 cino=J2 et:
