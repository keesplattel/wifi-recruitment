#include <WiFiClient.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>

String st;
String content;
int statusCode;
File fsUploadFile;              // a File object to temporarily store the received file

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS

  const char *softAP_ssid = "_Bij M2Media zoeken we AV";
//const char *softAP_ssid = "_Director! Interesse? Connect";
//const char *softAP_ssid = "_met dit wifi netwerk!";
//const char *softAP_ssid = "_____________________________";
//const char *softAP_ssid = "-M2Media zoekt nieuwe collega";


/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "M2Media";

// DNS server
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);
const byte DNS_PORT = 53;

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Current WLAN status */
int status = WL_IDLE_STATUS;
int statsNumber;

void setup() {
  pinMode(2, OUTPUT);

  delay(100);
  Serial.begin(115200);

  delay(100);
  SPIFFS.begin();

  delay(100);
  EEPROM.begin(512);

  delay(100);
  Serial.println("Reading EEPROM");

  String esid;
  for (int i = 0; i < 32; ++i){
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  
  if ( esid.length() > 1 ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(esid.c_str());
  }else{
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(softAP_ssid);
  }
 
  delay(100);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  delay(100);
  server.begin();  
  //server.on("/admin",handleAdmin);
  //server.on("/setting",handleSetting);
  //server.on("/cleareeprom",cleareeprom);
  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                    // Receive and save the file
  );

  server.on("/admin", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/admin.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/admin", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleSSIDchange                                   // Receive and save the file
  );


  server.onNotFound([]() {                              // If the client requests any URI
      handleFileRead(server.uri());

      if (!handleFileRead(server.uri())){   
        // send it if it exists     
        // otherwise, respond with a 404 (Not Found) error
        server.send(404, "text/plain", "404: Not Found");
      }

  });

  delay(100);
  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("HTTP server started");
}

void loop() {
  //LED ON
  digitalWrite(2, HIGH);
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}


bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);

  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    
    statsNumber++;
    EEPROMWriteInt(100, statsNumber);


    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", "");
    server.sendContent("");
    server.client().stop();
  }else{ 
    if (path.endsWith("/")){
      path += "index.html";
    }else if(path.endsWith("/hotspot-detect.html")){
      path += "index.html";
    }else if(path.endsWith("/generate_204")){
      path += "index.html";
    }else if(path.endsWith("/fwlink")){
      path += "index.html";
    }
    // else if(path.startsWith("/stats")){
    //   handleStats();
    // }else if(path.startsWith("/stats/reset")){
    //   // Reset Stats
    //   statsNumber = 0;
    //   EEPROMWriteInt(100, statsNumber);

    //   // Load stats page with reset URL
    //   handleStats();
    // }

  }

  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }

  //Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}


void handleAdmin() {
  server.on("/admin", []() {
    //IPAddress ip = WiFi.softAPIP();
    //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html>";
    //content += ipStr;
    content += "<h1>SSID Management</h1>";
    content += "<p>";
    content += st;
    content += "</p>";
    content += "<form method='get' action='settingssid'><label>Network Name: </label><input name='ssid' length=32><input type='submit'></form>";
    // content += "<h1>File Upload</h1>";
    // content += "<form method='post' enctype='multipart/form-data'>";
    // content += "<input type='file' name='name'>";
    // content += "<input class='button' type='submit' value='upload'>";
    // content += "</form>";
    content += "</html>";
    server.send(200, "text/html", content);  
  });
}


// void handleSetting() {
//   server.on("/settingssid", []() {
//     String qsid = server.arg("ssid");
  
//     if (qsid.length() > 0) {
//       Serial.println("clearing eeprom");
//       for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
//       Serial.println(qsid);
//       Serial.println("");
        
//       Serial.println("writing eeprom ssid:");
//       for (int i = 0; i < qsid.length(); ++i){
//         EEPROM.write(i, qsid[i]);
//         Serial.print("Wrote: ");
//         Serial.println(qsid[i]); 
//       }
//       Serial.println("writing eeprom pass:"); 
  
//       EEPROM.commit();
//       content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
//       statusCode = 200;
//     }
//     // else{
//     //   content = "{\"Error\":\"404 not found\"}";
//     //   statusCode = 404;
//     //   Serial.println("Sending 404");
//     // }
//     server.send(statusCode, "application/json", content);
//   });
// }

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/index.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void handleSSIDchange(){
  String qsid = server.arg("ssid");
  
  if (qsid.length() > 0) {
    Serial.println("clearing eeprom");
    for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
    Serial.println(qsid);
    Serial.println("");
      
    Serial.println("writing eeprom ssid:");
    for (int i = 0; i < qsid.length(); ++i){
      EEPROM.write(i, qsid[i]);
      Serial.print("Wrote: ");
      Serial.println(qsid[i]); 
    }
    Serial.println("writing eeprom pass:"); 
  
    EEPROM.commit();
    content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
    statusCode = 200;
  }
  // else{
  //   content = "{\"Error\":\"404 not found\"}";
  //   statusCode = 404;
  //   Serial.println("Sending 404");
  // }
  server.send(statusCode, "application/json", content);
}


/** Is this an IP? */
boolean isIp(String str) {
  for (uint8_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (uint8_t i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}


void EEPROMWriteInt(int address, int value)
{
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);
  
  EEPROM.write(address, two);
  EEPROM.write(address + 1, one);
}
 
int EEPROMReadInt(int address)
{
  long two = EEPROM.read(address);
  long one = EEPROM.read(address + 1);
 
  return ((two << 0) & 0xFFFFFF) + ((one << 8) & 0xFFFFFFFF);
}
