#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <iostream>
#include <list>
#include <sys/time.h>
#include <math.h>

using namespace std;

const int Border = 5;
const int BufferSize = 10;
const int FPS = 30;

struct XInfo {
	Display *display;
	int     screen;
	Window  window;
	GC      gc[1];
};



class Displayable {
    public:
        virtual void paint(XInfo &xinfo) = 0;
};

class Heli : public Displayable {
    public:
        virtual void paint(XInfo &xinfo) {
            XFillArc( xinfo.display, xinfo.window, xinfo.gc[0], x, y, width, height, 0, 360*64 );
        }
    
        int getX (){
            return x;
        }
        int getY (){
            return Y;
        }
        int getXV (){
            return xv;
        }
        int getYV (){
            return yv;
        }
    
        void changeV(char var, int offset, bool decelerate){
            if ( var == 'x' ){
                if ( decelerate ){
                    if ( xv < 0 ){
                        xv += abs(offset);
                    }else if ( xv > 0 ){
                        xv -= abs(offset);
                    }
                }else{
                    xv += offset;
                }
            }
            if ( var == 'y' ){
                if ( decelerate ){
                    if ( yv < 0 ){
                        yv += abs(offset);
                    }else if ( yv > 0 ){
                        yv -= abs(offset);
                    }
                }else{
                    yv += offset;
                }
            }
        }
    
        void move( XInfo &xinfo ) {
            int vlimit = 7;
            if ( xv > vlimit ){
                xv = vlimit;
            }else if ( xv < 0 - vlimit ){
                xv = 0 - vlimit;
            }
            if ( yv > vlimit ){
                yv = vlimit;
            }else if ( yv < 0 - vlimit){
                yv = 0 - vlimit;
            }
            x += xv;
            y += yv;
            if ( x < 0 ){
                x = 0;
                xv = 0;
            }else if ( x > 800 - width){
                x = 800 - width;
                xv = 0;
            }
            if ( y < 0 ){
                y = 0;
                yv = 0;
            }else if ( y > 600 - height){
                y = 600 - height;
                yv = 0;
            }
        }
    
        Heli(int x, int y, int width, int height, int xv, int yv):x(x), y(y), width(width), height(height), xv(xv), yv(yv){}
        
    private:
        int x;
        int y;
        int xv;
        int yv;
        int width;
        int height;
};

class Bomb : public Displayable {
public:
    virtual void paint(XInfo &xinfo) {
        XFillArc(xinfo.display, xinfo.window, xinfo.gc[0], x, y, diameter, diameter, 0, 360*64);
    }
    
    void move(XInfo &xinfo, int xv) {
        x = x + xv;
        y = y + 5;
    }
    
    Bomb(int x, int y, int diameter): x(x), y(y), diameter(diameter) {
        direction = 4;
    }
	
private:
    int x;
    int y;

};


list<Displayable *> dList;           // list of Displayables
Heli heli( 30, 30, 30, 30, 0, 0 );

void initX ( int argc, char *argv[], XInfo &xInfo ) {
    XSizeHints hints;
    unsigned long white, black;
    
    xInfo.display = XOpenDisplay ( "" );
    if ( !xInfo.display ) {
        cerr << "!open" <<endl;
        exit ( 0 );
    }
    
    xInfo.screen = DefaultScreen ( xInfo.display );
    white = XWhitePixel( xInfo.display, xInfo.screen );
	black = XBlackPixel( xInfo.display, xInfo.screen );
	hints.x = 50;
	hints.y = 50;
	hints.width = 800;
	hints.height = 600;
	hints.flags = PPosition | PSize;
    
    xInfo.window = XCreateSimpleWindow(
                                       xInfo.display,
                                       DefaultRootWindow( xInfo.display ),
                                       hints.x, hints.y,
                                       hints.width, hints.height,
                                       Border,
                                       black, white );
    
	XSetStandardProperties(
                           xInfo.display,
                           xInfo.window,
                           "The Helicopter Game",
                           "HeliGame",
                           None,
                           argv, argc,
                           &hints );
    
    int i = 0;
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground      (xInfo.display, xInfo.gc[i], BlackPixel(xInfo.display, xInfo.screen));
	XSetBackground      (xInfo.display, xInfo.gc[i], WhitePixel(xInfo.display, xInfo.screen));
	XSetFillStyle       (xInfo.display, xInfo.gc[i], FillSolid);
	XSetLineAttributes  (xInfo.display, xInfo.gc[i], 1, LineSolid, CapButt, JoinMiter);
    
    XSelectInput(xInfo.display, xInfo.window, KeyPressMask | KeyReleaseMask );
    
    XMapRaised( xInfo.display, xInfo.window );
	
	XFlush(xInfo.display);
	//sleep(2);
}

void repaint( XInfo &xinfo) {
	list<Displayable *>::const_iterator begin = dList.begin();
	list<Displayable *>::const_iterator end = dList.end();
    
	XClearWindow( xinfo.display, xinfo.window );
	
	XWindowAttributes windowInfo;
	XGetWindowAttributes(xinfo.display, xinfo.window, &windowInfo);
	unsigned int height = windowInfo.height;
	unsigned int width = windowInfo.width;
    
	//XFillRectangle(xinfo.display, xinfo.window, xinfo.gc[0], 0, 0, width, height);
	while( begin != end ) {
		Displayable *d = *begin;
		d->paint(xinfo);
		begin++;
	}
	XFlush( xinfo.display );
}

void handleKeyPress(XInfo &xinfo, XEvent &event) {
	KeySym key;
	char text[BufferSize];
	
	int i = XLookupString(
                          (XKeyEvent *)&event, 	// the keyboard event
                          text, 					// buffer when text will be written
                          BufferSize, 			// size of the text buffer
                          &key, 					// workstation-independent key symbol
                          NULL );					// pointer to a composeStatus structure (unused)
	switch ( key ) {
        case XK_Left:
            heli.changeV('x', -1, false);
            break;
        case XK_Right:
            heli.changeV('x', 1, false);
            break;
        case XK_Up:
            heli.changeV('y', -1, false);
            break;
        case XK_Down:
            heli.changeV('y', 1, false);
            break;
    }
}

void handleAnimation(XInfo &xinfo) {
	heli.move(xinfo);
	
}

unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void eventLoop(XInfo &xinfo) {
	// Add stuff to paint to the display list
	dList.push_front(&heli);
	
	XEvent event;
	unsigned long lastRepaint = 0;
    unsigned int counter = 0;
    
	while( true ) {
		if (XPending(xinfo.display) > 0) {
			XNextEvent( xinfo.display, &event );
			switch( event.type ) {
				case KeyPress:
                //case KeyRelease:
					handleKeyPress(xinfo, event);
					break;
			}
		}
		
		unsigned long end = now();
        
		if (end - lastRepaint > 1000000/FPS) {//30Hz
            handleAnimation(xinfo);
			repaint(xinfo);
			lastRepaint = now();
            counter++;
            if (counter >= FPS/5){
                counter = 0;
                heli.changeV('x', 1, true);
                heli.changeV('y', 1, true);
            }
		}
		if (XPending(xinfo.display) == 0) {
			usleep(1000000/FPS - (end - lastRepaint));
		}
	}
}



int main ( int argc, char *argv[] ) {
	XInfo xInfo;
    
	initX ( argc, argv, xInfo );
    
    eventLoop(xInfo);
    
	XCloseDisplay(xInfo.display);
}
