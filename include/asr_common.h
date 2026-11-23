/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Common error type and header macros for the ASR6502 HAL (Cortex-M0+ / PSoC 4100S
 * core + SX126x radio). The error set is scoped to the peripherals this part has;
 * numeric values are stable identifiers, so add back any code a ported driver needs.
 */

#ifndef ASR_SDK_COMMON_API_H
#define ASR_SDK_COMMON_API_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include <assert.h>
#include <stdint.h>

/* Pulls in the version macros and version types. */
#include "asr_version.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* Marks a parameter as intentionally unused; expands to nothing at runtime. */
#define ASR_PARAMETER_NOT_USED(p)    ((void) ((p)))

/* Force C linkage when the headers are pulled into a C++ translation unit. */
#if defined(__cplusplus)
 #define ASR_CPP_HEADER    extern "C" {
 #define ASR_CPP_FOOTER    }
#else
 #define ASR_CPP_HEADER
 #define ASR_CPP_FOOTER
#endif

/* Open/close guards placed at the top and bottom of each public header. */
#define ASR_HEADER    ASR_CPP_HEADER
#define ASR_FOOTER    ASR_CPP_FOOTER

/* Stand-in for an argument that is fixed on the secure side of a non-secure-callable entry. */
#define ASR_SECURE_ARGUMENT    (NULL)

ASR_HEADER

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/* Return codes shared across the ASR HAL. */
typedef enum e_asr_err
{
    ASR_SUCCESS = 0,

    ASR_ERR_ASSERTION             = 1,    /* Runtime assertion check failed */
    ASR_ERR_INVALID_POINTER       = 2,    /* Pointer does not reference valid memory */
    ASR_ERR_INVALID_ARGUMENT      = 3,    /* One or more arguments are out of range */
    ASR_ERR_INVALID_CHANNEL       = 4,    /* Requested channel is not implemented */
    ASR_ERR_INVALID_MODE          = 5,    /* Mode is not valid for this operation */
    ASR_ERR_UNSUPPORTED           = 6,    /* Operation not implemented by this driver */
    ASR_ERR_NOT_OPEN              = 7,    /* Driver instance has not been opened */
    ASR_ERR_IN_USE                = 8,    /* Resource is currently busy */
    ASR_ERR_OUT_OF_MEMORY         = 9,    /* No free memory remaining */
    ASR_ERR_HW_LOCKED             = 10,   /* Peripheral is locked */
    ASR_ERR_IRQ_BSP_DISABLED      = 11,   /* Interrupt line is disabled in the BSP */
    ASR_ERR_OVERFLOW              = 12,   /* Counter or register overflowed */
    ASR_ERR_UNDERFLOW             = 13,   /* Counter or register underflowed */
    ASR_ERR_ALREADY_OPEN          = 14,   /* Instance already open with a different configuration */
    ASR_ERR_APPROXIMATION         = 15,   /* Exact target value was not achievable */
    ASR_ERR_CLAMPED               = 16,   /* Value was saturated to a valid range */
    ASR_ERR_INVALID_RATE          = 17,   /* Requested rate cannot be produced */
    ASR_ERR_ABORTED               = 18,   /* Operation ended before completion */
    ASR_ERR_NOT_ENABLED           = 19,   /* Feature must be enabled first */
    ASR_ERR_TIMEOUT               = 20,   /* Operation did not complete in time */
    ASR_ERR_INVALID_BLOCKS        = 21,   /* Block count is out of range */
    ASR_ERR_INVALID_ADDRESS       = 22,   /* Address is out of range */
    ASR_ERR_INVALID_SIZE          = 23,   /* Length is out of range */
    ASR_ERR_WRITE_FAILED          = 24,   /* Write did not succeed */
    ASR_ERR_ERASE_FAILED          = 25,   /* Erase did not succeed */
    ASR_ERR_INVALID_CALL          = 26,   /* Call is not allowed in this context */
    ASR_ERR_INVALID_HW_CONDITION  = 27,   /* Hardware reported an unexpected state */
    ASR_ERR_INVALID_FACTORY_FLASH = 28,   /* Device has no factory flash region */
    ASR_ERR_INVALID_STATE         = 30,   /* Not permitted in the current state */
    ASR_ERR_NOT_ERASED            = 31,   /* Memory was not blank after erase */
    ASR_ERR_SECTOR_RELEASE_FAILED = 32,   /* Sector could not be released */
    ASR_ERR_NOT_INITIALIZED       = 33,   /* A prerequisite init step was skipped */
    ASR_ERR_NOT_FOUND             = 34,   /* Item was not located */
    ASR_ERR_NO_CALLBACK_MEMORY    = 35,   /* No RAM supplied for the callback trampoline */
    ASR_ERR_BUFFER_EMPTY          = 36,   /* Buffer holds no data */
    ASR_ERR_INVALID_DATA          = 37,   /* Data failed a validity check */

    /* RTOS */
    ASR_ERR_INTERNAL     = 100,           /* Unexpected internal fault */
    ASR_ERR_WAIT_ABORTED = 101,           /* Blocking wait was cancelled */

    /* UART */
    ASR_ERR_FRAMING            = 200,     /* Stop bit was not seen */
    ASR_ERR_BREAK_DETECT       = 201,     /* Break condition was seen */
    ASR_ERR_PARITY             = 202,     /* Parity mismatch on receive */
    ASR_ERR_RXBUF_OVERFLOW     = 203,     /* Receive FIFO overran */
    ASR_ERR_QUEUE_UNAVAILABLE  = 204,     /* Software queue could not be created */
    ASR_ERR_INSUFFICIENT_SPACE = 205,     /* Transmit buffer lacks room */
    ASR_ERR_INSUFFICIENT_DATA  = 206,     /* Receive buffer lacks the requested bytes */

    /* SPI (SX126x radio bus) */
    ASR_ERR_TRANSFER_ABORTED = 300,       /* Transfer stopped early */
    ASR_ERR_MODE_FAULT       = 301,       /* Mode fault detected */
    ASR_ERR_READ_OVERFLOW    = 302,       /* Read buffer overran */
    ASR_ERR_SPI_PARITY       = 303,       /* Parity mismatch */
    ASR_ERR_OVERRUN          = 304,       /* New data arrived before the previous byte was read */

    /* Clock (CGC) */
    ASR_ERR_CLOCK_INACTIVE        = 400,  /* Chosen system clock is not running */
    ASR_ERR_CLOCK_ACTIVE          = 401,  /* Clock must be stopped before it is changed */
    ASR_ERR_NOT_STABILIZED        = 403,  /* Clock has not settled yet */
    ASR_ERR_PLL_SRC_INACTIVE      = 404,  /* PLL source clock is off */
    ASR_ERR_OSC_STOP_DET_ENABLED  = 405,  /* Cannot stop the clock while stop-detect is armed */
    ASR_ERR_OSC_STOP_DETECTED     = 406,  /* Oscillator stop was flagged */
    ASR_ERR_OSC_STOP_CLOCK_ACTIVE = 407,  /* Cannot clear the stop flag with PLL/main oscillator running */
    ASR_ERR_CLKOUT_EXCEEDED       = 408,  /* Clock-out pin frequency is above the limit */
    ASR_ERR_HARDWARE_TIMEOUT      = 410,  /* Register access timed out */
    ASR_ERR_LOW_VOLTAGE_MODE      = 411,  /* Clock setting is not allowed in low-voltage mode */

    /* Flash */
    ASR_ERR_PE_FAILURE             = 500, /* Could not enter program/erase mode */
    ASR_ERR_CMD_LOCKED             = 501, /* Flash controller is in command-lock */
    ASR_ERR_FCLK                   = 502, /* Flash clock is below the required minimum */
    ASR_ERR_INVALID_LINKED_ADDRESS = 503, /* Code or data linked into a disallowed region */
    ASR_ERR_BLANK_CHECK_FAILED     = 504, /* Region was not blank */

    /* Peripheral presence */
    ASR_ERR_IP_HARDWARE_NOT_PRESENT = 1400, /* Peripheral is absent on this device */
    ASR_ERR_IP_UNIT_NOT_PRESENT     = 1401, /* Peripheral unit is absent on this device */
    ASR_ERR_IP_CHANNEL_NOT_PRESENT  = 1402, /* Peripheral channel is absent on this device */

    /* Byte queue (UART software queues) */
    ASR_ERR_QUEUE_FULL  = 10000,          /* Queue has no free slots */
    ASR_ERR_QUEUE_EMPTY = 10001,          /* Queue has nothing to pop */

    /* Sensor */
    ASR_ERR_SENSOR_INVALID_DATA             = 0x30000, /* Reading failed its validity check */
    ASR_ERR_SENSOR_IN_STABILIZATION         = 0x30001, /* Sensor is still settling */
    ASR_ERR_SENSOR_MEASUREMENT_NOT_FINISHED = 0x30002, /* Conversion is still in progress */

    /* Shared bus (I2C/comms) */
    ASR_ERR_COMMS_BUS_NOT_OPEN = 0x40000, /* Shared bus has not been opened */
} asr_err_t;

/***********************************************************************************************************************
 * Function prototypes
 **********************************************************************************************************************/

ASR_FOOTER

#endif                                 /* ASR_SDK_COMMON_API_H */
