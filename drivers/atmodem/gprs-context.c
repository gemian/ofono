/*
 *
 *  oFono - Open Source Telephony
 *
 *  Copyright (C) 2008-2010  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include <ofono/log.h>
#include <ofono/modem.h>
#include <ofono/gprs-context.h>

#include "gatchat.h"
#include "gatresult.h"
#include "gatppp.h"

#include "atmodem.h"

#define STATIC_IP_NETMASK "255.255.255.248"

static const char *none_prefix[] = { NULL };

enum state {
	STATE_IDLE,
	STATE_ENABLING,
	STATE_DISABLING,
	STATE_ACTIVE,
};

struct gprs_context_data {
	GAtChat *chat;
	unsigned int active_context;
	char username[OFONO_GPRS_MAX_USERNAME_LENGTH + 1];
	char password[OFONO_GPRS_MAX_PASSWORD_LENGTH + 1];
	GAtPPP *ppp;
	enum state state;
	union {
		ofono_gprs_context_cb_t down_cb;        /* Down callback */
		ofono_gprs_context_up_cb_t up_cb;       /* Up callback */
	};
	void *cb_data;                                  /* Callback data */
};

static void at_cgact_down_cb(gboolean ok, GAtResult *result, gpointer user_data)
{
	struct cb_data *cbd = user_data;
	ofono_gprs_context_cb_t cb = cbd->cb;
	struct ofono_gprs_context *gc = cbd->user;
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);
	struct ofono_error error;

	if (ok)
		gcd->active_context = 0;

	decode_at_error(&error, g_at_result_final_response(result));

	cb(&error, cbd->data);
}

static void ppp_connect(GAtPPPConnectStatus success,
			const char *interface, const char *ip,
			const char *dns1, const char *dns2,
			gpointer user_data)
{
	struct ofono_gprs_context *gc = user_data;
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);
	const char *dns[3];

	if (success != G_AT_PPP_CONNECT_SUCCESS) {
		gcd->active_context = 0;
		gcd->state = STATE_IDLE;

		CALLBACK_WITH_FAILURE(gcd->up_cb, NULL, 0, NULL, NULL, NULL,
					NULL, gcd->cb_data);
		return;
	}

	dns[0] = dns1;
	dns[1] = dns2;
	dns[2] = 0;

	gcd->state = STATE_ACTIVE;
	CALLBACK_WITH_SUCCESS(gcd->up_cb, interface, TRUE, ip,
					STATIC_IP_NETMASK, NULL,
					dns, gcd->cb_data);
}

static void ppp_disconnect(gpointer user_data)
{
	struct ofono_gprs_context *gc = user_data;
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);

	ofono_gprs_context_deactivated(gc, gcd->active_context);
	gcd->active_context = 0;
	gcd->state = STATE_IDLE;
}

static gboolean setup_ppp(struct ofono_gprs_context *gc)
{
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);
	GIOChannel *channel;

	channel = g_at_chat_get_channel(gcd->chat);
	g_at_chat_unref(gcd->chat);

	/* open ppp */
	gcd->ppp = g_at_ppp_new(channel);

	if (gcd->ppp == NULL)
		return FALSE;

	g_at_ppp_set_credentials(gcd->ppp, gcd->username, gcd->password);

	/* set connect and disconnect callbacks */
	g_at_ppp_set_connect_function(gcd->ppp, ppp_connect, gc);
	g_at_ppp_set_disconnect_function(gcd->ppp, ppp_disconnect, gc);

	/* open the ppp connection */
	g_at_ppp_open(gcd->ppp);

	return TRUE;
}

static void at_cgdata_cb(gboolean ok, GAtResult *result, gpointer user_data)
{
	struct ofono_gprs_context *gc = user_data;
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);

	if (!ok) {
		struct ofono_error error;

		ofono_info("Unable to enter data state");

		gcd->active_context = 0;
		gcd->state = STATE_IDLE;

		decode_at_error(&error, g_at_result_final_response(result));
		gcd->up_cb(&error, NULL, 0, NULL, NULL, NULL, NULL,
				gcd->cb_data);
		return;
	}

	setup_ppp(gc);
}

static void at_cgdcont_cb(gboolean ok, GAtResult *result, gpointer user_data)
{
	struct ofono_gprs_context *gc = user_data;
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);
	char buf[64];

	if (!ok) {
		struct ofono_error error;

		gcd->active_context = 0;
		gcd->state = STATE_IDLE;

		decode_at_error(&error, g_at_result_final_response(result));
		gcd->up_cb(&error, NULL, 0, NULL, NULL, NULL, NULL,
				gcd->cb_data);
		return;
	}

	sprintf(buf, "AT+CGDATA=\"PPP\",%u", gcd->active_context);
	if (g_at_chat_send(gcd->chat, buf, none_prefix,
				at_cgdata_cb, gc, NULL) > 0)
		return;

	gcd->active_context = 0;
	gcd->state = STATE_IDLE;

	CALLBACK_WITH_FAILURE(gcd->up_cb, NULL, 0, NULL, NULL, NULL, NULL,
				gcd->cb_data);
}

static void at_gprs_activate_primary(struct ofono_gprs_context *gc,
				const struct ofono_gprs_primary_context *ctx,
				ofono_gprs_context_up_cb_t cb, void *data)
{
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);
	char buf[OFONO_GPRS_MAX_APN_LENGTH + 128];
	int len;

	gcd->active_context = ctx->cid;
	gcd->up_cb = cb;
	gcd->cb_data = data;
	memcpy(gcd->username, ctx->username, sizeof(ctx->username));
	memcpy(gcd->password, ctx->password, sizeof(ctx->password));

	gcd->state = STATE_ENABLING;

	len = snprintf(buf, sizeof(buf), "AT+CGDCONT=%u,\"IP\"", ctx->cid);

	if (ctx->apn)
		snprintf(buf + len, sizeof(buf) - len - 3, ",\"%s\"",
				ctx->apn);

	if (g_at_chat_send(gcd->chat, buf, none_prefix,
				at_cgdcont_cb, gc, NULL) > 0)
		return;

	CALLBACK_WITH_FAILURE(cb, NULL, 0, NULL, NULL, NULL, NULL, data);
}

static void at_gprs_deactivate_primary(struct ofono_gprs_context *gc,
					unsigned int id,
					ofono_gprs_context_cb_t cb, void *data)
{
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);
	struct cb_data *cbd = cb_data_new(cb, data);
	char buf[64];

	if (!cbd)
		goto error;

	cbd->user = gc;

	snprintf(buf, sizeof(buf), "AT+CGACT=0,%u", id);

	if (g_at_chat_send(gcd->chat, buf, none_prefix,
				at_cgact_down_cb, cbd, g_free) > 0)
		return;

error:
	if (cbd)
		g_free(cbd);

	CALLBACK_WITH_FAILURE(cb, data);
}

static int at_gprs_context_probe(struct ofono_gprs_context *gc,
					unsigned int vendor, void *data)
{
	GAtChat *chat = data;
	struct gprs_context_data *gcd;

	gcd = g_new0(struct gprs_context_data, 1);
	gcd->chat = chat;

	ofono_gprs_context_set_data(gc, gcd);

	return 0;
}

static void at_gprs_context_remove(struct ofono_gprs_context *gc)
{
	struct gprs_context_data *gcd = ofono_gprs_context_get_data(gc);

	ofono_gprs_context_set_data(gc, NULL);
	g_free(gcd);
}

static struct ofono_gprs_context_driver driver = {
	.name			= "atmodem",
	.probe			= at_gprs_context_probe,
	.remove			= at_gprs_context_remove,
	.activate_primary	= at_gprs_activate_primary,
	.deactivate_primary	= at_gprs_deactivate_primary,
};

void at_gprs_context_init()
{
	ofono_gprs_context_driver_register(&driver);
}

void at_gprs_context_exit()
{
	ofono_gprs_context_driver_unregister(&driver);
}
