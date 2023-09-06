#include "aqm_lte_controller.h"

/**
 * Choose HTTPS or MQTT
 */
// #define USE_HTTPS
#define USE_MQTT

QueueHandle_t uart_queue;
TaskHandle_t uart_rec_task;

bool data_read = false;
const int data_sent_done = BIT0;
const int lte_flag=BIT1;
char incoming_message[RX_BUF_SIZE];

// login data
char lte_apn[30]="iot.1nce.net";
char lte_user[30];
char lte_pass[30];

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

void init_modem(const char *apn, const char *user, const char *pass)
{

  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk= UART_SCLK_DEFAULT};

  uart_param_config(LTE_SERIAL_PERIPHERAL, &uart_config);
  uart_set_pin(LTE_SERIAL_PERIPHERAL, LTE_TX, LTE_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  uart_driver_install(LTE_SERIAL_PERIPHERAL, RX_BUF_SIZE, 0, 0, NULL, 0);

  // upload security certificate
  // upload_certificate();
  SEND_AT("AT\r\n");
  await_serial();
  if(strstr((const char*)incoming_message,"OK")==NULL){
    ESP_LOGE(TAG1, "LTE communication unavailable");
    xEventGroupClearBits(comm_available_evt_grp, lte_flag);
  } 
  else{
    ESP_LOGI(TAG1, "LTE communication available");
    xEventGroupSetBits(comm_available_evt_grp, lte_flag);
  }

  vTaskDelay(50);
}

static void init_https()
{

  ESP_LOGI(TAG1, "Start https configuration\n");
  lte_serial_write("AT+QIACT?\r\n", sizeof("AT+QIACT?\r\n"));
  await_serial();

  ESP_LOGI(TAG1, "Getting DNS data\n");
  lte_serial_write("AT+QIDNSCFG=1\r\n", sizeof("AT+QIDNSCFG=1\r\n"));
  await_serial();

  lte_serial_write("AT+QHTTPCFG=\"sslctxid\",1\r\n", sizeof("AT+QHTTPCFG=\"sslctxid\",1\r\n"));
  ESP_LOGI(TAG1, "SSLCTXID: \n");
  await_serial();

  lte_serial_write("AT+QSSLCFG=?\r\n", sizeof("AT+QSSLCFG=?\r\n"));
  ESP_LOGI(TAG1, "SSL range: \n");
  await_serial();

  lte_serial_write("AT+QSSLCFG=\"sslversion\",1,4\r\n", sizeof("AT+QSSLCFG=\"sslversion\",1,4\r\n"));
  ESP_LOGI(TAG1, "SSLversion: \n");
  await_serial();

  lte_serial_write("AT+QSSLCFG=\"sni\",1,1\r\n", sizeof("AT+QSSLCFG=\"sni\",1,1\r\n"));
  ESP_LOGI(TAG1, "SNI: \n");
  await_serial();

  lte_serial_write("AT+QSSLCFG=\"seclevel\",1,0\r\n", sizeof("AT+QSSLCFG=\"seclevel\",1,0\r\n"));
  ESP_LOGI(TAG1, "SecLevel: \n");
  await_serial();

  // add certificate to ssl context
  lte_serial_write("AT+QSSLCFG=\"cacert\",1,\"RAM:cacert.pem\"\r\n", sizeof("AT+QSSLCFG=\"cacert\",1,\"RAM:cacert.pem\"\r\n"));
  await_serial();
}

static void init_mqtt(mqtt_config_t *config, uint client_idx)
{
  const char clientid[10] = "station";
  char *AT_BUFFER = (char *)calloc(100, sizeof(char));

  ESP_LOGI(TAG1, "Start mqtt configuration\n");
  SEND_AT("AT+QIACT?\r\n");
  await_serial();

  // configure keepalive
  memset(AT_BUFFER, 0, 100);
  sprintf(AT_BUFFER, "AT+QMTCFG=\"keepalive\",%d,%d\r\n", (int)client_idx, (int)(config->keepalive));
  // sprintf(AT_BUFFER, "AT+QMTCFG=\"keepalive\",0,60\r\n");
  const char *_buf = (const char *)AT_BUFFER;
  ESP_LOGW(TAG1, "Cmd: %s", _buf);
  SEND_AT(_buf);
  await_serial();

  // test OPEN command
  SEND_AT("AT+QMTOPEN=?\r\n");
  await_serial();
  // read OPEN command
  SEND_AT("AT+QMTOPEN?\r\n");
  await_serial();

  // open connection to server
  memset(AT_BUFFER, 0, 100);
  sprintf(AT_BUFFER, "AT+QMTOPEN=%d,\"%s\",%d\r\n", client_idx, config->broker_ip, config->port);
  SEND_AT((const char *)AT_BUFFER);
  await_serial();
  await_serial();

  // connect to server
  memset(AT_BUFFER, 0, 100);
  sprintf(AT_BUFFER, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r\n", client_idx, clientid, config->username, config->password);
  SEND_AT((const char *)AT_BUFFER);
  await_serial();
  await_serial();
}

static void publish_on_mqtt(mqtt_config_t *config, uint client_idx, char *message)
{
  int msg_id = 0;
  char *AT_BUFFER = (char *)calloc(100, sizeof(char));

  // send publish message
  memset(AT_BUFFER, 0, 100);
  sprintf(AT_BUFFER, "AT+QMTPUBEX=%d,%d,%d,0,\"%s\",%d\r\n", client_idx, msg_id, config->qos, config->topic, strlen((const char *)message));
  SEND_AT((const char *)AT_BUFFER);
  await_serial();
  SEND_AT((const char *)message);
  await_serial();

  free(AT_BUFFER);
}

static void disconnect_mqtt(uint client_idx)
{
  char *AT_BUFFER = (char *)calloc(100, sizeof(char));

  // send disconnect message
  memset(AT_BUFFER, 0, 100);
  sprintf(AT_BUFFER, "AT+QMTDISC=%d\r\n", (int)client_idx);
  SEND_AT((const char *)AT_BUFFER);
  await_serial();
  await_serial();

  // close mqtt connection
  //  send disconnect message
  memset(AT_BUFFER, 0, 100);
  sprintf(AT_BUFFER, "AT+QMTCLOSE=%d\r\n", (int)client_idx);
  SEND_AT((const char *)AT_BUFFER);
  await_serial();
  await_serial();

  free(AT_BUFFER);
}

static void send_post_request(lte_cloud target_server, char *body)
{

  char *my_request_header_builder = (char *)calloc(250, sizeof(char));

  sprintf(my_request_header_builder, "POST %s HTTP/1.1\r\nHost: %s:%d\r\nAccept: */*\r\nUser-Agent: QUECTEL_MODULE\r\nConnection: keep-alive\r\nContent-Type: application/json\r\nContent-Length: ", (const char *)target_server.path, (const char *)target_server.server, target_server.port);

  const char *myRequestHeader = my_request_header_builder;

  uint16_t reqHead_len = strlen(myRequestHeader);

  printf("Last context reset gives:\n");
  // verify context id
  lte_serial_write("AT+QIACT?\r\n", sizeof("AT+QIACT?\r\n"));
  await_serial();

  printf("Beginning sending POST request\n");

  // set the URL to be accessed
  uint16_t url_len = strlen((const char *)target_server.server) + strlen("https://") + strlen(":443");
  const char *cmd_header = "AT+QHTTPURL=";
  uint8_t head_len = strlen(cmd_header);

  char *cmd;
  cmd = (char *)calloc(1, head_len + 10);
  // strcpy(cmd, cmd_header);
  sprintf(cmd, "%s%d,80", cmd_header, (url_len + 2));
  strcat(cmd, "\r");
  strcat(cmd, "\n");
  printf("%s", cmd);

  lte_serial_write((const char *)cmd, strlen((const char *)cmd));
  await_serial();

  // free "cmd" variable
  free(cmd);

  // send the actual URL
  char *url_complete = (char *)calloc(url_len + 3, sizeof(char));
  sprintf(url_complete, "https://%s:%d", target_server.server, target_server.port);
  strcat(url_complete, "\r");
  strcat(url_complete, "\n");

  lte_serial_write((const char *)url_complete, strlen((const char *)url_complete));
  await_serial();

  // free url memory
  free(url_complete);

  // beginning POST
  printf("Reading and beginning POST\n");

  printf("Verifying url setting: ");
  lte_serial_write("AT+QHTTPURL?\r\n", sizeof("AT+QHTTPURL?\r\n"));
  await_serial();

  // get length of body
  uint16_t body_len = 0;
  char var;
  char *body_pointer = body;
  var = *body_pointer;
  while ((var) != '\0')
  {
    body_pointer++;
    body_len++;
    var = *body_pointer;
  }

  const char *cmdP_header = "AT+QHTTPPOST=";
  uint8_t headP_len = strlen(cmdP_header);

  char *cmdP;
  cmdP = (char *)calloc(1, headP_len + 15);
  sprintf(cmdP, "%s%d,80,80", cmdP_header, (body_len + reqHead_len + 14));
  strcat(cmdP, "\r");
  strcat(cmdP, "\n");

  printf("%s\n", cmdP);
  lte_serial_write((const char *)cmdP, strlen((const char *)cmdP));
  await_serial();

  free(cmdP);

  vTaskDelay(100 / portTICK_PERIOD_MS);

  // sending actual POST body

  // show body on serial monitor
  printf("%s", myRequestHeader);
  printf("%d", body_len);
  printf("\r\n");
  printf("\r\n");
  printf("%s\n", body);

  vTaskDelay(100 / portTICK_PERIOD_MS);

  char *final_post = calloc(TX_BUF_SIZE, sizeof(char));
  sprintf(final_post, "%s%d\r\n\r\n%s                                             ", myRequestHeader, body_len, body);

  lte_serial_write((const char *)final_post, strlen((const char *)final_post));

  vTaskDelay(5000 / portTICK_PERIOD_MS);
  // waiting for response after POST
  await_serial();

  // HTTPS response
  await_long_serial();

  free(my_request_header_builder);
  free(final_post);
}

static void power_on()
{

#define EN_ON 0
#define EN_OFF 1

  gpio_set_direction(LTE_RES, GPIO_MODE_OUTPUT);
  gpio_set_level(LTE_RES, 0);

  gpio_set_direction(LTE_POW, GPIO_MODE_OUTPUT);
  gpio_set_level(LTE_POW, EN_OFF);

  gpio_set_direction(LTE_WKP, GPIO_MODE_OUTPUT);
  gpio_set_level(LTE_WKP, 1);

  // power the board
  gpio_set_level(LTE_RES, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(LTE_POW, EN_ON);
  vTaskDelay(5000 / portTICK_PERIOD_MS);

  // wake the board
  gpio_set_level(LTE_RES, 1);
  vTaskDelay(6000 / portTICK_PERIOD_MS);
  gpio_set_level(LTE_WKP, 0);
  vTaskDelay(6000 / portTICK_PERIOD_MS);
  gpio_set_level(LTE_WKP, 1);
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  ESP_LOGI(TAG1, "Quectel powered up");
}

static void power_off()
{
  // reset the board permanently
  gpio_set_level(LTE_RES, 0);
  vTaskDelay(100);

  // cut the power to save energy
  gpio_set_level(LTE_POW, 0);
  vTaskDelay(1000);

  ESP_LOGW(TAG1, "Quectel powered off");
}

static inline void lte_serial_write(const char *data_string, size_t data_len)
{
  uart_write_bytes(LTE_SERIAL_PERIPHERAL, data_string, data_len);
}

static void init_modem_params(const char *apn, const char *user, const char *pass)
{
  lte_serial_write("AT+QIDEACT=1\r\n", sizeof("AT+QIDEACT=1\r\n"));
  await_serial();

  lte_serial_write("AT+CMEE=2\r\n", sizeof("AT+CMEE=2\r\n"));
  await_serial();

  // lte_serial_write("AT+QHTTPCFG=\"contextid\",1\r\n", sizeof("AT+QHTTPCFG=\"contextid\",1\r\n"));
  // await_serial();

  // lte_serial_write("AT+QHTTPCFG=\"responseheader\",1\r\n", sizeof("AT+QHTTPCFG=\"responseheader\",1\r\n"));
  // await_serial();

  // lte_serial_write("AT+QHTTPCFG=\"requestheader\",1\r\n", sizeof("AT+QHTTPCFG=\"requestheader\",1\r\n"));
  // await_serial();

  char *cmd;
  cmd = (char *)malloc(100);
  const char *start_string = "AT+QICSGP=1,1,\"";
  strcpy(cmd, start_string);
  strcat(cmd, apn);
  strcat(cmd, "\",\"");
  strcat(cmd, user);
  strcat(cmd, "\",\"");
  strcat(cmd, pass);
  strcat(cmd, "\",1");
  strcat(cmd, "\r\n");

  const char *final_cmd = (const char *)cmd;
  lte_serial_write(final_cmd, strlen(final_cmd));
  await_serial();

  lte_serial_write("AT+QIACT=1\r\n", sizeof("AT+QIACT=1\r\n"));
  await_serial();

  lte_serial_write("AT+QIACT?\r\n", sizeof("AT+QIACT?\r\n"));
  await_serial();

  lte_serial_write("AT+QIDNSCFG=1,\"8.8.8.8\",\"8.8.4.4\"\r\n", sizeof("AT+QIDNSCFG=1,\"8.8.8.8\",\"8.8.4.4\"\r\n"));
  await_serial();
}



static inline void await_serial()
{
  int length = 0;

  /*await should have a timeout in cycles (equivalates to 120 secs)*/
  int timeout_cycles=60000;

  while ((!length)&&(timeout_cycles--))
  {
    ESP_ERROR_CHECK(uart_get_buffered_data_len(LTE_SERIAL_PERIPHERAL, (size_t *)&length));
    vTaskDelay(2 / portTICK_PERIOD_MS);
  }
  printf("\n received: ");

  /*await should have a timeout in cycles*/
  timeout_cycles=200;

  while (length&&(timeout_cycles--))
  {
    memset(incoming_message, 0, sizeof(incoming_message));
    uart_read_bytes(LTE_SERIAL_PERIPHERAL, (uint8_t *)incoming_message, length, pdMS_TO_TICKS(500));
    printf("%s", incoming_message);
    esp_err_t ret = uart_get_buffered_data_len(LTE_SERIAL_PERIPHERAL, (size_t *)&length);
    if (ret != ESP_OK)
    {
      break;
    }
  }
  uart_flush(LTE_SERIAL_PERIPHERAL);
}

static void await_long_serial()
{
  char incoming_message[RX_BUF_SIZE];

  memset(incoming_message, 0, sizeof(incoming_message));
  uart_read_bytes(LTE_SERIAL_PERIPHERAL, (uint8_t *)incoming_message, RX_BUF_SIZE, pdMS_TO_TICKS(5000));
  printf("received: %s\n", incoming_message);
}

void setup_lte()
{
  // now retrieve LTE connection data from nvs

  lte_cloud target_clouds[MAX_SUPPORTED_SERVERS];
  
  size_t nvs_read_len = sizeof(lte_data);

  lte_data current_lte_config;
  memset(&current_lte_config,0, sizeof(current_lte_config));
  /**
   * Retrieve lte configuration from nvs
  */
  esp_err_t ret_err_lte_cfg = nvs_high_read_config_generic(NVS_KEY_CONFIG_LTE, (void*)&current_lte_config,sizeof(current_lte_config));

  if(ret_err_lte_cfg==ESP_OK){
    // store configuration more conveniently
  strcpy(lte_apn, (const char *)(current_lte_config.lte_apn));
  strcpy(lte_user, (const char *)(current_lte_config.lte_user));
  strcpy(lte_pass, (const char *)(current_lte_config.lte_pass));
  }


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

  power_on();

  vTaskDelay(6000 / portTICK_PERIOD_MS);

  init_modem(lte_apn, lte_user, lte_pass);

  vTaskDelay(300 / portTICK_PERIOD_MS);
  power_off();
}

void aqm_mqtt_lte_send_data()
{

  xEventGroupWaitBits(comm_available_evt_grp, lte_flag, false, true, portMAX_DELAY);

  /*make sure to execute only when lte_flag was set*/
    EventBits_t uxReturn = xEventGroupGetBits(comm_available_evt_grp);
    if ((uxReturn & lte_flag) == 0)
        return;

  power_on();

  vTaskDelay(6000 / portTICK_PERIOD_MS);

  char *data_body;

  build_body_as_JSON(&data_body);

  init_modem_params(lte_apn, lte_user, lte_pass);

#ifdef USE_MQTT
  uint mqtt_client_idx = 0;
  init_mqtt(&bitcom_aws_config, mqtt_client_idx);
  publish_on_mqtt(&bitcom_aws_config, mqtt_client_idx, data_body);
  disconnect_mqtt(mqtt_client_idx);

#endif

  // finally signal that send is complete -- no need in production
  //xEventGroupSetBits(data_sent_evt_grp, 0xff);

  vTaskDelay(300 / portTICK_PERIOD_MS);
  power_off();
}

void upload_certificate()
{

  extern const unsigned char certStart[] asm("_binary_cert4_cer_start");
  extern const unsigned char certEnd[] asm("_binary_cert4_cer_end");

  // check if it exists
  if (certStart == NULL || certEnd == NULL || ((certEnd - certStart) == 0))
  {
    ESP_LOGE(TAG1, "HTTPS certificate not found, cannot continue");
    return;
  }

  // now upload certificate
  uint32_t certSize = certEnd - certStart;
  ESP_LOGI(TAG1, "Uploading cert file to quectel:");

  // char certSizeStr[8];
  // sprintf(&certSizeStr[0], "%d\r\n", certSize);

  // give certificate length info
  char command[40];
  sprintf(command, "AT+QFUPL=\"RAM:cacert.pem\",%d\r\n", (int)certSize);
  lte_serial_write((const char *)command, strlen((const char *)command));
  await_serial();

  // upload the actual certificate
  uint8_t by = 0;
  uint8_t *cert_pointer = certStart;
  do
  {
    by = *cert_pointer;
    if (by != '\0')
    {
      uart_write_bytes(LTE_SERIAL_PERIPHERAL, (const uint8_t *)&by, sizeof(by));
      printf("%c", by);
    }
    cert_pointer++;
  } while (by != '\0');
}

void build_body_as_JSON(char **result_json)
{

  temp_data retrieved_temp;
  toxic_data retrieved_toxic;
  opc_data retrieved_opc;
  station_cal retrieved_sta_cal;
  size_t retrieved_len;

  bool data_retrieved = false;

  while (!data_retrieved)
  {

    nvs_handle aqm_handle;

    if (xSemaphoreTake(sensors_data_m, 0) == pdTRUE)
    {

      ESP_ERROR_CHECK(nvs_flash_init_partition("temp_nvs"));

      esp_err_t result = nvs_open_from_partition("temp_nvs", "store", NVS_READWRITE, &aqm_handle);

      switch (result)
      {
      case ESP_OK:

        break;

      default:
        ESP_LOGE(TAG1, "Error: %s", esp_err_to_name(result));
        break;
      }

      size_t retrieved_len_1 = sizeof(temp_data);
      size_t retrieved_len_2 = sizeof(toxic_data);
      size_t retrieved_len_3 = sizeof(opc_data);

      ESP_ERROR_CHECK(nvs_get_blob(aqm_handle, "aqm_temperature", (void *)&retrieved_temp, &retrieved_len_1));
      ESP_ERROR_CHECK(nvs_get_blob(aqm_handle, "aqm_toxic", (void *)&retrieved_toxic, &retrieved_len_2));
      ESP_ERROR_CHECK(nvs_get_blob(aqm_handle, "aqm_opc", (void *)&retrieved_opc, &retrieved_len_3));

      data_retrieved = true;

      nvs_close(aqm_handle);

      xSemaphoreGive(sensors_data_m);
    }
    else
    {
      printf("JSON BUILDER cannot read from memory\n");
      printf("Trying again in 5 seconds\n");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }

  // now get the configuration data
  data_retrieved = false;

  while (!data_retrieved)
  {

    nvs_handle aqm_handle;

    if (xSemaphoreTake(sensors_data_m, 0) == pdTRUE)
    {

      ESP_ERROR_CHECK(nvs_flash_init_partition("config_nvs"));

      esp_err_t result = nvs_open_from_partition("config_nvs", "store", NVS_READWRITE, &aqm_handle);

      switch (result)
      {
      case ESP_OK:

        break;

      default:
        ESP_LOGE(TAG1, "Error: %s", esp_err_to_name(result));
        break;
      }

      size_t retrieved_len_4 = sizeof(station_cal);

      ESP_ERROR_CHECK(nvs_get_blob(aqm_handle, "aqm_sta_cal", (void *)&retrieved_sta_cal, &retrieved_len_4));

      data_retrieved = true;

      nvs_close(aqm_handle);

      xSemaphoreGive(sensors_data_m);
    }
    else
    {
      printf("JSON BUILDER could not read station config from memory\n");
      printf("Trying again in 5 seconds\n");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
  }

  // NOW COMPOSE BODY JSON

  cJSON *json_body = cJSON_CreateObject();
  cJSON_AddNumberToObject(json_body, "SN", retrieved_sta_cal.station_SN);
  cJSON_AddNumberToObject(json_body, "is_new_data", 1);

  // create data element
  cJSON *json_data = cJSON_CreateObject();

  cJSON *temp_json = cJSON_CreateObject();
  cJSON_AddNumberToObject(temp_json, "env", retrieved_temp.temp_env);
  cJSON_AddNumberToObject(temp_json, "sta", retrieved_temp.temp_sta);
  cJSON_AddItemToObject(json_data, "temp", temp_json);

  cJSON *humid_json = cJSON_CreateObject();
  cJSON_AddNumberToObject(humid_json, "env", retrieved_temp.humid_env);
  cJSON_AddNumberToObject(humid_json, "sta", retrieved_temp.humid_sta);
  cJSON_AddItemToObject(json_data, "humid", humid_json);

  cJSON *pm_json = cJSON_CreateObject();
  cJSON_AddNumberToObject(pm_json, "pmA", retrieved_opc.pmA);
  cJSON_AddNumberToObject(pm_json, "pmB", retrieved_opc.pmB);
  cJSON_AddNumberToObject(pm_json, "pmC", retrieved_opc.pmC);
  cJSON_AddItemToObject(json_data, "pm", pm_json);

  cJSON *tox_mV_data = cJSON_CreateObject();
  cJSON_AddNumberToObject(tox_mV_data, "NO_W", retrieved_toxic.NO2_W);
  cJSON_AddNumberToObject(tox_mV_data, "SO2_W", retrieved_toxic.SO2_W);
  cJSON_AddNumberToObject(tox_mV_data, "CO_W", retrieved_toxic.CO_W);
  cJSON_AddNumberToObject(tox_mV_data, "Ox_W", retrieved_toxic.O3_W);
  cJSON_AddNumberToObject(tox_mV_data, "NO_A", retrieved_toxic.NO2_A);
  cJSON_AddNumberToObject(tox_mV_data, "SO2_A", retrieved_toxic.SO2_A);
  cJSON_AddNumberToObject(tox_mV_data, "CO_A", retrieved_toxic.CO_A);
  cJSON_AddNumberToObject(tox_mV_data, "Ox_A", retrieved_toxic.O3_A);
  cJSON_AddItemToObject(json_data, "tox_mV", tox_mV_data);

  // create calibration json element
  cJSON *calib_json = cJSON_CreateObject();

  cJSON *tox_mV_calib = cJSON_CreateObject();
  cJSON_AddNumberToObject(tox_mV_calib, "NO2_WEe", retrieved_sta_cal.NO2_cal.WEe);
  cJSON_AddNumberToObject(tox_mV_calib, "NO2_AEe", retrieved_sta_cal.NO2_cal.AEe);
  cJSON_AddNumberToObject(tox_mV_calib, "NO2_A0", retrieved_sta_cal.NO2_cal.AE0);
  cJSON_AddNumberToObject(tox_mV_calib, "NO2_W0", retrieved_sta_cal.NO2_cal.WE0);
  cJSON_AddNumberToObject(tox_mV_calib, "NO2_S", retrieved_sta_cal.NO2_cal.sensit);

  cJSON_AddNumberToObject(tox_mV_calib, "SO2_WEe", retrieved_sta_cal.SO2_cal.WEe);
  cJSON_AddNumberToObject(tox_mV_calib, "SO2_AEe", retrieved_sta_cal.SO2_cal.AEe);
  cJSON_AddNumberToObject(tox_mV_calib, "SO2_A0", retrieved_sta_cal.SO2_cal.AE0);
  cJSON_AddNumberToObject(tox_mV_calib, "SO2_W0", retrieved_sta_cal.SO2_cal.WE0);
  cJSON_AddNumberToObject(tox_mV_calib, "SO2_S", retrieved_sta_cal.SO2_cal.sensit);

  cJSON_AddNumberToObject(tox_mV_calib, "CO_WEe", retrieved_sta_cal.CO_cal.WEe);
  cJSON_AddNumberToObject(tox_mV_calib, "CO_AEe", retrieved_sta_cal.CO_cal.AEe);
  cJSON_AddNumberToObject(tox_mV_calib, "CO_A0", retrieved_sta_cal.CO_cal.AE0);
  cJSON_AddNumberToObject(tox_mV_calib, "CO_W0", retrieved_sta_cal.CO_cal.WE0);
  cJSON_AddNumberToObject(tox_mV_calib, "CO_S", retrieved_sta_cal.CO_cal.sensit);

  cJSON_AddNumberToObject(tox_mV_calib, "O3_WEe", retrieved_sta_cal.O3_cal.WEe);
  cJSON_AddNumberToObject(tox_mV_calib, "O3_AEe", retrieved_sta_cal.O3_cal.AEe);
  cJSON_AddNumberToObject(tox_mV_calib, "O3_A0", retrieved_sta_cal.O3_cal.AE0);
  cJSON_AddNumberToObject(tox_mV_calib, "O3_W0", retrieved_sta_cal.O3_cal.WE0);
  cJSON_AddNumberToObject(tox_mV_calib, "O3_S", retrieved_sta_cal.O3_cal.sensit);
  cJSON_AddNumberToObject(tox_mV_calib, "O3_S2", retrieved_sta_cal.O3_cal.NO2_sensit);
  cJSON_AddNumberToObject(tox_mV_calib, "O3_G", retrieved_sta_cal.O3_cal.gain);

  cJSON_AddItemToObject(calib_json, "tox_mV", tox_mV_calib);

  cJSON *gps_calib = cJSON_CreateObject();
  cJSON_AddNumberToObject(gps_calib, "lat", retrieved_sta_cal.pos_latitude);
  cJSON_AddNumberToObject(gps_calib, "long", retrieved_sta_cal.pos_longitude);
  cJSON_AddItemToObject(calib_json, "gps", gps_calib);

  // add data to big json
  cJSON_AddItemToObject(json_body, "data", json_data);
  cJSON_AddItemToObject(json_body, "calib", calib_json);

  char *json_body_string = cJSON_PrintUnformatted(json_body);

  *result_json = json_body_string;

  cJSON_Delete(json_body);

  printf("Body to be sent is: \n %s\n", json_body_string);
}