// Déclaration des bibliotèques (en sachant que la majorité ont étaient modifiées)
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Led.h>
#include <SoftwareSerial.h>
#include <Adafruit_BME280.h>

// Déclaration des constantes
#define Nb_capteur 4
#define PRESSURE_MIN_ALARME 850
#define PRESSURE_MAX_ALARME 1080

// Création et déclaration des deux structures de données en global
typedef struct Capteur Capteur;
struct Capteur{
  int Valeur;
  boolean Erreur;
};
Capteur Capteur_L_TPH[Nb_capteur];

typedef struct GPS GPS;
struct GPS{
  boolean Valide;
  float Latitude;
  char NS;
  float Longitude;
  char EO;
  boolean GPS_eco;
};
GPS Capteur_GPS;

// Définition et réception des variables reçus par l'eeprom
#define LOGINTERVAL (EEPROM.get(1, (Capteur_L_TPH[0].Valeur))) //10
#define MAX_SIZE (EEPROM.get(3, (Capteur_L_TPH[0].Valeur))) //2000
#define TIMEOUT (EEPROM.get(5, (Capteur_L_TPH[0].Valeur)))
#define LUMIN (EEPROM.get(7, (Capteur_L_TPH[0].Valeur)))
#define LUMIN_LOW (EEPROM.get(9, (Capteur_L_TPH[0].Valeur)))//255
#define LUMIN_HIGH (EEPROM.get(11, (Capteur_L_TPH[0].Valeur)))//768
#define TEMP_AIR (EEPROM.get(13, (Capteur_L_TPH[0].Valeur)))
#define MIN_TEMP_AIR (EEPROM.get(15, (Capteur_L_TPH[0].Valeur)))//-10
#define MAX_TEMP_AIR (EEPROM.get(17, (Capteur_L_TPH[0].Valeur)))//60
#define HYGR (EEPROM.get(19, (Capteur_L_TPH[0].Valeur)))
#define HYGR_MINT (EEPROM.put(21, (Capteur_L_TPH[0].Valeur)))//0
#define HYGR_MAXT (EEPROM.get(23, (Capteur_L_TPH[0].Valeur)))//50
#define PRESSURE (EEPROM.get(25, (Capteur_L_TPH[0].Valeur)))
#define PRESSURE_MIN (EEPROM.get(27, (Capteur_L_TPH[0].Valeur)))//850
#define PRESSURE_MAX (EEPROM.get(29, (Capteur_L_TPH[0].Valeur)))//1080


// Déclation des diversses variables globales
DateTime horloge;
SoftwareSerial GPS_serial(4,5); // redefinir les pins pour le GPS

boolean Mode_eco = false;

// variables globales utilisées par les interruptions mise en "volatile" par mesure de sécurité
volatile boolean Tab_refus[7] = {false, false, false, false, false, false, false};
volatile byte varCompteur1 = 0;
volatile byte varCompteur2 = 0;
volatile boolean boutonR = false;



// EEPROM -------------------------------------------------------------------------------------------------------------------------

// Fonction pour mettre les valeurs par défaut dans l'EEPROM
void reset(){
  EEPROM.write(0, 1);
  EEPROM.write(1, 10);
  EEPROM.put(3, 4096);
  EEPROM.write(5, 30);
  EEPROM.write(7, 1);
  EEPROM.write(9, 255);
  EEPROM.put(11, 768);
  EEPROM.write(13, 1);
  EEPROM.write(15, -10);
  EEPROM.write(17, 60);
  EEPROM.write(19, 1);
  EEPROM.write(21, 0);
  EEPROM.write(23, 50);
  EEPROM.write(25, 1);
  EEPROM.put(27, 850);
  EEPROM.put(29, 1080);
}

// Fonction pour trouver l'adresse de la valeur pour la commande rentrée dans le moniteur série
void Trouver(byte* adresse,String decoupe){
    if (decoupe == "CLOCK="){
      *adresse=-2;
 
    }
    else if (decoupe == "DATE=")
     *adresse=-3;

   }
   else if (decoupe == "LOG_INTERVALL="){
    *adresse = 1;

  }
  else if (decoupe == "FILE_MAX_SIZE="){
    *adresse = 3;

  }
  else if (decoupe == "TIMEOUT="){
    *adresse = 5;

  }
  else if (decoupe == "LUMIN="){
    *adresse = 7;
  
  }else if (decoupe == "LUMIN_LOW="){
    *adresse = 9;

  }
  else if (decoupe == "LUMIN_HIGH="){
    *adresse = 11;

  }
  else if (decoupe == "TEMP_AIR="){
    *adresse = 13;
  
  }
  else if (decoupe == "MIN_TEMP_AIR="){
    *adresse = 15;

  }
  else if (decoupe == "MAX_TEMP_AIR="){
    *adresse = 17;

  }
  else if (decoupe == "HYGR="){
   *adresse = 19;

  }
  else if (decoupe == "HYGR_MINT="){
   *adresse = 21;

  }
  else if (decoupe == "HYGR_MAXT="){
   *adresse = 23;

  }
  else if (decoupe == "PRESSURE="){
   *adresse = 25;

  }
  else if (decoupe == "PRESSURE_MIN="){
   *adresse = 27;

  }
  else if (decoupe == "PRESSURE_MAX="){
   *adresse = 29;
  
  }
}



// Découpe du message envoyé dans le moniteur série
void Message(String* message){
  String decoupe="";
  byte adresse=-1;
  for(unsigned int i=0; i<(*message).length(); i++) {
    decoupe += (*(message))[i];
    if((*(message))[i] == '='){
        Trouver(&adresse,decoupe); // Recherche de l'adresse où est stockée la valeur de la commande demandée dans l'EEPROM
        decoupe = "";
       
    }
 }
 DateTime tempo = nowRTC();
  switch (adresse){
    case -2: // Si la commande est "CLOCK"
        if (isDigit(decoupe[0]) && isDigit(decoupe[1]) && isDigit(decoupe[3]) && isDigit(decoupe[4]) && isDigit(decoupe[6]) && isDigit(decoupe[7])){
          adjust(DateTime(tempo.year(), tempo.month(), tempo.day(), 10*(decoupe[0]-'0')+(decoupe[1]-'0'), 10*(decoupe[3]-'0')+(decoupe[4]-'0'), 10*(decoupe[6]-'0')+(decoupe[7]-'0')));
        }
      break;
    case -3: // Si la commande est "DATE"
        if (isDigit(decoupe[0]) && isDigit(decoupe[1]) && isDigit(decoupe[3]) && isDigit(decoupe[4]) && isDigit(decoupe[6]) && isDigit(decoupe[7]) && isDigit(decoupe[8]) && isDigit(decoupe[9])){
          adjust(DateTime(  1000*(decoupe[6]-'0')+100*(decoupe[7]-'0')+10*(decoupe[8]-'0')+(decoupe[9]-'0')  , 10*(decoupe[3]-'0')+(decoupe[4]-'0'), 10*(decoupe[0]-'0')+(decoupe[1]-'0') , tempo.hour(), tempo.minute(), tempo.second()));
        }
      break;
    default: // Pour les autres commandes : Enregistrement de la valeur dans l'EEPROM a l'adresse recherchée plus tôt
      EEPROM.put(adresse, int(decoupe.toInt()));
  }
}


// Fonction d'analyse du message rentré dans le moniteur série
void Configuration(){
  String message="";
  int t;
  if(!Serial.available()){
    return;
  }
  while (Serial.available()){
    message += char(Serial.read());
    delayMicroseconds(2000);
  }
  if (message == "RESET"){ // Si la commande est "RESET"
    reset();
  }
  else if (message == "VERSION"){ // Si la commande est "VERSION"
    Serial.println("1.0");
  }
  else{ // Si c'est un tout autre message
    Message(&message);
  }
}

// ################################################################################################################################

// INTERRUPTION -------------------------------------------------------------------------------------------------------------------
void Changement_LED(){
  if (Tab_refus[6]){ // Tempete/Cyclone ****************************************
    setColorRGB(255, 0, 0); // Led = rouge
  }
  else if (Tab_refus[5]){ // Erreur horloge ************************************
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0); // Led = rouge
    } 
    else{
      setColorRGB(0, 0, 255); // Led = bleu
    }
  }
  else if (Tab_refus[4]){// Erreur d'ecriture de la carte SD ******************
    if (varCompteur1 < 42){
      setColorRGB(255, 0, 0); // Led = rouge
    } 
    else{
      setColorRGB(255, 255, 255); // Led = blanche
    }
  }
  else if (Tab_refus[3]){ // Erreur de GPS *************************************
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0); // Led = rouge
    } 
    else{
      setColorRGB(255, 255, 0); // Led = jaune
    }
  }
  else if (Tab_refus[2]){ // Erreur d'accees aux capteurs **********************
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0); // Led = rouge
    } 
    else{
      setColorRGB(0, 255, 0); // Led = vert
    }
  }
  else if (Tab_refus[1]){ // Erreur de donnée incoerante ou en dehors des valeurs definits
    if (varCompteur1 < 42){
      setColorRGB(255, 0, 0); // Led = rouge
    } 
    else{
      setColorRGB(0, 255, 0); // Led  = vert
    }
  }
  else if (Tab_refus[0]){ // Carte SD pleine ***********************************
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0); // Led = rouge
    } 
    else{
      setColorRGB(255, 255, 255); // Led = blanche
    }  
  }
  
  else if (Mode_eco){
    setColorRGB(0, 0, 255); // Mode eco : Led = bleu
  } 
  else{
    setColorRGB(0, 255, 0); // Mode standard : Led = vert
  }
}



void Interruption(){
  if(!digitalRead(2) || !digitalRead(3)){ // Si boutons appuyer on active le timer de 5 secondes
    TIMSK1 = 0b00000010;

  }
  else{
    if(digitalRead(2) && digitalRead(3)){// Si boutons lacher reinitialiser timer de 5 secondes
      TIMSK1 = 0b00000000;
      varCompteur2 = 0;

    }
  }
}



ISR(TIMER2_OVF_vect){
  TCNT2 = 256 - 125;
  if(varCompteur1++ > 125){//Timers pour le clignotement de la led en cas d'erreurs
    varCompteur1 = 0;
  }
  Changement_LED();
}

ISR(TIMER1_COMPA_vect){
  if(varCompteur2<5){
    varCompteur2++; //Compte des secondes de pression des boutons 
  }
  else if(varCompteur2 == 5){
     varCompteur2++;
    /*if(digitalRead(2)== LOW && digitalRead(3)==LOW){
//Decommenter pour 2 bouton 
    }
    else */if(digitalRead(2)== LOW){
      boutonR = !boutonR; // Pour determiner si on rentre ou sors de la maintenance
      interrupts();
      TIMSK2 = 0b00000000;//Desactivation clignotements Led
      while (boutonR){
      // fonction de  Maintenance()
      setColorRGB(230, 105, 0);
    }
    TIMSK2 = 0b00000001;//Reactivation Led
    }
    else{
      Mode_eco = !(Mode_eco);
    }

  }
}

// ################################################################################################################################

// INITIALISATION -----------------------------------------------------------------------------------------------------------------
void setup_Interruption(){
  // bouton sur les pins 2 et 3
  pinMode(0,INPUT);
  pinMode(1,INPUT);
  attachInterrupt(0,Interruption,CHANGE);
  attachInterrupt(1,Interruption,CHANGE);
  cli();
  bitClear (TCCR2A, WGM20);
  bitClear (TCCR2A, WGM21);
  bitClear (TCCR1A, WGM10);
  bitClear (TCCR1A, WGM11);  //Initialisation de tout les registres d'interruption.
  TCCR2B = 0b00000111;
  TIMSK2 = 0b00000001;
  TCCR1B = 0b00001101;
  TIMSK1 = 0b00000000;
  TCNT1 = 0;
  OCR1A = 15625;
  sei();
}

void setup_EEPROM(){
  if (EEPROM.read(0) == 0){
    reset();
  }
}

// ################################################################################################################################

// ACQUISITION --------------------------------------------------------------------------------------------------------------------
void Lecture_GPS() {
  String ordre = "";
  boolean Analyse = false;
  boolean fin = false;
  byte i = 0;
  unsigned long tempscapture = millis();

  while(!fin && millis()-tempscapture < TIMEOUT *1000.0){
    while(GPS_serial.available()){
      char Temp = char(GPS_serial.read());

      if(Temp == '$' && !Analyse){
        ordre="";
      }
      else if(Analyse && Temp == '$'){
        fin = true;
      }
      if(Analyse){
        if(i>=2 && i<=6){
          ordre+=Temp;
        }
        if(Temp == ','){
          i++;
        }
      }
      else{
        ordre += Temp;
      }
    }

    if(ordre == "$GPRMC"){
      Analyse = true;
      ordre = "";
    }
  }
  String Val = "";

  (&Capteur_GPS)-> Valide = (ordre[0] == 'A');

  Val = "";
  for(byte i=14;i<=23;i++){
    Val += ordre[i];
  }

  (&Capteur_GPS)-> Longitude = Val.toFloat();

  (&Capteur_GPS)-> EO =ordre[25];

  Val = "";
  for(byte i=2;i<=10;i++){
    Val += ordre[i];
  }

  (&Capteur_GPS)-> Latitude =Val.toFloat();

  (&Capteur_GPS)-> NS =ordre[12];
}



void Lecture_Capteur(){
  Adafruit_BME280 bme;
  unsigned status;
  status = bme.begin();

  unsigned long Td = millis();
  while(!status && (millis()-Td) <TIMEOUT*1000.0){
    status = bme.begin();
  }

  if(!status && !((Capteur_L_TPH[1]).Erreur) && !((Capteur_L_TPH[2]).Erreur && !((Capteur_L_TPH[3]).Erreur))){
    (Capteur_L_TPH+1) -> Erreur = true; // Erreur de température 
    (Capteur_L_TPH+2) -> Erreur = true; // Erreur de pression
    (Capteur_L_TPH+3) -> Erreur = true; // Erreur d'humidité
  } 
  else if(!status){
    Tab_refus[2] = true;  // Erreur d'accées au capteur TPH
  } 
  else{
    (Capteur_L_TPH+1) -> Valeur = floor(bme.readTemperature()); // Acquisition de la température
    (Capteur_L_TPH+2) -> Valeur = floor(bme.readPressure()/100.0F); // Acquisition de la pression et mise en hP (hecto Pascal) au lieu de P (Pascal)  

    if((Capteur_L_TPH[1]).Valeur>0){
      (Capteur_L_TPH+3) -> Valeur = floor(bme.readHumidity()); // Aquisition de l'humidité
    }
  }
  (Capteur_L_TPH[0]).Valeur = ((analogRead(A0)) *1023.0) /765.0; // Aquisition de la luminosité
}

// ################################################################################################################################

// TRAITEMENT ---------------------------------------------------------------------------------------------------------------------
void Gestion_erreur(){

  boolean Tab1 = false;

  int Test = (Capteur_L_TPH[0]).Valeur;

  if(Capteur_L_TPH[1].Valeur < MIN_TEMP_AIR || Capteur_L_TPH[1].Valeur > MAX_TEMP_AIR){ // Vérification que la température de l'air soit dans ses limites
    Tab1 = true;
  }

  if(!Capteur_L_TPH[2].Erreur){ // Vérification de l'erreur de la pression
    Tab_refus[6] = (Capteur_L_TPH[2].Valeur < PRESSURE_MIN_ALARME || Capteur_L_TPH[2].Valeur > PRESSURE_MAX_ALARME);

    if(Capteur_L_TPH[2].Valeur < PRESSURE_MIN || Capteur_L_TPH[2].Valeur > PRESSURE_MAX){ // Vérification que la pression soit dans ses limites
      Tab1 = true;
    }
  }

  Tab_refus[3] = !(Capteur_GPS).Valide;
  Tab_refus[1] = Tab1;

  (Capteur_L_TPH[0]).Valeur = Test;
}



void PrintDirectory_temps(File dir,char date[]){
  while (true) {
    File entry =  dir.openNextFile();

    if (! entry) {
      // no more files
      break;
    }

    if(entry.isDirectory()){
      PrintDirectory_temps(entry, date);
    }
    else if(String(entry.name()).endsWith(".LOG")){

      for(int i = 0;i<6;i++){
        if(date[i]>(entry.name())[i]){
          i= 7;
        }
        else if(date[i]<(entry.name())[i]){
          Tab_refus[5]=true;
          break;
        }
      }
    }
    entry.close();
  }
}



void File_temps_first(){
  if(isrunning()){
    File root = SD.open("/");
    PrintDirectory_temps(root, (nowRTC()).toString("AAMMDD"));
    root.close();
  }
  else{
    Tab_refus[5]=true;
  }
}

// ################################################################################################################################

// ENVOIE -------------------------------------------------------------------------------------------------------------------------
void Envoie(){
  if(!SD.begin()){
    Tab_refus[4] = true;
    return;
  }

  Tab_refus[4] = false;
   // Début de la construction du texte à envoyer -------------------------------
  String doc = "";

  // Horloge *******************************************************************
  doc += (horloge.toString("YY/MM/DD hh:mm:ss"));

  // GPS ***********************************************************************
  doc += ("/Position:");
  if(!(Capteur_GPS).GPS_eco || !(Capteur_GPS).Valide){
   doc += ("NA");
  } else{
    doc += (floor(((Capteur_GPS).Latitude)/100));
    doc += (" ");
    doc += ((Capteur_GPS).Latitude-floor(((Capteur_GPS).Latitude)/100));
    doc += (",");
    doc += ((Capteur_GPS).NS);
    doc += (" ");
    doc += (floor(((Capteur_GPS).Longitude)/100));
    doc += (" ");
    doc += ((Capteur_GPS).Longitude - floor(((Capteur_GPS).Longitude)/100));
    doc += (",");
    doc += ((Capteur_GPS).EO);
  }

  // Luminosite ****************************************************************
  doc += ("/Luminosite:");
  if(LUMIN == 0){
    doc += ("NA");
  }
  else{
    if((Capteur_L_TPH[0]).Valeur < LUMIN_LOW){
      doc += ("LOW");
    }
    else if((Capteur_L_TPH[0]).Valeur >= LUMIN_LOW && (Capteur_L_TPH[0]).Valeur < LUMIN_HIGH){
      doc += ("MEDUIM");
    }
    else if((Capteur_L_TPH[0]).Valeur >= LUMIN_HIGH){
      doc += ("HIGH");
    }
  }
  doc += '\n';

  // Temperature ***************************************************************
  doc += ("Temperature:");
  if((Capteur_L_TPH[1]).Erreur || TEMP_AIR == 0){ // Envoie des données selon l'erreur du capteur
    doc += ("NA");
  }
  else{
    doc += ((Capteur_L_TPH[1]).Valeur);
    doc += ("C/");
    doc += (int)(floor(((Capteur_L_TPH[1]).Valeur)*(9/5)+32)); // Convertion de la températion en Fahrenheit
    doc += 'F';
  }
  doc += '\n';

  // Pression ******************************************************************
  doc += ("Pression:");
  if((Capteur_L_TPH[2]).Erreur || PRESSURE == 0){  // Envoie des données selon l'erreur du capteur
    doc += ("NA");
  }
  else{
    doc += ((Capteur_L_TPH[2]).Valeur);
  }
  doc += ("HPa");
  doc += '\n';

  // Hygrometrie ***************************************************************
  doc += ("Hygrometrie:");
  if((Capteur_L_TPH[3]).Valeur == -1 || (Capteur_L_TPH[3]).Erreur || HYGR == 0){  // Envoie des données selon l'erreur du capteur
    doc += ("NA");
  }
  else{
    doc += ((Capteur_L_TPH[3]).Valeur);
  }
  doc += ("g/m3");
  doc += '\n';
  // fin de la construction du texte à envoyer ---------------------------------
  
  File file;
  unsigned long Size;
  // unsigned long Size_card = (512*SdVolume.blocksPerCluster() * SdVolume.clusterCount());
  
  File root = SD.open("/");
  Size = SD.Taille_carte() - root.size();
  root.close();

  String chemin = "";


  if(Size >= doc.length()){
    Tab_refus[0] = false;
      int j = 0;
      int i = 0;
    if(SD.exists(horloge.toString("YYMMDD"))){   
      while(1){
        if(j > 9){
          i++;
          j = 0;
          // break;
        }
        chemin = (horloge.toString("YYMMDD"));
        chemin += ("/");
        chemin += (i);
        SD.mkdir(chemin);
        delayMicroseconds(100);
        chemin = (horloge.toString("YYMMDD"));
        chemin += ("/");
        chemin += (i);
        chemin += ("/");
        chemin += (horloge.toString("YYMMDD"));
        chemin += ("-");
        chemin += (j);
        chemin += (".LOG");
        //Serial.println("-------");
        
        file = SD.open(chemin.c_str(), FILE_WRITE);
        Size = file.size();
        Serial.println(chemin.c_str());
        file.close();
        delayMicroseconds(100);
        if(MAX_SIZE > (Size + doc.length())){
          break;
        }else{
          j++;
        }
         
      }
      chemin = (horloge.toString("YYMMDD"));
      chemin += ("/");
      chemin += (i);
    }
    else{
      chemin = horloge.toString("YYMMDD/0");
      SD.mkdir(chemin.c_str());
      delayMicroseconds(100);
      chemin = horloge.toString("YYMMDD/0");
    }
  chemin += ("/");
  chemin += (horloge.toString("YYMMDD"));
  chemin += ("-");
  chemin += (j);
  chemin += (".LOG");
  
  file = SD.open(chemin.c_str(), FILE_WRITE);
  file.print(doc);
  file.close();
  delayMicroseconds(100);
  Serial.println(doc);
  

  } else{
    Tab_refus[0] = true;
  }


  
}

// ################################################################################################################################




// ################################################################################################################################


void setup(){
  Serial.begin(38400); // Ouverture du port serial
  GPS_serial.begin(9600); //  Ouverture de la connection GPS
  (beginRTC()); // Ouverture de la connection avec l'horloge (fonction de la bibliotèques RTC modifié)
  

  setup_EEPROM();

  if(digitalRead(2) == LOW){
    unsigned long temps = millis();
    while(millis()-temps < 1800000){
     setColorRGB(255, 128, 0);
     //Configuration();
    }
  }
  setup_Interruption();
}



void loop(){
  // Acquisition ****************************************************************
  unsigned long tempsPrecedent = millis();
  
  horloge = (nowRTC()); // Acquisition de la date et l'heure

  if(!Mode_eco){
    Lecture_GPS();
  } 
  else{
    if(!Capteur_GPS.GPS_eco){
      Lecture_GPS();
    }
    Capteur_GPS.GPS_eco = !(Capteur_GPS.GPS_eco);
  }

  Lecture_Capteur(); // Acquisition des données 

  // Traitement ****************************************************************
  File_temps_first(); // Vérification des erreurs d'horloge
  Gestion_erreur(); // Vérification des erreurs de capteur ou de GPS
  
  // Envoie ********************************************************************
  TIMSK2 = 0b00000000;
  Envoie(); // Envoie des données
  TIMSK2 = 0b00000001;
  SD.end();
  
  // Delay *********************************************************************
  while(millis()-tempsPrecedent < 10000*(1+(byte)Mode_eco)){
    // le code se bloquera dans cette boucle while pendant la duree LOGINTERVALL (de base 10 min) et ensuite reprendra la boucle loop ensuite
  } 
}
