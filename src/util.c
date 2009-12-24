/*
 *
 *  oFono - Open Source Telephony
 *
 *  Copyright (C) 2008-2009  Intel Corporation. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <glib.h>

#include "util.h"

/*
	Name:			GSM 03.38 to Unicode
	Unicode version:	3.0
	Table version:		1.1
	Table format:		Format A
	Date:			2000 May 30
	Authors:		Ken Whistler
				Kent Karlsson
				Markus Kuhn

	Copyright (c) 2000 Unicode, Inc.  All Rights reserved.

	This file is provided as-is by Unicode, Inc. (The Unicode Consortium).
	No claims are made as to fitness for any particular purpose.  No
	warranties of any kind are expressed or implied.  The recipient
	agrees to determine applicability of information provided.  If this
	file has been provided on optical media by Unicode, Inc., the sole
	remedy for any claim will be exchange of defective media within 90
	days of receipt.

	Unicode, Inc. hereby grants the right to freely use the information
	supplied in this file in the creation of products supporting the
	Unicode Standard, and to make copies of this file in any form for
	internal or external distribution as long as this notice remains
	attached.
*/

#define GUND			0xFFFF

#define UTF8_LENGTH(c) \
	((c) < 0x80 ? 1 : \
	 ((c) < 0x800 ? 2 : 3))

#define TABLE_SIZE(t) \
	(sizeof((t)) / sizeof(struct codepoint))

struct codepoint {
	unsigned short from;
	unsigned short to;
};

struct alphabet_conversion_table {
	const unsigned short *togsm_locking_shift;
	const struct codepoint *togsm_single_shift;
	unsigned int togsm_single_shift_len;
	const struct codepoint *tounicode_locking_shift;
	const struct codepoint *tounicode_single_shift;
	unsigned int tounicode_single_shift_len;
};

/* GSM to Unicode extension table, for GSM sequences starting with 0x1B */
static const struct codepoint def_ext_gsm[] = {
	{ 0x0A, 0x000C },		/* See NOTE 3 in 23.038 */
	{ 0x14, 0x005E },
	{ 0x1B, 0x0020 },		/* See NOTE 1 in 23.038 */
	{ 0x28, 0x007B },
	{ 0x29, 0x007D },
	{ 0x2F, 0x005C },
	{ 0x3C, 0x005B },
	{ 0x3D, 0x007E },
	{ 0x3E, 0x005D },
	{ 0x40, 0x007C },
	{ 0x65, 0x20AC }
};

static const struct codepoint def_ext_unicode[] = {
	{ 0x000C, 0x1B0A },
	{ 0x005B, 0x1B3C },
	{ 0x005C, 0x1B2F },
	{ 0x005D, 0x1B3E },
	{ 0x005E, 0x1B14 },
	{ 0x007B, 0x1B28 },
	{ 0x007C, 0x1B40 },
	{ 0x007D, 0x1B29 },
	{ 0x007E, 0x1B3D },
	{ 0x20AC, 0x1B65 }
};

/* Appendix A.2.1. in 3GPP TS23.038, V.8.2.0 */
static const struct codepoint tur_ext_gsm[] = {
	{ 0x0A, 0x000C },		/* See NOTE 3 */
	{ 0x14, 0x005E },
	{ 0x1B, 0x0020 },		/* See NOTE 1 */
	{ 0x28, 0x007B },
	{ 0x29, 0x007D },
	{ 0x2F, 0x005C },
	{ 0x3C, 0x005B },
	{ 0x3D, 0x007E },
	{ 0x3E, 0x005D },
	{ 0x40, 0x007C },
	{ 0x47, 0x011E },
	{ 0x49, 0x0130 },
	{ 0x53, 0x015E },
	{ 0x63, 0x00E7 },
	{ 0x65, 0x20AC },
	{ 0x67, 0x011F },
	{ 0x69, 0x0131 },
	{ 0x73, 0x015F }
};

static const struct codepoint tur_ext_unicode[] = {
	{ 0x000C, 0x1B0A },
	{ 0x005B, 0x1B3C },
	{ 0x005C, 0x1B2F },
	{ 0x005D, 0x1B3E },
	{ 0x005E, 0x1B14 },
	{ 0x007B, 0x1B28 },
	{ 0x007C, 0x1B40 },
	{ 0x007D, 0x1B29 },
	{ 0x007E, 0x1B3D },
	{ 0x00E7, 0x1B63 },
	{ 0x011E, 0x1B47 },
	{ 0x011F, 0x1B67 },
	{ 0x0130, 0x1B49 },
	{ 0x0131, 0x1B69 },
	{ 0x015E, 0x1B53 },
	{ 0x015F, 0x1B73 },
	{ 0x20AC, 0x1B65 }
};

/* Appendix A.2.2. in 3GPP TS23.038 V.8.2.0*/
static const struct codepoint spa_ext_gsm[] = {
	{ 0x09, 0x00E7 },
	{ 0x0A, 0x000C },		/* See NOTE 3 */
	{ 0x14, 0x005E },
	{ 0x1B, 0x0020 },		/* See NOTE 1 */
	{ 0x28, 0x007B },
	{ 0x29, 0x007D },
	{ 0x2F, 0x005C },
	{ 0x3C, 0x005B },
	{ 0x3D, 0x007E },
	{ 0x3E, 0x005D },
	{ 0x40, 0x007C },
	{ 0x41, 0x00C1 },
	{ 0x49, 0x00CD },
	{ 0x4F, 0x00D3 },
	{ 0x55, 0x00DA },
	{ 0x61, 0x00E1 },
	{ 0x65, 0x20AC },
	{ 0x69, 0x00ED },
	{ 0x6F, 0x00F3 },
	{ 0x75, 0x00FA }
};

static const struct codepoint spa_ext_unicode[] = {
	{ 0x000C, 0x1B0A },
	{ 0x005B, 0x1B3C },
	{ 0x005C, 0x1B2F },
	{ 0x005D, 0x1B3E },
	{ 0x005E, 0x1B14 },
	{ 0x007B, 0x1B28 },
	{ 0x007C, 0x1B40 },
	{ 0x007D, 0x1B29 },
	{ 0x007E, 0x1B3D },
	{ 0x00C1, 0x1B41 },
	{ 0x00CD, 0x1B49 },
	{ 0x00D3, 0x1B4F },
	{ 0x00DA, 0x1B55 },
	{ 0x00E1, 0x1B61 },
	{ 0x00E7, 0x1B09 },
	{ 0x00ED, 0x1B69 },
	{ 0x00F3, 0x1B6F },
	{ 0x00FA, 0x1B75 },
	{ 0x20AC, 0x1B65 }
};

/* Appendix A.2.3. in 3GPP TS23.038 V.8.2.0 */
static const struct codepoint por_ext_gsm[] = {
	{ 0x05, 0x00EA },
	{ 0x09, 0x00E7 },
	{ 0x0A, 0x000C },		/* See NOTE 3 */
	{ 0x0B, 0x00D4 },
	{ 0x0C, 0x00F4 },
	{ 0x0E, 0x00C1 },
	{ 0x0F, 0x00E1 },
	{ 0x12, 0x03A6 },
	{ 0x13, 0x0393 },
	{ 0x14, 0x005E },
	{ 0x15, 0x03A9 },
	{ 0x16, 0x03A0 },
	{ 0x17, 0x03A8 },
	{ 0x18, 0x03A3 },
	{ 0x19, 0x0398 },
	{ 0x1B, 0x0020 },		/* See NOTE 1 */
	{ 0x1F, 0x00CA },
	{ 0x28, 0x007B },
	{ 0x29, 0x007D },
	{ 0x2F, 0x005C },
	{ 0x3C, 0x005B },
	{ 0x3D, 0x007E },
	{ 0x3E, 0x005D },
	{ 0x40, 0x007C },
	{ 0x41, 0x00C0 },
	{ 0x49, 0x00CD },
	{ 0x4F, 0x00D3 },
	{ 0x55, 0x00DA },
	{ 0x5B, 0x00C3 },
	{ 0x5C, 0x00D5 },
	{ 0x61, 0x00C2 },
	{ 0x65, 0x20AC },
	{ 0x69, 0x00ED },
	{ 0x6F, 0x00F3 },
	{ 0x75, 0x00FA },
	{ 0x7B, 0x00E3 },
	{ 0x7C, 0x00F5 },
	{ 0x7F, 0x00E2 }
};

static const struct codepoint por_ext_unicode[] = {
	{ 0x000C, 0x1B0A },
	{ 0x005B, 0x1B3C },
	{ 0x005C, 0x1B2F },
	{ 0x005D, 0x1B3E },
	{ 0x005E, 0x1B14 },
	{ 0x007B, 0x1B28 },
	{ 0x007C, 0x1B40 },
	{ 0x007D, 0x1B29 },
	{ 0x007E, 0x1B3D },
	{ 0x00C0, 0x1B41 },
	{ 0x00C1, 0x1B0E },
	{ 0x00C2, 0x1B61 },
	{ 0x00C3, 0x1B5B },
	{ 0x00CA, 0x1B1F },
	{ 0x00CD, 0x1B49 },
	{ 0x00D3, 0x1B4F },
	{ 0x00D4, 0x1B0B },
	{ 0x00D5, 0x1B5C },
	{ 0x00DA, 0x1B55 },
	{ 0x00E1, 0x1B0F },
	{ 0x00E2, 0x1B7F },
	{ 0x00E3, 0x1B7B },
	{ 0x00E7, 0x1B09 },
	{ 0x00EA, 0x1B05 },
	{ 0x00ED, 0x1B69 },
	{ 0x00F3, 0x1B6F },
	{ 0x00F4, 0x1B0C },
	{ 0x00F5, 0x1B7C },
	{ 0x00FA, 0x1B75 },
	{ 0x0393, 0x1B13 },
	{ 0x0398, 0x1B19 },
	{ 0x03A0, 0x1B16 },
	{ 0x03A3, 0x1B18 },
	{ 0x03A6, 0x1B12 },
	{ 0x03A8, 0x1B17 },
	{ 0x03A9, 0x1B15 },
	{ 0x20AC, 0x1B65 }
};

/* Used for conversion of GSM to Unicode */
static const unsigned short def_gsm[] = {
	0x0040, 0x00A3, 0x0024, 0x00A5, 0x00E8, 0x00E9, 0x00F9, 0x00EC, /* 0x07 */
	0x00F2, 0x00C7, 0x000A, 0x00D8, 0x00F8, 0x000D, 0x00C5, 0x00E5, /* 0x0F */
	0x0394, 0x005F, 0x03A6, 0x0393, 0x039B, 0x03A9, 0x03A0, 0x03A8, /* 0x17 */
	0x03A3, 0x0398, 0x039E, 0x00A0, 0x00C6, 0x00E6, 0x00DF, 0x00C9, /* 0x1F */
	0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x0027, /* 0x27 */
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, /* 0x2F */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, /* 0x37 */
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, /* 0x3F */
	0x00A1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, /* 0x47 */
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, /* 0x4F */
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, /* 0x57 */
	0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00D1, 0x00DC, 0x00A7, /* 0x5F */
	0x00BF, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, /* 0x67 */
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, /* 0x6F */
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, /* 0x77 */
	0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00F1, 0x00FC, 0x00E0  /* 0x7F */
};

static const struct codepoint def_unicode[] = {
	{ 0x000A, 0x0A }, { 0x000D, 0x0D }, { 0x0020, 0x20 }, { 0x0021, 0x21 },
	{ 0x0022, 0x22 }, { 0x0023, 0x23 }, { 0x0024, 0x02 }, { 0x0025, 0x25 },
	{ 0x0026, 0x26 }, { 0x0027, 0x27 }, { 0x0028, 0x28 }, { 0x0029, 0x29 },
	{ 0x002A, 0x2A }, { 0x002B, 0x2B }, { 0x002C, 0x2C }, { 0x002D, 0x2D },
	{ 0x002E, 0x2E }, { 0x002F, 0x2F }, { 0x0030, 0x30 }, { 0x0031, 0x31 },
	{ 0x0032, 0x32 }, { 0x0033, 0x33 }, { 0x0034, 0x34 }, { 0x0035, 0x35 },
	{ 0x0036, 0x36 }, { 0x0037, 0x37 }, { 0x0038, 0x38 }, { 0x0039, 0x39 },
	{ 0x003A, 0x3A }, { 0x003B, 0x3B }, { 0x003C, 0x3C }, { 0x003D, 0x3D },
	{ 0x003E, 0x3E }, { 0x003F, 0x3F }, { 0x0040, 0x00 }, { 0x0041, 0x41 },
	{ 0x0042, 0x42 }, { 0x0043, 0x43 }, { 0x0044, 0x44 }, { 0x0045, 0x45 },
	{ 0x0046, 0x46 }, { 0x0047, 0x47 }, { 0x0048, 0x48 }, { 0x0049, 0x49 },
	{ 0x004A, 0x4A }, { 0x004B, 0x4B }, { 0x004C, 0x4C }, { 0x004D, 0x4D },
	{ 0x004E, 0x4E }, { 0x004F, 0x4F }, { 0x0050, 0x50 }, { 0x0051, 0x51 },
	{ 0x0052, 0x52 }, { 0x0053, 0x53 }, { 0x0054, 0x54 }, { 0x0055, 0x55 },
	{ 0x0056, 0x56 }, { 0x0057, 0x57 }, { 0x0058, 0x58 }, { 0x0059, 0x59 },
	{ 0x005A, 0x5A }, { 0x005F, 0x11 }, { 0x0061, 0x61 }, { 0x0062, 0x62 },
	{ 0x0063, 0x63 }, { 0x0064, 0x64 }, { 0x0065, 0x65 }, { 0x0066, 0x66 },
	{ 0x0067, 0x67 }, { 0x0068, 0x68 }, { 0x0069, 0x69 }, { 0x006A, 0x6A },
	{ 0x006B, 0x6B }, { 0x006C, 0x6C }, { 0x006D, 0x6D }, { 0x006E, 0x6E },
	{ 0x006F, 0x6F }, { 0x0070, 0x70 }, { 0x0071, 0x71 }, { 0x0072, 0x72 },
	{ 0x0073, 0x73 }, { 0x0074, 0x74 }, { 0x0075, 0x75 }, { 0x0076, 0x76 },
	{ 0x0077, 0x77 }, { 0x0078, 0x78 }, { 0x0079, 0x79 }, { 0x007A, 0x7A },
	{ 0x00A0, 0x20 }, { 0x00A1, 0x40 }, { 0x00A3, 0x01 }, { 0x00A4, 0x24 },
	{ 0x00A5, 0x03 }, { 0x00A7, 0x5F }, { 0x00BF, 0x60 }, { 0x00C4, 0x5B },
	{ 0x00C5, 0x0E }, { 0x00C6, 0x1C }, { 0x00C7, 0x09 }, { 0x00C9, 0x1F },
	{ 0x00D1, 0x5D }, { 0x00D6, 0x5C }, { 0x00D8, 0x0B }, { 0x00DC, 0x5E },
	{ 0x00DF, 0x1E }, { 0x00E0, 0x7F }, { 0x00E4, 0x7B }, { 0x00E5, 0x0F },
	{ 0x00E6, 0x1D }, { 0x00E8, 0x04 }, { 0x00E9, 0x05 }, { 0x00EC, 0x07 },
	{ 0x00F1, 0x7D }, { 0x00F2, 0x08 }, { 0x00F6, 0x7C }, { 0x00F8, 0x0C },
	{ 0x00F9, 0x06 }, { 0x00FC, 0x7E }, { 0x0393, 0x13 }, { 0x0394, 0x10 },
	{ 0x0398, 0x19 }, { 0x039B, 0x14 }, { 0x039E, 0x1A }, { 0x03A0, 0x16 },
	{ 0x03A3, 0x18 }, { 0x03A6, 0x12 }, { 0x03A8, 0x17 }, { 0x03A9, 0x15 }
};

/* Appendix A.3.1 in 3GPP TS23.038 */
static const unsigned short tur_gsm[] = {
	0x0040, 0x00A3, 0x0024, 0x00A5, 0x20AC, 0x00E9, 0x00F9, 0x0131, /* 0x07 */
	0x00F2, 0x00C7, 0x000A, 0x011E, 0x011F, 0x000D, 0x00C5, 0x00E5, /* 0x0F */
	0x0394, 0x005F, 0x03A6, 0x0393, 0x039B, 0x03A9, 0x03A0, 0x03A8, /* 0x17 */
	0x03A3, 0x0398, 0x039E, 0x00A0, 0x015E, 0x015F, 0x00DF, 0x00C9, /* 0x1F */
	0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x0027, /* 0x27 */
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, /* 0x2F */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, /* 0x37 */
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, /* 0x3F */
	0x0130, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, /* 0x47 */
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, /* 0x4F */
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, /* 0x57 */
	0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00D1, 0x00DC, 0x00A7, /* 0x5F */
	0x00E7, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, /* 0x67 */
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, /* 0x6F */
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, /* 0x77 */
	0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00F1, 0x00FC, 0x00E0  /* 0x7F */
};

static const struct codepoint tur_unicode[] = {
	{ 0x000A, 0x0A }, { 0x000D, 0x0D }, { 0x0020, 0x20 }, { 0x0021, 0x21 },
	{ 0x0022, 0x22 }, { 0x0023, 0x23 }, { 0x0024, 0x02 }, { 0x0025, 0x25 },
	{ 0x0026, 0x26 }, { 0x0027, 0x27 }, { 0x0028, 0x28 }, { 0x0029, 0x29 },
	{ 0x002A, 0x2A }, { 0x002B, 0x2B }, { 0x002C, 0x2C }, { 0x002D, 0x2D },
	{ 0x002E, 0x2E }, { 0x002F, 0x2F }, { 0x0030, 0x30 }, { 0x0031, 0x31 },
	{ 0x0032, 0x32 }, { 0x0033, 0x33 }, { 0x0034, 0x34 }, { 0x0035, 0x35 },
	{ 0x0036, 0x36 }, { 0x0037, 0x37 }, { 0x0038, 0x38 }, { 0x0039, 0x39 },
	{ 0x003A, 0x3A }, { 0x003B, 0x3B }, { 0x003C, 0x3C }, { 0x003D, 0x3D },
	{ 0x003E, 0x3E }, { 0x003F, 0x3F }, { 0x0040, 0x00 }, { 0x0041, 0x41 },
	{ 0x0042, 0x42 }, { 0x0043, 0x43 }, { 0x0044, 0x44 }, { 0x0045, 0x45 },
	{ 0x0046, 0x46 }, { 0x0047, 0x47 }, { 0x0048, 0x48 }, { 0x0049, 0x49 },
	{ 0x004A, 0x4A }, { 0x004B, 0x4B }, { 0x004C, 0x4C }, { 0x004D, 0x4D },
	{ 0x004E, 0x4E }, { 0x004F, 0x4F }, { 0x0050, 0x50 }, { 0x0051, 0x51 },
	{ 0x0052, 0x52 }, { 0x0053, 0x53 }, { 0x0054, 0x54 }, { 0x0055, 0x55 },
	{ 0x0056, 0x56 }, { 0x0057, 0x57 }, { 0x0058, 0x58 }, { 0x0059, 0x59 },
	{ 0x005A, 0x5A }, { 0x005F, 0x11 }, { 0x0061, 0x61 }, { 0x0062, 0x62 },
	{ 0x0063, 0x63 }, { 0x0064, 0x64 }, { 0x0065, 0x65 }, { 0x0066, 0x66 },
	{ 0x0067, 0x67 }, { 0x0068, 0x68 }, { 0x0069, 0x69 }, { 0x006A, 0x6A },
	{ 0x006B, 0x6B }, { 0x006C, 0x6C }, { 0x006D, 0x6D }, { 0x006E, 0x6E },
	{ 0x006F, 0x6F }, { 0x0070, 0x70 }, { 0x0071, 0x71 }, { 0x0072, 0x72 },
	{ 0x0073, 0x73 }, { 0x0074, 0x74 }, { 0x0075, 0x75 }, { 0x0076, 0x76 },
	{ 0x0077, 0x77 }, { 0x0078, 0x78 }, { 0x0079, 0x79 }, { 0x007A, 0x7A },
	{ 0x00A0, 0x20 }, { 0x00A3, 0x01 }, { 0x00A4, 0x24 }, { 0x00A5, 0x03 },
	{ 0x00A7, 0x5F }, { 0x00C4, 0x5B }, { 0x00C5, 0x0E }, { 0x00C7, 0x09 },
	{ 0x00C9, 0x1F }, { 0x00D1, 0x5D }, { 0x00D6, 0x5C }, { 0x00DC, 0x5E },
	{ 0x00DF, 0x1E }, { 0x00E0, 0x7F }, { 0x00E4, 0x7B }, { 0x00E5, 0x0F },
	{ 0x00E7, 0x60 }, { 0x00E9, 0x05 }, { 0x00F1, 0x7D }, { 0x00F2, 0x08 },
	{ 0x00F6, 0x7C }, { 0x00F9, 0x06 }, { 0x00FC, 0x7E }, { 0x011E, 0x0B },
	{ 0x011F, 0x0C }, { 0x0130, 0x40 }, { 0x0131, 0x07 }, { 0x015E, 0x1C },
	{ 0x015F, 0x1D }, { 0x0393, 0x13 }, { 0x0394, 0x10 }, { 0x0398, 0x19 },
	{ 0x039B, 0x14 }, { 0x039E, 0x1A }, { 0x03A0, 0x16 }, { 0x03A3, 0x18 },
	{ 0x03A6, 0x12 }, { 0x03A8, 0x17 }, { 0x03A9, 0x15 }, { 0x20AC, 0x04 }
};

/* Appendix A.3.2 in 3GPP TS23.038 */
static const unsigned short por_gsm[] = {
	0x0040, 0x00A3, 0x0024, 0x00A5, 0x00EA, 0x00E9, 0x00FA, 0x00ED, /* 0x07 */
	0x00F3, 0x00E7, 0x000A, 0x00D4, 0x00F4, 0x000D, 0x00C1, 0x00E1, /* 0x0F */
	0x0394, 0x005F, 0x00AA, 0x00C7, 0x00C0, 0x221E, 0x005E, 0x005C, /* 0x17 */
	0x20ac, 0x00D3, 0x007C, 0x00A0, 0x00C2, 0x00E2, 0x00CA, 0x00C9, /* 0x1F */
	0x0020, 0x0021, 0x0022, 0x0023, 0x00BA, 0x0025, 0x0026, 0x0027, /* 0x27 */
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, /* 0x2F */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, /* 0x37 */
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, /* 0x3F */
	0x00A1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, /* 0x47 */
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, /* 0x4F */
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, /* 0x57 */
	0x0058, 0x0059, 0x005A, 0x00C3, 0x00D5, 0x00DA, 0x00DC, 0x00A7, /* 0x5F */
	0x007E, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, /* 0x67 */
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, /* 0x6F */
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, /* 0x77 */
	0x0078, 0x0079, 0x007A, 0x00E3, 0x00F5, 0x0060, 0x00FC, 0x00E0  /* 0x7F */
};

static const struct codepoint por_unicode[] = {
	{ 0x000A, 0x0A }, { 0x000D, 0x0D }, { 0x0020, 0x20 }, { 0x0021, 0x21 },
	{ 0x0022, 0x22 }, { 0x0023, 0x23 }, { 0x0024, 0x02 }, { 0x0025, 0x25 },
	{ 0x0026, 0x26 }, { 0x0027, 0x27 }, { 0x0028, 0x28 }, { 0x0029, 0x29 },
	{ 0x002A, 0x2A }, { 0x002B, 0x2B }, { 0x002C, 0x2C }, { 0x002D, 0x2D },
	{ 0x002E, 0x2E }, { 0x002F, 0x2F }, { 0x0030, 0x30 }, { 0x0031, 0x31 },
	{ 0x0032, 0x32 }, { 0x0033, 0x33 }, { 0x0034, 0x34 }, { 0x0035, 0x35 },
	{ 0x0036, 0x36 }, { 0x0037, 0x37 }, { 0x0038, 0x38 }, { 0x0039, 0x39 },
	{ 0x003A, 0x3A }, { 0x003B, 0x3B }, { 0x003C, 0x3C }, { 0x003D, 0x3D },
	{ 0x003E, 0x3E }, { 0x003F, 0x3F }, { 0x0040, 0x00 }, { 0x0041, 0x41 },
	{ 0x0042, 0x42 }, { 0x0043, 0x43 }, { 0x0044, 0x44 }, { 0x0045, 0x45 },
	{ 0x0046, 0x46 }, { 0x0047, 0x47 }, { 0x0048, 0x48 }, { 0x0049, 0x49 },
	{ 0x004A, 0x4A }, { 0x004B, 0x4B }, { 0x004C, 0x4C }, { 0x004D, 0x4D },
	{ 0x004E, 0x4E }, { 0x004F, 0x4F }, { 0x0050, 0x50 }, { 0x0051, 0x51 },
	{ 0x0052, 0x52 }, { 0x0053, 0x53 }, { 0x0054, 0x54 }, { 0x0055, 0x55 },
	{ 0x0056, 0x56 }, { 0x0057, 0x57 }, { 0x0058, 0x58 }, { 0x0059, 0x59 },
	{ 0x005A, 0x5A }, { 0x005C, 0x17 }, { 0x005E, 0x16 }, { 0x005F, 0x11 },
	{ 0x0060, 0x7D }, { 0x0061, 0x61 }, { 0x0062, 0x62 }, { 0x0063, 0x63 },
	{ 0x0064, 0x64 }, { 0x0065, 0x65 }, { 0x0066, 0x66 }, { 0x0067, 0x67 },
	{ 0x0068, 0x68 }, { 0x0069, 0x69 }, { 0x006A, 0x6A }, { 0x006B, 0x6B },
	{ 0x006C, 0x6C }, { 0x006D, 0x6D }, { 0x006E, 0x6E }, { 0x006F, 0x6F },
	{ 0x0070, 0x70 }, { 0x0071, 0x71 }, { 0x0072, 0x72 }, { 0x0073, 0x73 },
	{ 0x0074, 0x74 }, { 0x0075, 0x75 }, { 0x0076, 0x76 }, { 0x0077, 0x77 },
	{ 0x0078, 0x78 }, { 0x0079, 0x79 }, { 0x007A, 0x7A }, { 0x007C, 0x1A },
	{ 0x007E, 0x60 }, { 0x00A0, 0x20 }, { 0x00A3, 0x01 }, { 0x00A5, 0x03 },
	{ 0x00A7, 0x5F }, { 0x00AA, 0x12 }, { 0x00BA, 0x24 }, { 0x00C0, 0x14 },
	{ 0x00C1, 0x0E }, { 0x00C2, 0x1C }, { 0x00C3, 0x5B }, { 0x00C7, 0x13 },
	{ 0x00C9, 0x1F }, { 0x00CA, 0x1E }, { 0x00CD, 0x40 }, { 0x00D3, 0x19 },
	{ 0x00D4, 0x0B }, { 0x00D5, 0x5C }, { 0x00DA, 0x5D }, { 0x00DC, 0x5E },
	{ 0x00E0, 0x7F }, { 0x00E1, 0x0F }, { 0x00E2, 0x1D }, { 0x00E3, 0x7B },
	{ 0x00E7, 0x09 }, { 0x00E9, 0x05 }, { 0x00EA, 0x04 }, { 0x00ED, 0x07 },
	{ 0x00F3, 0x08 }, { 0x00F4, 0x0C }, { 0x00F5, 0x7C }, { 0x00FA, 0x06 },
	{ 0x00FC, 0x7E }, { 0x0394, 0x10 }, { 0x20AC, 0x18 }, { 0x221E, 0x15 }
};

static const struct alphabet_conversion_table alphabet_lookup[] = {
	/* Default GSM 7 bit */
	{ def_gsm, def_ext_gsm, TABLE_SIZE(def_ext_gsm),
		def_unicode, def_ext_unicode, TABLE_SIZE(def_ext_unicode) },
	/* Turkish GSM dialect */
	{ tur_gsm, tur_ext_gsm, TABLE_SIZE(tur_ext_gsm),
		tur_unicode, tur_ext_unicode, TABLE_SIZE(tur_ext_unicode) },
	/* Spanish GSM dialect, note that this one only has extension table */
	{ def_gsm, spa_ext_gsm, TABLE_SIZE(spa_ext_gsm),
		def_unicode, spa_ext_unicode, TABLE_SIZE(spa_ext_unicode)  },
	/* Portuguese GSM dialect */
	{ por_gsm, por_ext_gsm, TABLE_SIZE(por_ext_gsm),
		por_unicode, por_ext_unicode, TABLE_SIZE(por_ext_unicode) },
};

static int compare_codepoints(const void *a, const void *b)
{
	const struct codepoint *ca = (const struct codepoint *)a;
	const struct codepoint *cb = (const struct codepoint *)b;

	return (ca->from > cb->from) - (ca->from < cb->from);
}

static unsigned short codepoint_lookup(struct codepoint *key,
					const struct codepoint *table,
					unsigned int len)
{
	struct codepoint *result = NULL;

	result = bsearch(key, table, len, sizeof(struct codepoint),
				compare_codepoints);

	return result ? result->to : GUND;
}

static unsigned short gsm_locking_shift_lookup(unsigned char k,
						unsigned char lang)
{
	return alphabet_lookup[lang].togsm_locking_shift[k];
}

static unsigned short gsm_single_shift_lookup(unsigned char k,
						unsigned char lang)
{
	struct codepoint key = { k, 0 };
	const struct codepoint *table;
	unsigned int len;
	
	table = alphabet_lookup[lang].togsm_single_shift;
	len = alphabet_lookup[lang].togsm_single_shift_len;

	return codepoint_lookup(&key, table, len);
}

static unsigned short unicode_locking_shift_lookup(unsigned short k,
							unsigned char lang)
{
	struct codepoint key = { k, 0 };
	const struct codepoint *table;
	unsigned int len = 128;

	table = alphabet_lookup[lang].tounicode_locking_shift;

	return codepoint_lookup(&key, table, len); 
}

static unsigned short unicode_single_shift_lookup(unsigned short k,
							unsigned char lang)
{
	struct codepoint key = { k, 0 };
	const struct codepoint *table;
	unsigned int len;

	table = alphabet_lookup[lang].tounicode_single_shift;
	len = alphabet_lookup[lang].tounicode_single_shift_len;

	return codepoint_lookup(&key, table, len);
}

/*!
 * Converts text coded using GSM codec into UTF8 encoded text, using
 * the given language identifiers for single shift and locking shift
 * tables.  If len is less than 0, and terminator character is given,
 * the length is computed automatically.
 *
 * Returns newly-allocated UTF8 encoded string or NULL if the conversion
 * could not be performed.  Returns the number of bytes read from the
 * GSM encoded string in items_read (if not NULL), not including the
 * terminator character. Returns the number of bytes written into the UTF8
 * encoded string in items_written (if not NULL) not including the terminal
 * '\0' character.  The caller is reponsible for freeing the returned value.
 */
char *convert_gsm_to_utf8_with_lang(const unsigned char *text, long len,
					long *items_read, long *items_written,
					unsigned char terminator,
					enum gsm_dialect locking_lang,
					enum gsm_dialect single_lang)
{
	char *res = NULL;
	char *out;
	long i = 0;
	long res_length;

	if (locking_lang >= GSM_DIALECT_INVALID)
		return NULL;

	if (single_lang >= GSM_DIALECT_INVALID)
		return NULL;

	if (len < 0 && !terminator)
		goto error;

	if (len < 0) {
		i = 0;

		while (text[i] != terminator)
			i++;

		len = i;
	}

	for (i = 0, res_length = 0; i < len; i++) {
		unsigned short c;

		if (text[i] > 0x7f)
			goto error;

		if (text[i] == 0x1b) {
			++i;
			if (i >= len)
				goto error;

			c = gsm_single_shift_lookup(text[i], single_lang);

			if (c == GUND)
				goto error;
		} else {
			c = gsm_locking_shift_lookup(text[i], locking_lang);
		}

		res_length += UTF8_LENGTH(c);
	}

	res = g_malloc(res_length + 1);

	if (!res)
		goto error;

	out = res;

	i = 0;
	while (out < res + res_length) {
		unsigned short c;

		if (text[i] == 0x1b)
			c = gsm_single_shift_lookup(text[++i], single_lang);
		else
			c = gsm_locking_shift_lookup(text[i], locking_lang);

		out += g_unichar_to_utf8(c, out);

		++i;
	}

	*out = '\0';

	if (items_written)
		*items_written = out - res;

error:
	if (items_read)
		*items_read = i;

	return res;
}

char *convert_gsm_to_utf8(const unsigned char *text, long len,
				long *items_read, long *items_written,
				unsigned char terminator)
{
	return convert_gsm_to_utf8_with_lang(text, len, items_read,
						items_written,
						terminator,
						GSM_DIALECT_DEFAULT,
						GSM_DIALECT_DEFAULT);
}

/*!
 * Converts UTF-8 encoded text to GSM alphabet.  The result is unpacked,
 * with the 7th bit always 0.  If terminator is not 0, a terminator character
 * is appended to the result.  This should be in the range 0x80-0xf0
 *
 * Returns the encoded data or NULL if the data could not be encoded.  The
 * data must be freed by the caller.  If items_read is not NULL, it contains
 * the actual number of bytes read.  If items_written is not NULL, contains
 * the number of bytes written.
 */
unsigned char *convert_utf8_to_gsm_with_lang(const char *text, long len,
					long *items_read, long *items_written,
					unsigned char terminator,
					enum gsm_dialect locking_lang,
					enum gsm_dialect single_lang)
{
	long nchars = 0;
	const char *in;
	unsigned char *out;
	unsigned char *res = NULL;
	long res_len;
	long i;

	if (locking_lang >= GSM_DIALECT_INVALID)
		return NULL;

	if (single_lang >= GSM_DIALECT_INVALID)
		return NULL;

	in = text;
	res_len = 0;

	while ((len < 0 || text + len - in > 0) && *in) {
		long max = len < 0 ? 6 : text + len - in;
		gunichar c = g_utf8_get_char_validated(in, max);
		unsigned short converted = GUND;

		if (c & 0x80000000)
			goto err_out;

		if (c > 0xffff)
			goto err_out;

		converted = unicode_locking_shift_lookup(c, locking_lang);

		if (converted == GUND)
			converted = unicode_single_shift_lookup(c, single_lang);

		if (converted == GUND)
			goto err_out;

		if (converted & 0x1b00)
			res_len += 2;
		else
			res_len += 1;

		in = g_utf8_next_char(in);
		nchars += 1;
	}

	res = g_malloc(res_len + (terminator ? 1 : 0));

	if (!res)
		goto err_out;

	in = text;
	out = res;
	for (i = 0; i < nchars; i++) {
		unsigned short converted;

		gunichar c = g_utf8_get_char(in);

		converted = unicode_locking_shift_lookup(c, locking_lang);

		if (converted == GUND)
			converted = unicode_single_shift_lookup(c, single_lang);

		if (converted & 0x1b00) {
			*out = 0x1b;
			++out;
		}

		*out = converted;
		++out;

		in = g_utf8_next_char(in);
	}

	if (terminator)
		*out = terminator;

	if (items_written)
		*items_written = out - res;

err_out:
	if (items_read)
		*items_read = in - text;

	return res;
}

unsigned char *convert_utf8_to_gsm(const char *text, long len,
					long *items_read, long *items_written,
					unsigned char terminator)
{
	return convert_utf8_to_gsm_with_lang(text, len, items_read,
						items_written,
						terminator,
						GSM_DIALECT_DEFAULT,
						GSM_DIALECT_DEFAULT);
}

/*!
 * Decodes the hex encoded data and converts to a byte array.  If terminator
 * is not 0, the terminator character is appended to the end of the result.
 * This might be useful for converting GSM encoded data if the CSCS is set
 * to HEX.
 *
 * Please note that this since GSM does allow embedded null characeters, use
 * of the terminator or the items_writen is encouraged to find the real size
 * of the result.
 */
unsigned char *decode_hex_own_buf(const char *in, long len, long *items_written,
					unsigned char terminator,
					unsigned char *buf)
{
	long i, j;
	char c;
	unsigned char b;

	if (len < 0)
		len = strlen(in);

	len &= ~0x1;

	for (i = 0, j = 0; i < len; i++, j++) {
		c = toupper(in[i]);

		if (c >= '0' && c <= '9')
			b = c - '0';
		else if (c >= 'A' && c <= 'F')
			b = 10 + c - 'A';
		else
			return NULL;

		i += 1;

		c = toupper(in[i]);

		if (c >= '0' && c <= '9')
			b = b*16 + c - '0';
		else if (c >= 'A' && c <= 'F')
			b = b*16 + 10 + c - 'A';
		else
			return NULL;

		buf[j] = b;
	}

	if (terminator)
		buf[j] = terminator;

	if (items_written)
		*items_written = j;

	return buf;
}

unsigned char *decode_hex(const char *in, long len, long *items_written,
				unsigned char terminator)
{
	long i;
	char c;
	unsigned char *buf;

	if (len < 0)
		len = strlen(in);

	len &= ~0x1;

	for (i = 0; i < len; i++) {
		c = toupper(in[i]);

		if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
			continue;

		return NULL;
	}

	buf = g_new(unsigned char, (len >> 1) + (terminator ? 1 : 0));

	return decode_hex_own_buf(in, len, items_written, terminator, buf);
}

/*!
 * Encodes the data using hexadecimal characters.  len can be negative,
 * in that case the terminator is used to find the last character.  This is
 * useful for handling GSM-encoded strings which allow ASCII NULL character
 * in the stream.
 */
char *encode_hex_own_buf(const unsigned char *in, long len,
				unsigned char terminator, char *buf)
{
	long i, j;
	char c;

	if (len < 0) {
		i = 0;

		while (in[i] != terminator)
			i++;

		len = i;
	}

	for (i = 0, j = 0; i < len; i++, j++) {
		c = (in[i] >> 4) & 0xf;

		if (c <= 9)
			buf[j] = '0' + c;
		else
			buf[j] = 'A' + c - 10;

		j += 1;

		c = (in[i]) & 0xf;

		if (c <= 9)
			buf[j] = '0' + c;
		else
			buf[j] = 'A' + c - 10;
	}

	buf[j] = '\0';

	return buf;
}

char *encode_hex(const unsigned char *in, long len, unsigned char terminator)
{
	char *buf;
	int i;

	if (len < 0) {
		i = 0;

		while (in[i] != terminator)
			i++;

		len = i;
	}

	buf = g_new(char, len * 2 + 1);

	return encode_hex_own_buf(in, len, terminator, buf);
}

unsigned char *unpack_7bit_own_buf(const unsigned char *in, long len,
					int byte_offset, gboolean ussd,
					long max_to_unpack, long *items_written,
					unsigned char terminator,
					unsigned char *buf)
{
	unsigned char rest = 0;
	unsigned char *out = buf;
	int bits = 7 - (byte_offset % 7);
	long i;

	if (len <= 0)
		return NULL;

	/* In the case of CB, unpack as much as possible */
	if (ussd == TRUE)
		max_to_unpack = len * 8 / 7;

	for (i = 0; (i < len) && ((out-buf) < max_to_unpack); i++) {
		/* Grab what we have in the current octet */
		*out = (in[i] & ((1 << bits) - 1)) << (7 - bits);

		/* Append what we have from the previous octet, if any */
		*out |= rest;

		/* Figure out the remainder */
		rest = (in[i] >> bits) & ((1 << (8-bits)) - 1);

		/* We have the entire character, here we don't increate
		 * out if this is we started at an offset.  Instead
		 * we effectively populate variable rest */
		if (i != 0 || bits == 7)
			out++;

		if ((out-buf) == max_to_unpack)
			break;

		/* We expected only 1 bit from this octet, means there's 7
		 * left, take care of them here */
		if (bits == 1) {
			*out = rest;
			out++;
			bits = 7;
			rest = 0;
		} else {
			bits = bits - 1;
		}
	}

	/* According to 23.038 6.1.2.3.1, last paragraph:
	 * "If the total number of characters to be sent equals (8n-1)
	 * where n=1,2,3 etc. then there are 7 spare bits at the end
	 * of the message. To avoid the situation where the receiving
	 * entity confuses 7 binary zero pad bits as the @ character,
	 * the carriage return or <CR> character shall be used for
	 * padding in this situation, just as for Cell Broadcast."
	 *
	 * "The receiving entity shall remove the final <CR> character where
	 * the message ends on an octet boundary with <CR> as the last
	 * character.
	 */
	if (ussd && (((out - buf) % 8) == 0) && (*(out-1) == '\r'))
			out = out - 1;

	if (terminator)
		*out = terminator;

	if (items_written)
		*items_written = out - buf;

	return buf;
}

unsigned char *unpack_7bit(const unsigned char *in, long len, int byte_offset,
				gboolean ussd, long max_to_unpack,
				long *items_written, unsigned char terminator)
{
	unsigned char *buf = g_new(unsigned char,
					len * 8 / 7 + (terminator ? 1 : 0));

	return unpack_7bit_own_buf(in, len, byte_offset, ussd, max_to_unpack,
				items_written, terminator, buf);
}

unsigned char *pack_7bit_own_buf(const unsigned char *in, long len,
					int byte_offset, gboolean ussd,
					long *items_written,
					unsigned char terminator,
					unsigned char *buf)
{
	int bits = 7 - (byte_offset % 7);
	unsigned char *out = buf;
	long i;
	long total_bits;

	if (len == 0)
		return NULL;

	if (len < 0) {
		i = 0;

		while (in[i] != terminator)
			i++;

		len = i;
	}

	total_bits = len * 7;

	if (bits != 7) {
		total_bits += bits;
		bits = bits - 1;
		*out = 0;
	}

	for (i = 0; i < len; i++) {
		if (bits != 7) {
			*out |= (in[i] & ((1 << (7 - bits)) - 1)) <<
					(bits + 1);
			out++;
		}

		/* This is a no op when bits == 0, lets keep valgrind happy */
		if (bits != 0)
			*out = in[i] >> (7 - bits);

		if (bits == 0)
			bits = 7;
		else
			bits = bits - 1;
	}

	/* If <CR> is intended to be the last character and the message
	 * (including the wanted <CR>) ends on an octet boundary, then
	 * another <CR> must be added together with a padding bit 0. The
	 * receiving entity will perform the carriage return function twice,
	 * but this will not result in misoperation as the definition of
	 * <CR> in clause 6.1.1 is identical to the definition of <CR><CR>.
	 */
	if (ussd && ((total_bits % 8) == 1))
		*out |= '\r' << 1;

	if (bits != 7)
		out++;

	if (ussd && ((total_bits % 8) == 0) && (in[len-1] == '\r')) {
		*out = '\r';
		out++;
	}

	if (items_written)
		*items_written = out - buf;

	return buf;
}

unsigned char *pack_7bit(const unsigned char *in, long len, int byte_offset,
				gboolean ussd, long *items_written,
				unsigned char terminator)
{
	int bits = 7 - (byte_offset % 7);
	long i;
	long total_bits;
	unsigned char *buf;

	if (len == 0 || !items_written)
		return NULL;

	if (len < 0) {
		i = 0;

		while (in[i] != terminator)
			i++;

		len = i;
	}

	total_bits = len * 7;

	if (bits != 7)
		total_bits += bits;

	/* Round up number of bytes, must append <cr> if true */
	if (ussd && ((total_bits % 8) == 0) && (in[len-1] == '\r'))
		buf = g_new(unsigned char, (total_bits + 14) / 8);
	else
		buf = g_new(unsigned char, (total_bits + 7) / 8);

	return pack_7bit_own_buf(in, len, byte_offset, ussd, items_written,
					terminator, buf);
}

char *sim_string_to_utf8(const unsigned char *buffer, int length)
{
	int i;
	int j;
	int num_chars;
	unsigned short ucs2_offset;
	int res_len;
	int offset;
	char *utf8 = NULL;
	char *out;

	if (length < 1)
		return NULL;

	if (buffer[0] < 0x80) {
		/* We have to find the real length, since on SIM file system
		 * alpha fields are 0xff padded
		 */
		for (i = 0; i < length; i++)
			if (buffer[i] == 0xff)
				break;

		return convert_gsm_to_utf8(buffer, i, NULL, NULL, 0);
	}

	switch (buffer[0]) {
	case 0x80:
		if (((length - 1) % 2) == 1) {
			if (buffer[length - 1] != 0xff)
				return NULL;

			length = length - 1;
		}

		for (i = 1; i < length; i += 2)
			if (buffer[i] == 0xff && buffer[i + 1] == 0xff)
				break;

		return g_convert((char *)buffer + 1, i - 1,
					"UTF-8//TRANSLIT", "UCS-2BE",
					NULL, NULL, NULL);
	case 0x81:
		if (length < 3 || (buffer[1] > (length - 3)))
			return NULL;

		num_chars = buffer[1];
		ucs2_offset = buffer[2] << 7;
		offset = 3;
		break;

	case 0x82:
		if (length < 4 || buffer[1] > length - 4)
			return NULL;

		num_chars = buffer[1];
		ucs2_offset = (buffer[2] << 8) | buffer[3];
		offset = 4;
		break;

	default:
		return NULL;
	}

	res_len = 0;
	i = offset;
	j = 0;

	while ((i < length) && (j < num_chars)) {
		unsigned short c;

		if (buffer[i] & 0x80) {
			c = (buffer[i++] & 0x7f) + ucs2_offset;

			if (c >= 0xd800 && c < 0xe000)
				return NULL;

			res_len += UTF8_LENGTH(c);
			j += 1;
			continue;
		}

		if (buffer[i] == 0x1b) {
			++i;
			if (i >= length)
				return NULL;

			c = gsm_single_shift_lookup(buffer[i++], 0);

			if (c == 0)
				return NULL;

			j += 2;
		} else {
			c = gsm_locking_shift_lookup(buffer[i++], 0);
			j += 1;
		}

		res_len += UTF8_LENGTH(c);
	}

	if (j != num_chars)
		return NULL;

	/* Check that the string is padded out to the length by 0xff */
	for (; i < length; i++)
		if (buffer[i] != 0xff)
			return NULL;

	utf8 = g_malloc(res_len + 1);

	if (!utf8)
		return NULL;

	i = offset;
	out = utf8;

	while (out < utf8 + res_len) {
		unsigned short c;

		if (buffer[i] & 0x80)
			c = (buffer[i++] & 0x7f) + ucs2_offset;
		else if (buffer[i] == 0x1b) {
			++i;
			c = gsm_single_shift_lookup(buffer[i++], 0);
		} else
			c = gsm_locking_shift_lookup(buffer[i++], 0);

		out += g_unichar_to_utf8(c, out);
	}

	*out = '\0';

	return utf8;
}
