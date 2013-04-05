/*#include <>*/

enum {
	WM_PROTOCOLS,
	WM_DELETE,
	WM_STATE,
	WM_TASK_FOCUS,
	WM_LAST
};

/* EWMH atoms */
enum {
	NET_SUPPORTED,
	NET_WM_NAME,
	NET_WM_STATE,
	NET_WM_FULLSCREEN,
	NET_ACTIVE_WINDOW,
	NET_WM_WINDOW_TYPE,
	NET_WM_WINDOW_TYPE_DIALOG,
	NET_CLIENT_LIST,
	NET_LAST
};

/* Cursor */
enum {
	CURSOR_NORMAL,
	CURSOR_RESIZE,
	CURSOR_MOVE,
	CURSOR_LAST
};

/* Color */
enum {
	COLOR_BORDER,
	COLOR_FG,
	COLOR_BG,
	COLOR_LAST
};

/* Clicks */
enum {
	CLICK_TAGBAR,
	CLICK_SYMBOL,
	CLICK_STATUS,
	CLICK_TITLE,
	CLICK_CLIENT,
	CLICK_ROOT,
	CLICK_ROOT
};

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;

typedef struct {
	int x;
	int y;
	int width;
	int height;
} Size;

struct Client {
	char name[256];
	int x, y, width, height;
	int save_x, save_y, save_width, save_height;
	unsigned int tags;
	Client *next;
	Monitor *monitor;
	Window window;
};

struct Monitor {
//	char symbol[16];
	Client *active;
	Monitor *next;
	Window bar;
};

typedef struct {
	int x, y, width, height;
	XftColor normal[COLOR_LAST];
	XftColor active[COLOR_LAST];
	Drawable drawable;
	GC gc;
	struct {
		int ascent;
		int descent;
		int height;
		XftFont *xfont;
	} font;
} Canvas;

static Display *display;
static int (*xerror_handler)(Display *, XErrorEvent *);
static Window root;
static int screen;
static int screen_width, screen_height;
static int bar_height;

static Atom atom_wm[WM_LAST], atom_net[NET_LAST];
static Cursor cursor[CURSOR_LAST];
static Bool use_xkb;

static Canvas canvas;
static monitor_t *monitors, *active_monitor = NULL;


static void die(const char *error, ...) {
	va_list ap;
	va_start(ap, error);
	vfprintf(stderr, error, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void manage(Window window, XWindowAttributes *attributes) {
	Client *c;
	Window trans = None;

	if (!(c = calloc(1, sizeof(Client)))) {
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));
	}
	c->window = window;
	//update_title(c);
	if (XGEtTransientForHint(display, window, &trans) && (t)) {
		c->monitor = t->monitor;
		c->tags = t->tags;
	} else {
		c->monitor = active_monitor;
	}
}

void update_bars() {
	monitor_t *m;
	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ButtonPressMask | ExposureMask;
	for (m = monitors; m; m = m->next) {
		if (m->bar) {
			continue;
		}
		m->bar = XCreateWindow(display, root, m->win_x, m->bar_y, m->win_width, bar_height, 0, DefaultDepth(display, screen), CopyFromParent, DefaultVisual(display, screen), CWOverrideRedirect | CWBackPixmap | CWeventMask, &wa);
		XDefineCursor(display, m->bar, cursor[CURSOR_NORMAL]);
		XMapRaised(display, m->bar);
	}
}

void update_status() {
	//
}

void update_windows() {
	unsigned int i, n;
	Window w, w2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(display, root, &w, &w2, &wins, &n)) {
		for (i = 0; i < n; i++) {
			if (!XGetWindowAttributes(display, wins[i], &wa) || wa.override_redirect || XGEtTransientForHint(display, wins[i], &w)) {
				continue;
			}
			if (wa.map_state == IsViewable || win_state(wins[i]) == IconicState) {
				manage(wins[i], &wa);
			}
		}
		for (i = 0; i < n; i++) {
			if (!XGetWindowAttributes(display, wins[i], &wa)) {
				continue;
			}
			if (XGEtTransientForHint(display, wins[i], &w) && (wa.map_state == IsViewable || win_state(wins[i]) == IconicState)) {
				manage(wins[i], &wa);
			}
		}
		if (wins) {
			XFree(wins);
		}
	}
}

static int xerror_start(Display *display, XErrorEvent *event) {
	die("dwm: another window manager is already running\n");
	return -1;
}

void prepare() {
	/* check other wm */
	xerror_handler = XSetErrorHandler(xerror_start);
	XSelectInput(display, DefaultRootWindow(display), SubstructureRedirectMask);
	XSync(display, False);
	XSetErrorHandler(xerror);
	XSync(display, False);
}

void initialize() {
	XSetWindowAttributes wa;
	int dummy = 0, major = XkbMajorVersion, minor = XkbMinorVersion;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init screen */
	screen = DefaultScreen(display);
	root = RootWindow(display, screen);
	screen_width = DisplayWidth(display, screen);
	screen_height = DisplayHeight(display, screen);

	/* init font*/
	if (!(canvas.font.xfont = XftFontOpenName(display, screen, font)) && !(canvas.font.xfont = XftFontOpenName(display, screen, "fixed"))) {
		die("error, cannot load font: '%s'\n", font);
	}
	canvas.font.ascent = canvas.font.xfont->ascent;
	canvas.font.descent = canvas.font.xfont->descent;
	canvas.font.height = canvas.font.ascent + canvas.font.descent;

	/* init atoms */
	atom_wm[WM_PROTOCOLS] = XInternAtom(display, "WM_PROTOCOLS", False);
	atom_wm[WM_DELETE] = XInternAtom(display, "WM_DELETE_WINDOW", False);
	atom_wm[WM_STATE] = XInternAtom(display, "WM_STATE", False);
	atom_wm[WM_TASK_FOCUS] = XInternAtom(display, "WM_TASK_FOCUS", False);

	atom_net[NET_ACTIVE_WINDOW] = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
	atom_net[NET_SUPPORTED] = XInternAtom(display, "_NET_SUPPORTED", False);
	atom_net[NET_WM_NAME] = XInternAtom(display, "_NET_WM_NAME", False);
	atom_net[NET_WM_FULLSCREEN] = XInternAtom(display, "_NET_WM_FULLSCREEN", False);
	atom_net[NET_WM_WINDOW_TYPE] = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
	atom_net[NET_WM_WINDOW_TYPE_DIALOG] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	atom_net[NET_CLIENT_LIST] = XInternAtom(display, "_NET_CLIENT_LIST", False);

	/* init cursors */
	cursor[CURSOR_NORMAL] = XCreateFontCursor(display, XC_left_ptr);
	cursor[CURSOR_RESIZE] = XCreateFontCursor(display, XC_sizing);
	cursor[CURSOR_MOVE] = XCreateFontCursor(display, XC_fleur);

	/* init appearance TODO: use config file load */
	canvas.normal[COLOR_BORDER] = 0;
	canvas.normal[COLOR_FG] = 0;
	canvas.normal[COLOR_BG] = 0;
	canvas.active[COLOR_BORDER] = 0;
	canvas.active[COLOR_FG] = 0;
	canvas.active[COLOR_BG] = 0;

	canvas.drawable = XCreatePixmap(display, root, screen_width, bar_height, DefaultDepth(display, screen));
	canvas.gc = XCreateGC(display, root, 0, NULL);
	XSetLineAttributes(display, canvas.gc, 1, LineSolid, CapButt, JoinMiter);

	/* init bars */
	update_bars();
	update_status();

	/* EWMH support per view */
	XChangeProperty(display, root, atom_net[NET_SUPPORTED], XA_ATOM, 32, PropModeReplace, atom_net, NET_LAST);
	XDeleteProperty(display, root, atom_net[NET_CLIENT_LIST]);

	/* select for events */
	wa.cursor = cursor[CURSOR_NORMAL];
	wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
	XChangeWindowAttributes(display, root, CWEventMask | CWCursor, &wa);
	XSelectInput(display, root, wa.event_mask);

	/* init xkb */
	use_xkb = XkbQueryExtension(display, &dummy, &dummy, &dummy, &major, &minor);

	grab_keys();
	update_windows();
}

void button_press(XEvent *event) {

	Monitor *m;
}

void client_message(XEvent *event) {
	//
}

void configure_request(XEvent *event) {
	//
}

void configure_notify(XEvent *event) {
	//
}

void destroy_notify(XEvent *event) {
	//
}

void enter_notify(XEvent *event) {
	//
}

void expose(XEvent *event) {
	//
}

void focus_in(XEvent *event) {
	//
}

void key_press(XEvent *event) {
	//
}

void mapping_notify(XEvent *event) {
	//
}

void map_request(XEvent *event) {
	//
}

void motion_notify(XEvent *event) {
	//
}

void property_notify(XEvent *event) {
	//
}

void unmap_notify(XEvent *event) {
	//
}

void run() {
	XEvent e;
	void (*handler)(XEvent *);

	XSync(display, False);
	while (running && !XNextEvent(display, &e)) {
		switch (e.type) {
		case ButtonPress:
			handler = button_press;
			break;
		case ClientMessage:
			handler = client_message;
			break;
		case ConfigureRequest:
			handler = configure_request;
			break;
		case ConfigureNotify:
			handler = configure_notify;
			break;
		case DestroyNotify:
			handler = destroy_notify;
			break;
		case EnterNotify:
			handler = enter_notify;
			break;
		case Expose:
			handler = expose;
			break;
		case FocusIn:
			handler = focus_in;
			break;
		case KeyPress:
			handler = key_press;
			break;
		case MappingNotify:
			handler = mapping_notify;
			break;
		case MapRequest:
			handler = map_request;
			break;
		case MotionNotify:
			handler = motion_notify;
			break;
		case PropertyNotify:
			handler = property_notify;
			break;
		case UnmapNotify:
			handler = unmap_notify;
			break;
		default:
			continue;
		}
		handler(&e);
	}
}

void free_monitor(Monitor *monitor) {
	Monitor *m;
	if (monitor == monitors) {
		monitors = monitors->next;
	} else {
		for (m = monitors; m && m->next != monitor; m = m->next);
		m->next = monitor->next;
	}
	XUnmapwindow(display, monitor->bar);
	XDestroyWindow(display, monitor->bar);
	free(monitor);
}

void cleanup() {
	Monitor *m;
	for (m = monitors; m; m = m->next) {
		//unmanage
	}
	XUngrabKey(display, AnyKey, AnyModifier, root);
	XFreePixmap(display, canvas.drawable);
	XFreeGC(display, canvas.gc);
	XFreeCursor(display, cursor[CURSOR_NORMAL]);
	XFreeCursor(display, cursor[CURSOR_RESIZE]);
	XFreeCursor(display, cursor[CURSOR_MOVE]);
	while (monitors) {
		free_monitor(monitors);
	}
	XSync(display, False);
	XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(display, root, atom_net[NET_ACTIVE_WINDOW]);
}

int main(int argc, char *argv[]) {
	if (argc == 2 && !strcmp("-v", argv[1])) {
		die("dwm-"VERSION);
	} else if (argc != 1) {
		die("usage: dwm [-v]\n");
	}
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale()) {
		fputs("warning: no locale supports\n", stderr);
	}
	if (!(display = XOpenDisplay(NULL))) {
		die("dwm: cannot open display\n");
	}
	prepare();
	initialize();

	run();
	cleanup();
	XCloseDisplay(display);
	return EXIT_SUCCESS;
}