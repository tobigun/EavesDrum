/****************************************************************************************************************************
  AsyncTCPClient.ino

  For RP2040W with CYW43439 WiFi

  AsyncTCP_RP2040W is a library for the RP2040W with CYW43439 WiFi

  Based on and modified from ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
  Built by Khoi Hoang https://github.com/khoih-prog/AsyncTCP_RP2040W
  Licensed under GPLv3 license
 *****************************************************************************************************************************/

#include <AsyncTCP_RP2040W.h>

char ssid[] = "your_ssid";        // your network SSID (name)
char pass[] = "12345678";         // your network password (use for WPA, or use as key for WEP), length must be 8+

int status = WL_IDLE_STATUS;

// Change the Server IPAddress accordingly
IPAddress serverIP(192, 168, 2, 128);

#define TCP_PORT              5698

#define CHECK_INTERVAL_MS     1000L         // Check connection
#define SEND_INTERVAL_MS      10000L        // delay between updates, in milliseconds

unsigned long lastCheck = SEND_INTERVAL_MS;         // last time you connected to the server, in milliseconds

AsyncClient* client = nullptr;

bool clientConnected = false;

bool dataReceived    = false;

#define REPLY_SIZE      64

static void replyToServer(void* arg)
{
  (void) arg;

  Serial.println("\n********************");
  Serial.println("New replyToServer");

  AsyncClient* client = reinterpret_cast<AsyncClient*>(arg);

  // send reply
  if (client->space() > REPLY_SIZE && client->canSend())
  {
    char message[REPLY_SIZE];
    sprintf(message, "This is from AsyncTCPClient @ %s", WiFi.localIP().toString().c_str());
    client->add(message, strlen(message));
    client->send();

    dataReceived = false;
  }
}

/* event callbacks */
static void handleData(void* arg, AsyncClient* client, void *data, size_t len)
{
  (void) arg;

  Serial.printf("\nData received from %s \n", client->remoteIP().toString().c_str());
  Serial.write((uint8_t*)data, len);

  lastCheck = millis();

  dataReceived = true;
}

void onConnect(void* arg, AsyncClient* client)
{
  (void) arg;

  clientConnected = true;

  Serial.printf("\nAsyncTCPClient has been connected to Server %s, port %d \n", serverIP.toString().c_str(), TCP_PORT);

  replyToServer(client);
}

void onDisconnect(void* arg, AsyncClient* client)
{
  (void) arg;
  (void) client;

  Serial.printf("\nAsyncTCPClient has been disconnected from Server %s, port %d \n", serverIP.toString().c_str(), TCP_PORT);

  clientConnected = false;
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("Local IP Address: ");
  Serial.println(ip);
}

bool connectServer()
{
  if (client)
    delete(client);

  client = new AsyncClient;

  if (client)
  {
    client->onData(&handleData, client);
    client->onConnect(&onConnect, client);

    client->onDisconnect(&onDisconnect, client);

    client->connect(serverIP, TCP_PORT);

    return true;
  }
  else
  {
    Serial.println("\nError, NULL client");

    return false;
  }
}

void setup()
{
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  delay(200);

  Serial.print("\nStart AsyncTCP_Client on ");
  Serial.print(BOARD_NAME);
  Serial.print(" with ");
  Serial.println(SHIELD_TYPE);
  Serial.println(ASYNCTCP_RP2040W_VERSION);

  ///////////////////////////////////

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");

    // don't continue
    while (true);
  }

  Serial.print(F("Connecting to SSID: "));
  Serial.println(ssid);

  status = WiFi.begin(ssid, pass);

  delay(1000);

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED)
  {
    delay(500);

    // Connect to WPA/WPA2 network
    status = WiFi.status();
  }

  printWifiStatus();

  ///////////////////////////////////

  connectServer();

  lastCheck = millis();
}

void loop()
{
  static unsigned long lastConnectCheck = CHECK_INTERVAL_MS;
  
  if (millis() - lastCheck > SEND_INTERVAL_MS)
  {
    if (clientConnected && dataReceived)
    {
      replyToServer(client);
    }
    else if ( !clientConnected || !dataReceived )
    {
      Serial.printf("\nReconnecting to Server %s, port %d \n", serverIP.toString().c_str(), TCP_PORT);

      connectServer();
    }

    lastCheck = millis();
  }
}
