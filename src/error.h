#pragma once

#ifndef ERROR_H_
#define ERROR_H_

typedef enum {
    // Success
    SUCCESS = 0,
    // DB Errors
    ERR_OPEN_DB = -10,
    ERR_EXEC_SQL = -11,
    ERR_PREPARE_SQL = -12,
    ERR_EXECUTE_QUERY = -13,
    ERR_BIND_PARAMETER = -14,
    ERR_LOCK_DB = -15,
    ERR_VIOLATE_CONSTRAINT = -16,

    ERR_USER_NOT_FOUND = -30,

   // Mongoose Errors
} Error;

#endif // ERROR_H_
