/*
* Copyright 2011 Kestrel Signal Processing, Inc.
* Copyright 2011, 2012 Range Networks, Inc.
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "SubscriberRegistry.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "sqlite3.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <Configuration.h>

extern ConfigurationTable gConfig;


using namespace std;

static const char* createRRLPTable = {
    "CREATE TABLE IF NOT EXISTS RRLP ("
		"id				INTEGER PRIMARY KEY, "
		"name           VARCHAR(80) not null, "
		"latitude       real not null, "
		"longitude      real not null, "
		"error          real not null, "
		"time           text not null "
    ")"
};

static const char* createDDTable = {
    "CREATE TABLE IF NOT EXISTS DIALDATA_TABLE ("
		"id				INTEGER PRIMARY KEY, "
		"exten           VARCHAR(40)     NOT NULL        DEFAULT '', "
		"dial			VARCHAR(128)    NOT NULL        DEFAULT '' "
    ")"
};

static const char* createRateTable = {
"create table if not exists rates (service varchar(30) not null, rate integer not null)"
};

static const char* createSBTable = {
    "CREATE TABLE IF NOT EXISTS SIP_BUDDIES ("
		"id                    integer primary key, "
		"name                  VARCHAR(80) not null, "
		"context               VARCHAR(80), "
		"callingpres           VARCHAR(30) DEFAULT 'allowed_not_screened', "
		"deny                  VARCHAR(95), "
		"permit                VARCHAR(95), "
		"secret                VARCHAR(80), "
		"md5secret             VARCHAR(80), "
		"remotesecret          VARCHAR(250), "
		"transport             VARCHAR(10), "
		"host                  VARCHAR(31) default '' not null, "
		"nat                   VARCHAR(5) DEFAULT 'no' not null, "
		"type                  VARCHAR(10) DEFAULT 'friend' not null, "
		"accountcode           VARCHAR(20), "
		"amaflags              VARCHAR(13), "
		"callgroup             VARCHAR(10), "
		"callerid              VARCHAR(80), "
		"defaultip             VARCHAR(40) DEFAULT '0.0.0.0', "
		"dtmfmode              VARCHAR(7) DEFAULT 'info', "
		"fromuser              VARCHAR(80), "
		"fromdomain            VARCHAR(80), "
		"insecure              VARCHAR(4), "
		"language              CHAR(2), "
		"mailbox               VARCHAR(50), "
		"pickupgroup           VARCHAR(10), "
		"qualify               CHAR(3), "
		"regexten              VARCHAR(80), "
		"rtptimeout            CHAR(3), "
		"rtpholdtimeout        CHAR(3), "
		"setvar                VARCHAR(100), "
		"disallow              VARCHAR(100) DEFAULT 'all', "
		"allow                 VARCHAR(100) DEFAULT 'gsm' not null, "
		"fullcontact           VARCHAR(80), "
		"ipaddr                VARCHAR(45), "
		"port                  int(5) DEFAULT 5062, "
		"username              VARCHAR(80), "
		"defaultuser           VARCHAR(80), "
		"subscribecontext      VARCHAR(80), "
		"directmedia           VARCHAR(3), "
		"trustrpid             VARCHAR(3), "
		"sendrpid              VARCHAR(3), "
		"progressinband        VARCHAR(5), "
		"promiscredir          VARCHAR(3), "
		"useclientcode         VARCHAR(3), "
		"callcounter           VARCHAR(3), "
		"busylevel             int(11) default 1, "
		"allowoverlap          VARCHAR(3) DEFAULT 'no', "
		"allowsubscribe        VARCHAR(3) DEFAULT 'no', "
		"allowtransfer         VARCHAR(3) DEFAULT 'no', "
		"ignoresdpversion      VARCHAR(3) DEFAULT 'no', "
		"template              VARCHAR(100), "
		"videosupport          VARCHAR(6) DEFAULT 'no', "
		"maxcallbitrate        int(11), "
		"rfc2833compensate     VARCHAR(3) DEFAULT 'yes', "
		"'session-timers'      VARCHAR(10) DEFAULT 'accept', "
		"'session-expires'     int(6) DEFAULT 1800, "
		"'session-minse'       int(6) DEFAULT 90, "
		"'session-refresher'   VARCHAR(3) DEFAULT 'uas', "
		"t38pt_usertpsource    VARCHAR(3), "
		"outboundproxy         VARCHAR(250), "
		"callbackextension     VARCHAR(250), "
		"registertrying        VARCHAR(3) DEFAULT 'yes', "
		"timert1               int(6) DEFAULT 500, "
		"timerb                int(9), "
		"qualifyfreq           int(6) DEFAULT 120, "
		"contactpermit         VARCHAR(250), "
		"contactdeny           VARCHAR(250), "
		"lastms                int(11) DEFAULT 0 not null, "
		"regserver             VARCHAR(100), "
		"regseconds            int(11) DEFAULT 0 not null, "
		"useragent             VARCHAR(100), "
		"cancallforward        CHAR(3) DEFAULT 'yes' not null, "
		"canreinvite           CHAR(3) DEFAULT 'no' not null, "
		"mask                  VARCHAR(95), "
		"musiconhold           VARCHAR(100), "
		"restrictcid           CHAR(3), "
		"calllimit             int(5) default 1, "
		"WhiteListFlag         timestamp not null default '0', "
		"WhiteListCode         varchar(8) not null default '0', "
		"rand                  varchar(33) default '', "
		"sres                  varchar(33) default '', "
		"ki                    varchar(33) default '', "
		"kc                    varchar(33) default '', "
		"prepaid               int(1) DEFAULT 0 not null, "	// flag to indicate prepaid customer
		"account_balance       int(9) default 0 not null, "	// current account, neg is debt, pos is credit
		"RRLPSupported         int(1) default 1 not null, "
  		"hardware              VARCHAR(20), "
		"regTime               INTEGER default 0 NOT NULL, " // Unix time of most recent registration
		"a3_a8                 varchar(45) default NULL"
    ")"
};


int SubscriberRegistry::init()
{
	string ldb = gConfig.getStr("SubscriberRegistry.db");
	size_t p = ldb.find_last_of('/');
	if (p == string::npos) {
		LOG(EMERG) << "SubscriberRegistry.db not in a directory?";
		mDB = NULL;
		return 1;
	}
	string dir = ldb.substr(0, p);
	struct stat buf;
	if (stat(dir.c_str(), &buf)) {
		LOG(EMERG) << dir << " does not exist";
		mDB = NULL;
		return 1;
	}
	mNumSQLTries=gConfig.getNum("Control.NumSQLTries"); 
	int rc = sqlite3_open(ldb.c_str(),&mDB);
	if (rc) {
		LOG(EMERG) << "Cannot open SubscriberRegistry database: " << ldb << " error: " << sqlite3_errmsg(mDB);
		sqlite3_close(mDB);
		mDB = NULL;
		return 1;
	}
	if (!sqlite3_command(mDB,createRRLPTable,mNumSQLTries)) {
		LOG(EMERG) << "Cannot create RRLP table";
		return 1;
	}
	if (!sqlite3_command(mDB,createDDTable,mNumSQLTries)) {
		LOG(EMERG) << "Cannot create DIALDATA_TABLE table";
		return 1;
	}
	if (!sqlite3_command(mDB,createRateTable,mNumSQLTries)) {
		LOG(EMERG) << "Cannot create rate table";
		return 1;
	}
	if (!sqlite3_command(mDB,createSBTable,mNumSQLTries)) {
		LOG(EMERG) << "Cannot create SIP_BUDDIES table";
		return 1;
	}
	// Set high-concurrency WAL mode.
	if (!sqlite3_command(mDB,enableWAL,mNumSQLTries)) {
		LOG(EMERG) << "Cannot enable WAL mode on database at " << ldb << ", error message: " << sqlite3_errmsg(mDB);
	}
	return 0;
}



SubscriberRegistry::~SubscriberRegistry()
{
	if (mDB) sqlite3_close(mDB);
}



SubscriberRegistry::Status SubscriberRegistry::sqlHttp(const char *stmt, char **resultptr)
{
	LOG(INFO) << stmt;
	HttpQuery qry("sql");
	qry.send("stmts", stmt);
	if (!qry.http(false)) return FAILURE;
	const char *res = qry.receive("res");
	if (!res) return FAILURE; 
	*resultptr = strdup(res);
	return SUCCESS;
}



SubscriberRegistry::Status SubscriberRegistry::sqlLocal(const char *query, char **resultptr)
{
	LOG(INFO) << query;

	if (!resultptr) {
		if (!sqlite3_command(db(), query, mNumSQLTries)) return FAILURE;
		return SUCCESS;
	}

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_statement(db(), &stmt, query, mNumSQLTries)) {
		LOG(ERR) << "sqlite3_prepare_statement problem with query \"" << query << "\"";
		return FAILURE;
	}
	int src = sqlite3_run_query(db(), stmt, mNumSQLTries);
	if (src != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return FAILURE;
	}
	char *column = (char*)sqlite3_column_text(stmt, 0);
	if (!column) {
		LOG(ERR) << "Subscriber registry returned a NULL column.";
		sqlite3_finalize(stmt);
		return FAILURE;
	}
	*resultptr = strdup(column);
	sqlite3_finalize(stmt);
	return SUCCESS;
}



char *SubscriberRegistry::sqlQuery(const char *unknownColumn, const char *table, const char *knownColumn, const char *knownValue)
{
	char *result = NULL;
	SubscriberRegistry::Status st;
	ostringstream os;
	os << "select " << unknownColumn << " from " << table << " where " << knownColumn << " = \"" << knownValue << "\"";
	// try to find locally
	st = sqlLocal(os.str().c_str(), &result);
	if ((st == SUCCESS) && result) {
		// got it.  return it.
		LOG(INFO) << "result = " << result;
		return result;
	}
	// didn't find locally, so try over http
	st = sqlHttp(os.str().c_str(), &result);
	if ((st == SUCCESS) && result) {
		// found over http but not locally, so cache locally
		ostringstream os2;
		os2 << "insert into " << table << "(" << knownColumn << ", " << unknownColumn << ") values (\"" <<
			knownValue << "\",\"" << result << "\")";
		// ignore whether it succeeds
		sqlLocal(os2.str().c_str(), NULL);
		LOG(INFO) << "result = " << result;
		return result;
	}
	// didn't find locally or over http
	LOG(INFO) << "not found: " << os.str();
	return NULL;
}



SubscriberRegistry::Status SubscriberRegistry::sqlUpdate(const char *stmt)
{
	LOG(INFO) << stmt;
	SubscriberRegistry::Status st = sqlLocal(stmt, NULL);
	sqlHttp(stmt, NULL);
	// status of local is only important one because asterisk talks to that db directly
	// must update local no matter what
	return st;
}

string SubscriberRegistry::imsiGet(string imsi, string key)
{
	string name = imsi.substr(0,4) == "IMSI" ? imsi : "IMSI" + imsi;
	char *st = sqlQuery(key.c_str(), "sip_buddies", "name", imsi.c_str());
	if (!st) {
		LOG(WARNING) << "cannot get key " << key << " for username " << imsi;
		return "";
	}
	return st;
}

bool SubscriberRegistry::imsiSet(string imsi, string key, string value)
{
	string name = imsi.substr(0,4) == "IMSI" ? imsi : "IMSI" + imsi;
	ostringstream os;
	os << "update sip_buddies set " << key << " = \"" << value << "\" where name = \"" << name << "\"";
	return sqlUpdate(os.str().c_str()) == FAILURE;
}

char *SubscriberRegistry::getIMSI(const char *ISDN)
{
	if (!ISDN) {
		LOG(WARNING) << "SubscriberRegistry::getIMSI attempting lookup of NULL ISDN";
		return NULL;
	}
	LOG(INFO) << "getIMSI(" << ISDN << ")";
	return sqlQuery("dial", "dialdata_table", "exten", ISDN);
}



char *SubscriberRegistry::getCLIDLocal(const char* IMSI)
{
	if (!IMSI) {
		LOG(WARNING) << "SubscriberRegistry::getCLIDLocal attempting lookup of NULL IMSI";
		return NULL;
	}
	LOG(INFO) << "getCLIDLocal(" << IMSI << ")";
	return sqlQuery("callerid", "sip_buddies", "username", IMSI);
}



char *SubscriberRegistry::getCLIDGlobal(const char* IMSI)
{
	if (!IMSI) {
		LOG(WARNING) << "SubscriberRegistry::getCLIDGlobal attempting lookup of NULL IMSI";
		return NULL;
	}
	LOG(INFO) << "getCLIDGlobal(" << IMSI << ")";
	return sqlQuery("callerid", "sip_buddies", "username", IMSI);
}



char *SubscriberRegistry::getRegistrationIP(const char* IMSI)
{
	if (!IMSI) {
		LOG(WARNING) << "SubscriberRegistry::getRegistrationIP attempting lookup of NULL IMSI";
		return NULL;
	}
	LOG(INFO) << "getRegistrationIP(" << IMSI << ")";
	return sqlQuery("ipaddr", "sip_buddies", "username", IMSI);
}



SubscriberRegistry::Status SubscriberRegistry::setRegTime(const char* IMSI)
{
	if (!IMSI) {
		LOG(WARNING) << "SubscriberRegistry::setRegTime attempting set for NULL IMSI";
		return FAILURE;
	}
	unsigned now = (unsigned)time(NULL);
	ostringstream os;
	os << "update sip_buddies set regTime = " << now  << " where username = " << '"' << IMSI << '"';
	return sqlUpdate(os.str().c_str());
}



SubscriberRegistry::Status SubscriberRegistry::addUser(const char* IMSI, const char* CLID)
{
	if (!IMSI) {
		LOG(WARNING) << "SubscriberRegistry::addUser attempting add of NULL IMSI";
		return FAILURE;
	}
	if (!CLID) {
		LOG(WARNING) << "SubscriberRegistry::addUser attempting add of NULL CLID";
		return FAILURE;
	}
	LOG(INFO) << "addUser(" << IMSI << "," << CLID << ")";
	ostringstream os;
	os << "insert into sip_buddies (name, username, type, context, host, callerid, canreinvite, allow, dtmfmode, ipaddr, port) values (";
	os << "\"" << IMSI << "\"";
	os << ",";
	os << "\"" << IMSI << "\"";
	os << ",";
	os << "\"" << "friend" << "\"";
	os << ",";
	os << "\"" << "phones" << "\"";
	os << ",";
	os << "\"" << "dynamic" << "\"";
	os << ",";
	os << "\"" << CLID << "\"";
	os << ",";
	os << "\"" << "no" << "\"";
	os << ",";
	os << "\"" << "gsm" << "\"";
	os << ",";
	os << "\"" << "info" << "\"";
	os << ",";
	os << "\"" << "127.0.0.1" << "\"";
	os << ",";
	os << "\"" << "5062" << "\"";
	os << ")";
	os << ";";
	SubscriberRegistry::Status st = sqlUpdate(os.str().c_str());
	ostringstream os2;
	os2 << "insert into dialdata_table (exten, dial) values (";
	os2 << "\"" << CLID << "\"";
	os2 << ",";
	os2 << "\"" << IMSI << "\"";
	os2 << ")";
	SubscriberRegistry::Status st2 = sqlUpdate(os2.str().c_str());
	return st == SUCCESS && st2 == SUCCESS ? SUCCESS : FAILURE;
}


// For handover.  Only remove the local cache.  BS2 will have updated the global.
SubscriberRegistry::Status SubscriberRegistry::removeUser(const char* IMSI)
{
	if (!IMSI) {
		LOG(WARNING) << "SubscriberRegistry::addUser attempting add of NULL IMSI";
		return FAILURE;
	}
	LOG(INFO) << "removeUser(" << IMSI << ")";
	string server = gConfig.getStr("SubscriberRegistry.UpstreamServer");
	if (server.length() == 0) {
		LOG(INFO) << "not removing user if no upstream server";
		return FAILURE;
	}
	ostringstream os;
	os << "delete from sip_buddies where name = ";
	os << "\"" << IMSI << "\"";
	os << ";";
	LOG(INFO) << os.str();
	SubscriberRegistry::Status st = sqlLocal(os.str().c_str(), NULL);
	ostringstream os2;
	os2 << "delete from dialdata_table where dial = ";
	os2 << "\"" << IMSI << "\"";
	LOG(INFO) << os2.str();
	SubscriberRegistry::Status st2 = sqlLocal(os2.str().c_str(), NULL);
	return st == SUCCESS && st2 == SUCCESS ? SUCCESS : FAILURE;
}



char *SubscriberRegistry::mapCLIDGlobal(const char *local)
{
	if (!local) {
		LOG(WARNING) << "SubscriberRegistry::mapCLIDGlobal attempting lookup of NULL local";
		return NULL;
	}
	LOG(INFO) << "mapCLIDGlobal(" << local << ")";
	char *IMSI = getIMSI(local);
	if (!IMSI) return NULL;
	char *global = getCLIDGlobal(IMSI);
	free(IMSI);
	return global;
}

SubscriberRegistry::Status SubscriberRegistry::RRLPUpdate(string name, string lat, string lon, string err){
	ostringstream os;
	os << "insert into RRLP (name, latitude, longitude, error, time) values (" <<
	  '"' << name << '"' << "," <<
	  lat << "," <<
	  lon << "," <<
	  err << "," <<
	  "datetime('now')"
	  ")";
	LOG(INFO) << os.str();
	return sqlUpdate(os.str().c_str());
}



string SubscriberRegistry::getRandForAuthentication(bool sip, string IMSI)
{
	if (IMSI.length() == 0) {
		LOG(WARNING) << "SubscriberRegistry::getRandForAuthentication attempting lookup of NULL IMSI";
		return "";
	}
	LOG(INFO) << "getRandForAuthentication(" << IMSI << ")";
	// get rand from SR server
	HttpQuery qry("rand");
	qry.send("imsi", IMSI);
	qry.log();
	if (!qry.http(sip)) return "";
	return qry.receive("rand");
}

bool SubscriberRegistry::getRandForAuthentication(bool sip, string IMSI, uint64_t *hRAND, uint64_t *lRAND)
{
	string strRAND = getRandForAuthentication(sip, IMSI);
	if (strRAND.length() == 0) {
		*hRAND = 0;
		*lRAND = 0;
		return false;
	}
	stringToUint(strRAND, hRAND, lRAND);
	return true;
}

void SubscriberRegistry::stringToUint(string strRAND, uint64_t *hRAND, uint64_t *lRAND)
{
	assert(strRAND.size() == 32);
	string strhRAND = strRAND.substr(0, 16);
	string strlRAND = strRAND.substr(16, 16);
	stringstream ssh;
	ssh << hex << strhRAND;
	ssh >> *hRAND;
	stringstream ssl;
	ssl << hex << strlRAND;
	ssl >> *lRAND;
}

string SubscriberRegistry::uintToString(uint64_t h, uint64_t l)
{
	ostringstream os1;
	os1.width(16);
	os1.fill('0');
	os1 << hex << h;
	ostringstream os2;
	os2.width(16);
	os2.fill('0');
	os2 << hex << l;
	ostringstream os3;
	os3 << os1.str() << os2.str();
	return os3.str();
}

string SubscriberRegistry::uintToString(uint32_t x)
{
	ostringstream os;
	os.width(8);
	os.fill('0');
	os << hex << x;
	return os.str();
}

SubscriberRegistry::Status SubscriberRegistry::authenticate(bool sip, string IMSI, uint64_t hRAND, uint64_t lRAND, uint32_t SRES)
{
	string strRAND = uintToString(hRAND, lRAND);
	string strSRES = uintToString(SRES);
	return authenticate(sip, IMSI, strRAND, strSRES);
}


SubscriberRegistry::Status SubscriberRegistry::authenticate(bool sip, string IMSI, string rand, string sres)
{
	if (IMSI.length() == 0) {
		LOG(WARNING) << "SubscriberRegistry::authenticate attempting lookup of NULL IMSI";
		return FAILURE;
	}
	LOG(INFO) << "authenticate(" << IMSI << "," << rand << "," << sres << ")";
	HttpQuery qry("auth");
	qry.send("imsi", IMSI);
	qry.send("rand", rand);
	qry.send("sres", sres);
	qry.log();
	if (!qry.http(sip)) return FAILURE;
	const char *status = qry.receive("status");
	if (strcmp(status, "SUCCESS") == 0) return SUCCESS;
	if (strcmp(status, "FAILURE") == 0) return FAILURE;
	// status is Kc
	return SUCCESS;
}



bool SubscriberRegistry::useGateway(const char* ISDN)
{
	// FIXME -- Do something more general in Asterisk.
	// This is a hack for Burning Man.
	int cmp = strncmp(ISDN,"88351000125",11);
	return cmp!=0;
}



SubscriberRegistry::Status SubscriberRegistry::setPrepaid(const char *IMSI, bool yes)
{
	ostringstream os;
	os << "update sip_buddies set prepaid = " << (yes ? 1 : 0)  << " where username = " << '"' << IMSI << '"';
	return sqlUpdate(os.str().c_str());
}


SubscriberRegistry::Status SubscriberRegistry::isPrepaid(const char *IMSI, bool &yes)
{
	char *st = sqlQuery("prepaid", "sip_buddies", "username", IMSI);
	if (!st) {
		LOG(NOTICE) << "cannot get prepaid status for username " << IMSI;
		return FAILURE;
	}
	yes = *st == '1';
	free(st);
	return SUCCESS;
}


SubscriberRegistry::Status SubscriberRegistry::balanceRemaining(const char *IMSI, int &balance)
{
	char *st = sqlQuery("account_balance", "sip_buddies", "username", IMSI);
	if (!st) {
		LOG(NOTICE) << "cannot get balance for " << IMSI;
		return FAILURE;
	}
	balance = (int)strtol(st, (char **)NULL, 10);
	free(st);
	return SUCCESS;
}


SubscriberRegistry::Status SubscriberRegistry::addMoney(const char *IMSI, int moneyToAdd)
{
	ostringstream os;
	os << "update sip_buddies set account_balance = account_balance + " << moneyToAdd << " where username = " << '"' << IMSI << '"';
	if (sqlUpdate(os.str().c_str()) == FAILURE) {
		LOG(NOTICE) << "cannot update rate for username " << IMSI;
		return FAILURE;
	}
	return SUCCESS;
}

int SubscriberRegistry::serviceCost(const char* service)
{
	char *rateSt = sqlQuery("rate", "rates", "service", service);
	if (!rateSt) {
		LOG(ALERT) << "cannot get rate for service " << service;
		return -1;
	}
	int rate = (int)strtol(rateSt, (char **)NULL, 10);
	free(rateSt);
	return rate;
}

SubscriberRegistry::Status SubscriberRegistry::serviceUnits(const char *IMSI, const char* service, int &units)
{
	int balance;
	Status stat = balanceRemaining(IMSI,balance);
	if (stat == FAILURE) return FAILURE;
	int rate = serviceCost(service);
	if (rate<0) return FAILURE;
	units = balance / rate;
	return SUCCESS;
}



HttpQuery::HttpQuery(const char *req)
{
	sends = map<string,string>();
	sends["req"] = req;
	receives = map<string,string>();
}

void HttpQuery::send(const char *label, string value)
{
	sends[label] = value;
}

void HttpQuery::log()
{
	ostringstream os;
	bool first = true;
	for (map<string,string>::iterator it = sends.begin(); it != sends.end(); it++) {
		if (first) {
			first = false;
		} else {
			os << "&";
		}
		os << it->first << "=" << it->second;
	}
	LOG(INFO) << os.str();
}

bool HttpQuery::http(bool sip)
{
	// unique temporary file names
	ostringstream os1;
	ostringstream os2;
	os1 << "/tmp/subscriberregistry.1." << getpid();
	os2 << "/tmp/subscriberregistry.2." << getpid();
	string tmpFile1 = os1.str();
	string tmpFile2 = os2.str();

	// write the request and params to temp file
	ofstream file1(tmpFile1.c_str());
	if (file1.fail()) {
		LOG(ERR) << "HttpQuery::http: can't write " << tmpFile1.c_str();
		return false;
	}
	bool first = true;
	for (map<string,string>::iterator it = sends.begin(); it != sends.end(); it++) {
		if (first) {
			first = false;
		} else {
			file1 << "&";
		}
		file1 << it->first << "=" << it->second;
	}
	file1.close();

	// call the server
	string server = sip ? 
		gConfig.getStr("SIP.Proxy.Registration"):
		gConfig.getStr("SubscriberRegistry.UpstreamServer");
	if (server.length() == 0 && !sip) return false;
	ostringstream os;
	os << "curl -s --data-binary @" << tmpFile1.c_str() << " " << server << " > " << tmpFile2.c_str();
	LOG(INFO) << os.str();
	if (server == "testing") {
		return false;
	} else {
		int st = system(os.str().c_str());
		if (st != 0) {
			LOG(ERR) << "curl call returned " << st;
			return false;
		}
	}

	// read the http return from another temp file
	ifstream file2(tmpFile2.c_str());
	if (file2.fail()) {
		LOG(ERR) << "HTTPQuery::http: can't read " << tmpFile2.c_str();
		return false;
	}
	string tmp;
	while (getline(file2, tmp)) {
		size_t pos = tmp.find('=');
		if (pos != string::npos) {
			string key = tmp.substr(0, pos);
			string value = tmp.substr(pos+1);
			if (key == "error") {
				LOG(ERR) << "HTTPQuery::http error: " << value;
				file2.close();
				return false;
			}
			receives[key] = value;
		} else {
			file2.close();
			LOG(ERR) << "HTTPQuery::http: bad server return:";
			ifstream file22(tmpFile2.c_str());
			while (getline(file22, tmp)) {
				LOG(ERR) << tmp;
			}
			file22.close();
			return false;
		}
	}
	file2.close();
	return true;
}

const char *HttpQuery::receive(const char *label)
{
	map<string,string>::iterator it = receives.find(label);
	return it == receives.end() ? NULL : it->second.c_str();
}








// vim: ts=4 sw=4









