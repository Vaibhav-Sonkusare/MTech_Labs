/*To read a button:

3.3V
 |
(Internal pull-up)
 |
GPIO Pin ---- Button ---- GND


Select your pin (e.g., PC13)

Set Mode → GPIO_Input

Set Pull-up → Pull-up

No speed required

Generate code
*/

if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
    print_string("Button pressed!\n");
}


// or 

while (1)
{
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        print_string("Pressed\n");
        HAL_Delay(200);
    }
}


// or

uint8_t read_button(void)
{
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
    {
        HAL_Delay(20);   // debounce delay
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET)
            return 1;
    }
    return 0;
}
