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

#ifndef SubscriberRegistry_H
#define SubscriberRegistry_H

#include <map>
#include <stdlib.h>
#include <Logger.h>
// #include <Timeval.h>
// #include <Threads.h>
#include <map>
#include <string>
#include "sqlite3.h"

using namespace std;

class SubscriberRegistry {

	private:

	sqlite3 *mDB;			///< database connection
	unsigned mNumSQLTries;		///< Number of times to try an sqlite command before giving up.

	public:

	~SubscriberRegistry();

	/**
			Initialize the subscriber registry using parameters from gConfig.
			@return 0 if the database was successfully opened and initialized; 1 otherwise
	*/
	int init();

	typedef enum {
		SUCCESS=0,		///< operation successful
		FAILURE=1,		///< operation not successful
		DELAYED=2,		///< operation successful, but effect delayed
		TRYAGAIN=3		///< operation not attempted, try again later
	} Status;


	sqlite3 *db()
	{
		return mDB;
	}



	/**
		Resolve an ISDN or other numeric address to an IMSI.
		@param ISDN Any numeric address, E.164, local extension, etc.
		@return A C-string to be freed by the caller,
			NULL if the ISDN cannot be resolved.
	*/
	char* getIMSI(const char* ISDN);

	/**
		Given an IMSI, return the local CLID, which should be a numeric address.
		@param IMSI The subscriber IMSI.
		@return A C-string to be freed by the caller,
			NULL if the IMSI isn't found.
	*/
	char* getCLIDLocal(const char* IMSI);

	/**
		Given an IMSI, return the global CLID, which should be a numeric address.
		@param IMSI The subscriber IMSI.
		@return A C-string to be freed by the caller,
			NULL if the IMSI isn't found.
	*/
	char* getCLIDGlobal(const char* IMSI);

	/**
		Given an IMSI, return the IP address of the most recent registration.
		@param IMSI The subscriber IMSI
		@return A C-string to be freed by the caller, "111.222.333.444:port",
			NULL if the ISMI isn't registered.
	*/
	char* getRegistrationIP(const char* IMSI);

	/**
		Set a specific variable indexed by imsi from sip_buddies
		@param imsi The user's IMSI or SIP username.
		@param key to index into table
	*/
	string imsiGet(string imsi, string key);

	/**
		Set a specific variable indexed by imsi_from sip_buddies
		@param imsi The user's IMSI or SIP username.
		@param key to index into table
		@param value to set indexed by the key
	*/
	bool imsiSet(string imsi, string key, string value);

	/**
		Add a new user to the SubscriberRegistry.
		@param IMSI The user's IMSI or SIP username.
		@param CLID The user's local CLID.
	*/
	Status addUser(const char* IMSI, const char* CLID);


	/**
		Remove a user from the SubscriberRegistry.
		@param IMSI The user's IMSI or SIP username.
	*/
	Status removeUser(const char* IMSI);


	/**
		Set the current time as the time of the most recent registration for an IMSI.
		@param IMSI The user's IMSI or SIP username.
	*/
	Status setRegTime(const char* IMSI);


	char *mapCLIDGlobal(const char *local);



	/**
		Get a 128-bit number for authentication.
		@param sip sip server (true) or http
		@param IMSI The user's IMSI or SIP username.
		@return the 128-bit number in hex
	*/
	string getRandForAuthentication(bool sip, string IMSI);


	/**
		Get a 128-bit number for authentication.
		@param sip sip server (true) or http
		@param IMSI The user's IMSI or SIP username;
		@param hRAND upper 64 bits
		@param lRAND lower 64 bits
	*/
	bool getRandForAuthentication(bool sip, string IMSI, uint64_t *hRAND, uint64_t *lRAND);

	void stringToUint(string strRAND, uint64_t *hRAND, uint64_t *lRAND);

	string uintToString(uint64_t h, uint64_t l);

	string uintToString(uint32_t x);

	SubscriberRegistry::Status authenticate(bool sip, string IMSI, uint64_t hRAND, uint64_t lRAND, uint32_t SRES);



	/**
		Authenticate a handset.
		@param sip sip server (true) or http
		@param IMSI The user's IMSI or SIP username.
		@param rand RAND.
		@param sres SRES
		@return ok or fail
	*/
	SubscriberRegistry::Status authenticate(bool sip, string IMSI, string rand, string sres);



	bool useGateway(const char* ISDN);


	/**
		Set whether a subscriber is prepaid.
		@param IMSI Subscriber's IMSI
		@param yes true for prepaid, false for postpaid
		@return SUCCESS or FAILURE
	*/
	Status setPrepaid(const char *IMSI, bool yes);


	/**
		Is a subscriber postpaid?
		@param IMSI Subscriber's IMSI
		@param yes set to true if subscriber is postpaid, false if prepaid
		@return SUCCESS or FAILURE
	*/
	Status isPrepaid(const char *IMSI, bool &yes);


	/**
		Get the balance remaining in a subscriber's account.
		@param IMSI Subscriber's IMSI
		@param balance current account balance
		@return SUCCESS or FAILURE
	*/
	Status balanceRemaining(const char *IMSI, int &balance);


	/**
		Atomic operation to add to the account balance
		@param IMSI subscriber's IMSI
		@param moneyToAdd money to add (negative to subtract)
		@return SUCCESS or FAILURE
	*/
	Status addMoney(const char *IMSI, int moneyToAdd);

	/**
		Return the number of "units" of a particular transcation type available in the user's account.
		@param IMSI subscriber's IMSI
		@param service the service type
		@param units number of units of this service type remaining
		@return SUCCESS or FAILURE
	*/
	Status serviceUnits(const char *IMSI, const char* service, int &units);

	/**
		Return the cost per unit of a specific service type.
	*/
	int serviceCost(const char* service);

	/**
		Update the RRLP location for user
		@param name IMSI to be updated
		@param lat Latitude
		@param lon Longitude
		@param err Approximate Error
	*/
	Status RRLPUpdate(string name, string lat, string lon, string err);


	private:



	/**
		Run sql statments locally.
		@param stmt The sql statements.
		@param resultptr Set this to point to the result of executing the statements.
	*/
	Status sqlLocal(const char *stmt, char **resultptr);



	/**
		Run sql statments over http.
		@param stmt The sql statements.
		@param resultptr Set this to point to the result of executing the statements.
	*/
	Status sqlHttp(const char *stmt, char **resultptr);



	/**
		Run an sql query (select unknownColumn from table where knownColumn = knownValue).
		@param unknownColumn The column whose value you want.
		@param table The table to look in.
		@param knownColumn The column with the value you know.
		@param knownValue The known value of knownColumn.
	*/
	char *sqlQuery(const char *unknownColumn, const char *table, const char *knownColumn, const char *knownValue);



	/**
		Run an sql update.
		@param stmt The update statement.
	*/
	Status sqlUpdate(const char *stmt);








};



/** Class that SubscriberRegistry uses to setup an http query, run it, and get the results. */
class HttpQuery {



	public:



	/**
		Constructor.
		@param req The type of http query (sql, (get a)rand(om number) auth(enticate), etc).
	*/
	HttpQuery(const char *req);



	/**
		Specify a parameter to send in the http query.
		@param label The label or name for the parameter.
		@param value The value of the parameter.
	*/
	void send(const char *label, string value);



	/**
		Log the query.
		*/
	void log();


	/**
		This runs the http query.
		@param sip Whether to call the sip server as opposed to the http server.
	*/
	bool http(bool sip);



	/**
		Get result from the http query.
		@param label The label or name of the parameter whose value you want.
	*/
	const char *receive(const char *label);





	private:



	/** stores the parameters to send. */
	map<string,string> sends;



	/** stores the return parameters. */
	map<string,string> receives;



};



#endif

// vim: ts=4 sw=4
