/* This program allows ESP32 to connect to a Wi-Fi network
 and upload specified text content to a designated Google Drive folder 
 using Google Apps Script. Users can customize the Wi-Fi credentials,
  the Google Drive folder name, and the file content. The program establishes
  a secure connection, constructs an HTTP POST request, and sends the data 
  to Google Drive for storage. */

const char* ssid     = "WIFI-ssid";  //customize Wi-Fi ssid
const char* password = "WIFI-password";  // customize Wi-Fi password

const char* myDomain = "script.google.com";
String myScript = "myScript-URL";  // Set Google Script path
String myFoldername = "myDriveFolder";  // Set Google Drive folder name to store text data
String myFilename = "data.txt";  // Set Google Drive file name to store text data

// Content to be sent to Google Drive
String fileContent = "This is the content of the file.";  // Edit this string to change the file content

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"  // For power instability, no restart
#include "soc/rtc_cntl_reg.h"  // For power instability, no restart

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable restart when power unstable
  
  Serial.begin(115200);
  delay(10);
  
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  
  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if ((StartTime + 10000) < millis()) break;
  } 

  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed");
    ESP.restart();
  }

  // Send the content to Google Drive
  sendTextToGoogleDrive();
}

void loop() {
  delay(100);
}

String sendTextToGoogleDrive() {
  String Data = "myFoldername=" + urlencode(myFoldername) + "&myFilename=" + urlencode(myFilename) + "&myFile=" + urlencode(fileContent);  
  
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client_tcp;

  if (client_tcp.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    String postRequest = "POST " + myScript + " HTTP/1.1\r\n" +
                         "Host: " + String(myDomain) + "\r\n" +
                         "Content-Type: application/x-www-form-urlencoded\r\n" +
                         "Content-Length: " + String(Data.length()) + "\r\n" +
                         "Connection: close\r\n\r\n" +
                         Data;

    client_tcp.print(postRequest);
    Serial.println("Request sent: " + postRequest);
    
    unsigned long timeout = millis();
    while (client_tcp.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client_tcp.stop();
        return "Connection Timeout";
      }
    }
      
    String response = "";
    bool redirect = false;
    String location = "";
    
    while(client_tcp.available()) {
      String line = client_tcp.readStringUntil('\n');
      response += line + "\n";
      
      if (line.startsWith("Location: ")) {
        redirect = true;
        location = line.substring(10);
        location.trim();
      }
      
      if (line == "\r") {
        break;  // Headers are complete
      }
    }
    
    if (redirect) {
      Serial.println("Redirecting to: " + location);
      client_tcp.stop();
      return sendRequestToUrl(location, Data);
    }
    
    // Read the rest of the response
    while(client_tcp.available()) {
      char c = client_tcp.read();
      response += c;
    }
    
    Serial.println("Response:");
    Serial.println(response);
    
    client_tcp.stop();
    Serial.println("Connection closed");
    
    return response;
  } else {
    Serial.println("Connection to " + String(myDomain) + " failed.");
    return "Connection failed";
  }
}

String sendRequestToUrl(String url, String data) {
  // Extract host and path from URL
  int colonIndex = url.indexOf(':');
  int slashIndex = url.indexOf('/', colonIndex + 3);
  String host = url.substring(colonIndex + 3, slashIndex);
  String path = url.substring(slashIndex);

  WiFiClientSecure client_tcp;

  if (client_tcp.connect(host.c_str(), 443)) {
    String postRequest = "POST " + path + " HTTP/1.1\r\n" +
                         "Host: " + host + "\r\n" +
                         "Content-Type: application/x-www-form-urlencoded\r\n" +
                         "Content-Length: " + String(data.length()) + "\r\n" +
                         "Connection: close\r\n\r\n" +
                         data;

    client_tcp.print(postRequest);
    
    String response = "";
    while(client_tcp.available()) {
      char c = client_tcp.read();
      response += c;
    }
    
    client_tcp.stop();
    return response;
  } else {
    return "Redirect connection failed";
  }
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}
