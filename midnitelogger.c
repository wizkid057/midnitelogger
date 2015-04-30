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


#include "midnitelogger.h"

int cc_count = 0;
CC_DATA *cclist=NULL;

int debug=0;
int local_output=0;
int list_from_db=1;
int write_db=1;
int poll_interval=5;
int print_header=0;
int human_output=0;
int modbus_port=502;
int temps_F=0;

void print_usage(char *pname) {
	printf("\n");
	printf("midnitelogger v" VERSION " - (C) 2015 Jason Hughes (wk_fs@skie.net)\n");

	printf("Usage: %s [-dboL] [--dbname=DB_NAME] [--dbuser=X] [--dbpass=X|--dbpassfile=X] [--dbhost=X] [--C=host [--C=host]...]\n\n", pname);
	printf("   -d               debugging output (silent otherwise)\n");
	printf("   -b               fork into background after successful launch\n");
	printf("   -o               output read data to console (default is csv output)\n");
	printf("   -T               print csv header as first output line (for use with -o)\n");
	printf("   -H               human readable local output (for use with -o, not compatible with -H)\n");
	printf("   -N               do not load charge controller list from database (requires -C)\n");
	printf("   -R               do not write data to database\n");
	printf("   -L               local usage only, no database. (same as -R -L)\n");
	printf("                     requires at least one ip/port argument (-C)\n");
	printf("                     goes well with -o.  db* arguments ignored\n");
	printf("   -C host          IP or hostname of Midnite Classic (for use with -L), can do multiple\n");
	printf("   -i seconds       polling interval in seconds (default: 5)\n");
	printf("   -O               one shot (exit after first data output)\n");
	printf("   -P port          port to use for modbus TCP connection (default: 502)\n");
	printf("                     ^ Applies to all connections (local or DB fetched)\n");
	printf("   -F               temperatures in fahrenheit (only works with -H, db always in C)\n");
	printf("   --dbname=X       postgres database name (-n)\n");
	printf("   --dbuser=X       postgres username (-u)\n");
	printf("   --dbhost=X       postgres database host/ip (default: localhost) (-h)\n");
	printf("   --dbpass=X       postgres password for dbuser (-p)\n");
	printf("   --dbpassfile=X   file containing password for dbuser (-f)\n\n");

	printf("The program will attempt to connect to all hosts specified on the command\n");
	printf("line or in the database.  If not using -O it will periodially attempt to\n");
	printf("reconnect to specified hosts it doesn't have a working connection to.\n\n");

	printf("The program ignores valid hostnames/ips that it can not connect to.\n\n");

	printf("* THE AUTHOR OF THIS SOFTWARE IS NOT AFFILIATED WITH MIDNITE SOLAR *\n");
	printf("* USE OF THIS SOFTWARE IS AT YOUR OWN RISK.  THE AUTHOR ASSUMES NO *\n");
	printf("* RESPONSIBILITY FOR ANYTHING THAT OCCURS THAT IS UNDESIRED.       *\n\n");


	printf("/----------------------------------------------------------------------\\\n");
	printf("| This program is free software: you can redistribute it and/or modify |\n");
	printf("| it under the terms of the GNU General Public License as published by |\n");
	printf("| the Free Software Foundation, either version 3 of the License, or    |\n");
	printf("| (at your option) any later version.                                  |\n");
	printf("|                                                                      |\n");
	printf("| This program is distributed in the hope that it will be useful,      |\n");
	printf("| but WITHOUT ANY WARRANTY; without even the implied warranty of       |\n");
	printf("| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |\n");
	printf("| GNU General Public License for more details.                         |\n");
	printf("|                                                                      |\n");
	printf("| You should have received a copy of the GNU General Public License    |\n");
	printf("| along with this program.  If not, see www.gnu.org/licenses/          |\n");
	printf("\\----------------------------------------------------------------------/\n\n");

	return;
}


int main(int argc, char **argv) {

	PGconn  *conn;
	PGresult *res;
	int i,j,c=0;
	extern char *optarg;
	extern int optind, opterr;
	register int op;
	char *dbname=NULL, *dbhost=NULL, *dbuser=NULL, *dbpass=NULL, *dbstring=NULL;

	int option_index;
	int fork_to_bg=0,one_shot=0;

	static struct option long_options[] = {
		{"debug", no_argument, 0, 'd'},
		{"verbose", no_argument, 0, 'd'},
		{"dbname", required_argument, 0, 'n'},
		{"dbhost", required_argument, 0, 'h'},
		{"dbuser", required_argument, 0, 'u'},
		{"dbpass", required_argument, 0, 'p'},
		{"dbpassfile", required_argument, 0, 'f'},
		{0,0,0,0}
	};
	while ((op = getopt_long(argc, argv, "i:n:h:u:p:f:C:P:bdoFHTLNOR?", long_options, &option_index)) != EOF) {
		switch (op) {
			case 0: {
				break;
			}
			case 'H': {
				human_output=1;
				break;
			}
			case 'F': {
				temps_F=1;
				break;
			}
			case 'O': {
				one_shot=1;
				break;
			}
			case 'T': {
				print_header=1;
				break;
			}
			case 'N': {
				list_from_db=0;
				break;
			}
			case 'R': {
				write_db=0;
				break;
			}
			case 'd': {
				debug=1;
				break;
			}
			case 'b': {
				fork_to_bg=1;
				break;
			}
			case 'L': {
				write_db=0;
				list_from_db=0;
				break;
			}
			case 'o': {
				local_output=1;
				break;
			}
			case 'C': {
				if (i=strlen(optarg)) {
					if (!cclist) {
						cclist = (CC_DATA *) calloc(sizeof(CC_DATA),2);
					} else {
						cclist = (CC_DATA *) realloc(cclist,sizeof(CC_DATA)*(cc_count+2));
						if (cclist) memset(&cclist[cc_count], 0, sizeof(CC_DATA));
					}
					if (!cclist) {
						printf("Error allocating memory for internal data\n");
						exit(1);
					}
					strncpy(cclist[cc_count].ip,optarg,CC_MAX_HOST_LEN-1);
					strcpy(cclist[cc_count].name,"LOCALCC");
					cclist[cc_count].cid=cc_count+1;
					cc_count++;
				}
				break;
			}
			case 'i': {
				if (i = strlen(optarg)) {
					poll_interval = atoi(optarg);
					if (poll_interval < 1) {
						printf("invalid polling interval\n");
						exit(1);
					}

				}
				break;
			}
			case 'P': {
				if (i = strlen(optarg)) {
					modbus_port = atoi(optarg);
					if ((modbus_port < 1) || (modbus_port > 65535)) {
						printf("invalid port\n");
						exit(1);
					}

				}
				break;
			}
			case 'n': {
				if (i = strlen(optarg)) {
					dbname = (char *) calloc(i+1,1);
					if (dbname)
						strcpy(dbname,optarg);
				}
				break;
			}
			case 'h': {
				if (i = strlen(optarg)) {
					dbhost = (char *) calloc(i+1,1);
					if (dbhost)
						strcpy(dbhost,optarg);
				}
				break;
			}
			case 'u': {
				if (i = strlen(optarg)) {
					dbuser = (char *) calloc(i+1,1);
					if (dbuser)
						strcpy(dbuser,optarg);
				}
				break;
			}
			case 'p': {
				if (i = strlen(optarg)) {
					dbpass = (char *) calloc(i+1,1);
					if (dbpass)
						strcpy(dbpass,optarg);
					memset(optarg, 'x', i); // wipe from command line? can't hurt
				}
				break;
			}
			case 'f': {
				FILE *pf = fopen(optarg,"r");
				if (!pf) {
					printf("Unable to open '%s': error %d: %s\n",optarg,errno,strerror(errno));
					break;
				}
				memset(optarg, 'x', strlen(optarg)); // wipe from command line? can't hurt
				dbpass = (char *) calloc(1024,1);
				if (dbpass) {
					fgets(dbpass, 1023, pf);
					if ((dbstring=strchr(dbpass, '\n')) != NULL) *dbstring = 0;
				}
				fclose(pf);
				break;
			}
			case '?':
        	        default: {
				print_usage(argv[0]);
				exit(0);
			}
		}
	}

	if(debug) printf("Debug mode enabled command line scanning done.\n");

	if (write_db || list_from_db) {
		if (!dbhost) {
			dbhost = (char *) calloc(10,1);
			if (dbhost) strcpy(dbhost,"localhost");
		}

		if (!dbname || !dbhost || !dbuser || !dbpass) {
			print_usage(argv[0]);
			exit(0);
		}

		dbstring = (char *) calloc(strlen(dbname)+strlen(dbhost)+strlen(dbuser)+strlen(dbpass)+100,1);
		if (!dbstring) {
			printf("memory error.\n");
			exit(1);
		}

		sprintf(dbstring, "dbname=%s host=%s user=%s password=%s", dbname, dbhost, dbuser, dbpass);


		conn = PQconnectdb(dbstring);
		if (PQstatus(conn) != CONNECTION_OK) {
			printf("Could not connect to database %s with user %s on host %s: %s\n",dbname, dbuser,dbhost,PQerrorMessage(conn));
			PQfinish(conn);
			return 1;
		}

		if (debug) printf("Database connected\n");



		if (list_from_db) {
			if (cclist) { // just in case some specified on command line
				free(cclist);
				cc_count = 0;
			}


			if (!load_charge_controllers(conn)) {
				printf("Unable to load charge controller list\n");
				return 1;
			}
		}
	}

	if (!cc_count) {
		printf("No IPs specified.\n");
		return 1;
	}

	if (debug) printf("\n* Starting main loop...\n");

	connect_all();

	if (print_header)
		if (!human_output)
			printf("local_id,host,battery_volts,pv_volts,battery_volts_raw,pv_volts_raw,battery_amps,pv_amps,pv_voc,watts,kWh_today,Ah_today,ext_temp,int_fet_temp,int_pcb_temp,life_kWh,life_Ah,float_seconds_today,combochargestate\n");


	if (fork_to_bg) {
		i = daemon(1,1);
		if (i==-1) {
			printf("Error forking into the background: %d: %s\n",errno,strerror(errno));
			exit(1);
		}
	}
	while (1) {
		gather_data_all();
		if (write_db) write_to_db(conn);
		if (local_output) print_local_status();

		if (one_shot) exit(0);

		c++;
		if (c == 30) {

			j = 0;
			for(i=0;i<cc_count;i++) {
				if (cclist[i].alive) j++;
			}

			if (!j) {
				close_all();
				sleep(poll_interval);
				connect_all();
			} else if (j<cc_count) {
				connect_all();
			}
			c=0;
		} else {
			sleep(poll_interval);
		}
	}
}



void print_local_status() {

	float battery_volts, pv_volts, battery_volts_raw, pv_volts_raw, battery_amps, pv_amps, pv_voc, kWh_today, ext_temp, int_fet_temp, int_pcb_temp, life_kWh;
	unsigned int watts, Ah_today, life_Ah, float_seconds_today, combochargestate;
	int i,j=0;
	char tchar = 'C';
	for(i=0;i<cc_count;i++) {
		if (cclist[i].alive) {
			j++;
			battery_volts = (float)cclist[i].modbus_register[14]/10.0f;
			pv_volts = (float)cclist[i].modbus_register[15]/10.0f;
			battery_volts_raw = (float)cclist[i].modbus_register[175]/10.0f;
			pv_volts_raw = (float)cclist[i].modbus_register[176]/10.0f;
			battery_amps = (float)cclist[i].modbus_register[16]/10.0f;
			pv_amps = (float)cclist[i].modbus_register[20]/10.0f;
			pv_voc = (float)cclist[i].modbus_register[21]/10.0f;
			watts = cclist[i].modbus_register[18];
			kWh_today = (float)cclist[i].modbus_register[17]/10.0f;
			Ah_today = cclist[i].modbus_register[24];
			ext_temp = (float)cclist[i].modbus_register[31]/10.0f;
			int_fet_temp = (float)cclist[i].modbus_register[32]/10.0f;
			int_pcb_temp = (float)cclist[i].modbus_register[33]/10.0f;
			life_kWh =    (float)((unsigned long)cclist[i].modbus_register[25] + ((unsigned long)cclist[i].modbus_register[26]<<16))/10.0f;
			life_Ah = (unsigned long)cclist[i].modbus_register[27] + ((unsigned long)cclist[i].modbus_register[28]<<16);
			float_seconds_today = cclist[i].modbus_register[37];
			combochargestate = cclist[i].modbus_register[19];
			if (human_output) {
				printf("%s (%d): \n",cclist[i].ip,cclist[i].cid);
				printf(" --- Battery:   %.1f V @ %.1f A\n",battery_volts,battery_amps);
				printf(" --- PV:        %.1f V @ %.1f A (VoC: %.1f)\n",pv_volts,pv_amps,pv_voc);
				printf(" --- PV Power:  %u W (%.1f kWh / %u Ah today)\n",watts,kWh_today,Ah_today);
				if (temps_F) {
					int_pcb_temp = (int_pcb_temp * (9.0f/5.0f)) + 32;
					int_fet_temp = (int_fet_temp * (9.0f/5.0f)) + 32;
					ext_temp = (ext_temp * (9.0f/5.0f)) + 32;
					tchar = 'F';
				}
				printf(" --- Temps:     %.1f %c (PCB) / %.1f %c (FET) / %.1f %c (EXT)\n\n",int_pcb_temp,tchar,int_fet_temp,tchar,ext_temp,tchar);
			} else {
				printf("%d,'%s',%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%u,%.1f,%u,%.1f,%.1f,%.1f,%.1f,%u,%u,%u\n",cclist[i].cid,cclist[i].ip,battery_volts, pv_volts, battery_volts_raw, pv_volts_raw, battery_amps, pv_amps, pv_voc, watts, kWh_today, Ah_today, ext_temp, int_fet_temp, int_pcb_temp, life_kWh, life_Ah, float_seconds_today, combochargestate);
			}
		}
	}


	if (!j) {
		printf("0,No Hosts Connected out of %d\n",cc_count);
	}

}

void write_to_db(PGconn *conn) {

	int i,f=0;

	float battery_volts, pv_volts, battery_volts_raw, pv_volts_raw, battery_amps, pv_amps, pv_voc, kWh_today, ext_temp, int_fet_temp, int_pcb_temp, life_kWh;
	unsigned int watts, Ah_today, life_Ah, float_seconds_today, combochargestate;
	PGresult *res;


	char sql[50000];
	char sql2[5000];

	strcpy(sql,"insert into charge_controller_data (cid, battery_volts, pv_volts, battery_volts_raw, pv_volts_raw, battery_amps, pv_amps, pv_voc, watts, kWh_today, Ah_today, ext_temp, int_fet_temp, int_pcb_temp, life_kWh, life_Ah, float_seconds_today, combochargestate) VALUES ");

	for(i=0;i<cc_count;i++) {
		if (cclist[i].alive) {
			battery_volts = (float)cclist[i].modbus_register[14]/10.0f;
			pv_volts = (float)cclist[i].modbus_register[15]/10.0f;
			battery_volts_raw = (float)cclist[i].modbus_register[175]/10.0f;
			pv_volts_raw = (float)cclist[i].modbus_register[176]/10.0f;
			battery_amps = (float)cclist[i].modbus_register[16]/10.0f;
			pv_amps = (float)cclist[i].modbus_register[20]/10.0f;
			pv_voc = (float)cclist[i].modbus_register[21]/10.0f;
			watts = cclist[i].modbus_register[18];
			kWh_today = (float)cclist[i].modbus_register[17]/10.0f;
			Ah_today = cclist[i].modbus_register[24];
			ext_temp = (float)cclist[i].modbus_register[31]/10.0f;
			int_fet_temp = (float)cclist[i].modbus_register[32]/10.0f;
			int_pcb_temp = (float)cclist[i].modbus_register[33]/10.0f;
			life_kWh =    (float)((unsigned long)cclist[i].modbus_register[25] + ((unsigned long)cclist[i].modbus_register[26]<<16))/10.0f;
			life_Ah = (unsigned long)cclist[i].modbus_register[27] + ((unsigned long)cclist[i].modbus_register[28]<<16);
			float_seconds_today = cclist[i].modbus_register[37];
			combochargestate = cclist[i].modbus_register[19];


			if (f) {
				strcat(sql,", ");
			}
			f++;

			sprintf(sql2,"(%d, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %u, %.1f, %u, %.1f, %.1f, %.1f, %.1f, %u, %u, %u)",cclist[i].cid, battery_volts, pv_volts, battery_volts_raw, pv_volts_raw, battery_amps, pv_amps, pv_voc, watts, kWh_today, Ah_today, ext_temp, int_fet_temp, int_pcb_temp, life_kWh, life_Ah, float_seconds_today, combochargestate);
			strcat(sql,sql2);

		}
	}

	if (f) {
		if (debug) { printf(sql); printf("\n"); }
		res = psql_query(conn, sql);
		if (!res) {
			printf("Error doing SQL insert!\n");
		}
	}

}



void close_all() {
	int i, flags;

	for(i=0;i<cc_count;i++) {
		if (cclist[i].alive) {
			close(cclist[i].s);
			cclist[i].s = 0;
			cclist[i].alive = 0;
			if (debug) printf("closed %d\n",i);
		}
	}
}


int modbus_read_registers_command(int cc, int addr, int number, int offset) {


	unsigned char id[12] = { 0, 2, 0, 0, 0, 6, 255, 3, ((addr-1)>>8), ((addr-1)&0xff), 0, number };
	int i;

	if(i=write(cclist[cc].s,&id, 12) != 12) {
		if (debug) printf("couldn't send to %d, send returned %d\n",cc,i);
		cclist[cc].alive = 0;
		close(cclist[cc].s);
		return 0;
	}
	return 1;

}

int modbus_read_registers_finish_read(int cc, int addr, int number, int offset) {

	int len,n;
	unsigned char id[12];

	len = read(cclist[cc].s, id, 7);
	if (len != 7) {
		// problem reading...
		if (debug) printf("couldn't read from %d, read returned %d (a)\n",cc,len);
		cclist[cc].alive = 0;
		close(cclist[cc].s);
		return 0;
	}

	len = read(cclist[cc].s, id, 2);
	if (len != 2) {
		// problem reading...
		if (debug) printf("couldn't read from %d, read returned %d (b)\n",cc,len);
		cclist[cc].alive = 0;
		close(cclist[cc].s);
		return 0;
	}
	if ((id[0]) != 3 || (id[1]&0xff) != number<<1) {
		// problem reading...
		if (debug) printf("couldn't read from %d, modbus issue %d %d (c)\n",cc,id[0],id[1]);
		cclist[cc].alive = 0;
		close(cclist[cc].s);
		return 0;
	}
	n=id[1]&0xff;
	len=read(cclist[cc].s, &cclist[cc].modbus_register[offset], n);
	if (len != n) {
		// problem reading...
		if (debug) printf("couldn't read from %d, modbus issue %d %d (c)\n",cc,id[0],id[1]);
		cclist[cc].alive = 0;
		close(cclist[cc].s);
		return 0;
	}


	// fix endianness
	for(len=0;len<number;len++) {
		cclist[cc].modbus_register[offset+len] = ((cclist[cc].modbus_register[offset+len]>>8)&0xff) | ((cclist[cc].modbus_register[offset+len]<<8)&0xff00);
	}

	return 1;


}


void wait_for_write_ready() {

	fd_set fset;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int i,j,max_sd,c,status;

	// wait for write readyness
	for(j=0;j<30;j++) {
		FD_ZERO(&fset);
		max_sd = -1;
		c = 0;
		for(i=0;i<cc_count;i++) {
			if (cclist[i].alive) {
				FD_SET(cclist[i].s, &fset);
				if (cclist[i].s > max_sd)
					max_sd = cclist[i].s;
				c++;
			}
		}

		status = select(max_sd + 1, NULL, &fset, NULL, &timeout);
		if (debug) printf("select write ready: %d\n",status);
		if (status >= c) break;
		usleep(100000);
	}

}



void wait_for_data_ready() {
	fd_set fset;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int i,j,max_sd,c,status;
	// wait for data on all or timeout
	for(j=0;j<30;j++) {
		FD_ZERO(&fset);
		max_sd = -1;
		c = 0;
		for(i=0;i<cc_count;i++) {
			if (cclist[i].alive) {
				FD_SET(cclist[i].s, &fset);
				if (cclist[i].s > max_sd) 
					max_sd = cclist[i].s;
				c++;
			}
		}

		status = select(max_sd + 1, &fset, NULL, NULL, &timeout);
		if (debug) printf("select read ready: %d\n",status);
		if (status >= c) break;
		usleep(100000);
	}
}


void get_from_all(int offset, int count) {

	int i,c=0,j,ic=-1;

	wait_for_write_ready();

	c = 0;

	for (i=0;i<cc_count;i++) {
		if (cclist[i].alive) {
			c += modbus_read_registers_command(i, 4101+offset, count, offset);
		}
	}

	if (ic == -1) ic = c;

	wait_for_data_ready();

	c = 0;

	for (i=0;i<cc_count;i++) {
		if (cclist[i].alive) {
			c+=modbus_read_registers_finish_read(i, 4101+offset, count, offset);
		}
	}

	if (debug) printf("successful commands to %d out of initial %d\n",c,ic);

}

void gather_data_all() {

	int i,c=0,j,ic=-1,ox=0;
	unsigned int ctime;

	get_from_all(10, 30);
	get_from_all(170, 10);

	ctime = time(NULL);
	for(i=0;i<cc_count;i++) {
		if (cclist[i].alive) {
			cclist[i].last_success = ctime;
			if (debug) printf("%u\n",ctime);
		}
	}

}



void connect_all() {

	// connect to all charge controllers with a 3 second timeout
	int i, j, status, flags;
	int max_sd = -1;
	int connected = 0;
	fd_set fset;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int so_error=0, flen=0;
	socklen_t len = sizeof so_error;
	unsigned int ctime;

	ctime = time(NULL);

	FD_ZERO(&fset);

	for(i=0;i<cc_count;i++) {
		if (cclist[i].alive && (cclist[i].last_success < (ctime-90))) {
			cclist[i].alive = 0;
			close(cclist[i].s);
			if (debug) printf("No success from open socket for %d for > 90 seconds.\n",i);
		}


		if (!cclist[i].alive) {
			if (!cclist[i].hp) {
				if((cclist[i].hp = gethostbyname(cclist[i].ip)) == NULL) {
					printf("gethostbyname error on CC %d\n",i);
					exit(1);
				}
				bcopy((char *)cclist[i].hp->h_addr, (char *)&cclist[i].sa.sin_addr, cclist[i].hp->h_length);
				cclist[i].sa.sin_family = cclist[i].hp->h_addrtype;
				cclist[i].sa.sin_port=modbus_port>>8 | (modbus_port<<8 & 0xffff);
			}

			if((cclist[i].s = socket(cclist[i].hp->h_addrtype, SOCK_STREAM,0)) < 0) {
				printf("couldn't create socket for %d - ",i);
				perror("socket");
				exit(1);
			}

			if (cclist[i].s > max_sd) max_sd = cclist[i].s;

			flags = fcntl(cclist[i].s, F_GETFL, 0);
			flags |= O_NONBLOCK;
			fcntl(cclist[i].s, F_SETFL, flags);

			status = connect(cclist[i].s, (const struct sockaddr *)&cclist[i].sa, sizeof(cclist[i].sa));

			if (debug) printf("connect status: %d for socket %d\n",status,cclist[i].s);

			cclist[i].alive = 0;

			FD_SET(cclist[i].s, &fset);

		}
	}

	for(j=0;j<30;j++) {
		usleep(100000); // 100ms?
		status = select(max_sd + 1, NULL, &fset, NULL, &timeout);
		if (debug) printf("select status: %d\n",status);

		if (status >= cc_count) j=100000;

		FD_ZERO(&fset);
		for(i=0;i<cc_count;i++) FD_SET(cclist[i].s, &fset);
	}

	for(i=0;i<cc_count;i++) {
			cclist[i].alive = 1;
	}


}


int load_charge_controllers(PGconn *conn) {

	PGresult *res;
	int i,j;

	res = psql_query(conn, "select cid,ip,smallname from charge_controller_list order by cid asc");
	if (!res) {
		printf("Unable to list charge controllers.\n");
		return 0;
	}

	i = PQntuples(res);
	if (!i) {
		printf("Unable to list charge controllers, returned 0 rows\n");
		PQclear(res);
		return 0;
	}

	if (debug) printf("%d charge controllers in database\n",i);
	cc_count=i;

	if (cclist) free(cclist);

	cclist = (CC_DATA *) calloc(cc_count+1,sizeof(CC_DATA));

	if (!cclist) {
		printf("Error allocating memory for charge controller list\n");
		PQclear(res);
		return 0;
	}

	for(i=0;i<cc_count;i++) {

		strncpy(cclist[i].name, PQgetvalue(res, i, 2), 8);
		strncpy(cclist[i].ip, PQgetvalue(res, i, 1), 18);
		cclist[i].cid = atoi( PQgetvalue(res, i, 0));
		if (debug) printf("--- Loaded ID #%d - '%s' (%s)\n",cclist[i].cid,cclist[i].name,cclist[i].ip);

	}
	PQclear(res);
	return i;

}



PGresult* psql_query(PGconn *lconn, char *str) {

	PGresult *lres;

	if (PQstatus(lconn) != CONNECTION_OK) {
		fprintf(stderr,"psql_query: postgresql connection broken ...\n");
		return 0;
	}

	lres = PQexec(lconn, str);

	if ((PQresultStatus(lres) != PGRES_TUPLES_OK) && (PQresultStatus(lres) != PGRES_COMMAND_OK)) {
		fprintf(stderr,"psql_query: Could not execute query '%s': %s - %s (%d != %d)\n",str, PQerrorMessage(lconn), PQresultErrorMessage(lres), PQresultStatus(lres), PGRES_TUPLES_OK);
		PQclear(lres);
		return 0;
	}

	return lres;

}

