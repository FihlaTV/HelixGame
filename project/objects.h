
/**************************************************/
/* header contenente la definizione della maggior */
/* parte degli oggetti mesh presenti nella scena  */
/* che non fanno parte della classe Helix         */
/**************************************************/

bool deveEssereCreato = true; // il cubo deve essere rigenerato
bool generatore = true; // devo inizializzare il seme
int pos_x, pos_y, pos_z; // traslazione del cubo

bool deveEssereCreato2 = true; // il cubo deve essere rigenerato
int pos_x2, pos_y2, pos_z2; // traslazione del cubo

bool colori = false; // il terreno è colorato o ha una textura applicata

/* caricamento degli oggetti mesh da file */
Mesh helipad((char*) "./objects/helipad.obj");
Mesh cross((char*)"./objects/cross.obj");
Mesh pista((char *)"./objects/pista.obj");
Mesh pallone1((char *)"./objects/balloon1.obj");
Mesh pallone2((char *)"./objects/balloon2.obj");

extern int punteggio;

/**************************/
/* disegna la mongolfiera */
/**************************/
void drawMongolfiera(float mozzo) {
  glPushMatrix();
    glScalef(0.07,0.07,0.07); // scalo l'oggetto
    glRotatef(0.1*mozzo,0,1,0); // l'oggetto ruota nel mondo
    glTranslatef(1330,80,0); // trasliamo l'oggetto
    glRotatef(mozzo,0,1,0); // l'oggetto ruota intorno al suo centro

    glPushMatrix();
      glColor3f(.4,.1,.1);
      glTranslatef(0,4,0);
      pallone1.RenderNxF();
    glPopMatrix();

    glPushMatrix();
      glColor3f(0.6,0.40,0.12);
      glTranslatef(0,4,0);
      pallone2.RenderNxF();
    glPopMatrix();

  glPopMatrix();
}

/********************************************************
 * function per il disengo del terreno su cui si troveranno
 * i diversi oggetti presenti nella scena
 ********************************************************/
void drawFloor() {
  const float S=100; // size
  const float H=0;   // altezza
  const int K=150; //disegna K x K quads

  // ulilizzo le coordinate OGGETTO
  // cioe' le coordnate originali, PRIMA della moltiplicazione per la ModelView
  // in modo che la texture sia "attaccata" all'oggetto, e non "proiettata" su esso
  
  if(!colori) {
    // disegno il terreno ripetendo una texture su di esso
    glBindTexture(GL_TEXTURE_2D, 5);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    //glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }

  // disegna KxK quads
  glBegin(GL_QUADS);
    glNormal3f(0,1,0); // normale verticale uguale x tutti
    for (int x=0; x<K; x++) 
      for (int z=0; z<K; z++) {
        // scelgo il colore per quel quad
        if(colori) {
          if(z%2==0 && x%2 == 0) glColor3f(0.1, 0.4, 0.1);
          else if(z%2 != 0 && x%2 !=0) glColor3f(0.1, 0.43, 0.1);
          else if(z%3 == 0 && x%3 == 0) glColor3f(0.12,0.4,0.12);
          else glColor3f(0.15,0.43,0.15);
        }
        float x0=-S + 2*(x+0)*S/K;
        float x1=-S + 2*(x+1)*S/K;
        float z0=-S + 2*(z+0)*S/K;
        float z1=-S + 2*(z+1)*S/K;
        if(!colori) glTexCoord2f(0.0, 0.0);
        glVertex3d(x0, H, z0);
        if(!colori) glTexCoord2f(1.0, 0.0);
        glVertex3d(x1, H, z0);
        if(!colori) glTexCoord2f(1.0, 1.0);
        glVertex3d(x1, H, z1);
        if(!colori) glTexCoord2f(0.0, 1.0);
        glVertex3d(x0, H, z1);
      }
  glEnd();
  glDisable(GL_TEXTURE_2D);

}

/***********************/
/* disegno della pista */
/***********************/
void drawPista () {
  glPushMatrix();
  //glColor3f(0.2,0.2,.8);
  glColor3f(0.5,0.5,.5);
  glScalef(0.75, 1.0, 0.75);
  glTranslatef(0,0.01,0);
  glTranslatef(-5,0,0);
  pista.RenderNxF();
  glPopMatrix();
}

/***********************************/
/* disegna la base dell'elicottero */
/***********************************/
void drawBase() {
  glPushMatrix();
  glScalef(-0.02,0.02,-0.02);

  glPushMatrix();
  glColor3f(.5,.5,.4);
  glTranslatef(0,5,0);
  helipad.RenderNxF();
  glPopMatrix();

  glPushMatrix();
  glColor3f(0.8,0,0);
  glTranslatef(0,4,0);
  cross.RenderNxF();
  glPopMatrix();

  glPopMatrix();
}

/*********************************************/
/* disegna due cubi con una texture personale */
/*********************************************/
void drawBox(float mozzo, Helix helix) {

  // se devo inizializzare il seme
  if(generatore){
    srand(time(NULL));
    generatore = false;
  }

  /* disegno del primo cubo */

  // se devo rigenerare il cubo
  if(deveEssereCreato) {
    pos_x = (rand()%59+1)-30;
    pos_y = (rand()%29+1);
    pos_z = (rand()%59+1)-30;
    deveEssereCreato = false;
    //printf("COORD1: %d %d %d\n", pos_x, pos_y, pos_z);
  }

  // disegno del cubo con una texture personale su tutti e sei i lati
  glPushMatrix();
  glBindTexture(GL_TEXTURE_2D, 3);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

  glTranslatef(pos_x,pos_y,pos_z);
  glTranslatef(2, 2, 2);
  glRotatef(mozzo,1,1,0);
  glTranslatef(-2, -2, -2);

  glBegin(GL_QUADS);
      /* Front. */
      glTexCoord2f(0.0, 0.0);
      glVertex3f(1.0, 1.0, 3.0);
      glTexCoord2f(1.0, 0.0);
      glVertex3f(3.0, 1.0, 3.0);
      glTexCoord2f(1.0, 1.0);
      glVertex3f(3.0, 3.0, 3.0);
      glTexCoord2f(0.0, 1.0);
      glVertex3f(1.0, 3.0, 3.0);

      /* Down. */
      glTexCoord2f(0.0, 0.0);
      glVertex3f(1.0, 1.0, 1.0);
      glTexCoord2f(1.0, 0.0);
      glVertex3f(3.0, 1.0, 1.0);  
      glTexCoord2f(1.0, 1.0);
      glVertex3f(3.0, 1.0, 3.0);
      glTexCoord2f(0.0, 1.0);
      glVertex3f(1.0, 1.0, 3.0);

      /* Back. */
      glTexCoord2f(0.0, 0.0);
      glVertex3f(1.0, 3.0, 1.0);
      glTexCoord2f(1.0, 0.0);
      glVertex3f(3.0, 3.0, 1.0);  
      glTexCoord2f(1.0, 1.0);
      glVertex3f(3.0, 1.0, 1.0);
      glTexCoord2f(0.0, 1.0);
      glVertex3f(1.0, 1.0, 1.0);

      /* Up. */
      glTexCoord2f(0.0, 0.0);
      glVertex3f(1.0, 3.0, 3.0);
      glTexCoord2f(1.0, 0.0);
      glVertex3f(3.0, 3.0, 3.0);  
      glTexCoord2f(1.0, 1.0);
      glVertex3f(3.0, 3.0, 1.0);
      glTexCoord2f(0.0, 1.0);
      glVertex3f(1.0, 3.0, 1.0);
      
      /* SideLeft. */
      glTexCoord2f(0.0, 0.0);
      glVertex3f(1.0, 3.0, 1.0);
      glTexCoord2f(1.0, 0.0);
      glVertex3f(1.0, 3.0, 3.0);  
      glTexCoord2f(1.0, 1.0);
      glVertex3f(1.0, 1.0, 3.0);
      glTexCoord2f(0.0, 1.0);
      glVertex3f(1.0, 1.0, 1.0);

      /* SideRight. */
      glTexCoord2f(0.0, 0.0);
      glVertex3f(3.0, 3.0, 1.0);
      glTexCoord2f(1.0, 0.0);
      glVertex3f(3.0, 3.0, 3.0);  
      glTexCoord2f(1.0, 1.0);
      glVertex3f(3.0, 1.0, 3.0);
      glTexCoord2f(0.0, 1.0);
      glVertex3f(3.0, 1.0, 1.0);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    glLineWidth(1);

    glColor3f(0,0,0);
    glBegin(GL_LINE_LOOP);
      glVertex3f(1.0, 1.0, 3.0);
      glVertex3f(3.0, 1.0, 3.0);
      glVertex3f(3.0, 3.0, 3.0);
      glVertex3f(1.0, 3.0, 3.0);
    glEnd();

    glBegin(GL_LINE_LOOP);
      glVertex3f(1.0, 3.0, 1.0);
      glVertex3f(3.0, 3.0, 1.0);  
      glVertex3f(3.0, 1.0, 1.0);
      glVertex3f(1.0, 1.0, 1.0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(1.0, 1.0, 3.0);
    glVertex3f(1.0, 1.0, 1.0);

    glVertex3f(3.0, 1.0, 3.0);
    glVertex3f(3.0, 1.0, 1.0);

    glVertex3f(1.0, 3.0, 3.0);
    glVertex3f(1.0, 3.0, 1.0);

    glVertex3f(3.0, 3.0, 3.0);
    glVertex3f(3.0, 3.0, 1.0);      
    glEnd();

    // se l'aereo ha catturato il cubo
    if (helix.px >= pos_x - 3 && helix.px <= pos_x + 3 &&
        helix.py >= pos_y - 3 && helix.py <= pos_y + 3 &&
        helix.pz >= pos_z - 4 && helix.pz <= pos_z + 4) {
          punteggio++;
          deveEssereCreato = true;
          // printf("Punteggio: %d\n", punteggio);
    }

glPopMatrix();

/* disegno del secondo cubo */

// il cubo appare solo quando il resto della divisione fra 
// il punteggio più uno con sei è uguale a zero
if((punteggio+1)%6 == 0) {
  glPushMatrix();

  // se devo rigenerare il cubo
    if(deveEssereCreato2) {
      pos_x2 = (rand()%59+1)-30;
      pos_y2 = (rand()%29+1);
      pos_z2 = (rand()%59+1)-30;
      deveEssereCreato2 = false;
      //printf("COORD2: %d %d %d\n", pos_x2, pos_y2, pos_z2);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 4);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

     glTranslatef(pos_x2,pos_y2,pos_z2);
     glTranslatef(-2, 2, -2);
     glRotatef(mozzo,1,1,0);
     glTranslatef(2, -2, 2);

    glBegin(GL_QUADS);
      glColor4f(0.0, 0.0, 1.0, 0.5);
        /* Front. */
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, 1.0, -3.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(-3.0, 1.0, -3.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(-3.0, 3.0, -3.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 3.0, -3.0);

        /* Down. */
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, 1.0, -1.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(-3.0, 1.0, -1.0);  
        glTexCoord2f(1.0, 1.0);
        glVertex3f(-3.0, 1.0, -3.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, -3.0);

        /* Back. */
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, 3.0, -1.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(-3.0, 3.0, -1.0);  
        glTexCoord2f(1.0, 1.0);
        glVertex3f(-3.0, 1.0, -1.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, -1.0);

        /* Up. */
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, 3.0, -3.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(-3.0, 3.0, -3.0);  
        glTexCoord2f(1.0, 1.0);
        glVertex3f(-3.0, 3.0, -1.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 3.0, -1.0);
        
        /* SideLeft. */
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-1.0, 3.0, -1.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(-1.0, 3.0, -3.0);  
        glTexCoord2f(1.0, 1.0);
        glVertex3f(-1.0, 1.0, -3.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-1.0, 1.0, -1.0);

        /* SideRight. */
        glTexCoord2f(0.0, 0.0);
        glVertex3f(-3.0, 3.0, -1.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(-3.0, 3.0, -3.0);  
        glTexCoord2f(1.0, 1.0);
        glVertex3f(-3.0, 1.0, -3.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(-3.0, 1.0, -1.0);

      glEnd();
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);

      // se l'aereo ha catturato il cubo
      if (helix.px >= pos_x2 - 3 && helix.px <= pos_x2 + 3 &&
          helix.py >= pos_y2 - 3 && helix.py <= pos_y2 + 3 &&
          helix.pz >= pos_z2 - 3 && helix.pz <= pos_z2 + 3) {
            punteggio += 2;
            deveEssereCreato2 = true;
            //printf("Punteggio: %d\n", punteggio);
      }

  glPopMatrix();
}
}