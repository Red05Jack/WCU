#include "ADC.h"

ADC::ADC() {
}

ADC::ADC(adc_unit_t unit, adc_channel_t channel, adc_atten_t attenuation) {
  Init(unit, channel, attenuation);
}

ADC::~ADC() {
  if (m_caliHandle != nullptr) {
    adc_cali_delete_scheme_curve_fitting(m_caliHandle);
    m_caliHandle = nullptr;
  }

  if (m_adcHandle != nullptr) {
    adc_oneshot_del_unit(m_adcHandle);
    m_adcHandle = nullptr;
  }

  m_isInitialized = false;
  m_hasCalibration = false;
}

bool ADC::Init(adc_unit_t unit, adc_channel_t channel, adc_atten_t attenuation) {
  m_unit = unit;
  m_channel = channel;
  m_attenuation = attenuation;

  adc_oneshot_unit_init_cfg_t unitConfig = {};
  unitConfig.unit_id = m_unit;

  if (adc_oneshot_new_unit(&unitConfig, &m_adcHandle) != ESP_OK) {
    return false;
  }

  adc_oneshot_chan_cfg_t channelConfig = {};
  channelConfig.atten = m_attenuation;
  channelConfig.bitwidth = ADC_BITWIDTH_DEFAULT;

  if (adc_oneshot_config_channel(m_adcHandle, m_channel, &channelConfig) != ESP_OK) {
    adc_oneshot_del_unit(m_adcHandle);
    m_adcHandle = nullptr;
    return false;
  }

  m_hasCalibration = InitCalibration();
  m_isInitialized = true;

  return true;
}

bool ADC::InitCalibration() {
  adc_cali_curve_fitting_config_t caliConfig = {};
  caliConfig.unit_id = m_unit;
  caliConfig.chan = m_channel;
  caliConfig.atten = m_attenuation;
  caliConfig.bitwidth = ADC_BITWIDTH_DEFAULT;

  return adc_cali_create_scheme_curve_fitting(&caliConfig, &m_caliHandle) == ESP_OK;
}

int ADC::ReadRaw() const {
  if (!m_isInitialized || m_adcHandle == nullptr) {
    return -1;
  }

  int rawValue = 0;

  if (adc_oneshot_read(m_adcHandle, m_channel, &rawValue) != ESP_OK) {
    return -1;
  }

  return rawValue;
}

int ADC::ReadMilliVolts() const {
  if (!m_isInitialized || m_adcHandle == nullptr) {
    return -1;
  }

  const int rawValue = ReadRaw();

  if (rawValue < 0) {
    return -1;
  }

  if (!m_hasCalibration || m_caliHandle == nullptr) {
    return -1;
  }

  int voltageMv = 0;

  if (adc_cali_raw_to_voltage(m_caliHandle, rawValue, &voltageMv) != ESP_OK) {
    return -1;
  }

  return voltageMv;
}

bool ADC::IsValid() const {
  return m_isInitialized && m_adcHandle != nullptr;
}