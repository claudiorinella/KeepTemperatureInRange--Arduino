#include <ESP8266WiFi.h>

#include "FirebaseESP8266.h"

#include <time.h>

#include <DHT.h>

#define LED 2

#define DHTPIN 14
#define DHTTYPE DHT11

#define WIFI_SSID //"insert username"
#define WIFI_PASSWORD //"insert password"

#define WIFI_AP_SSID "ESPAP"
#define WIFI_AP_PASSWORD "password"

#define FIREBASE_HOST "prova-reti-51cd8.firebaseio.com"
#define FIREBASE_AUTH //"insert auth"

#define FIREBASEDATO_HOST "compitinogiugno2020.firebaseio.com"
#define FIREBASEDATO_AUTH //"insert auth"

DHT dht(DHTPIN, DHTTYPE);
FirebaseData firebaseData;
FirebaseData firebaseDatadato;
WiFiServer server(80);

int timezone = 2;
int dst = 0;
String timestamp = "";
int tempon = 5;
int tempoff = 600;
double tempdata = 0;
double tempext = 0;
double tempint = 4;
double tempsens = 0;
boolean state = 0;
int Delay1 = 500;
int Delay2 = 1000;
int Delay3 = 5000;

void setup() {
  delay(Delay2);
  Serial.begin(115200);
  delay(Delay3);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, !state);

  dht.begin();

  Firebase.begin(FIREBASEDATO_HOST, FIREBASEDATO_AUTH);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connessione a " + String(WIFI_SSID));

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(Delay1);
  }
  Serial.println();
  Serial.print("Connesso, con indirizzo IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Configurazione access point...");
  Serial.println(WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD) ? "Configurato" : "Fallito");
  Serial.print("Connesso, con indirizzo IP AP: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
  Serial.print("Server Attivato! Utilizza questo URL per connetterti: http://");
  Serial.println(WiFi.localIP());

  configTime(timezone * 3600, dst * 0, "pool.ntp-org", "time.nist.gov");
  Serial.println("Waiting for time");

  while (!time(nullptr)) {
    Serial.print(".");
    delay(Delay1);
  }

  Serial.println("");

} //FINE SETUP

void loop() {

  WiFiClient client;
  FirebaseJson updateData;

  timestamp = getTimestamp();
  tempsens = dht.readTemperature();
  int i = 0;

  Firebase.begin(FIREBASEDATO_HOST, FIREBASEDATO_AUTH);

  if (Firebase.getDouble(firebaseDatadato, "/main/temp")) {
    tempdata = firebaseDatadato.doubleData() - 273.15;
  } else {
    Serial.println(firebaseDatadato.errorReason());
  }

  tempext = (tempsens + tempdata) / 2;

  //ELABORAZIONE DATI

  if (tempint < 4) {

    state = 0;
    digitalWrite(LED, !state);
    Serial.println("Il motore è spento");

    tempint = tempint + (0.0001 * (tempext - tempint)) * tempoff;

  } else if (tempint > 8) {

    state = 1;
    digitalWrite(LED, !state);
    Serial.println("Il motore è acceso");

    tempint = tempint - (0.4 * tempon);

  } else {

    if (state == 0) {

      Serial.println("Il motore è spento");

      tempint = tempint + (0.0001 * (tempext - tempint)) * tempoff;

    } else {

      Serial.println("Il motore è acceso");

      tempint = tempint - (0.4 * tempon);

    }
  }

  //UPLOAD E DOWNLOAD DATI FIREBASE INTERNO

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  if (timestamp != "") {
    updateData.set("timestamp", timestamp);
    updateData.set("tempsens", tempsens);
    updateData.set("tempdata", tempdata);
    updateData.set("tempext", tempext);
    updateData.set("tempint", tempint);
    updateData.set("engine", state);

    if (Firebase.updateNode(firebaseData, "/", updateData)) {
      Serial.println("Corretto update dei dati!");
    } else {
      Serial.println(firebaseData.errorReason());
    }
  }

  if (Firebase.getString(firebaseData, "/timestamp")) {
    Serial.print("Orario: ");
    Serial.print(firebaseData.stringData());
    Serial.println(" GMT-3");
  } else {
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.getDouble(firebaseData, "/tempsens")) {
    Serial.print("Temperatura Sensore: ");
    Serial.println(firebaseData.doubleData());
  } else {
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.getDouble(firebaseData, "/tempdata")) {
    Serial.print("Temperatura Data: ");
    Serial.println(firebaseData.doubleData());
  } else {
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.getDouble(firebaseData, "/tempext")) {
    Serial.print("Temperatura Esterna: ");
    Serial.println(firebaseData.doubleData());
  } else {
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.getDouble(firebaseData, "/tempint")) {
    Serial.print("Temperatura Interna: ");
    Serial.println(firebaseData.doubleData());
  } else {
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.getBool(firebaseData, "/engine")) {
    Serial.print("Motore: ");
    Serial.println(firebaseData.boolData());
  } else {
    Serial.println(firebaseData.errorReason());
  }

  Serial.println("");

  //GESTIONE DELAY E RICHIESTE WEBSERVER

  if (state == 0) {
    while (i < tempoff) {

      client = server.available();

      if (client) {
        clientResponse(client);
      }

      delay(Delay2);
      i++;
    }
  } else {
    while (i < tempon) {

      client = server.available();

      if (client) {
        clientResponse(client);
      }

      delay(Delay2);
      i++;
    }
  }
  Serial.println("");

} // FINE LOOP

//FUNZIONI

String getTimestamp() {
  String timestamp = "";
  time_t now = time(nullptr);
  timestamp = String(ctime( & now));
  timestamp.remove(24);
  if (timestamp.endsWith("1970") || timestamp.endsWith("1969")) {
    return "";
  } else {
    return timestamp;
  }
}

void clientResponse(WiFiClient client) {
  String request = "";
  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(Delay1);
  }
  client.flush();
  client.println("HTTP/1.1 200 OK");
  client.println("Contet-Type: text/html; charset=utf8");
  client.println("");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<title>Is your fridge cool?</title>");
  client.println("<meta charset=\"utf-8\">");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<meta name=\"description\" content=\"Progetto Reti di Dati\">");
  client.println("<meta name=\"keyword\" content=\"Sensori, dht11, html, css, università, progetto\">");
  client.println("<meta name=\"author\" content=\"Morganti Claudio, Rinella Claudio, Crestini Riccardo\">");
  client.println("<meta http-equiv=\"refresh\" content=\"5\">");
  client.println("<link rel=\"stylesheet\" type=\"text/css\" href=\"http://telematica.altervista.org/styles.css\">");
  client.println("</head>");
  client.println("<body>");
  client.println("<header id=\"top\">");
  client.println("<section>");
  client.println("<aside class=\"left\"><h1>");
  client.println("<img align=\"middle\" src=\"https://media1.arduiner.com/14465-big_default_2x/modulo-dht11-sensore-umidit-e-temperatura-con-led.jpg\" width=\"150\" height=\"150\" alt=\"Immagine\">");
  client.println("</h1></aside>");
  client.println("<h1>Is your fridge cool?</h1>");
  client.println("<aside class=\"right\"><h1>");
  client.println("<img align=\"middle\" src=\"https://www.puntosicuro.it/_resources/images/Celle%20frigorifere_15mar17.jpg\" width=\"150\" height=\"150\" alt=\"Immagine\">");
  client.println("</h1></aside>");
  client.println("</section>");
  client.println("</header>");
  client.println("<nav>");
  client.println("<table class=\"menu\"><tr>");
  client.println("<th><a href=\"#S1\">Temperatura Sensore</a></th>");
  client.println("<th><a href=\"#S2\">Temperatura Database Esterno</a></th>");
  client.println("<th><a href=\"#S3\">Temperatura Esterna</a></th>");
  client.println("<th><a href=\"#S4\">Temperatura Interna</a></th>");
  client.println("<th><a href=\"#S5\">Motore</a></th>");
  client.println("</tr></table>");
  client.println("</nav>");
  client.println("<section id=\"S1\">");
  client.print("<aside class='left'><h1>");
  client.print(tempsens);
  client.println("°C</h1></aside>");
  client.println("<article class=\"right\">");
  client.println("<h3>La temperatura del sensore DHT11</h3>");
  client.println("<p>Questa temperatura viene direttamente misurata dal sensore DHT11 collegato alla nostra scheda ESP8266</p>");
  client.println("<p><span><a href=\"#top\">Back</a></span></p>");
  client.println("</article>");
  client.println("</section>");
  client.println("<section id=\"S2\">");
  client.println("<article class=\"left\">");
  client.println("<h3>La temperatura estratta dal database esterno</h3>");
  client.println("<p>Questa temperatura viene raccolta dal database fornito dal professor Caputo</p>");
  client.println("<p><span><a href=\"#top\">Back</a></span></p>");
  client.println("</article>");
  client.println("<aside class=\"right\"><h1>");
  client.print(tempdata);
  client.println("°C</a></h1></aside>");
  client.println("</section>");
  client.println("<section id=\"S3\">");
  client.println("<aside class=\"left\"><h1>");
  client.print(tempext);
  client.println("°C</a></h1>");
  client.println("</aside>");
  client.println("<article class=\"right\">");
  client.println("<h3>La temperatura esterna</h3>");
  client.println("<p>Questa temperatura viene calcolata facendo la media fra quella misurata dal sensore DHT11 collegato alla nostra scheda ESP8266 e la temperatura fornita dal database esterno fornito dal professor Caputo</p>");
  client.println("<p><span><a href=\"#top\">Back</a></span></p>");
  client.println("</article>");
  client.println("</section>");
  client.println("<section id=\"S4\">");
  client.println("<article class=\"left\">");
  client.println("<h3>La temperatura interna</h3>");
  client.println("<p>Questa temperatura viene calcolata tramite le formule del compito assegnato in funzione del motore</p>");
  client.println("<p><span><a href=\"#top\">Back</a></span></p>");
  client.println("</article>");
  client.print("<aside class=\"right\"><h1>");
  client.print(tempint);
  client.println("°C</a></h1>");
  client.println("</aside>");
  client.println("</section>");
  client.println("<section id=\"S5\">");
  client.println("<aside class=\"left\"><h1>");
  if (state == 1) {
    client.print("Acceso");
  } else {
    client.print("Spento");
  }
  client.println("</a></h1>");
  client.println("</aside>");
  client.println("<article class=\"right\">");
  client.println("<h3>Motore</h3>");
  client.println("<p>E la situazione attuale del motore, se è acceso il motore la temperatura interna sta diminuendo, se spento la temperatura interna sta aumentando</p>");
  client.println("<p><span><a href=\"#top\">Back</a></span></p>");
  client.println("</article>");
  client.println("</section>");
  client.println("<footer>");
  client.println("<table class=\"foot\"><tr>");
  client.println("<th>Claudio Morganti</th>");
  client.println("<th>Riccardo Crestini</th>");
  client.println("<th>Claudio Aglieri Rinella</th>");
  client.println("<th><a href=\"#top\">Back</a></th>");
  client.println("</tr></table>");
  client.println("</footer>");
  client.println("</body>");
  client.println("</html>");
}
