/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{ 
 * @addtogroup FaultModule Fault Module
 * @{ 
 *
 * @file       Fault.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @brief      Fault module, inserts faults for testing
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// ****************

#include "openpilot.h"
#include <stdbool.h>
#include "modulesettings.h"
#include "faultsettings.h"
#include "pios_thread.h"

static bool module_enabled;
static uint8_t active_fault;

static int32_t fault_initialize(void)
{
#ifdef MODULE_Fault_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_STATE_NUMELEM];
	ModuleSettingsEnabledGet(module_state);
	if (module_state[MODULESETTINGS_STATE_FAULT] == MODULESETTINGS_STATE_ENABLED) {
		module_enabled = true;
	} else {
		module_enabled = false;
	}
#endif

	/* Do this outside the module_enabled test so that it
	 * can be changed even when the module has been disabled.
	 * This is important so we can remove faults even when
	 * we've booted in BootFault recovery mode with all optional
	 * modules disabled.
	 */
	if (FaultSettingsInitialize() == -1) {
		module_enabled = false;
		return -1;
	}

	if (module_enabled) {
		FaultSettingsActivateFaultGet(&active_fault);

		switch (active_fault) {
		case FAULTSETTINGS_ACTIVATEFAULT_MODULEINITASSERT:
			/* Simulate an assert during module init */
			PIOS_Assert(0);
			break;
		case FAULTSETTINGS_ACTIVATEFAULT_INITOUTOFMEMORY:
			/* Leak all available memory */
			while (PIOS_malloc(10)) ;
			break;
		case FAULTSETTINGS_ACTIVATEFAULT_INITBUSERROR:
			{
				/* Force a bad access */
				uint32_t * bad_ptr = (uint32_t *)0xFFFFFFFF;
				*bad_ptr = 0xAA55AA55;
			}
			break;
		}
	}

	return 0;
}

static void fault_task(void *parameters);

static int32_t fault_start(void)
{
	struct pios_thread *fault_task_handle;

	if (module_enabled) {
		switch (active_fault) {
		case FAULTSETTINGS_ACTIVATEFAULT_RUNAWAYTASK:
		case FAULTSETTINGS_ACTIVATEFAULT_TASKOUTOFMEMORY:
			fault_task_handle = PIOS_Thread_Create(fault_task, "Fault", PIOS_THREAD_STACK_SIZE_MIN, NULL, PIOS_THREAD_PRIO_HIGHEST);
			return 0;
			break;
		}
	}
	return -1;
}
MODULE_INITCALL(fault_initialize, fault_start)

static void fault_task(void *parameters)
{
	switch (active_fault) {
	case FAULTSETTINGS_ACTIVATEFAULT_RUNAWAYTASK:
		/* Consume all realtime, not letting the systemtask run */
		while(1);
		break;
	case FAULTSETTINGS_ACTIVATEFAULT_TASKOUTOFMEMORY:
		/* Leak all available memory and then sleep */
		while (PIOS_malloc(10)) ;
		while (1) {
			PIOS_Thread_Sleep(1000);
		}
		break;
	}
}

/** 
  * @}
  * @}
  */
