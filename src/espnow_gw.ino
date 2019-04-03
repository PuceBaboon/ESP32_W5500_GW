/*
 *   $Id: espnow_gw.ino,v 1.16 2019/02/02 15:14:06 gaijin Exp $
 *
 * ESP-Now WiFi to Eternet Gateway.  Takes packets from other ESP-Nodes
 * and publishes them to MQTT via a cheap, readily available and easily
 * interfaceable W5500 Ethernet board.
 *
 * The intended use of this project is to enable multiple (up to 20)
 * battery-powered ESP8266 boards to support various sensors around the
 * home, sending data back to the main network via this gateway.  The use
 * of ESP-Now should greatly extend the battery life of the ESP8266 nodes
 * beyond that of normal WiFi.  ESP-Now communicates using low-level
 * protocols (using the module's MAC address instead of requiring an IP),
 * thus shortening the time during which the node needs to consume 
 * relatively large amounts of power for radio transmissions.
 *
 * This gateway is intended to be mains powered and permanently linked to
 * a LAN via the W5500 board.  The ESP-Now connection is via the WiFi, which
 * -never- operates in normal "station" or "access-point" mode; it only
 * processes the short, MAC addressed, ESP-Now protocol messages.
 *
 *
 *   *** IMPORTANT ***  The latest version of the arduino SPI library for the ESP32
 *                      has been copied into the local lib directory for this
 *                      project, as the fixes for Paul Stoffregen's changes
 *                      haven't propagated out into the PlatformIO chain, yet.
 *
 */

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>           // *** FOR REFERENCE *** Important info in this file.
#include <EthernetUdp.h>
#include <Ethernet.h>           // Required for ETHERNET. Wiznet code now included by default.
#include <PubSubClient.h>
#include <esp_now.h>            // Note "esp_now.h" for the ESP32 ("espnow.h" for the 8266).
/* *INDENT-OFF* */
extern "C" {
//    #include <user_interface.h>
    #include "espnow_common.h"
    #include "espnow_gw.h"
}
/* *INDENT-ON* */ 


/*
 * Display numeric MAC address in standard format.
 */
void printMacAddress(uint8_t * macaddr) {
    for (int i = 0; i < 6; i++) {
        Serial.print(macaddr[i], HEX);
        if (i < 5)
            Serial.print(':');
    }
    Serial.println();
}


/*
 * Set up ESP-Now WiFi, role and call-backs.
 */
void initEspNow() {

    /*
     * Initialize the on-board WiFi before starting ESP-Now.
     */
    esp_wifi_set_mode(WIFI_AP_STA);
    esp_wifi_set_mac(WIFI_IF_AP, &wifi_mac[0]); // Set our special MAC address.
    esp_wifi_set_channel(WIFI_DEFAULT_CHANNEL, WIFI_SECOND_CHAN_NONE);  // Set our specific channel.

    WiFi.disconnect(true);

    if (esp_now_init() != ESP_OK) {
        Serial.println("*** ESP_Now init failed");
        ESP.restart();
    }

    Serial.print("This node AP mac: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("This node STA mac: ");
    Serial.println(WiFi.macAddress());


/* *INDENT-OFF* */
/*
    esp_now_register_recv_cb([](uint8_t * r_mac, uint8_t * r_data, uint8_t len) {
                 memcpy(&rec_MAC, r_mac, sizeof(rec_MAC));	// Save MAC passed in cb.
                 memcpy(&sensorData, r_data, sizeof(sensorData));	// Save data struct passed in cb.
                 haveReading = true;}
    );
*/
/* *INDENT-ON* */

    /*
     * Brute-force add of our peers to the peer list.
     (void *) esp_now_add_peer(cont01_mac,
     (uint8_t) ESP_NOW_ROLE_CONTROLLER,
     (uint8_t) WIFI_DEFAULT_CHANNEL, NULL, 0);
     (void *) esp_now_add_peer(cont02_mac,
     (uint8_t) ESP_NOW_ROLE_CONTROLLER,
     (uint8_t) WIFI_DEFAULT_CHANNEL, NULL, 0);
     (void *) esp_now_add_peer(cont03_mac,
     (uint8_t) ESP_NOW_ROLE_CONTROLLER,
     (uint8_t) WIFI_DEFAULT_CHANNEL, NULL, 0);
     (void *) esp_now_add_peer(cont04_mac,
     (uint8_t) ESP_NOW_ROLE_CONTROLLER,
     (uint8_t) WIFI_DEFAULT_CHANNEL, NULL, 0);
     (void *) esp_now_add_peer(cont05_mac,
     (uint8_t) ESP_NOW_ROLE_CONTROLLER,
     (uint8_t) WIFI_DEFAULT_CHANNEL, NULL, 0);
     (void *) esp_now_add_peer(cont06_mac,
     (uint8_t) ESP_NOW_ROLE_CONTROLLER,
     (uint8_t) WIFI_DEFAULT_CHANNEL, NULL, 0);
     */

    /* If the MAC address does not already exist in peer list, add it. */

}


/*
 * Wiz W5500 reset function.  Change this for the specific reset
 * sequence required for your particular board or module.
 */
void WizReset() {
    Serial.print("Resetting Wiz W5500 Ethernet Board...  ");
    pinMode(RESET_P, OUTPUT);
    digitalWrite(RESET_P, HIGH);
    delay(250);
    digitalWrite(RESET_P, LOW);
    delay(50);
    digitalWrite(RESET_P, HIGH);
    delay(350);
    Serial.println("Done.");
}


void mq_callback(char *topic, byte * payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
}


bool mq_publish(const char *topic, const char *payload) {
    if (!client.connected()) {
	mq_connect();
    }
#ifdef DEBUG
    Serial.println("Publishing... ");
    Serial.print("  Topic: ");
    Serial.print(topic);
    Serial.print("  Payload: ");
    Serial.println(payload);
#endif
    if (client.publish(topic, payload)) {
#ifdef DEBUG
        Serial.print(">> OK ");
#endif
        return (0);
    } else {
#ifdef DEBUG
        Serial.print(">> FAILED ");
#endif
        return (1);
    }
}


/*
 * MQTT connect/reconnect loop.
 */
void mq_connect() {
    uint8_t rc_cnt = 0;         // Reconnect attempts.
    // Loop until we're connected.
    while (!client.connected()) {
#ifdef DEBUG
        Serial.print("Attempting MQTT connection...");
#endif
        // Attempt to connect
        if (client.connect("ESP-Now_Gateway")) {
#ifdef DEBUG
            Serial.println("   connected.");
#endif
            // Once connected, publish an announcement...
            mq_publish((char *) TOPIC00, "ESP-Now Gateway On-Line");
        } else {
            if (++rc_cnt >= MQRC_MAX) {
                Serial.println("MQTT Fail... Restarting.");
                ESP.restart();
            } else {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" .  Trying again in 5 seconds...");
                // Wait 5 seconds before retrying
                delay(5000);
            }
        }
    }
}


/*
 * This is a crock. It's here in an effort
 * to help people debug hardware problems with
 * their W5100 ~ W5500 board setups.  It's 
 * a copy of the Ethernet library enums and
 * should, at the very least, be regenerated
 * from Ethernet.h automatically before the
 * compile starts (that's a TODO item).
 *
 */
/*
 * Print the result of the hardware status enum
 * as a string.
 * Ethernet.h currently contains these values:-
 *
 *  enum EthernetHardwareStatus {
 *  	EthernetNoHardware,
 *  	EthernetW5100,
 *  	EthernetW5200,
 *  	EthernetW5500
 *  };
 *
 */
void prt_hwval(uint8_t refval) {
    switch (refval) {
    case 0:
        Serial.println("No hardware detected.");
        break;
    case 1:
        Serial.println("WizNet W5100 detected.");
        break;
    case 2:
        Serial.println("WizNet W5200 detected.");
        break;
    case 3:
        Serial.println("WizNet W5500 detected.");
        break;
    default:
        Serial.println
            ("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
    }
}


/*
 * Print the result of the ethernet connection
 * status enum as a string.
 * Ethernet.h currently contains these values:-
 *
 *  enum EthernetLinkStatus {
 *     Unknown,
 *     LinkON,
 *     LinkOFF
 *  };
 *
 */
void prt_ethval(uint8_t refval) {
    switch (refval) {
    case 0:
        Serial.println("Uknown status.");
        break;
    case 1:
        Serial.println("Link flagged as UP.");
        break;
    case 2:
        Serial.println("Link flagged as DOWN. Check cable connection.");
        break;
    default:
        Serial.println
            ("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
    }
}


void setup() {
    Serial.begin(115200);
    Serial.println("\n\n\nEthernet-Based MQTT Gateway v1.0\n\n");

    /*
     * Start the ESP-Now listener on the WiFi network.
     */

    /*
     * Start the Ethernet (wired, non-ESPNOW) connection and the server.
     * First, set up the slave select pin as an output, as the Arduino API
     * apparently doesn't handle automatically pulling SS low.
     */
    Ethernet.init(5);           // ESP32 VSPI-SS.
    WizReset();

    /* 
     * Network configuration - all except the MAC are optional.
     *
     * IMPORTANT NOTE - The mass-produced W5500 boards do -not-
     *                  have a built-in MAC address (depite 
     *                  comments to the contrary elsewhere). You
     *                  -must- supply a MAC address here.
     */
    Serial.println("Starting ETHERNET connection...");
    Ethernet.begin(eth_MAC, eth_IP, eth_DNS, eth_GW, eth_MASK);

    delay(200);

    Serial.print("Ethernet IP is: ");
    Serial.println(Ethernet.localIP());

    /*
     * Sanity checks for W5500 and cable connection.
     */
    Serial.print("Checking connection.");
    bool rdy_flag = false;
    for (uint8_t i = 0; i <= 20; i++) {
        if ((Ethernet.hardwareStatus() == EthernetNoHardware)
            || (Ethernet.linkStatus() == LinkOFF)) {
            Serial.print(".");
            rdy_flag = false;
            delay(80);
        } else {
            rdy_flag = true;
            break;
        }
    }
    if (rdy_flag == false) {
        Serial.println
            ("\n\r\tHardware fault, or cable problem... cannot continue.");
        Serial.print("Hardware Status: ");
        prt_hwval(Ethernet.hardwareStatus());
        Serial.print("   Cable Status: ");
        prt_ethval(Ethernet.linkStatus());
        while (true) {
            delay(10);          // Halt.
        }
    } else {
        Serial.println(" OK");
    }


    /* 
     * MQTT client is associated with the ETHERNET 
     * connection (see espnow_gw.h).
     */
    Serial.println("Connecting to MQTT broker...");
    client.setServer(mq_server, 1883);
    client.setCallback(mq_callback);

    if (!client.connected()) {
        mq_connect();
    }
}


void loop() {
    char buff[MQ_TOPIC_MAX];
    static int i = 0;
    static int last = 0;

    if (haveReading) {
        haveReading = false;
        Serial.print("Connection from: ");
        printMacAddress(rec_MAC);
        Serial.printf
            ("  Id: %i, Temp=%0.1f, Hum=%0.0f%%, pressure=%0.0fmb\r\n",
             sensorData.loc_id, sensorData.temp, sensorData.humidity,
             sensorData.pressure);
        snprintf(buff, MQ_TOPIC_MAX, "%i:%0.2f:%0.0f:%0.0f",
                 sensorData.loc_id, sensorData.temp, sensorData.humidity,
                 sensorData.pressure);
        mq_publish((char *) TOPIC01, buff);
    }
    if (!client.connected()) {
        mq_connect();
    }
    client.loop();
    yield();
}
