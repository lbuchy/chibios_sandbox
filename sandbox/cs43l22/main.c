/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "test.h"

#include "chprintf.h"
#include "shell.h"
#include "cs43l22.h"
#include "codec.h"

#include "usbcfg.h"

/* Virtual serial port over USB.*/
SerialUSBDriver SDU1;

int codec_started = 0;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)
#define TEST_WA_SIZE    THD_WA_SIZE(256)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {THD_STATE_NAMES};
  Thread *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state time\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %lu\r\n",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
            states[tp->p_state], (uint32_t)tp->p_time);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  Thread *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: test\r\n");
    return;
  }
  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriority(),
                           TestThread, chp);
  if (tp == NULL) {
    chprintf(chp, "out of memory\r\n");
    return;
  }
  chThdWait(tp);
}

void initSndDriver(BaseSequentialStream* stream, int argc, char* argv[])
{

  codec_hw_init();
  //codec_pwrCtl(0);
  codec_i2s_init(8000, 16);

  codec_muteCtl(0);
  codec_pwrCtl(1);
  codec_volCtl(0xe5);
  codec_started = 1;
	//sndInit(32000);
}

void sendBeep(BaseSequentialStream* stream, int argc, char* argv[])
{

  codec_sendBeep();
  //sndInit(32000);
}

static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"test", cmd_test},
  {"snd", initSndDriver},
  {"beep", sendBeep},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};


/*===========================================================================*/
/* Initialization and main thread.                                           */
/*===========================================================================*/

/*
 * Application entry point.
 */
int main(void) {

  Thread *shelltp = NULL;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);


  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);
  


  /*
   * Activates the serial driver 2 using the driver default configuration.
   * PA2(TX) and PA3(RX) are routed to USART2.
   */
  //sdStart(&SD2, NULL);
  //palSetPadMode(GPIOA, 4, PAL_MODE_ALTERNATE(9));
  //palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));

/*
  codec_hw_init();
  //codec_pwrCtl(0);
  codec_i2s_init(32000, 16);

  codec_muteCtl(0);
  //codec_pwrCtl(1);
  codec_volCtl(0xe6);
*/
  /*
   * Normal main() thread activity, in this demo it just performs
   * a shell respawn upon its termination.
   */
  while (TRUE) {
    if (!shelltp) {
      if (SDU1.config->usbp->state == USB_ACTIVE) {
        /* Spawns a new shell.*/
        shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
      }
    }
    else {
      /* If the previous shell exited.*/
      if (chThdTerminated(shelltp)) {
        /* Recovers memory of the previous shell.*/
        chThdRelease(shelltp);
        shelltp = NULL;
      }
    }
    palTogglePad(GPIOD, 12);
    if (codec_started == 1)
      codec_sendBeep();
    chThdSleepMilliseconds(500);
    
  }
}
