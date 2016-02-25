/**

Copyright (c) 2014-2015 "M-Way Solutions GmbH"
FruityMesh - Bluetooth Low Energy mesh protocol [http://mwaysolutions.com/]

This file is part of FruityMesh

FruityMesh is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <BuzzerWrapper.h>

extern "C"{
#include <nrf.h>
#include <nrf_gpio.h>
#include <nrf_drv_config.h>
#include <app_pwm.h>
#include <app_error.h>
}

APP_PWM_INSTANCE(PWM1, 1);

void pwm_ready_callback(uint32_t pwm_id)
{
}

BuzzerWrapper::BuzzerWrapper(uint32_t io_num)
{
  active = true;
  pin_num = io_num;

  lastStateChangeMs = 0;
}

void BuzzerWrapper::On(void)
{
  if(!active) return;

  app_pwm_config_t pwm_config = APP_PWM_DEFAULT_CONFIG_1CH(500L, pin_num);
  uint32_t err = app_pwm_init(&PWM1, &pwm_config, pwm_ready_callback);
  APP_ERROR_CHECK(err);
  app_pwm_enable(&PWM1);
  app_pwm_channel_duty_set(&PWM1, 0, 10);
}

void BuzzerWrapper::Off(void)
{
  if(!active) return;

  app_pwm_channel_duty_set(&PWM1, 0, 0);
  app_pwm_disable(&PWM1);
  app_pwm_uninit(&PWM1);
}

/* EOF */
