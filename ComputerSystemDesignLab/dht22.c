// Enable DWT for microsecond delays
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(volatile uint32_t microseconds) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = microseconds * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}






void DHT_SetPinOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void DHT_SetPinInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}






uint8_t DHT_ReadData(float *temperature, float *humidity)
{
    uint8_t bits[5] = {0};
    uint32_t idx = 0, cnt = 0;

    // ----- MCU pulls line LOW for >= 1ms -----
    DHT_SetPinOutput();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    DWT_Delay_us(1100);
    
    // Release line
    DHT_SetPinInput();
    DWT_Delay_us(30);

    // ----- DHT Response -----
    // Wait for DHT to pull low (80us)
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) {
        if (++cnt > 10000) return 1;  // timeout
    }
    cnt = 0;
    while (!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) {
        if (++cnt > 10000) return 1;
    }

    // DHT pulls high for 80us
    cnt = 0;
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) {
        if (++cnt > 10000) return 1;
    }

    // ----- Read 40 bits -----
    for (int i = 0; i < 40; i++) {
        // wait for LOW
        while (!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1));

        // measure length of HIGH pulse
        DWT_Delay_us(40);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) {
            bits[i / 8] |= (1 << (7 - (i % 8)));

            // wait for end of bit
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1));
        }
    }

    // ----- checksum -----
    if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) != bits[4]) {
        return 2; // checksum error
    }

    // Convert raw data
    *humidity = ((bits[0] << 8) | bits[1]) * 0.1f;

    int16_t t = ((bits[2] & 0x7F) << 8) | bits[3];
    *temperature = t * 0.1f;
    if (bits[2] & 0x80) *temperature *= -1;

    return 0; // success
}





int main(void)
{
    HAL_Init();
    SystemClock_Config();
    DWT_Init();

    float temp, hum;

    while (1) {
        if (DHT_ReadData(&temp, &hum) == 0) {
            char buffer[64];
            sprintf(buffer, "Temp: %.1f C, Hum: %.1f %%\n", temp, hum);
            print_string(buffer);
        } else {
            print_string("Error reading DHT22\n");
        }

        HAL_Delay(2000); // DHT22 min sampling period
    }
}
