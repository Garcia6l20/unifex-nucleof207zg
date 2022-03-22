/** @file gpio.hpp
 *
 * @date Mar 21, 2022
 * @author Sylvain Garcia
 */

#pragma once

extern "C" {
#include <stm32f2xx_hal.h>
}

namespace stm32 {

class gpio {
public:
	GPIO_TypeDef *port_;
	uint16_t pin_;

	void toogle() const noexcept {
		HAL_GPIO_TogglePin(port_, pin_);
	}

	void operator=(bool on) const noexcept {
		HAL_GPIO_WritePin(port_, pin_, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}

	operator bool() const noexcept {
		return HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET;
	}
};

}
