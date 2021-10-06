#ifndef MAIN_H_
#define MAIN_H_

void scan_callback(ble_gap_evt_adv_report_t* report);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void bleuart_rx_callback(BLEClientUart& uart_svc);


#endif