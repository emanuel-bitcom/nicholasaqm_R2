#include "aqm_eth_controller.h"

static const char *TAG = "ETH_DRIVER";
const int eth_flag = BIT0;

/*Private variables*/
static esp_mqtt_client_handle_t client;
//***********************************************************************
// mqtt config struct
static mqtt_config_t bitcom_aws_config = {
    .broker_ip = "3.74.167.151",
    .port = 1883,
    .keepalive = 60,
    .username = "bitcom",
    .password = "aqmbitcom2023",
    .topic = "aqm_station/nicholas_aqm",
    .qos = 0};
//***********************************************************************

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

        /*We should signal here that ethernet is UP and other comm should not be used*/
        xEventGroupSetBits(comm_available_evt_grp, eth_flag);

        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        /*We should signal here that ethernet is DOWN and other comm should be used*/
        xEventGroupClearBits(comm_available_evt_grp, eth_flag);
        mqtt_app_stop();

        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // start mqtt client
    mqtt_app_start();
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        /*Signal that an mqtt connection is available*/
        xEventGroupSetBits(comm_available_evt_grp, mqtt_flag);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

        /*Signal that mqtt connection was broken*/
        xEventGroupClearBits(comm_available_evt_grp, mqtt_flag);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    /*Create uri*/
    char uri_buffer[150];
    memset(uri_buffer,0,150);
    
    sprintf(uri_buffer,"mqtt://%s:%s@%s:%d",
                (const char*)bitcom_aws_config.username,
                (const char*)bitcom_aws_config.password,
                (const char*)bitcom_aws_config.broker_ip,
                bitcom_aws_config.port);

    /**
     * Debug
    */
    ESP_LOGI(TAG,"mqtt uri: \"%s\"",(const char*)uri_buffer);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = (const char*)uri_buffer
        // .broker.address.port = 1883,
        // .broker.address.hostname = CONFIG_BROKER_URL,
        // .credentials.username = "bitcom",
        // .credentials.authentication.password = "aqmbitcom2023"
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void mqtt_app_stop(void)
{
    esp_mqtt_client_stop(client);
    vTaskDelay(5);
    esp_mqtt_client_destroy(client);
}

void aqm_mqtt_eth_publish_data()
{
    xEventGroupWaitBits(comm_available_evt_grp, eth_flag | mqtt_flag, false, true, portMAX_DELAY);

    /*make sure to execute only when eth_flag was set*/
    EventBits_t uxReturn = xEventGroupGetBits(comm_available_evt_grp);
    if ((uxReturn & eth_flag) == 0)
        return;

    char *data_to_send;
    int msg_id;

    build_body_as_JSON(&data_to_send);
    msg_id = esp_mqtt_client_publish(client, "aqm_station/nicholas_aqm", (const char *)data_to_send, (int)strlen((const char *)data_to_send), 0, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

    /*generate next measurings -- no needed in production*/
    // xEventGroupSetBits(data_sent_evt_grp, 0xff);
}

void init_ETH()
{

    /**
     * Set log level
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    /**
     * Retrieve mqtt connectionn data from nvs
     */
    mqtt_conn_config_t eth_mqtt_conn_config;
    memset(&eth_mqtt_conn_config, 0, sizeof(eth_mqtt_conn_config));
    esp_err_t ret_err = nvs_high_read_config_generic(NVS_KEY_CONFIG_MQTT, (void *)&eth_mqtt_conn_config, sizeof(eth_mqtt_conn_config));

    if (ret_err == ESP_OK)
    {
        memset(bitcom_aws_config.broker_ip, 0, sizeof(bitcom_aws_config.broker_ip));
        memset(bitcom_aws_config.password, 0, sizeof(bitcom_aws_config.password));
        memset(bitcom_aws_config.username, 0, sizeof(bitcom_aws_config.username));
        strcpy(bitcom_aws_config.broker_ip, eth_mqtt_conn_config.url);
        bitcom_aws_config.port = eth_mqtt_conn_config.port;
        strcpy(bitcom_aws_config.password, eth_mqtt_conn_config.password);
        strcpy(bitcom_aws_config.username, eth_mqtt_conn_config.username);
    }

    /*ISR service is already installed*/
    //ESP_ERROR_CHECK(gpio_install_isr_service(0));

    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize TCP/IP network interface (should be called only once in application)
    // if(!is_netif_init)ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    // if(!is_eventloop_init)ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

    spi_bus_config_t buscfg = {
        .miso_io_num = AQM_MISO_PIN,
        .mosi_io_num = AQM_MOSI_PIN,
        .sclk_io_num = AQM_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    /*SPI was previously configured*/
    // ESP_ERROR_CHECK(spi_bus_initialize(AQM_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t spi_devcfg = {
        .mode = 0,
        .clock_speed_hz = 8 * 1000 * 1000,
        .spics_io_num = ETH_CS_PIN,
        .queue_size = 20,
        .cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(8),
    };

    eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(AQM_SPI_HOST, &spi_devcfg);
    enc28j60_config.int_gpio_num = ETH_INT_PIN;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1;     // ENC28J60 doesn't have a pin to reset internal PHY
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

    /* ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
       02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    mac->set_addr(mac, (uint8_t[]){
                           0x02, 0x00, 0x00, 0x12, 0x34, 0x46});

    // ENC28J60 Errata #1 check
    if (emac_enc28j60_get_chip_info(mac) < ENC28J60_REV_B5 && 8 < 8)
    {
        ESP_LOGE(TAG, "SPI frequency must be at least 8 MHz for chip revision less than 5");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    /* It is recommended to use ENC28J60 in Full Duplex mode since multiple errata exist to the Half Duplex mode */

    eth_duplex_t duplex = ETH_DUPLEX_FULL;
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_DUPLEX_MODE, &duplex));

    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}