# ESP32_W5500_GW
An MQTT ESP-Now Gateway   --   Using the W5500 module to create a hardwired ethernet connection

##  == WARNING ==
This is an alpha version of this code,released here in the hope that it will help others to get the basic hardware (ESP32 + WizNet W5500 Ethernet module) working.

## == LICENCE NOTE ==
Because this version of the code is released complete with Hristo Gochkov's SPI library (see lib directory), it is also released under the same GNU Lesser 2.1 licence.  Later versions will probably be released without the library and under a different licence.

### The code
This is a PlatformIO build (and if you haven't tried PlatformIO yet, you really should, it's great!) and is easily adaptable to the Arduino IDE, if that happens to be your weapon of choice.

The PlatformIO platformio.ini file will automatically download and install the correct Ethernet and PubSubClient libraries for you when you type "pio run" the first time.  The SPI library is already shipped in the lib directory, as that particular version wasn't generally available when I originally wrote the code (and it contains a coule of fixes essential to the ESP32 platform).

### What does it do?
At the moment, this is simply something to get you going with the ESP32 and the W5500 module.  It doesn't do anything much more than initialize the module and make an initial connection to your MQTT server (broker).

It does have some clunky hardware checks in the start-up code, so pay attention to the serial output from your ESP32 when you first run it; it will try to guide you to solutions for simple hardware issues.

You should be able to build on this code to implement any of the client/server samples in the /examples directory of the Ethernet library.

### Practical notes
This was written with the specific intention of building an ESP-Now to MQTT gateway, so there is no code on the ESP32 side for authenticating with or joining a local WiFi network (the premis of ESP-Now is that it is a fast, point-to-point connection without all of the niceties, or overhead, of a full WiFi network connection).  Adding back the WiFi stuff is extremely easy (basically, look at any networked, non-ESP-Now code and copy it).

Don't ask me where you should input your access-point logon credentials.  :-)

The physical connections between the ESP32 and W5500 are different, but simpler than the ESP8266 version (you don't need to buffer SCS chip-select signal on the ESP32).

-  **GPIO23**   <-->   **MOSI**
-  **GPIO19**   <-->   **MISO**
-  **GPIO18**   <-->   **SCLK**
-  **GPIO5**    <-->   **SCS**
-  **GPIO26**   <-->   **RESET**

As always, you do need a good solid DC supply for both boards and a good ground connection between them in addition to the data lines listed above.




