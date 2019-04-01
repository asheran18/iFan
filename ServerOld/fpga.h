#ifndef FPGA_H_
#define FPGA_H_
#include "stdbool.h"
#include "stdint.h"
bool FPGALedSet(int mask);
bool HexSet(int index, int value);
bool KeyRead(uint32_t *mask);
bool SwitchRead(uint32_t *mask);
bool ReadADC(uint32_t *mask, int address);
bool WriteADC(int mask, int channel);
bool VideoEnable(bool bEnable);
bool VideoMove(int x, int y);
bool IsVideoEnabled();
bool IrDataRead(uint32_t *scan_code);
bool IrIsDataReady(void);
bool m_bInitSuccess;
int m_file_mem;
bool m_bIsVideoEnabled;
uint8_t *m_led_base;
uint8_t *m_hex_base;
uint8_t *m_key_base;
uint8_t *m_sw_base;
uint8_t *m_ir_rx_base;
uint8_t *m_adc_spi_base;
uint8_t *m_vip_cti_base;
uint8_t *m_vip_mix_base;
uint8_t *m_adc_base;
uint8_t * m_gpio_base;
bool FPGAInit();
void hello_world();
void HexName();
bool newHexSet(int value);
#endif /*FPGA_H_*/
