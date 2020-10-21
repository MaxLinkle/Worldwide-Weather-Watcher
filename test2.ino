#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <Adafruit_BME280.h>

#define chipSelect 4
#define Nb_capteur 4
#define PRESSURE_MIN_ALARME 850
#define PRESSURE_MAX_ALARME 1080


//EEPROM.get(0, 1);
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



DateTime horloge;

SoftwareSerial GPS_serial(4,5); // redefinir les pins pour le GPS



boolean Mode_eco = false;

volatile boolean Tab_refus[7] = {false, false, false, false, false, false, false};
//volatile char Bouton_appuye = 'A';
volatile byte varCompteur1 = 0;
volatile byte varCompteur2 = 0;
volatile boolean boutonR = false;



// BIBLIOTEQUE --------------------------------------------------------------------------------------------------------------------
  // ChainableLED
void sendByte(byte b){
    // Send one bit at a time, starting with the MSB
    for (byte i=0; i<8; i++)
    {
      pinMode(6,OUTPUT);
      pinMode(5,OUTPUT);
        // If MSB is 1, write one and clock it, else write 0 and clock
        if ((b & 0x80) != 0)
            digitalWrite(6, HIGH);
        else
            digitalWrite(6, LOW);

    digitalWrite(5, LOW);
    delayMicroseconds(20);
    digitalWrite(5, HIGH);
    delayMicroseconds(20);

        // Advance to the next bit to send
        b <<= 1;
    }
}

void setColorRGB(byte red, byte green, byte blue){
    // Send data frame prefix (32x "0")
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);

        // Start by sending a byte with the format "1 1 /B7 /B6 /G7 /G6 /R7 /R6"
    byte prefix = 0b11000000;
    if ((blue & 0x80) == 0)     prefix|= 0b00100000;
    if ((blue & 0x40) == 0)     prefix|= 0b00010000;
    if ((green & 0x80) == 0)    prefix|= 0b00001000;
    if ((green & 0x40) == 0)    prefix|= 0b00000100;
    if ((red & 0x80) == 0)      prefix|= 0b00000010;
    if ((red & 0x40) == 0)      prefix|= 0b00000001;
    sendByte(prefix);

    // Now must send the 3 colors
    sendByte(blue);
    sendByte(green);
    sendByte(red);

    // Terminate data frame (32x "0")
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
    sendByte(0x00);
}



// ################################################################################################################################

// EEPROM -------------------------------------------------------------------------------------------------------------------------
void Message(String* message){
  String decoupe="";
  byte adresse=-1;
  for(int i=0; i<(*message).length(); i++) {
    decoupe += (*(message))[i];
    if((*(message))[i] == '='){
        Trouver(&adresse,decoupe);
        decoupe = "";
       
    }
 }
  switch (adresse){
    case -2:
        if (isDigit(decoupe[0]) && isDigit(decoupe[1]) && decoupe[2]==':' && isDigit(decoupe[3]) && isDigit(decoupe[4]) && decoupe[5]==':' && isDigit(decoupe[6]) && isDigit(decoupe[7])){
          char hour[2] = {decoupe[0], decoupe[1]};
          char minute[2] = {decoupe[3], decoupe[4]};
          char second[2] = {decoupe[6], decoupe[7]};
          adjust(DateTime((nowRTC()).year(), (nowRTC()).month(), (nowRTC()).day(), (atoi(hour)), atoi(minute), atoi(second)));
        }
      break;
    case -3:
        if (isDigit(decoupe[0]) && isDigit(decoupe[1]) && decoupe[2]=='/' && isDigit(decoupe[3]) && isDigit(decoupe[4]) && decoupe[5]=='/' && isDigit(decoupe[6]) && isDigit(decoupe[7]) && isDigit(decoupe[8]) && isDigit(decoupe[9])){
          char day[2] = {decoupe[0], decoupe[1]};
          char month[2] = {decoupe[3], decoupe[4]};
          char year[4] = {decoupe[6], decoupe[7], decoupe[8], decoupe[9]};
          adjust(DateTime(atoi(year), atoi(month), atoi(day), (nowRTC()).hour(), (nowRTC()).minute(), (nowRTC()).second()));
        }
      break;
    default:
      EEPROM.put(adresse, int(decoupe.toInt()));
  }
}


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

void Trouver(byte* adresse,String decoupe){
  
        if (decoupe == "CLOCK="){
        *adresse=-2;
 
      }
      else if (decoupe == "DATE="){
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
  if (message == "RESET"){
    reset();
  }
  else if (message == "VERSION"){
    Serial.println("1.0");
  }else{
    Message(&message);
  }
}

// ################################################################################################################################

// INTERRUPTION -------------------------------------------------------------------------------------------------------------------
void Changement_LED(){
  if (Tab_refus[6]){ // Tempete
    setColorRGB(255, 0, 0);


  }
  else if (Tab_refus[5]){ // Erreur horloge
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0);

    } else{
      setColorRGB(0, 0, 255);
    }

  }
  else if (Tab_refus[4]){ // Erreur d'ecriture de la carte SD
    if (varCompteur1 < 42){
      setColorRGB(255, 0, 0);
    } else{
      setColorRGB(255, 255, 255);
    }

  }
  else if (Tab_refus[3]){ // Erreur de GPS
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0);
    } else{
      setColorRGB(220,0,200) ;   //(255, 255, 0);
    }

  }
  else if (Tab_refus[2]){ // Erreur d'accees aux capteurs
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0);
    } else{
      setColorRGB(0, 255, 0);
    }

  }
  else if (Tab_refus[1]){ // Donnee pas belle
    if (varCompteur1 < 42){
      setColorRGB(255, 0, 0);
    } else{
      setColorRGB(0, 255, 0);
    }
  }
  else if (Tab_refus[0]){ // Carte SD pleine
    if (varCompteur1 < 63){
      setColorRGB(255, 0, 0);
    } else{
      setColorRGB(255, 255, 255);
    }

  }else if (Mode_eco){
    setColorRGB(0, 0, 255);
  } else{
    setColorRGB(0, 255, 0);
  }
}



void Interruption(){
  if(!digitalRead(2) || !digitalRead(3)){
    TIMSK1 = 0b00000010;

  }
  else{
    if(digitalRead(2) && digitalRead(3)){
      TIMSK1 = 0b00000000;
      varCompteur2 = 0;

    }
  }
}

ISR(TIMER2_OVF_vect){
  TCNT2 = 256 - 125;
  if(varCompteur1++ > 125){
    varCompteur1 = 0;
  }
  Changement_LED();
}

ISR(TIMER1_COMPA_vect){
  if(varCompteur2<5){
    varCompteur2++;
  }
  else if(varCompteur2 == 5){
     varCompteur2++;
    if(digitalRead(2)== LOW && digitalRead(3)==LOW){

    }
    else if(digitalRead(2)== LOW){
      boutonR = !boutonR;
      interrupts();
      TIMSK2 = 0b00000000;
      while (boutonR){
      // fonction de  Maintenance()
      setColorRGB(230, 105, 0);
    }
    TIMSK2 = 0b00000001;
    }
    else{
      Mode_eco = !(Mode_eco);
    }

  }
}

// ################################################################################################################################

// INITIALISATION -----------------------------------------------------------------------------------------------------------------
void setup_RTC(){
  Wire.begin();
}

void setup_Interruption(){
  // bouton sur les pins 2 et 3
  pinMode(2,INPUT);
  pinMode(3,INPUT);
  attachInterrupt(digitalPinToInterrupt(2),Interruption,CHANGE);
  attachInterrupt(digitalPinToInterrupt(3),Interruption,CHANGE);
  cli();
  bitClear (TCCR2A, WGM20);
  bitClear (TCCR2A, WGM21);
  bitClear (TCCR1A, WGM10);
  bitClear (TCCR1A, WGM11);
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

// AQUISITION ---------------------------------------------------------------------------------------------------------------------
void Lecture_GPS() {
  String ordre = "";
  boolean Analyse = false;
  boolean fin = false;
  byte i = 0;
  unsigned long tempscapture = millis();

  while(!fin && millis()-tempscapture < TIMEOUT){
    while(GPS_serial.available()){
      char Temp = char(GPS_serial.read());

      if(Temp == '$' && !Analyse){
        ordre="";
      }else if(Analyse && Temp == '$'){
        fin = true;
      }
      if(Analyse){
        if(i>=2 && i<=6){
          ordre+=Temp;
        }



        if(Temp == ','){
          i++;
        }
      }else{
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

void Lecture_luminosite(){
  (Capteur_L_TPH[0]).Valeur = analogRead(A0);
  (Capteur_L_TPH[0]).Valeur = ((Capteur_L_TPH[0]).Valeur)*1023.0/765.0;
  
}

void Lecture_TPH(){
  Adafruit_BME280 bme;
  unsigned status;
  status = bme.begin();

  unsigned long Td = millis();
  while(!status && (millis()-Td) <TIMEOUT*1000.0){
    status = bme.begin();
  }

  if(!status && !((Capteur_L_TPH[1]).Erreur) && !((Capteur_L_TPH[2]).Erreur && !((Capteur_L_TPH[3]).Erreur))){
    (Capteur_L_TPH+1) -> Erreur = true;
    (Capteur_L_TPH+2) -> Erreur = true;
    (Capteur_L_TPH+3) -> Erreur = true;
  } else if(!status){

    Tab_refus[2] = true;

    } else{
    (Capteur_L_TPH+1) -> Valeur = floor(bme.readTemperature());
    (Capteur_L_TPH+2) -> Valeur = floor(bme.readPressure()/100.0F);

    if((Capteur_L_TPH[1]).Valeur>0){
      (Capteur_L_TPH+3) -> Valeur = floor(bme.readHumidity());
    }
  }
}

// ################################################################################################################################

// TRAITEMENT ---------------------------------------------------------------------------------------------------------------------
void Gestion_erreur(){

boolean Tab1 = false;
boolean Tab2 = false;

 int Test = (Capteur_L_TPH[0]).Valeur;
  if(Capteur_L_TPH[1].Erreur){ // Temperature de l'air
    Tab2 = true;
  } else if(Capteur_L_TPH[1].Valeur < MIN_TEMP_AIR || Capteur_L_TPH[1].Valeur > MAX_TEMP_AIR){
    Tab1 = true;
  }



  
  if(Capteur_L_TPH[2].Erreur){ // Pression
    Tab2 = true;
  } else{
    if(Capteur_L_TPH[2].Valeur < PRESSURE_MIN || Capteur_L_TPH[2].Valeur > PRESSURE_MAX){
      Tab1 = true;
    }


      Tab_refus[6] = (Capteur_L_TPH[2].Valeur < PRESSURE_MIN_ALARME || Capteur_L_TPH[2].Valeur > PRESSURE_MAX_ALARME);
    
  }

  if(Capteur_L_TPH[3].Erreur){ // Hygrometrie
    Tab2 = true;
  }



    Tab_refus[3] = !(Capteur_GPS).Valide;
  

  Tab_refus[1] = Tab1;
  Tab_refus[2] = Tab2;

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
    } else if(String(entry.name()).endsWith(".LOG")){

      for(int i = 0;i<6;i++){
        if(date[i]>(entry.name())[i]){
          i= 7;
        } else if(date[i]<(entry.name())[i]){
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
  }else{
    Tab_refus[5]=true;
  }
}

// ################################################################################################################################

// ENVOIE -------------------------------------------------------------------------------------------------------------------------
void Envoie(){
  Sd2Card card;
  SdVolume volume;

  if(!SD.begin(chipSelect) || !card.init(SPI_HALF_SPEED, chipSelect) || !volume.init(card)){
    Tab_refus[4] = true;
    return;
  }
Tab_refus[4] = false;
  // ---------------------------------------------------------------------------
  String doc = "";

  // Horloge
  doc += (horloge.toString("YY/MM/DD hh:mm:ss"));

  // GPS
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

  // Luminosite
  doc += ("/Luminosite:");
  if((Capteur_L_TPH[0]).Valeur < LUMIN_LOW){
    doc += ("LOW");
  }
  else if((Capteur_L_TPH[0]).Valeur >= LUMIN_LOW && (Capteur_L_TPH[0]).Valeur < LUMIN_HIGH){
    doc += ("MEDUIM");
  }
  else if((Capteur_L_TPH[0]).Valeur >= LUMIN_HIGH){
    doc += ("HIGH");
  }
  doc += '\n';

  // Temperature
  doc += ("Temperature:");
  doc += ((Capteur_L_TPH[1]).Valeur);
  doc += ("C/");
  doc += (int)(floor(((Capteur_L_TPH[1]).Valeur)*(9/5)+32));
  doc += 'F';
  doc += '\n';

  // Pression
  doc += ("Pression:");
  doc += ((Capteur_L_TPH[2]).Valeur);
  doc += ("HPa");
  doc += '\n';

  // Hygrometrie
  doc += ("Hygrometrie:");
  doc += ((Capteur_L_TPH[3]).Valeur);
  doc += ("g/m3");
  doc += '\n';
  // ---------------------------------------------------------------------------

  File file;
  unsigned long Size;
  // unsigned long Size_card = (512*SdVolume.blocksPerCluster() * SdVolume.clusterCount());

  File root = SD.open("/");
  Size = (512*volume.blocksPerCluster() * volume.clusterCount()) - root.size();
  root.close();

  String chemin = "";

  int j = 0;
  int i = 0;

  if(Size >= doc.length()){
    Tab_refus[0] = false;

    if(SD.exists(horloge.toString("YYMMDD"))){
      while(1){

        if(j > 9){
          i++;
          j = 0;
          // break;
        }
        
        chemin = (horloge.toString("YYMMDD/"));
        chemin += (i);
        SD.mkdir(chemin);


        chemin += (horloge.toString("/YYMMDD_"));
        chemin += (j);
        chemin += (".log");
        file = SD.open(chemin, FILE_WRITE);
        Size = file.size();
        file.close();
        if(MAX_SIZE > (Size + doc.length())){
          break;
        }
          j++;
        
         
      }

      chemin = (horloge.toString("YYMMDD/"));
      chemin += (i);

    }
    else{
      chemin = String(horloge.toString("YYMMDD/0"));
      SD.mkdir(chemin);

    }

  } else{
    Tab_refus[0] = true;
  }

  chemin += (horloge.toString("/YYMMDD_"));
  chemin += (j);
  chemin += (".LOG");
  file = SD.open(chemin, FILE_WRITE);
  file.print(doc);
  file.close();
   Serial.println(chemin);
  Serial.println(doc);
  
}

// ################################################################################################################################




// ################################################################################################################################


void setup(){
  Serial.begin(9600);
  GPS_serial.begin(9600);
  beginRTC();
  

  //setup_EEPROM();

if(digitalRead(2) == LOW){


  unsigned long temps = millis;
  while(millis()-temps < 1800000){
    setColorRGB(255, 128, 0);
   //Configuration();
  }
}
  setup_Interruption();


}

void loop(){

 horloge = nowRTC();
  if(!Mode_eco){
    Lecture_GPS();
  } else{
    if(!Capteur_GPS.GPS_eco){
      Lecture_GPS();
    }
    Capteur_GPS.GPS_eco = !(Capteur_GPS.GPS_eco);
  }



  Lecture_luminosite();
  Lecture_TPH();

  // Traitement

  File_temps_first();
  

  Gestion_erreur();

  // Envoie

  Envoie();

  // Boucle de gestion du temps

  unsigned long tempsPrecedent = millis();

while(millis()-tempsPrecedent < (unsigned long)(60*1000*LOGINTERVAL*(1+(byte)Mode_eco))){

}

 
}
