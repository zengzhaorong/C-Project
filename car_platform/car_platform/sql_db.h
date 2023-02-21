#ifndef SQL_DB_H
#define SQL_DB_H

#include <QSqlDatabase>

#define CAR_DB_NAME     "./car_data.cb"
#define TABLE_CAR_DATA  "car_table"
#define TABLE_HISTORY   "history_table"

#define DB_COL_ID           "id"
#define DB_COL_LICENSE      "license"

#define DB_COL_TIME         "time"
#define DB_COL_LICENSE      "license"

#define LICENSE_NUM_LEN     32

typedef struct {
    int id;
    char license[LICENSE_NUM_LEN];
}car_info_t;

typedef struct {
    int time;
    char license[LICENSE_NUM_LEN];
}history_info_t;

int sql_car_add(QSqlDatabase &sql, car_info_t *car_info);
int sql_car_del(QSqlDatabase &sql, char *license);
int sql_car_traverse(QSqlDatabase &sql, int *cursor, car_info_t *car_info);
int sql_car_get_total(QSqlDatabase &sql);

int sql_history_add(QSqlDatabase &sql, history_info_t *hist_info);

int sql_db_init(QSqlDatabase &sql);


#endif // SQL_DB_H
