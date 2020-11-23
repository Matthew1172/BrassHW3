/*Testcode for  220 Homework 3 Fall 2020: Exploration of a grid graph */
/* compiles with command line  gcc test.c -lX11 -lm */
/*
 *Matthew Pecko
 *Professor Brass
 *11/22/2020
 *CSc 220 Homework 3
 */
/* compiles with command line  g++ brasshw3.cpp -lX11 -lm */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>
#include <string>
using namespace std;

#define SIZE 50
#define BLOCKPERCENTAGE 47
#define FREE 1
#define BLOCKED 0

Display *display_ptr;
Screen *screen_ptr;
int screen_num;
char *display_name = NULL;
unsigned int display_width, display_height;

Window win;
int border_width;
unsigned int win_width, win_height;
int win_x, win_y;

XWMHints *wm_hints;
XClassHint *class_hints;
XSizeHints *size_hints;
XTextProperty win_name, icon_name;
char *win_name_string = "Example Window";
char *icon_name_string = "Icon for Example Window";

XEvent report;

GC gc, gc_yellow, gc_green, gc_red, gc_orange, gc_grey;
unsigned long valuemask = 0;
XGCValues gc_values, gc_yellow_values, gc_green_values,
  gc_red_values, gc_orange_values, gc_grey_values;
Colormap color_map;
XColor tmp_color1, tmp_color2;

int stepx, stepy, startx, starty;
void used_edge(int sx, int sy, int tx, int ty);
void path_edge(int sx, int sy, int tx, int ty);
void find_path(int *g, int sx, int sy, int tx, int ty);

int main(int argc, char **argv)
{ int graph[SIZE][SIZE][4]; int i,j,k;
  int sx,sy,tx,ty; /* start and target point */
  int rand_init_value;
  if( argc > 1 )
    {  sscanf(argv[1], "%d", &rand_init_value);
       srand( rand_init_value );
    }
  /* setup grid */
  for( i=0;i< SIZE; i++ )
    for( j=0; j <SIZE; j++)
      { graph[i][j][0]= FREE; graph[i][j][1] = FREE;
	graph[i][j][2]= FREE; graph[i][j][3] = FREE;
      }
  for( i=0;i< SIZE; i++ )
    { graph[i][0][3] = BLOCKED; graph[i][SIZE-1][1] = BLOCKED;
      graph[0][i][2] = BLOCKED; graph[SIZE-1][i][0] = BLOCKED;
    }
  /* Block random edges */
  for( i=0;i< SIZE; i++ )
    for( j=0; j <SIZE-1; j++)
      { if( rand()%100 < BLOCKPERCENTAGE )
	  {   graph[i][j][1]= BLOCKED; graph[i][j+1][3] = BLOCKED;
	  }
	if( rand()%100 < BLOCKPERCENTAGE )
	  {   graph[j][i][0]= BLOCKED; graph[j+1][i][2] = BLOCKED;
	  }
      }
  /* now generate start and target point */
  /* and make the edges going out FREE, and connected to the graph*/
  sx = rand()%(SIZE-2) +1;
  sy = rand()%(SIZE-2) +1; 
  graph[sx][sy][0] = FREE;   graph[sx+1][sy][2] = FREE; 
  graph[sx][sy][2] = FREE;   graph[sx-1][sy][0] = FREE;  
  graph[sx][sy][1] = FREE;   graph[sx][sy+1][3] = FREE;  
  graph[sx][sy][3] = FREE;   graph[sx][sy-1][1] = FREE;  
  tx = rand()%(SIZE-4) +1;
  ty = rand()%(SIZE-4) +1;
  graph[tx][ty][0] = FREE;   graph[tx+1][ty][2] = FREE; 
  graph[tx][ty][2] = FREE;   graph[tx-1][ty][0] = FREE;  
  graph[tx][ty][1] = FREE;   graph[tx][ty+1][3] = FREE;  
  graph[tx][ty][3] = FREE;   graph[tx][ty-1][1] = FREE;  



  /* opening display: basic connection to X Server */
  if( (display_ptr = XOpenDisplay(display_name)) == NULL )
    { printf("Could not open display. \n"); exit(-1);}
  printf("Connected to X server  %s\n", XDisplayName(display_name) );
  screen_num = DefaultScreen( display_ptr );
  screen_ptr = DefaultScreenOfDisplay( display_ptr );
  color_map  = XDefaultColormap( display_ptr, screen_num );
  display_width  = DisplayWidth( display_ptr, screen_num );
  display_height = DisplayHeight( display_ptr, screen_num );

  printf("Width %d, Height %d, Screen Number %d\n", 
           display_width, display_height, screen_num);
  /* creating the window */
  border_width = 10;
  win_x = 0; win_y = 0;
  win_height = (int) (display_height/1.3);
  win_width = win_height; /*square window*/
  
  win= XCreateSimpleWindow( display_ptr, RootWindow( display_ptr, screen_num),
                            win_x, win_y, win_width, win_height, border_width,
                            BlackPixel(display_ptr, screen_num),
                            WhitePixel(display_ptr, screen_num) );
  /* now try to put it on screen, this needs cooperation of window manager */
  size_hints = XAllocSizeHints();
  wm_hints = XAllocWMHints();
  class_hints = XAllocClassHint();
  if( size_hints == NULL || wm_hints == NULL || class_hints == NULL )
    { printf("Error allocating memory for hints. \n"); exit(-1);}

  size_hints -> flags = PPosition | PSize | PMinSize  ;
  size_hints -> min_width = 60;
  size_hints -> min_height = 60;

  XStringListToTextProperty( &win_name_string,1,&win_name);
  XStringListToTextProperty( &icon_name_string,1,&icon_name);
  
  wm_hints -> flags = StateHint | InputHint ;
  wm_hints -> initial_state = NormalState;
  wm_hints -> input = False;

  class_hints -> res_name = "x_use_example";
  class_hints -> res_class = "examples";

  XSetWMProperties( display_ptr, win, &win_name, &icon_name, argv, argc,
                    size_hints, wm_hints, class_hints );

  /* what events do we want to receive */
  XSelectInput( display_ptr, win, 
            ExposureMask | StructureNotifyMask | ButtonPressMask );
  
  /* finally: put window on screen */
  XMapWindow( display_ptr, win );

  XFlush(display_ptr);

  /* create graphics context, so that we may draw in this window */
  gc = XCreateGC( display_ptr, win, valuemask, &gc_values);
  XSetForeground( display_ptr, gc, BlackPixel( display_ptr, screen_num ) );
  XSetLineAttributes( display_ptr, gc, 4, LineSolid, CapRound, JoinRound);
  /* and some other graphics contexts, to draw in yellow and red and grey*/
  /* yellow*/
  gc_yellow = XCreateGC( display_ptr, win, valuemask, &gc_yellow_values);
  XSetLineAttributes(display_ptr, gc_yellow, 2, LineSolid,CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "yellow", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color yellow\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_yellow, tmp_color1.pixel );
  /* green */
  gc_green = XCreateGC( display_ptr, win, valuemask, &gc_green_values);
  XSetLineAttributes(display_ptr, gc_yellow, 2, LineSolid,CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "green", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color green\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_green, tmp_color1.pixel );
  /* red*/
  gc_red = XCreateGC( display_ptr, win, valuemask, &gc_red_values);
  XSetLineAttributes( display_ptr, gc_red, 2, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "red", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color red\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_red, tmp_color1.pixel );
  /* orange*/
  gc_orange = XCreateGC( display_ptr, win, valuemask, &gc_orange_values);
  XSetLineAttributes( display_ptr, gc_orange, 2, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "orange", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color orange\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_orange, tmp_color1.pixel );
  /* grey */
  gc_grey = XCreateGC( display_ptr, win, valuemask, &gc_grey_values);
  XSetLineAttributes( display_ptr, gc_grey, 3, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "dark grey", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color grey\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_grey, tmp_color1.pixel );

  /* and now it starts: the event loop */
  while(1)
    { XNextEvent( display_ptr, &report );
      switch( report.type )
      { 
        case ConfigureNotify:
          /* This event happens when the user changes the size of the window*/
          win_width = report.xconfigure.width;
          win_height = report.xconfigure.height;
          /* break; this case continues into the next:after a resize, 
             the figure gets redrawn */
	case Expose:
          /* (re-)draw the figure. This event happens
             each time some part of the window gets exposed (becomes visible) */
          XClearWindow( display_ptr, win );
          stepx = (int) (win_width/(SIZE+3));
          stepy = (int) (win_height/(SIZE+3));
	  startx = 2*stepx;
	  starty = 2*stepy;
	  /* Draw Grid Subgraph */
	  for(i = 0; i < SIZE; i++ )
	    for( j = 0; j < SIZE; j++ )
	      { /* First draw edges */
		if(graph[i][j][0] ==  FREE)
		    XDrawLine(display_ptr, win, gc_grey,
			    startx + i*stepx + (int) (0.33*stepx),
			    starty + j*stepy + (int) (0.33*stepy),
                            startx + (i+1)*stepx + (int) (0.33*stepx),
			    starty + j*stepy + (int) (0.33*stepy) );
		if(graph[i][j][1] ==  FREE)
		    XDrawLine(display_ptr, win, gc_grey,
			    startx + i*stepx + (int) (0.33*stepx),
			    starty + j*stepy + (int) (0.33*stepy),
                            startx + i*stepx + (int) (0.33*stepx),
			    starty + (j+1)*stepy + (int) (0.33*stepy) );
		if(graph[i][j][2] ==  FREE)
		    XDrawLine(display_ptr, win, gc_grey,
			    startx + i*stepx + (int) (0.33*stepx),
			    starty + j*stepy + (int) (0.33*stepy),
                            startx + (i-1)*stepx + (int) (0.33*stepx),
			    starty + j*stepy + (int) (0.33*stepy) );
		if(graph[i][j][3] ==  FREE)
		    XDrawLine(display_ptr, win, gc_grey,
			    startx + i*stepx + (int) (0.33*stepx),
			    starty + j*stepy + (int) (0.33*stepy),
                            startx + i*stepx + (int) (0.33*stepx),
			    starty + (j-1)*stepy + (int) (0.33*stepy) );
	      }
	  /* now draw vertex */
	  for(i = 0; i < SIZE; i++ )
	    for( j = 0; j < SIZE; j++ )
		XFillArc( display_ptr, win, gc, /*black*/
		       startx+ i*stepx, starty+ j*stepy, /*upper left corner */
		       (int) (0.66*stepx), (int) (0.66*stepy), 0, 360*64);
                

	  /* Draw Start and Target point yellow: before call of function*/
	  XFillArc( display_ptr, win, gc_red, /*red*/
	       startx+ sx*stepx, starty+ sy*stepy, 
	       (int) (0.66*stepx), (int) (0.66*stepy), 0, 360*64);
	  XFillArc( display_ptr, win, gc_red, /*red*/
	       startx+ tx*stepx, starty+ ty*stepy, 
	       (int) (0.66*stepx), (int) (0.66*stepy), 0, 360*64);

          /* Now call function to draw shortest path */
	  find_path(&(graph[0][0][0]), sx,sy,tx,ty);
	  /* Draw Start and Target point red, after function did its work*/
	  XFillArc( display_ptr, win, gc_red, /*red*/
	       startx+ sx*stepx, starty+ sy*stepy, 
	       (int) (0.66*stepx), (int) (0.66*stepy), 0, 360*64);
	  XFillArc( display_ptr, win, gc_red, /*red*/
	       startx+ tx*stepx, starty+ ty*stepy, 
	       (int) (0.66*stepx), (int) (0.66*stepy), 0, 360*64);

          break;
         default:
	  /* this is a catch-all for other events; it does not do anything.
             One could look at the report type to see what the event was */ 
          break;
	}

    }
  exit(0);
}

void used_edge(int sx, int sy, int tx, int ty)
{ if( abs(sx-tx) + abs(sy-ty) != 1)
    printf(" called used_edge to connect non-neighbors (%d,%d) and (%d,%d)\n", sx,sy, tx, ty);
  else
    XDrawLine(display_ptr, win, gc_green,
	      startx + sx*stepx + (int) (0.33*stepx),
	      starty + sy*stepy + (int) (0.33*stepy),
              startx + tx*stepx + (int) (0.33*stepx),
	      starty + ty*stepy + (int) (0.33*stepy) );
}
void path_edge(int sx, int sy, int tx, int ty)
{ if( abs(sx-tx) + abs(sy-ty) != 1)
    printf(" called path_edge to connect non-neighbors (%d,%d) and (%d,%d)\n", sx,sy, tx, ty);
  else
    XDrawLine(display_ptr, win, gc_orange,
	      startx + sx*stepx + (int) (0.33*stepx),
	      starty + sy*stepy + (int) (0.33*stepy),
              startx + tx*stepx + (int) (0.33*stepx),
	      starty + ty*stepy + (int) (0.33*stepy) );
}
    
class Node{
private:
	int x;
	int y;
	string id;
public:
	Node(int x, int y){
		this->x = x;
		this->y = y;
		this->id = to_string(x)+","+to_string(y);
	}
	Node(const Node &n){
		this->x = n.x;
		this->y = n.y;
		this->id = n.id;
	}
	int getX(){ return this->x; }
	int getY(){ return this->y; }
	string getId(){ return this->id; }
};

void find_path(int *g, int sx, int sy, int tx, int ty)
{   
queue<Node*> q;
unordered_map<string, Node*> r;

bool visited[SIZE][SIZE] = {false};

Node* n = new Node(sx, sy);
q.push(n);

int row;
int col;

while(!q.empty()){
	Node* x = new Node(*q.front());
	q.pop();
	row = x->getX();
	col = x->getY();
	if(visited[row][col] == false){
		visited[row][col] = true;
	
		if(g[row*(SIZE*4)+col*4+0] == FREE) {
			used_edge(row, col, row+1, col);
			q.push(new Node(row+1, col));
			if(visited[row+1][col] == false) r[to_string(row+1)+","+to_string(col)] = x;
		}
		if(g[row*(SIZE*4)+col*4+1] == FREE) {
			used_edge(row, col, row, col+1);
			q.push(new Node(row, col+1));
			if(visited[row][col+1] == false) r[to_string(row)+","+to_string(col+1)] = x;
		}
		if(g[row*(SIZE*4)+col*4+2] == FREE) {
			used_edge(row, col, row-1, col);
			q.push(new Node(row-1, col));
			if(visited[row-1][col] == false) r[to_string(row-1)+","+to_string(col)] = x;
		}
		if(g[row*(SIZE*4)+col*4+3] == FREE) {
			used_edge(row, col, row, col-1);
			q.push(new Node(row, col-1));
			if(visited[row][col-1] == false) r[to_string(row)+","+to_string(col-1)] = x;
		}
	}
}

string a = to_string(tx)+","+to_string(ty);
int b1, b2;
while(a != to_string(sx)+","+to_string(sy)){
	Node* myn = r[a];
	b1 = stoi(a.substr(0, a.find(",")));
	b2 = stoi(a.substr(a.find(",")+1, a.size()-1));
	path_edge(myn->getX(), myn->getY(), b1, b2);
	a = to_string(myn->getX())+","+to_string(myn->getY());
}


}
