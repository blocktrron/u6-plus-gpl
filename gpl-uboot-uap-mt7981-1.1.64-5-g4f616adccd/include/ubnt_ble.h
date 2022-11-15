#ifndef _UBNT_BLE_H_
#define _UBNT_BLE_H_
#ifdef CONFIG_UBNT_APP
#define UUID128_SIZE              16

/* ADV */
typedef struct {
	uint8_t len;
	uint8_t ad_type;
	uint8_t ad_data[UUID128_SIZE];
} __attribute__((packed)) ad_element_uuid_t;

typedef struct {
	uint8_t len;
	uint8_t ad_type;
	uint8_t ad_service_uuid[2];
	uint8_t ad_data;
} __attribute__((packed)) ad_element_mode_t;

typedef struct {
	uint8_t hci_packet_type;
	uint8_t opcode[2];
	uint8_t total_len;
	uint8_t ad_data_len;    //  1,  1
	ad_element_uuid_t uuid; // 18, 19
	ad_element_mode_t mode; //  5, 24
	uint8_t reserved[8];    //  8, 32
} __attribute__((packed)) hci_le_adv_data_cmd_t;

/* Scan Response */
typedef struct {
	uint8_t len;
	uint8_t ad_type;
	uint8_t ad_service_uuid[2];
	uint8_t ad_data[ETH_ALEN];
} __attribute__((packed)) ad_element_mac_t;

typedef struct {
	uint8_t len;
	uint8_t ad_type;
	uint8_t ad_service_uuid[2];
	uint8_t time;
} __attribute__((packed)) ad_element_boottime_t;

typedef struct {
	uint8_t len;
	uint8_t ad_type;
	uint8_t ad_data[14];
} __attribute__((packed)) ad_element_name_t;

typedef struct {
	uint8_t hci_packet_type;
	uint8_t opcode[2];
	uint8_t total_len;
	uint8_t ad_data_len;             //  1,  1
	ad_element_mac_t mac;            // 10, 11
	ad_element_boottime_t boot_time; //  5, 16
	ad_element_name_t name;          // 16, 32
} __attribute__((packed)) hci_le_scan_rsp_data_cmd_t;

typedef struct {
	uint8_t ad_interval_min[2];
	uint8_t ad_interval_max[2];
	uint8_t ad_type;
	uint8_t own_type;
	uint8_t peer_type;
	uint8_t peer_addr[6];
	uint8_t channel_map;
	uint8_t policy;
} __attribute__((packed)) ad_param_t;

typedef struct {
	uint8_t hci_packet_type;
	uint8_t opcode[2];
	uint8_t total_len;
	ad_param_t param;
} __attribute__((packed)) hci_le_ad_param_cmd_t;

typedef struct {
	int mac_addr_increment;
	char * board_name;
	uint8_t factory_uuid[UUID128_SIZE];
	uint8_t config_uuid[UUID128_SIZE];
} ubnt_model_bt_t;
void do_ubnt_set_ble_data(void);
int ubnt_get_bt_mac(uint8_t*, int);
#endif

#ifdef CONFIG_CSR8811_BT_SUPPORT
void ubnt_set_csr8811_mac(const uint8_t*);
typedef struct {
	uint8_t pskey[2];
	uint8_t len[2];
	uint8_t stores[2];
	uint8_t data[8];
} __attribute__((packed)) ps_t;

typedef struct {
	uint8_t type[2];
	uint8_t len[2];
	uint8_t seqno[2];
	uint8_t varid[2];
	uint8_t status[2];
} __attribute__((packed)) bccmd_t;

typedef struct {
	uint8_t opcode[2];
	uint8_t total_len;
	uint8_t payload_desc;
	bccmd_t bccmd;
	ps_t    ps;
} __attribute__((packed)) csr_psset_t;

typedef struct {
	uint8_t opcode[2];
	uint8_t total_len;
	uint8_t payload_desc;
	bccmd_t bccmd;
	uint8_t reserved[8];
} __attribute__((packed)) csr_reset_t;
#endif /* CONFIG_CSR8811_BT_SUPPORT */

struct hci_cmd {
	unsigned char* data;
	unsigned int len;
};

extern struct hci_cmd hci_cmds[];
extern int hci_cmds_count;

#ifdef CONFIG_CSR8811_BT_SUPPORT
extern struct hci_cmd bccmd_init_cmds[];
extern int bccmd_init_cmds_count;
#endif
#endif /* _UBNT_BLE_H_ */
