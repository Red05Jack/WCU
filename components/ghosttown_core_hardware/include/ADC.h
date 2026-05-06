// ADC.h
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

class ADC {
public:
  ADC();
  ADC(adc_unit_t unit, adc_channel_t channel, adc_atten_t attenuation = ADC_ATTEN_DB_12);
  ~ADC();

  bool Init(adc_unit_t unit, adc_channel_t channel, adc_atten_t attenuation = ADC_ATTEN_DB_12);

  int ReadRaw() const;
  int ReadMilliVolts() const;

  bool IsValid() const;

private:
  bool InitCalibration();

private:
  adc_unit_t m_unit = ADC_UNIT_1;
  adc_channel_t m_channel = ADC_CHANNEL_0;
  adc_atten_t m_attenuation = ADC_ATTEN_DB_12;

  adc_oneshot_unit_handle_t m_adcHandle = nullptr;
  adc_cali_handle_t m_caliHandle = nullptr;

  bool m_isInitialized = false;
  bool m_hasCalibration = false;
};