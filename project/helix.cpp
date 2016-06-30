// helix.cpp
// implementazione dei metodi definiti in helix.h

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <vector>

#include <stdlib.h>
#include <time.h>

#include "helix.h"
#include "point3.h"
#include "mesh.h"
#include "objects.h"

// variabili globali di tipo mesh
Mesh carlinga((char *)"./objects/elicottero_carlinga_vera.obj");
Mesh elica((char *)"./objects/elicottero_elica.obj");
Mesh elicaPR((char *)"./objects/elicottero_elica2.obj");
Mesh finestre((char *)"./objects/elicottero_finestre.obj");

extern bool useEnvmap; // var globale esterna: per usare l'evnrionment mapping
extern bool useShadow; // var globale esterna: per generare l'ombra
extern int punteggio; // punteggio del giocatore

/*****************************************************************************/
/* da invocare quando e' stato premuto/rilasciato il tasto numero "keycode"  */
/*****************************************************************************/
void Controller::EatKey(int keycode, int* keymap, bool pressed_or_released) {
  for (int i=0; i<NKEYS; i++) {
    if (keycode == keymap[i]) 
      key[i] = pressed_or_released;
  }
}

/*************************************************************/
/* da invocare quando e' stato premuto/rilasciato un jbutton */
/*************************************************************/
void Controller::Joy(int keymap, bool pressed_or_released) {
    key[keymap] = pressed_or_released;
}

/***************************************************/
/* funzione che prepara tutto per usare un env map */
/***************************************************/
void SetupEnvmapTexture() {
  // facciamo binding con la texture 1
  glBindTexture(GL_TEXTURE_2D, 1);
   
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
  glEnable(GL_TEXTURE_GEN_T);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP); // Env map
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
  glDisable(GL_LIGHTING); // disabilito il lighting OpenGL standard (lo faccio con la texture)
}

/***************************************************************/
// DoStep: facciamo un passo di fisica (a delta_t costante)
// Indipendente dal rendering.
// Ricordiamoci che possiamo LEGGERE ma mai SCRIVERE
// la struttura controller da DoStep
/***************************************************************/
void Helix::DoStep(){
  // computiamo l'evolversi dell'elicottero
  static int i=5;
  
  float vxm, vym, vzm; // velocita' in spazio elicottero
  
  // da vel frame mondo a vel frame elicottero
  float cosf = cos(facing*M_PI/180.0);
  float sinf = sin(facing*M_PI/180.0);
  vxm = +cosf*vx - sinf*vz;
  vym = vy;
  vzm = +sinf*vx + cosf*vz;
  
  // gestione dello sterzo
  if (controller.key[Controller::LEFT]) sterzo += velSterzo;
  if (controller.key[Controller::RIGHT]) sterzo -= velSterzo;
  if (controller.key[Controller::UP]) py += velVolante;
  if (controller.key[Controller::DOWN]) py -= velVolante;
  if (py < 0) py = 0; // l'elicottero non sprofonda nel terreno

  sterzo*=velRitornoSterzo; // ritorno a volante dritto
  
  if (controller.key[Controller::ACC]) vzm-=accMax; // accelerazione in avanti 
  if (controller.key[Controller::DEC]) vzm+=accMax; // accelerazione indietro
  if(py == 0) vzm = 0; // se l'elicottero è a terra non può muoversi
  
  // attirti (semplificando)
  vxm*=attritoX;  
  vym*=attritoY;
  vzm*=attritoZ;
  
  // l'orientamento dell'elicottero' segue quello dello sterzo
  // (a seconda della velocita' sulla z)
  facing = facing - (vzm*grip)*sterzo;
  
  // ritorno a vel coord mondo
  vx = +cosf*vxm + sinf*vzm;
  vy = vym;
  vz = -sinf*vxm + cosf*vzm;
  
  // posizione = posizione + velocita * delta t (ma delta t e' costante)
  px+=vx;
  py+=vy;
  pz+=vz;

  /* imposto i limiti del mondo di gioco */
  if(pz >= 61) pz = 61;
  if(pz <= -61) pz = -61;
  if(px >= 61) px = 61;
  if(px <= -61) px = -61;
  if(py >= 45) py = 45;

}

/***********************/
/* init del controller */
/***********************/
void Controller::Init(){
  for (int i=0; i<NKEYS; i++) 
    key[i] = false;
}


/***************************/
/* init della classe Helix */
/***************************/
void Helix::Init(){
  // inizializzo lo stato dell'elicottero
  
  // posizione e orientamento
  px = 0;
  py = 0;
  pz = 0;
  facing = 0;
  
  // stato
  sterzo = 0;

  // velocita' attuale
  vx = 0;
  vy = 0;
  vz = 0;
  
  // inizializzo la struttura di controllo
  controller.Init();
  
  velSterzo = 2.4;         // A
  velRitornoSterzo = 0.93; // B, sterzo massimo = A*B / (1-B)
  
  velVolante = 0.04;

  accMax = 0.0011;
  
  // attriti: percentuale di velocita' che viene mantenuta
  // 1 = no attrito
  // <<1 = attrito grande
  attritoZ = 0.991;  // piccolo attrito sulla Z (nel senso di rotolamento delle ruote)
  attritoX = 0.8;  // grande attrito sulla X (per evitare slittamento)
  attritoY = 1.0;  // attrito sulla y nullo
  
  // Nota: vel max = accMax*attritoZ / (1-attritoZ)
  
  raggioRuotaA = 0.25;
  raggioRuotaP = 0.35;
  
  grip = 0.45;
}

/***************************************************************/
// funzione che disegna tutti i pezzi dell'elicottero
// (carlinga + eliche + finestre e base)
// (da invocarsi due volte: per l'elicottero, e per la sua ombra)
// (se usecolor e' falso, NON sovrascrive il colore corrente
//  e usa quello stabilito prima di chiamare la funzione)
/***************************************************************/
void Helix::RenderAllParts(bool usecolor, float mozzo) const{
  
  // disegna la carliga con una mesh
  glPushMatrix();
    // patch: riscaliamo la mesh di 1/10 
    glScalef(-0.02,0.02,-0.02);

    glPushMatrix();
      if(usecolor) glColor3f(.1,.1,.1);
      glRotatef(25*mozzo,0,1,0);
      elica.RenderNxF();
    glPopMatrix();

    glPushMatrix();
      if(usecolor) glColor3f(.1,.1,.1);
      glTranslatef(0, +elicaPR.Center().Y(), +elicaPR.Center().Z());
      glRotatef(20*mozzo,1,0,0);
      glTranslatef(0, -elicaPR.Center().Y(), -elicaPR.Center().Z() );
      elicaPR.RenderNxF();
    glPopMatrix();
  
    glPushMatrix();
      if(usecolor) glColor3f(.9,.9,.9);
      finestre.RenderNxF();
    glPopMatrix();

    if (!useEnvmap) {
      if (usecolor)
        glColor3f(0,0,0.40);
    }
    else {
      if (usecolor)
        SetupEnvmapTexture();
    }

    glPushMatrix();
      carlinga.RenderNxV();
      glDisable(GL_TEXTURE_2D);
      if (usecolor) 
        glEnable(GL_LIGHTING);
    glPopMatrix();

  glPopMatrix(); 
}

/*********************/
/* disegna a schermo */
/*********************/
void Helix::Render(float mozzo) const{
  // sono nello spazio mondo
  
  glPushMatrix();
     
    glTranslatef(px,py,pz);
    glRotatef(facing, 0,1,0);

    // sono nello spazio elicottero

    RenderAllParts(true, mozzo); 
    
    // ombra!
    if(useShadow) {
      glTranslatef(0,-py,0);
      glScalef(1-py/50,1-py/50,1-py/50);
      glColor3f(0.4,0.4,0.4); // colore fisso
      glTranslatef(0,0.01,0); // alzo l'ombra di un epsilon per evitare z-fighting con il pavimento
      glScalef(1.01,0,1.01);  // appiattisco sulla Y, ingrandisco dell'1% sulla Z e sulla X 
      glDisable(GL_LIGHTING); // niente lighing per l'ombra
      RenderAllParts(false, mozzo);  // disegno l'elicottero appiattito

      glEnable(GL_LIGHTING);
    }
    glPopMatrix(); 
  
  glPopMatrix();
}
