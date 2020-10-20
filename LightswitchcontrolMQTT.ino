#include <Wire.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Adafruit_MCP23017.h>



/*--------------------------- CONFIGURATION ------------------------------*/
// Configuration should be done in the included file:
#include "config.h"



/*--------------------------- SETUP -------------------------------------*/
EthernetClient network_client;

PubSubClient mqtt_client(network_client);

Adafruit_MCP23017 mcps[6];

bool watchdog_status = false;
unsigned long watchdog_timer = 0;
unsigned long mqtt_timer = 0;
unsigned long heart_timer = 0;
char mqtt_buffer[100];
byte button_states[96] = {HIGH};
unsigned long button_timers[96] = {0};

/*------------------------------------------------------------------------*/



/**
 * Determine if given timer has reached given interval
 *
 * @param unsigned long tmr The current timer
 * @param int interval Length of time to run timer
 * @return bool True when timer is complete
 * @return bool False when timer is counting
 */
bool getTimer (unsigned long &tmr, int interval)
{
  // Set initial value
  if (tmr < 1)
  {
    tmr = millis();
  }

  // Determine difference of our timer against millis()
  if (millis() - tmr >= interval)
  {
    // Complete. Reset timer
    tmr = 0;
    return true;
  }

  // Still needs to "count"
  return false;
}

/**
 * Converts IP address to char array
 *
 * @param IPAddress address
 * @return char*
 */
char* ip2CharArray(IPAddress address)
{
    static char ip[16];
    sprintf(ip, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);
    return ip;
}

/**
 * Reconnect to MQTT broker
 * @return bool True on successful connection
 * @return bool False on failure to connect
 */
bool mqttReconnect()
{
  // Watchdog low incase we never reconnect
  #ifdef ENABLE_WATCHDOG
    digitalWrite(watchdog_pin, LOW);
  #endif

  // Too early to reconnect
  if (!getTimer(mqtt_timer, 1000))
  {
    return false;
  }

  // Connection successful
  if (mqtt_client.connect(String(mqtt_name + String(random(0xffff), HEX)).c_str(), mqtt_user, mqtt_pass))
  {
    // Publish json-string payload of connection status: event type, local IP, controller ID
    sprintf(mqtt_buffer, "{\"event\":\"reconnected\",\"ip\":\"%s\",\"controller\":\"%d\"}", ip2CharArray(Ethernet.localIP()), controller_id);
    mqtt_client.publish(mqtt_status_topic, mqtt_buffer);

    return true;
  }

  // Connection failed
  return false;
}




/**
 * Reset watch-dog module
 * Prevents Arduino from resetting if everything is fine
 *
 * @return void
 */
void resetWatchDog()
{
  if (getTimer(watchdog_timer, 1000))
  {
    watchdog_status = !watchdog_status;
    digitalWrite(watchdog_pin, watchdog_status);
  }
}



/**
 * Reads byte value from given register
 *
 * @param byte reg The register
 * @return byte The value at given register
 */
byte readRegister (byte reg)
{
  Wire.beginTransmission(MAC_I2C_ADDR);

  // Register to read
  Wire.write(reg);
  Wire.endTransmission();

  // Read a byte
  Wire.requestFrom(MAC_I2C_ADDR, 1);
  while(!Wire.available())
  {
    // Wait
  }

  return Wire.read();
}



/**
 * Determine given button's state and report if needed
 * 
 * @param signed char id Button's ID: 0-95
 * @param byte state Button's state
 * @return void
 */
void updateButton(signed char id, byte state)
{
  if (state != button_states[id] && getTimer(button_timers[id], DEBOUNCE_RATE))
  {
    button_states[id] = state;
  }
  else
  {
    return;
  }

  // Button is pressed
  if (!state)
  {
    // Publish json-string payload of button event: controller ID, button ID
    sprintf(mqtt_buffer, "{\"controller\":\"%d\",\"button\":\"%d\"}", controller_id, id);
    mqtt_client.publish(mqtt_publish_topic, mqtt_buffer);
  }
}



/**
 * Initial setup
 *
 * @return void
 */
void setup()
{
  Wire.begin();

  #ifdef ENABLE_MAC_ADDRESS_ROM
    ardunio_mac[0] = readRegister(0xFA);
    ardunio_mac[1] = readRegister(0xFB);
    ardunio_mac[2] = readRegister(0xFC);
    ardunio_mac[3] = readRegister(0xFD);
    ardunio_mac[4] = readRegister(0xFE);
    ardunio_mac[5] = readRegister(0xFF);
  #endif

  #ifdef ENABLE_DHCP
    // Wait for network connection
    while (Ethernet.begin(ardunio_mac) == 0)
    {
      delay(1000);
    }
  #else
    Ethernet.begin(ardunio_mac, ardunio_ip);
  #endif

  // Initializes the pseudo-random number generator
  randomSeed(micros());

  // Setup MQTT
  mqtt_client.setServer(mqtt_broker, 1883);

  // Setup pins
  signed char id = 0;
  for (signed char i = 0; i < 6; i++)
  {
    mcps[i].begin(i);

    for (signed char p = 0; p < 16; p ++)
    {
      mcps[i].pinMode(p, INPUT);
      mcps[i].pullUp(p, HIGH);
      id ++;
    }
  }
}


/**
 * Main program loop
 *
 * @return void
 */
void loop()
{
  // Keep network connection
  Ethernet.maintain();

  // Reconnect to MQTT server
  if (!mqtt_client.connected())
  {
    if (!mqttReconnect())
    {
      // Exit early of main loop
      return;
    }
  }

  // Check for incoming MQTT messages
  mqtt_client.loop();

  // Update each button's state
  signed char id = 0;
  for (signed char i = 0; i < 6; i++)
  {
    for (signed char p = 0; p < 16; p ++)
    {
      updateButton(id, mcps[i].digitalRead(p));
      id ++;
    }
  }

  // Heartbeat report
  if (getTimer(heart_timer, HEARTBEAT))
  {
    // Publish json-string payload of connection status: event type, local IP, controller ID
    sprintf(mqtt_buffer, "{\"event\":\"status\",\"ip\":\"%s\",\"controller\":\"%d\"}", ip2CharArray(Ethernet.localIP()), controller_id);
    mqtt_client.publish(mqtt_status_topic, mqtt_buffer);
  }

  // Prevent arduino from resetting if everything is fine
  #ifdef ENABLE_WATCHDOG
    resetWatchDog();
  #endif
}
