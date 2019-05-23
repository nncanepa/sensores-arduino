#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetUdp.h>
#include <PubSubClient.h>
#include <DHT.h>


#define DHTPIN 9
#define DHTTYPE DHT22
#define FLOW_SW1 5
#define FLOW_SW2 6

// Serial -> USB
// Serial1 -> MaxiGauge
// Serial2 -> Flujimetro

char buffMG[200];
char buffDHT[200];
char buffFLUX[200];
char buffFLOW[200];
char lecturaFLUX[7];
char lecturaMg[20];
char tempMG[6][20];

byte mac[] = {  0x0A, 0xAB, 0xBB, 0xCC, 0xDE, 0x43  };
IPAddress server(192, 168, 1, 100); //IP del server MQTT

// Topics para MQTT
char TOPIC_MG_READ[50];
char TOPIC_DHT_READ[50];
char TOPIC_FLUX_READ[50];
char TOPIC_FLUX_WRITE[100];
char TOPIC_FLOW_SW_READ[50];

// JSONs para mensaje MQTT
StaticJsonDocument<JSON_OBJECT_SIZE(6)> MG_READ;
StaticJsonDocument<JSON_OBJECT_SIZE(2)> DHT_READ;
StaticJsonDocument<JSON_OBJECT_SIZE(1)> FLUX_READ;
StaticJsonDocument<JSON_OBJECT_SIZE(1)> FLUX_WRITE;
StaticJsonDocument<JSON_OBJECT_SIZE(2)> FLOW_SW_READ;

//Config DHT22
DHT dht(DHTPIN, DHTTYPE);

// RED y MQTT clients
EthernetClient ethClient;
PubSubClient mqtt(ethClient);

// Maxigauge controls
/*  'ETX': "\x03", # End of Text (Ctrl-C)   Reset the interface
  'CR':  "\x0D", # Carriage Return        Go to the beginning of line
  'LF':  "\x0A", # Line Feed              Advance by one line
  'ENQ': "\x05", # Enquiry                Request for data transmission
  'ACQ': "\x06", # Acknowledge            Positive report signal
  'NAK': "\x15", # Negative Acknowledge   Negative report signal
  'ESC': "\x1b", # Escape
*/
char ETX=0x03;
char CR=0x0D;
char LF=0x0A;
char ENQ=0x05;
char ACQ=0x06;
char NAK=0x15;
char ESC=0x1b;

// Callback de mensaje recibido de MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  deserializeJson(FLUX_WRITE, payload, length);
  float fluxSet = FLUX_WRITE["setV"];
  Serial2.print('a');
  Serial2.println(fluxSet*64000/50.0);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//Funcion que reconecta al MQTT si se pierde la conexion.
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("Sensores_Tierra")) {
      Serial.println("connected to MQTT");
      mqtt.subscribe(TOPIC_FLUX_WRITE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Funcion que imprime la direccion IP en el serie USB.
void printIPAddress() {
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  Serial.println();
}

void lecturaDHT(){
  // Lectura de presion y humedad
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Falló lectura de temperatura y humedad");
  }
  DHT_READ["getT"] = t;
  DHT_READ["getH"] = h;
  strcpy(buffDHT, "\0");
  serializeJson(DHT_READ, buffDHT);
  mqtt.publish(TOPIC_DHT_READ, buffDHT);
}

void lecturaMG(){
  // Lectura de valores de presion del MaxiGauge
  for(int i=0;i<=6;i++){
    while(Serial1.read()>=0);
    Serial1.write(ETX);
    Serial1.print("PR");
    Serial1.print(i+1);
    Serial1.print("\r\n");
    delay(50);
    while(Serial1.read()>=0);
    Serial1.write(ENQ);
    delay(50);
    if(Serial1.available()>0){
      int index=0;
      strcpy(lecturaMg, '\0');
      do{
        if(index>1){
          lecturaMg[index-2]=Serial1.read();
        }else{
          Serial1.read();
        }
        index++;
      }while(Serial1.available()>0);
      strcpy(tempMG[i], lecturaMg);
      delay(50);
    }
  }
  MG_READ["getV1"]=atof(tempMG[0]);
  MG_READ["getV2"]=atof(tempMG[1]);
  MG_READ["getV3"]=atof(tempMG[2]);
  MG_READ["getV4"]=atof(tempMG[3]);
  MG_READ["getV5"]=atof(tempMG[4]);
  MG_READ["getV6"]=atof(tempMG[5]);
  strcpy(buffMG, '\0');
  serializeJson(MG_READ, buffMG);
  //serializeJson(MG_READ, Serial);
  mqtt.publish(TOPIC_MG_READ, buffMG);
}

void lecturaFlux(){
  while(Serial2.read()>=0);
  Serial2.print("a\r");
  delay(50);
  int index=0;
  lecturaFLUX[0]='\0';
    do{
      if(index>26 && index<33){
        lecturaFLUX[index-27]=Serial2.read();
      } else {
      Serial2.read();
      }
      index++;
    }while(Serial2.available()>0);
  FLUX_READ["getV"] = atof(lecturaFLUX);
  strcpy(buffFLUX, '\0');
  serializeJson(FLUX_READ, buffFLUX);
  //serializeJson(FLUX_READ, Serial);
  mqtt.publish(TOPIC_FLUX_READ, buffFLUX);
}

void lecturaFlow_SW(){
  bool sw1 = not digitalRead(FLOW_SW1);
  bool sw2 = not digitalRead(FLOW_SW2);
  FLOW_SW_READ["getS1"] = sw1;
  FLOW_SW_READ["getS2"] = sw2;
  serializeJson(FLOW_SW_READ, buffFLOW);
  mqtt.publish(TOPIC_FLOW_SW_READ, buffFLOW);
  delay(50);
}

void setup() {
  // put your setup code here, to run once:
  // hardware serial
  Serial.begin(9600); // Serial USB para DEBUG.
  while (!Serial);
  Serial1.begin(9600); // Serial1 comunicacion con MaxiGauge.
  while (!Serial1);
  Serial2.begin(9600); // Serial2 comunicacion con Flujimetro.
  while (!Serial2);
  
  Serial.println("Hardware Serial: 9600 8N1 (USB)");
  Serial.println("-- Serial Monitor");
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Fallo de configuracion Ethernet");
    for(;;)
    ;
  }
  printIPAddress();

  // Config MQTT
  mqtt.setServer(server, 1883);
  mqtt.setCallback(callback);
  
  // Maxigauge
  TOPIC_MG_READ[0]='\0';
  strcpy(TOPIC_MG_READ, "N_TIERRA/SENSORES/VACIO/read");
  
  // Config DHT22
  TOPIC_DHT_READ[0] = '\0';
  strcpy(TOPIC_DHT_READ, "N_TIERRA/SENSORES/TEMPHUM/read");
  dht.begin();

  // Config Flux
  TOPIC_FLUX_READ[0] ='\0';
  TOPIC_FLUX_WRITE[0] ='\0';
  strcpy(TOPIC_FLUX_READ, "N_TIERRA/SENSORES/FLUX/read");
  strcpy(TOPIC_FLUX_WRITE, "N_TIERRA/SENSORES/FLUX/write");

  // Config Flow Switches
  pinMode(FLOW_SW1, INPUT_PULLUP);
  pinMode(FLOW_SW2, INPUT_PULLUP);
  TOPIC_FLOW_SW_READ[0] = '\0';
  strcpy(TOPIC_FLOW_SW_READ, "N_TIERRA/SENSORES/FLOW_SW/read");

}

void loop() {
    
  if (!mqtt.connected()) {
    reconnect();
  }
  delay(200);
  mqtt.loop();

  lecturaDHT();
  lecturaMG();
  lecturaFlux();
  lecturaFlow_SW();
}
