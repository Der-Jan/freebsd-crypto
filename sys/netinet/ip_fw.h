/*-
 * Copyright (c) 2002 Luigi Rizzo, Universita` di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * $FreeBSD$
 */

#ifndef _IPFW2_H
#define _IPFW2_H

/*
 * The default rule number.  By the design of ip_fw, the default rule
 * is the last one, so its number can also serve as the highest number
 * allowed for a rule.  The ip_fw code relies on both meanings of this
 * constant. 
 */
#define	IPFW_DEFAULT_RULE	65535

/*
 * The kernel representation of ipfw rules is made of a list of
 * 'instructions' (for all practical purposes equivalent to BPF
 * instructions), which specify which fields of the packet
 * (or its metadata) should be analysed.
 *
 * Each instruction is stored in a structure which begins with
 * "ipfw_insn", and can contain extra fields depending on the
 * instruction type (listed below).
 * Note that the code is written so that individual instructions
 * have a size which is a multiple of 32 bits. This means that, if
 * such structures contain pointers or other 64-bit entities,
 * (there is just one instance now) they may end up unaligned on
 * 64-bit architectures, so the must be handled with care.
 *
 * "enum ipfw_opcodes" are the opcodes supported. We can have up
 * to 256 different opcodes. When adding new opcodes, they should
 * be appended to the end of the opcode list before O_LAST_OPCODE,
 * this will prevent the ABI from being broken, otherwise users
 * will have to recompile ipfw(8) when they update the kernel.
 */

enum ipfw_opcodes {		/* arguments (4 byte each)	*/
	O_NOP,

	O_IP_SRC,		/* u32 = IP			*/
	O_IP_SRC_MASK,		/* ip = IP/mask			*/
	O_IP_SRC_ME,		/* none				*/
	O_IP_SRC_SET,		/* u32=base, arg1=len, bitmap	*/

	O_IP_DST,		/* u32 = IP			*/
	O_IP_DST_MASK,		/* ip = IP/mask			*/
	O_IP_DST_ME,		/* none				*/
	O_IP_DST_SET,		/* u32=base, arg1=len, bitmap	*/

	O_IP_SRCPORT,		/* (n)port list:mask 4 byte ea	*/
	O_IP_DSTPORT,		/* (n)port list:mask 4 byte ea	*/
	O_PROTO,		/* arg1=protocol		*/

	O_MACADDR2,		/* 2 mac addr:mask		*/
	O_MAC_TYPE,		/* same as srcport		*/

	O_LAYER2,		/* none				*/
	O_IN,			/* none				*/
	O_FRAG,			/* none				*/

	O_RECV,			/* none				*/
	O_XMIT,			/* none				*/
	O_VIA,			/* none				*/

	O_IPOPT,		/* arg1 = 2*u8 bitmap		*/
	O_IPLEN,		/* arg1 = len			*/
	O_IPID,			/* arg1 = id			*/

	O_IPTOS,		/* arg1 = id			*/
	O_IPPRECEDENCE,		/* arg1 = precedence << 5	*/
	O_IPTTL,		/* arg1 = TTL			*/

	O_IPVER,		/* arg1 = version		*/
	O_UID,			/* u32 = id			*/
	O_GID,			/* u32 = id			*/
	O_ESTAB,		/* none (tcp established)	*/
	O_TCPFLAGS,		/* arg1 = 2*u8 bitmap		*/
	O_TCPWIN,		/* arg1 = desired win		*/
	O_TCPSEQ,		/* u32 = desired seq.		*/
	O_TCPACK,		/* u32 = desired seq.		*/
	O_ICMPTYPE,		/* u32 = icmp bitmap		*/
	O_TCPOPTS,		/* arg1 = 2*u8 bitmap		*/

	O_VERREVPATH,		/* none				*/
	O_VERSRCREACH,		/* none				*/

	O_PROBE_STATE,		/* none				*/
	O_KEEP_STATE,		/* none				*/
	O_LIMIT,		/* ipfw_insn_limit		*/
	O_LIMIT_PARENT,		/* dyn_type, not an opcode.	*/

	/*
	 * These are really 'actions'.
	 */

	O_LOG,			/* ipfw_insn_log		*/
	O_PROB,			/* u32 = match probability	*/

	O_CHECK_STATE,		/* none				*/
	O_ACCEPT,		/* none				*/
	O_DENY,			/* none 			*/
	O_REJECT,		/* arg1=icmp arg (same as deny)	*/
	O_COUNT,		/* none				*/
	O_SKIPTO,		/* arg1=next rule number	*/
	O_PIPE,			/* arg1=pipe number		*/
	O_QUEUE,		/* arg1=queue number		*/
	O_DIVERT,		/* arg1=port number		*/
	O_TEE,			/* arg1=port number		*/
	O_FORWARD_IP,		/* fwd sockaddr			*/
	O_FORWARD_MAC,		/* fwd mac			*/
	O_NAT,                  /* nope                         */

	/*
	 * More opcodes.
	 */
	O_IPSEC,		/* has ipsec history 		*/
	O_IP_SRC_LOOKUP,	/* arg1=table number, u32=value	*/
	O_IP_DST_LOOKUP,	/* arg1=table number, u32=value	*/
	O_ANTISPOOF,		/* none				*/
	O_JAIL,			/* u32 = id			*/
	O_ALTQ,			/* u32 = altq classif. qid	*/
	O_DIVERTED,		/* arg1=bitmap (1:loop, 2:out)	*/
	O_TCPDATALEN,		/* arg1 = tcp data len		*/
	O_IP6_SRC,		/* address without mask		*/
	O_IP6_SRC_ME,		/* my addresses			*/
	O_IP6_SRC_MASK,		/* address with the mask	*/
	O_IP6_DST,
	O_IP6_DST_ME,
	O_IP6_DST_MASK,
	O_FLOW6ID,		/* for flow id tag in the ipv6 pkt */
	O_ICMP6TYPE,		/* icmp6 packet type filtering	*/
	O_EXT_HDR,		/* filtering for ipv6 extension header */
	O_IP6,

	/*
	 * actions for ng_ipfw
	 */
	O_NETGRAPH,		/* send to ng_ipfw		*/
	O_NGTEE,		/* copy to ng_ipfw		*/

	O_IP4,

	O_UNREACH6,		/* arg1=icmpv6 code arg (deny)  */

	O_TAG,   		/* arg1=tag number */
	O_TAGGED,		/* arg1=tag number */

	O_SETFIB,		/* arg1=FIB number */
	O_FIB,			/* arg1=FIB desired fib number */

	O_LAST_OPCODE		/* not an opcode!		*/
};

/*
 * The extension header are filtered only for presence using a bit
 * vector with a flag for each header.
 */
#define EXT_FRAGMENT	0x1
#define EXT_HOPOPTS	0x2
#define EXT_ROUTING	0x4
#define EXT_AH		0x8
#define EXT_ESP		0x10
#define EXT_DSTOPTS	0x20
#define EXT_RTHDR0		0x40
#define EXT_RTHDR2		0x80

/*
 * Template for instructions.
 *
 * ipfw_insn is used for all instructions which require no operands,
 * a single 16-bit value (arg1), or a couple of 8-bit values.
 *
 * For other instructions which require different/larger arguments
 * we have derived structures, ipfw_insn_*.
 *
 * The size of the instruction (in 32-bit words) is in the low
 * 6 bits of "len". The 2 remaining bits are used to implement
 * NOT and OR on individual instructions. Given a type, you can
 * compute the length to be put in "len" using F_INSN_SIZE(t)
 *
 * F_NOT	negates the match result of the instruction.
 *
 * F_OR		is used to build or blocks. By default, instructions
 *		are evaluated as part of a logical AND. An "or" block
 *		{ X or Y or Z } contains F_OR set in all but the last
 *		instruction of the block. A match will cause the code
 *		to skip past the last instruction of the block.
 *
 * NOTA BENE: in a couple of places we assume that
 *	sizeof(ipfw_insn) == sizeof(u_int32_t)
 * this needs to be fixed.
 *
 */
typedef struct	_ipfw_insn {	/* template for instructions */
	enum ipfw_opcodes	opcode:8;
	u_int8_t	len;	/* numer of 32-byte words */
#define	F_NOT		0x80
#define	F_OR		0x40
#define	F_LEN_MASK	0x3f
#define	F_LEN(cmd)	((cmd)->len & F_LEN_MASK)

	u_int16_t	arg1;
} ipfw_insn;

/*
 * The F_INSN_SIZE(type) computes the size, in 4-byte words, of
 * a given type.
 */
#define	F_INSN_SIZE(t)	((sizeof (t))/sizeof(u_int32_t))

#define MTAG_IPFW	1148380143	/* IPFW-tagged cookie */

/*
 * This is used to store an array of 16-bit entries (ports etc.)
 */
typedef struct	_ipfw_insn_u16 {
	ipfw_insn o;
	u_int16_t ports[2];	/* there may be more */
} ipfw_insn_u16;

/*
 * This is used to store an array of 32-bit entries
 * (uid, single IPv4 addresses etc.)
 */
typedef struct	_ipfw_insn_u32 {
	ipfw_insn o;
	u_int32_t d[1];	/* one or more */
} ipfw_insn_u32;

/*
 * This is used to store IP addr-mask pairs.
 */
typedef struct	_ipfw_insn_ip {
	ipfw_insn o;
	struct in_addr	addr;
	struct in_addr	mask;
} ipfw_insn_ip;

/*
 * This is used to forward to a given address (ip).
 */
typedef struct  _ipfw_insn_sa {
	ipfw_insn o;
	struct sockaddr_in sa;
} ipfw_insn_sa;

/*
 * This is used for MAC addr-mask pairs.
 */
typedef struct	_ipfw_insn_mac {
	ipfw_insn o;
	u_char addr[12];	/* dst[6] + src[6] */
	u_char mask[12];	/* dst[6] + src[6] */
} ipfw_insn_mac;

/*
 * This is used for interface match rules (recv xx, xmit xx).
 */
typedef struct	_ipfw_insn_if {
	ipfw_insn o;
	union {
		struct in_addr ip;
		int glob;
	} p;
	char name[IFNAMSIZ];
} ipfw_insn_if;

/*
 * This is used for storing an altq queue id number.
 */
typedef struct _ipfw_insn_altq {
	ipfw_insn	o;
	u_int32_t	qid;
} ipfw_insn_altq;

/*
 * This is used for limit rules.
 */
typedef struct	_ipfw_insn_limit {
	ipfw_insn o;
	u_int8_t _pad;
	u_int8_t limit_mask;	/* combination of DYN_* below	*/
#define	DYN_SRC_ADDR	0x1
#define	DYN_SRC_PORT	0x2
#define	DYN_DST_ADDR	0x4
#define	DYN_DST_PORT	0x8

	u_int16_t conn_limit;
} ipfw_insn_limit;

/*
 * This is used for log instructions.
 */
typedef struct  _ipfw_insn_log {
        ipfw_insn o;
	u_int32_t max_log;	/* how many do we log -- 0 = all */
	u_int32_t log_left;	/* how many left to log 	*/
} ipfw_insn_log;

/*
 * Data structures required by both ipfw(8) and ipfw(4) but not part of the
 * management API are protected by IPFW_INTERNAL.
 */
#ifdef IPFW_INTERNAL
/* Server pool support (LSNAT). */
struct cfg_spool {
	LIST_ENTRY(cfg_spool)   _next;          /* chain of spool instances */
	struct in_addr          addr;
	u_short                 port;
};
#endif

/* Redirect modes id. */
#define REDIR_ADDR      0x01
#define REDIR_PORT      0x02
#define REDIR_PROTO     0x04

#ifdef IPFW_INTERNAL
/* Nat redirect configuration. */
struct cfg_redir {
	LIST_ENTRY(cfg_redir)   _next;          /* chain of redir instances */
	u_int16_t               mode;           /* type of redirect mode */
	struct in_addr	        laddr;          /* local ip address */
	struct in_addr	        paddr;          /* public ip address */
	struct in_addr	        raddr;          /* remote ip address */
	u_short                 lport;          /* local port */
	u_short                 pport;          /* public port */
	u_short                 rport;          /* remote port  */
	u_short                 pport_cnt;      /* number of public ports */
	u_short                 rport_cnt;      /* number of remote ports */
	int                     proto;          /* protocol: tcp/udp */
	struct alias_link       **alink;	
	/* num of entry in spool chain */
	u_int16_t               spool_cnt;      
	/* chain of spool instances */
	LIST_HEAD(spool_chain, cfg_spool) spool_chain;
};
#endif

#define NAT_BUF_LEN     1024

#ifdef IPFW_INTERNAL
/* Nat configuration data struct. */
struct cfg_nat {
	/* chain of nat instances */
	LIST_ENTRY(cfg_nat)     _next;
	int                     id;                     /* nat id */
	struct in_addr          ip;                     /* nat ip address */
	char                    if_name[IF_NAMESIZE];   /* interface name */
	int                     mode;                   /* aliasing mode */
	struct libalias	        *lib;                   /* libalias instance */
	/* number of entry in spool chain */
	int                     redir_cnt;              
	/* chain of redir instances */
	LIST_HEAD(redir_chain, cfg_redir) redir_chain;  
};
#endif

#define SOF_NAT         sizeof(struct cfg_nat)
#define SOF_REDIR       sizeof(struct cfg_redir)
#define SOF_SPOOL       sizeof(struct cfg_spool)

/* Nat command. */
typedef struct	_ipfw_insn_nat {
 	ipfw_insn	o;
 	struct cfg_nat *nat;	
} ipfw_insn_nat;

/* Apply ipv6 mask on ipv6 addr */
#define APPLY_MASK(addr,mask)                          \
    (addr)->__u6_addr.__u6_addr32[0] &= (mask)->__u6_addr.__u6_addr32[0]; \
    (addr)->__u6_addr.__u6_addr32[1] &= (mask)->__u6_addr.__u6_addr32[1]; \
    (addr)->__u6_addr.__u6_addr32[2] &= (mask)->__u6_addr.__u6_addr32[2]; \
    (addr)->__u6_addr.__u6_addr32[3] &= (mask)->__u6_addr.__u6_addr32[3];

/* Structure for ipv6 */
typedef struct _ipfw_insn_ip6 {
       ipfw_insn o;
       struct in6_addr addr6;
       struct in6_addr mask6;
} ipfw_insn_ip6;

/* Used to support icmp6 types */
typedef struct _ipfw_insn_icmp6 {
       ipfw_insn o;
       uint32_t d[7]; /* XXX This number si related to the netinet/icmp6.h
                       *     define ICMP6_MAXTYPE
                       *     as follows: n = ICMP6_MAXTYPE/32 + 1
                        *     Actually is 203 
                       */
} ipfw_insn_icmp6;

/*
 * Here we have the structure representing an ipfw rule.
 *
 * It starts with a general area (with link fields and counters)
 * followed by an array of one or more instructions, which the code
 * accesses as an array of 32-bit values.
 *
 * Given a rule pointer  r:
 *
 *  r->cmd		is the start of the first instruction.
 *  ACTION_PTR(r)	is the start of the first action (things to do
 *			once a rule matched).
 *
 * When assembling instruction, remember the following:
 *
 *  + if a rule has a "keep-state" (or "limit") option, then the
 *	first instruction (at r->cmd) MUST BE an O_PROBE_STATE
 *  + if a rule has a "log" option, then the first action
 *	(at ACTION_PTR(r)) MUST be O_LOG
 *  + if a rule has an "altq" option, it comes after "log"
 *  + if a rule has an O_TAG option, it comes after "log" and "altq"
 *
 * NOTE: we use a simple linked list of rules because we never need
 * 	to delete a rule without scanning the list. We do not use
 *	queue(3) macros for portability and readability.
 */

struct ip_fw {
	struct ip_fw	*next;		/* linked list of rules		*/
	struct ip_fw	*next_rule;	/* ptr to next [skipto] rule	*/
	/* 'next_rule' is used to pass up 'set_disable' status		*/

	u_int16_t	act_ofs;	/* offset of action in 32-bit units */
	u_int16_t	cmd_len;	/* # of 32-bit words in cmd	*/
	u_int16_t	rulenum;	/* rule number			*/
	u_int8_t	set;		/* rule set (0..31)		*/
#define	RESVD_SET	31	/* set for default and persistent rules */
	u_int8_t	_pad;		/* padding			*/

	/* These fields are present in all rules.			*/
	u_int64_t	pcnt;		/* Packet counter		*/
	u_int64_t	bcnt;		/* Byte counter			*/
	u_int32_t	timestamp;	/* tv_sec of last match		*/

	ipfw_insn	cmd[1];		/* storage for commands		*/
};

#define ACTION_PTR(rule)				\
	(ipfw_insn *)( (u_int32_t *)((rule)->cmd) + ((rule)->act_ofs) )

#define RULESIZE(rule)  (sizeof(struct ip_fw) + \
	((struct ip_fw *)(rule))->cmd_len * 4 - 4)

/*
 * This structure is used as a flow mask and a flow id for various
 * parts of the code.
 */
struct ipfw_flow_id {
	u_int32_t	dst_ip;
	u_int32_t	src_ip;
	u_int16_t	dst_port;
	u_int16_t	src_port;
	u_int8_t	fib;
	u_int8_t	proto;
	u_int8_t	flags;	/* protocol-specific flags */
	uint8_t		addr_type; /* 4 = ipv4, 6 = ipv6, 1=ether ? */
	struct in6_addr dst_ip6;	/* could also store MAC addr! */
	struct in6_addr src_ip6;
	u_int32_t	flow_id6;
	u_int32_t	frag_id6;
};

#define IS_IP6_FLOW_ID(id)	((id)->addr_type == 6)

/*
 * Dynamic ipfw rule.
 */
typedef struct _ipfw_dyn_rule ipfw_dyn_rule;

struct _ipfw_dyn_rule {
	ipfw_dyn_rule	*next;		/* linked list of rules.	*/
	struct ip_fw *rule;		/* pointer to rule		*/
	/* 'rule' is used to pass up the rule number (from the parent)	*/

	ipfw_dyn_rule *parent;		/* pointer to parent rule	*/
	u_int64_t	pcnt;		/* packet match counter		*/
	u_int64_t	bcnt;		/* byte match counter		*/
	struct ipfw_flow_id id;		/* (masked) flow id		*/
	u_int32_t	expire;		/* expire time			*/
	u_int32_t	bucket;		/* which bucket in hash table	*/
	u_int32_t	state;		/* state of this rule (typically a
					 * combination of TCP flags)
					 */
	u_int32_t	ack_fwd;	/* most recent ACKs in forward	*/
	u_int32_t	ack_rev;	/* and reverse directions (used	*/
					/* to generate keepalives)	*/
	u_int16_t	dyn_type;	/* rule type			*/
	u_int16_t	count;		/* refcount			*/
};

/*
 * Definitions for IP option names.
 */
#define	IP_FW_IPOPT_LSRR	0x01
#define	IP_FW_IPOPT_SSRR	0x02
#define	IP_FW_IPOPT_RR		0x04
#define	IP_FW_IPOPT_TS		0x08

/*
 * Definitions for TCP option names.
 */
#define	IP_FW_TCPOPT_MSS	0x01
#define	IP_FW_TCPOPT_WINDOW	0x02
#define	IP_FW_TCPOPT_SACK	0x04
#define	IP_FW_TCPOPT_TS		0x08
#define	IP_FW_TCPOPT_CC		0x10

#define	ICMP_REJECT_RST		0x100	/* fake ICMP code (send a TCP RST) */
#define	ICMP6_UNREACH_RST	0x100	/* fake ICMPv6 code (send a TCP RST) */

/*
 * These are used for lookup tables.
 */
typedef struct	_ipfw_table_entry {
	in_addr_t	addr;		/* network address		*/
	u_int32_t	value;		/* value			*/
	u_int16_t	tbl;		/* table number			*/
	u_int8_t	masklen;	/* mask length			*/
} ipfw_table_entry;

typedef struct	_ipfw_table {
	u_int32_t	size;		/* size of entries in bytes	*/
	u_int32_t	cnt;		/* # of entries			*/
	u_int16_t	tbl;		/* table number			*/
	ipfw_table_entry ent[0];	/* entries			*/
} ipfw_table;

#define IP_FW_TABLEARG	65535

/*
 * Main firewall chains definitions and global var's definitions.
 */
#ifdef _KERNEL

/* Return values from ipfw_chk() */
enum {
	IP_FW_PASS = 0,
	IP_FW_DENY,
	IP_FW_DIVERT,
	IP_FW_TEE,
	IP_FW_DUMMYNET,
	IP_FW_NETGRAPH,
	IP_FW_NGTEE,
	IP_FW_NAT,
};

/* flags for divert mtag */
#define	IP_FW_DIVERT_LOOPBACK_FLAG	0x00080000
#define	IP_FW_DIVERT_OUTPUT_FLAG	0x00100000

/*
 * Structure for collecting parameters to dummynet for ip6_output forwarding
 */
struct _ip6dn_args {
       struct ip6_pktopts *opt_or;
       struct route_in6 ro_or;
       int flags_or;
       struct ip6_moptions *im6o_or;
       struct ifnet *origifp_or;
       struct ifnet *ifp_or;
       struct sockaddr_in6 dst_or;
       u_long mtu_or;
       struct route_in6 ro_pmtu_or;
};

/*
 * Arguments for calling ipfw_chk() and dummynet_io(). We put them
 * all into a structure because this way it is easier and more
 * efficient to pass variables around and extend the interface.
 */
struct ip_fw_args {
	struct mbuf	*m;		/* the mbuf chain		*/
	struct ifnet	*oif;		/* output interface		*/
	struct sockaddr_in *next_hop;	/* forward address		*/
	struct ip_fw	*rule;		/* matching rule		*/
	struct ether_header *eh;	/* for bridged packets		*/

	struct ipfw_flow_id f_id;	/* grabbed from IP header	*/
	u_int32_t	cookie;		/* a cookie depending on rule action */
	struct inpcb	*inp;

	struct _ip6dn_args	dummypar; /* dummynet->ip6_output */
	struct sockaddr_in hopstore;	/* store here if cannot use a pointer */
};

/*
 * Function definitions.
 */

/* Firewall hooks */
struct sockopt;
struct dn_flow_set;

int ipfw_check_in(void *, struct mbuf **, struct ifnet *, int, struct inpcb *inp);
int ipfw_check_out(void *, struct mbuf **, struct ifnet *, int, struct inpcb *inp);

int ipfw_chk(struct ip_fw_args *);

int ipfw_init(void);
void ipfw_destroy(void);

typedef int ip_fw_ctl_t(struct sockopt *);
extern ip_fw_ctl_t *ip_fw_ctl_ptr;
extern int fw_one_pass;
extern int fw_enable;
#ifdef INET6
extern int fw6_enable;
#endif

/* For kernel ipfw_ether and ipfw_bridge. */
typedef	int ip_fw_chk_t(struct ip_fw_args *args);
extern	ip_fw_chk_t	*ip_fw_chk_ptr;
#define	IPFW_LOADED	(ip_fw_chk_ptr != NULL)

#ifdef IPFW_INTERNAL

#define        IPFW_TABLES_MAX         128
struct ip_fw_chain {
       struct ip_fw    *rules;         /* list of rules */
       struct ip_fw    *reap;          /* list of rules to reap */
       LIST_HEAD(, cfg_nat) nat;       /* list of nat entries */
       struct radix_node_head *tables[IPFW_TABLES_MAX];
       struct rwlock   rwmtx;
};
#define        IPFW_LOCK_INIT(_chain) \
       rw_init(&(_chain)->rwmtx, "IPFW static rules")
#define        IPFW_LOCK_DESTROY(_chain)       rw_destroy(&(_chain)->rwmtx)
#define        IPFW_WLOCK_ASSERT(_chain)       rw_assert(&(_chain)->rwmtx, RA_WLOCKED)

#define IPFW_RLOCK(p) rw_rlock(&(p)->rwmtx)
#define IPFW_RUNLOCK(p) rw_runlock(&(p)->rwmtx)
#define IPFW_WLOCK(p) rw_wlock(&(p)->rwmtx)
#define IPFW_WUNLOCK(p) rw_wunlock(&(p)->rwmtx)

#define LOOKUP_NAT(l, i, p) do {                                       \
               LIST_FOREACH((p), &(l.nat), _next) {                    \
                       if ((p)->id == (i)) {                           \
                               break;                                  \
                       }                                               \
               }                                                       \
       } while (0)

typedef int ipfw_nat_t(struct ip_fw_args *, struct cfg_nat *, struct mbuf *);
typedef int ipfw_nat_cfg_t(struct sockopt *);
#endif

#endif /* _KERNEL */
#endif /* _IPFW2_H */
