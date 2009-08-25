/*
 * This file is part of oFono - Open Source Telephony
 *
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Aki Niemi <aki.niemi@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include <gisi/client.h>

#include <ofono/log.h>
#include <ofono/modem.h>
#include <ofono/call-meter.h>

#include "isi.h"

#define PN_SS			0x06

struct call_meter_data {
	GIsiClient *client;
	struct isi_version version;
};

static void isi_call_meter_query(struct ofono_call_meter *cm,
					ofono_call_meter_query_cb_t cb,
					void *data)
{
}

static void isi_acm_query(struct ofono_call_meter *cm,
				ofono_call_meter_query_cb_t cb,
				void *data)
{
}

static void isi_acm_reset(struct ofono_call_meter *cm, const char *sim_pin2,
				ofono_call_meter_set_cb_t cb, void *data)
{
}

static void isi_acm_max_query(struct ofono_call_meter *cm,
				ofono_call_meter_query_cb_t cb, void *data)
{
}

static void isi_acm_max_set(struct ofono_call_meter *cm, int new_value,
				const char *sim_pin2,
				ofono_call_meter_set_cb_t cb, void *data)
{
}

static void isi_puct_query(struct ofono_call_meter *cm,
				ofono_call_meter_puct_query_cb_t cb, void *data)
{
}

static void isi_puct_set(struct ofono_call_meter *cm, const char *currency,
				double ppu, const char *sim_pin2,
				ofono_call_meter_set_cb_t cb, void *data)
{
}

static int isi_call_meter_probe(struct ofono_call_meter *cm)
{
	GIsiModem *idx = ofono_call_meter_get_data(cm);
	struct call_meter_data *data = g_try_new0(struct call_meter_data, 1);

	if (!data)
		return -ENOMEM;

	data->client = g_isi_client_create(idx, PN_SS);
	if (!data->client)
		return -ENOMEM;

	ofono_call_meter_set_data(cm, data);

	return 0;
}

static int isi_call_meter_remove(struct ofono_call_meter *cm)
{
	struct call_meter_data *data = ofono_call_meter_get_data(cm);

	if (data) {
		g_isi_client_destroy(data->client);
		g_free(data);
	}

	return 0;
}

static struct ofono_call_meter_driver driver = {
	.name			= "isi",
	.probe			= isi_call_meter_probe,
	.remove			= isi_call_meter_remove,
	.call_meter_query	= isi_call_meter_query,
	.acm_query		= isi_acm_query,
	.acm_reset		= isi_acm_reset,
	.acm_max_query		= isi_acm_max_query,
	.acm_max_set		= isi_acm_max_set,
	.puct_query		= isi_puct_query,
	.puct_set		= isi_puct_set
};

void isi_call_meter_init()
{
	ofono_call_meter_driver_register(&driver);
}

void isi_call_meter_exit()
{
	ofono_call_meter_driver_unregister(&driver);
}
