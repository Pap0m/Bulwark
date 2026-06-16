#include "db.h"
#include "error.h"
#include <sqlite3.h>
#include <stdio.h>

int init_database(const char *filename, sqlite3 **db) {
    int ret = sqlite3_open_v2(filename, db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    if (ret != SQLITE_OK) {
        sqlite3_close_v2(*db);
        return ERR_OPEN_DB;
    }
    return SUCCESS;
}

int setup_schema(sqlite3 *db) {
    char *err_msg = 0;

    // Enable WAL for better concurrency
    const char *pragma_sql = "PRAGMA journal_mode=WAL;";
    (void)sqlite3_exec(db, pragma_sql, 0, 0, &err_msg);

    // Create user table
    const char *user_schema_sql =
        "CREATE TABLE IF NOT EXISTS Users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "master_password TEXT NOT NULL, "
        "created_at INTEGER(4) NOT NULL DEFAULT (strftime('%s', 'now')), "
        "updated_at INTEGER(4) NOT NULL DEFAULT (strftime('%s', 'now')));";

    int ret = sqlite3_exec(db, user_schema_sql, 0, 0, &err_msg);

    if (ret != SQLITE_OK) {
        sqlite3_free(err_msg);
        return ERR_EXEC_SQL;
    }
    return SUCCESS;
}

int insert_user(sqlite3 *db, const char *username, const char *master_password) {
    int ret = 0;
    sqlite3_stmt *stmt = {0};

    const char *sql = "INSERT INTO Users (username, master_password) VALUES (?, ?);";

    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        return ERR_PREPARE_SQL;
    }
    ret = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return ERR_BIND_PARAMETER;
    }
    ret = sqlite3_bind_text(stmt, 2, master_password, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return ERR_BIND_PARAMETER;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return ERR_EXEC_SQL;
    }

    sqlite3_finalize(stmt);

    return SUCCESS;
}

int valid_user(sqlite3 *db, const char *username, const char *master_password) {
    int ret;
    sqlite3_stmt *stmt = NULL;

    const char *sql = "SELECT 1 FROM Users WHERE username = ? AND master_password = ?;";

    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "SQL ERROR: %s\n", sqlite3_errmsg(db));
        return ERR_PREPARE_SQL;
    }
    ret = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return ERR_BIND_PARAMETER;
    }
    ret = sqlite3_bind_text(stmt, 2, master_password, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        sqlite3_finalize(stmt);
        return ERR_BIND_PARAMETER;
    }

    int result = SUCCESS;
    ret = sqlite3_step(stmt);
    if (ret == SQLITE_ROW) {
        // done and user found
        result = SUCCESS;
    } else if (ret == SQLITE_DONE) {
        // done but user not found
        result = ERR_USER_NOT_FOUND;
    } else {
        // db errors
        switch (ret) {
            case SQLITE_BUSY: result = ERR_LOCK_DB; break;
            case SQLITE_CONSTRAINT: result = ERR_VIOLATE_CONSTRAINT; break;
            default: result = ERR_EXECUTE_QUERY; break;
        }
    }

    sqlite3_finalize(stmt);

    return result;
}

int get_user_by_id() {

}
