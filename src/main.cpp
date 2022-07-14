#include <IBMIOTF32.h>
#include <DHT.h>
#include <esp_log.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>


#define LED 2
#define RELAY 17

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 //TODO: define again when MM cable arrives

#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

const int trigPin = 5;
const int echoPin = 18;

long duration;
float ultrasonicDistance;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


String user_html = "";




char*               ssid_pfix = (char*)"iothome";
unsigned long       lastPublishMillis = 0;

char *ledon = "on";
char *ledoff = "off";
char *ledonoff;

char *relayon = "on";
char *relayoff = "off";
char *relayonoff;

const char *displaytext = "Ready";

void ledstate(){
    if(digitalRead(LED) == 1){
        ledonoff = ledon;
    }
    else{
        ledonoff = ledoff;
    }
}

void relayloop() {
    if(digitalRead(RELAY) == 1) {
        relayonoff = relayon;
    } else {
        relayonoff = relayoff;
    }
    
}

void ultrasonicloop() {

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);

    ultrasonicDistance = duration * SOUND_SPEED/2;

    // Serial.print("Distance: ");
    // Serial.print(ultrasonicDistance);
    // Serial.println(" cm");

    //delay(5000);
    delay(100);

}

// void cleardisplay(JsonDocument* root) {

//     JsonObject d = (*root)["d"];

//     if(d.containsKey("DISP")) {
//         if(strcmp(d["DISP", "clearclearclear"])) {

//             display.clearDisplay();

//             d["DISP"] = "";

//         }
//     } 
// }



void publishData() {
    StaticJsonDocument<512> root;
    JsonObject data = root.createNestedObject("d");

    
    data["LED"] = ledonoff;
    data["RELAY"] = relayonoff;
    data["DISP"];
    data["ULTRASONIC"] = ultrasonicDistance;
    

    serializeJson(root, msgBuffer);
    client.publish(publishTopic, msgBuffer);


}

void handleUserCommand(JsonDocument* root) {
    JsonObject d = (*root)["d"];

    // YOUR CODE for command handling
    
     if(d.containsKey("LED")) {
        if (strcmp(d["LED"], "off")) {
            digitalWrite(LED,HIGH);
            
            Serial.println("LED on");
        } 
        else {
            digitalWrite(LED,LOW);
            
            Serial.println("LED off");
        }
    }

    if(d.containsKey("RELAY")) {
        if (strcmp(d["RELAY"], "off")) {
            digitalWrite(RELAY, HIGH);

            Serial.println("RELAY on");
        } else {
            digitalWrite(RELAY, LOW);

            Serial.println("RELAY off");
        }
    }

    if(d.containsKey("DISP")) {

        if (strcmp(d["DISP"], "clearclearclear")) {

            displaytext = d["DISP"].as<char*>();
            String dispText = (String) displaytext;

            display.clearDisplay();

            display.setTextSize(1);
            display.setTextColor(WHITE);
            display.setCursor(0,10);
            display.println(dispText);
            display.display();
            Serial.println(dispText);

        } else {

            d["DISP"] = "";

            display.clearDisplay();
            display.display();

            Serial.println("Cleared");

        }

    }


}

void message(char* topic, byte* payload, unsigned int payloadLength) {
    byte2buff(msgBuffer, payload, payloadLength);
    StaticJsonDocument<512> root;
    DeserializationError error = deserializeJson(root, String(msgBuffer));

    if (error) {
        Serial.println("handleCommand: payload parse FAILED");
        return;
    }

    handleIOTCommand(topic, &root);
    if (strstr(topic, "/device/update")) {
        JsonObject meta = cfg["meta"];

    // YOUR CODE for meta data synchronization

    } else if (strstr(topic, "/cmd/")) {            // strcmp return 0 if both string matches
        handleUserCommand(&root);
    }
}

void displaySetup() {

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // or 0x3C
        Serial.println("allocation failed");
        for(;;);
    }

    delay(3000);

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,10);
    display.println("Ready");
    display.display();

}

void ultrasonicSetup() {

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

}

void setup() {
    Serial.begin(115200);
    initDevice();
    
    pinMode(LED,OUTPUT);
    pinMode(RELAY,OUTPUT);

    digitalWrite(LED,LOW);
    digitalWrite(RELAY,LOW);

    displaySetup();

    ultrasonicSetup();

    yield();
    JsonObject meta = cfg["meta"];
    pubInterval = meta.containsKey("pubInterval") ? atoi((const char*)meta["pubInterval"]) : 0;
    lastPublishMillis = - pubInterval;
    startIOTWatchDog((void*)&lastPublishMillis, (int)(pubInterval * 5));
    // YOUR CODE for initialization of device/environment
    yield();
    WiFi.mode(WIFI_STA);

    yield();
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    int i = 0;
    yield();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if(i++ > 25) reboot();
    }
    Serial.println("\nIP address : "); Serial.println(WiFi.localIP());

    client.setCallback(message);
    set_iot_server();
}

void loop() {
    
    if (!client.connected()) {
        yield();
        iot_connect();
    }
    client.loop();
    // YOUR CODE for routine operation in loop

    if ((pubInterval != 0) && (millis() - lastPublishMillis > pubInterval)) {
        publishData();
        lastPublishMillis = millis();
        yield();
    }
    ledstate();
    relayloop();
    ultrasonicloop();
    // cleardisplay();

}