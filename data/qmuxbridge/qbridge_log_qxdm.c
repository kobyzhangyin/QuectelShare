/******************************************************************************

                        QBRIDGE_LOG_QXDM.C

******************************************************************************/

/******************************************************************************

  @file    qbridge_log_qxdm.c
  @brief   This handles logging for Qmuxbridge Component.

  ---------------------------------------------------------------------------
  Copyright (c) 2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ---------------------------------------------------------------------------


******************************************************************************/


/******************************************************************************

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header$

when       who        what, where, why
--------   ---        -------------------------------------------------------
05/02/18    rv        Initial version

******************************************************************************/

/*=============================================================================

  Include Files

=============================================================================*/

#include "qbridge_log_qxdm.h"

/*=============================================================================

  Private Constants and Macros

=============================================================================*/

/*=============================================================================

  Private Variables

=============================================================================*/
static boolean qbridge_msg_silent_mode = FALSE;

/*=============================================================================

  Function Prototypes

=============================================================================*/

/*===========================================================================
  FUNCTION: qbi_log_silent_mode_get
===========================================================================*/
/*!
    @brief Check if silent mode is on

    @details

    @return boolean TRUE if silent mode is on, FALSE if otherwise
*/
/*=========================================================================*/
boolean qbridge_log_silent_mode_get
(
  void
)
{
/*-------------------------------------------------------------------------*/
  return qbridge_msg_silent_mode;
} /* qbridge_log_silent_mode_get */


/*===========================================================================
  FUNCTION: qbridge_log_silent_mode_set
===========================================================================*/
/*!
    @brief Set silent mode on or off

    @details

    @param boolean
*/
/*=========================================================================*/
void qbridge_log_silent_mode_set
(
  boolean mode
)
{
  qbridge_msg_silent_mode = mode;
} /* qbridge_log_silent_mode_set */
