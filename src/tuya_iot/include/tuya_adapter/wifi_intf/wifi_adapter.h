#include "wifi_hwl.h"

int wifi_scan_networks(SACN_AP_RESULT_S* scan_res);

int wifi_scan_assign_networks(SACN_AP_RESULT_S* scan_res);

int wifi_set_channel(BYTE_T chan);

int wifi_get_channel(BYTE_T *chan);

int wifi_set_promisc(xr_promisc_t enable);

int wifi_is_link_up(struct netif * netif);

int wifi_get_mac_address(uint8_t *mac, int mac_len);

int wifi_set_mac_address(uint8_t *mac, int mac_len);

int wifi_rf_on();

int wifi_rf_off();

int wifi_on(enum wlan_mode mode);

int wifi_off();

int wifi_connect(CONST CHAR_T *ssid,CONST CHAR_T *passwd);

int wifi_disconnect();

int wifi_is_connected_to_ap();

int wifi_ap_start(CONST WF_AP_CFG_IF_S *cfg);

int wifi_ap_stop();

int wifi_get_rssi(int* rssi);




