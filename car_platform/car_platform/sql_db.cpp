#include "sql_db.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlQuery>

static QSqlQuery* sqlquery;


int sql_history_add(QSqlDatabase &sql, history_info_t *hist_info)
{
    QString sql_cmd;

    sql_cmd = QString("insert into %1(%2,%3) values(%4,'%5');").arg(TABLE_HISTORY).arg(DB_COL_TIME).arg(DB_COL_LICENSE).arg(hist_info->time).arg(hist_info->license);
    qDebug() << sql_cmd;
    if(sqlquery->exec(sql_cmd))
        qDebug() << "add history ok." ;
    else
        qDebug() << "add history failed!" ;

    return 0;
}

int sql_car_add(QSqlDatabase &sql, car_info_t *car_info)
{
    QString sql_cmd;

    sql_cmd = QString("insert into %1(%2,%3) values(%4,'%5');").arg(TABLE_CAR_DATA).arg(DB_COL_ID).arg(DB_COL_LICENSE).arg(car_info->id).arg(car_info->license);
    qDebug() << sql_cmd;
    if(sqlquery->exec(sql_cmd))
        qDebug() << "add car ok." ;
    else
        qDebug() << "add car failed!" ;

    sql_car_get_total(sql);

    return 0;
}

int sql_car_del(QSqlDatabase &sql, char *license)
{
    QString sql_cmd;

    sql_cmd = QString("delete from %1 where %2='%3';").arg(TABLE_CAR_DATA).arg(DB_COL_LICENSE).arg(license);
    qDebug() << sql_cmd;
    if(sqlquery->exec(sql_cmd))
        qDebug() << "del car ok." ;
    else
        qDebug() << "del car failed!";

    return 0;
}

int sql_car_get_total(QSqlDatabase &sql)
{
    QString sql_cmd;
    int total = 0;

    sql_cmd = QString("select count(*) from %1").arg(TABLE_CAR_DATA);
    qDebug() << sql_cmd;
    if(sqlquery->exec(sql_cmd) == false)
    {
        qDebug() << "get car total failed!" ;
        return 0;
    }

    sqlquery->next();
    total = sqlquery->value(0).toInt();

    return total;
}

int sql_car_traverse(QSqlDatabase &sql, int *cursor, car_info_t *car_info)
{
    QString sql_cmd;

    if(*cursor == 0)
    {
        sql_cmd = QString("select * from %1").arg(TABLE_CAR_DATA);
        qDebug() << sql_cmd;
        if(sqlquery->exec(sql_cmd) == false)
        {
            qDebug() << "car traverse failed!" ;
            return -1;
        }
        (*cursor) ++;
    }

    if(sqlquery->next())
    {
        memset(car_info, 0, sizeof(car_info_t));
        car_info->id = sqlquery->value(0).toInt();
        strcpy(car_info->license, sqlquery->value(1).toString().toLocal8Bit().data());
    }
    else
    {
        return -1;
    }

    return 0;
}


int sql_create_tbl_car(void)
{
    QString sql_cmd;

    sql_cmd = QString("create table if not exists %1(%2 int primary key not null,%3 char(32));").arg(TABLE_CAR_DATA).arg(DB_COL_ID).arg(DB_COL_LICENSE);
    qDebug() << sql_cmd;
    if(sqlquery->exec(sql_cmd))
        qDebug() << "create car TABLE success." ;
    else
        qDebug() << "create car TABLE failed!" ;

    return 0;
}

int sql_create_tbl_history(void)
{
    QString sql_cmd;

    sql_cmd = QString("create table if not exists %1(%2 int primary key not null,%3 char(32));").arg(TABLE_HISTORY).arg(DB_COL_TIME).arg(DB_COL_LICENSE);
    qDebug() << sql_cmd;
    if(sqlquery->exec(sql_cmd))
        qDebug() << "create history TABLE success." ;
    else
        qDebug() << "create history TABLE failed!" ;

    return 0;
}

int sql_db_init(QSqlDatabase &sql)
{

    sql = QSqlDatabase::addDatabase("QSQLITE");
    sql.setDatabaseName(CAR_DB_NAME);
    if(!sql.open())
    {
        qDebug() << "fail to open sqlite!" ;
    }

    sqlquery = new QSqlQuery(sql);

    sql_create_tbl_car();
    sql_create_tbl_history();

    qDebug() << "sql db init ok." ;

    return 0;
}

