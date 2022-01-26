#include "mbed.h"
#include "nRF24L01P.h"
#include <cstdio>

enum { TRANSFER_SIZE = 2 };
char txData[TRANSFER_SIZE], rxData[TRANSFER_SIZE];


SPI spi(D11, D12, D13); // mosi, miso, sclk
DigitalOut cs(D10, 1);

const int leds[6] = {0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE};
int currentLedIndex = 0;
const int NUMBER_OF_LEDS = 6; 

bool isNextButtonClicked = false;
bool isBackButtonClicked = false;

// BufferedSerial pc(USBTX, USBRX); // tx, rx

nRF24L01P my_nrf24l01p(D11, D12, D13, D8, D9, D7);    // mosi, miso, sck, csn, ce, irq

// FileHandle *mbed::mbed_override_console(int fd)
// {
//     return &pc;
// }

int SPIread() {
    cs.write(0);

    spi.write(0x47);
    spi.write(9);
    unsigned int value = spi.write(0x0);    
    cs.write(1);
    return value;
}

void writeLed(int nextLedIndex) {

    if (nextLedIndex >= NUMBER_OF_LEDS) {
        nextLedIndex = 0;
    } else if (nextLedIndex < 0) {
        nextLedIndex = NUMBER_OF_LEDS - 1;
    }
    currentLedIndex = nextLedIndex;

    cs = 0;
    spi.write(0x46);
    spi.write(0x0);
    spi.write(leds[nextLedIndex]);
    cs = 1;
}

void ledLoop() {
    int currentValue = SPIread();

    if (!isNextButtonClicked && (currentValue & 128) == 0) {
        isNextButtonClicked = true;

        txData[0] = 'A';
        txData[1] = '1';
        my_nrf24l01p.write( NRF24L01P_PIPE_P0, txData, TRANSFER_SIZE );
        printf( "Increase!\r\n");

        writeLed(++currentLedIndex);
    } else if (!isBackButtonClicked && (currentValue & 64) == 0) {
        isBackButtonClicked = true;

        txData[0] = 'C';
        txData[1] = '2';
        my_nrf24l01p.write( NRF24L01P_PIPE_P0, txData, TRANSFER_SIZE );
        printf( "Decrease!\r\n");

        writeLed(--currentLedIndex);
    } else if ((currentValue & 64) != 0 && (currentValue & 128) != 0) {
        isNextButtonClicked = false;
        isBackButtonClicked = false;
    }
}

void checkReceivedMessage(char message[TRANSFER_SIZE]) {
    printf("%s", message);

    char commandType = message[0];
    char ledIndex = message[1];
    if (ledIndex == '1') {
        writeLed(++currentLedIndex);
    }
}

int main() {
    spi.format(8, 0);

    cs = 0;
    spi.write(0x46);
    spi.write(6);
    spi.write(0x80 | 0x40);
    cs = 1;  

    cs = 0;
    spi.write(0x46);
    spi.write(0); 
    spi.write(0xDF);
    cs = 1;   

    my_nrf24l01p.setTxAddress(0xC74FB7CDC7);
    my_nrf24l01p.setRxAddress(0xC74FB7CDC8);

    my_nrf24l01p.powerUp();

    // Display the (default) setup of the nRF24L01+ chip
    printf( "nRF24L01+ Frequency    : %d MHz\r\n",  my_nrf24l01p.getRfFrequency() );
    printf( "nRF24L01+ Output power : %d dBm\r\n",  my_nrf24l01p.getRfOutputPower() );
    printf( "nRF24L01+ Data Rate    : %d kbps\r\n", my_nrf24l01p.getAirDataRate() );
    printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", my_nrf24l01p.getTxAddress() );
    printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", my_nrf24l01p.getRxAddress() );

    printf( "Type keys to test transfers:\r\n");

    my_nrf24l01p.setTransferSize( TRANSFER_SIZE );
    my_nrf24l01p.setRfFrequency(NRF24L01P_MAX_RF_FREQUENCY);
    
    my_nrf24l01p.setReceiveMode();
    my_nrf24l01p.enable();

    while (1) {
        // ledLoop();

        // If we've received anything in the nRF24L01+...
        if ( my_nrf24l01p.readable() ) {
            // ...read the data into the receive buffer
            my_nrf24l01p.read( NRF24L01P_PIPE_P0, rxData, sizeof( rxData ) );
            // Display the receive buffer contents via the host serial link
            // pc.write(&rxData[0], sizeof( rxData ));

            checkReceivedMessage(&rxData[0]);

        }
    }
}
