#pragma once

#ifdef ESP32

#include "esp_a2dp_api.h"
#include "driver/i2s.h"
#include "freertos/queue.h"

namespace sound_tools {

enum I2SMode  { TX_MODE, RX_MODE };
const char* I2S_TAG = "I2S";
/**
 * @brief ESP32 specific configuration for all i2s settings
 * 
 */
template<typename T>
class I2SConfig {
  public:
    int port_no = 0;
    i2s_config_t i2s_config;
    i2s_pin_config_t pin_config;

    I2SConfig(I2SMode mode = TX) {
        i2s_config = defaultConfig(mode);
        pin_config = defaultPinConfig(mode);
    }

    I2SConfig(const I2SConfig<T> &cfg) {
      port_no = cfg.port_no;
      i2s_config = cfg.i2s_config;
      pin_config = cfg.pin_config;
    }

    I2SConfig(){
        i2s_config = defaultConfig(TX);
        pin_config = defaultPinConfig(TX);
    }


  protected:
      i2s_config_t defaultConfig(I2SMode mode = TX_MODE) {
      ESP_LOGD(I2S_TAG, "%s", __func__);
      i2s_config_t i2s_config = {
            .mode = (i2s_mode_t) ((mode == TX) ? (I2S_MODE_MASTER | I2S_MODE_TX) :  (I2S_MODE_MASTER | I2S_MODE_RX)) ,
            .sample_rate = 44100,
            .bits_per_sample = (i2s_bits_per_sample_t) (sizeof(T) * 8),
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // default interrupt priority
            .dma_buf_count = 8,
            .dma_buf_len = 512,
            .use_apll = false,
        };
        return i2s_config;
    }

    i2s_pin_config_t defaultPinConfig(I2SMode mode = TX) {
        ESP_LOGD(I2S_TAG, "%s - mode: %s", __func__, mode==TX ? "TX" : "RX");
        i2s_pin_config_t pin_config_const = {
            .bck_io_num = 26,
            .ws_io_num = 25,
            .data_out_num = mode == TX_MODE ? 22 : I2S_PIN_NO_CHANGE,
            .data_in_num = mode == RX_MODE ? 32 : I2S_PIN_NO_CHANGE
        };
       return pin_config_const;
    }
};


/**
 * @brief A Simple I2S interface class.
 */
template<typename T>
class I2S {
  public:
    I2S() {
    }

    ~I2S() {
      stop();
    }

    I2SConfig<T> defaultConfig(I2SMode mode) {
        ESP_LOGD(I2S_TAG, "%s", __func__);
        I2SConfig<T> config(mode);
        return config;
    }

    /// starts the DAC 
    void begin(I2SConfig<T> cfg) {
      ESP_LOGD(I2S_TAG, "%s", __func__);
      this->i2s_num = (i2s_port_t) cfg.port_no;            
      this->i2s_config = cfg.i2s_config;
      this->pin_config = cfg.pin_config;

      ESP_LOGD(I2S_TAG, "pin bck_io_num: %d", pin_config.bck_io_num);
      ESP_LOGD(I2S_TAG, "pin ws_io_num: %d", pin_config.ws_io_num);
      ESP_LOGD(I2S_TAG, "pin data_out_num: %d", pin_config.data_out_num);
      ESP_LOGD(I2S_TAG, "pin data_in_num: %d", pin_config.data_in_num);

      // setup config
      if (i2s_driver_install(i2s_num, &i2s_config, 0, NULL)!=ESP_OK){
        ESP_LOGE(I2S_TAG, "%s - %s", __func__, "i2s_driver_install");
      }      

      // setup pin config
      if (i2s_set_pin(i2s_num, &pin_config)!= ESP_OK){
        ESP_LOGE(I2S_TAG, "%s - %s", __func__, "i2s_set_pin");
      }

      i2s_zero_dma_buffer(i2s_num);

    }

    /// stops the I2C and unistalls the driver
    void stop(){
        ESP_LOGD(I2S_TAG, "%s", __func__);
        i2s_driver_uninstall(i2s_num);    
    }

    /// writes the data to the I2S interface
    size_t write(T (*src)[2], size_t sizeFrames, TickType_t ticks_to_wait=portMAX_DELAY){
      ESP_LOGD(I2S_TAG, "%s", __func__);
      return writeBytes(src, sizeFrames * sizeof(T) * 2, ticks_to_wait); // 2 bytes * 2 channels      
    }
    
    /// Reads data from I2S
    size_t read(T (*src)[2], size_t sizeFrames, TickType_t ticks_to_wait=portMAX_DELAY){
      size_t len = readBytes(src, sizeFrames * sizeof(T) * 2, ticks_to_wait); // 2 bytes * 2 channels     
      size_t result = len / (sizeof(T) * 2); 
      ESP_LOGD(I2S_TAG, "%s - len: %d -> %d", __func__,sizeFrames, result);
      return result;
    }
    
  protected:
    i2s_port_t i2s_num = (i2s_port_t) 0; // i2s port number
    i2s_pin_config_t pin_config;
    i2s_config_t i2s_config;
    
    /// writes the data to the I2S interface
    size_t writeBytes(const void *src, size_t size_bytes, TickType_t ticks_to_wait=portMAX_DELAY){
      size_t result = 0;            
      if (i2s_write(i2s_num, src, size_bytes, &result, ticks_to_wait)!=ESP_OK){
        ESP_LOGE(I2S_TAG, "%s", __func__);
      }
      return result;
    }

    size_t readBytes(void *dest, size_t size_bytes,  TickType_t ticks_to_wait=portMAX_DELAY){
      size_t result = 0;
      if (i2s_read(i2s_num, dest, size_bytes, &result, ticks_to_wait)!=ESP_OK){
        ESP_LOGE(I2S_TAG, "%s", __func__);
      }
      return result;
    }

};

}

#endif