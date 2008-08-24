#ifndef BSG_H
#define BSG_H

#include <stdint.h>

#define BSG_PROTOCOL_SCSI		0

#define BSG_SUB_PROTOCOL_SCSI_CMD	0
#define BSG_SUB_PROTOCOL_SCSI_TMF	1
#define BSG_SUB_PROTOCOL_SCSI_TRANSPORT	2

struct sg_io_v4 {
	int32_t guard;		/* [i] 'Q' to differentiate from v3 */
	uint32_t protocol;		/* [i] 0 -> SCSI , .... */
	uint32_t subprotocol;	/* [i] 0 -> SCSI command, 1 -> SCSI task
				   management function, .... */

	uint32_t request_len;	/* [i] in bytes */
	uint64_t request;		/* [i], [*i] {SCSI: cdb} */
	uint64_t request_tag;	/* [i] {SCSI: task tag (only if flagged)} */
	uint32_t request_attr;	/* [i] {SCSI: task attribute} */
	uint32_t request_priority;	/* [i] {SCSI: task priority} */
	uint32_t request_extra;	/* [i] {spare, for padding} */
	uint32_t max_response_len;	/* [i] in bytes */
	uint64_t response;		/* [i], [*o] {SCSI: (auto)sense data} */

        /* "dout_": data out (to device); "din_": data in (from device) */
	uint32_t dout_iovec_count;	/* [i] 0 -> "flat" dout transfer else
				   dout_xfer points to array of iovec */
	uint32_t dout_xfer_len;	/* [i] bytes to be transferred to device */
	uint32_t din_iovec_count;	/* [i] 0 -> "flat" din transfer */
	uint32_t din_xfer_len;	/* [i] bytes to be transferred from device */
	uint64_t dout_xferp;	/* [i], [*i] */
	uint64_t din_xferp;	/* [i], [*o] */

	uint32_t timeout;		/* [i] units: millisecond */
	uint32_t flags;		/* [i] bit mask */
	uint64_t usr_ptr;		/* [i->o] unused internally */
	uint32_t spare_in;		/* [i] */

	uint32_t driver_status;	/* [o] 0 -> ok */
	uint32_t transport_status;	/* [o] 0 -> ok */
	uint32_t device_status;	/* [o] {SCSI: command completion status} */
	uint32_t retry_delay;	/* [o] {SCSI: status auxiliary information} */
	uint32_t info;		/* [o] additional information */
	uint32_t duration;		/* [o] time to complete, in milliseconds */
	uint32_t response_len;	/* [o] bytes of response actually written */
	int32_t din_resid;	/* [o] din_xfer_len - actual_din_xfer_len */
	int32_t dout_resid;	/* [o] dout_xfer_len - actual_dout_xfer_len */
	uint64_t generated_tag;	/* [o] {SCSI: transport generated task tag} */
	uint32_t spare_out;	/* [o] */

	uint32_t padding;
};

#endif
