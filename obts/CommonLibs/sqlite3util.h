#ifndef SQLITE3UTIL_H
#define SQLITE3UTIL_H

#include <sqlite3.h>

// (pat) Dont put statics in .h files - they generate a zillion g++ error messages.
extern const char *enableWAL;
//static const char* enableWAL = {
//	"PRAGMA journal_mode=WAL"
//};

int sqlite3_prepare_statement(sqlite3* DB, sqlite3_stmt **stmt, const char* query, unsigned retries = 5);

int sqlite3_run_query(sqlite3* DB, sqlite3_stmt *stmt, unsigned retries = 5);

bool sqlite3_single_lookup(sqlite3* DB, const char *tableName,
		const char* keyName, const char* keyData,
		const char* valueName, unsigned &valueData, unsigned retries = 5);

bool sqlite3_single_lookup(sqlite3* DB, const char* tableName,
		const char* keyName, const char* keyData,
		const char* valueName, char* &valueData, unsigned retries = 5);

// This function returns an allocated string that must be free'd by the caller.
bool sqlite3_single_lookup(sqlite3* DB, const char* tableName,
		const char* keyName, unsigned keyData,
		const char* valueName, char* &valueData, unsigned retries = 5);

bool sqlite3_exists(sqlite3* DB, const char* tableName,
		const char* keyName, const char* keyData, unsigned retries = 5);

/** Run a query, ignoring the result; return true on success. */
bool sqlite3_command(sqlite3* DB, const char* query, unsigned retries = 5);

#endif
