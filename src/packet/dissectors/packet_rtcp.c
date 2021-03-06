/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file packet_rtcp.c
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Source of functions defined in packet_rtcp.h
 */

#include "config.h"
#include <glib.h>
#include "glib-extra.h"
#include "storage.h"
#include "packet/dissector.h"
#include "packet/packet.h"
#include "packet/dissectors/packet_ip.h"
#include "packet/dissectors/packet_udp.h"
#include "packet_rtp.h"


/**
 * @brief Check if the data is a RTCP packet
 * RFC 5761 Section 4.  Distinguishable RTP and RTCP Packets
 * RFC 5764 Section 5.1.2.  Reception (packet demultiplexing)
 */
static gboolean
packet_rtcp_valid(GByteArray *data)
{
    g_return_val_if_fail(data != NULL, FALSE);
    struct rtcp_hdr_generic *hdr = (struct rtcp_hdr_generic *) data;

    if ((data->len >= RTCP_HDR_LENGTH) &&
        (RTP_VERSION(data->data[0]) == RTP_VERSION_RFC1889) &&
        (data->data[0] > 127 && data->data[0] < 192) &&
        (hdr->type >= 192 && hdr->type <= 223)) {
        return 0;
    }

    // Not a RTCP packet
    return FALSE;
}

static GByteArray *
packet_rtcp_parse(G_GNUC_UNUSED PacketParser *parser, Packet *packet, GByteArray *data)
{
    struct rtcp_hdr_generic hdr;
    struct rtcp_hdr_sr hdr_sr;
    struct rtcp_hdr_xr hdr_xr;
    struct rtcp_blk_xr blk_xr;
    struct rtcp_blk_xr_voip blk_xr_voip;

    // Not RTCP Packet
    if (!packet_rtcp_valid(data)) {
        return data;
    }

    // Allocate RTCP packet data
    PacketRtcpData *rtcp = g_malloc0(sizeof(PacketRtcpData));

    // Parse all packet payload headers
    while (data->len > 0) {

        // Check we have at least rtcp generic info
        if (data->len < sizeof(struct rtcp_hdr_generic))
            break;

        // Copy into RTCP generic header
        memcpy(&hdr, data->data, sizeof(hdr));

        // Check RTP version
        if (RTP_VERSION(hdr.version) != RTP_VERSION_RFC1889)
            break;

        // Header length
        guint hlen = (guint) g_ntohs(hdr.len) * 4 + 4;

        // No enough data for this RTCP header
        if (hlen > data->len)
            break;

        // Check RTCP packet header type
        switch (hdr.type) {
            case RTCP_HDR_SR:
                // Get Sender Report header
                memcpy(&hdr_sr, data->data, sizeof(hdr_sr));
                rtcp->spc = ntohl(hdr_sr.spc);
                break;
            case RTCP_HDR_RR:
            case RTCP_HDR_SDES:
            case RTCP_HDR_BYE:
            case RTCP_HDR_APP:
            case RTCP_RTPFB:
            case RTCP_PSFB:
                break;
            case RTCP_XR:
                // Get Sender Report Extended header
                memcpy(&hdr_xr, data->data, sizeof(hdr_xr));
                gsize bsize = sizeof(hdr_xr);

                // Read all report blocks
                while (bsize < (guint) ntohs(hdr_xr.len) * 4 + 4) {
                    // Read block header
                    memcpy(&blk_xr, data->data + bsize, sizeof(blk_xr));
                    // Check block type
                    switch (blk_xr.type) {
                        case RTCP_XR_VOIP_METRCS:
                            memcpy(&blk_xr_voip, data->data + sizeof(hdr_xr), sizeof(blk_xr_voip));
                            rtcp->fdiscard = blk_xr_voip.drate;
                            rtcp->flost = blk_xr_voip.lrate;
                            rtcp->mosl = blk_xr_voip.moslq;
                            rtcp->mosc = blk_xr_voip.moscq;
                            break;
                        default:
                            break;
                    }
                    bsize += ntohs(blk_xr.len) * 4 + 4;
                }
                break;
            case RTCP_AVB:
            case RTCP_RSI:
            case RTCP_TOKEN:
            default:
                // Not handled headers. Skip the rest of this packet
                g_byte_array_set_size(data, 0);
                break;
        }

        // Remove this header data
        g_byte_array_remove_range(data, 0, hlen);
    }

    // Set packet RTP informaiton
    g_ptr_array_set(packet->proto, PACKET_RTCP, rtcp);

    /** @TODO Backwards compatibility during refactoring */
    storage_add_packet(packet);
    return NULL;
}

PacketDissector *
packet_rtcp_new()
{
    PacketDissector *proto = g_malloc0(sizeof(PacketDissector));
    proto->id = PACKET_RTCP;
    proto->dissect = packet_rtcp_parse;

    return proto;
}
