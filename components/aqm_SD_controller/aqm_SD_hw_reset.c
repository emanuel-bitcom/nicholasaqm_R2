#include "aqm_SD_hw_reset.h"

static void IRAM_ATTR gpio_isr_handler(void *args)
{
    esp_deep_sleep_start();
}

void init_hw_reset()
{
    /*add a 100ms sleep on interrupt to allow a debounce*/
    esp_sleep_enable_timer_wakeup(200000);
    gpio_set_direction(PIN_HW_RESET, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_HW_RESET, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_HW_RESET, gpio_isr_handler, (void *)PIN_HW_RESET);

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
    {
        /*    the hw_reset button was pressed and this current session
         *    must perform hw reset of
         *    config variables
         */
        load_root_to_config();
    }
}