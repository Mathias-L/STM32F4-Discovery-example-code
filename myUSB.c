

#include "ch.h"
#include "hal.h"
#include "usb_cdc.h"
#include "shell.h"

#include "myUSB.h"
#include "usbdescriptor.h"
/*
 * Don't ask me, I have no idea what is done here...
 * I think most things are either self explanatory, well documented or not to be touched
 */

/*
 * USB Driver structure.
 */
SerialUSBDriver SDU1;


/*===========================================================================*/
/* USB related stuff.                                                        */
/*===========================================================================*/




/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &vcom_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &vcom_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 4)
      return &vcom_strings[dindex];
  }
  return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   EP1 initialization structure (IN only).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  sduDataTransmitted,
  NULL,
  0x0040,
  0x0000,
  &ep1instate,
  NULL,
  NULL
};

/**
 * @brief   OUT EP2 state.
 */
USBOutEndpointState ep2outstate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  sduInterruptTransmitted,
  NULL,
  0x0010,
  0x0000,
  NULL,
  &ep2outstate,
  NULL
};

/**
 * @brief   OUT EP2 state.
 */
USBOutEndpointState ep3outstate;

/**
 * @brief   EP3 initialization structure (OUT only).
 */
static const USBEndpointConfig ep3config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  NULL,
  sduDataReceived,
  0x0000,
  0x0040,
  NULL,
  &ep3outstate,
  NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {

  switch (event) {
  case USB_EVENT_RESET:
    return;
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromIsr();

    /* Enables the endpoints specified into the configuration.
       Note, this callback is invoked from an ISR so I-Class functions
       must be used.*/
    usbInitEndpointI(usbp, USB_CDC_DATA_REQUEST_EP, &ep1config);
    usbInitEndpointI(usbp, USB_CDC_INTERRUPT_REQUEST_EP, &ep2config);
    usbInitEndpointI(usbp, USB_CDC_DATA_AVAILABLE_EP, &ep3config);

    /* Resetting the state of the CDC subsystem.*/
    sduConfigureHookI(usbp);

    chSysUnlockFromIsr();
    return;
  case USB_EVENT_SUSPEND:
    return;
  case USB_EVENT_WAKEUP:
    return;
  case USB_EVENT_STALLED:
    return;
  }
  return;
}


/*
 * Serial over USB driver configuration.
 */
static const SerialUSBConfig serusbcfg = {
  &USBD1,
  {
    usb_event,
    get_descriptor,
    sduRequestsHook,
    NULL
  }
};

int isUsbActive(void){
  return SDU1.config->usbp->state == USB_ACTIVE;
}


void myUSBinit(void){
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1000);
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusbcfg);  // => usbStart(config->usbp, &config->usb_config);
  usbConnectBus(serusbcfg.usbp);

  /*
   * Shell manager initialization.
   */
  shellInit();
}
