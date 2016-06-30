#include <math.h>
#include <time.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>

#include "helix.h"

#define CAMERA_BACK_HELIX 0
#define CAMERA_LATERAL_HELIX 1
#define CAMERA_TOP_HELIX 2
#define CAMERA_DOWN_HELIX 3
#define CAMERA_MOUSE 4
#define CAMERA_TYPE_MAX 5

float viewAlpha   = 20;
float viewBeta    = 40; // angoli che definiscono la vista
float eyeDist     = 5.0; // distanza dell'occhio dall'origine
bool useWireframe = false;
bool useEnvmap    = true;
bool useShadow    = true;
int cameraType    = 0;
float mozzo       = 0; // var che permette alle eliche di girare

Helix helix; // l'elicottero
int nstep = 0; // numero di passi di FISICA fatti fin'ora
const int PHYS_SAMPLING_STEP = 10; // numero di millisec che un passo di fisica simula

// Frames Per Seconds
const int fpsSampling = 3000; // lunghezza intervallo di calcolo fps
float fps = 0; // valore di fps dell'intervallo precedente
int fpsNow = 0; // quanti fotogrammi ho disegnato fin'ora nell'intervallo attuale
Uint32 timeLastInterval = 0; // quando e' cominciato l'ultimo intervallo

int punteggio = 0; // punteggio del giocatore

/* functions (e variabili) definite altrove */
extern void drawPista();
extern void drawBase();
extern void drawBox(float mozzo, Helix helix);
extern void drawFloor();
extern void drawMongolfiera(float mozzo);

/* posizioni del cubo da ricalcolare per disegno su minimappa */
extern int pos_x;
extern int pos_z;

/* il terreno è colorato o ha una texture applicata */
extern bool colori;

/* qualita del testo scritto e id della realtiva texture */
enum textquality {solid, shaded, blended};
uint font_id = -1;


/**************************************
 * Function per la scrittura di testo *
 **************************************/
void SDL_GL_DrawText(TTF_Font *font, // font
    char fgR, char fgG, char fgB, char fgA, // colore testo
    char bgR, char bgG, char bgB, char bgA, // colore background
    char *text, int x, int y, // testo e posizione
    enum textquality quality) { // qualità del testo
  SDL_Color tmpfontcolor = {fgR,fgG,fgB,fgA};
  SDL_Color tmpfontbgcolor = {bgR, bgG, bgB, bgA};
  SDL_Surface *initial;
  SDL_Surface *intermediary;
  SDL_Rect location;
  int w,h;

  /* Usiamo SDL_TTF per il rendering del testo */
  initial=NULL;
  if (quality == solid)
    initial = TTF_RenderText_Solid(font, text, tmpfontcolor);
  else if (quality == shaded)
    initial = TTF_RenderText_Shaded(font, text, tmpfontcolor, tmpfontbgcolor);
  else if (quality == blended)
    initial = TTF_RenderText_Blended(font, text, tmpfontcolor);

  /* Convertiamo il testo in un formato conosciuto */
  w = initial->w;
  h = initial->h;

  /* Allochiamo una nuova surface RGB */
  intermediary = SDL_CreateRGBSurface(0, w, h, 32,
    0x000000ff,0x0000ff00, 0x00ff0000,0xff000000);

  /* Copiamo il contenuto dalla prima alla seconda surface */
  SDL_BlitSurface(initial, 0, intermediary, 0);

  /* Informiamo GL della nuova texture */
  glBindTexture(GL_TEXTURE_2D, font_id);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, intermediary->pixels );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if ( initial != NULL ){
    location.x = x;
    location.y = y;
  }

  glLineWidth(2);
  glColor3f(0,0,0);
  glBegin(GL_LINE_LOOP);
    glVertex2f(location.x-2    , location.y-2);
    glVertex2f(location.x + w+2, location.y-2);
    glVertex2f(location.x + w+2, location.y + h+2);
    glVertex2f(location.x   -2 , location.y + h+2);
  glEnd();

  /* prepariamoci al rendering del testo */
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, font_id);
  glColor3f(1.0f, 1.0f, 1.0f);

/* Disegnamo un quads come location del testo */
  glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
      glVertex2f(location.x    , location.y);
    glTexCoord2f(1.0f, 1.0f);
      glVertex2f(location.x + w, location.y);
    glTexCoord2f(1.0f, 0.0f);
      glVertex2f(location.x + w, location.y + h);
    glTexCoord2f(0.0f, 0.0f);
      glVertex2f(location.x    , location.y + h);
  glEnd();

/* Disegnamo un contorno al quads */
  glColor3f(0.0f, 0.0f, 0.0f);
  glBegin(GL_LINE_STRIP);
    glVertex2f((GLfloat)location.x-1, (GLfloat)location.y-1);
    glVertex2f((GLfloat)location.x + w +1, (GLfloat)location.y-1);
    glVertex2f((GLfloat)location.x + w +1, (GLfloat)location.y + h +1);
    glVertex2f((GLfloat)location.x-1    , (GLfloat)location.y + h +1);
    glVertex2f((GLfloat)location.x-1, (GLfloat)location.y-1);
  glEnd();

/* Bad things happen if we delete the texture before it finishes */
  glFinish();

/* return the deltas in the unused w,h part of the rect */
  location.w = initial->w;
  location.h = initial->h;

/* Clean up */
  glDisable(GL_TEXTURE_2D);
  SDL_FreeSurface(initial);
  SDL_FreeSurface(intermediary);

}



/********************************************************
 * setta le matrici di trasformazione in modo
 * che le coordinate in spazio oggetto siano le coord 
 * del pixel sullo schemo (per disegno in 2D sopra la scena)
 ********************************************************/
void  SetCoordToPixel(int scrH, int scrW){
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(-1,-1,0);
  glScalef(2.0/scrW, 2.0/scrH, 1);
}


/********************************************************
 * function che permette il caricamento di una texture
 * da essere applicata ad un oggetto
 ********************************************************/
bool LoadTexture(int textbind,char *filename) {
  SDL_Surface *s = IMG_Load(filename);
  if (!s) return false;
  
  glBindTexture(GL_TEXTURE_2D, textbind);
  gluBuild2DMipmaps(
    GL_TEXTURE_2D, 
    GL_RGB,
    s->w, s->h, 
    GL_RGB,
    GL_UNSIGNED_BYTE,
    s->pixels
  );
  glTexParameteri(
    GL_TEXTURE_2D, 
    GL_TEXTURE_MAG_FILTER,
    GL_LINEAR ); 
  glTexParameteri(
    GL_TEXTURE_2D, 
    GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR ); 
  return true;
}


/********************************************************
 * function per il disegno di una sfera che conterrà
 * la nostra scena e il nostro mondo
 ********************************************************/
void drawSphere(double r, int lats, int longs) {
  int i, j;
  for(i = 0; i <= lats; i++) {
     double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
     double z0  = sin(lat0);
     double zr0 =  cos(lat0);
   
     double lat1 = M_PI * (-0.5 + (double) i / lats);
     double z1 = sin(lat1);
     double zr1 = cos(lat1);
    
     glBegin(GL_QUAD_STRIP);
     for(j = 0; j <= longs; j++) {
        double lng = 2 * M_PI * (double) (j - 1) / longs; // circonferenza
        double x = cos(lng);
        double y = sin(lng);
    
        // le normali servono per l'EnvMap
        glNormal3f(x * zr0, y * zr0, z0);
        glVertex3f(r * x * zr0, r * y * zr0, r * z0);
        glNormal3f(x * zr1, y * zr1, z1);
        glVertex3f(r * x * zr1, r * y * zr1, r * z1);
     }
     glEnd();
  }
}


/**************************************
 * setta la posizione della telecamera
 **************************************/
void setCamera(){

  double px = helix.px;
  double py = helix.py;
  double pz = helix.pz;
  double angle = helix.facing;
  double cosf = cos(angle*M_PI/180.0);
  double sinf = sin(angle*M_PI/180.0);
  double camd, camh, ex, ey, ez, cx, cy, cz;
  double cosff, sinff;

  // controllo la posizione della camera a seconda dell'opzione selezionata
  switch (cameraType) {
    case CAMERA_BACK_HELIX:
    camd = 15.5;
    camh = 7.0;
    ex = px + camd*sinf;
    ey = py + camh;
    ez = pz + camd*cosf;
    cx = px - camd*sinf;
    cy = py + camh;
    cz = pz - camd*cosf;
    gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
    break;
    case CAMERA_LATERAL_HELIX:
      camd = 5.0;
      camh = 2.15;
      angle = helix.facing + 40.0;
      cosff = cos(angle*M_PI/180.0);
      sinff = sin(angle*M_PI/180.0);
      ex = px + camd*sinff;
      ey = py + camh;
      ez = pz + camd*cosff;
      cx = px - camd*sinf;
      cy = py + camh;
      cz = pz - camd*cosf;
      gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
      break;
    case CAMERA_TOP_HELIX:
      camd = 5.5;
      camh = 1.0;
      ex = px + camd*sinf;
      ey = py + camh;
      ez = pz + camd*cosf;
      cx = px - camd*sinf;
      cy = py + camh;
      cz = pz - camd*cosf;
      gluLookAt(ex,ey+5,ez,cx,cy,cz,0.0,1.0,0.0);
      break;
    case CAMERA_DOWN_HELIX:
      camd = 1.0;
      camh = 0.25;
      ex = px + camd*sinf;
      ey = py + camh;
      ez = pz + camd*cosf;
      cx = px - camd*sinf;
      cy = py + camh;
      cz = pz - camd*cosf;
      gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
      break;
    case CAMERA_MOUSE:
      glTranslatef(0,0,-eyeDist);
      glRotatef(viewBeta,  1,0,0);
      glRotatef(viewAlpha, 0,1,0);
      break;
    }
}


/*******************************************
 * function per il disegno dello sfondo
 *******************************************/
void drawSky() {
int H = 100;

  if (useWireframe) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(0,0,0);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    drawSphere(100.0, 20, 20);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glColor3f(1,1,1);
    glEnable(GL_LIGHTING);
  }
  else {
    glBindTexture(GL_TEXTURE_2D,2);
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
    glColor3f(1,1,1);
    glDisable(GL_LIGHTING);

    drawSphere(100.0, 20, 20);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
  }

}


/****************************************/
/* function per disegnare una minimappa */
/****************************************/
void drawMinimap(int scrH, int scrW) {
  /* calcolo delle coordinate reali dell'oggetto su minimappa */
  float minimap_posx;
  float minimap_posz;
  minimap_posx = ((50*helix.px)/67) + 50 + 20;
  minimap_posz = ((50*helix.pz)/67) + 50 + scrH-20-100;

  float minimap_cubex;
  float minimap_cubez;
  minimap_cubex = ((50*pos_x)/67) + 50 + 20;
  minimap_cubez = ((50*pos_z)/67) + 50 + scrH-20-100;

  /* disegno del cursore */
  glColor3ub(0,0,255);
  glBegin(GL_QUADS);
    glVertex2d(minimap_posx, minimap_posz + 3);
    glVertex2d(minimap_posx + 3, minimap_posz);
    glVertex2d(minimap_posx, minimap_posz - 3);
    glVertex2d(minimap_posx - 3, minimap_posz);
  glEnd();

  /* disegno del target */
  glColor3ub(255,0,0);
  glBegin(GL_QUADS);
    glVertex2d(minimap_cubex, minimap_cubez + 3);
    glVertex2d(minimap_cubex + 3, minimap_cubez);
    glVertex2d(minimap_cubex, minimap_cubez - 3);
    glVertex2d(minimap_cubex - 3, minimap_cubez);
  glEnd();

  /* disegno minimappa */
  glColor3ub(0,0,0);
  glBegin(GL_LINE_LOOP);
    glVertex2d(20,scrH -20 -100);
    glVertex2d(20,scrH -20);
    glVertex2d(120,scrH-20);
    glVertex2d(120,scrH-20-100);
  glEnd();

  glColor3ub(210,210,210);
  glBegin(GL_POLYGON);
    glVertex2d(20,scrH -20 -100);
    glVertex2d(20,scrH -20);
    glVertex2d(120,scrH -20);
    glVertex2d(120,scrH-20-100);
   glEnd();
}


/************************************************
 * disegna un helper con una texture personale
 * di fianco alla minimappa: la texture cambia
 * quando il tempo sta per finire
 ************************************************/
void drawHelper(float tempo_attuale, int scrH, int scrW) {
  int chosen_texture;
  if((punteggio+1)%6 == 0 && tempo_attuale/CLOCKS_PER_SEC < 90)
    chosen_texture = 8;
  else if((punteggio+1)%6 == 0 && tempo_attuale/CLOCKS_PER_SEC >= 90)
    chosen_texture = 9;
  else if(tempo_attuale/CLOCKS_PER_SEC >= 90)
    chosen_texture = 7;
  else
    chosen_texture = 6;

  glPushMatrix();
  glBindTexture(GL_TEXTURE_2D, chosen_texture);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

  glBegin(GL_QUADS);
    glColor3f(1,1,0);
    glTexCoord2f(0.0, 0.0);
    glVertex2d(180,scrH -20 -50);
    glTexCoord2f(0.0, 1.0);
    glVertex2d(180,scrH -20);
    glTexCoord2f(1.0, 1.0);    
    glVertex2d(130,scrH -20);
    glTexCoord2f(1.0, 0.0);
    glVertex2d(130,scrH -20 -50);
  glEnd();

  glDisable(GL_TEXTURE_2D);
  glPopMatrix();

  glColor3f(0,0,0);
  glBegin(GL_LINE_LOOP);
    glColor3f(0,0,0);
    glVertex2d(130,scrH -20 -50);
    glVertex2d(130,scrH -20);  
    glVertex2d(180,scrH -20);
    glVertex2d(180,scrH -20 -50);
  glEnd();
}


/*********************************************
 * esegue il rendering dell'intera scena
 *********************************************/
void rendering(SDL_Window *win, double tempo_attuale, TTF_Font *font, int scrH, int scrW){
  
  // un frame in piu'!!!
  fpsNow++;
  
  // impostiamo spessore linee
  glLineWidth(3);
     
  // settiamo il viewport
  glViewport(0,0, scrW, scrH);
  
  // colore di sfondo (fuori dal mondo)
  glClearColor(1,1,1,1);

  // settiamo la matrice di proiezione
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluPerspective( 70, // fovy,
		((float)scrW) / scrH, // aspect Y/X,
		0.2, // distanza del NEAR CLIPPING PLANE in coordinate vista
		1000 // distanza del FAR CLIPPING PLANE in coordinate vista
  );

  /* passiamo a lavorare sui modelli */
  glMatrixMode( GL_MODELVIEW ); 
  glLoadIdentity();
  
  // riempe tutto lo screen buffer di pixel color sfondo
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  /* imposto diversi parametri di illuminazione per una luce */
  float tmpv[4] = {3,8,5,  1}; // ultima comp=0 => luce direzionale
                               // ultima comp=1 => i raggi escono in tutte le direzioni
  glLightfv(GL_LIGHT0, GL_POSITION, tmpv );
  
  // the ambient RGBA intensity of the light
  float tmpp[4] = {0.3,0.3,0.3,  1}; 
  glLightfv(GL_LIGHT0, GL_AMBIENT, tmpp);

  //glShadeModel(GL_FLAT);
  // GL_FLAT è il default: ogni poligono ha un colore
  // GL_SMOOTH: ogni punto della superficie del poligono ha un'ombreggiatura derivata
  //            dall'interpolazione delle normali ai vertici

  // settiamo la telecamera  
  setCamera();

  static float tmpcol[4] = {1,1,1,  1};
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpcol);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 127);
  
  glEnable(GL_LIGHTING);
  
  // disegna il cielo come sfondo
  drawSky();
  
  // disegna il suolo
  drawFloor();
  // disegna il fiume
  drawPista();
  // disegna la base dell'elicottero
  drawBase();
  // disegna la mongolfiera
  drawMongolfiera(mozzo);
  // disegna l'elicottero
  helix.Render(mozzo);
  // disegna il target
  drawBox(mozzo, helix);

  glDisable(GL_LIGHTING);
	
  // attendiamo la fine della rasterizzazione di 
  // tutte le primitive mandate 
 
  // impostiamo le matrici per poter disegnare in 2D
  SetCoordToPixel(scrH, scrW);

  // disegnamo i fps (frame x sec) come una barra a sinistra.
  // (vuota = 0 fps, piena = 100 fps)
  glBegin(GL_QUADS);
    float y = scrH*fps/100;
    float ramp = fps/100;
    glColor3f(1-ramp,0.8,ramp);
    glVertex2d(10,0);
    glVertex2d(10,y);
    glVertex2d(0,y);
    glVertex2d(0,0);
  glEnd();

  // disegnamo il tempo rimanente al giocatore come una barra
  // gialla a sinistra
  glBegin(GL_QUADS);
    float yy = 120*3 - 3*(tempo_attuale/CLOCKS_PER_SEC);
    glColor3f(1,1,0);
    glVertex2d(21,0);
    glVertex2d(21,yy);
    glVertex2d(11,yy);
    glVertex2d(11,0);
  glEnd();

  glLineWidth(2);

  // disegna la minimappa in alto a destra
  drawMinimap(scrH, scrW);

  // disegna un helper di fianco alla minimappa
  drawHelper(tempo_attuale, scrH, scrW);
  
  char str[3];
  sprintf(str, "%d", punteggio);
  char myword[] = "Punteggio: ";
  SDL_GL_DrawText(font, 0, 0, 0, 0, 210, 210, 210, 255, strcat(myword, str), scrW-150, scrH-50, shaded);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  
  glFinish();
  
  // ho finito: buffer di lavoro diventa visibile
  SDL_GL_SwapWindow(win);
}


/***************************
 * function di redraw
 ***************************/
void redraw(){
  // ci automandiamo un messaggio che
  // ci fara' ridisegnare la finestra
  SDL_Event e;
  e.type = SDL_WINDOWEVENT;
  e.window.event = SDL_WINDOWEVENT_EXPOSED;
  SDL_PushEvent(&e);
}


/*************************************
 * disegna la schermata di game over 
 *************************************/
void gameOver(SDL_Window *win, TTF_Font *font, int scrH, int scrW) {
  // settiamo il viewport
  glViewport(0,0, scrW, scrH);
  
  // colore di sfondo (fuori dal mondo)
  glClearColor(.4,0,0,1);
  
  // riempe tutto lo screen buffer di pixel color sfondo
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
 
  SetCoordToPixel(scrH, scrW);

  glLineWidth(2);
  
  // conversione della variabile punteggio
  char punti[3];
  sprintf(punti, "%d", punteggio);

  char stringa_punti[] = "Punteggio: ";
  char g_over[] = "GAME OVER";
  char continuare[] = "Premi un tasto per continuare";

  int distanzaMargine;
  if(punteggio >= 10) 
    distanzaMargine = 75;
  else 
    distanzaMargine = 73;

  SDL_GL_DrawText(font, 0, 0, 0, 0, 210, 210, 210, 255, strcat(stringa_punti, punti), scrW/2-distanzaMargine, scrH/2+100, shaded);
  SDL_GL_DrawText(font, 0, 0, 0, 0, 210, 210, 210, 255, g_over, scrW/2-80, scrH/3+20, shaded);
  SDL_GL_DrawText(font, 0, 0, 0, 0, 210, 210, 210, 255, continuare, scrW/2-150, scrH/4+20, shaded);
  glFinish();
  
  SDL_GL_SwapWindow(win);
}


/*************************************/
/*       PROGRAMMA PRINCIPALE        */
/*************************************/
int main(int argc, char* argv[]) {
  SDL_Window *win;
  SDL_GLContext mainContext;
  Uint32 windowID;
  SDL_Joystick *joystick;
  static int keymap[Controller::NKEYS] = {SDLK_a, SDLK_d, SDLK_w, SDLK_s, SDLK_q, SDLK_e};

  int scrH          = 750;
  int scrW          = 750; // altezza e larghezza viewport (in pixels)

  // inizializzazione di SDL
  SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

  if(TTF_Init() < 0) { 
    fprintf(stderr, "Impossibile inizializzare TTF: %s\n",SDL_GetError());
    SDL_Quit();
    return(2);
  }

  TTF_Font *font;
  font = TTF_OpenFont ("FreeSans.ttf", 22);
  if (font == NULL) {
    fprintf (stderr, "Impossibile caricare il font.\n");
  }

  // abilitazione joystick
  SDL_JoystickEventState(SDL_ENABLE);
  joystick = SDL_JoystickOpen(0);

  // settaggio dei buffer
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 ); 
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  // creazione di una finestra di scrW x scrH pixels
  win = SDL_CreateWindow("Helix", 0, 0, scrW, scrH, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

  // creiamo il nostro contesto OpenGL e lo colleghiamo alla nostra window
  mainContext = SDL_GL_CreateContext(win);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE); // opengl, per favore, rinormalizza le normali prima di usarle
  glFrontFace(GL_CW); // consideriamo Front Facing le facce ClockWise
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_POLYGON_OFFSET_FILL); // sposta i frammenti generati dalla
  glPolygonOffset(1,1);             // rasterizzazione poligoni
                                    // indietro di 1
  
  /* caricamento di tutte le textures necessarie nel programma */
  if (!LoadTexture(1,(char *)"./images/envmap_flipped5.jpg")) return -1;
  if (!LoadTexture(2,(char *)"./images/sky_ok.jpg")) return -1;
  if (!LoadTexture(3,(char *)"./images/cubo1.jpg")) return -1;
  if (!LoadTexture(4,(char *)"./images/cubo2.jpg")) return -1;
  if (!LoadTexture(5,(char *)"./images/erba.jpg")) return -1;
  if (!LoadTexture(6,(char *)"./images/prima.jpg")) return -1;
  if (!LoadTexture(7,(char *)"./images/seconda.jpg")) return -1;
  if (!LoadTexture(8,(char *)"./images/surprise1.jpg")) return -1;
  if (!LoadTexture(9,(char *)"./images/surprise2.jpg")) return -1;
 
  double start; // variabili per il calcolo del tempo
  double end;
  start = clock();
  double tempo_attuale;
  
  /* CICLO DEGLI EVENTI */
  bool done = 0;
  while (!done) {
    
    SDL_Event e;
    
    // guardo se c'e' un evento:
    if (SDL_PollEvent(&e)) {
    // se si: processa evento
     switch (e.type) {
      case SDL_KEYDOWN: // tasto abbassato
        helix.controller.EatKey(e.key.keysym.sym, keymap , true);
        if (e.key.keysym.sym==SDLK_F1) cameraType=(cameraType+1)%CAMERA_TYPE_MAX;
        if (e.key.keysym.sym==SDLK_F2) useWireframe=!useWireframe;
        if (e.key.keysym.sym==SDLK_F3) useEnvmap=!useEnvmap;
        if (e.key.keysym.sym==SDLK_F4) colori = !colori;
        if (e.key.keysym.sym==SDLK_F5) useShadow=!useShadow;
        break;
      case SDL_KEYUP: // tasto sollevato
        helix.controller.EatKey(e.key.keysym.sym, keymap , false);
        break;
      case SDL_QUIT: // quit
          done=1; 
          break;
      case SDL_WINDOWEVENT: // dobbiamo ridisegnare la finestra
        if (e.window.event == SDL_WINDOWEVENT_EXPOSED)
          rendering(win, tempo_attuale, font, scrH, scrW);
        else {
          windowID = SDL_GetWindowID(win);
          if (e.window.windowID == windowID) {
            switch (e.window.event)  {
              case SDL_WINDOWEVENT_SIZE_CHANGED: {
                scrW = e.window.data1;
                scrH = e.window.data2;
                glViewport(0,0,scrW,scrH);
                rendering(win, tempo_attuale, font, scrH, scrW);
                break;
              }
            }
          }
        }
        break;
        
      case SDL_MOUSEMOTION: /* movimento del mouse */
        if (e.motion.state & SDL_BUTTON(1) & cameraType == CAMERA_MOUSE) {
          viewAlpha+=e.motion.xrel;
          viewBeta +=e.motion.yrel;
          if (viewBeta<+5) viewBeta =+ 5;
          if (viewBeta>+90) viewBeta =+ 90;
        }
        break; 
       
      case SDL_MOUSEWHEEL: /* movimento della rotellina del mouse */
        if (e.wheel.y < 0 ) {
        // avvicino il punto di vista (zoom in)
          eyeDist = eyeDist*0.9;
          if (eyeDist<1) 
            eyeDist = 1;
        }
        if (e.wheel.y > 0 ) {
         // allontano il punto di vista (zoom out)
         eyeDist = eyeDist/0.9;
        }
        if(eyeDist > 96) // impedisco all'utente di uscire con la visuale dal mondo per vedere
                         // cosa c'è al di fuori (lo sfondo bianco) 
          eyeDist = 96;
        break;

      case SDL_JOYAXISMOTION: /* gestione analogico sinistro joystick */
        if( e.jaxis.axis == 0) {
          if ( e.jaxis.value < -3200 ) {
            helix.controller.Joy(0 , true);
            helix.controller.Joy(1 , false);                 
//	        printf("%d <-3200 \n",e.jaxis.value);
          }
          if ( e.jaxis.value > 3200 ) {
            helix.controller.Joy(0 , false); 
            helix.controller.Joy(1 , true);
//	        printf("%d >3200 \n",e.jaxis.value);
          }
          if ( e.jaxis.value >= -3200 && e.jaxis.value <= 3200 ) {
            helix.controller.Joy(0 , false);
            helix.controller.Joy(1 , false);                 
//	        printf("%d in [-3200,3200] \n",e.jaxis.value);
          }
        }
          
        rendering(win, tempo_attuale, font, scrH, scrW);
        break;
      
      case SDL_JOYBUTTONDOWN: /* gestione pressione di un tasto del joystick */
        printf("%d\n", e.jbutton.button);
        if ( e.jbutton.button == 7 ) // Tasto 7: l'elicottero scende
          helix.controller.Joy(4 , true);

        if ( e.jbutton.button == 6 ) // TASTO 8: l'elicottero sale
          helix.controller.Joy(5 , true);

        if ( e.jbutton.button == 2 ) // TASTO 3: accelerazione
          helix.controller.Joy(2 , true);

        if ( e.jbutton.button == 3 ) // TASTO 4: decelerazione
          helix.controller.Joy(3 , true);

        if ( e.jbutton.button == 8 ) // Tasto 9: cambiamento telecamera
          cameraType=(cameraType+1)%CAMERA_TYPE_MAX;
          if(cameraType == CAMERA_MOUSE)
            cameraType=(cameraType+1)%CAMERA_TYPE_MAX;

        if ( e.jbutton.button == 9 ) // TASTO 10: wireframe
          useWireframe=!useWireframe;

        if ( e.jbutton.button == 5 ) // TASTO 6: texture elicottero
          useEnvmap=!useEnvmap;

        if ( e.jbutton.button == 0 ) // TASTO 1: ombra
          useShadow=!useShadow;

        if ( e.jbutton.button == 4 ) // TASTO 5: texture del mondo
          colori=!colori;

        break;
      
      case SDL_JOYBUTTONUP: /* Gestione rilascio di un tasto del joystick */
        helix.controller.Joy(2 , false);
        helix.controller.Joy(3 , false);
        helix.controller.Joy(4 , false);
        helix.controller.Joy(5 , false);
        break; 
      }
    } else {
        mozzo--;
        // nessun evento: siamo IDLE
      
        Uint32 timeNow = SDL_GetTicks(); // che ore sono?
        // ritorniamo il nuemro di millisecondi da quando la libreria SDL è stata inizializzata
      
        if (timeLastInterval + fpsSampling < timeNow) {
          fps = 1000.0*((float)fpsNow) / (timeNow-timeLastInterval);
          fpsNow = 0;
          timeLastInterval = timeNow;
        }
      
        bool doneSomething = false;
        int guardia = 0; // sicurezza da loop infinito
      
        // finche' il tempo simulato e' rimasto indietro rispetto
        // al tempo reale facciamo i passi di fisica che ci mancano
        // e poi ridisegniamo la scena
        while (nstep*PHYS_SAMPLING_STEP < timeNow ) {
          helix.DoStep();
          nstep++;
          doneSomething = true;
          timeNow = SDL_GetTicks();
          guardia++;
          if (guardia > 1000) {
            done = true; 
            break;
          } // siamo troppo lenti!
        }
      
        if (doneSomething) 
          rendering(win, tempo_attuale, font, scrH, scrW);

        // dopo due minuti il gioco termina 
        end = clock();
        tempo_attuale = end - start;

        if(tempo_attuale/CLOCKS_PER_SEC >= 120){
          done = 1;
          printf("Hai totalizzato %d punti!\n", punteggio);
          
          int done1 = 0;

          // schermata di game over
          while(!done1) { 
            if (SDL_PollEvent(&e)) {
              switch (e.type) {
                case SDL_KEYDOWN: 
                  done1 = 1; 
                  break;
                case SDL_JOYBUTTONDOWN:
                  done1 = 1;
                  break;
                case SDL_WINDOWEVENT:
                  windowID = SDL_GetWindowID(win);
                  if (e.window.windowID == windowID) {
                    switch (e.window.event)  {
                      case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        scrW = e.window.data1;
                        scrH = e.window.data2;
                        glViewport(0,0,scrW,scrH);
                        break;
                      }
                    }
                  }
              }
            } else { // disegno la schermata di game over
                gameOver(win, font, scrH, scrW);
            } // fine else
          } // fine while
        } // fine if del tempo attuale

      else {
        // tempo libero!!!
      }
    }
  }

  SDL_GL_DeleteContext(mainContext);
  SDL_DestroyWindow(win);
  SDL_Quit ();
  return (0);
}

