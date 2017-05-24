#include "bootloader.h"
#include <string.h>


#define GET_CMD_COMMAND             0x00U  /*!< Get CMD command               */
#define GET_VER_COMMAND             0x01U  /*!< Get Version command           */
#define GET_ID_COMMAND              0x02U  /*!< Get ID command                */
#define RMEM_COMMAND                0x11U  /*!< Read Memory command           */
#define GO_COMMAND                  0x21U  /*!< Go command                    */
#define WMEM_COMMAND                0x31U  /*!< Write Memory command          */
#define EMEM_COMMAND                0x44U  /*!< Erase Memory command          */
#define WP_COMMAND                  0x63U  /*!< Write Protect command         */
#define WU_COMMAND                  0x73U  /*!< Write Unprotect command       */
#define RP_COMMAND                  0x82U  /*!< Readout Protect command       */
#define RU_COMMAND                  0x92U  /*!< Readout Unprotect command     */


#define SSBC_SPI_SOF             0x5AU
#define SSBC_ACK                 0x79U
#define SSBC_NAK                 0x1FU

#define MAX_BOOTLOADER_TRANSFER_SIZE    256

static ssbc_result_t _wait_for_ack(void);
static ssbc_result_t _wait_for_ack_timeout(const uint32_t attempts);
static uint8_t _xor_checksum(const uint8_t *data, uint32_t size);
static ssbc_result_t _send_cmd(uint8_t cmd);
static ssbc_result_t _send_addr(const uint32_t address);
static void _read_data(uint8_t *data, uint16_t size);
static void _transmit(const uint8_t *data, uint32_t size);
static void _receive(uint8_t pData[], uint32_t size);
static void _transmit_receive(const uint8_t pTxData[], uint8_t pRxData[], uint32_t size);

static spi_tx_rx_byte_t _tx_tx_byte = NULL;


static ssbc_result_t _send_cmd(uint8_t cmd) {

    uint8_t cmd_frame[3];
    cmd_frame[0] = SSBC_SPI_SOF;
    cmd_frame[1] = cmd;
    cmd_frame[2] = cmd ^ 0xFF;

    _transmit(cmd_frame, sizeof(cmd_frame));

    return _wait_for_ack();
}

static void _read_data(uint8_t *data, uint16_t size) {

    /* dummy receive */
    _receive(data, 1U);
    _receive(data, size);
}

static ssbc_result_t _send_addr(const uint32_t address) {

    uint8_t addr_frame[5];

    addr_frame[0] = ((uint8_t) (address >> 24) & 0xFFU);
    addr_frame[1] = ((uint8_t) (address >> 16) & 0xFFU);
    addr_frame[2] = ((uint8_t) (address >> 8) & 0xFFU);
    addr_frame[3] = ((uint8_t) address & 0xFFU);
    addr_frame[4] = _xor_checksum(addr_frame, sizeof(address));
    _transmit(addr_frame, sizeof(addr_frame));

    return _wait_for_ack();
}

static ssbc_result_t _wait_for_ack(void) {

    return _wait_for_ack_timeout(UINT32_MAX);
}

static ssbc_result_t _wait_for_ack_timeout(uint32_t attempts) {

    uint8_t resp = 0x00;
    ssbc_result_t res = SSBC_RESULT_ERROR;
    const uint8_t dummy = 0x00U;
    const uint8_t ack = SSBC_ACK;

    _transmit(&dummy, sizeof(dummy));

    while( (resp != SSBC_NAK) && (resp != SSBC_ACK) && (attempts > 0)) {
        _transmit_receive(&dummy, &resp, sizeof(resp));
        if(resp == SSBC_ACK) {
            res = SSBC_RESULT_OK;
        }

        attempts--;
        if(attempts == 0) {
            res = SSBC_RESULT_TIMEOUT;
        }
    }

    _transmit(&ack, sizeof(ack));

    return res;
}

static uint8_t _xor_checksum(const uint8_t pData[], uint32_t size) {

    uint8_t sum = *pData;
    uint32_t i = 0;

    for (i = 1U; i < size; i++) {
        sum ^= pData[i];
    }

    return sum;
}

static void _transmit(const uint8_t pData[], uint32_t size) {

    uint32_t i = 0;
    if(_tx_tx_byte != NULL) {
        for(i=0; i<size; i++) {
            _tx_tx_byte(pData[i]);
        }
    }
}

static void _receive(uint8_t pData[], uint32_t size) {

    uint32_t i = 0;
    if(_tx_tx_byte != NULL) {
        for(i=0; i<size; i++) {
            pData[i] = _tx_tx_byte(0x00);
        }
    }
}

static void _transmit_receive(const uint8_t pTxData[], uint8_t pRxData[], uint32_t size) {

    uint32_t i = 0;
    if(_tx_tx_byte != NULL) {
        for(i=0; i<size; i++) {
            pRxData[i] = _tx_tx_byte(pTxData[i]);
        }
    }
}

/* -------------------------------------------------------------------------- */

ssbc_result_t ssbc_connect(spi_tx_rx_byte_t fn) {

    uint8_t sync_byte = SSBC_SPI_SOF;

    _tx_tx_byte = fn;

    _transmit(&sync_byte, sizeof(sync_byte));

    return _wait_for_ack_timeout(1000);
}


uint16_t ssbc_get(uint8_t *data, uint16_t max_buffer_size) {

    uint8_t count;

    _send_cmd(GET_CMD_COMMAND);

    _read_data(&count, sizeof(count));

    if(max_buffer_size > count) {
        _receive(data, count + 1U);
    } else {
        _receive(data, max_buffer_size);
    }

    _wait_for_ack();

    return (count + 1U);
}

uint8_t ssbc_get_version(void) {

    uint8_t version = 0x00U;

    _send_cmd(GET_VER_COMMAND);

    _read_data(&version, sizeof(version));

    _wait_for_ack();

    return version;
}

uint16_t ssbc_get_chip_id(void) {

    uint8_t count;
    uint8_t rx_buffer[2];
    uint32_t i = 0;

    _send_cmd(GET_ID_COMMAND);

    _read_data(&count, sizeof(count));

    if( count < sizeof(uint16_t) ) {
        _receive(rx_buffer, count + 1U);
    } else {
        _receive(rx_buffer, sizeof(count));

        //TODO ret status

        for(i = 0; i < (count + 1 -  sizeof(uint16_t)); i++) {
            if(_tx_tx_byte != NULL) {
                _tx_tx_byte(0);
            }
        }
    }

    _wait_for_ack();

    return ((uint16_t) rx_buffer[0] << 8) | (rx_buffer[1]);
}

ssbc_result_t ssbc_read(uint8_t *data, uint32_t address, uint16_t size) {

    uint8_t size_frame[2];

    if(data == NULL || size == 0 || size > MAX_BOOTLOADER_TRANSFER_SIZE) {
        return SSBC_RESULT_ERROR;
    }

    if(_send_cmd(RMEM_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    if(_send_addr(address) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    size_frame[0] = (size - 1U);
    size_frame[1] = size_frame[0] ^ 0xFFU;
    _transmit(size_frame, sizeof(size_frame));

    if(_wait_for_ack() != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    _read_data(data, size);

    return SSBC_RESULT_OK;
}

ssbc_result_t ssbc_go(uint32_t address) {

    if(_send_cmd(GO_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    return _send_addr(address);
}

ssbc_result_t ssbc_erase(uint16_t *page_list, uint16_t page_count) {

    uint8_t temp[3];
    uint16_t count;
    uint32_t i;

    if(page_list == NULL) {
        return SSBC_RESULT_ERROR;
    }

    if(page_count > MAX_BOOTLOADER_TRANSFER_SIZE) {
        return SSBC_RESULT_ERROR;
    }

    if(_send_cmd(EMEM_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    count = (page_count < 0xFFF0) ? page_count - 1 : page_count;

    temp[0] = (count >> 8);
    temp[1] = count;
    temp[2] = _xor_checksum(temp, sizeof(page_count));
    _transmit(temp, sizeof(temp));

    if (page_count < 0xFFF0) {

        if(_wait_for_ack() != SSBC_RESULT_OK) {
            return SSBC_RESULT_ERROR;
        }

        for(i = 0; i<page_count; i++) {

            temp[0] = (page_list[i] >> 8);
            temp[1] = page_list[i];
            _transmit(temp, sizeof(page_list[0]));
        }

        temp[0] = _xor_checksum((uint8_t*)page_list, sizeof(page_list[0]) * page_count);
        _transmit(temp, sizeof(page_list[0]));
    }

    return _wait_for_ack();
}

ssbc_result_t ssbc_write(uint32_t address, const uint8_t *data, uint16_t size) {

    uint8_t n = size - 1;
    uint8_t checksum = _xor_checksum(data, size) ^ n;

    if( _send_cmd(WMEM_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    if(_send_addr(address) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    _transmit(&n, sizeof(n));
    _transmit(data, size);
    _transmit(&checksum, sizeof(checksum));

    return _wait_for_ack();
}

ssbc_result_t ssbc_erase_page(uint16_t page_number) {

    return ssbc_erase(&page_number, 1U);
}

ssbc_result_t ssbc_erase_all(void) {

    return ssbc_erase(NULL, ERASE_ALL);
}

ssbc_result_t ssbc_write_protect(uint8_t *sectors_list, uint16_t count) {

    uint8_t data_frame[2];
    uint8_t checksum;

    if( (sectors_list == NULL) ||
       (count == 0) ||
        (count > MAX_BOOTLOADER_TRANSFER_SIZE) ) {
        return SSBC_RESULT_ERROR;
    }

    _send_cmd(WP_COMMAND);

    data_frame[0] = count - 1U;
    data_frame[1] = data_frame[0] ^ 0xFFU;
    _transmit(data_frame, sizeof(data_frame));

    _wait_for_ack();

    checksum = _xor_checksum(sectors_list, count);

    _transmit(sectors_list, count);
    _transmit(&checksum, sizeof(checksum));

    return _wait_for_ack();
}

ssbc_result_t ssbc_write_unprotect(void) {

    if(_send_cmd(WU_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    return _wait_for_ack();
}

ssbc_result_t ssbc_readout_protect(void) {

    if(_send_cmd(RP_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

    return _wait_for_ack();
}

ssbc_result_t ssbc_readout_unprotect(void) {

    if(_send_cmd(RU_COMMAND) != SSBC_RESULT_OK) {
        return SSBC_RESULT_ERROR;
    }

   return _wait_for_ack();
}
