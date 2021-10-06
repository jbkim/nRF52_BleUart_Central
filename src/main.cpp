/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/*
 * This sketch demonstrate the central API(). A additional bluefruit
 * that has bleuart as peripheral is required for the demo.
 */
#include <bluefruit.h>
#include "main.h"

// #define CONNECT_BY_NAME

// * Local Name: "Adafruit Bluefruit LE" 
// Convert String  https://codebeautify.org/string-hex-converter
uint8_t DEVICE_LOCAL_NAME[] = {0x41,0x64,0x61,0x66,0x72,0x75,0x69,0x74,0x20,0x42,0x6c,0x75,0x65,0x66,0x72,0x75,0x69,0x74,0x20,0x4c,0x45};  

BLEClientBas  clientBas;  // battery client
BLEClientDis  clientDis;  // device information client
BLEClientUart clientUart; // bleuart client

void setup()
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 Central BLEUART Example");
  Serial.println("-----------------------------------\n");
  
  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 1);
  
  Bluefruit.setName("Bluefruit52 Central");

  // Configure Battyer client
  clientBas.begin();  

  // Configure DIS client
  clientDis.begin();

  // Init BLE Central Uart Serivce
  clientUart.begin();
  clientUart.setRxCallback(bleuart_rx_callback);

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(250);

  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Interval = 100 ms, window = 80 ms
   * - Don't use active scan
   * - Start(timeout) with timeout = 0 will scan forever (until connected)
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);                   // // 0 = Don't stop scanning after n seconds
}

/* Prints a hex list to the Serial Monitor */
void printHexList(uint8_t* buffer, uint8_t len)
{  
  // print forward order
  for(int i=0; i<len; i++)
  { 
    Serial.printf("%02X-", buffer[i]);
  } 
  Serial.println();  
}

void printReport( const ble_gap_evt_adv_report_t* report )
{
  int j;

  Serial.print( "  rssi: " );
  Serial.println( report->rssi );
  Serial.print( "  scan_rsp: " );
  Serial.println( report->type.scan_response );
//  Serial.print( "  type: " );
//  Serial.println( report->type );
  
  Serial.print( "  peer_addr: " );

  for(j = 0; j < 6; j++)  
    Serial.printf( "%02X ",report->peer_addr.addr[j]);
  Serial.println("");    
  
  // Serial.print( "  dlen: " );
  // Serial.println( report->data.len );  
  // Serial.print( "  data: " );
  // for( int i = 0; i < report->data.len; i+= sizeof(uint8_t) )
  // {
  //   Serial.printf( "%02X-", report->data.p_data[ i ] );
  // }
  Serial.println(""); 
}


/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void scan_callback(ble_gap_evt_adv_report_t* report)
{

#ifdef CONNECT_BY_NAME
  /* Choose a peripheral to connect with by searching for an advertisement packet with a 
  Complete Local Name matching our target device*/
  uint8_t buffer[BLE_GAP_ADV_SET_DATA_SIZE_MAX] = { 0 };

  // Serial.print("Parsing report for Local Name ... ");
  // if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, sizeof(buffer)))
  if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, buffer, sizeof(buffer)))  
  {
    Serial.println("Found Local Name");
    Serial.printf("%14s %s\n", "Local Name:", buffer);

    // Serial.print("   Local Name data: ");
    // printHexList(buffer, BLE_GAP_ADV_SET_DATA_SIZE_MAX ); 

    Serial.print("Determining Local Name Match ... ");
    if ( !memcmp( buffer, DEVICE_LOCAL_NAME, sizeof(DEVICE_LOCAL_NAME)) )
    {
      Serial.println("Local Name Match!");

      Serial.println("Connecting to Peripheral ... ");
      Bluefruit.Central.connect(report);
    }
    else
    {
      Serial.println("No Match");
      Bluefruit.Scanner.resume(); // continue scanning
    } 
  } 
  else
  {
    // Serial.println("Failed");

    // For Softdevice v6: after received a report, scanner will be paused
    // We need to call Scanner resume() to continue scanning
    Bluefruit.Scanner.resume();
  }  

#else
  // Check if advertising contain BleUart service
  if ( Bluefruit.Scanner.checkReportForService(report, clientUart) )
  {
    Serial.print("BLE UART service detected. Connecting ... ");

    printReport( report );     

    // Connect to device with bleuart service in advertising
    // Bluefruit.Central.connect(report);
  }else
  {      
    // For Softdevice v6: after received a report, scanner will be paused
    // We need to call Scanner resume() to continue scanning
    Bluefruit.Scanner.resume();
  }
#endif  
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");

  Serial.print("Dicovering Device Information ... ");
  if ( clientDis.discover(conn_handle) )
  {
    Serial.println("Found it");
    char buffer[32+1];
    
    // read and print out Manufacturer
    memset(buffer, 0, sizeof(buffer));
    if ( clientDis.getManufacturer(buffer, sizeof(buffer)) )
    {
      Serial.print("Manufacturer: ");
      Serial.println(buffer);
    }

    // read and print out Model Number
    memset(buffer, 0, sizeof(buffer));
    if ( clientDis.getModel(buffer, sizeof(buffer)) )
    {
      Serial.print("Model: ");
      Serial.println(buffer);
    }

    Serial.println();
  }else
  {
    Serial.println("Found NONE");
  }

  Serial.print("Dicovering Battery ... ");
  if ( clientBas.discover(conn_handle) )
  {
    Serial.println("Found it");
    Serial.print("Battery level: ");
    Serial.print(clientBas.read());
    Serial.println("%");
  }else
  {
    Serial.println("Found NONE");
  }

  Serial.print("Discovering BLE Uart Service ... ");
  if ( clientUart.discover(conn_handle) )
  {
    Serial.println("Found it");

    Serial.println("Enable TXD's notify");
    clientUart.enableTXD();

    Serial.println("Ready to receive from peripheral");
  }else
  {
    Serial.println("Found NONE");
    
    // disconnect since we couldn't find bleuart service
    Bluefruit.disconnect(conn_handle);
  }  
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

/**
 * Callback invoked when uart received data
 * @param uart_svc Reference object to the service where the data 
 * arrived. In this example it is clientUart
 */
void bleuart_rx_callback(BLEClientUart& uart_svc)
{
  Serial.print("[RX]: ");
  
  while ( uart_svc.available() )
  {
    Serial.print( (char) uart_svc.read() );
  }

  Serial.println();
}

void loop()
{
  if ( Bluefruit.Central.connected() )
  {
    // Not discovered yet
    if ( clientUart.discovered() )
    {
      // Discovered means in working state
      // Get Serial input and send to Peripheral
      if ( Serial.available() )
      {
        delay(2); // delay a bit for all characters to arrive
        
        char str[20+1] = { 0 };
        Serial.readBytes(str, 20);
        
        clientUart.print( str );
      }
    }
  }
}
