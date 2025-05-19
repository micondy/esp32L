#ifndef LEDC_MOTOR_PWM_H
#define LEDC_MOTOR_PWM_H

#include "driver/ledc.h"

// Function to initialize the LEDC for motor PWM control
void ledc_motor_pwm_init(void);

// Function to set the PWM duty cycle for the motor
void ledc_set_motor_pwm_duty(int channel, int duty);

#endif // LEDC_MOTOR_PWM_H