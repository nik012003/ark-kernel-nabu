// SPDX-License-Identifier: GPL-2.0

#include "ddk750_reg.h"
#include "ddk750_mode.h"
#include "ddk750_chip.h"

/*
 * SM750LE only:
 * This function takes care extra registers and bit fields required to set
 * up a mode in SM750LE
 *
 * Explanation about Display Control register:
 * HW only supports 7 predefined pixel clocks, and clock select is
 * in bit 29:27 of Display Control register.
 */
static unsigned long
display_control_adjust_SM750LE(struct mode_parameter *mode_param,
			       unsigned long disp_control)
{
	unsigned long x, y;

	x = mode_param->horizontal_display_end;
	y = mode_param->vertical_display_end;

	/*
	 * SM750LE has to set up the top-left and bottom-right
	 * registers as well.
	 * Note that normal SM750/SM718 only use those two register for
	 * auto-centering mode.
	 */
	poke32(CRT_AUTO_CENTERING_TL, 0);

	poke32(CRT_AUTO_CENTERING_BR,
	       (((y - 1) << CRT_AUTO_CENTERING_BR_BOTTOM_SHIFT) &
		CRT_AUTO_CENTERING_BR_BOTTOM_MASK) |
	       ((x - 1) & CRT_AUTO_CENTERING_BR_RIGHT_MASK));

	/*
	 * Assume common fields in disp_control have been properly set before
	 * calling this function.
	 * This function only sets the extra fields in disp_control.
	 */

	/* Clear bit 29:27 of display control register */
	disp_control &= ~CRT_DISPLAY_CTRL_CLK_MASK;

	/* Set bit 29:27 of display control register for the right clock */
	/* Note that SM750LE only need to supported 7 resolutions. */
	if (x == 800 && y == 600)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL41;
	else if (x == 1024 && y == 768)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL65;
	else if (x == 1152 && y == 864)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL80;
	else if (x == 1280 && y == 768)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL80;
	else if (x == 1280 && y == 720)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL74;
	else if (x == 1280 && y == 960)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL108;
	else if (x == 1280 && y == 1024)
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL108;
	else /* default to VGA clock */
		disp_control |= CRT_DISPLAY_CTRL_CLK_PLL25;

	/* Set bit 25:24 of display controller */
	disp_control |= (CRT_DISPLAY_CTRL_CRTSELECT | CRT_DISPLAY_CTRL_RGBBIT);

	/* Set bit 14 of display controller */
	disp_control |= DISPLAY_CTRL_CLOCK_PHASE;

	poke32(CRT_DISPLAY_CTRL, disp_control);

	return disp_control;
}

/* only timing related registers will be  programed */
static void program_mode_registers(struct mode_parameter *mode_param,
				   struct pll_value *pll)
{
	int cnt = 0;
	unsigned int tmp, reg;

	if (pll->clock_type == SECONDARY_PLL) {
		/* programe secondary pixel clock */
		poke32(CRT_PLL_CTRL, sm750_format_pll_reg(pll));

		tmp = ((mode_param->horizontal_total - 1) <<
		       CRT_HORIZONTAL_TOTAL_TOTAL_SHIFT) &
		     CRT_HORIZONTAL_TOTAL_TOTAL_MASK;
		tmp |= (mode_param->horizontal_display_end - 1) &
		      CRT_HORIZONTAL_TOTAL_DISPLAY_END_MASK;

		poke32(CRT_HORIZONTAL_TOTAL, tmp);

		tmp = (mode_param->horizontal_sync_width <<
		       CRT_HORIZONTAL_SYNC_WIDTH_SHIFT) &
		     CRT_HORIZONTAL_SYNC_WIDTH_MASK;
		tmp |= (mode_param->horizontal_sync_start - 1) &
		      CRT_HORIZONTAL_SYNC_START_MASK;

		poke32(CRT_HORIZONTAL_SYNC, tmp);

		tmp = ((mode_param->vertical_total - 1) <<
		       CRT_VERTICAL_TOTAL_TOTAL_SHIFT) &
		     CRT_VERTICAL_TOTAL_TOTAL_MASK;
		tmp |= (mode_param->vertical_display_end - 1) &
		      CRT_VERTICAL_TOTAL_DISPLAY_END_MASK;

		poke32(CRT_VERTICAL_TOTAL, tmp);

		tmp = ((mode_param->vertical_sync_height <<
		       CRT_VERTICAL_SYNC_HEIGHT_SHIFT)) &
		     CRT_VERTICAL_SYNC_HEIGHT_MASK;
		tmp |= (mode_param->vertical_sync_start - 1) &
		      CRT_VERTICAL_SYNC_START_MASK;

		poke32(CRT_VERTICAL_SYNC, tmp);

		tmp = DISPLAY_CTRL_TIMING | DISPLAY_CTRL_PLANE;
		if (mode_param->vertical_sync_polarity)
			tmp |= DISPLAY_CTRL_VSYNC_PHASE;
		if (mode_param->horizontal_sync_polarity)
			tmp |= DISPLAY_CTRL_HSYNC_PHASE;

		if (sm750_get_chip_type() == SM750LE) {
			display_control_adjust_SM750LE(mode_param, tmp);
		} else {
			reg = peek32(CRT_DISPLAY_CTRL) &
				~(DISPLAY_CTRL_VSYNC_PHASE |
				  DISPLAY_CTRL_HSYNC_PHASE |
				  DISPLAY_CTRL_TIMING | DISPLAY_CTRL_PLANE);

			poke32(CRT_DISPLAY_CTRL, tmp | reg);
		}

	} else if (pll->clock_type == PRIMARY_PLL) {
		unsigned int reserved;

		poke32(PANEL_PLL_CTRL, sm750_format_pll_reg(pll));

		reg = ((mode_param->horizontal_total - 1) <<
			PANEL_HORIZONTAL_TOTAL_TOTAL_SHIFT) &
			PANEL_HORIZONTAL_TOTAL_TOTAL_MASK;
		reg |= ((mode_param->horizontal_display_end - 1) &
			PANEL_HORIZONTAL_TOTAL_DISPLAY_END_MASK);
		poke32(PANEL_HORIZONTAL_TOTAL, reg);

		poke32(PANEL_HORIZONTAL_SYNC,
		       ((mode_param->horizontal_sync_width <<
			 PANEL_HORIZONTAL_SYNC_WIDTH_SHIFT) &
			PANEL_HORIZONTAL_SYNC_WIDTH_MASK) |
		       ((mode_param->horizontal_sync_start - 1) &
			PANEL_HORIZONTAL_SYNC_START_MASK));

		poke32(PANEL_VERTICAL_TOTAL,
		       (((mode_param->vertical_total - 1) <<
			 PANEL_VERTICAL_TOTAL_TOTAL_SHIFT) &
			PANEL_VERTICAL_TOTAL_TOTAL_MASK) |
		       ((mode_param->vertical_display_end - 1) &
			PANEL_VERTICAL_TOTAL_DISPLAY_END_MASK));

		poke32(PANEL_VERTICAL_SYNC,
		       ((mode_param->vertical_sync_height <<
			 PANEL_VERTICAL_SYNC_HEIGHT_SHIFT) &
			PANEL_VERTICAL_SYNC_HEIGHT_MASK) |
		       ((mode_param->vertical_sync_start - 1) &
			PANEL_VERTICAL_SYNC_START_MASK));

		tmp = DISPLAY_CTRL_TIMING | DISPLAY_CTRL_PLANE;
		if (mode_param->vertical_sync_polarity)
			tmp |= DISPLAY_CTRL_VSYNC_PHASE;
		if (mode_param->horizontal_sync_polarity)
			tmp |= DISPLAY_CTRL_HSYNC_PHASE;
		if (mode_param->clock_phase_polarity)
			tmp |= DISPLAY_CTRL_CLOCK_PHASE;

		reserved = PANEL_DISPLAY_CTRL_RESERVED_MASK |
			PANEL_DISPLAY_CTRL_VSYNC;

		reg = (peek32(PANEL_DISPLAY_CTRL) & ~reserved) &
			~(DISPLAY_CTRL_CLOCK_PHASE | DISPLAY_CTRL_VSYNC_PHASE |
			  DISPLAY_CTRL_HSYNC_PHASE | DISPLAY_CTRL_TIMING |
			  DISPLAY_CTRL_PLANE);

		/*
		 * May a hardware bug or just my test chip (not confirmed).
		 * PANEL_DISPLAY_CTRL register seems requiring few writes
		 * before a value can be successfully written in.
		 * Added some masks to mask out the reserved bits.
		 * Note: This problem happens by design. The hardware will wait
		 *       for the next vertical sync to turn on/off the plane.
		 */
		poke32(PANEL_DISPLAY_CTRL, tmp | reg);

		while ((peek32(PANEL_DISPLAY_CTRL) & ~reserved) !=
			(tmp | reg)) {
			cnt++;
			if (cnt > 1000)
				break;
			poke32(PANEL_DISPLAY_CTRL, tmp | reg);
		}
	}
}

int ddk750_set_mode_timing(struct mode_parameter *parm, enum clock_type clock)
{
	struct pll_value pll;

	pll.input_freq = DEFAULT_INPUT_CLOCK;
	pll.clock_type = clock;

	sm750_calc_pll_value(parm->pixel_clock, &pll);
	if (sm750_get_chip_type() == SM750LE) {
		/* set graphic mode via IO method */
		outb_p(0x88, 0x3d4);
		outb_p(0x06, 0x3d5);
	}
	program_mode_registers(parm, &pll);
	return 0;
}
