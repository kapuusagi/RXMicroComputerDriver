#include "board.h"
#include <iodefine.h>
#include "../rx_utils/rx_utils.h"

static void operating_frequency_set (void);
static void clock_source_select (void);

/**
 * リセット時の初期化を行う。
 */
void
board_init_on_reset(void)
{
	operating_frequency_set();


	return ;
}



/***********************************************************************************************************************
* Function name: operating_frequency_set
* Description  : Configures the clock settings for each of the device clocks
* Arguments    : none
* Return value : none
***********************************************************************************************************************/
static void
operating_frequency_set (void)
{
	/* このソースはRX65N Evaluation Kitのサンプルコードから
	 * マクロで有効になった部分だけを抜粋してきた。 */

    /* Used for constructing value to write to SCKCR register. */
    uint32_t sckcr;
    uint16_t sckcr2;

    /*
    Default settings:
    Clock Description              Frequency
    ----------------------------------------
    Input Clock Frequency............  24 MHz
    PLL frequency (x10).............. 240 MHz
    Internal Clock Frequency......... 120 MHz
    Peripheral Clock Frequency A..... 120 MHz
    Peripheral Clock Frequency B.....  60 MHz
    Peripheral Clock Frequency C.....  60 MHz
    Peripheral Clock Frequency D.....  60 MHz
    Flash IF Clock Frequency.........  60 MHz
    External Bus Clock Frequency..... 120 MHz
    USB Clock Frequency..............  48 MHz */

    /* Protect off. */
    SYSTEM.PRCR.WORD = 0xA50B;

    /* Select the clock based upon user's choice. */
    clock_source_select();

    sckcr = 0;

    /* Figure out setting for FCK bits. */
    sckcr |= 0x20000000; // FCK = PCLK x 1/4

    /* Figure out setting for ICK bits. */
    sckcr |= 0x01000000; // ICK = PCLK x 1/2

    /* Figure out setting for BCK bits. */
    sckcr |= 0x00010000; // BCK = PCK x 1/2

    /* Configure PSTOP1 bit for BCLK output. */
    /* Set PSTOP1 bit */
    sckcr |= 0x00800000; // BCLK stop.

    /* Configure PSTOP0 bit for SDCLK output. */
    /* Set PSTOP0 bit */
    sckcr |= 0x00400000; // SDCLK stop.

    /* Figure out setting for PCKA bits. */
    sckcr |= 0x00001000; // PCKA = PCLK x 1/2

    /* Figure out setting for PCKB bits. */
    sckcr |= 0x00000200; // PCKB = PCLK x 1/4

    /* Figure out setting for PCKC bits. */
    sckcr |= 0x00000020; // PCKC = PCLK x 1/4

    /* Figure out setting for PCKD bits. */
    sckcr |= 0x00000002; // PCKD = PCLK x 1/4

    /* Set SCKCR register. */
    SYSTEM.SCKCR.LONG = sckcr;


    sckcr2 = 0;

    /* Figure out setting for UCK bits. */
    sckcr2 |= 0x0041; // UCK = PCLK/5

    /* Set SCKCR2 register. */
    SYSTEM.SCKCR2.WORD = sckcr2;

    /* Choose clock source. Default for r_bsp_config.h is PLL. */
    SYSTEM.SCKCR3.BIT.CKSEL = 4;

    /* We can now turn LOCO off since it is not going to be used. */
    SYSTEM.LOCOCR.BYTE = 0x01;

    /* Protect on. */
    SYSTEM.PRCR.WORD = 0xA500;
}

/***********************************************************************************************************************
* Function name: clock_source_select
* Description  : Enables and disables clocks as chosen by the user. This function also implements the delays
*                needed for the clocks to stabilize.
* Arguments    : none
* Return value : none
***********************************************************************************************************************/
static void clock_source_select (void)
{
    volatile uint8_t i;
    volatile uint8_t dummy;
    volatile uint8_t tmp;

    /* Main clock will be not oscillate in software standby or deep software standby modes. */
    SYSTEM.MOFCR.BIT.MOFXIN = 0;

    /* Set the oscillation source of the main clock oscillator. */
    SYSTEM.MOFCR.BIT.MOSEL = 0;

    /* Use HOCO if HOCO is chosen or if PLL is chosen with HOCO as source. */
    /* If HOCO is already operating, it doesn't stop.  */
    if (1 == SYSTEM.HOCOCR.BIT.HCSTP)
    {
        /* Turn off power to HOCO. */
        SYSTEM.HOCOPCR.BYTE = 0x01;
    }
    else
    {
        while(0 == SYSTEM.OSCOVFSR.BIT.HCOVF)
        {
            /* The delay period needed is to make sure that the HOCO has stabilized. */
        }
    }

    /* Use Main clock if Main clock is chosen or if PLL is chosen with Main clock as source. */
    /* Main clock oscillator is chosen. Start it operating. */

    /* If the main oscillator is >10MHz then the main clock oscillator forced oscillation control register (MOFCR) must
       be changed. */
    SYSTEM.MOFCR.BIT.MODRV2 = 2; /* 8 - 16MHz. */

    /* Set the oscillation stabilization wait time of the main clock oscillator. */
    SYSTEM.MOSCWTCR.BYTE = 0x53;

    /* Set the main clock to operating. */
    SYSTEM.MOSCCR.BYTE = 0x00;

    if(0x00 ==  SYSTEM.MOSCCR.BYTE)
    {
        /* Dummy read and compare. cf."5. I/O Registers", "(2) Notes on writing to I/O registers" in User's manual.
           This is done to ensure that the register has been written before the next register access. The RX has a
           pipeline architecture so the next instruction could be executed before the previous write had finished. */
        nop();
    }

    while(0 == SYSTEM.OSCOVFSR.BIT.MOOVF)
    {
        /* The delay period needed is to make sure that the Main clock has stabilized. */
    }

    /* Sub-clock setting. */

    /* Cold start setting */
    if (0 == SYSTEM.RSTSR1.BIT.CWSF)
    {
        /* Stop the sub-clock oscillator */
        /* RCR4 - RTC Control Register 4
        b7:b1    Reserved - The write value should be 0.
        b0       RCKSEL   - Count Source Select - Sub-clock oscillator is selected. */
        RTC.RCR4.BIT.RCKSEL = 0;

        /* dummy read four times */
        for (i = 0; i < 4; i++)
        {
            dummy = RTC.RCR4.BYTE;
        }

        if (0 != RTC.RCR4.BIT.RCKSEL)
        {
            /* Confirm that the written */
            nop();
        }

        /* RCR3 - RTC Control Register 3
        b7:b4    Reserved - The write value should be 0.
        b3:b1    RTCDV    - Sub-clock oscillator Drive Ability Control.
        b0       RTCEN    - Sub-clock oscillator is stopped. */
        RTC.RCR3.BIT.RTCEN = 0;

        /* dummy read four times */
        for (i = 0; i < 4; i++)
        {
            dummy = RTC.RCR3.BYTE;
        }

        if (0 != RTC.RCR3.BIT.RTCEN)
        {
            /* Confirm that the written */
            nop();
        }

        /* SOSCCR - Sub-Clock Oscillator Control Register
        b7:b1    Reserved - The write value should be 0.
        b0       SOSTP    - Sub-clock oscillator Stop - Sub-clock oscillator is stopped. */
        SYSTEM.SOSCCR.BYTE = 0x01;

        if (0x01 != SYSTEM.SOSCCR.BYTE)
        {
            /* Dummy read and compare. cf."5. I/O Registers", "(2) Notes on writing to I/O registers" in User's manual.
               This is done to ensure that the register has been written before the next register access. The RX has a
               pipeline architecture so the next instruction could be executed before the previous write had finished. */
            nop();
        }

        while (0 != SYSTEM.OSCOVFSR.BIT.SOOVF)
        {
            /* The delay period needed is to make sure that the sub-clock has stopped. */
        }



    } else {
    	/* Warm start setting */
        /* SOSCCR - Sub-Clock Oscillator Control Register
        b7:b1    Reserved - The write value should be 0.
        b0       SOSTP    - Sub-clock oscillator Stop - Sub-clock oscillator is stopped. */
        SYSTEM.SOSCCR.BYTE = 0x01;

        if (0x01 != SYSTEM.SOSCCR.BYTE)
        {
            /* Dummy read and compare. cf."5. I/O Registers", "(2) Notes on writing to I/O registers" in User's manual.
               This is done to ensure that the register has been written before the next register access. The RX has a
               pipeline architecture so the next instruction could be executed before the previous write had finished. */
            nop();
        }

        while (0 != SYSTEM.OSCOVFSR.BIT.SOOVF)
        {
            /* Confirm that the Sub clock stopped. */
        }

    }


    /* Set PLL Input Divisor. */
    SYSTEM.PLLCR.BIT.PLIDIV = 0;

    /* Clear PLL clock source if PLL clock source is Main clock.  */
    SYSTEM.PLLCR.BIT.PLLSRCSEL = 0;

    /* Set PLL Multiplier. */
    SYSTEM.PLLCR.BIT.STC = 0x27; // x20

    /* Set the PLL to operating. */
    SYSTEM.PLLCR2.BYTE = 0x00;

    while(0 == SYSTEM.OSCOVFSR.BIT.PLOVF)
    {
        /* The delay period needed is to make sure that the PLL has stabilized. */
    }


    /* LOCO is saved for last since it is what is running by default out of reset. This means you do not want to turn
       it off until another clock has been enabled and is ready to use. */
    /* LOCO is not chosen but it cannot be turned off yet since it is still being used. */

    /* Make sure a valid clock was chosen. */

    /* RX65N has a ROMWT register which controls the cycle waiting for access to code flash memory.
       It is set as zero coming out of reset.
       When setting ICLK to [50 MHz < ICLK <= 100 MHz], set the ROMWT.ROMWT[1:0] bits to 01b.
       When setting ICLK to [100 MHz < ICLK <= 120 MHz], set the ROMWT.ROMWT[1:0] bits to 10b. */
    SYSTEM.ROMWT.BYTE = 0x02;
}
