#include "stm32_spi_bootloader_client.h"
static uint8_t _buffer[256];
extern unsigned char stm32_firmware_section$$Base;
extern unsigned char stm32_firmware_section$$Length;

const uint8_t *stm32_firmware = (uint8_t*)&stm32_firmware_section$$Base;
const uint32_t stm32_firmware_size = (uint32_t)&stm32_firmware_section$$Length;

const uint32_t STM32_FLASH_ADDRESS = 0x08000000;

const uint32_t BLOCK_SIZE = 256;

static uint8_t _stm32_spi_write_read_byte(const uint8_t tx) {

    uint8_t rx = 0;

    
    //bsp_spi_write_read(&tx, &rx, sizeof(rx));

    return rx;
}

static uint8_t _stm32_power_on(void) {

    //bsp_pwr1v8_on();
}

static void _stm32_nss_select(void) {

    //bsp_spi_cs_set_low();
}

static void _stm32_nss_release(void) {

    //bsp_spi_cs_set_high();
}

static void _delay_ms(uint32_t ms) {

    //bsp_delay_ms(ms);
}

int main(void) {

    /* Initialize peripherals */
    //bsp_init();

    _stm32_nss_select();
    _stm32_power_on();
    _delay_ms(20);


    DEBUG("STM32 Enter to SPI Bootloader mode...");
    if(ssbc_connect(_stm32_spi_write_read_byte) == SSBC_RESULT_OK) {

        uint8_t version = ssbc_get_version();
        uint16_t chipID = ssbc_get_chip_id();

        DEBUG("SUCCESS\r\nVersion: 0x%04X\r\nChipID:  0x%04X\r\n", version, chipID);

        ssbc_read(bl_buffer, STM32_FLASH_ADDRESS, 8);

        const uint32_t *w=(uint32_t*) bl_buffer;

        if( w[0] == 0xFFFFFFFF &&  w[1] == 0xFFFFFFFF) {
            DEBUG("Memory is blank\r\n");
        } else {

            uint32_t start_erase_time = bsp_get_tick_ms();
            DEBUG("Time = %d\r\n", start_erase_time);
            DEBUG("Start Erase MCU...");

            if(ssbc_erase_all() == SSBC_RESULT_OK) {
                DEBUG("SUCCESS\r\n");
            } else {
                DEBUG("Error\r\n");
            }

            DEBUG("Erase Time = %d\r\n", bsp_get_tick_ms() - start_erase_time);
        }

        const uint32_t start_time = bsp_get_tick_ms();

        const uint32_t total_blocks = ((stm32_firmware_size/BLOCK_SIZE) + 1);
        DEBUG("\r\nFirmware size %08X\r\n", stm32_firmware_size);
            
        for(uint32_t i = 0; i<total_blocks; i++) {

            const uint32_t offset = BLOCK_SIZE*i;
repeat:
            DEBUG("Write block %d/%d\r", i, total_blocks);
            if(ssbc_write(STM32_FLASH_ADDRESS + offset, &stm32_firmware[offset], BLOCK_SIZE) != SSBC_RESULT_OK) {
                DEBUG("\nError write, try again\r\n");
                goto repeat;
            }

            if(ssbc_read(bl_buffer, STM32_FLASH_ADDRESS + offset, BLOCK_SIZE)!= SSBC_RESULT_OK) {
                DEBUG("\nError read, try again\r\n");
                goto repeat;
            }

            if(memcmp(bl_buffer, &stm32_firmware[offset], BLOCK_SIZE)) {
                DEBUG("\nVerify Error\r\n");
                goto repeat;
            }
        }

        DEBUG("Write complete! Size %d KB, Time %d\r\n", stm32_firmware_size /1024, (bsp_get_tick_ms() - start_time));

        ssbc_go(STM32_FLASH_ADDRESS);
        _stm32_nss_release();

        DEBUG("\nOk\r\n");

    } else {
        DEBUG("Error\r\n");
    }

}
