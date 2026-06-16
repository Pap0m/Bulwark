#pragma once

#ifndef DB_H_
#define DB_H_

#include <sqlite3.h>
#include <stdbool.h>

int init_database(const char *filename, sqlite3 **db);

int setup_schema(sqlite3 *db);

int insert_user(sqlite3 *db, const char *username, const char *master_password);

int valid_user(sqlite3 *db, const char *username, const char *master_password);

int get_user_by_id();

#endif // DB_H_
