
/*********************/
/* CLASSE CONTROLLER */
/*********************/
class Controller {
public:
  enum { LEFT=0, RIGHT=1, ACC=2, DEC=3, UP=4, DOWN=5, NKEYS=6 };
  bool key[NKEYS];
  
  void Init();
  void EatKey(int keycode, int* keymap, bool pressed_or_released);
  void Joy(int keymap, bool pressed_or_released);
  Controller(){ Init(); } // costruttore
};

/****************/
/* CLASSE HELIX */
/****************/
class Helix {

  void RenderAllParts(bool usecolor, float mozzo) const; 
                         // disegna tutte le parti dell'elicottero
                         // invocato due volte: per helix e la sua ombra

public:
  // Metodi
  void Init(); // inizializza variabili
  void Render(float mozzo) const; // disegna a schermo
  void DoStep(); // computa un passo del motore fisico
  Helix(){ Init(); } // costruttore
 
  Controller controller;  
  
  // STATO DELL'ELICOTTERO
  // (DoStep fa evolvere queste variabili nel tempo)
  float px,py,pz,facing; // posizione e orientamento
  float mozzoA, mozzoP, sterzo; // stato interno
  float vx,vy,vz; // velocita' attuale
  //float volante;
  
  // STATS DELL'ELICOTTERO
  // (di solito rimangono costanti)
  float velSterzo, velRitornoSterzo, accMax, attrito,
        raggioRuotaA, raggioRuotaP, grip,
        attritoX, attritoY, attritoZ; // attriti
  float velVolante; // velocità di quota
};
