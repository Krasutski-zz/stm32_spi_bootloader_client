#ifndef __STM32_SPI_BOOTLOADER_CLIENT_H__
#define __STM32_SPI_BOOTLOADER_CLIENT_H__

#include <stdint.h>
#include <stdbool.h>

#define ERASE_ALL    0xFFFFU
#define ERASE_BANK1  0xFFFEU
#define ERASE_BANK2  0xFFFDU

typedef uint8_t(*spi_tx_rx_byte_t)(uint8_t tx);

typedef enum ssbc_result_e {
    SSBC_RESULT_OK,
    SSBC_RESULT_ERROR,
    SSBC_RESULT_TIMEOUT,
}ssbc_result_t;

ssbc_result_t ssbc_connect(spi_tx_rx_byte_t fn);

/* Information functions */
uint16_t ssbc_get(uint8_t *data, uint16_t max_buffer_size);
uint8_t ssbc_get_version(void);
uint16_t ssbc_get_chip_id(void);

/* IO functions */
ssbc_result_t ssbc_read(uint8_t *data, uint32_t address, uint16_t size);
ssbc_result_t ssbc_write(uint32_t address, const uint8_t *data, uint16_t size);

/* Protection control */
ssbc_result_t ssbc_readout_unprotect(void);
ssbc_result_t ssbc_readout_protect(void);
ssbc_result_t ssbc_write_unprotect(void);
ssbc_result_t ssbc_write_protect(uint8_t *sectors_list, uint16_t count);

ssbc_result_t ssbc_erase(uint16_t *page_list, uint16_t page_count);
ssbc_result_t ssbc_erase_all(void);
ssbc_result_t ssbc_erase_page(uint16_t page_number);

ssbc_result_t ssbc_go(uint32_t address);


#endif /* __STM32_SPI_BOOTLOADER_CLIENT_H__ */
