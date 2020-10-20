/* ------------------ Your specific config -------------------------- */


/* ----------------- Network config --------------------------------- */
#define ENABLE_MAC_ADDRESS_ROM      true
#define MAC_I2C_ADDR          0x50
#define ENABLE_DHCP           true
byte ardunio_mac[] = {0xDE, 0x51, 0xAF, 0x24, 0x3E, 0x4E};
IPAddress ardunio_ip(000, 000, 000, 000);


/* ----------------- MQTT config ------------------------------------ */
IPAddress mqtt_broker(XXX, XXX, XXX, XXX);     //IP address of your MQTT Broker
String mqtt_name = "I2CButtons";
const char mqtt_user[] = "XXXX";              //MQTT UserName
const char mqtt_pass[] = "XXXX";              //MQTT Password
const char mqtt_status_topic[] = "status";
const char mqtt_publish_topic[] = "buttons";


/* ----------------- Watchdog config -------------------------------- */
#define ENABLE_WATCHDOG   true;
const char watchdog_pin = 3;



/* ----------------- Button config -------------------------------- */
#define DEBOUNCE_RATE   50




/* ----------------- Controller config -------------------------------- */
// If or when you're using multiple controllers
const byte controller_id = 1;

// Interval to report heartbeat
#define HEARTBEAT     60000         // Set time in ms for Heartbeat 60000 = 1Min
