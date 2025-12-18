extern I2C_HandleTypeDef hi2c1;




#define HTS221_ADDR  (0x5F << 1)  // 8-bit address for HAL

uint8_t HTS221_Read(uint8_t reg)
{
    uint8_t val;
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, reg, 1, &val, 1, HAL_MAX_DELAY);
    return val;
}

void HTS221_Write(uint8_t reg, uint8_t val)
{
    HAL_I2C_Mem_Write(&hi2c1, HTS221_ADDR, reg, 1, &val, 1, HAL_MAX_DELAY);
}





void HTS221_Init(void)
{
    uint8_t whoami = HTS221_Read(0x0F);

    if (whoami != 0xBC) {
        print_string("HTS221 not found!\n");
        return;
    }

    // Power on, enable humidity & temp, 1Hz output rate
    HTS221_Write(0x20, 0x85);    
    // 0x80 = PD (power on)
    // 0x04 = BDU (block data update)
    // 0x01 = 1 Hz ODR
}





typedef struct {
    int16_t T0_out, T1_out;
    float T0_degC, T1_degC;

    int16_t H0_out, H1_out;
    float H0_rH, H1_rH;
} HTS221_Calib_t;

HTS221_Calib_t calib;

void HTS221_ReadCalibration(void)
{
    uint8_t buffer[16];

    // Read calibration registers
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x30 | 0x80, 1, buffer, 16, HAL_MAX_DELAY);

    calib.H0_rH = buffer[0] / 2.0f;
    calib.H1_rH = buffer[1] / 2.0f;

    calib.H0_out = (int16_t)(buffer[6] | (buffer[7] << 8));
    calib.H1_out = (int16_t)(buffer[10] | (buffer[11] << 8));

    uint16_t T0 = buffer[2];
    uint16_t T1 = buffer[3];
    uint8_t msb = buffer[5];

    T0 |= (msb & 0x03) << 8;
    T1 |= (msb & 0x0C) << 6;

    calib.T0_degC = T0 / 8.0f;
    calib.T1_degC = T1 / 8.0f;

    calib.T0_out = (int16_t)(buffer[12] | (buffer[13] << 8));
    calib.T1_out = (int16_t)(buffer[14] | (buffer[15] << 8));
}





int16_t HTS221_ReadRawHumidity(void)
{
    uint8_t l = HTS221_Read(0x28 | 0x80);
    uint8_t h = HTS221_Read(0x29 | 0x80);
    return (int16_t)(l | (h << 8));
}

int16_t HTS221_ReadRawTemperature(void)
{
    uint8_t l = HTS221_Read(0x2A | 0x80);
    uint8_t h = HTS221_Read(0x2B | 0x80);
    return (int16_t)(l | (h << 8));
}



float HTS221_GetHumidity(void)
{
    int16_t H_out = HTS221_ReadRawHumidity();

    return calib.H0_rH +
        ( (H_out - calib.H0_out) * (calib.H1_rH - calib.H0_rH) ) /
        (calib.H1_out - calib.H0_out);
}

float HTS221_GetTemperature(void)
{
    int16_t T_out = HTS221_ReadRawTemperature();

    return calib.T0_degC +
        ( (T_out - calib.T0_out) * (calib.T1_degC - calib.T0_degC) ) /
        (calib.T1_out - calib.T0_out);
}





int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();  // From CubeMX

    HTS221_Init();
    HTS221_ReadCalibration();

    char msg[64];

    while (1)
    {
        float hum = HTS221_GetHumidity();
        float temp = HTS221_GetTemperature();

        sprintf(msg, "Temp: %.2f C  Hum: %.2f %%\n", temp, hum);
        print_string(msg);

        HAL_Delay(1000);
    }
}
