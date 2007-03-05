
		PFC Workaround in Netgraph PPP Implementation
		---------------------------------------------

An interoperability problem has arisen when using certain broken PPTP
implementations with the netgraph PPTP/PPP code.  This is, at least in part,
due to a lack of clear specification in the RFCs as to whether protocol-field
compression should be allowed for additional nested PPP encapsulations.  It
is never explicitly stated whether the LCP-negotiated PFC enable is to apply
to additional levels.  Although the PPP protocol encoding was designed to be
self-describing with respect to PFC, and hence the robustness principle dictates
that it should always be accepted by the receiver, in practice there are
implementations that choke on unexpected PFC.

Part of the problem arises because, when Multilink PPP is in use, most levels
of protocol type are per-bundle rather than per-link, but there are no LCP
negotiations at the bundle level.  Thus, the PFC enable is conceptually
nonexistent in the protocol for some protocol levels.  However, RFC1990 does
suggest using the PFC enable from the first link to determine the bundle's use
of PFC.

There are three places in ng_ppp.c where PPP protocol types are inserted, with
possible PFC.  Two are used only at the bundle level, and normally enable PFC
unconditionally.  The third could be used at either the link or bundle level,
and uses the link's PFC enable in the latter case while unconditionally enabling
it in the former.

The initially recommended patch to get around the buggy peer involved disabling
PFC in the two calls where it was unconditionally true.  This of course means
disabling PFC even in cases where it works.  The version of ng_ppp.c released
with FreeBSD 4.11 made this change in *one of* the two places (perhaps the only
one immediately causing trouble) while leaving the other alone.  The version
released with FreeBSD 5.3 did not have this change at all.

The modification to ng_ppp.c here changes all three bundle-level protocol-type
insertions to use the PFC enable from the first link as the condition.  While
this is not completely ideal, it does permit PFC to be used everywhere when it
doesn't cause trouble, while also permitting it to be disabled by configuration
at either end.  In particular, it can be disabled in buggy peers without
penalizing others.

A more flexible approach would be to introuduce a bundle-level PFC enable in
the configuration parameters, perhaps even three separate enables (one for each
instance in the code).  That would allow the userland code to decide where PFC
is permitted, without further kernel changes.  Probably the most reasonable
default would be to derive those enables from the first link (as is hard-coded
now), or perhaps even from the AND across all links.


Although RFC1990 suggests taking alignment considerations into account when
deciding whether or not to use PFC, that issue is not addressed by this change.

					Fred Wright
					fw@well.com
					5-Apr-2005
