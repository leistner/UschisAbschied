#include <Arduino.h>
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include<TFT_eSPI.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "LittleFS.h"
#include <ArduinoOTA.h>
#include <Arduino_JSON.h>
#include <JPEGDecoder.h>
#include <wire.h>
#include "driver/uart.h"
#include <TFT_eWidget.h> 
//#define FileSys LittleFS
//TFT Pins sind bereits in der Usersetup Datei hinterlegt
/*#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   22  // Chip select control pin
#define TFT_DC   21 // Data Command control pin
#define TFT_RST   -1
#define TOUCH_CS 10*/
#define FORMAT_LITTLEFS_IF_FAILED true
#define CALIBRATION_FILE "/TochCalData3"
#define REPEAT_CAL false
#define stoppx 100
#define stoppy 150
#define startx 100
#define starty 150
//RS485 Pinbelegung
#define UART_FULL_THRESH_DEFAULT (64)
#define ECHO_UART_PORT UART_NUM_2
#define ECHO_TEST_TXD   (17)
#define ECHO_TEST_RXD   (16)
#define ECHO_TEST_RTS   (14)
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE
#define BUF_SIZE        (128)
#define BAUD_RATE       (115200)
#define PACKET_READ_TICS        (100 / portTICK_RATE_MS)
#define TS_MINX 342
#define TS_MAXX 3545
#define TS_MINY 265
#define TS_MAXY 3338
static QueueHandle_t uart0_queue;
typedef struct circular_buf_t circular_buf_t;
typedef circular_buf_t* cbuf_handle_t;
static const char *TAG = "uart_events";
const char* ap_ssid = "UschisKiste";
const char* ap_password = "123456789";
AsyncEventSource events("/events");
#define LOOP_PERIOD 35 // Display updates every 35 ms
/*int rawX = 0; // Define rawX
int rawY = 0; // Define rawY
int touchX = map(rawX, TS_MAXX,TS_MINX,  7, 480);
int touchY = map(rawY, TS_MAXY,TS_MINY,  7, 320);*/



TFT_eSPI tft = TFT_eSPI();
MeterWidget   gramm = MeterWidget(&tft);
File file;
AsyncWebServer server(80);
AsyncWebSocket wss("/ws");
JSONVar readings;
JSONVar Doc(2048);
String CBefehl;
int ledState = 0;
char Messwert[20];
signed int converted;
int t=0;
boolean Startbool=false;
boolean Stoppbool=false;
int Messwertzaehler=0;
//JPEG Anfang- Ladedauer der einzelnen Bilder wird angezeigt
//####################################################################################################
void showTime(uint32_t msTime) {
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}
//####################################################################################################
//Informationen zu den Bildern
//####################################################################################################
void jpegInfo() {
  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}
//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {
  //jpegInfo(); // Print information from the JPEG file (could comment this line out)
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;
  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();
  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;
  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)
    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;
    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;
    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }
    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;
    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }
  tft.setSwapBytes(swapBytes);
  showTime(millis() - drawTime); // These lines are for sketch testing only
}
//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {
  // Open the named file (the Jpeg decoder library will close it)
  //File jpegFile = SD.open( filename, FILE_READ);  //SD-Karte
  File jpegFile = LittleFS.open(filename, "r");  //LittleFS Dateiensystem
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }
  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");
  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)
  if (decoded) {
    // print information about the image to the serial port
    //jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

// TFT kalibrieren- nur einmal am Anfang notwendig

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!LittleFS.begin()) {
    Serial.println("Formatting file system");
    LittleFS.format();
  LittleFS.begin();
  
  }
   Serial.println("Bis hierhin");
  // check if calibration file exists and size is correct
  if (LittleFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      LittleFS.remove(CALIBRATION_FILE);
      Serial.println("Kein File");
    }
    else
    {
      File f = LittleFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
        Serial.println("File");
      }
    }
  }
Serial.println("Bis hierhin 1");
  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    //tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = LittleFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}
//JPEG Ende
// SD Karte bearbeiten
// SD-Karte Verzeichnis erzeugen

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}
// SD-Karte Verzeichnis entfernen

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}
// SD-Karte Datei lesen
void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

// SD-Karte DDatei schreiben
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// SD-Karte Daten in vorhandene Datei schreiben
void appendFile(fs::FS &fs, const char * path, const char * message){
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
// SD-Karte Datei umbenennen
void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}
// SD-Karte Datei löschen
void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}


//Datei erzeugen SD Karte
void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }
  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}
//Ende SD Karte bearbeiten
// Soft IP Login

void softAP() {
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("AP IP address: ");
  Serial.println(IP);
}

// Ende Soft IP

// WIFI Login, falls notwendig
/*
void initWiFi() { 
WiFi.begin(ssid, assword);

  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
 
 
}*/

//SD Karte Initialisierung- nicht implementiert, Dateien werden im Little FS System gespeichert- Kann abe in Betrie genommen werden, falls notwendig
  
void initSDCard() {
  if (!SD.begin(5,tft.getSPIinstance())) {
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
      Serial.println("No SD card attached");
      return;
  }

  Serial.print("SD Card Type: ");
  switch (cardType) {
      case CARD_MMC: Serial.println("MMC"); break;
      case CARD_SD: Serial.println("SDSC"); break;
      case CARD_SDHC: Serial.println("SDHC"); break;
      default: Serial.println("UNKNOWN"); break;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}
//SD Karte Dateien listen
void listFiles(const char *dirname) {
  Serial.printf("Files in directory: %s\n", dirname);
  File root = LittleFS.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

// Dateiensystem LittleFS wird eingerichtet
void initSPIFFS() {
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("LittleFS Mount Failed");
    return;
}
else{
Serial.println("LittleFS mounted successfully");
// Liste der Dateien im Ordner "comon"
listFiles("/common");
// Liste der Dateien im Ordner "logo"
listFiles("/logo");
}
  }


// Websocket Sende Befehl Allgemein

void notifyClients() {
  Doc["ledState"]=ledState;
  String jsonString = JSON.stringify(Doc);
   wss.textAll(jsonString);
}

// Befehl zum Starten des Füllvorgangs-muss noch angepasst werden- Startbool hilft gegen ein erneutes Einschalten, solange der Füllvorgang läuft

void startFuellen(){
  if (!Startbool){
  const uart_port_t uart_num = ECHO_UART_PORT;
                 // Allocate buffers for UART
        cbuf_handle_t circular_buf_init(uint8_t* data, size_t size);
  void circular_buf_put(cbuf_handle_t cbuf, uint8_t data);
      uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
      uart_write_bytes(uart_num, "MSV?\r\n", 7);
      delay(10);
      long length = 0;
      size_t data_len;
      if (uart_get_buffered_data_len(uart_num, (size_t*)&length) == ESP_OK) {
          length = uart_read_bytes(uart_num,data, length,0);
      }    
      Startbool=true;
}}

// Befehl zum Stoppen des Füllvorgangs-muss noch angepasst werden

void stoppFuellen(){

  if (!Stoppbool){
  const uart_port_t uart_num = ECHO_UART_PORT;
  // Allocate buffers for UART
cbuf_handle_t circular_buf_init(uint8_t* data, size_t size);
void circular_buf_put(cbuf_handle_t cbuf, uint8_t data);
uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
uart_write_bytes(uart_num, "MSV?\r\n", 7);
delay(10);
long length = 0;
size_t data_len;
if (uart_get_buffered_data_len(uart_num, (size_t*)&length) == ESP_OK) {
length = uart_read_bytes(uart_num,data, length,0);
}  
Stoppbool=true;
  }
}

//Informationen, die von der Webseite gesendet wurden

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message=(char*)data;
    Serial.println(message);
    CBefehl = message.substring(0, message.indexOf("_"));
     if (strncmp("https:\\", (char*)data, 8)==0) {
      }
      else if (strcmp((char*)data, "Start") == 0){
        //startFuellen();
        Startbool=true;
        ledState=1001;
        notifyClients();
        Serial.println("Abfuellprozess gestartet");
      }
      else if (strcmp((char*)data, "Stopp") == 0){
       // stoppFuellen();
        Stoppbool=true;
        ledState=1002;
        notifyClients();
        Serial.println("Abfuellprozess gestoppt");
      }
      }}

      //Websocket bei Ein, oder Ausgang

      void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
        void *arg, uint8_t *data, size_t len) {
      switch (type) {
      case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
      case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
      case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
      break;
      }
      }

      // Websocket initialisierung
      void initWebSocket() {
  wss.onEvent(onEvent);
  server.addHandler(&wss);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse* response = request->beginResponse(LittleFS, "/index.html", "text/html");
    request->send(response);
});
  server.serveStatic("/", LittleFS, "/");
  server.begin();   
      }

      // Ende Websocket


      // RS485

      void switchToTX() {
        digitalWrite(ECHO_TEST_RTS, HIGH); // Senden aktivieren
      }
      
      void switchToRX() {
        digitalWrite(ECHO_TEST_RTS, LOW); // Empfangen aktivieren
      }


// Messwert wird (soll) über Websocket alle 300 ms gesendet
      void Messwertsenden(){
      ledState=1000;
      Doc["Messwert"]=Messwert;
      Doc["ledState"]=ledState;
      String jsonString = JSON.stringify(Doc);
      wss.textAll(jsonString);
      //tft.fillScreen(TFT_BLACK);
      tft.setCursor(300, 160);
      tft.setTextFont(2);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK,false);
      tft.println(Messwert);
      }
      



//Abfrage an AD05/AD105 RS485
       void Messwertholen() {
       if (millis() > t+(300)) {
       const uart_port_t uart_num = ECHO_UART_PORT;
                 // Allocate buffers for UART
                 cbuf_handle_t circular_buf_init(uint8_t* data, size_t size);
                  void circular_buf_put(cbuf_handle_t cbuf, uint8_t data);
                   uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
                  uart_write_bytes(uart_num, "MSV?\r\n", 7);
                  delay(10);
                //const int uart_num = UART_NUM_2;
                  //uint8_t data[128];
                long length = 0;
              size_t data_len; 
                   if (uart_get_buffered_data_len(uart_num, (size_t*)&length) == ESP_OK) {
                  length = uart_read_bytes(uart_num,data, length,0);
                 }
                Serial.println(String(length));
               if (length>=5){
               if (data[0]<0xF0){
                //positive Werte
                converted=(int)(data[2]|((int)data[1]<<8)|((int)data[0]<<16)); //
               }
               else {
                //negative Werte
                converted=(int)(data[2]|((int)data[1]<<8)|((int)data[0]<<16)|(int)0xFF000000);//
               }
      
               sprintf(Messwert,"%d",converted);
               Messwertsenden();

              }
                ESP_ERROR_CHECK(uart_flush_input(ECHO_UART_PORT));
               // free(data);
                converted=0;
      
              t = millis();
              
            }
       }

      //Ende RS485

// Touch Kontrolle

void Beruehrungskontrolle(){
  uint16_t x, y;
 
   // See if there's any touch data for us
   if (tft.getTouch(&x, &y))
   {
     // Draw a block spot to show where touch was calculated to be
     #ifdef BLACK_SPOT
       tft.fillCircle(x, y, 2, TFT_BLACK);
     #endif
     //Entgegen der Platzierung von Bildern ist der Nullpunkt unten links
     // Bei den Bilderplatzierung ist der Nullpunkt oben rechts
     //Startbutton Abfrage
      if ((x > 43) && (x < (43 + 92))) {
         if ((y >20) && (y <= (20+ 92))) {
           Serial.println("Start gedrückt");
          drawSdJpeg("/logo/Uschi_start_m.jpg", 43, 208); 
          delay(1000);
          drawSdJpeg("/logo/Uschi_start_o.jpg", 43, 208); 
          // Hier wird der Füllvorgang gestartet, am besten über ein boolsche Variable
          
         }}
     //Stopptbutton Abfrage
     
       if ((x > 355) && (x < (355 + 92))) {
         if ((y > 20) && (y <= (20+ 92))) {
          Serial.println("Stopp gedrückt");
          drawSdJpeg("/logo/Uschi_stopp_m.jpg", 355, 208); 
          delay(1000);
          drawSdJpeg("/logo/Uschi_stopp_o.jpg", 355, 208); 
          // Hier wird der Füllvorgang gestoppt, am besten über ein boolsche Variable
       }}
      }}

// Meter auf dem Display

float mapValue(float ip, float ipmin, float ipmax, float tomin, float tomax)
{
  return tomin + (((tomax - tomin) * (ip - ipmin))/ (ipmax - ipmin));
}

// Hier wird im Moment der Messwert erzeugt, später die Übergabe Messwert in value

void meter(){
  static int d = 0;
  static uint32_t updateTime = 0;  

  if (millis() - updateTime >= LOOP_PERIOD) 
  {
    updateTime = millis();

    d += 4; if (d > 360) d = 0;

    // Create a Sine wave for testing, value is in range 0 - 100
    float value = 50.0 + 50.0 * sin((d + 0) * 0.0174532925);

    if(Messwertzaehler==3){
      Messwertzaehler=0;
      dtostrf(value, 4, 2, Messwert);
      Messwertsenden();
    }
    else{
      Messwertzaehler++;
    }

    

    float gramm_;
    gramm_ = mapValue(value, (float)0.0, (float)100.0, (float)0.0, (float)10.0);
    //Serial.print(", V = "); Serial.println(voltage);
    gramm.updateNeedle(gramm_, 0);
    
    
  }

}



void setup() {
  Serial.begin(115200);

  
  digitalWrite(22, HIGH); // Touch controller chip select (if used)
  digitalWrite(15, HIGH); // TFT screen chip select
  digitalWrite( 5, HIGH); 
  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI); 
  tft.begin();
  
  
  initSPIFFS();
  initSDCard();
  
  //touch_calibrate();
  
  //initWiFi();
  softAP();
  initWebSocket();
  ArduinoOTA.begin();
  tft.setRotation(1);  // portrait
  tft.fillScreen(random(0xFFFF));
  tft.fillScreen(TFT_RED); // Black screen fill
  
  drawSdJpeg("/common/Team.jpg", 0, 0); 
  delay(5000);
  drawSdJpeg("/common/Basis1.jpg", 0, 0); 
  drawSdJpeg("/logo/Uschi_start_o.jpg", 43, 208); 
  drawSdJpeg("/logo/Uschi_stopp_o.jpg", 355, 208); 
  gramm.setZones(0, 100, 25, 75, 0, 0, 40, 60);
  gramm.analogMeter(115, 60, 10.0, "g", "0", "2.5", "5", "7.5", "10"); 
  
  

}

void loop() {
  ArduinoOTA.handle();
  wss.cleanupClients();
  vTaskDelay(1); 
  Beruehrungskontrolle();
  meter();
  //Messwertholen();
  
  // put your main code here, to run repeatedly:
}

