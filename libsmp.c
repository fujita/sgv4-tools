/*
 * Copyright (c) 2006 Douglas Gilbert.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include "libsmp.h"

struct smp_val_name {
	int value;
	char * name;
};

static struct smp_val_name smp_func_results[] =
{
	{SMP_FRES_FUNCTION_ACCEPTED, "SMP function accepted"},
	{SMP_FRES_UNKNOWN_FUNCTION, "Unknown SMP function"},
	{SMP_FRES_FUNCTION_FAILED, "SMP function failed"},
	{SMP_FRES_INVALID_REQUEST_LEN, "Invalid request frame length"},
	{SMP_FRES_INVALID_EXP_CHANGE_COUNT, "Invalid expander change count"},
	{SMP_FRES_BUSY, "Busy"},
	{SMP_FRES_NO_PHY, "Phy does not exist"},
	{SMP_FRES_NO_INDEX, "Index does not exist"},
	{SMP_FRES_NO_SATA_SUPPORT, "Phy does not support SATA"},
	{SMP_FRES_UNKNOWN_PHY_OP, "Unknown phy operation"},
	{SMP_FRES_UNKNOWN_PHY_TEST_FN, "Unknown phy test function"},
	{SMP_FRES_PHY_TEST_IN_PROGRESS, "Phy test function in progress"},
	{SMP_FRES_PHY_VACANT, "Phy vacant"},
	{SMP_FRES_UNKNOWN_PHY_EVENT_INFO_SRC,
	 "Unknown phy event information source"},
	{SMP_FRES_UNKNOWN_DESCRIPTOR_TYPE, "Unknown descriptor type"},
	{SMP_FRES_UNKNOWN_PHY_FILTER, "Unknown phy filter"},
	{SMP_FRES_LOGICAL_LINK_RATE, "Logical link rate not supported"},
	{SMP_FRES_SMP_ZONE_VIOLATION, "SMP zone violation"},
	{SMP_FRES_NO_MANAGEMENT_ACCESS, "No management access rights"},
	{SMP_FRES_UNKNOWN_EN_DIS_ZONING_VAL,
	 "Unknown enable disable zoning value"},
	{SMP_FRES_ZONE_LOCK_VIOLATION, "Zone lock violation"},
	{SMP_FRES_NOT_ACTIVATED, "Not activated"},
	{SMP_FRES_UNKNOWN_ZONE_PHY_INFO_VAL,
	 "Unknown zone phy information value"},
	{0x0, NULL},
};

char *smp_get_func_res_str(int func_res, int buff_len, char * buff)
{
	struct smp_val_name * vnp;

	for (vnp = smp_func_results; vnp->name; ++vnp) {
		if (func_res == vnp->value) {
			snprintf(buff, buff_len, vnp->name);
			return buff;
		}
	}
	snprintf(buff, buff_len, "Unknown function result code=0x%x\n", func_res);
	return buff;
}
