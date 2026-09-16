/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "fssh.h"

#include "command_checkfs.h"
#include "command_resizefs.h"


namespace FSShell {

fssh_status_t command_clonefs(int argc, const char* const* argv);


void
register_additional_commands()
{
	CommandManager::Default()->AddCommand(command_clonefs, "clonefs",
		"clone and verify a read-only BFS image into an empty volume");
	CommandManager::Default()->AddCommand(command_checkfs, "checkfs",
		"check file system");
	CommandManager::Default()->AddCommand(command_resizefs, "resizefs",
		"resize file system");
}


}	// namespace FSShell
