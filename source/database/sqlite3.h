#pragma once

/*
 * CS 499 Computer Science Capstone
 * Milestone Four: Enhancement Three - Databases
 * Student: Brianna Reed
 *
 * SQLite compatibility header for the Visual Studio build
 *
 * The project uses WinSQLite supplied by the Windows SDK
 * WinSQLite exposes the SQLite C API used by the Milestone Four
 * database enhancement
 */

#include <winsqlite/winsqlite3.h>

 /*
  * Automatically link the Windows SQLite library when the
  * project is compiled with Microsoft Visual C++
  */
#pragma comment(lib, "winsqlite3.lib")