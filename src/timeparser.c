/*
 * ZNC Helper
 * Copyright (C) 2009 Konrad Gräfe
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include "config.h"
#include "internal.h"

#include "timeparser.h"

time_t get_time(const char *timestamp) {
	static struct tm t;
	
	int read = 0;
	int year = 0;
	int month = 0;
	
	read = sscanf(timestamp, "[%04d-%02d-%02d %02d:%02d:%02d]", &year, &month, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec);
	
	if(read != 6 || year <= 1900) return 0;
	
	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	
	return mktime(&t);
} 
