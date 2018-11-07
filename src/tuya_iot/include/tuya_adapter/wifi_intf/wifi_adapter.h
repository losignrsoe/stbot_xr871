#include "wifi_hwl.h"

#define WPA_FLAGS_WPA				WPA_BIT(0)
#define WPA_FLAGS_WPA2				WPA_BIT(1)
#define WPA_FLAGS_WEP				WPA_BIT(2)
#define WPA_FLAGS_WPS_PBC			WPA_BIT(3)
#define WPA_FLAGS_WPS_AUTH			WPA_BIT(4)
#define WPA_FLAGS_WPS_PIN			WPA_BIT(5)
#define WPA_FLAGS_WPS				WPA_BIT(6)
#define WPA_FLAGS_IBSS				WPA_BIT(7)
#define WPA_FLAGS_ESS				WPA_BIT(8)


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

int get_wep_security(int *is_wep_security, char *ssid, int ssid_len);

int wifi_wep_connect(CONST CHAR_T *ssid,CONST CHAR_T *passwd);

void wifi_ps_enable();



