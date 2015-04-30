/*
    midnitelogger - A data logging software for use with Midnite Solar's
                    "Classic" charge controllers over TCP 

    Copyright (C) 2015 Jason Hughes (wk_fs@skie.net)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ML_MAIN_H
#define ML_MAIN_H


typedef struct {
	int cid; // charge controller id number
	char ip[CC_MAX_HOST_LEN+18]; // ip address
	char name[9]; // short name
	unsigned char alive; // alive?!

	// socket stuff
	int s;
	struct hostent *hp;
	struct sockaddr_in sa;

	// modbus registers - might as well keep these
	// we'll use this for 4100 to 4355
	unsigned short modbus_register[256];

	// last successful read
	unsigned int last_success;
} CC_DATA;


PGresult* psql_query(PGconn *lconn, char *str);
int load_charge_controllers(PGconn *conn);
void connect_all();
void gather_data_all();
void close_all();
int modbus_read_registers_command(int cc, int addr, int number, int offset);
int modbus_read_registers_finish_read(int cc, int addr, int number, int offset);
void wait_for_write_ready();
void wait_for_data_ready();
void write_to_db(PGconn *conn);
void get_from_all(int offset, int count);
void print_usage(char *pname);
void print_local_status();

#endif
