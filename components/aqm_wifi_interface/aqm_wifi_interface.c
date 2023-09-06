#include <aqm_wifi_interface.h>

TaskHandle_t start_wifi_handle;
TaskHandle_t main_wifi_task_handle;

QueueHandle_t interuptQueue;

const int wifi_interface_done = BIT3;

static const char *TAG = "SERVER";

#define AMX_APs 20

static httpd_handle_t server = NULL;

/*Variable in which configuration from nvs is received*/
wifi_aqm_config_t NVS_wifi_config;

const char *uname = "nicholasaqm";
const char *pwd = "bitcom2023";

const char *wifi_ssid = "NicholasAQM";
const char *wifi_pwd = "bitcom2023";

/**
 * login is global so once login is successfull, the interface must be stopped to logout
 */
bool login = false;

void init_wifi_interface()
{

  // delay a little bit so that errors are minimized
  vTaskDelay(100 / portTICK_PERIOD_MS);

  /**
   * WiFi proccess starts rightaway
   * with no interrupt buttons
   */
  // start main_wifi_task
  xTaskCreatePinnedToCore(
      main_wifi_fun,
      "main wifi interface process",
      5000,
      NULL,
      1,
      &main_wifi_task_handle,
      app_cpu_wifi);
}

void start_mdns_service()
{
  mdns_init();

  // access by "nicholasaqm.local"
  mdns_hostname_set("nicholasaqm");
  mdns_instance_name_set("Measure your air");
}

// this is the main function serving wifi interface
void main_wifi_fun(void *arg)
{
  /*Read WiFi configuration from nvs*/
  memset((void *)&NVS_wifi_config, 0x00, sizeof(NVS_wifi_config)); // make sure that structure is empty before writing to it
  nvs_high_read_wifi(&NVS_wifi_config);
  if (!strlen(NVS_wifi_config.wifi_ssid))
  {
    ESP_LOGW(TAG, "Using default ssid");
    /*Use default ssid and password*/
    strcat(NVS_wifi_config.wifi_ssid, wifi_ssid);
    strcat(NVS_wifi_config.wifi_pwd, wifi_pwd);
  }
  if (!strlen(NVS_wifi_config.web_uname))
  {
    ESP_LOGW(TAG, "Using default web auth");
    /*Use default uname and password*/
    strcat(NVS_wifi_config.web_uname, uname);
    strcat(NVS_wifi_config.web_pwd, pwd);
  }

  ESP_ERROR_CHECK(nvs_flash_init());
  wifi_init();
  wifi_connect_ap(NVS_wifi_config.wifi_ssid, NVS_wifi_config.wifi_pwd);
  start_mdns_service();
  init_server();

  while (true)
  {
    vTaskDelay(1000);
  };

  {
    vTaskDelete(NULL);
  }
}

void deinit_wifi_interface()
{
  httpd_stop(server);
  mdns_service_remove_all();
  wifi_disconnect();
}

static void init_server()
{

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.recv_wait_timeout = 7;
  config.send_wait_timeout = 7;

  ESP_ERROR_CHECK(httpd_start(&server, &config));

  httpd_uri_t check_uname_url = {
      .uri = "/api/check_uname",
      .method = HTTP_POST,
      .handler = on_check_uname_url};
  httpd_register_uri_handler(server, &check_uname_url);

  httpd_uri_t interval_config_url = {
      .uri = "/api/interval_config",
      .method = HTTP_POST,
      .handler = on_interval_config_url};
  httpd_register_uri_handler(server, &interval_config_url);

  httpd_uri_t mqtt_config_url = {
      .uri = "/api/mqtt_config",
      .method = HTTP_POST,
      .handler = on_mqtt_config_url};
  httpd_register_uri_handler(server, &mqtt_config_url);

  httpd_uri_t lte_config_url = {
      .uri = "/api/lte_config",
      .method = HTTP_POST,
      .handler = on_lte_config_url};
  httpd_register_uri_handler(server, &lte_config_url);

  httpd_uri_t lorawan_config_url = {
      .uri = "/api/lorawan_config",
      .method = HTTP_POST,
      .handler = on_lorawan_config_url};
  httpd_register_uri_handler(server, &lorawan_config_url);

  httpd_uri_t finish_config_url = {
      .uri = "/api/finish_config",
      .method = HTTP_POST,
      .handler = on_finish_config_url};
  httpd_register_uri_handler(server, &finish_config_url);

  httpd_uri_t default_url = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = on_default_url};
  httpd_register_uri_handler(server, &default_url);
}

static esp_err_t on_default_url(httpd_req_t *req)
{
  // first make sure user is logged in

  ESP_LOGI(TAG, "URL: %s", req->uri);

  esp_vfs_spiffs_conf_t esp_vfs_spiffs_conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};
  esp_vfs_spiffs_register(&esp_vfs_spiffs_conf);

  char path[600];
  if (strcmp(req->uri, "/") == 0)
  {
    strcpy(path, "/spiffs/index.html");
  }
  else
  {
    sprintf(path, "/spiffs%s", req->uri);
  }
  char *ext = strrchr(path, '.');
  if (strcmp(ext, ".css") == 0)
  {
    httpd_resp_set_type(req, "text/css");
  }
  if (strcmp(ext, ".js") == 0)
  {
    httpd_resp_set_type(req, "text/javascript");
  }
  if (strcmp(ext, ".png") == 0)
  {
    httpd_resp_set_type(req, "image/png");
  }
  if (strcmp(ext, ".svg") == 0)
  {
    httpd_resp_set_type(req, "image/svg+xml");
  }

  FILE *file = fopen(path, "r");
  if (file == NULL)
  {
    httpd_resp_send_404(req);
    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
  }

  char lineRead[256];
  while (fgets(lineRead, sizeof(lineRead), file))
  {
    httpd_resp_sendstr_chunk(req, lineRead);

    // clear lineRead
    for (uint16_t i = 0; i < 256; i++)
    {
      lineRead[i] = 0;
    }
  }
  httpd_resp_sendstr_chunk(req, NULL);
  fclose(file);

  esp_vfs_spiffs_unregister(NULL);
  return ESP_OK;
}

static esp_err_t on_check_uname_url(httpd_req_t *req)
{
  char buffer[150];
  memset(&buffer, 0, sizeof(buffer));
  httpd_req_recv(req, buffer, req->content_len);

  /**
   * process the receieved buffer
   */
  char *buffer_uname_start = strstr((const char *)buffer, "uname=") + sizeof("uname=") - 1;
  char *buffer_uname_end = strstr((const char *)buffer, "&pwd=");
  char parsed_uname[30];
  memset(parsed_uname, 0, 30);
  strncpy(parsed_uname, (const char *)buffer_uname_start, buffer_uname_end - buffer_uname_start);

  char *buffer_pwd_start = buffer_uname_end + sizeof("&pwd=") - 1;
  char parsed_pwd[30];
  memset(parsed_pwd, 0, 30);
  strcpy(parsed_pwd, (const char *)buffer_pwd_start);
  /**
   * debug
   */
  {
    printf("%s \n", buffer);
    printf("%s \n", parsed_uname);
    printf("%s \n", parsed_pwd);
  }

  int compare1 = strcmp((const char *)parsed_uname, (const char *)NVS_wifi_config.web_uname);
  int compare2 = strcmp((const char *)parsed_pwd, (const char *)NVS_wifi_config.web_pwd);

  /**
   * debug
   */
  {
    printf("compare1: %d, compare2:%d \n", compare1, compare2);
  }

  if ((compare1 == 0) && (compare2 == 0))
  {
    login = true;
  }

  /**
   * start spiffs to send response
   */
  esp_vfs_spiffs_conf_t esp_vfs_spiffs_conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};
  esp_vfs_spiffs_register(&esp_vfs_spiffs_conf);

  char path[150];
  memset(path, 0, 150);

  if (!login)
  {
    httpd_resp_set_status(req, "200");

    /**
     * send login failed path
     */
    sprintf(path, "/spiffs/intern/login_fail.html");
  }
  else
  {
    httpd_resp_set_status(req, "200");

    /**
     * send login success path
     */
    sprintf(path, "/spiffs/intern/login_success.html");
  }

  FILE *file = fopen(path, "r");
  if (file == NULL)
  {
    httpd_resp_send_404(req);
    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
  }

  char lineRead[256];
  while (fgets(lineRead, sizeof(lineRead), file))
  {
    httpd_resp_sendstr_chunk(req, lineRead);

    // clear lineRead
    for (uint16_t i = 0; i < 256; i++)
    {
      lineRead[i] = 0;
    }
  }
  httpd_resp_sendstr_chunk(req, NULL);
  fclose(file);

  esp_vfs_spiffs_unregister(NULL);
  return ESP_OK;
}

static esp_err_t on_interval_config_url(httpd_req_t *req)
{
  if (!login)
  {
    return send_spiffs_resource("/spiffs/intern/login_fail.html", req);
  }

  /**
   * section to save specific data
   */
  char buffer[150];
  memset(&buffer, 0, sizeof(buffer));
  httpd_req_recv(req, buffer, req->content_len);
  char *p_interval = get_value_from_post_string("interval", buffer);
  char *p_repetitions = get_value_from_post_string("repetitions", buffer);

  /*if there is an error*/
  if (!p_interval || !p_repetitions)
  {
    printf("Data not received correctly");
    send_spiffs_resource("/spiffs/intern/data_not_saved.html", req);
    return ESP_OK;
  }

  /**
   * debug: print received data
   */
  ESP_LOGI(TAG, "interval: %s \nrepetitions: %s\n", p_interval, p_repetitions);

  /**
   * Save data to nvs
   */
  interval_config_t wifi_interval_config;
  memset(&wifi_interval_config, 0, sizeof(wifi_interval_config));
  /*get old data from nvs*/
  nvs_high_read_interval(&wifi_interval_config);
  /*modify the updated fields*/
  if (strlen(p_interval))
  {
    int i_interval = atoi((const char *)p_interval);
    wifi_interval_config.interval = i_interval;
    ESP_LOGI(TAG, "Interval was updated to: %d", i_interval);
  }
  if (strlen(p_repetitions))
  {
    int i_repetitions = atoi((const char *)p_repetitions);
    wifi_interval_config.repetitions = i_repetitions;
    ESP_LOGI(TAG, "Repetitions was updated to: %d", i_repetitions);
  }
  /*write updated structure to memory*/
  nvs_high_write_interval(&wifi_interval_config);

  /**
   * free memory
   */
  if (p_interval)
    free(p_interval);
  if (p_repetitions)
    free(p_repetitions);
  send_spiffs_resource("/spiffs/intern/data_saved.html", req);
  return ESP_OK;
}

static esp_err_t on_mqtt_config_url(httpd_req_t *req)
{
  if (!login)
  {
    return send_spiffs_resource("/spiffs/intern/login_fail.html", req);
  }

  /**
   * section to save specific data
   */
  char buffer[150];
  memset(&buffer, 0, sizeof(buffer));
  httpd_req_recv(req, buffer, req->content_len);
  char *p_URL = get_value_from_post_string("url", buffer);
  char *p_port = get_value_from_post_string("port", buffer);
  char *p_uname = get_value_from_post_string("user", buffer);
  char *p_pwd = get_value_from_post_string("pwd", buffer);

  if (!(p_URL && p_port && p_uname && p_pwd))
  {
    ESP_LOGE(TAG, "Data not received correctly");
    send_spiffs_resource("/spiffs/intern/data_not_saved.html", req);
    return ESP_OK;
  }

  /**
   * Save data to nvs
   */
  mqtt_conn_config_t wifi_mqtt_config;
  memset(&wifi_mqtt_config, 0, sizeof(wifi_mqtt_config));
  /*get old data from nvs*/
  nvs_high_read_config_generic(NVS_KEY_CONFIG_MQTT, &wifi_mqtt_config, sizeof(wifi_mqtt_config));
  /*update relevand fields*/
  if (strlen(p_URL))
  {
    memset(wifi_mqtt_config.url, 0, sizeof(wifi_mqtt_config.url));
    strcpy(wifi_mqtt_config.url, (const char *)p_URL);
  }
  if (strlen(p_port))
  {
    int i_port = atoi((const char *)p_port);
    wifi_mqtt_config.port = i_port;
  }
  if (strlen(p_uname))
  {
    memset(wifi_mqtt_config.username, 0, sizeof(wifi_mqtt_config.username));
    strcpy(wifi_mqtt_config.username, (const char *)p_uname);
  }
  if (strlen(p_pwd))
  {
    memset(wifi_mqtt_config.password, 0, sizeof(wifi_mqtt_config.password));
    strcpy(wifi_mqtt_config.password, (const char *)p_pwd);
  }

  /*write updated structure to memory*/
  nvs_high_write_config_generic(NVS_KEY_CONFIG_MQTT, (void *)&wifi_mqtt_config, sizeof(wifi_mqtt_config));

  /**
   * free memory
   */
  if(p_URL)free(p_URL);
  if(p_port)free(p_port);
  if(p_uname)free(p_uname);
  if(p_pwd)free(p_pwd);

  send_spiffs_resource("/spiffs/intern/data_saved.html", req);
  return ESP_OK;
}

static esp_err_t on_lte_config_url(httpd_req_t *req)
{
  if (!login)
  {
    return send_spiffs_resource("/spiffs/intern/login_fail.html", req);
  }

  /**
   * section to save specific data
   */
  char buffer[150];
  memset(&buffer, 0, sizeof(buffer));
  httpd_req_recv(req, buffer, req->content_len);
  char *p_apn=get_value_from_post_string("apn", buffer);
  char *p_user=get_value_from_post_string("user", buffer);
  char *p_pwd=get_value_from_post_string("pwd", buffer);

  if (!(p_apn && p_user && p_pwd))
  {
    ESP_LOGE(TAG, "Data not received correctly");
    send_spiffs_resource("/spiffs/intern/data_not_saved.html", req);
    return ESP_OK;
  }

  /*save data to nvs*/
  lte_data wifi_lte_data;
  memset(&wifi_lte_data, 0, sizeof(wifi_lte_data));
  /*get old data from nvs*/
  nvs_high_read_config_generic(NVS_KEY_CONFIG_LTE, (void*)&wifi_lte_data, sizeof(wifi_lte_data));

  /*update relevant fields*/
  if(strlen(p_apn)){
    memset(wifi_lte_data.lte_apn,0,sizeof(wifi_lte_data.lte_apn));
    strcpy(wifi_lte_data.lte_apn, (const char*)p_apn);
  }
  /*user can be an empty field*/
    if(true){
    memset(wifi_lte_data.lte_user,0,sizeof(wifi_lte_data.lte_user));
    strcpy(wifi_lte_data.lte_user, (const char*)p_user);
  }
  /*password can be an empty field*/
    if(true){
    memset(wifi_lte_data.lte_pass,0,sizeof(wifi_lte_data.lte_pass));
    strcpy(wifi_lte_data.lte_pass, (const char*)p_pwd);
  }
  /*Write updated structure to memory*/
  nvs_high_write_config_generic(NVS_KEY_CONFIG_LTE, (void*)&wifi_lte_data, sizeof(wifi_lte_data));

  /**
   * Free memory
  */
  if(p_apn)free(p_apn);
  if(p_user)free(p_user);
  if(p_pwd)free(p_pwd);

  send_spiffs_resource("/spiffs/intern/data_saved.html", req);
  return ESP_OK;
}

static esp_err_t on_lorawan_config_url(httpd_req_t *req)
{
  if (!login)
  {
    return send_spiffs_resource("/spiffs/intern/login_fail.html", req);
  }

  /**
   * section to save specific data
   */

  send_spiffs_resource("/spiffs/intern/data_saved.html", req);
  return ESP_OK;
}

static esp_err_t on_finish_config_url(httpd_req_t *req)
{
  if (!login)
  {
    return send_spiffs_resource("/spiffs/intern/login_fail.html", req);
  }

  /**
   * section to save specific data
   */

  send_spiffs_resource("/spiffs/intern/finished_config.html", req);

  /**
   * signal finish of wifi interface
   */
  xEventGroupSetBits(sensors_evt_grp, wifi_interface_done);
  return ESP_OK;
}

/**
 * Path is a spiffs path, so it starts with "/spiffs/"
 */
static esp_err_t send_spiffs_resource(char *path, httpd_req_t *req)
{
  /**
   * start spiffs to send response
   */
  esp_vfs_spiffs_conf_t esp_vfs_spiffs_conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};
  esp_vfs_spiffs_register(&esp_vfs_spiffs_conf);

  FILE *file = fopen(path, "r");
  if (file == NULL)
  {
    httpd_resp_send_404(req);
    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
  }

  /*file on path is found*/
  httpd_resp_set_status(req, "200");

  char lineRead[256];
  while (fgets(lineRead, sizeof(lineRead), file))
  {
    httpd_resp_sendstr_chunk(req, lineRead);

    // clear lineRead
    memset(lineRead, 0, 256);
  }
  httpd_resp_sendstr_chunk(req, NULL);
  fclose(file);

  esp_vfs_spiffs_unregister(NULL);
  return ESP_OK;
}

/**
 * in a post string: key1=value1&key2=value2&key3=value3
 * get a pointer to "value1" for "key1"
 * key should be variable name without "="
 */
static char *get_value_from_post_string(const char *key, const char *post_str)
{
  char *p_val = strstr(post_str, key);
  if (p_val)
    p_val += (strlen(key) + 1);
  else
    return NULL;
  /*assume a value not longer that 49 characters*/
  char *ret_ptr = (char *)malloc(50 * sizeof(char));
  char *ret_ptr_start = ret_ptr;
  for (uint8_t i = 0; i < 100 && *p_val != '&' && *p_val != '\0'; *ret_ptr++ = *p_val++, i++)
    ;
  *ret_ptr = '\0';
  return ret_ptr_start;
}