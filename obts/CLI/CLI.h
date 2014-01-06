/*
* Copyright 2009 Free Software Foundation, Inc.
*
* This software is distributed under multiple licenses; see the COPYING file in the main directory for licensing information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/


#ifndef OPENBTSCLI_H
#define OPENBTSCLI_H

#include <string>
#include <map>
#include <iostream>


namespace CommandLine {


/** A table for matching strings to actions. */
typedef std::map<std::string,int (*)(int,char**,std::ostream&)> ParseTable;

/** The help table. */
typedef std::map<std::string,std::string> HelpTable;

class Parser {

	private:

	ParseTable mParseTable;
	HelpTable mHelpTable;
	static const int mMaxArgs = 10;

	public:

	void addCommands();

	/**
		Process a command line.
		@return 0 on sucess, -1 on exit request, error codes otherwise
	*/
	int process(const char* line, std::ostream& os) const;

	/** Add a command to the parsing table. */
	void addCommand(const char* name, int (*func)(int,char**,std::ostream&), const char* helpString)
		{ mParseTable[name] = func; mHelpTable[name]=helpString; }

	ParseTable::const_iterator begin() const { return mParseTable.begin(); }
	ParseTable::const_iterator end() const { return mParseTable.end(); }

	/** Return a help string. */
	const char *help(const std::string& cmd) const;

	private:

	/**
		Parse and execute a command string.
		@line a writeable copy of the original line
		@cline the original line
		@os output stream
		@return status code
	*/
	int execute(char* line, std::ostream& os) const;

};



} 	// CLI



#endif
// vim: ts=4 sw=4
