#include <IBMIOTF32.h>
#include <DHT.h>
#include <esp_log.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 //TODO: define again when MM cable arrives

#define POWER_PIN 17
#define SIGNAL_PIN 36

int waterValue = 0;


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


String user_html = "";


char*               ssid_pfix = (char*)"iothome";
unsigned long       lastPublishMillis = 0;

const char *displaytext = "Ready";


void publishData() {
    StaticJsonDocument<512> root;
    JsonObject data = root.createNestedObject("d");


    data["DISP"];
    data["WATER"] = waterValue;

    serializeJson(root, msgBuffer);
    client.publish(publishTopic, msgBuffer);

}

void handleUserCommand(JsonDocument* root) {
    JsonObject d = (*root)["d"];

    // YOUR CODE for command handling

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

void waterSetup() {

    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW);
}

void waterLoop() {
    digitalWrite(POWER_PIN, HIGH);
    delay(10);
    waterValue = analogRead(SIGNAL_PIN);
    digitalWrite(POWER_PIN, LOW);

    Serial.print("The water sensor value: ");
    Serial.println(waterValue);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,10);
    display.print("The water sensor value: ");
    display.println(waterValue);
    display.display();

    delay(1000);
}


void setup() {
    Serial.begin(115200);
    initDevice();

    displaySetup();

    waterSetup();

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

    waterLoop();

}