/* hashpipe_pktsock.h
 *
 * Routines for dealing with packet sockets.
 * See `man 7 packet` and `packet_mmap.txt` for more info.
 */
#ifndef _HASHPIPE_PKTSOCK_H
#define _HASHPIPE_PKTSOCK_H

// For IPPROTO_UDP
#include <netinet/ip.h>

// Clients will need PACKET_RX_RING and/or PACKET_TX_RING
#include <linux/if_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hashpipe_pktsock {
  // These sizing fields should be initialized by the caller.  See the comments
  // for `pktsock_open` for details.
  unsigned int frame_size;
  unsigned int nframes;
  unsigned int nblocks;
  // The following fields are "private" and should not be modified by the
  // caller.
  int fd;
  unsigned char *p_ring;
  int next_idx;
};

// Return header field `h` from packet frame pointed to by `p`
#define TPACKET_HDR(p,h) (((struct tpacket_hdr *)p)->h)

// Return pointer to MAC header inside packet frame pointed to by `p`.
#define PKT_MAC(p) (p+TPACKET_HDR(p, tp_mac))

// Return pointer to network (e.g. IP) packet inside
// packet frame pointed to by `p`.
#define PKT_NET(p) (p+TPACKET_HDR(p, tp_net))

// Returns true (non-zero) if this is a UDP packet
#define PKT_IS_UDP(p) ((PKT_NET(p)[0x09]) == IPPROTO_UDP)

// Returns UDP destination port of packet.
// NB: this assumes but does NOT verify that the packet is a UDP packet!
#define PKT_UDP_DST(p) (((PKT_NET(p)[0x16]) << 8)\
                       |((PKT_NET(p)[0x17])     ))

// Returns size of UDP packet (including the 8 byte UDP header).
// NB: this assumes but does NOT verify that the packet is a UDP packet!
#define PKT_UDP_SIZE(p) (((PKT_NET(p)[0x18]) << 8)\
                        |((PKT_NET(p)[0x19])     ))

// Returns pointer to UDP packet payload.
// NB: this assumes but does NOT verify that the packet is a UDP packet!
#define PKT_UDP_DATA(p) (PKT_NET(p) + 0x1c)

// `p_ps` should point to a `struct pktsock` that has been initialized by
// caller with desired values for the sizing parmaters `frame_size`, `nframes`,
// and `nblocks`.  `nblocks` MUST be a multiple of `nframes` and the resulting
// block size (i.e. `frame_size * nframes / nblocks`) MUST be a multiple of
// PAGE_SIZE.
//
// `ifname` should specify the name of the interface to bind to (e.g. "eth2").
//
// `ring_type` should be `PACKET_RX_RING` or `PACKET_TX_RING`.
//
// Returns 0 for success, non-zero for failure.  On failure, errno will be set.
//
// Upon successful compltetion, p_ps->fd will the the file descriptor of the
// socket, and p_ps->p_ring will be a poitner to the start of the ring buffer.
int hashpipe_pktsock_open(struct hashpipe_pktsock *p_ps, const char *ifname, int ring_type);

// Return pointer to frame or NULL if no frame ready.
//
// If a non-NULL frame pointer is returned, the caller MUST release the frame
// back to the kernel (via `pktsock_release_frame`) once it is finished with
// the frame.
unsigned char * hashpipe_pktsock_recv_frame_nonblock(struct hashpipe_pktsock *p_ps);

// Return pointer to frame or NULL on timeout.
//
// If a non-NULL frame pointer is returned, the caller MUST release the frame
// back to the kernel (via `pktsock_release_frame`) once it is finished with
// the frame.
unsigned char * hashpipe_pktsock_recv_frame(struct hashpipe_pktsock *p_ps, int timeout_ms);

// If no frame is ready, returns NULL.  If a non-matching frame is ready, it is
// released back to the kernel and NULL is returned.  Otherwise, returns a
// pointer to the matching frame.
//
// If a non-NULL frame pointer is returned, the caller MUST release the frame
// back to the kernel (via `pktsock_release_frame`) once it is finished with
// the frame.
unsigned char * hashpipe_pktsock_recv_udp_frame_nonblock(
    struct hashpipe_pktsock *p_ps, int dst_port);

// If no frame is ready, wait up to `timeout_ms` to get a frame.  If no frame
// is ready after timeout, returns NULL.  If frame does not match, releases
// non-matching frame back to the kernel and returns NULL.  Otherwise returns
// pointer to matching frame.
//
// If a non-NULL frame pointer is returned, the caller MUST release the frame
// back to the kernel (via `pktsock_release_frame`) once it is finished with
// the frame.
unsigned char * hashpipe_pktsock_recv_udp_frame(
    struct hashpipe_pktsock *p_ps, int dst_port, int timeout_ms);

// Releases frame back to the kernel.  The caller must be release each frame
// back to the kernel once the caller is done with the frame.
void hashpipe_pktsock_release_frame(unsigned char * frame);

// Stores packet counter and drop counter values in `*p_pkts` and `*p_drops`,
// provided they are non-NULL.  This makes it possible to request one but not
// the other.
void hashpipe_pktsock_stats(struct hashpipe_pktsock *p_ps,
    unsigned int *p_pkts, unsigned int *p_drops);

// Unmaps kernel ring buffer and closes socket.
int hashpipe_pktsock_close(struct hashpipe_pktsock *p_ps);

#ifdef __cplusplus
}
#endif

#endif // _HASHPIPE_PKTSOCK_H
