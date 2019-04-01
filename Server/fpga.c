#include "fpga.h"
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include "stdio.h"


#include <sys/mman.h>
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"


//#include "hps_0.h"

// QSyS dependent address
#define FPGA_LED_PIO_BASE   0x10000
#define FPGA_KEY_PIO_BASE   0x10010
#define FPGA_SW_PIO_BASE    0x10040
#define FPGA_HEX_BASE       0x10060
#define FPGA_ADC_BASE	    0x00000
#define FPGA_GPIO_BASE 	    0x00060
#define FPGA_VIP_CTI_BASE   0x10080
#define FPGA_VIP_MIX_BASE   0x10100
#define FPGA_IR_RX_BASE     0x10200

//#define FPGA_ADC_SPI_BASE   0x40040



// ///////////////////////////////////////
// memory map

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

// ///////////////////////////////////////////////////
// SPI Micro
#define ALTERA_AVALON_SPI_RXDATA_REG                  0
#define ALTERA_AVALON_SPI_TXDATA_REG                  1
#define ALTERA_AVALON_SPI_STATUS_REG                  2
#define ALTERA_AVALON_SPI_CONTROL_REG                 3
#define ALTERA_AVALON_SPI_SLAVE_SEL_REG               5
#define IORD(base, index)                             (*( ((uint32_t *)base)+index))
#define IOWR(base, index, data)                       (*(((uint32_t *)base)+index) = data)
#define IORD_ALTERA_AVALON_SPI_RXDATA(base)           IORD(base, ALTERA_AVALON_SPI_RXDATA_REG)
#define IORD_ALTERA_AVALON_SPI_STATUS(base)           IORD(base, ALTERA_AVALON_SPI_STATUS_REG)
#define IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, data)  IOWR(base, ALTERA_AVALON_SPI_SLAVE_SEL_REG, data)
#define IOWR_ALTERA_AVALON_SPI_CONTROL(base, data)    IOWR(base, ALTERA_AVALON_SPI_CONTROL_REG, data)
#define IOWR_ALTERA_AVALON_SPI_TXDATA(base, data)     IOWR(base, ALTERA_AVALON_SPI_TXDATA_REG, data)

#define ALTERA_AVALON_SPI_STATUS_ROE_MSK              (0x8)
#define ALTERA_AVALON_SPI_STATUS_ROE_OFST             (3)
#define ALTERA_AVALON_SPI_STATUS_TOE_MSK              (0x10)
#define ALTERA_AVALON_SPI_STATUS_TOE_OFST             (4)
#define ALTERA_AVALON_SPI_STATUS_TMT_MSK              (0x20)
#define ALTERA_AVALON_SPI_STATUS_TMT_OFST             (5)
#define ALTERA_AVALON_SPI_STATUS_TRDY_MSK             (0x40)
#define ALTERA_AVALON_SPI_STATUS_TRDY_OFST            (6)
#define ALTERA_AVALON_SPI_STATUS_RRDY_MSK             (0x80)
#define ALTERA_AVALON_SPI_STATUS_RRDY_OFST            (7)
#define ALTERA_AVALON_SPI_STATUS_E_MSK                (0x100)
#define ALTERA_AVALON_SPI_STATUS_E_OFST               (8)

// end
// ///////////////////////////////////////////////////
bool FPGALedSet(int mask){
    if (!m_bInitSuccess)
        return false;

    //qDebug() << "FPGA:LedSet\r\n";
    *(uint32_t *)m_led_base = mask;
    return true;
}
bool newHexSet(int value) {
  printf("inside newHexSet");
  if (!m_bInitSuccess)
    return false;
  printf("the value is %d\n", value);
    *((uint32_t *)m_hex_base) = value;
  return true;
}
bool HexSet(int index, int value){
    if (!m_bInitSuccess)
        return false;

    uint8_t szMask[] = {
        63, 6, 91, 79, 102, 109, 125, 7,
        127, 111, 119, 124, 57, 94, 121, 113
    };

    if (value < 0)
        value = 0;
    else if (value > 15)
        value = 15;

    //qDebug() << "index=" << index << "value=" << value << "\r\n";

    *((uint32_t *)m_hex_base+index) = szMask[value];
    return true;
}
void HexName(){
  uint8_t szMap3[] = {
    121, 56, 121, 102, 63, 111
  };
  int i = 0;
  for(i = 0;i<6;i++){
    *((uint32_t *)m_hex_base+5-i) = szMap3[i];
  }
}
void hello_world(void){
	uint8_t szmap2[] = {
		116, 121, 56, 56, 63, 0, 62, 14, 63, 80, 56, 94, 0, 0
	};
	int i;
	int j;
	int temp;
	for(j = 0;j<14;j++){
		temp = j;
		for(i=0;i<6;i++){
			if(temp-1<0){
				*((uint32_t *)m_hex_base+i) = 0x00;
			} else {
				*((uint32_t *)m_hex_base+i) = szmap2[temp-1];
			}
			temp--;
		}
		usleep(500*1000);
	}
}
bool KeyRead(uint32_t *mask){
    if (!m_bInitSuccess)
        return false;

    *mask = *(uint32_t *)m_key_base;
    return true;

}
bool SwitchRead(uint32_t *mask){
    if (!m_bInitSuccess)
        return false;

    *mask = *(uint32_t *)m_sw_base;
    return true;

}
bool ReadADC(uint32_t *mask, int address){
    if (!m_bInitSuccess)
        return false;
		*mask = *((uint32_t *)m_adc_base+address);
    return true;

}
bool WriteADC(int mask, int channel){
    if (!m_bInitSuccess)
        return false;
		*((uint32_t *)m_adc_base+channel) = mask;
    return true;
}
bool VideoEnable(bool bEnable){
    if (!m_bInitSuccess)
        return false;

    const int nLayerIndex = 1;

    //qDebug() << "Video-In" << (bEnable?"Yes":"No") << "\r\n";

    IOWR(m_vip_cti_base, 0x00, bEnable?0x01:0x00);
    IOWR(m_vip_mix_base, nLayerIndex*3+1, bEnable?0x01:0x00);

    if (bEnable)
        VideoMove(0,0);

    m_bIsVideoEnabled = bEnable;

    return true;
}
bool VideoMove(int x, int y){
    if (!m_bInitSuccess)
        return false;

    const int nLayerIndex = 1;

    IOWR(m_vip_mix_base, nLayerIndex*3-1, x);
    IOWR(m_vip_mix_base, nLayerIndex*3+0, y);

    return true;
}
bool IsVideoEnabled(){
    return m_bIsVideoEnabled;
}
bool IrDataRead(uint32_t *scan_code){
    if (!m_bInitSuccess)
        return false;

    *scan_code = *(uint32_t *)m_ir_rx_base;

    return true;
}
bool IrIsDataReady(void){

    if (!m_bInitSuccess)
        return false;

    uint32_t status;
    status = *(((uint32_t *)m_ir_rx_base)+1);

    if (status)
        return true;
    return false;

}
bool FPGAInit()
{
    bool bSuccess = false;

    m_file_mem = open( "/dev/mem", ( O_RDWR | O_SYNC ) );
    if (m_file_mem != -1){
        void *virtual_base;
        virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, m_file_mem, HW_REGS_BASE );
        if (virtual_base == MAP_FAILED){
        }else{
            m_led_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_LED_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_key_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_KEY_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_sw_base = (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_SW_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_hex_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_HEX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_vip_cti_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_VIP_CTI_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            m_vip_mix_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_VIP_MIX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	    m_adc_base=(uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_ADC_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	    m_gpio_base=(uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_GPIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );

            m_ir_rx_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_IR_RX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            //m_adc_spi_base= (uint8_t *)virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_ADC_SPI_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
            bSuccess = true;
        }
        close(m_file_mem);
    }

	m_bInitSuccess = bSuccess;
    return bSuccess;
}
