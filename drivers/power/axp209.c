/*
 * (C) Copyright 2012
 * Henrik Nordstrom <henrik@henriknordstrom.net>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <i2c.h>
#include <axp209.h>

typedef enum {
	AXP209_CHIP_VERSION = 0x3,
	AXP209_DCDC2_VOLTAGE = 0x23,
	AXP209_DCDC3_VOLTAGE = 0x27,
	AXP209_LDO24_VOLTAGE = 0x28,
	AXP209_LDO3_VOLTAGE = 0x29,
	AXP209_SHUTDOWN = 0x32,
} axp209_reg;

int axp209_write(axp209_reg reg, u8 val)
{
	return i2c_write(0x34, reg, 1, &val, 1);
}

int axp209_read(axp209_reg reg, u8 * val)
{
	return i2c_read(0x34, reg, 1, val, 1);
}

int axp209_set_dcdc2(int mvolt)
{
	int target = (mvolt - 700) / 25;
	int rc;
	u8 current;

	if (target < 0)
		target = 0;
	if (target > (1 << 6) - 1)
		target = (1 << 6) - 1;

	/* Do we really need to be this gentle? It has built-in voltage slope */
	while ((rc = axp209_read(AXP209_DCDC2_VOLTAGE, &current)) == 0
	       && current != target) {
		if (current < target)
			current++;
		else
			current--;

		rc = axp209_write(AXP209_DCDC2_VOLTAGE, current);
		if (rc)
			break;
	}

	return rc;
}

int axp209_set_dcdc3(int mvolt)
{
	int target = (mvolt - 700) / 25;
	u8 reg;
	int rc;

	if (target < 0)
		target = 0;
	if (target > (1 << 7) - 1)
		target = (1 << 7) - 1;

	rc = axp209_write(AXP209_DCDC3_VOLTAGE, target);
	rc |= axp209_read(AXP209_DCDC3_VOLTAGE, &reg);

	return rc;
}

int axp209_set_ldo2(int mvolt)
{
	int target = (mvolt - 1800) / 100;
	int rc;
	u8 reg;

	if (target < 0)
		target = 0;
	if (target > 15)
		target = 15;

	rc = axp209_read(AXP209_LDO24_VOLTAGE, &reg);
	if (rc)
		return rc;

	reg = (reg & 0x0f) | (target << 4);
	rc = axp209_write(AXP209_LDO24_VOLTAGE, reg);
	if (rc)
		return rc;

	return 0;
}

int axp209_set_ldo3(int mvolt)
{
	int target = (mvolt - 700) / 25;

	if (target < 0)
		target = 0;
	if (target > 127)
		target = 127;
	if (mvolt == -1)
		target = 0x80;	/* detemined by LDO3IN pin */

	return axp209_write(AXP209_LDO3_VOLTAGE, target);
}

int axp209_set_ldo4(int mvolt)
{
	int target = (mvolt - 1800) / 100;
	int rc;
	static const int vindex[] = {
		1250, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2500,
		2700, 2800, 3000, 3100, 3200, 3300
	};
	u8 reg;

	// test-only: what is this next loop doing?? add some comment
	for (target = 0; mvolt < vindex[target] && target < 15; target++) {
	}

	rc = axp209_read(AXP209_LDO24_VOLTAGE, &reg);
	if (rc)
		return rc;

	reg = (reg & 0xf0) | (target << 0);
	rc = axp209_write(AXP209_LDO24_VOLTAGE, reg);
	if (rc)
		return rc;

	return 0;
}

void axp209_poweroff(void)
{
	u8 val;

	if (axp209_read(AXP209_SHUTDOWN, &val) != 0)
		return;

	val |= 1 << 7;

	if (axp209_write(AXP209_SHUTDOWN, val) != 0)
		return;

	udelay(10000);		/* wait for power to drain */
}

int axp209_init(void)
{
	u8 ver;
	int rc;

	rc = axp209_read(AXP209_CHIP_VERSION, &ver);
	if (rc)
		return rc;

	if (ver != 0x21)
		return -1;

	return 0;
}
